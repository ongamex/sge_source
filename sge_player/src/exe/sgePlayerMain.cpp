#if 1
// This thing makes the driver to auto-choose the high-end GPU instead of the "integrated" one.
#ifdef _WIN32
extern "C" {
__declspec(dllexport) int NvOptimusEnablement = 0x00000001;
}
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "GameMode.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/application.h"
#include "sge_core/setImGuiContexCore.h"
#include "sge_engine/EngineGlobal.h"
#include "sge_engine/GamePlayerSettings.h"
#include "sge_engine/IPlugin.h"
#include "sge_engine/TypeRegister.h"
#include "sge_engine/setImGuiContextEngine.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include "sge_utils/utils/DLLHandler.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/json.h"

#include <filesystem>
#include <thread>

#include "MiniDump.h"

#if defined(__EMSCRIPTEN__)
sge::IPlugin* getInterop();
#endif

using namespace sge;

int g_argc = 0;
char** g_argv = nullptr;

GamePlayerSettings g_playerSettings;

struct SGEGameWindow : public WindowBase {
	GamePlayerSettings playerSets;
	GameMode gameMode;

	Timer m_timer;
	std::string pluginName;
	DLLHandler m_dllHandler;
	IPlugin* m_pluginInst = nullptr;
	IGameDrawer* m_pGameDrawer = nullptr;

	vec2i cachedWindowSize = vec2i(0);

	void HandleEvent(const WindowEvent event, const void* const eventData) override {
		if (event == WE_Create) {
			OnCreate();
		}

		if (event == WE_Resize) {
			const WE_Resize_Data& resizeData = *static_cast<const WE_Resize_Data*>(eventData);
			cachedWindowSize.x = resizeData.width;
			cachedWindowSize.y = resizeData.height;

			SGEDevice* device = getCore()->getDevice();
			if (device) {
				const WE_Resize_Data& data = *(const WE_Resize_Data*)eventData;
				getCore()->getDevice()->resizeBackBuffer(data.width, data.height);
				SGEImGui::setViewport(Rect2s(short(data.width), short(data.height)));
			}
		}

		if (event == WE_Destroying) {
			gameMode.m_sceneInstance.getWorld().clear();

			SGEImGui::destroy();
			SGE_DEBUG_LOG("Destroy Called!");
		}
	}

	void OnCreate() {
		int const backBufferWidth = this->GetClientWidth();
		int const backBufferHeight = this->GetClientHeight();

		MainFrameTargetDesc mainTargetDesc;
		mainTargetDesc.width = backBufferWidth;
		mainTargetDesc.height = backBufferHeight;
		mainTargetDesc.numBuffers = 2;
		mainTargetDesc.vSync = true;
		mainTargetDesc.sampleDesc = SampleDesc(1);
#ifdef _WIN32
		mainTargetDesc.hWindow = GetNativeHandle();
		mainTargetDesc.bWindowed = true;
#endif

		// Obtain the backbuffer rendertarget
		// initialized the device and the immediate context
		SGEDevice* const device = SGEDevice::create(mainTargetDesc);

		SGEImGui::initialize(device->getContext(), device->getWindowFrameTarget(), device->getWindowFrameTarget()->getViewport());

#if !defined(__EMSCRIPTEN__)
		ImGui::SetCurrentContext(getImGuiContextCore());
		setImGuiContextEngine(getImGuiContextCore());
#endif
		ImGui::GetIO().IniFilename = NULL;
		ImGui::GetIO().LogFilename = NULL;

		// Setup Audio device
		AudioDevice* const audioDevice = AudioDevice::create(AudioDeviceDesc{});

		getCore()->setup(device, audioDevice);
		getCore()->getAssetLib()->scanForAvailableAssets("assets");

#if !defined(__EMSCRIPTEN__)
		for (auto const& entry : std::filesystem::directory_iterator("./")) {
			if (std::filesystem::is_regular_file(entry) && entry.path().extension() == ".gll") {
				pluginName = entry.path().string();
			}
		}

		typeLib().performRegistration();
		createAndInitializeEngineGlobal();
		// When we are playing the game (in sge_player) we needs to allow the game
		// to do whatever it wants with the cursor, there is no editor to restrict that.
		getEngineGlobal()->setEngineAllowingRelativeCursor(true);

		if (pluginName.empty()) {
			return;
		} else {
			loadPlugin();
		}
#else
		createAndInitializeEngineGlobal();
		loadPlugin();
#endif

		m_pGameDrawer = m_pluginInst->allocateGameDrawer();



		gameMode.create(m_pGameDrawer, g_playerSettings.initalLevel.c_str());
	}

	void loadPlugin() {
		// Notify that we are about to unload the plugin.
		getEngineGlobal()->notifyOnPluginPreUnload();
#if !defined(__EMSCRIPTEN__)
		// Unload the old plugin DLL and load the new one.
		m_dllHandler.load(pluginName.c_str());

		// Obain the new plugin interface.
		GetInpteropFnPtr interopGetter = reinterpret_cast<GetInpteropFnPtr>(m_dllHandler.getProcAdress("getInterop"));
		sgeAssert(interopGetter != nullptr && "The loaded game dll does not have a getInterop()!\n");
		if (interopGetter) {
			m_pluginInst = interopGetter();
		}
#else
		m_pluginInst = getInterop();
#endif
		m_pluginInst->onLoaded(ImGui::GetCurrentContext(), getCore());
		typeLib().performRegistration();
		getEngineGlobal()->changeActivePlugin(m_pluginInst);
		typeLib().performRegistration();
	}

	void run() {
		m_timer.tick();
		getEngineGlobal()->update(m_timer.diff_seconds());

		if (getEngineGlobal()->getEngineAllowingRelativeCursor() && getEngineGlobal()->doesAnyoneNeedForRelativeCursorThisFrame()) {
			setMouseCursorRelative(true);
		} else {
			setMouseCursorRelative(false);
		}
		getEngineGlobal()->clearAnyoneNeedForRelativeCursorThisFrame();

		float const bgColor[] = {0.f, 0.f, 0.f, 1.f};

		SGEImGui::newFrame(GetInputState());

		SGEContext* const sgecon = getCore()->getDevice()->getContext();
		sgecon->clearColor(getCore()->getDevice()->getWindowFrameTarget(), -1, bgColor);
		sgecon->clearDepth(getCore()->getDevice()->getWindowFrameTarget(), 1.f);

		if (m_pluginInst) {
			m_pluginInst->run();
		}

		gameMode.update(GetInputState());
		RenderDestination rdest = RenderDestination(sgecon, sgecon->getDevice()->getWindowFrameTarget());
		gameMode.draw(rdest);

		// Render the ImGui User Interface.
		SGEImGui::render();

		// Finally display everyting to the screen.
		getCore()->setLastFrameStatistics(getCore()->getDevice()->getFrameStatistics());
		getCore()->getDevice()->present();

		return;
	}
};

#ifdef __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif



#ifdef __EMSCRIPTEN__
EM_BOOL emsc_canvasResizeCb(int eventType, const void* reserved, void* userData) {
	double width, height;
	emscripten_get_element_css_size("canvas", &width, &height);

	int w = (int)width, h = (int)height;

	printf("resized to %fx%f\n", width, height);

	if (userData) {
		SGEGameWindow* wnd = reinterpret_cast<SGEGameWindow*>(userData);
		wnd->resizeWindow(w, h);
	}

	return 0;
}
#endif

void main_loop() {
	sge::ApplicationHandler::get()->PollEvents();
	for (WindowBase* wnd : sge::ApplicationHandler::get()->getAllWindows()) {
		SGEGameWindow* gameWindow = dynamic_cast<SGEGameWindow*>(wnd);
		if (gameWindow) {
			gameWindow->run();
		}
	}
}

// Caution:
// SDL2 might have a macro (depending on the target platform) for the main function!
int main(int argc, char* argv[]) {
	SGE_DEBUG_LOG("main()\n");

	sgeRegisterMiniDumpHandler();
	setlocale(LC_NUMERIC, "C");

	g_argc = argc;
	g_argv = argv;

	// float ddpi = 0.f;
	// SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr);

#ifdef __EMSCRIPTEN__
	// Emscripten file system reassembles default Unix installation with the /home, /dev and all the other dirs.
	// Placing the game in the root "/" doesn't seems to work for some reason this is why when we embed files in
	// emscrpiten we add them to the path below. Make that path current as the game is expecting to be in that path.
	std::filesystem::current_path("/home/game");

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#else
	SDL_Init(SDL_INIT_EVERYTHING);
#ifdef SGE_RENDERER_GL
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	// SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
#endif

	if (!g_playerSettings.loadFromJsonFile("appdata/game_project_settings.json")) {
		DialongOk("Error", "appData/game_project_settings.json seems to be missing or invalid! The game will not start!");
		return 0;
	}

	vec2i windowSize(g_playerSettings.windowWidth, g_playerSettings.windowHeight);

	JsonParser jp;
	[[maybe_unused]] SGEGameWindow* wnd = sge::ApplicationHandler::get()->NewWindow<SGEGameWindow>(
	    "SGEEngine Game Player", windowSize.x, windowSize.y, false, !g_playerSettings.windowIsResizable);

#ifdef __EMSCRIPTEN__
	EmscriptenFullscreenStrategy fullscreenStrategy = {};
	fullscreenStrategy.scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT;
	fullscreenStrategy.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE;
	fullscreenStrategy.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
	fullscreenStrategy.canvasResizedCallback = emsc_canvasResizeCb;
	fullscreenStrategy.canvasResizedCallbackUserData = wnd;

	emscripten_request_fullscreen_strategy("#canvas", false, &fullscreenStrategy);
#endif

	// Main loop in web builds cannot run in an infinite loop as the browser JavaScript thread would freeze.
	// instead we specify a function to emscripten that will get called instead as a main loop.
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(main_loop, 0, true);
#else
	while (sge::ApplicationHandler::get()->shouldStopRunning() == false) {
		main_loop();
	};
#endif

	return 0;
}
