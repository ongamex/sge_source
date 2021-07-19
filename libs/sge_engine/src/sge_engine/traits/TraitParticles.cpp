#include "sge_engine/traits/TraitParticles.h"
#include "sge_engine/Camera.h"
#include "sge_engine/GameDrawer/RenderItems/TraitParticlesRenderItem.h"
#include "sge_engine/GameWorld.h"

namespace sge {

// clang-format off
DefineTypeId(TraitParticlesSimple, 20'03'02'0051);
ReflBlock() {
	ReflAddType(TraitParticlesSimple)
		ReflMember(TraitParticlesSimple, m_isEnabled)
		ReflMember(TraitParticlesSimple, m_isInWorldSpace)
		ReflMember(TraitParticlesSimple, m_pgroups)
	;
}

DefineTypeId(ParticleGroupDesc::SpawnShape, 20'03'02'0046);
ReflBlock() {
	ReflAddType(ParticleGroupDesc::SpawnShape)
		ReflEnumVal(ParticleGroupDesc::spawnShape_sphere, "sphere")
		ReflEnumVal(ParticleGroupDesc::spawnShape_box, "box")
		ReflEnumVal(ParticleGroupDesc::spawnShape_rect, "rect")
		ReflEnumVal(ParticleGroupDesc::spawnShape_disc, "disc")
	;
}

DefineTypeId(ParticleGroupDesc::BirthType, 20'03'02'0047);
ReflBlock() {
	ReflAddType(ParticleGroupDesc::BirthType)
		ReflEnumVal(ParticleGroupDesc::birthType_constant, "Constant")
		ReflEnumVal(ParticleGroupDesc::birthType_fromCurve, "Curve")
	;
}

DefineTypeId(ParticleGroupDesc::Visualization, 20'03'02'0048);
ReflBlock() {
	ReflAddType(ParticleGroupDesc::Visualization)
		ReflEnumVal(ParticleGroupDesc::vis_model3D, "Model")
		ReflEnumVal(ParticleGroupDesc::vis_sprite, "Sprite")
	;
}

DefineTypeId(ParticleGroupDesc::VelocityType, 20'03'06'0007);
ReflBlock() {
	ReflAddType(ParticleGroupDesc::VelocityType) ReflEnumVal(ParticleGroupDesc::velocityType_directional, "directional")
	    ReflEnumVal(ParticleGroupDesc::velocityType_radial, "radial") ReflEnumVal(ParticleGroupDesc::velocityType_cone, "cone");
}


DefineTypeId(ParticleGroupDesc, 20'03'02'0049);
DefineTypeId(std::vector<ParticleGroupDesc>, 20'03'02'0050);

ReflBlock() {
	ReflAddType(ParticleGroupDesc)
		ReflMember(ParticleGroupDesc, m_name)
		ReflMember(ParticleGroupDesc, m_timeMultiplier)
		ReflMember(ParticleGroupDesc, m_birthType)
		ReflMember(ParticleGroupDesc, m_lifecycle)
		ReflMember(ParticleGroupDesc, m_cleanLifecycle)
		ReflMember(ParticleGroupDesc, m_visMethod)
		ReflMember(ParticleGroupDesc, m_particleModel)
		ReflMember(ParticleGroupDesc, m_particlesSprite)
		ReflMember(ParticleGroupDesc, m_spriteGrid)
		ReflMember(ParticleGroupDesc, m_spriteFPS)
		ReflMember(ParticleGroupDesc, m_spritePixelsPerUnit)
		ReflMember(ParticleGroupDesc, m_spawnRate)
		ReflMember(ParticleGroupDesc, m_particleSpawnOnSecond)
		ReflMember(ParticleGroupDesc, m_spawnShape)
		ReflMember(ParticleGroupDesc, m_shapeRadius)
		ReflMember(ParticleGroupDesc, m_spawnScale)
		ReflMember(ParticleGroupDesc, m_veclotiyType)
		ReflMember(ParticleGroupDesc, m_coneHalfAngle).addMemberFlag(MFF_FloatAsDegrees)
		ReflMember(ParticleGroupDesc, m_particleLife)
		ReflMember(ParticleGroupDesc, m_particleVelocity)
		ReflMember(ParticleGroupDesc, m_gravity)
		ReflMember(ParticleGroupDesc, m_xzDrag)
		ReflMember(ParticleGroupDesc, m_yDrag)
		ReflMember(ParticleGroupDesc, m_useNoise)
		ReflMember(ParticleGroupDesc, m_noiseVelocityStr)
		ReflMember(ParticleGroupDesc, m_noiseScaleStr)
		ReflMember(ParticleGroupDesc, m_noiseScaling)
		ReflMember(ParticleGroupDesc, m_noiseDetail)
	;

	ReflAddType(std::vector<ParticleGroupDesc>);
}
// clang-format on

//--------------------------------------------------------------
// ParticleGroupState
//--------------------------------------------------------------
void ParticleGroupState::update(bool isInWorldSpace, const mat4f node2world, const ParticleGroupDesc& pdesc, float dt) {
	m_bboxFromLastUpdate = AABox3f();
	m_isInWorldSpace = isInWorldSpace;
	m_n2w = node2world;

	dt *= pdesc.m_timeMultiplier;

	// Update the Noise if needed.
	if (pdesc.m_useNoise) {
		if (!m_noise.isValid() || m_noise->getNumPointsPerSide() != pdesc.m_noiseDetail) {
			m_noise = PerlinNoise3D(pdesc.m_noiseDetail);
		}
	} else {
		m_noise = NullOptional();
	}

	if (pdesc.m_lifecycle > 0.f && m_timeRunning > pdesc.m_lifecycle) {
		if (pdesc.m_cleanLifecycle) {
			*this = ParticleGroupState();
		} else {
			m_lastSpawnTime = 0.f;
			m_timeRunning = 0.f;
			m_particlesSpawnedSoFar = 0;
		}
	}

	const Optional<mat4f> spawnLocationMtx = m_isInWorldSpace ? Optional<mat4f>(node2world) : NullOptional();

	// Delete all dead particles.
	for (int t = 0; t < int(m_particles.size()); ++t) {
		if (m_particles[t].isDead()) {
			m_particles.erase(m_particles.begin() + t);
			t--;
			continue;
		}
	}

	// Spawn the new particles.
	int numParticleToSpawn = 0;
	if (pdesc.m_birthType == ParticleGroupDesc::birthType_constant) {
		const float spawnRate = float(pdesc.m_spawnRate);
		const int numParticleToSpawnSoFar = int(m_lastSpawnTime * spawnRate);
		const int toBeSpawnedTotalWithThisUpdate = int(m_timeRunning * spawnRate);
		numParticleToSpawn = toBeSpawnedTotalWithThisUpdate - numParticleToSpawnSoFar;
	} else if (pdesc.m_birthType == ParticleGroupDesc::birthType_fromCurve) {
		// Integrate the function.
		float numTotalParticlesExpectedToSpawn = 0.f;
		const float kIntegrationStep = 0.01f;
		for (float time = 0; time < m_timeRunning + dt; time += kIntegrationStep) {
			float fn = std::max(0.f, pdesc.m_particleSpawnOnSecond.sample(time));
			numTotalParticlesExpectedToSpawn += fn * kIntegrationStep;
		}
		numParticleToSpawn = int(numTotalParticlesExpectedToSpawn) - m_particlesSpawnedSoFar;
	}

	if (numParticleToSpawn > 0) {
		m_lastSpawnTime = m_timeRunning;
	}

	for (int t = 0; t < numParticleToSpawn; ++t) {
		vec3f spawnPos = vec3f(0.f);
		switch (pdesc.m_spawnShape) {
			case ParticleGroupDesc::spawnShape_sphere: {
				float u = m_rnd.next01();
				float v = m_rnd.next01();
				float theta = u * two_pi();
				float phi = acosf(2.f * v - 1.f);
				float r = std::max(1e-3f, cbrtf(m_rnd.next01()) * pdesc.m_shapeRadius);
				float sinTheta = sinf(theta);
				float cosTheta = cosf(theta);
				float sinPhi = sinf(phi);
				float cosPhi = cosf(phi);
				spawnPos.x = r * cosPhi;
				spawnPos.y = r * sinPhi * sinTheta;
				spawnPos.z = r * sinPhi * cosTheta;
			} break;
			case ParticleGroupDesc::spawnShape_box: {
				spawnPos.x = m_rnd.next01() * 2.f - 1.f;
				spawnPos.y = m_rnd.next01() * 2.f - 1.f;
				spawnPos.z = m_rnd.next01() * 2.f - 1.f;
			} break;
			case ParticleGroupDesc::spawnShape_disc: {
				float rho = std::max(1e-3f, sqrtf(m_rnd.next01()) * pdesc.m_shapeRadius);
				float theta = m_rnd.next01() * two_pi();
				spawnPos = vec3f(0.f, sin(theta), cosf(theta)) * rho;
			} break;
			case ParticleGroupDesc::spawnShape_rect: {
				spawnPos.x = m_rnd.next01() * 2.f - 1.f;
				spawnPos.y = m_rnd.next01() * 2.f - 1.f;
				spawnPos.z = 0.f;
			} break;
			default: {
				sgeAssert(false && "Unimplemented spawn shape!");
			}
		};

		vec3f velocity = vec3f(1.f, 0.f, 0.f);
		switch (pdesc.m_veclotiyType) {
			case ParticleGroupDesc::velocityType_directional: {
				velocity = vec3f(pdesc.m_particleVelocity.sample(m_rnd.next01()), 0.f, 0.f);
			} break;
			case ParticleGroupDesc::velocityType_radial: {
				velocity = (spawnPos.normalized0()) * (pdesc.m_particleVelocity.sample(m_rnd.next01()));
			} break;
			case ParticleGroupDesc::velocityType_cone: {
				float discSize = sinf(pdesc.m_coneHalfAngle);
				const float angle = m_rnd.nextInRange(two_pi());
				const float radius = m_rnd.nextInRange(discSize);
				const float y = sinf(angle) * radius;
				const float z = -cosf(angle) * radius;
				velocity = vec3f(discSize, y, z).normalized0() * pdesc.m_particleVelocity.sample(m_rnd.next01());
			} break;
			default:
				sgeAssert(false);
		}

		if (spawnLocationMtx.isValid()) {
			spawnPos = mat_mul_pos(spawnLocationMtx.get(), spawnPos);
			velocity = mat_mul_dir(spawnLocationMtx.get(), velocity);
		}

		const float life = (pdesc.m_particleLife.sample(m_rnd.next01()));
		const float scale = (pdesc.m_spawnScale.sample(m_rnd.next01()));
		const ParticleState newParticle = {
		    spawnPos,
		    velocity,
		    scale,
		    life,
		};

		m_particles.emplace_back(newParticle);
	}

	// Update the particles.
	for (ParticleState& particle : m_particles) {
		// Apply gravity and drag.
		particle.velocity -= particle.velocity.x0z() * pdesc.m_xzDrag * dt + particle.velocity.yOnly() * pdesc.m_yDrag * dt;
		particle.velocity += pdesc.m_gravity * dt;

		// Apply vecloty noise.
		if (m_noise.isValid() && pdesc.m_noiseScaling > 1e-6f) {
			const float dPos = 1e-2f;
			float f = 2.f * m_noise->sample(particle.pos / pdesc.m_noiseScaling) - 1.f;
			float fx = 2.f * m_noise->sample(particle.pos / pdesc.m_noiseScaling + vec3f::getAxis(0, dPos) * pdesc.m_noiseScaling) - 1.f;
			float fy = 2.f * m_noise->sample(particle.pos / pdesc.m_noiseScaling + vec3f::getAxis(1, dPos) * pdesc.m_noiseScaling) - 1.f;
			float fz = 2.f * m_noise->sample(particle.pos / pdesc.m_noiseScaling + vec3f::getAxis(2, dPos) * pdesc.m_noiseScaling) - 1.f;
			vec3f noiseVelocity(fx - f, fy - f, fz - f);
			noiseVelocity = normalized0(noiseVelocity) * f * pdesc.m_noiseVelocityStr;

			particle.velocity += noiseVelocity * dt;
		}

		// Apply scaling noise.
		if (m_noise.isValid() && pdesc.m_noiseScaleStr > 1e-6f) {
			float f = 2.f * m_noise->sample(particle.pos * pdesc.m_noiseScaleStr) - 1.f;
			particle.scale += f * pdesc.m_noiseScaleStr * dt;
		}

		particle.m_fSpriteIndex += dt * pdesc.m_spriteFPS;

		// Apply the velocity and acommodate the time change/
		particle.pos += particle.velocity * dt;
		particle.m_timeSpendAlive += dt;
		m_bboxFromLastUpdate.expand(particle.pos);
	}

	m_particlesSpawnedSoFar += numParticleToSpawn;
	m_timeRunning += dt;
}

ParticleGroupState::SpriteRendData*
    ParticleGroupState::computeSpriteRenderData(SGEContext& sgecon, const ParticleGroupDesc& pdesc, const ICamera& camera) {
	struct Vertex {
		vec3f pos = vec3f(0.f);
		vec3f normal = vec3f(1.f, 0.f, 0.f);
		vec2f uv = vec2f(0.f);
	};

	if (m_particles.empty()) {
		return nullptr;
	}

	// Sort the particles along the ray, so the generated vertex buffer have them sorted
	// so they could be blender correctly during rendering.
	vec3f const camPosWs = camera.getCameraPosition();
	vec3f const camLookWs = camera.getCameraLookDir();

	indicesForSorting.resize(m_particles.size());
	for (int t = 0; t < int(indicesForSorting.size()); ++t) {
		indicesForSorting[t].index = t;
		indicesForSorting[t].distanceAlongRay = projectPointOnLine(camPosWs, camLookWs, m_particles[t].pos);
	}

	std::sort(indicesForSorting.begin(), indicesForSorting.end(),
	          [&](const SortingData& a, const SortingData& b) { return a.distanceAlongRay > b.distanceAlongRay; });

	// Obtain the sprite texture and check if it is valid.
	Texture* const sprite = pdesc.m_particlesSprite.getAssetTexture() ? pdesc.m_particlesSprite.getAssetTexture()->tex.GetPtr() : nullptr;
	if (sprite == nullptr) {
		spriteRenderData = NullOptional();
		return nullptr;
	}

	// Compute the sprite sub-images UV regions.
	const int numFrames = std::max(pdesc.m_spriteGrid.volume(), 0);
	if (numFrames != spriteFramesUVCache.size()) {
		float frmUWidth = 1.f / float(pdesc.m_spriteGrid.x);
		float frmVHeight = 1.f / float(pdesc.m_spriteGrid.y);

		spriteFramesUVCache.resize(numFrames);
		for (int iFrame = 0; iFrame < numFrames; ++iFrame) {
			int gridPosX = iFrame % pdesc.m_spriteGrid.x;
			int gridPosY = iFrame / pdesc.m_spriteGrid.x;

			Pair<vec2f, vec2f> uvRegion;
			uvRegion.first = vec2f(gridPosX * frmUWidth, gridPosY * frmVHeight);
			uvRegion.second = vec2f((gridPosX + 1.f) * frmUWidth, (gridPosY + 1.f) * frmVHeight);

			spriteFramesUVCache[iFrame] = uvRegion;
		}
	}

	// Obtain the sprite size in world space.
	const float particleWidthWs = sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;
	const float particleHeightWs = sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;

	const float hx = particleWidthWs * 0.5f;
	const float hy = particleHeightWs * 0.5f;

	std::vector<Vertex> vertices;
	vertices.reserve(int(m_particles.size()) * 6);

	mat4f faceCameraMtx = camera.getView();
	faceCameraMtx.c3 = vec4f(0.f, 0.f, 0.f, 1.f); // kill the translation.
	faceCameraMtx = inverse(faceCameraMtx);       // TODO: Optimize.

	// Generate the vertices so they are oriented towards the camera.
	for (const SortingData& sortedData : indicesForSorting) {
		const ParticleState& p = m_particles[sortedData.index];
		// Compute the transformation that will make the particle face the camera.
		vec3f particlePosRaw = p.pos;
		vec3f particlePosTransformed = (!m_isInWorldSpace) ? m_n2w.transfPos(particlePosRaw) : particlePosRaw;

		mat4f orientationMtx = mat4f::getTranslation(particlePosTransformed) * faceCameraMtx;

		vec2f uv0 = vec2f(0.f);
		vec2f uv1 = vec2f(1.f);
		int frmIndex = int(p.m_fSpriteIndex) % int(spriteFramesUVCache.size());
		if (frmIndex >= 0 && frmIndex < spriteFramesUVCache.size()) {
			uv0 = spriteFramesUVCache[frmIndex].first;
			uv1 = spriteFramesUVCache[frmIndex].second;
		}

		// A template vertices represeting a quad, centered at (0,0,0) with correct size,
		// later we are going to transform these vertices to get the final transform
		// for the vertex buffer.
		const Vertex templateVetex[6] = {
		    Vertex{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), uv0},
		    Vertex{vec3f(+hx, -hy, 0.f), vec3f::getAxis(2), vec2f(uv1.x, uv0.y)},
		    Vertex{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), vec2f(uv1.x, uv1.y)},

		    Vertex{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), vec2f(uv0.x, uv0.y)},
		    Vertex{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), uv1},
		    Vertex{vec3f(-hx, +hy, 0.f), vec3f::getAxis(2), vec2f(uv0.x, uv1.y)},
		};

		for (const Vertex& templateVtx : templateVetex) {
			Vertex vtx = templateVtx;
			vtx.pos *= fabsf(p.scale) * m_n2w.extractUnsignedScalingVector();
			vtx.pos = mat_mul_pos(orientationMtx, vtx.pos);
			vtx.normal = mat_mul_dir(orientationMtx, vtx.normal); // TODO: inverse transpose.
			vertices.push_back(vtx);
		}
	}

	// Generate the Geometry data.
	if (spriteRenderData.isValid() == false) {
		spriteRenderData = SpriteRendData();
	}

	const int strideSizeBytes = sizeof(vertices[0]);
	const int neededVtxBufferByteSize = int(vertices.size()) * strideSizeBytes;

	const bool isVtxBufferCorrectSize =
	    spriteRenderData->vertexBuffer.IsResourceValid() && spriteRenderData->vertexBuffer->getDesc().sizeBytes == neededVtxBufferByteSize;

	if (isVtxBufferCorrectSize == false) {
		BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(neededVtxBufferByteSize, ResourceUsage::Dynamic);
		spriteRenderData->vertexBuffer = sgecon.getDevice()->requestResource<Buffer>();
		spriteRenderData->vertexBuffer->create(vbDesc, vertices.data());
	} else {
		// The vertex buffer is large enough, just update it's contents.
		void* mappedMem = sgecon.map(spriteRenderData->vertexBuffer, Map::WriteDiscard);
		if (mappedMem) {
			memcpy(mappedMem, vertices.data(), neededVtxBufferByteSize);
		}
		sgecon.unMap(spriteRenderData->vertexBuffer);
	}

	VertexDecl vertexDecl[3] = {
	    VertexDecl(0, "a_position", UniformType::Float3, 0),
	    VertexDecl(0, "a_normal", UniformType::Float3, 3 * sizeof(float)),
	    VertexDecl(0, "a_uv", UniformType::Float2, 6 * sizeof(float)),
	};

	VertexDeclIndex vertexDeclIdx = sgecon.getDevice()->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));

	spriteRenderData->geometry =
	    Geometry(spriteRenderData->vertexBuffer, nullptr, nullptr, -1, vertexDeclIdx, false, true, true, false,
	             PrimitiveTopology::TriangleList, 0, 0, strideSizeBytes, UniformType::Unknown, int(vertices.size()));

	spriteRenderData->material.diffuseTexture = sprite;
	return &spriteRenderData.get();
}

//--------------------------------------------------------------
// TraitParticlesSimple
//--------------------------------------------------------------

void TraitParticlesSimple::update(const GameUpdateSets& u) {
	if (u.isGamePaused() || !m_isEnabled) {
		return;
	}

	for (ParticleGroupDesc& desc : m_pgroups) {
		desc.m_particleModel.update();
		desc.m_particlesSprite.update();

		// TODO: handle duplicated names!
		m_pgroupState[desc.m_name].update(m_isInWorldSpace, getActor()->getTransformMtx(), desc, u.dt);
	}
}

AABox3f TraitParticlesSimple::getBBoxOS() const {
	if (m_isInWorldSpace) {
		const mat4f ownerWorld2Object = inverse(getActor()->getTransformMtx());
		AABox3f bbox;
		for (auto& pair : m_pgroupState) {
			bbox.expand(pair.second.getBBox().getTransformed(ownerWorld2Object));
		}
		return bbox;
	} else {
		AABox3f bbox;
		for (auto& pair : m_pgroupState) {
			bbox.expand(pair.second.getBBox());
		}
		return bbox;
	}
}

void TraitParticlesSimple::getRenderItems(std::vector<TraitParticlesSimpleRenderItem>& renderItems) {
	TraitParticlesSimpleRenderItem renderItem;

	renderItem.traitParticles = this;

	// TODO: Is this the best alpha sorting point that we can come up with?
	// TODO: We do not really always need alpha sorting for particles. Add some logic to determine if we need it.
	renderItem.needsAlphaSorting = true;
	renderItem.zSortingPositionWs = mat_mul_pos(getActor()->getTransformMtx(), getBBoxOS().center());

	renderItems.push_back(renderItem);
}


//--------------------------------------------------------------
// TraitParticlesProgrammable
//--------------------------------------------------------------
void TraitParticlesProgrammable::getRenderItems(std::vector<TraitParticlesProgrammableRenderItem>& renderItems) {
	// TODO: add separate render item for each group.

	TraitParticlesProgrammableRenderItem renderItem;

	renderItem.traitParticles = this;

	// TODO: Is this the best alpha sorting point that we can come up with?
	// TODO: We do not really always need alpha sorting for particles. Add some logic to determine if we need it.
	renderItem.needsAlphaSorting = false;
	

	
	//renderItem.zSortingPositionWs = mat_mul_pos(getActor()->getTransformMtx(), getBBoxOS().center());



	renderItems.push_back(renderItem);
}

//--------------------------------------------------------------
// ParticleRenderDataGen
//--------------------------------------------------------------
DefineTypeId(TraitParticlesProgrammable, 20'11'23'0001);
bool ParticleRenderDataGen::generate(const TraitParticlesProgrammable::ParticleGroup& particles,
                                     SGEContext& sgecon,
                                     const ICamera& camera,
                                     const mat4f& n2w) {
	if (particles.allParticles.empty()) {
		return false;
	}

	// Sort the particles along the ray, so the generated vertex buffer have them sorted
	// so they could be blender correctly during rendering.
	vec3f const camPosWs = camera.getCameraPosition();
	vec3f const camLookWs = camera.getCameraLookDir();

	indicesForSorting.resize(particles.allParticles.size());
	for (int t = 0; t < int(indicesForSorting.size()); ++t) {
		indicesForSorting[t].index = t;
		indicesForSorting[t].distanceAlongRay = projectPointOnLine(camPosWs, camLookWs, particles.allParticles[t].position);
	}

	std::sort(indicesForSorting.begin(), indicesForSorting.end(),
	          [&](const SortingData& a, const SortingData& b) { return a.distanceAlongRay > b.distanceAlongRay; });

	// Obtain the sprite texture and check if it is valid.
	Texture* const sprite = particles.spriteTexture->asTextureView()->tex.GetPtr();


	// Compute the sprite sub-images UV regions.
	const int numFrames = std::max(particles.spriteFramsCount.volume(), 0);
	if (numFrames != spriteFramesUVCache.size()) {
		float frmUWidth = 1.f / float(particles.spriteFramsCount.x);
		float frmVHeight = 1.f / float(particles.spriteFramsCount.y);

		spriteFramesUVCache.resize(numFrames);
		for (int iFrame = 0; iFrame < numFrames; ++iFrame) {
			int gridPosX = iFrame % particles.spriteFramsCount.x;
			int gridPosY = iFrame / particles.spriteFramsCount.x;

			Pair<vec2f, vec2f> uvRegion;
			uvRegion.first = vec2f(gridPosX * frmUWidth, gridPosY * frmVHeight);
			uvRegion.second = vec2f((gridPosX + 1.f) * frmUWidth, (gridPosY + 1.f) * frmVHeight);

			spriteFramesUVCache[iFrame] = uvRegion;
		}
	}

	// Obtain the sprite size in world space.
	const float particleWidthWs = 1.f;  // sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;
	const float particleHeightWs = 1.f; // sprite->getDesc().texture2D.width / pdesc.m_spritePixelsPerUnit;

	const float hx = particleWidthWs * 0.5f;
	const float hy = particleHeightWs * 0.5f;

	std::vector<ParticleVertexData> vertices;
	vertices.reserve(int(particles.allParticles.size()) * 6);

	mat4f faceCameraMtx = camera.getView();
	faceCameraMtx.c3 = vec4f(0.f, 0.f, 0.f, 1.f); // kill the translation.
	faceCameraMtx = inverse(faceCameraMtx);       // TODO: Optimize.

	// Generate the vertices so they are oriented towards the camera.
	for (const SortingData& sortedData : indicesForSorting) {
		const TraitParticlesProgrammable::ParticleGroup::ParticleData& p = particles.allParticles[sortedData.index];
		// Compute the transformation that will make the particle face the camera.
		vec3f particlePosRaw = p.position;
		vec3f particlePosTransformed = (!particles.isInWorldSpace) ? n2w.transfPos(particlePosRaw) : particlePosRaw;

		mat4f orientationMtx = mat4f::getTranslation(particlePosTransformed) * faceCameraMtx;

		vec2f uv0 = vec2f(0.f);
		vec2f uv1 = vec2f(1.f);
		int frmIndex = int(p.frameIndex) % int(spriteFramesUVCache.size());
		if (frmIndex >= 0 && frmIndex < spriteFramesUVCache.size()) {
			uv0 = spriteFramesUVCache[frmIndex].first;
			uv1 = spriteFramesUVCache[frmIndex].second;
		}

		// A template vertices represeting a quad, centered at (0,0,0) with correct size,
		// later we are going to transform these vertices to get the final transform
		// for the vertex buffer.
		const ParticleVertexData templateVetex[6] = {
		    ParticleVertexData{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), p.tint, uv0},
		    ParticleVertexData{vec3f(+hx, -hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv1.x, uv0.y)},
		    ParticleVertexData{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv1.x, uv1.y)},

		    ParticleVertexData{vec3f(-hx, -hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv0.x, uv0.y)},
		    ParticleVertexData{vec3f(+hx, +hy, 0.f), vec3f::getAxis(2), p.tint, uv1},
		    ParticleVertexData{vec3f(-hx, +hy, 0.f), vec3f::getAxis(2), p.tint, vec2f(uv0.x, uv1.y)},
		};

		for (const ParticleVertexData& templateVtx : templateVetex) {
			ParticleVertexData vtx = templateVtx;
			vtx.a_position *= fabsf(p.scale) * n2w.extractUnsignedScalingVector();
			vtx.a_position = mat_mul_pos(orientationMtx, vtx.a_position);
			vtx.a_normal = mat_mul_dir(orientationMtx, vtx.a_normal); // TODO: inverse transpose.
			vertices.push_back(vtx);
		}
	}

	const int strideSizeBytes = sizeof(vertices[0]);
	const int neededVtxBufferByteSize = int(vertices.size()) * strideSizeBytes;

	const bool isVtxBufferCorrectSize = vertexBuffer.IsResourceValid() && vertexBuffer->getDesc().sizeBytes == neededVtxBufferByteSize;

	if (isVtxBufferCorrectSize == false) {
		BufferDesc const vbDesc = BufferDesc::GetDefaultVertexBuffer(neededVtxBufferByteSize, ResourceUsage::Dynamic);
		vertexBuffer = sgecon.getDevice()->requestResource<Buffer>();
		vertexBuffer->create(vbDesc, vertices.data());
	} else {
		// The vertex buffer is large enough, just update it's contents.
		void* mappedMem = sgecon.map(vertexBuffer, Map::WriteDiscard);
		if (mappedMem) {
			memcpy(mappedMem, vertices.data(), neededVtxBufferByteSize);
		}
		sgecon.unMap(vertexBuffer);
	}

	VertexDecl vertexDecl[4] = {
	    VertexDecl(0, "a_position", UniformType::Float3, 0),
	    VertexDecl(0, "a_normal", UniformType::Float3, -1),
	    VertexDecl(0, "a_color", UniformType::Float4, -1),
	    VertexDecl(0, "a_uv", UniformType::Float2, -1),
	};

	VertexDeclIndex vertexDeclIdx = sgecon.getDevice()->getVertexDeclIndex(vertexDecl, SGE_ARRSZ(vertexDecl));

	geometry = Geometry(vertexBuffer, nullptr, nullptr, -1, vertexDeclIdx, false, true, true, false, PrimitiveTopology::TriangleList, 0, 0,
	                    strideSizeBytes, UniformType::Unknown, int(vertices.size()));

	material.diffuseTexture = sprite;

	return true;
}

} // namespace sge
