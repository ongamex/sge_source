#pragma once

#include "ShadingProgramPermuator.h"
#include "sge_core/sgecore_api.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/utils/optional.h"

namespace sge {

struct ICamera;

struct SGE_CORE_API SkyShader {
	void draw(const RenderDestination& rdest, const vec3f& cameraPosition, const mat4f view, const mat4f proj, Texture* skyTexture);

  private:
	GpuHandle<Buffer> m_skySphereVB;
	int m_skySphereNumVerts = 0;
	VertexDeclIndex m_skySphereVBVertexDeclIdx = VertexDeclIndex_Null;
	Optional<ShadingProgramPermuator> shadingPermut;
	GpuHandle<Buffer> cbParms;
	StateGroup stateGroup;
};

} // namespace sge
