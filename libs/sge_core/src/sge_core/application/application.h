#pragma once

#include "input.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/sge_utils.h"
#include <map>
#include <string>
#include <vector>

namespace sge {

//-----------------------------------------------
// Events and Event data.
//-----------------------------------------------
/// An enum window messages.
enum WindowEvent : int {
	WE_Create,
	WE_Destroying,
	WE_Resize,
	WE_FileDrop,
};

/// @brief The message data of the WE_Resize event.
struct WE_Resize_Data {
	WE_Resize_Data() = default;

	WE_Resize_Data(int width, int height)
	    : width(width)
	    , height(height) {}

	int width;
	int height;
};

/// @brief The message data of the WE_FileDrop event.
struct WE_FileDrop_Data {
	std::string filename;
};

struct WindowBase;
struct WindowImplData;

/// @brief Application handler manages all opened windows.
struct SGE_CORE_API ApplicationHandler {
  private:
	ApplicationHandler() = default;

  public:
	~ApplicationHandler() = default;

	static ApplicationHandler* get() {
		static ApplicationHandler inst;
		return &inst;
	}

	// Polls and dispatches recieved events to the respective windows.
	void PollEvents();

	template <typename T>
	T* NewWindow(const char* windowName, int width, int height, bool isMaximized, bool noResize = false) {
		T* wnd = new T;
		NewWindowInternal(wnd, windowName, width, height, isMaximized, noResize);
		return wnd;
	}

	void DeregisterWindowInternal(WindowBase* wnd);
	bool HasAliveWindows() const { return m_wnds.size() != 0; }
	const std::vector<WindowBase*>& getAllWindows() { return m_wnds; }
	bool shouldStopRunning() const { return m_isAppQuitRequested || HasAliveWindows() == false; }

  private:
	WindowBase* findWindowBySDLId(const uint32 id);
	void removeWindow(WindowBase* const wndToRemove);
	void NewWindowInternal(WindowBase* window, const char* windowName, int width, int height, bool isMaximized, bool noResize);

  private:
	bool m_isAppQuitRequested = false;
	std::vector<WindowBase*> m_wnds;

	std::map<int, int> sdlJoystickInstanceIdIdToIndex;

	// To be honest having a global input state makes a lot of sense, however,
	// I'm not sure if this is the corrent approch as there is going to be a main window
	// sooo. I'm not going to implement that for now.
	InputState m_inputState;
};

/// @brief An instance of a native window.
struct SGE_CORE_API WindowBase {
	WindowBase();
	virtual ~WindowBase();
	virtual void HandleEvent(const WindowEvent UNUSED(event), const void* const UNUSED(eventData)){};

	WindowBase(WindowBase&) = delete;
	WindowBase& operator=(WindowBase&) = delete;

	void* GetNativeHandle() const;
	bool IsActive() const;
	const InputState& GetInputState() { return m_inputState; }

	void resizeWindow(int width, int height);
	int GetClientWidth() const;
	int GetClientHeight() const;
	bool isMaximized() const;
	vec2f getClientSizef() const { return vec2f((float)GetClientWidth(), (float)GetClientHeight()); }
	void setWindowTitle(const char* title);

  public:
	InputState m_inputState;
	WindowImplData* m_implData;
};

/// @brief Enables/Disables the mouse being captured.
/// Useful for camera controls like FPS.
/// If you are using the SGEEditor with GameWorld, do not call this directly,
/// instead use GameWorld::setNeedsLockedCursor() inside your game logic.
/// The reason is that the editor needs to be able to toggle between allowing
/// cursor capturing or not.
/// Search for [SGE_EDITOR_MOUSE_CAPTURE_HOTKEY] for more details.
/// @param isRelative if true the mouse will be invisible and will not go ouside the current window.
SGE_CORE_API void setMouseCursorRelative(bool isRelative);

/// @brief Checks if the mouse if currently captured or not.
/// If you have access to InputState, you can use InputState::isCursorRelative for gameplay.
SGE_CORE_API bool getMouseCursorRelative();

} // namespace sge
