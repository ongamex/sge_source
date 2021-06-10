#define IMGUI_DEFINE_MATH_OPERATORS

#include "IconsForkAwesome/IconsForkAwesome.h"
#include "application/application.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/StaticArray.h"


#include "SGEImGui.h"
#include <imgui/imgui_internal.h>

namespace sge {

inline void Style() {
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	/// 0 = FLAT APPEARENCE
	/// 1 = MORE "3D" LOOK
	int is3D = 1;

	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.62f, 0.62f, 0.62f, 0.71f);
	colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.42f, 0.42f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.67f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.17f, 0.17f, 0.17f, 0.90f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.335f, 0.335f, 0.335f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.54f, 0.54f, 0.54f, 0.35f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.52f, 0.52f, 0.52f, 0.59f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.77f);
	colors[ImGuiCol_Separator] = ImVec4(0.000f, 0.000f, 0.000f, 0.137f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.700f, 0.671f, 0.600f, 0.290f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.702f, 0.671f, 0.600f, 0.674f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.73f, 0.73f, 0.73f, 0.35f);
	// colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	// colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	// colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	// colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

	style.PopupRounding = 3;

	style.WindowPadding = ImVec2(4, 4);
	style.FramePadding = ImVec2(6, 4);
	style.ItemSpacing = ImVec2(6, 2);

	style.ScrollbarSize = 18;

	style.WindowBorderSize = 1;
	style.ChildBorderSize = 1;
	style.PopupBorderSize = 1;
	style.FrameBorderSize = float(is3D);

	style.WindowRounding = 3;
	style.ChildRounding = 3;
	style.FrameRounding = 3;
	style.ScrollbarRounding = 2;
	style.GrabRounding = 3;

#ifdef IMGUI_HAS_DOCK
	style.TabBorderSize = float(is3D);
	style.TabRounding = 3;

	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
	colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}
#endif

	// ImGui::StyleColorsLight(&style);
	// ImGui::StyleColorsDark(&style);
}

namespace {
	// ImGui vertex uses int for a color, currently sge doesn't suppoty int_unorm as
	// as a vertex attribute, and this is why we have to convert the input vertices.
	struct SGEImGuiVertex {
		vec2f position;
		vec2f uv;
		int color;
	};

	VertexDecl vertDecl[3] = {
	    VertexDecl(0, "a_position", UniformType::Float2, 0),
	    VertexDecl(0, "a_uv", UniformType::Float2, 8),
	    VertexDecl(0, "a_color", UniformType::Int_RGBA_Unorm_IA, 16),
	};

	const char imDrawShaders[] = R"(
uniform float4x4 projViewWorld;
uniform sampler2D colorTex;

struct VERTEX_IN {
	float2 a_position : a_position;
	float2 a_uv : a_uv;
	float4 a_color : a_color;
};

struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
	float2 v_uv : v_uv;
	float4 v_color : v_color;
};

//----------------------------------------
//
//----------------------------------------
VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

	OUT.v_uv = IN.a_uv;
	OUT.v_color = IN.a_color;
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 0.0, 1.0));

	return OUT;
}

//----------------------------------------
//
//----------------------------------------
float4 psMain(VERTEX_OUT OUT) : COLOR {
	return tex2D(colorTex, OUT.v_uv) * OUT.v_color;
}
)";

} // namespace

SGEContext* SGEImGui::sgecon;
Timer SGEImGui::timer;
GpuHandle<Buffer> SGEImGui::vertBuffer;
GpuHandle<Buffer> SGEImGui::idxBuffer;

GpuHandle<RasterizerState> SGEImGui::rasterizerState;
GpuHandle<DepthStencilState> SGEImGui::depthStencilState;
GpuHandle<BlendState> SGEImGui::blendState;
StateGroup SGEImGui::stateGroup;
GpuHandle<Texture> SGEImGui::font_texture;

GpuHandle<FrameTarget> SGEImGui::frame_target;
Rect2s SGEImGui::viewport;
VertexDeclIndex SGEImGui::m_imgui_vertexDeclIdx = VertexDeclIndex_Null;

BindLocation SGEImGui::colorTexBindLoc;
BindLocation SGEImGui::colorTexSamplerBindLoc;
BindLocation SGEImGui::projViewWorldBindLoc;

GpuHandle<sge::ShadingProgram> SGEImGui::shadingProgram;

//--------------------------------------------------------------------
// struct SGEImGui
//--------------------------------------------------------------------
void SGEImGui::initialize(SGEContext* sgecon_arg, FrameTarget* frameTarget, const Rect2s& viewport_arg) {
	// Create the texture.
	ImGuiContext* imContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(imContext);
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// io.Fonts->AddFontDefault();

	{ io.Fonts->AddFontFromFileTTF("assets/editor/fonts/UbuntuMono-Regular.ttf", 32.0f); }

	// merge in icons from Font Awesome
	{
		const ImWchar icons_ranges[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		io.Fonts->AddFontFromFileTTF("assets/editor/fonts/" FONT_ICON_FILE_NAME_FK, 32.0f, &icons_config, icons_ranges);
	}

	io.FontGlobalScale = 0.5f;

	// use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

	sgecon = sgecon_arg;
	frame_target = frameTarget;
	viewport = viewport_arg;

	shadingProgram = sgecon->getDevice()->requestResource<ShadingProgram>();
	shadingProgram->create(imDrawShaders, imDrawShaders);

	RasterDesc rasterDesc;
	rasterDesc.cullMode = CullMode::None;
	rasterDesc.useScissor = true;

	rasterizerState = sgecon->getDevice()->requestRasterizerState(rasterDesc);

	DepthStencilDesc depthDesc;
	depthDesc.comparisonFunc = DepthComparisonFunc::LessEqual;
	depthStencilState = sgecon->getDevice()->requestDepthStencilState(depthDesc);

	blendState = sgecon->getDevice()->requestBlendState(BlendStateDesc::GetDefaultBackToFrontAlpha());

	int width, height;
	unsigned char* imgui_font_data;
	io.Fonts->GetTexDataAsRGBA32(&imgui_font_data, &width, &height);

	TextureDesc font_tex_desc;
	font_tex_desc.textureType = UniformType::Texture2D;
	font_tex_desc.format = TextureFormat::R8G8B8A8_UNORM;
	font_tex_desc.usage = TextureUsage::ImmutableResource;
	font_tex_desc.texture2D = Texture2DDesc(width, height);

	TextureData font_tex_data(imgui_font_data, width * 4);
	font_texture = sgecon->getDevice()->requestResource<Texture>();
	SamplerDesc fontSamplerDesc;
	fontSamplerDesc.filter = TextureFilter::Min_Mag_Mip_Point;
	font_texture->create(font_tex_desc, &font_tex_data, fontSamplerDesc);

	ImGui::GetIO().Fonts[0].SetTexID(font_texture);

	// io.IniSavingRate = FLT_MAX;
	io.IniFilename = "imgui_layout_cache.ini";
	io.LogFilename = NULL;

	// io.MouseDrawCursor = true;
	io.DisplaySize = ImVec2(viewport.width, viewport.height);

	m_imgui_vertexDeclIdx = sgecon->getDevice()->getVertexDeclIndex(vertDecl, SGE_ARRSZ(vertDecl));
	sgeAssert(m_imgui_vertexDeclIdx != VertexDeclIndex_Null);

	// Set the mapping to ImGui default mapping.
	// sgeAssert(ImGuiKey_Tab == 0); // When implemented ImGuiKey_Tab was the 1st ImGuiKey, update the code if that changes(or just start
	// from zero...)
	for (int t = 0; t < ImGuiKey_COUNT; ++t) {
		io.KeyMap[t] = t;
	}

	colorTexBindLoc = shadingProgram->getReflection().findUniform("colorTex", ShaderType::PixelShader);
	colorTexSamplerBindLoc = shadingProgram->getReflection().findUniform("colorTex_sampler", ShaderType::PixelShader);
	projViewWorldBindLoc = shadingProgram->getReflection().findUniform("projViewWorld", ShaderType::VertexShader);

	// ImGui::GetStyle().ScaleAllSizes(1.25f);s
	Style();
}

void SGEImGui::destroy() {
	vertBuffer.Release();
	idxBuffer.Release();
	rasterizerState.Release();
	depthStencilState.Release();
	blendState.Release();
	font_texture.Release();
	frame_target.Release();
}

void SGEImGui::setViewport(const Rect2s& viewport_arg) {
	viewport = viewport_arg;
	ImGui::GetIO().DisplaySize = ImVec2(viewport.width, viewport.height);
}

void SGEImGui::newFrame(const InputState& inputState) {
	timer.tick();

	ImGui::GetIO().DeltaTime = timer.diff_seconds();

	if (!inputState.isCursorRelative()) {
		ImGui::GetIO().MousePos.x = inputState.GetCursorPos().x;
		ImGui::GetIO().MousePos.y = inputState.GetCursorPos().y;
	}

	if (inputState.wasActiveWhilePolling()) {
		auto& io = ImGui::GetIO();

		if (!inputState.isCursorRelative()) {
			io.MouseDown[0] = inputState.IsKeyDown(Key::Key_MouseLeft);
			io.MouseDown[1] = inputState.IsKeyDown(Key::Key_MouseRight);
			io.MouseDown[2] = inputState.IsKeyDown(Key::Key_MouseMiddle);
		}
		io.MouseWheel = float(-inputState.GetWheelCount());

		io.KeyCtrl = inputState.IsKeyDown(Key::Key_LCtrl) || inputState.IsKeyDown(Key::Key_RCtrl);
		io.KeyShift = inputState.IsKeyDown(Key::Key_LShift) || inputState.IsKeyDown(Key::Key_RShift);
		io.KeyAlt = inputState.IsKeyDown(Key::Key_LAlt) || inputState.IsKeyDown(Key::Key_RAlt);

		io.KeysDown[ImGuiKey_Tab] = inputState.IsKeyDown(Key::Key_Tab);
		io.KeysDown[ImGuiKey_LeftArrow] = inputState.IsKeyDown(Key::Key_Left);
		io.KeysDown[ImGuiKey_RightArrow] = inputState.IsKeyDown(Key::Key_Right);
		io.KeysDown[ImGuiKey_UpArrow] = inputState.IsKeyDown(Key::Key_Up);
		io.KeysDown[ImGuiKey_DownArrow] = inputState.IsKeyDown(Key::Key_Down);
		io.KeysDown[ImGuiKey_PageUp] = inputState.IsKeyDown(Key::Key_PageUp);
		io.KeysDown[ImGuiKey_PageDown] = inputState.IsKeyDown(Key::Key_PageDown);
		io.KeysDown[ImGuiKey_Home] = inputState.IsKeyDown(Key::Key_Home);
		io.KeysDown[ImGuiKey_End] = inputState.IsKeyDown(Key::Key_End);
		io.KeysDown[ImGuiKey_Delete] = inputState.IsKeyDown(Key::Key_Delete);
		io.KeysDown[ImGuiKey_Backspace] = inputState.IsKeyDown(Key::Key_Backspace);
		io.KeysDown[ImGuiKey_Enter] = inputState.IsKeyDown(Key::Key_Enter);
		io.KeysDown[ImGuiKey_Escape] = inputState.IsKeyDown(Key::Key_Escape);
		io.KeysDown[ImGuiKey_A] = inputState.IsKeyDown(Key::Key_A);
		io.KeysDown[ImGuiKey_C] = inputState.IsKeyDown(Key::Key_C);
		io.KeysDown[ImGuiKey_V] = inputState.IsKeyDown(Key::Key_V);
		io.KeysDown[ImGuiKey_X] = inputState.IsKeyDown(Key::Key_X);
		io.KeysDown[ImGuiKey_Y] = inputState.IsKeyDown(Key::Key_Y);
		io.KeysDown[ImGuiKey_Z] = inputState.IsKeyDown(Key::Key_Z);
		io.KeysDown[ImGuiKey_S] = inputState.IsKeyDown(Key::Key_S);

		// TODO: This seems correct, but is it?
		const char* inputText = inputState.GetText();
		while (*inputText != '\0') {
			// Skip tabs, as they are handled by the ImGuiKey_Tab in ImGui.
			// if(*inputText != '\t')
			{ ImGui::GetIO().AddInputCharacter((ImWchar)*inputText); }
			inputText++;
		}
	}

	ImGui::NewFrame();
}

void SGEImGui::render() {
	ImGui::Render();

	ImDrawData* const data = ImGui::GetDrawData();
	if (data) {
		renderDrawLists(data);
	}
}

void SGEImGui::renderDrawLists(ImDrawData* imDrawData) {
	const int vbNeededBytes = sizeof(SGEImGuiVertex) * imDrawData->TotalVtxCount;

	if (vbNeededBytes == 0) {
		return;
	}

	if ((vertBuffer.IsResourceValid() == false) || (vertBuffer->getDesc().sizeBytes < vbNeededBytes)) {
		vertBuffer = sgecon->getDevice()->requestResource<sge::Buffer>();
		sge::BufferDesc bd;
		bd.sizeBytes = vbNeededBytes;
		bd.bindFlags = sge::ResourceBindFlags::VertexBuffer;
		bd.usage = sge::ResourceUsage::Dynamic;

		vertBuffer->create(bd, NULL);
	}

	const int ibNeededBytes = sizeof(ImDrawIdx) * imDrawData->TotalIdxCount;

	if ((idxBuffer.IsResourceValid() == false) || (idxBuffer->getDesc().sizeBytes < ibNeededBytes)) {
		idxBuffer = sgecon->getDevice()->requestResource<Buffer>();
		sge::BufferDesc bd;
		bd.sizeBytes = ibNeededBytes;
		bd.bindFlags = sge::ResourceBindFlags::IndexBuffer;
		bd.usage = sge::ResourceUsage::Dynamic;

		idxBuffer->create(bd, NULL);
	}

	SGEImGuiVertex* vbData = (SGEImGuiVertex*)sgecon->map(vertBuffer, sge::Map::WriteDiscard);
	ImDrawIdx* ibData = (ImDrawIdx*)sgecon->map(idxBuffer, sge::Map::WriteDiscard);

	for (int iCmdList = 0; iCmdList < imDrawData->CmdListsCount; ++iCmdList) {
		const ImDrawList* cmdList = imDrawData->CmdLists[iCmdList];

		// memcpy(vbData, &cmdList->VtxBuffer[0], cmdList->VtxBuffer.size() * sizeof(ImDrawVert));
		for (int t = 0; t < cmdList->VtxBuffer.size(); ++t) {
			vbData->position = vec2f(cmdList->VtxBuffer[t].pos.x, cmdList->VtxBuffer[t].pos.y);
			vbData->uv = vec2f(cmdList->VtxBuffer[t].uv.x, cmdList->VtxBuffer[t].uv.y);
			vbData->color = cmdList->VtxBuffer[t].col;
			vbData++;
		}

		memcpy(ibData, &cmdList->IdxBuffer[0], cmdList->IdxBuffer.size() * sizeof(ImDrawIdx));
		ibData += cmdList->IdxBuffer.size();
	}


	sgecon->unMap(vertBuffer);
	sgecon->unMap(idxBuffer);

	const mat4f ortho = mat4f::getOrthoRH(viewport.width, viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);

	stateGroup.setRenderState(rasterizerState, depthStencilState, blendState);
	stateGroup.setProgram(shadingProgram);
	stateGroup.setVBDeclIndex(m_imgui_vertexDeclIdx);
	stateGroup.setVB(0, vertBuffer, 0, sizeof(SGEImGuiVertex));
	stateGroup.setIB(idxBuffer, UniformType::Uint16, 0);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	StaticArray<BoundUniform, 10> uniforms;
	uniforms.push_back(BoundUniform(projViewWorldBindLoc, (void*)&ortho));

	// Redner the command list.
	int vtx_offset = 0;
	int idx_offset = 0;
	for (int t = 0; t < imDrawData->CmdListsCount; ++t) {
		const auto& cmd_list = imDrawData->CmdLists[t];
		for (int iCmd = 0; iCmd < cmd_list->CmdBuffer.size(); iCmd++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[iCmd];

			if (pcmd->UserCallback != NULL) {
				pcmd->UserCallback(cmd_list, pcmd);
			} else {
				Rect2s scissorsRect;

				scissorsRect.x = (short)pcmd->ClipRect.x;
				scissorsRect.y = (short)pcmd->ClipRect.y;
				scissorsRect.width = (short)pcmd->ClipRect.z - (short)pcmd->ClipRect.x;
				scissorsRect.height = (short)pcmd->ClipRect.w - (short)pcmd->ClipRect.y;

				Texture* pTexture = ((Texture*)pcmd->TextureId);

				// [HACK] Some resource may die during composing of ImGui Windows,
				// An examples is a window with a FrameTarget. When the user closes the window
				// the texture dies with it, but the draw call in ImGui were already submited,
				// in order not to crash we have to check if the texture is still valid.
				// [TODO] fix how ImGui is used!
				// const int textureIndex = textures->find_pointer_index(pTexture);
				// if(textureIndex >= 0 && textures->is_in_freelist(textureIndex) == false)
				{
					uniforms.push_back(BoundUniform(colorTexBindLoc, pTexture));
					if (colorTexSamplerBindLoc.isNull() == false)
						uniforms.push_back(BoundUniform(colorTexSamplerBindLoc, pTexture->getSamplerState()));
					DrawCall dc;

					dc.setUniforms(uniforms.data(), uniforms.size());
					dc.setStateGroup(&stateGroup);

					dc.drawIndexed(pcmd->ElemCount, idx_offset, vtx_offset);

					sgecon->executeDrawCall(dc, frame_target, &viewport, &scissorsRect);

					uniforms.pop_back();
					uniforms.pop_back();
				}
			}

			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.size();
	}
}

bool SGEImGui::IsItemActiveLastFrame() {
	ImGuiContext& g = *GImGui;

	if (g.ActiveIdPreviousFrame)
		return g.ActiveIdPreviousFrame == g.CurrentWindow->DC.LastItemId;
	return false;
}

bool SGEImGui::IsItemJustReleased() {
	return IsItemActiveLastFrame() && !ImGui::IsItemActive();
}

bool SGEImGui::IsItemJustActivated() {
	return !IsItemActiveLastFrame() && ImGui::IsItemActive();
}

bool SGEImGui::DragFloats(const char* label,
                          float* floats,
                          int numFloats,
                          bool* pJustReleased,
                          bool* pJustActivated,
                          float middleClickResetValue,
                          float v_speed,
                          float v_min,
                          float v_max,
                          const char* display_format,
                          float power) {
	ImGuiWindow* const window = ImGui::GetCurrentWindow();
	if (window->SkipItems) {
		return false;
	}

	const ImGuiStyle& style = ImGui::GetStyle();

	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID(label);
	ImGui::PushMultiItemsWidths(numFloats, ImGui::CalcItemWidth());
	for (int i = 0; i < numFloats; i++) {
		ImGui::PushID(i);
		value_changed |= ImGui::DragFloat("##v", &floats[i], v_speed, v_min, v_max, display_format, power);
		ImGui::PopID();
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		ImGui::PopItemWidth();

		if (ImGui::IsItemClicked(2)) {
			floats[i] = middleClickResetValue;
			value_changed = true;

			if (pJustActivated) {
				*pJustActivated = true;
			}

			if (pJustReleased) {
				*pJustReleased |= true;
			}
		}

		if (pJustReleased) {
			*pJustReleased |= IsItemJustReleased();
		}

		if (pJustActivated) {
			*pJustActivated |= IsItemJustActivated();
		}
	}
	ImGui::PopID();
	ImGui::EndGroup();

	return value_changed;
}

bool SGEImGui::DragInts(const char* label,
                        int* ints,
                        int numInts,
                        bool* pJustReleased,
                        bool* pJustActivated,
                        float v_speed,
                        int v_min,
                        int v_max,
                        const char* display_format) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
		return false;

	const ImGuiStyle& style = ImGui::GetStyle();

	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID(label);
	ImGui::PushMultiItemsWidths(numInts, ImGui::CalcItemWidth());
	for (int i = 0; i < numInts; i++) {
		ImGui::PushID(i);
		value_changed |= ImGui::DragInt("##v", &ints[i], v_speed, v_min, v_max, display_format);
		ImGui::PopID();
		ImGui::SameLine(0, style.ItemInnerSpacing.x);
		ImGui::PopItemWidth();


		if (pJustReleased) {
			*pJustReleased |= IsItemJustReleased();
		}

		if (pJustActivated) {
			*pJustActivated |= IsItemJustActivated();
		}
	}
	ImGui::PopID();
	ImGui::EndGroup();

	return value_changed;
}

// ColorPicker
// Note: only access 3 floats if ImGuiColorEditFlags_NoAlpha flag is set.
// FIXME: we adjust the big color square height based on item width, which may cause a flickering feedback loop (if automatic height makes a
// vertical scrollbar appears, affecting automatic width..)
bool SGEImGui::ColorPicker4(const char* label,
                            float col[4],
                            bool* const pJustReleased,
                            bool* const pJustActivated,
                            ImGuiColorEditFlags flags,
                            const float* ref_col) {
	const auto RenderArrowsForVerticalBar = [](ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, float bar_w) -> void {
		const auto RenderArrow = [](ImDrawList* draw_list, ImVec2 pos, ImVec2 half_sz, ImGuiDir direction, ImU32 col) -> void {
			switch (direction) {
				case ImGuiDir_Left:
					draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y + half_sz.y),
					                             pos, col);
					return;
				case ImGuiDir_Right:
					draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y - half_sz.y),
					                             pos, col);
					return;
				case ImGuiDir_Up:
					draw_list->AddTriangleFilled(ImVec2(pos.x + half_sz.x, pos.y + half_sz.y), ImVec2(pos.x - half_sz.x, pos.y + half_sz.y),
					                             pos, col);
					return;
				case ImGuiDir_Down:
					draw_list->AddTriangleFilled(ImVec2(pos.x - half_sz.x, pos.y - half_sz.y), ImVec2(pos.x + half_sz.x, pos.y - half_sz.y),
					                             pos, col);
					return;
				case ImGuiDir_None:
				case ImGuiDir_COUNT:
					break; // Fix warnings
			}
		};

		RenderArrow(draw_list, ImVec2(pos.x + half_sz.x + 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Right, IM_COL32_BLACK);
		RenderArrow(draw_list, ImVec2(pos.x + half_sz.x, pos.y), half_sz, ImGuiDir_Right, IM_COL32_WHITE);
		RenderArrow(draw_list, ImVec2(pos.x + bar_w - half_sz.x - 1, pos.y), ImVec2(half_sz.x + 2, half_sz.y + 1), ImGuiDir_Left,
		            IM_COL32_BLACK);
		RenderArrow(draw_list, ImVec2(pos.x + bar_w - half_sz.x, pos.y), half_sz, ImGuiDir_Left, IM_COL32_WHITE);
	};

	const auto updateActivationResult = [&pJustReleased, &pJustActivated]() -> void {
		if (pJustReleased) {
			*pJustReleased |= IsItemJustReleased();
		}

		if (pJustActivated) {
			*pJustActivated |= IsItemJustActivated();
		}
	};

	using namespace ImGui;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* draw_list = window->DrawList;

	ImGuiStyle& style = g.Style;
	ImGuiIO& io = g.IO;

	PushID(label);
	BeginGroup();

	if (!(flags & ImGuiColorEditFlags_NoSidePreview))
		flags |= ImGuiColorEditFlags_NoSmallPreview;

	// Read stored options
	if (!(flags & ImGuiColorEditFlags__PickerMask))
		flags |= ((g.ColorEditOptions & ImGuiColorEditFlags__PickerMask) ? g.ColorEditOptions : ImGuiColorEditFlags__OptionsDefault) &
		         ImGuiColorEditFlags__PickerMask;
	IM_ASSERT(ImIsPowerOfTwo((int)(flags & ImGuiColorEditFlags__PickerMask))); // Check that only 1 is selected
	if (!(flags & ImGuiColorEditFlags_NoOptions))
		flags |= (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar);

	// Setup
	int components = (flags & ImGuiColorEditFlags_NoAlpha) ? 3 : 4;
	bool alpha_bar = (flags & ImGuiColorEditFlags_AlphaBar) && !(flags & ImGuiColorEditFlags_NoAlpha);
	ImVec2 picker_pos = window->DC.CursorPos;
	float square_sz = GetFrameHeight();
	float bars_width = square_sz; // Arbitrary smallish width of Hue/Alpha picking bars
	float sv_picker_size = ImMax(
	    bars_width * 1, CalcItemWidth() - (alpha_bar ? 2 : 1) * (bars_width + style.ItemInnerSpacing.x)); // Saturation/Value picking box
	float bar0_pos_x = picker_pos.x + sv_picker_size + style.ItemInnerSpacing.x;
	float bar1_pos_x = bar0_pos_x + bars_width + style.ItemInnerSpacing.x;
	float bars_triangles_half_sz = (float)(int)(bars_width * 0.20f);

	float backup_initial_col[4];
	memcpy(backup_initial_col, col, components * sizeof(float));

	float wheel_thickness = sv_picker_size * 0.08f;
	float wheel_r_outer = sv_picker_size * 0.50f;
	float wheel_r_inner = wheel_r_outer - wheel_thickness;
	ImVec2 wheel_center(picker_pos.x + (sv_picker_size + bars_width) * 0.5f, picker_pos.y + sv_picker_size * 0.5f);

	// Note: the triangle is displayed rotated with triangle_pa pointing to Hue, but most coordinates stays unrotated for logic.
	float triangle_r = wheel_r_inner - (int)(sv_picker_size * 0.027f);
	ImVec2 triangle_pa = ImVec2(triangle_r, 0.0f);                            // Hue point.
	ImVec2 triangle_pb = ImVec2(triangle_r * -0.5f, triangle_r * -0.866025f); // Black point.
	ImVec2 triangle_pc = ImVec2(triangle_r * -0.5f, triangle_r * +0.866025f); // White point.

	float H, S, V;
	ColorConvertRGBtoHSV(col[0], col[1], col[2], H, S, V);

	bool value_changed = false, value_changed_h = false, value_changed_sv = false;

	// Hue Bar
	{
		// SV rectangle logic
		InvisibleButton("sv", ImVec2(sv_picker_size, sv_picker_size));
		if (IsItemActive()) {
			S = ImSaturate((io.MousePos.x - picker_pos.x) / (sv_picker_size - 1));
			V = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_sv = true;
		}

		updateActivationResult();

		if (!(flags & ImGuiColorEditFlags_NoOptions))
			OpenPopupOnItemClick("context");

		// Hue bar logic
		SetCursorScreenPos(ImVec2(bar0_pos_x, picker_pos.y));
		InvisibleButton("hue", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive()) {
			H = ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = value_changed_h = true;
		}

		updateActivationResult();
	}

	// Alpha bar logic
	if (alpha_bar) {
		SetCursorScreenPos(ImVec2(bar1_pos_x, picker_pos.y));
		InvisibleButton("alpha", ImVec2(bars_width, sv_picker_size));
		if (IsItemActive()) {
			col[3] = 1.0f - ImSaturate((io.MousePos.y - picker_pos.y) / (sv_picker_size - 1));
			value_changed = true;
		}

		updateActivationResult();
	}

	if (!(flags & ImGuiColorEditFlags_NoSidePreview)) {
		SameLine(0, style.ItemInnerSpacing.x);
		BeginGroup();
	}

	if (!(flags & ImGuiColorEditFlags_NoLabel)) {
		const char* label_display_end = FindRenderedTextEnd(label);
		if (label != label_display_end) {
			if ((flags & ImGuiColorEditFlags_NoSidePreview))
				SameLine(0, style.ItemInnerSpacing.x);
			TextUnformatted(label, label_display_end);
		}
	}

	if (!(flags & ImGuiColorEditFlags_NoSidePreview)) {
		ImVec4 col_v4(col[0], col[1], col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : col[3]);
		if ((flags & ImGuiColorEditFlags_NoLabel))
			Text("Current");
		ColorButton("##current", col_v4,
		            (flags & (ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf |
		                      ImGuiColorEditFlags_NoTooltip)),
		            ImVec2(square_sz * 3, square_sz * 2));
		if (ref_col != NULL) {
			Text("Original");
			ImVec4 ref_col_v4(ref_col[0], ref_col[1], ref_col[2], (flags & ImGuiColorEditFlags_NoAlpha) ? 1.0f : ref_col[3]);
			if (ColorButton("##original", ref_col_v4,
			                (flags & (ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_AlphaPreviewHalf |
			                          ImGuiColorEditFlags_NoTooltip)),
			                ImVec2(square_sz * 3, square_sz * 2))) {
				memcpy(col, ref_col, components * sizeof(float));
				value_changed = true;
			}

			updateActivationResult();
		}

		updateActivationResult();
		EndGroup();
	}

	// Convert back color to RGB
	if (value_changed_h || value_changed_sv)
		ColorConvertHSVtoRGB(H >= 1.0f ? H - 10 * 1e-6f : H, S > 0.0f ? S : 10 * 1e-6f, V > 0.0f ? V : 1e-6f, col[0], col[1], col[2]);

	// R,G,B and H,S,V slider color editor
	if ((flags & ImGuiColorEditFlags_NoInputs) == 0) {
		PushItemWidth((alpha_bar ? bar1_pos_x : bar0_pos_x) + bars_width - picker_pos.x);
		UNUSED(ImGuiColorEditFlags sub_flags_to_forward = ImGuiColorEditFlags__DataTypeMask | ImGuiColorEditFlags_HDR |
		                                                  ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoOptions |
		                                                  ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_AlphaPreview |
		                                                  ImGuiColorEditFlags_AlphaPreviewHalf;)
		UNUSED(ImGuiColorEditFlags sub_flags = (flags & sub_flags_to_forward) | ImGuiColorEditFlags_NoPicker;)
		value_changed |= DragFloats("##rgbf", col, 3, pJustReleased, pJustActivated);
		PopItemWidth();
	}

	// Try to cancel hue wrap (after ColorEdit), if any
	if (value_changed) {
		float new_H, new_S, new_V;
		ColorConvertRGBtoHSV(col[0], col[1], col[2], new_H, new_S, new_V);
		if (new_H <= 0 && H > 0) {
			if (new_V <= 0 && V != new_V)
				ColorConvertHSVtoRGB(H, S, new_V <= 0 ? V * 0.5f : new_V, col[0], col[1], col[2]);
			else if (new_S <= 0)
				ColorConvertHSVtoRGB(H, new_S <= 0 ? S * 0.5f : new_S, new_V, col[0], col[1], col[2]);
		}
	}

	ImVec4 hue_color_f(1, 1, 1, 1);
	ColorConvertHSVtoRGB(H, 1, 1, hue_color_f.x, hue_color_f.y, hue_color_f.z);
	ImU32 hue_color32 = ColorConvertFloat4ToU32(hue_color_f);
	ImU32 col32_no_alpha = ColorConvertFloat4ToU32(ImVec4(col[0], col[1], col[2], 1.0f));

	const ImU32 hue_colors[6 + 1] = {IM_COL32(255, 0, 0, 255),   IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255),
	                                 IM_COL32(0, 255, 255, 255), IM_COL32(0, 0, 255, 255),   IM_COL32(255, 0, 255, 255),
	                                 IM_COL32(255, 0, 0, 255)};
	ImVec2 sv_cursor_pos;

	{
		// Render SV Square
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_WHITE, hue_color32,
		                                   hue_color32, IM_COL32_WHITE);
		draw_list->AddRectFilledMultiColor(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), IM_COL32_BLACK_TRANS,
		                                   IM_COL32_BLACK_TRANS, IM_COL32_BLACK, IM_COL32_BLACK);
		RenderFrameBorder(picker_pos, picker_pos + ImVec2(sv_picker_size, sv_picker_size), 0.0f);
		sv_cursor_pos.x = ImClamp((float)(int)(picker_pos.x + ImSaturate(S) * sv_picker_size + 0.5f), picker_pos.x + 2,
		                          picker_pos.x + sv_picker_size - 2); // Sneakily prevent the circle to stick out too much
		sv_cursor_pos.y = ImClamp((float)(int)(picker_pos.y + ImSaturate(1 - V) * sv_picker_size + 0.5f), picker_pos.y + 2,
		                          picker_pos.y + sv_picker_size - 2);

		// Render Hue Bar
		for (int i = 0; i < 6; ++i)
			draw_list->AddRectFilledMultiColor(ImVec2(bar0_pos_x, picker_pos.y + i * (sv_picker_size / 6)),
			                                   ImVec2(bar0_pos_x + bars_width, picker_pos.y + (i + 1) * (sv_picker_size / 6)),
			                                   hue_colors[i], hue_colors[i], hue_colors[i + 1], hue_colors[i + 1]);
		float bar0_line_y = (float)(int)(picker_pos.y + H * sv_picker_size + 0.5f);
		RenderFrameBorder(ImVec2(bar0_pos_x, picker_pos.y), ImVec2(bar0_pos_x + bars_width, picker_pos.y + sv_picker_size), 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar0_pos_x - 1, bar0_line_y),
		                           ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	// Render cursor/preview circle (clamp S/V within 0..1 range because floating points colors may lead HSV values to be out of range)
	float sv_cursor_rad = value_changed_sv ? 10.0f : 6.0f;
	draw_list->AddCircleFilled(sv_cursor_pos, sv_cursor_rad, col32_no_alpha, 12);
	draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad + 1, IM_COL32(128, 128, 128, 255), 12);
	draw_list->AddCircle(sv_cursor_pos, sv_cursor_rad, IM_COL32_WHITE, 12);

	// Render alpha bar
	if (alpha_bar) {
		float alpha = ImSaturate(col[3]);
		ImRect bar1_bb(bar1_pos_x, picker_pos.y, bar1_pos_x + bars_width, picker_pos.y + sv_picker_size);
		RenderColorRectWithAlphaCheckerboard(ImGui::GetWindowDrawList(), bar1_bb.Min, bar1_bb.Max, IM_COL32(0, 0, 0, 0),
		                                     bar1_bb.GetWidth() / 2.0f, ImVec2(0.0f, 0.0f));
		draw_list->AddRectFilledMultiColor(bar1_bb.Min, bar1_bb.Max, col32_no_alpha, col32_no_alpha, col32_no_alpha & ~IM_COL32_A_MASK,
		                                   col32_no_alpha & ~IM_COL32_A_MASK);
		float bar1_line_y = (float)(int)(picker_pos.y + (1.0f - alpha) * sv_picker_size + 0.5f);
		RenderFrameBorder(bar1_bb.Min, bar1_bb.Max, 0.0f);
		RenderArrowsForVerticalBar(draw_list, ImVec2(bar1_pos_x - 1, bar1_line_y),
		                           ImVec2(bars_triangles_half_sz + 1, bars_triangles_half_sz), bars_width + 2.0f);
	}

	EndGroup();
	PopID();

	return value_changed && memcmp(backup_initial_col, col, components * sizeof(float));
}

bool SGEImGui::ColorPicker3(
    const char* label, float col[3], bool* const pJustPressed, bool* const pJustReleased, ImGuiColorEditFlags flags, const float* ref_col) {
	using namespace ImGui;

	float col4[4] = {col[0], col[1], col[2], 1.0f};
	if (!ColorPicker4(label, col4, pJustPressed, pJustReleased, flags | ImGuiColorEditFlags_NoAlpha, ref_col))
		return false;
	col[0] = col4[0];
	col[1] = col4[1];
	col[2] = col4[2];
	return true;
}


//--------------------------------------------------------------
//
//--------------------------------------------------------------
std::unordered_map<ImGuiID, UIState> g_sgeUIState;

UIState* getUIStateInternal(const ImGuiID id, void* (*newer)(), void (*deleter)(void*)) {
	UIState& res = g_sgeUIState[id];

	if (res.storage == nullptr && deleter != nullptr && newer != nullptr) {
		res.storage = newer();
		res.deleter = deleter;
	}

	return &res;
}

} // namespace sge

namespace ImGuiEx {

static ImVector<ImRect> s_GroupPanelLabelStack;

void BeginGroupPanel(const char* name, const ImVec2 size) {
	ImGui::BeginGroup();

	auto cursorPos = ImGui::GetCursorScreenPos();
	auto itemSpacing = ImGui::GetStyle().ItemSpacing;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();
	ImGui::BeginGroup();

	ImVec2 effectiveSize = size;
	if (size.x < 0.0f)
		effectiveSize.x = ImGui::GetContentRegionAvailWidth();
	else
		effectiveSize.x = size.x;
	ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));

	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginGroup();
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextUnformatted(name);
	auto labelMin = ImGui::GetItemRectMin();
	auto labelMax = ImGui::GetItemRectMax();
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(0.0, frameHeight + itemSpacing.y));
	ImGui::BeginGroup();

	// ImGui::GetWindowDrawList()->AddRect(labelMin, labelMax, IM_COL32(255, 0, 255, 255));

	ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
#else
	ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x -= frameHeight * 0.5f;
#endif
	ImGui::GetCurrentWindow()->Size.x -= frameHeight;

	auto itemWidth = ImGui::CalcItemWidth();
	ImGui::PushItemWidth(ImMax(0.0f, itemWidth - frameHeight));

	s_GroupPanelLabelStack.push_back(ImRect(labelMin, labelMax));
}

void EndGroupPanel() {
	ImGui::PopItemWidth();

	auto itemSpacing = ImGui::GetStyle().ItemSpacing;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	auto frameHeight = ImGui::GetFrameHeight();

	ImGui::EndGroup();

	// ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 255, 0, 64), 4.0f);

	ImGui::EndGroup();

	ImGui::SameLine(0.0f, 0.0f);
	ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
	ImGui::Dummy(ImVec2(0.0, frameHeight - frameHeight * 0.5f - itemSpacing.y));

	ImGui::EndGroup();

	auto itemMin = ImGui::GetItemRectMin();
	auto itemMax = ImGui::GetItemRectMax();
	// ImGui::GetWindowDrawList()->AddRectFilled(itemMin, itemMax, IM_COL32(255, 0, 0, 64), 4.0f);

	auto labelRect = s_GroupPanelLabelStack.back();
	s_GroupPanelLabelStack.pop_back();

	ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
	ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));
	labelRect.Min.x -= itemSpacing.x;
	labelRect.Max.x += itemSpacing.x;
	for (int i = 0; i < 4; ++i) {
		switch (i) {
			// left half-plane
			case 0:
				ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(labelRect.Min.x, FLT_MAX), true);
				break;
			// right half-plane
			case 1:
				ImGui::PushClipRect(ImVec2(labelRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), true);
				break;
			// top
			case 2:
				ImGui::PushClipRect(ImVec2(labelRect.Min.x, -FLT_MAX), ImVec2(labelRect.Max.x, labelRect.Min.y), true);
				break;
			// bottom
			case 3:
				ImGui::PushClipRect(ImVec2(labelRect.Min.x, labelRect.Max.y), ImVec2(labelRect.Max.x, FLT_MAX), true);
				break;
		}

		ImGui::GetWindowDrawList()->AddRect(frameRect.Min, frameRect.Max, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_Border)), halfFrame.x);

		ImGui::PopClipRect();
	}

	ImGui::PopStyleVar(2);

#if IMGUI_VERSION_NUM >= 17301
	ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
	ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
#else
	ImGui::GetCurrentWindow()->ContentsRegionRect.Max.x += frameHeight * 0.5f;
#endif
	ImGui::GetCurrentWindow()->Size.x += frameHeight;

	ImGui::Dummy(ImVec2(0.0f, 0.0f));

	ImGui::EndGroup();
}

void Label(const char* label, bool shouldSetNextItemWidth, float labelWidthProportion) {
	if (label == nullptr) {
		sgeAssert(false);
		return;
	}

	ImGuiWindow* const window = ImGui::GetCurrentWindow();

	const ImVec2 lineStart = ImGui::GetCursorScreenPos();
	const float fullWidth = ImGui::GetContentRegionAvail().x;
	const float labelWidth = fullWidth * labelWidthProportion;

	const ImVec2 textSize = ImGui::CalcTextSize(label);

	ImRect textRect;
	textRect.Min = ImGui::GetCursorScreenPos();
	textRect.Max = textRect.Min;
	textRect.Max.x += labelWidth;
	textRect.Max.y += textSize.y;

	ImGui::SetCursorScreenPos(textRect.Min);

	ImGui::AlignTextToFramePadding();
	textRect.Min.y += window->DC.CurrLineTextBaseOffset;
	textRect.Max.y += window->DC.CurrLineTextBaseOffset;

	ImGui::ItemSize(textRect);
	if (ImGui::ItemAdd(textRect, window->GetID(label))) {
		ImGui::RenderTextEllipsis(ImGui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x, textRect.Max.x, label, nullptr,
		                          &textSize);
	}

	// If the space used for the label is too small then the text wouldn't be readable.
	// Add the label as a tooltip as a workaround.
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text(label);
		ImGui::EndTooltip();
	}

	ImGui::SetCursorScreenPos(textRect.Max - ImVec2{0, textSize.y + window->DC.CurrLineTextBaseOffset});
	ImGui::SameLine();

	if (shouldSetNextItemWidth) {
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float nextItemWidth = std::max(fullWidth * (1.f - labelWidthProportion) - spacing, 0.f);
		ImGui::SetNextItemWidth(nextItemWidth);
	}
}

bool InputText(const char* label,
               std::string& str,
               ImGuiInputTextFlags flags,
               ImGuiInputTextCallback callback,
               void* user_data,
               bool acceptOnlyIdentifierStyleText) {
	struct InputTextCallback_UserData {
		std::string* Str;
		ImGuiInputTextCallback ChainCallback;
		void* ChainCallbackUserData;
	};

	auto InputTextCallback = [](ImGuiInputTextCallbackData* data) -> int {
		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		} else if (user_data->ChainCallback) {
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	};

	auto InputTextCallbackIdentifierFilter = [](ImGuiInputTextCallbackData* data) -> int {
		InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
		if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
			if (data->EventChar <= 255 && (std::isalpha(data->EventChar) || std::isdigit(data->EventChar) || data->EventChar == '_')) {
				return 0;
			}

			return 1;
		} else if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
			// Resize string callback
			// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
			std::string* str = user_data->Str;
			IM_ASSERT(data->Buf == str->c_str());
			str->resize(data->BufTextLen);
			data->Buf = (char*)str->c_str();
		} else if (user_data->ChainCallback) {
			// Forward to user callback, if any
			data->UserData = user_data->ChainCallbackUserData;
			return user_data->ChainCallback(data);
		}
		return 0;
	};



	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;
	if (acceptOnlyIdentifierStyleText) {
		flags |= ImGuiInputTextFlags_CallbackCharFilter;
	}

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str = &str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;

	if (acceptOnlyIdentifierStyleText) {
		return ImGui::InputText(label, (char*)str.c_str(), str.capacity() + 1, flags, InputTextCallbackIdentifierFilter, &cb_user_data);
	} else {
		return ImGui::InputText(label, (char*)str.c_str(), str.capacity() + 1, flags, InputTextCallback, &cb_user_data);
	}
}

SGE_CORE_API void TextTooltip(const char* const text) {
	if (text != nullptr && ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::Text(text);
		ImGui::EndTooltip();
	}
}

SGE_CORE_API void TextTooltipDelayed(const char* const text, float delay) {
	if (text != nullptr && ImGui::IsItemHovered() && (GImGui->HoveredIdTimer >= delay)) {
		ImGui::BeginTooltip();
		ImGui::Text(text);
		ImGui::EndTooltip();
	}
}

} // namespace ImGuiEx
