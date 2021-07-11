#pragma once

#include "sge_engine_api.h"
#include <vector>

namespace sge {

struct EvaluatedModel;
struct CollsionShapeDesc;
struct TraitModel;

SGE_ENGINE_API bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const EvaluatedModel& evaluatedMode);
SGE_ENGINE_API bool addCollisionShapeBasedOnModel(std::vector<CollsionShapeDesc>& shapeDescs, const char* modelAssetPath);
SGE_ENGINE_API bool addCollisionShapeBasedOnTraitModel(std::vector<CollsionShapeDesc>& shapeDescs, TraitModel& traitModel);

} // namespace sge
