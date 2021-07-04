#pragma once

#include <string>

#include "IImGuiWindow.h"
#include "imgui/imgui.h"

namespace sge {

struct InputState;
struct GameInspector;
struct GameObject;
struct MemberChain;

struct CollsionShapeDesc;

enum class AssetType : int;

/// A set of functions that generate the User Interface for the specified member of game object.
/// The functions could be called anywhere.
namespace ProperyEditorUIGen {
	SGE_ENGINE_API void doGameObjectUI(GameInspector& inspector, GameObject* const gameObject);
	SGE_ENGINE_API void doMemberUI(GameInspector& inspector, GameObject* const gameObject, MemberChain chain);
	SGE_ENGINE_API void editFloat(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editInt(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editString(GameInspector& inspector, const char* label, GameObject* gameObject, MemberChain chain);
	SGE_ENGINE_API void editStringAsAssetPath(GameInspector& inspector,
	                                          const char* label,
	                                          GameObject* gameObject,
	                                          MemberChain chain,
	                                          const AssetType possibleAssetTypes[],
	                                          const int numPossibleAssetTypes);

	SGE_ENGINE_API void editDynamicProperties(GameInspector& inspector, GameObject* gameObject, MemberChain chain);
} // namespace ProperyEditorUIGen

//----------------------------------------------------------
// PropertyEditorWindow
//----------------------------------------------------------
struct SGE_ENGINE_API PropertyEditorWindow : public IImGuiWindow {
	PropertyEditorWindow(std::string windowName, GameInspector& inspector)
	    : m_windowName(std::move(windowName))
	    , m_inspector(inspector) {}

	bool isClosed() override { return !m_isOpened; }
	void update(SGEContext* const sgecon, const InputState& is) override;
	const char* getWindowName() const override { return m_windowName.c_str(); }


  private:
	char m_outlinerFilter[512] = {'*', '\0'};

	bool m_isOpened = true;
	GameInspector& m_inspector;
	std::string m_windowName;

	bool m_showLogicTransformInLocalSpace = false;
};


} // namespace sge
