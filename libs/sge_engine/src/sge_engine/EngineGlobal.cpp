#include "EngineGlobal.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_engine/windows/EditorWindow.h"

namespace sge {

/// EditorNotification describes a message and its state that should be diplayed somehow to the user.
struct EditorGUINotification {
	EditorGUINotification() = default;

	EditorGUINotification(std::string text)
	    : text(std::move(text)) {}

	// The message of this notification.
	std::string text;

	// The amount of time that the notification was visible in the window.
	float timeDisplayed = 0.f;
};

//--------------------------------------------------------------------------
// EngineGlobalAssets
//--------------------------------------------------------------------------
void EngineGlobalAssets::initialize() {
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	unknownObjectIcon = assetLib->getAsset(AssetType::Texture2D, "assets/editor/textures/icons/obj/UnknownObject.png", true);

	sgeAssert(typeLib().m_gameObjectTypes.empty() == false && "Aren't you calling this too early?");

	for (const auto& typeId : typeLib().m_gameObjectTypes) {
		const TypeDesc* td = typeLib().find(typeId);
		std::string assetName = "assets/editor/textures/icons/obj/" + std::string(td->name) + ".png";
		std::shared_ptr<Asset> asset = getCore()->getAssetLib()->getAsset(AssetType::Texture2D, assetName.c_str(), true);
		if (isAssetLoaded(asset)) {
			perObjectTypeIcon[typeId] = asset;
		}
	}
}

std::shared_ptr<Asset> EngineGlobalAssets::getIconForObjectType(const TypeId type) {
	auto itr = perObjectTypeIcon.find(type);

	if (itr != perObjectTypeIcon.end()) {
		return itr->second;
	}

	return unknownObjectIcon;
}

//--------------------------------------------------------------------------
// EngineGlobal
//--------------------------------------------------------------------------
struct EngineGlobal final : public IEngineGlobal {
	EngineGlobal() = default;
	~EngineGlobal() { [[maybe_unused]] int x = 10; }

	void initialize() override;
	void update(float dt) override;

	void changeActivePlugin(IPlugin* pPlugin) override;
	IPlugin* getActivePlugin() override;
	void notifyOnPluginPreUnload() override;
	EventSubscription subscribeOnPluginChange(std::function<void()> fn) override;
	EventSubscription subscribeOnPluginPreUnload(std::function<void()> fn) override;

	void addPropertyEditorIUGeneratorForType(TypeId type, PropertyEditorGeneratorForTypeFn function) override;
	PropertyEditorGeneratorForTypeFn getPropertyEditorIUGeneratorForType(TypeId type) override;

	// UI windows.
	void addWindow(IImGuiWindow* window) override;
	void removeWindow(IImGuiWindow* window) override;

	EditorWindow* getEditorWindow() override;
	std::vector<std::unique_ptr<IImGuiWindow>>& getAllWindows() override;
	IImGuiWindow* findWindowByName(const char* const name) override;

	// UI Notifications.
	void showNotification(std::string text) override;
	const char* getNotificationText(const int iNotification) const override;
	int getNotificationCount() const override;

	// Assets for the engine.
	EngineGlobalAssets& getEngineAssets() override;

	void setEngineAllowingRelativeCursor(bool isRelativeCursorAllowed) override;
	bool getEngineAllowingRelativeCursor() const override;

	void setNeedForRelativeCursorThisFrame() override;
	bool doesAnyoneNeedForRelativeCursorThisFrame() const override;
	void clearAnyoneNeedForRelativeCursorThisFrame() override;

  private:
	std::unordered_map<TypeId, PropertyEditorGeneratorForTypeFn> m_propertyEditorUIGenFuncs;

	// A cached value from m_allActiveWindows, which hold the main EditorWindow.
	// There should be only one instance of this window;
	EditorWindow* m_editorWindow;
	std::vector<std::unique_ptr<IImGuiWindow>> m_allActiveWindows;

	EngineGlobalAssets m_globalAssets;

	std::vector<EditorGUINotification> m_notifications;

	IPlugin* m_activePlugin = nullptr;
	EventEmitter<> m_onPluginPreUnloadEvent;
	EventEmitter<> m_onPluginChangeEvent;

	bool m_isEngineAllowingRelativeCursor = false;
	bool m_doesAnythingNeedsRelativeCursorThisFrame = false;
};

void EngineGlobal::addPropertyEditorIUGeneratorForType(TypeId type, PropertyEditorGeneratorForTypeFn function) {
	m_propertyEditorUIGenFuncs[type] = function;
}

PropertyEditorGeneratorForTypeFn EngineGlobal::getPropertyEditorIUGeneratorForType(TypeId type) {
	return m_propertyEditorUIGenFuncs[type];
}

void EngineGlobal::initialize() {
	m_globalAssets.initialize();
}

void EngineGlobal::update(float dt) {
	// Delete expiered notification messages.
	for (int t = 0; t < int(m_notifications.size()); ++t) {
		m_notifications[t].timeDisplayed += dt;

		const float kNotificationLifetime = 2.f;
		if (m_notifications[t].timeDisplayed > kNotificationLifetime) {
			m_notifications.erase(m_notifications.begin() + t);
			--t;
			continue;
		}
	}
}

void EngineGlobal::changeActivePlugin(IPlugin* pPlugin) {
	m_activePlugin = pPlugin;
	m_propertyEditorUIGenFuncs.clear();

	for (auto& fn : getPluginRegisterFunctions()) {
		if (fn) {
			fn();
		}
	}

	m_onPluginChangeEvent.invokeEvent();
}

IPlugin* EngineGlobal::getActivePlugin() {
	return m_activePlugin;
}

void EngineGlobal::notifyOnPluginPreUnload() {
	m_onPluginPreUnloadEvent.invokeEvent();
}

EventSubscription EngineGlobal::subscribeOnPluginChange(std::function<void()> fn) {
	return m_onPluginChangeEvent.subscribe(std::move(fn));
}

EventSubscription EngineGlobal::subscribeOnPluginPreUnload(std::function<void()> fn) {
	return m_onPluginPreUnloadEvent.subscribe(std::move(fn));
}

void EngineGlobal::addWindow(IImGuiWindow* window) {
	if (window == nullptr) {
		sgeAssert(false);
		return;
	}

	EditorWindow* const newWindowAsEditorWnd = dynamic_cast<EditorWindow*>(window);
	if (newWindowAsEditorWnd != nullptr) {
		if (m_editorWindow == nullptr) {
			m_editorWindow = newWindowAsEditorWnd;
		} else {
			sgeAssert(false && "It is expected that there will be only one EditorWindow");
		}
	} else {
		m_allActiveWindows.emplace_back(window);
	}
}

void EngineGlobal::removeWindow(IImGuiWindow* window) {
	for (int t = 0; t < int(m_allActiveWindows.size()); ++t) {
		if (m_allActiveWindows[t].get() == window) {
			m_allActiveWindows.erase(m_allActiveWindows.begin() + t);
			return;
		}
	}

	sgeAssert(false && "Trying to remove a window that hasn't been added to the engineGlobal list");
}

EditorWindow* EngineGlobal::getEditorWindow() {
	return m_editorWindow;
}

std::vector<std::unique_ptr<IImGuiWindow>>& EngineGlobal::getAllWindows() {
	return m_allActiveWindows;
}

IImGuiWindow* EngineGlobal::findWindowByName(const char* const name) {
	if (name == nullptr) {
		return nullptr;
	}

	for (std::unique_ptr<IImGuiWindow>& wnd : m_allActiveWindows) {
		if (wnd && strcmp(wnd->getWindowName(), name) == 0) {
			return wnd.get();
		}
	}

	return nullptr;
}

void EngineGlobal::showNotification(std::string text) {
	m_notifications.emplace_back(EditorGUINotification(std::move(text)));
}

const char* EngineGlobal::getNotificationText(const int iNotification) const {
	if (iNotification < 0 || iNotification >= m_notifications.size()) {
		return nullptr;
	}

	return m_notifications[iNotification].text.c_str();
}

int EngineGlobal::getNotificationCount() const {
	return int(m_notifications.size());
}

EngineGlobalAssets& EngineGlobal::getEngineAssets() {
	return m_globalAssets;
}

void EngineGlobal::setEngineAllowingRelativeCursor(bool isRelativeCursorAllowed) {
	m_isEngineAllowingRelativeCursor = isRelativeCursorAllowed;
}

bool EngineGlobal::getEngineAllowingRelativeCursor() const {
	return m_isEngineAllowingRelativeCursor;
}

void EngineGlobal::setNeedForRelativeCursorThisFrame() {
	m_doesAnythingNeedsRelativeCursorThisFrame |= true;
}

bool EngineGlobal::doesAnyoneNeedForRelativeCursorThisFrame() const {
	return m_doesAnythingNeedsRelativeCursorThisFrame;
}

void EngineGlobal::clearAnyoneNeedForRelativeCursorThisFrame() {
	m_doesAnythingNeedsRelativeCursorThisFrame = false;
}

//------------------------------------------------------------------
//
//------------------------------------------------------------------
IEngineGlobal* g_worklingEngineGlobal = nullptr;
std::vector<void (*)()> g_pluginRegisterFunctionsToCall;

IEngineGlobal* createAndInitializeEngineGlobal() {
	if (g_worklingEngineGlobal == nullptr) {
		g_worklingEngineGlobal = new EngineGlobal();
		g_worklingEngineGlobal->initialize();
	}
	return g_worklingEngineGlobal;
}

IEngineGlobal* getEngineGlobal() {
	return g_worklingEngineGlobal;
}

void setEngineGlobal(IEngineGlobal* global) {
	g_worklingEngineGlobal = global;
}

const std::vector<void (*)()>& getPluginRegisterFunctions() {
	return g_pluginRegisterFunctionsToCall;
}

int addPluginRegisterFunction(void (*fnPtr)()) {
	g_pluginRegisterFunctionsToCall.push_back(fnPtr);
	return 0;
}


} // namespace sge
