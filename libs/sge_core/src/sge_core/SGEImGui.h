#pragma once

#include "sge_renderer/renderer/renderer.h"
#include "sgecore_api.h"

#include "application/input.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <sge_utils/utils/timer.h>

namespace sge {

class SGE_CORE_API ISgeImGuiStateWrapper {
	ISgeImGuiStateWrapper() = default;
	void* ptr = nullptr;
	void (*deleter)(void*) = nullptr;
};

struct FrameTargetD3D11;
struct TextureImpl;

struct transf3d;

//--------------------------------------------------------------------
// ImGui rendering implementation using SGE API.
//--------------------------------------------------------------------
struct SGE_CORE_API SGEImGui {
  public:
	static void initialize(SGEContext* con, FrameTarget* frameTarget, const Rect2s& viewport_arg);
	static void destroy();

	static void newFrame(const InputState& inputState);
	static void render();
	static void setViewport(const Rect2s& viewport_arg);

	static void renderDrawLists(ImDrawData* imDrawData);

	static bool IsItemActiveLastFrame();
	static bool IsItemJustReleased();
	static bool IsItemJustActivated();

	static bool DragFloats(const char* label,
	                       float* floats,
	                       int numFloats,
	                       bool* pJustReleased = nullptr,
	                       bool* pJustActivated = nullptr,
	                       float middleClickResetValue = 0.f,
	                       float v_speed = 1.0f,
	                       float v_min = 0.0f,
	                       float v_max = 0.0f,
	                       const char* display_format = "%.3f",
	                       float power = 1.0f);
	static bool DragInts(const char* label,
	                     int* ints,
	                     int numInts,
	                     bool* pJustReleased = nullptr,
	                     bool* pJustActivated = nullptr,
	                     float v_speed = 1.f,
	                     int v_min = 0.0f,
	                     int v_max = 0.0f,
	                     const char* display_format = "%.0f");
	static bool ColorPicker4(const char* label,
	                         float col[4],
	                         bool* const pJustReleased,
	                         bool* const pJustActivated,
	                         ImGuiColorEditFlags flags = 0,
	                         const float* ref_col = nullptr);

	static bool ColorPicker3(const char* label,
	                         float col[3],
	                         bool* const pJustReleased,
	                         bool* const pJustActivated,
	                         ImGuiColorEditFlags flags = 0,
	                         const float* ref_col = nullptr);

  private:
	static StateGroup stateGroup;

	static SGEContext* sgecon;
	static Timer timer;

	static GpuHandle<Buffer> vertBuffer;
	static GpuHandle<Buffer> idxBuffer;
	static GpuHandle<RasterizerState> rasterizerState;
	static GpuHandle<DepthStencilState> depthStencilState;
	static GpuHandle<BlendState> blendState;
	static GpuHandle<Texture> font_texture;
	static GpuHandle<FrameTarget> frame_target;
	static Rect2s viewport;
	static VertexDeclIndex m_imgui_vertexDeclIdx;

	static BindLocation colorTexBindLoc;
	static BindLocation colorTexSamplerBindLoc;
	static BindLocation projViewWorldBindLoc;

	static GpuHandle<ShadingProgram> shadingProgram;
};

//--------------------------------------------------------------
//
//--------------------------------------------------------------
struct SGE_CORE_API UIState {
	void* storage = nullptr;
	void (*deleter)(void*) = nullptr;

	~UIState() {
		sgeAssert((storage != nullptr) == (deleter != nullptr) && "These are expected to both be present or not!");
		if (deleter && storage) {
			deleter(storage);
			storage = nullptr;
		}
	}
};

UIState* getUIStateInternal(const ImGuiID id, void* (*newer)(), void (*deleter)(void*));

template <typename T>
T& getUIState() {
	const ImGuiID id = ImGui::GetCurrentWindow()->IDStack.back();
	UIState* res = getUIStateInternal(
	    id, []() -> void* { return new T; }, [](void* p) { delete p; });

	sgeAssert(res != nullptr && res->storage != nullptr && res->deleter != nullptr);
	T* const resObj = reinterpret_cast<T*>(res->storage);
	return *resObj;
}

inline ImVec2 toImGui(const vec2f& v) {
	return ImVec2(v.x, v.y);
}

inline vec2f fromImGui(const ImVec2& v) {
	return vec2f(v.x, v.y);
}

inline ImVec4 toImGui(const vec4f& v) {
	return ImVec4(v.x, v.y, v.z, v.w);
}

inline vec4f fromImGui(const ImVec4& v) {
	return vec4f(v.x, v.y, v.z, v.w);
}

} // namespace sge

namespace ImGuiEx {

struct IDGuard {
	[[nodiscard]] explicit IDGuard(ImGuiID id) { ImGui::PushID(id); }
	[[nodiscard]] explicit IDGuard(const void* ptr) { ImGui::PushID(ptr); }
	[[nodiscard]] explicit IDGuard(const char* const cString) { ImGui::PushID(cString); }

	~IDGuard() { ImGui::PopID(); }

	// Disable copy:
	IDGuard(const IDGuard&) = delete;
	IDGuard& operator=(const IDGuard&) = delete;

	// Disable move:
	IDGuard(IDGuard&&) = delete;
	IDGuard& operator=(IDGuard&&) = delete;
};

SGE_CORE_API void BeginGroupPanel(const char* name, const ImVec2 size = ImVec2(-1.f, -1.f));
SGE_CORE_API void EndGroupPanel();
SGE_CORE_API void Label(const char* label, bool shouldSetNextItemWidth = true, float labelWidthProportion = 0.3f);
SGE_CORE_API bool InputText(const char* label,
                            std::string& str,
                            ImGuiInputTextFlags flags = 0,
                            ImGuiInputTextCallback callback = nullptr,
                            void* user_data = nullptr,
                            bool acceptOnlyIdentifierStyleText = false);

/// @brief Create a tooltip if the previous item was hovered.
SGE_CORE_API void TextTooltip(const char* const text);

/// @brief Create a tooltip if the previous item was hovered. The tooltip shows
/// only after the item was hovered for the @delay amout of seconds.
/// Caution: See https://github.com/ocornut/imgui/issues/1485
/// This function works only on some items (like buttons).
SGE_CORE_API void TextTooltipDelayed(const char* const text, float delay = 1.f);
} // namespace ImGuiEx
