#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "sge_engine/IPlugin.h"
#include "sge_engine/sge_engine_api.h"
#include "sge_engine/typelibHelper.h"
#include "sge_engine/windows/IImGuiWindow.h"
#include "sge_utils/utils/Event.h"

namespace sge {
struct Asset;
struct EditorWindow;

struct GameWorld;
struct GameInspector;
struct GameObject;
struct MemberChain;

using PropertyEditorGeneratorForTypeFn = void (*)(GameInspector& inspector, GameObject* actor, MemberChain chain);


/// EngineGlobalAssets holds assets that are used by the interface and across multiple GameWorlds.
struct SGE_ENGINE_API EngineGlobalAssets {
	void initialize();
	std::shared_ptr<Asset> getIconForObjectType(const TypeId type);

  private:
	std::shared_ptr<Asset> unknownObjectIcon;
	std::unordered_map<TypeId, std::shared_ptr<Asset>> perObjectTypeIcon;
};

struct IEngineGlobal {
	IEngineGlobal() = default;
	virtual ~IEngineGlobal() = default;

	virtual void initialize() = 0;
	virtual void update(float dt) = 0;

	virtual void changeActivePlugin(IPlugin* pPlugin) = 0;
	virtual IPlugin* getActivePlugin() = 0;
	virtual void notifyOnPluginPreUnload() = 0;
	virtual EventSubscription subscribeOnPluginChange(std::function<void()> fn) = 0;
	virtual EventSubscription subscribeOnPluginPreUnload(std::function<void()> fn) = 0;

	///
	virtual void addPropertyEditorIUGeneratorForType(TypeId type, PropertyEditorGeneratorForTypeFn function) = 0;
	virtual PropertyEditorGeneratorForTypeFn getPropertyEditorIUGeneratorForType(TypeId type) = 0;

	/// Adds a root window to be managed by the engine. Usually these hold tools.
	virtual void addWindow(IImGuiWindow* window) = 0;

	/// Closes and removes the specified window.
	virtual void removeWindow(IImGuiWindow* window) = 0;

	/// Retrieves the main editor window for the application. This window maanges all other windows.
	/// There must be only one such window at any time.
	virtual EditorWindow* getEditorWindow() = 0;

	/// Retrieves all existing windows EXCEPT the EditorWindow.
	virtual std::vector<std::unique_ptr<IImGuiWindow>>& getAllWindows() = 0;

	/// @brief Finds the first window with the specified name.
	virtual IImGuiWindow* findWindowByName(const char* const name) = 0;

	template <typename T>
	T* findFirstWindowOfType() {
		for (auto& wnd : getAllWindows()) {
			T* const wndTyped = dynamic_cast<T*>(wnd.get());
			if (wndTyped != nullptr) {
				return wndTyped;
			}
		}

		return nullptr;
	}

	virtual void showNotification(std::string text) = 0;
	virtual const char* getNotificationText(const int iNotification) const = 0;
	virtual int getNotificationCount() const = 0;

	virtual EngineGlobalAssets& getEngineAssets() = 0;

	/// @brief  When creating a game you do not need to modify this. This is intended
	/// for the editor to restrict the usage of a relative cursor (a hidden cursor useful for FPS like games).
	/// This function sets if relative cursors are allowed or not. They migth not be allowed when we are
	/// in the editor editing the game.
	/// If you want to use relative cursor in the game use @setNeedForRelativeCursorThisFrame.
	virtual void setEngineAllowingRelativeCursor(bool isRelativeCursorAllowed) = 0;

	/// @brief Returns true if relative cursor is allowed, not that it is relative.
	virtual bool getEngineAllowingRelativeCursor() const = 0;

	virtual void setNeedForRelativeCursorThisFrame() = 0;
	virtual bool doesAnyoneNeedForRelativeCursorThisFrame() const = 0;
	virtual void clearAnyoneNeedForRelativeCursorThisFrame() = 0;
};

SGE_ENGINE_API int addPluginRegisterFunction(void (*fnPtr)());
SGE_ENGINE_API const std::vector<void (*)()>& getPluginRegisterFunctions();

SGE_ENGINE_API IEngineGlobal* createAndInitializeEngineGlobal();
SGE_ENGINE_API IEngineGlobal* getEngineGlobal();
SGE_ENGINE_API void setEngineGlobal(IEngineGlobal* global);

#define SgePluginOnLoad() PluginRegisterBlock_Impl(SGE_ANONYMOUS(_SGE_ENGINE_BLOCK_ANON__FUNC))

#define PluginRegisterBlock_Impl(fnName)                                                   \
	static void fnName();                                                                  \
	static int SGE_ANONYMOUS(fnNameRegisterVariable) = addPluginRegisterFunction(&fnName); \
	static void fnName()

} // namespace sge
