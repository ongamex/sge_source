#include "ModelPreviewWindow.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/QuickDraw.h"
#include "sge_core/SGEImGui.h"
#include "sge_core/application/input.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/tiny/FileOpenDialog.h"
#include <imgui/imgui.h>

#include "sge_core/ICore.h"

namespace sge {

static bool promptForModel(std::shared_ptr<Asset>& asset) {
	AssetLibrary* const assetLib = getCore()->getAssetLib();

	const std::string filename = FileOpenDialog("Pick a model", true, "*.mdl\0*.mdl\0", nullptr);
	if (!filename.empty()) {
		asset = assetLib->getAsset(AssetType::Model, filename.c_str(), true);
		return true;
	}
	return false;
}

void ModelPreviewWidget::doWidget(SGEContext* const sgecon, const InputState& is, EvaluatedModel& m_eval, Optional<vec2f> widgetSize) {
	if (m_frameTarget.IsResourceValid() == false) {
		m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
		m_frameTarget->create2D(64, 64);
	}

	RenderDestination rdest(sgecon, m_frameTarget);

	const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	vec2f canvas_size = widgetSize.isValid() ? widgetSize.get() : fromImGui(ImGui::GetContentRegionAvail());

	if (canvas_size.x < 0.f)
		canvas_size.x = ImGui::GetContentRegionAvail().x;
	if (canvas_size.y < 0.f)
		canvas_size.y = ImGui::GetContentRegionAvail().y;

	if (canvas_size.y < 64)
		canvas_size.y = 64;

	vec2f texsize = canvas_size;

	if (texsize.x < 64)
		texsize.x = 64;
	if (texsize.y < 64)
		texsize.y = 64;

	if (m_frameTarget->getWidth() != texsize.x || m_frameTarget->getHeight() != texsize.y) {
		m_frameTarget->create2D((int)texsize.x, (int)texsize.y);
	}

	float ratio = (float)m_frameTarget->getWidth() / (float)m_frameTarget->getHeight();

	mat4f lookAt = camera.GetViewMatrix();
	mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(45.f), ratio, 0.1f, 10000.f,
	                                        kIsTexcoordStyleD3D); // The Y flip for OpenGL is done by the modelviewer.

	sgecon->clearColor(m_frameTarget, 0, vec4f(0.f).data);
	sgecon->clearDepth(m_frameTarget, 1.f);

	QuickDraw& debugDraw = getCore()->getQuickDraw();


	debugDraw.drawWired_Clear();
	debugDraw.drawWiredAdd_Grid(vec3f(0), vec3f::getAxis(0), vec3f::getAxis(2), 5, 5, 0xFF333733);
	debugDraw.drawWired_Execute(rdest, proj * lookAt, nullptr);

	DrawReasonInfo mods;
	InstanceDrawMods imods;
	imods.forceNoLighting = true;

	getCore()->getModelDraw().draw(rdest, camera.eyePosition(), -camera.orbitPoint.normalized0(), proj * lookAt, mat4f::getIdentity(),
	                               DrawReasonInfo(), m_eval, imods);

	if (kIsTexcoordStyleD3D) {
		ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y));
	} else {
		ImGui::Image(m_frameTarget->getRenderTarget(0), ImVec2(canvas_size.x, canvas_size.y), ImVec2(0, 1), ImVec2(1, 0));
	}

	if (ImGui::IsItemHovered()) {
		camera.update(true, is.IsKeyDown(Key_MouseLeft), false, is.IsKeyDown(Key_MouseRight),
		              is.GetCursorPos());
	}
}

void ModelPreviewWindow::update(SGEContext* const sgecon, const InputState& is) {
	if (isClosed()) {
		return;
	}

	if (ImGui::Begin(m_windowName.c_str(), &m_isOpened)) {
		AssetLibrary* const assetLib = getCore()->getAssetLib();

		if (m_frameTarget.IsResourceValid() == false) {
			m_frameTarget = sgecon->getDevice()->requestResource<FrameTarget>();
			m_frameTarget->create2D(64, 64);
		}

		ImGui::Columns(2);
		ImGui::BeginChild("sidebar_wnd");

		if (ImGui::Button("Pick##ModeToPreview")) {
			promptForModel(m_model);
			m_eval = EvaluatedModel();
			iPreviewAnimDonor = -1;
			iPreviewAnimation = -1;
			animationComboPreviewValue = "<None>";
			animationDonors.clear();
			previewAimationTime = 0.f;
			autoPlayAnimation = true;
			if (m_model) {
				m_eval.initialize(assetLib, &m_model->asModel()->model);
			}
		}

		if (isAssetLoaded(m_model)) {
			if (ImGui::Button("Add Animation Donor")) {
				PAsset donor;
				if (promptForModel(donor)) {
					animationDonors.push_back(donor);
					m_eval.addAnimationDonor(donor);
				}
			}
		}

		ImGui::SameLine();
		if (isAssetLoaded(m_model))
			ImGui::Text("%s", m_model->getPath().c_str());
		else
			ImGui::Text("Pick a Model");

		ImGui::EndChild();
		ImGui::NextColumn();
		if (m_model.get() != NULL) {
			if (ImGui::BeginCombo("Animation", animationComboPreviewValue.c_str())) {
				if (ImGui::Selectable("<None>")) {
					animationComboPreviewValue = "<None>";
					iPreviewAnimDonor = -1;
					iPreviewAnimation = -1;
				}

				for (int t = 0; t < m_eval.m_model->numAnimations(); ++t) {
					const ModelAnimation* anim = m_eval.m_model->animationAt(t);
					if (ImGui::Selectable(anim->animationName.c_str())) {
						animationComboPreviewValue = anim->animationName;
						iPreviewAnimDonor = -1;
						iPreviewAnimation = t;
					}
				}

				for (int iDonor = 0; iDonor < animationDonors.size(); ++iDonor) {
					const Model& donorModel = animationDonors[iDonor]->asModel()->model;
					for (int t = 0; t < donorModel.numAnimations(); ++t) {
						const ModelAnimation* anim = donorModel.animationAt(t);
						if (ImGui::Selectable((animationDonors[iDonor]->getPath() + " | " + anim->animationName).c_str())) {
							animationComboPreviewValue = anim->animationName;
							iPreviewAnimDonor = iDonor;
							iPreviewAnimation = t;
						}
					}
				}

				ImGui::EndCombo();
			}

			EvalMomentSets evalMoment;
			if (iPreviewAnimation == -1) {
				evalMoment = EvalMomentSets();
			} else {
				const ModelAnimation* anim = m_eval.findAnimation(iPreviewAnimDonor, iPreviewAnimation);

				ImGui::Checkbox("Auto Play", &autoPlayAnimation);
				if (autoPlayAnimation) {
					previewAimationTime += 0.016f; // TODO timer.

					if (previewAimationTime > anim->durationSec) {
						previewAimationTime -= anim->durationSec;
					}
				}

				ImGui::SliderFloat("Time", &previewAimationTime, 0.f, anim->durationSec);

				evalMoment.donorIndex = iPreviewAnimDonor;
				evalMoment.animationIndex = iPreviewAnimation;
				evalMoment.time = previewAimationTime;
			}

			m_eval.evaluateFromMoments(&evalMoment, 1);

			const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
			ImVec2 canvas_size = ImGui::GetContentRegionAvail();

			if (canvas_size.y < 64)
				canvas_size.y = 64;

			ImVec2 texsize = canvas_size;

			if (texsize.x < 64)
				texsize.x = 64;
			if (texsize.y < 64)
				texsize.y = 64;

			if (m_frameTarget->getWidth() != texsize.x || m_frameTarget->getHeight() != texsize.y) {
				m_frameTarget->create2D((int)texsize.x, (int)texsize.y);
			}

			float ratio = (float)m_frameTarget->getWidth() / (float)m_frameTarget->getHeight();

			mat4f lookAt = camera.GetViewMatrix();
			mat4f proj = mat4f::getPerspectiveFovRH(deg2rad(45.f), ratio, 0.1f, 10000.f,
			                                        kIsTexcoordStyleD3D); // The Y flip for OpenGL is done by the modelviewer.

			sgecon->clearColor(m_frameTarget, 0, vec4f(0.f).data);
			sgecon->clearDepth(m_frameTarget, 1.f);

			QuickDraw& debugDraw = getCore()->getQuickDraw();

			RenderDestination rdest(sgecon, m_frameTarget, m_frameTarget->getViewport());

			debugDraw.drawWiredAdd_Grid(vec3f(0), vec3f::getAxis(0), vec3f::getAxis(2), 5, 5, 0xFF333733);
			debugDraw.drawWired_Execute(rdest, proj * lookAt, nullptr);

			getCore()->getModelDraw().draw(rdest, camera.eyePosition(), -camera.orbitPoint.normalized0(), proj * lookAt,
			                               mat4f::getIdentity(), DrawReasonInfo(), m_eval, InstanceDrawMods());

			ImGui::InvisibleButton("TextureCanvasIB", canvas_size);

			if (ImGui::IsItemHovered()) {
				camera.update(is.IsKeyDown(Key_LAlt), is.IsKeyDown(Key_MouseLeft), is.IsKeyDown(Key_MouseMiddle),
				              is.IsKeyDown(Key_MouseRight), is.GetCursorPos());
			}

			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 canvasMax = canvas_pos;
			canvasMax.x += canvas_size.x;
			canvasMax.y += canvas_size.y;

			draw_list->AddImage(m_frameTarget->getRenderTarget(0), canvas_pos, canvasMax);
			draw_list->AddRect(canvas_pos, canvasMax, ImColor(255, 255, 255));
		}
	}
	ImGui::End();
}

} // namespace sge
