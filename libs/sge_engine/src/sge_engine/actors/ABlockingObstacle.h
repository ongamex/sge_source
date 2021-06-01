#pragma once

#include "sge_renderer/renderer/renderer.h"

#include "sge_engine/Actor.h"
#include "sge_engine/Physics.h"
#include "sge_engine/RigidBodyEditorConfig.h"
#include "sge_engine/TerrainGenerator.h"
#include "sge_engine/traits/TraitCustomAE.h"
#include "sge_engine/traits/TraitModel.h"
#include "sge_engine/traits/TraitRigidBody.h"

namespace sge {

struct GameInspector;

//--------------------------------------------------------------------
// ABlockingObstacle
//--------------------------------------------------------------------
struct SGE_ENGINE_API ABlockingObstacle final : public Actor, public IActorCustomAttributeEditorTrait {
	ABlockingObstacle()
	    : m_textureX(AssetType::Texture2D)
	    , m_textureY(AssetType::Texture2D) {}

	void create() final;
	void onPlayStateChanged(bool const isStartingToPlay) override;
	void postUpdate(const GameUpdateSets& updateSets) final;
	AABox3f getBBoxOS() const final;

	// IActorCustomAttributeEditorTrait
	void doAttributeEditor(GameInspector* inspector) override;

  public:
	SimpleObstacleDesc targetDesc;
	SimpleObstacleDesc currentDesc;

	RigidBodyPropertiesConfigurator m_rbPropConfig;

	AssetProperty m_textureX;
	float m_textureXScale = 1.f;
	AssetProperty m_textureY;
	float m_textureYScale = 1.f;

	// TODO: move that away form here!
	AABox3f boundingBox;
	GpuHandle<Buffer> vertexBuffer;
	GpuHandle<Buffer> indexBuffer;
	Geometry geometry;
	Material material;
	int numVerts = 0;
	int numIndices = 0;
	TraitRigidBody m_traitRB;
};

} // namespace sge
