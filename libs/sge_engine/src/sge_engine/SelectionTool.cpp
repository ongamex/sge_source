#include "imgui/imgui.h"

#include "Actor.h"
#include "GameDrawer.h"
#include "GameInspector.h"
#include "sge_core/QuickDraw.h"
#include "sge_utils/math/Box.h"

#include "SelectionTool.h"

namespace sge {

int SelectionToolModeActors::getNumItems(GameInspector* inspector) {
	int result = 0;

	// CAUTION: We realy on the same ordering in the whole SelectionToolModeActors.
	for (const auto& actors : inspector->m_world->playingObjects) {
		result += int(actors.second.size());
	}

	return result;
}

void SelectionToolModeActors::drawItem(GameInspector* inspector, int const itemIndex, const GameDrawSets& gameDrawSets) {
	Actor* const actor = itemIndexToActor(inspector, itemIndex);
	if (actor != nullptr) {
		gameDrawSets.gameDrawer->drawActor(gameDrawSets, editMode_actors, actor, itemIndex, drawReason_selectionTool);
	}
}

void SelectionToolModeActors::setSelected(GameInspector* inspector,
                                          int const itemIndex,
                                          bool isSelected,
                                          bool promoteToPrimaryIfAlreadySelected,
                                          bool autoSelectHierarchyUnderActorInActorMode,
                                          bool autoSelectHierarchyAboveActorInActorMode,
                                          bool autoSelectAllRelativesInActorMode) {
	const Actor* const actor = itemIndexToActor(inspector, itemIndex);

	if (actor) {
		if (isSelected || promoteToPrimaryIfAlreadySelected)
			inspector->select(actor->getId(), inspector->isSelected(actor->getId()));
		else
			inspector->deselect(actor->getId());

		if (autoSelectAllRelativesInActorMode) {
			vector_set<ObjectId> allRelatives;
			inspector->getWorld()->getAllRelativesOf(allRelatives, actor->getId());

			for (ObjectId const id : allRelatives) {
				if (isSelected)
					inspector->select(id);
				else
					inspector->deselect(id);
			}
		} else {
			if (autoSelectHierarchyUnderActorInActorMode) {
				vector_set<ObjectId> actorsUnder;
				inspector->getWorld()->getAllChildren(actorsUnder, actor->getId());

				for (ObjectId const id : actorsUnder) {
					if (isSelected)
						inspector->select(id);
					else
						inspector->deselect(id);
				}
			}

			if (autoSelectHierarchyAboveActorInActorMode) {
				vector_set<ObjectId> actorsAbove;
				inspector->getWorld()->getAllParents(actorsAbove, actor->getId());

				for (ObjectId const id : actorsAbove) {
					if (isSelected)
						inspector->select(id);
					else
						inspector->deselect(id);
				}
			}
		}
	}
}

Actor* SelectionToolModeActors::itemIndexToActor(GameInspector* inspector, int itemIndex) {
	for (const auto& objects : inspector->m_world->playingObjects) {
		size_t const sz = objects.second.size();
		if (itemIndex < sz) {
			return dynamic_cast<Actor*>(objects.second[itemIndex]);
		}

		itemIndex -= int(sz);
	}

	// In theroy we shouldn't look for game objects that doesn't exist.
	sgeAssert(false);
	return nullptr;
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void SelectionToolModePoints::begin(GameInspector* inspector) {
	items = decltype(items)();

	for (const auto& objects : inspector->m_world->playingObjects)
		for (const GameObject* const object : objects.second) {
			const Actor* actor = object->getActor();
			if (actor) {
				int numItems = actor->getNumItemsInMode(editMode_points);
				items.reserve(items.size() + numItems);

				for (int t = 0; t < numItems; ++t) {
					items.emplace_back(SelectedItem(editMode_points, actor->getId(), t));
				}
			}
		}
}


void SelectionToolModePoints::end(GameInspector* UNUSED(inspector)) {
	items = decltype(items)();
}

int SelectionToolModePoints::getNumItems(GameInspector* UNUSED(inspector)) {
	return int(items.size());
}

void SelectionToolModePoints::drawItem(GameInspector* inspector, int const itemIndex, const GameDrawSets& gameDrawSets) {
	const Actor* const actor = inspector->m_world->getActorById(items[itemIndex].objectId);
	if (actor) {
		gameDrawSets.gameDrawer->drawItem(gameDrawSets, items[itemIndex], false);
	}
}

void SelectionToolModePoints::setSelected(GameInspector* inspector,
                                          int const itemIndex,
                                          bool isSelected,
                                          bool promoteToPrimaryIfAlreadySelected,
                                          bool UNUSED(autoSelectHierarchyUnderActorInActorMode),
                                          bool UNUSED(autoSelectHierarchyAboveActorInActorMode),
                                          bool UNUSED(autoSelectAllRelativesInActorMode)) {
	// Primary selections here are ignored.
	(void)(promoteToPrimaryIfAlreadySelected);

	// TODO: HACK: Optimize
	for (int t = 0; t < inspector->m_selection.size(); ++t) {
		if (inspector->m_selection[t] == items[itemIndex]) {
			if (isSelected)
				return;
			else {
				inspector->m_selection.erase(inspector->m_selection.begin() + t);
				inspector->m_selectionChangeIdx++;
			}
		}
	}

	if (isSelected) {
		inspector->m_selection.emplace_back(items[itemIndex]);
		inspector->m_selectionChangeIdx++;
	}
}

//--------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------
void SelectionTool::onSetActive(GameInspector* const UNUSED(inspector)) {
	m_isSelecting = false;
}

void SelectionTool::onUI(GameInspector* UNUSED(inspector)) {
	ImGui::Checkbox("Select Children", &m_autoSelectAllChildren);
	ImGui::Checkbox("Select Parents", &m_autoSelectAllParents);
	ImGui::Checkbox("Select All Relatives", &m_autoSelectAllRelatives);

#if 0
	if(m_renderTarget.IsResourceValid()) {
		void* tex = m_renderTarget->getRenderTarget(0);
		ImGui::Image(tex, ImVec2(128, 128));
	}
#endif
}

InspectorToolResult SelectionTool::updateTool(GameInspector* const inspector,
                                              bool isAllowedToTakeInput,
                                              const InputState& is,
                                              const GameDrawSets& drawSets) {
	InspectorToolResult result;

	m_lastUpdateCursorPos = is.GetCursorPos();

	if (isAllowedToTakeInput == false) {
		m_isSelecting = false;
		return result;
	}

	if (is.isCursorRelative()) {
		m_isSelecting = false;
	} else {
		if (is.IsKeyPressed(Key::Key_MouseLeft)) {
			m_pickingPointStartCS = is.GetCursorPos();
			m_isSelecting = true;
		}
	}

	if (m_isSelecting && is.IsKeyReleased(Key::Key_MouseLeft)) {
		// Determine the picking region
		const vec2f pickingPointEndCS = is.GetCursorPos();

		AABox2f selectionRectCS;
		selectionRectCS.expand(m_pickingPointStartCS);
		selectionRectCS.expand(pickingPointEndCS);

		// std::vector<int> affectedIndices;
		// std::vector<ObjectId> affectedActors;
		//
		// Perform the picking for every different type.
		performPicking(inspector, selectionRectCS, drawSets, is.IsKeyDown(Key::Key_LCtrl), is.IsKeyDown(Key::Key_LShift));

		m_isSelecting = false;
	}

	result.propagateInput = !m_isSelecting;

	return result;
}

void SelectionTool::onCancel(GameInspector* UNUSED(inspector)) {
	m_isSelecting = false;
}

void SelectionTool::drawOverlay(const GameDrawSets& drawSets) {
	if (m_isSelecting) {
		SGEDevice* const sgedev = drawSets.rdest.sgecon->getDevice();

		AABox2f rect;
		rect.expand(m_pickingPointStartCS);
		rect.expand(m_lastUpdateCursorPos);

		BlendStateDesc const bsd = BlendStateDesc::GetDefaultBackToFrontAlpha();
		drawSets.quickDraw->drawRect(drawSets.rdest, rect.min.x, rect.min.y, rect.size().x, rect.size().y, vec4f(0.f, 0.f, 0.f, 0.6f),
		                             sgedev->requestBlendState(bsd));
	}
}

void SelectionTool::performPicking(
    GameInspector* inspector, AABox2f selectionRectCS, const GameDrawSets& drawSets, bool ctrlDown, bool shiftDown) {
	const bool singleClickSelection = selectionRectCS.halfDiagonal().length() <= 3.f;

	if (singleClickSelection) {
		vec2f const center = selectionRectCS.center();
		selectionRectCS.setEmpty();
		selectionRectCS.expand(center - vec2f(2.5f, 2.5f));
		selectionRectCS.expand(center + vec2f(2.5f, 2.5f));
	} else {
		// Add an extra pixel on every side to avoid empty render targets.
		selectionRectCS.expand(selectionRectCS.max + vec2f(1.f, 1.f));
		selectionRectCS.expand(selectionRectCS.min - vec2f(1.f, 1.f));
	}

	// Determing size of the picking render target.
	SGEContext* const sgecon = drawSets.rdest.sgecon;
	SGEDevice* const sgedev = sgecon->getDevice();

	// Create the picking frame target.
	if (m_renderTarget == NULL) {
		m_renderTarget = sgedev->requestResource<FrameTarget>();
		sgeAssert(m_renderTarget != NULL);
	}

	vec2f const pickingTargetSize = selectionRectCS.size();

	if (!m_renderTarget->isSizeEqual(pickingTargetSize) && pickingTargetSize != vec2f(0.f, 0.f)) {
		m_renderTarget->create2D((int)pickingTargetSize.x, (int)pickingTargetSize.y);
	}

	vec2f const viewportSize = drawSets.rdest.viewport.getSizeFloats();
	vec2f minPickingNDC = vec2f(selectionRectCS.min.x, selectionRectCS.max.y) / viewportSize;
	vec2f maxPickingNDC = vec2f(selectionRectCS.max.x, selectionRectCS.min.y) / viewportSize;

	minPickingNDC.x = 2.f * minPickingNDC.x - 1.f;
	minPickingNDC.y = -2.f * minPickingNDC.y + 1.f;

	maxPickingNDC.x = 2.f * maxPickingNDC.x - 1.f;
	maxPickingNDC.y = -2.f * maxPickingNDC.y + 1.f;

	// Compute the picking region in NDC and construct the projection matrix.
	bool const isOrthographic = drawSets.drawCamera->getProj().data[2][3] == 0.f;
	mat4f const projInv = drawSets.drawCamera->getProj().inverse();

	vec4f minPickingNearPlane = projInv * vec4f(minPickingNDC, kNDCNear, 1.f);
	minPickingNearPlane = minPickingNearPlane / minPickingNearPlane.w;

	vec4f maxPickingNearPlane = projInv * vec4f(maxPickingNDC, kNDCNear, 1.f);
	maxPickingNearPlane = maxPickingNearPlane / maxPickingNearPlane.w;

	// Compute picking projection matrix.
	float const l = minPickingNearPlane.x;
	float const r = maxPickingNearPlane.x;

	float const t = maxPickingNearPlane.y;
	float const b = minPickingNearPlane.y;

	// TODO: Near and far plane of the projection matrix are currently hardcoded.
	RawCamera pickingCamera(drawSets.drawCamera->getCameraPosition(), drawSets.drawCamera->getView(),
	                        isOrthographic ? mat4f::getOrthoRH(l, r, b, t, 0.1f, 10000.f, kIsTexcoordStyleD3D)
	                                       : mat4f::getPerspectiveOffCenterRH(l, r, b, t, 0.1f, 10000.f, kIsTexcoordStyleD3D));

	GameDrawSets pickingDrawSets;
	pickingDrawSets.setup(sgecon, m_renderTarget, drawSets.quickDraw, &pickingCamera, drawSets.gameCamera, drawSets.gameDrawer);

	GpuHandle<Query> query = sgecon->getDevice()->requestResource<Query>();
	query->create(QueryType::NumSamplesPassedDepthStencilTest);

	sgecon->clearColor(m_renderTarget, -1, vec4f(0.f, 0.f, 0.f, 1.f).data);
	sgecon->clearDepth(m_renderTarget, 1.f);

	SelectionToolMode* const toolMode =
	    inspector->editMode == editMode_actors ? (SelectionToolMode*)&m_modeActors : (SelectionToolMode*)&m_modePoints;


	if (!ctrlDown && !shiftDown) {
		inspector->deselectAll();
	}

	toolMode->begin(inspector);

	std::vector<int> affectedItemIndices;

	const int numItems = toolMode->getNumItems(inspector);
	for (int iItem = 0; iItem < numItems; ++iItem) {
		// Draw the item and check if any pixels made it to the final target.
		sgecon->beginQuery(query);
		toolMode->drawItem(inspector, iItem, pickingDrawSets);
		sgecon->endQuery(query);

		uint64 numPixelsThatMadeIt;
		while (!sgecon->getQueryData(query, numPixelsThatMadeIt))
			;

		if (numPixelsThatMadeIt != 0) {
			if (singleClickSelection) {
				affectedItemIndices.resize(1);
				affectedItemIndices[0] = iItem;
			} else {
				affectedItemIndices.push_back(iItem);
			}
		}

		if (!singleClickSelection) {
			sgecon->clearDepth(m_renderTarget, 1.f);
		}
	}

	for (int const idx : affectedItemIndices) {
		toolMode->setSelected(inspector, idx, !ctrlDown, singleClickSelection && shiftDown, m_autoSelectAllChildren, m_autoSelectAllParents,
		                      m_autoSelectAllRelatives);
	}

	toolMode->end(inspector);

	return;
}


} // namespace sge
