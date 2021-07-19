#include "AParticles.h"
#include "sge_engine/Actor.h"
#include "sge_engine/windows/PropertyEditorWindow.h"
#include "sge_utils/utils/strings.h"

namespace sge {

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
DefineTypeId(AParticlesSimple, 20'03'02'0052);
// clang-format off

ReflBlock() {
	// AParticlesSimple
	ReflAddActor(AParticlesSimple)
		ReflMember(AParticlesSimple, m_particles)
	;
}
// clang-format on

AABox3f AParticlesSimple::getBBoxOS() const {
	return m_particles.getBBoxOS();
}

void AParticlesSimple::create() {
	registerTrait(m_particles);
	registerTrait(*static_cast<IActorCustomAttributeEditorTrait*>(this));
	registerTrait(m_traitViewportIcon);
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/AParticles.png", true);
}

void AParticlesSimple::doAttributeEditor(GameInspector* inspector) {
	MemberChain chain;

	const TypeDesc* const tdAParticles = typeLib().find<AParticlesSimple>();
	const TypeDesc* const tdPartTrait = typeLib().find<TraitParticlesSimple>();
	const TypeDesc* const tdPartDesc = typeLib().find<ParticleGroupDesc>();
	chain.add(tdAParticles->findMember(&AParticlesSimple::m_particles));
#define CHAIN_ADD(member) chain.add(tdPartTrait->findMember(&TraitParticlesSimple::member));
#define CHAIN_POP() chain.pop()

	CHAIN_ADD(m_isEnabled);
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	CHAIN_POP();

	CHAIN_ADD(m_isInWorldSpace);
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	CHAIN_POP();

	// Draw the select particle group combo
	const std::string* pSelectedGroupName = nullptr;

	if (m_uiSelectedGroup >= 0 && m_uiSelectedGroup < m_particles.m_pgroups.size()) {
		pSelectedGroupName = &m_particles.m_pgroups[m_uiSelectedGroup].m_name;
	}

	bool forceOpenGroupHeader = false;

	if (ImGui::BeginCombo("Group", pSelectedGroupName ? pSelectedGroupName->c_str() : "")) {
		for (int iGrp = 0; iGrp < int(m_particles.m_pgroups.size()); ++iGrp) {
			ParticleGroupDesc& pdesc = m_particles.m_pgroups[iGrp];
			if (ImGui::Selectable(pdesc.m_name.c_str(), m_uiSelectedGroup == iGrp)) {
				m_uiSelectedGroup = iGrp;
				forceOpenGroupHeader = true;
			}
		}

		if (ImGui::Selectable("+ Add New", false)) {
			// TODO Member change.
			m_particles.m_pgroups.emplace_back(ParticleGroupDesc());
			m_particles.m_pgroups.back().m_name = string_format("New Group %d", m_particles.m_pgroups.size());
		}

		ImGui::EndCombo();
	}

	// Do the group desc user interface.
	if (m_uiSelectedGroup >= 0 && m_uiSelectedGroup < m_particles.m_pgroups.size()) {
		ParticleGroupDesc& pdesc = m_particles.m_pgroups[m_uiSelectedGroup];

		if (forceOpenGroupHeader) {
			ImGui::SetNextItemOpen(true);
		}
		if (ImGui::CollapsingHeader(pdesc.m_name.c_str())) {
			chain.add(tdPartTrait->findMember(&TraitParticlesSimple::m_pgroups), m_uiSelectedGroup);

			chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_name));
			ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
			chain.pop();

			ImGui::Separator();

			chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_timeMultiplier));
			ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
			chain.pop();


			chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_lifecycle));
			ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
			chain.pop();


			if (ImGui::CollapsingHeader("Visualization")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_visMethod));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				if (pdesc.m_visMethod == ParticleGroupDesc::vis_model3D) {
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particleModel));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();
				}

				if (pdesc.m_visMethod == ParticleGroupDesc::vis_sprite) {
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particlesSprite));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spriteGrid));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spriteFPS));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();

					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spritePixelsPerUnit));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();
				}
			}

			// Birth.
			if (ImGui::CollapsingHeader("Birth")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_birthType));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				if (pdesc.m_birthType == ParticleGroupDesc::birthType_constant) {
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spawnRate));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();
				} else if (pdesc.m_birthType == ParticleGroupDesc::birthType_fromCurve) {
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particleSpawnOnSecond));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();
				}

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particleLife));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}

			if (ImGui::CollapsingHeader("Spawning Shape")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spawnShape));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_shapeRadius));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}

			if (ImGui::CollapsingHeader("Scale")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_spawnScale));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}

			if (ImGui::CollapsingHeader("Velocity")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_veclotiyType));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				if (pdesc.m_veclotiyType == ParticleGroupDesc::velocityType_cone) {
					chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_coneHalfAngle));
					ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
					chain.pop();
				}

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_particleVelocity));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}


			if (ImGui::CollapsingHeader("Forces")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_gravity));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_xzDrag));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_yDrag));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}

			if (ImGui::CollapsingHeader("Noise")) {
				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_useNoise));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_noiseVelocityStr));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_noiseScaleStr));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_noiseScaling));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();

				chain.add(tdPartDesc->findMember(&ParticleGroupDesc::m_noiseDetail));
				ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
				chain.pop();
			}

			// Delete button.
			ImGui::Separator();

			if (ImGui::Button("Remove")) {
				m_particles.m_pgroups.erase(m_particles.m_pgroups.begin() + m_uiSelectedGroup);
				m_uiSelectedGroup--;
			}
		}
	}
}

void AParticlesSimple::postUpdate(const GameUpdateSets& u) {
	m_particles.update(u);
}

} // namespace sge
