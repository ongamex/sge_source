#pragma once

#include "sge_engine/GameDrawer/IRenderItem.h"

namespace sge {

struct TraitModel;
struct EvaluatedModel;

struct TraitModelRenderItem : public IRenderItem {
	TraitModel* traitModel = nullptr;
	int iModel = -1;
	const EvaluatedModel* evalModel = nullptr;
	int iEvalNode = -1; // The mesh to be rendered from the model.
	int iEvalNodeMechAttachmentIndex = -1;

	Material mtl;
};

} // namespace sge
