#pragma once

#include "sge_utils/utils/optional.h"

#include "sge_utils/math/MultiCurve2D.h"
#include "sge_utils/math/Random.h"
#include "sge_utils/math/Rangef.h"

#include "sge_engine/Actor.h"
#include "sge_engine/AssetProperty.h"

namespace sge {

struct ICamera;
//--------------------------------------------------------------
// ParticleGroupDesc
//--------------------------------------------------------------
struct ParticleGroupDesc {
	enum BirthType : int {
		birthType_constant,
		birthType_fromCurve,
	};

	enum SpawnShape : int {
		spawnShape_sphere,
		spawnShape_box,
		spawnShape_rect,
		spawnShape_disc,
	};

	enum VelocityType : int {
		velocityType_directional,
		velocityType_radial,
		velocityType_cone,
	};

	enum Visualization { vis_model3D, vis_sprite };

  public:
	std::string m_name;
	float m_timeMultiplier = 1.f;
	float m_lifecycle = 0.f;
	bool m_cleanLifecycle = false; // True if the lifecylcle should delete all of it's remaining particles at the end.

	// Model and sprites.
	Visualization m_visMethod = vis_model3D;
	AssetProperty m_particleModel = AssetProperty(AssetType::Model);
	AssetProperty m_particlesSprite = AssetProperty(AssetType::TextureView);
	vec2i m_spriteGrid = vec2i(1); // The number of sub images in the specified texture by x and y. They go row-wise left to right.
	float m_spritePixelsPerUnit = 32.f;
	float m_spriteFPS = 30.f;

	BirthType m_birthType = birthType_constant;
	int m_spawnRate = 10; // Number of spawned particle per second.
	MultiCurve2D m_particleSpawnOnSecond;

	Rangef m_particleLife = Rangef(1.f);

	// Shape
	SpawnShape m_spawnShape = spawnShape_sphere;
	float m_shapeRadius = 1.f;

	// Scale
	Rangef m_spawnScale = Rangef(1.f);

	// Velocity
	VelocityType m_veclotiyType = velocityType_radial;
	Rangef m_particleVelocity = Rangef(1.f);
	float m_coneHalfAngle = deg2rad(15.f);

	// Forces
	vec3f m_gravity = vec3f(0.f, -9.8f, 0.f);
	float m_xzDrag = 0.05f;
	float m_yDrag = 0.0f;

	// Noise over velocity, size and anything else.
	bool m_useNoise = false;
	float m_noiseVelocityStr = 1.f;
	float m_noiseScaleStr = 0.f;
	float m_noiseScaling = 1.f;
	int m_noiseDetail = 5; // Translates to number of octaves.
	                       // bool m_useNoiseForVelocity = false;
};

//--------------------------------------------------------------
// ParticleGroupState
//--------------------------------------------------------------
struct SGE_ENGINE_API ParticleGroupState {
	struct ParticleState {
		vec3f pos = vec3f(0.f);
		vec3f velocity = vec3f::getAxis(0);
		float scale = 1.f;
		float m_maxLife = 0.f;

		float m_timeSpendAlive = 0.f;
		float m_fSpriteIndex = 0.f; // The sprites are used for rendering this points to the sub-image being used for visualization.

		bool isDead() const { return m_timeSpendAlive >= m_maxLife; }
	};

	// Sprite visulaization mode
	struct SpriteRendData {
		GpuHandle<Buffer> vertexBuffer;
		// GpuHandle<Buffer> indexBuffer;

		Geometry geometry;
		Material material;
	};

	mat4f getParticlesToWorldMtx() const { return m_isInWorldSpace ? mat4f::getIdentity() : m_n2w; }

  private:
	AABox3f m_bboxFromLastUpdate;
	Random m_rnd;

	float m_lastSpawnTime = 0.f;
	float m_timeRunning = 0.f; // Time in seconds the simulation went running.
	int m_particlesSpawnedSoFar = 0;

	std::vector<ParticleState> m_particles;
	Optional<PerlinNoise3D> m_noise;

	std::vector<Pair<vec2f, vec2f>> spriteFramesUVCache;

	Optional<SpriteRendData> spriteRenderData;
	struct SortingData {
		int index;
		float distanceAlongRay = FLT_MAX;
	};
	std::vector<SortingData> indicesForSorting;

	mat4f m_n2w = mat4f::getIdentity();
	bool m_isInWorldSpace = true;

  public:
	void update(bool isInWorldSpace, const mat4f node2world, const ParticleGroupDesc& spawnDesc, float dt);

	/// @param camera the camera to be used to billboard the particles.
	SpriteRendData* computeSpriteRenderData(SGEContext& sgecon, const ParticleGroupDesc& pdesc, const ICamera& camera);

	const std::vector<ParticleState>& getParticles() const { return m_particles; }

	/// Returns the bounding box in the space they are being simulated (world or node).
	AABox3f getBBox() const { return m_bboxFromLastUpdate; }
};

//--------------------------------------------------------------
// TraitParticles
//--------------------------------------------------------------
DefineTypeIdExists(TraitParticles);
struct SGE_ENGINE_API TraitParticles : public Trait {
	SGE_TraitDecl_Full(TraitParticles);

	void update(const GameUpdateSets& u);
	AABox3f getBBoxOS() const;

  public:
	bool m_isEnabled = true;
	bool m_isInWorldSpace = false; // if true, the particles are in world space, false is node space of the owning actor.
	std::vector<ParticleGroupDesc> m_pgroups;
	std::unordered_map<std::string, ParticleGroupState> m_pgroupState;
};

//--------------------------------------------------------------
// TriatParticles2
//--------------------------------------------------------------
DefineTypeIdExists(TraitParticles2);
struct SGE_ENGINE_API TraitParticles2 : public Trait {
	SGE_TraitDecl_BaseFamily(TraitParticles2);

	struct ParticleGroup {
		struct ParticleData {
			vec3f position = vec3f(0.f);
			vec3f velocity = vec3f(0.f);
			vec4f tint = vec4f(1.f);
			quatf spin = quatf::getIdentity();
			vec3f angularSpeedAxis = vec3f(0.f);
			float scale = 1.f;
			float angularSpeed = 0.f;
			float age = 0.f;
			int frameIndex = 0;
			int isDead = false;
			int tag = 0;
		};

		bool isInWorldSpace = true;
		std::vector<ParticleData> allParticles;
		std::shared_ptr<Asset> spriteTexture;
		vec2i spriteFramsCount = vec2i(1); // the number of images in X and Y direction.
		AABox3f bbox;                      // the bounding box of the particles in world or in object space.
	};

	virtual int getNumPGroups() const { return 0; }
	virtual ParticleGroup* getPGroup(const int UNUSED(idx)) { return nullptr; }
	virtual void update(const GameUpdateSets& UNUSED(u)) {}
};

struct SGE_ENGINE_API ParticleRenderDataGen {
	struct SortingData {
		int index;
		float distanceAlongRay = FLT_MAX;
	};

	struct ParticleVertexData {
		vec3f a_position;
		vec3f a_normal;
		vec4f a_color;
		vec2f a_uv;
	};

  public:
	/// Returns true if there was data ready for rendering and it succeeded.
	bool generate(const TraitParticles2::ParticleGroup& particles, SGEContext& sgecon, const ICamera& camera, const mat4f& n2w);

	std::vector<SortingData> indicesForSorting;
	std::vector<Pair<vec2f, vec2f>> spriteFramesUVCache;
	std::vector<ParticleVertexData> vertexBufferData;

	GpuHandle<Buffer> vertexBuffer;
	Geometry geometry;
	Material material;
};

} // namespace sge
