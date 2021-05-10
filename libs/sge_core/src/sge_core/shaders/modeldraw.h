#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/OptionPermutator.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct EvaluatedModel;
namespace Model {
	struct Mesh;
}

struct MaterialOverride {
	std::string name;
	Material mtl;
};

//------------------------------------------------------------
// ShadingLightData
// Describes the light data in a converted form, suitable for
// passing it toe shader
//------------------------------------------------------------
struct ShadingLightData {
	// (x,y,z) - position. w holds type of the light as a float.
	vec4f lightPositionAndType = vec4f(0.f);
	vec4f lightSpotDirAndCosAngle = vec4f(1.f, 0.f, 0.f, 0.f);
	// (x,y,z) the color of the light multiplied by the intensity). w - flags as float.
	vec4f lightColorWFlags = vec4f(0.f);

	Texture* shadowMap = nullptr;
	mat4f shadowMapProjView = mat4f::getIdentity();
	// x is the light shadow range, (y,z,w) unused.
	vec4f lightXShadowRange = vec4f(0.f);

	AABox3f lightBoxWs;
};

//------------------------------------------------------------
// GeneralDrawMod
//------------------------------------------------------------
struct GeneralDrawMod {
	bool isRenderingShadowMap = false;
	bool isShadowMapForPointLight = false;
	float shadowMapPointLightDepthRange = 0.f;

	vec4f highlightColor = vec4f(0.f);
	vec3f ambientLightColor = vec3f(1.f);
	vec4f uRimLightColorWWidth = vec4f(vec3f(0.1f), 0.7f);

	int lightsCount = 0;
	const ShadingLightData** ppLightData = nullptr;
};

struct InstanceDrawMods {
	mat4f uvwTransform = mat4f::getIdentity();
	float gameTime = 0.f;
	bool forceNoLighting = false;
	bool forceAdditiveBlending = false;
	bool forceNoCulling = false;
};

//------------------------------------------------------------
// BasicModelDraw
//------------------------------------------------------------
struct SGE_CORE_API BasicModelDraw {
  public:
	BasicModelDraw() = default;

	void draw(const RenderDestination& rdest,
	          const vec3f& camPos,
	          const vec3f& camLookDir,
	          const mat4f& projView,
	          const mat4f& preRoot,
	          const GeneralDrawMod& generalMods,
	          const EvaluatedModel& model,
	          const InstanceDrawMods& mods,
	          const std::vector<MaterialOverride>* mtlOverrides = nullptr);

	void drawGeometry(const RenderDestination& rdest,
	                  const vec3f& camPos,
	                  const vec3f& camLookDir,
	                  const mat4f& projView,
	                  const mat4f& world,
	                  const GeneralDrawMod& generalMods,
	                  const Geometry* geometry,
	                  const Material& material,
	                  const InstanceDrawMods& mods);

  private:
	void drawGeometry_FWDShading(const RenderDestination& rdest,
	                             const vec3f& camPos,
	                             const vec3f& camLookDir,
	                             const mat4f& projView,
	                             const mat4f& world,
	                             const GeneralDrawMod& generalMods,
	                             const Geometry* geometry,
	                             const Material& material,
	                             const InstanceDrawMods& mods);

	void drawGeometry_FWDBuildShadowMap(const RenderDestination& rdest,
	                                    const vec3f& camPos,
	                                    const vec3f& camLookDir,
	                                    const mat4f& projView,
	                                    const mat4f& world,
	                                    const GeneralDrawMod& generalMods,
	                                    const Geometry* geometry,
	                                    const Material& material,
	                                    const InstanceDrawMods& mods);

  private:
	bool isInitialized = false;
	Optional<ShadingProgramPermuator> shadingPermutFWDShading;
	Optional<ShadingProgramPermuator> shadingPermutFWDBuildShadowMaps;
	GpuHandle<Texture> emptyCubeShadowMap;
	StateGroup stateGroup;
};

} // namespace sge
