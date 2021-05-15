#pragma once

#include "sge_engine/Actor.h"

namespace sge {

struct GameInspector;

DefineTypeIdExists(IActorCustomAttributeEditorTrait);
struct SGE_ENGINE_API IActorCustomAttributeEditorTrait : public Trait {
	SGE_TraitDecl_BaseFamily(IActorCustomAttributeEditorTrait)

	    virtual void doAttributeEditor(GameInspector* inspector) = 0;
};
DefineTypeIdInline(IActorCustomAttributeEditorTrait, 20'03'06'0006);

} // namespace sge
