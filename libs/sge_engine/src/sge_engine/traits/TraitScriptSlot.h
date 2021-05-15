#pragma once

#include "sge_engine/GameObject.h"
#include <string>

namespace sge {
struct GameInspector;

DefineTypeIdExists(TraitScriptSlot);
struct SGE_ENGINE_API TraitScriptSlot : public Trait {
	SGE_TraitDecl_Full(TraitScriptSlot);

	void addSlot(const char* const name, TypeId possibleType);

	struct ScriptSlot {
		/// The visual name of the slot.
		std::string slotName;

		/// The if of the Script object that is assigned.
		ObjectId scriptObjectId;

		///
		std::vector<TypeId> possibleTypesIncludeList;
	};

	std::vector<ScriptSlot> slots;
};

void TraitScriptSlot_doProperyEditor(GameInspector& inspector, GameObject* const actor, MemberChain chain);

} // namespace sge
