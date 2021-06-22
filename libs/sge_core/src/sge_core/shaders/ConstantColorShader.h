#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct EvaluatedModel;
struct ModelMesh;

// ConstantColorWireShader draws the specified geometry in wireframe with a constant color.
struct SGE_CORE_API ConstantColorWireShader {
  public:
	ConstantColorWireShader() = default;

	void draw(const RenderDestination& rdest,
	          const mat4f& projView,
	          const mat4f& preRoot,
	          const EvaluatedModel& model,
	          const vec4f& shadingColor);

	void drawGeometry(
	    const RenderDestination& rdest, const mat4f& projView, const mat4f& world, const Geometry& geometry, const vec4f& shadingColor);

  private:
	Optional<ShadingProgramPermuator> shadingPermut;
	StateGroup stateGroup;
	GpuHandle<RasterizerState> m_rasterWireframeDepthBias;
	GpuHandle<RasterizerState> m_rasterWireframeDepthBiasCCW;
};

} // namespace sge
