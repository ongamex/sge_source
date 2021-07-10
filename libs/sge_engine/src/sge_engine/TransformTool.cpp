#include "TransformTool.h"
#include "GameInspector.h"
#include "sge_core/AssetLibrary.h"
#include <imgui/imgui.h>

#include "sge_engine/GameDrawer/GameDrawer.h"
#include "sge_core/ICore.h"

namespace sge {

//---------------------------------------------------------------
//
//---------------------------------------------------------------
void TransformTool::onSetActive(GameInspector* const inspector) {
	GameWorld* const world = inspector->m_world;

	clear();
	perItemData.resize(inspector->getSelection().size());

	AABox3f allBBoxesWs;

	for (int iItem = 0; iItem < int(inspector->getSelection().size()); ++iItem) {
		const SelectedItem& sel = inspector->m_selection[iItem];
		Actor* const actor = inspector->m_world->getActorById(sel.objectId);
		if (actor) {
			transf3d itemTransform;
			actor->getItemTransform(itemTransform, sel.editMode, sel.index);

			if (sel.editMode == editMode_actors) {
				allBBoxesWs.expand(actor->getBBoxOS().getTransformed(itemTransform.toMatrix()));
			}

			perItemData[iItem].item = sel;
			perItemData[iItem].initialTasform = itemTransform;
			perItemData[iItem].editedTransform = itemTransform;
			perItemData[iItem].shouldPerformLocalSpaceTransformChange = false;
			perItemData[iItem].parentInitialTransform = transf3d();

			if (sel.editMode == editMode_actors) {
				ObjectId parentId = world->getParentId(sel.objectId);
				if (inspector->isSelected(parentId)) {
					Actor* parentActor = world->getActorById(parentId);
					if (parentActor) {
						perItemData[iItem].shouldPerformLocalSpaceTransformChange = true;
						perItemData[iItem].parentInitialTransform = parentActor->getTransform();
					}
				}
			}
		}
	}

	workingSelectionDirtyIndex = inspector->m_selectionChangeIdx;

	// Determine the inital trasformation of the gizmo.
	transf3d gizmoTransform = transf3d::getIdentity();
	AABox3f gizmoBBoxScaleVolume;
	if (m_mode == Gizmo3D::Mode_ScaleVolume) {
		if (inspector->getSelection().size() > 1) {
			gizmoTransform = transf3d::getIdentity();
			gizmoBBoxScaleVolume = allBBoxesWs;
		} else {
			Actor* const actor = inspector->m_world->getActorById(inspector->getPrimarySelection());
			if (actor) {
				gizmoTransform = actor->getTransform();
				gizmoBBoxScaleVolume = actor->getBBoxOS();
			}
		}
	} else {
		if (perItemData.size() == 1) {
			gizmoTransform = perItemData[0].initialTasform;
		} else if (perItemData.size() > 1) {
			vec3f averagePosition(0.f);
			for (const PerControlledItemData& i : perItemData) {
				averagePosition += i.initialTasform.p;
			}

			averagePosition *= 1.f / (float)(perItemData.size());

			gizmoTransform = transf3d::getIdentity();

			if (m_origin == transformToolOrigin_lastSelected) {
				gizmoTransform.p = perItemData.back().initialTasform.p;
			}

			if (m_origin == transformToolOrigin_firstSelected) {
				gizmoTransform.p = perItemData.front().initialTasform.p;
			} else if (m_origin == transformToolOrigin_averagePosition) {
				gizmoTransform.p = averagePosition;
			}
		}
	}


	m_gizmo.reset(m_mode, gizmoTransform, gizmoBBoxScaleVolume);

	// HACK: If the View isn't focused, and the user changes the gizmo mode than the gizmo would appear small,
	// as it did not recaculated it's new size.
	// To workaround that we pass an invalud input state.
	// m_gizmo.interact(InputState(), Ray(vec3f(0.f), vec3f(1.f, 0.f, 0.f)), nullptr, nullptr);
}

InspectorToolResult TransformTool::updateTool(GameInspector* const inspector,
                                              bool isAllowedToTakeInput,
                                              const InputState& is,
                                              const GameDrawSets& drawSets) {
	InspectorToolResult result;

	if (workingSelectionDirtyIndex != inspector->m_selectionChangeIdx) {
		result.reapply = true;
		return result;
	}

	if (!isAllowedToTakeInput || perItemData.size() == 0) {
		result.propagateInput = true;
		return result;
	}

	// Compute the picking ray.
	vec2f const viewportSize(drawSets.rdest.viewport.width, drawSets.rdest.viewport.height);
	vec2f const cursorUV = is.GetCursorPos() / viewportSize;
	// vec3f const pickDirVS = rayFromProjectionMatrix(cursorUV, deg2rad(60.f), drawSets.rdest.viewport.ratioWbyH());

	const mat4f proj = drawSets.drawCamera->getProj();
	const mat4f projInv = drawSets.drawCamera->getProj().inverse();
	bool const isOrthographic = drawSets.drawCamera->getProj().data[2][3] == 0.f;


	// Determine the mouse position in view space.
	const float vx = (2.f * cursorUV.x - 1.f);
	const float vy = (-2.f * cursorUV.y + 1.f);
	vec4f pickDirVS = projInv * vec4f(vx, vy, -1.f, 1.f);
	pickDirVS.x = pickDirVS.x / pickDirVS.w;
	pickDirVS.y = pickDirVS.y / pickDirVS.w;
	pickDirVS.z = pickDirVS.z / pickDirVS.w;
	pickDirVS.w = 0.f;
	mat4f const viewInvMtx = drawSets.drawCamera->getView().inverse();

	// Determine the rays origin and direction.
	vec3f pickDirWS;
	vec3f cameraEyePos;
	float customScale;
	if (isOrthographic) {
		cameraEyePos = mat_mul_pos(viewInvMtx, vec3f(pickDirVS.x, pickDirVS.y, 0.f));
		pickDirWS = -viewInvMtx.data[2].xyz();
		customScale = 2.f * 1.f / proj.data[1][1];
	} else {
		cameraEyePos = viewInvMtx.data[3].xyz();
		pickDirWS = (viewInvMtx * pickDirVS).xyz();
		customScale = 0.f;
	}

	bool userClickedAway;
	bool const gizmoDone = interact(is, cameraEyePos, pickDirWS, customScale, &userClickedAway);

	if (userClickedAway) {
		result.propagateInput = true;
		return result;
	}

	// Apply the edited transform form the gizmo. The gizmo overrieds everyone.
	if (!userClickedAway && perItemData.size() != 0) {
		CmdCompound* cmdMassMove = gizmoDone ? new CmdCompound() : nullptr;

		for (int t = 0; t < perItemData.size(); ++t) {
			Actor* const actor = inspector->m_world->getActorById(perItemData[t].item.objectId);
			if (!actor) {
				continue;
			}

			// Update the item's location.
			actor->setItemTransform(perItemData[t].item.editMode, perItemData[t].item.index, perItemData[t].editedTransform);
			actor->onMemberChanged();

			if (cmdMassMove) {
				// Generate the command that would move the item.
				InspectorCmd* const cmd =
				    actor->generateItemSetTransformCmd(inspector, perItemData[t].item.editMode, perItemData[t].item.index,
				                                       perItemData[t].initialTasform, perItemData[t].editedTransform);

				cmdMassMove->addCommand(cmd);
			}
		}

		if (cmdMassMove) {
			inspector->appendCommand(cmdMassMove, false);
		}

		if (gizmoDone) {
			clear();
			result.reapply = true;
		}
	}

	return result;
}

void TransformTool::onCancel(GameInspector* UNUSED(inspector)) {
	workingSelectionDirtyIndex = 0;
	clear();
}

void TransformTool::onUI(GameInspector* inspector) {
	int origin_int = m_origin;
	if (ImGui::Combo("Gizmo Location", &origin_int, "Last Selected\0First Selected\0Average\0")) {
		m_origin = (TransformToolOrigin)origin_int;
		inspector->setTool(&inspector->m_transformTool);
	}

	ImGui::DragFloat3("Move snapping", m_snapSettings.translationSnapping.data, 0.1f);

	float rotationAngleSnappingInDegrees = rad2deg(m_snapSettings.rotationSnapping);
	ImGui::DragFloat("Rotation angle snapping", &rotationAngleSnappingInDegrees, 0.1f);
	m_snapSettings.rotationSnapping = deg2rad(rotationAngleSnappingInDegrees);

	ImGui::DragFloat3("Scaling snapping", m_snapSettings.scaleSnapping.data, 0.1f);

	ImGui::Checkbox("Apply Snapping", &m_useSnapSettings);
	ImGui::Checkbox("Local space rotation", &m_localSpaceRotation);
}

void TransformTool::drawOverlay(const GameDrawSets& drawSets) {
	getCore()->drawGizmo(drawSets.rdest, m_gizmo, drawSets.drawCamera->getProjView());
}


void TransformTool::prepareForEditing() {
}

bool TransformTool::interact(
    const InputState& is, const vec3f& rayOrigin, const vec3f rayDir, const float customScale, bool* hasClickedAway) {
	// Interact with the gizmo.

	if (is.IsKeyPressed(Key::Key_LCtrl) || is.IsKeyReleased(Key::Key_LCtrl)) {
		m_useSnapSettings = !m_useSnapSettings;
	}

	GizmoInteractArgs const args(&is, m_useSnapSettings ? &m_snapSettings : nullptr, Ray(rayOrigin, rayDir), customScale);
	GizmoInteractResult const gizmoRes = m_gizmo.interact(args);

	if (hasClickedAway) {
		*hasClickedAway = gizmoRes.userClickedAway;
	}

	// Apply the edited trasform to the nodes:
	if (perItemData.size() == 1) {
		const transf3d transform = m_gizmo.getEditedTransform();
		perItemData[0].editedTransform = transform;
	} else {
		const vec3f gizmoInitPos = m_gizmo.getInitalTransform().p;
		const transf3d gizmoTransform = m_gizmo.getTransformDiff();

		for (int t = 0; t < int(perItemData.size()); ++t) {
			transf3d& objectTrasform = perItemData[t].editedTransform;
			objectTrasform = perItemData[t].initialTasform;

			if (m_gizmo.getMode() == Gizmo3D::Mode_Translation) {
				objectTrasform.p += gizmoTransform.p;
			} else if (m_gizmo.getMode() == Gizmo3D::Mode_Rotation) {
				if (!m_localSpaceRotation) {
					// Rotate the position around the gizmo.
					const vec3f posOffset = objectTrasform.p - gizmoInitPos;
					const float dist = (objectTrasform.p - gizmoInitPos).lengthSqr();

					// if (dist > 1e-6f) {
					objectTrasform.p = gizmoInitPos + mat_mul_pos(mat4f::getRotationQuat(gizmoTransform.r), posOffset);
					//}
				}

				objectTrasform.r = gizmoTransform.r * objectTrasform.r;

			} else if (m_gizmo.getMode() == Gizmo3D::Mode_Scaling) {
				const vec3f posOffset = objectTrasform.p - gizmoInitPos;
				objectTrasform.s = gizmoTransform.s * objectTrasform.s;
				objectTrasform.p = gizmoInitPos + gizmoTransform.s * (objectTrasform.p - gizmoInitPos);
			} else if (m_gizmo.getMode() == Gizmo3D::Mode_ScaleVolume) {
				const vec3f posOffset = objectTrasform.p - gizmoInitPos;
				objectTrasform.s = gizmoTransform.s * objectTrasform.s;
				objectTrasform.p = gizmoTransform.p + gizmoTransform.s * (objectTrasform.p - gizmoInitPos);
			}
		}
	}

	return gizmoRes.isDone;
}

} // namespace sge
