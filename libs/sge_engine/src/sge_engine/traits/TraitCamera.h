#pragma once

#include "sge_engine/Actor.h"
#include "sge_engine/Camera.h"

namespace sge {

DefineTypeIdExists(TraitCamera);
struct SGE_ENGINE_API TraitCamera : public Trait {
	SGE_TraitDecl_BaseFamily(TraitCamera);
	virtual ICamera* getCamera() = 0;
	virtual const ICamera* getCamera() const = 0;
};
DefineTypeIdInline(TraitCamera, 20'03'06'0002);

} // namespace sge