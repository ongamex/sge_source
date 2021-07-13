#pragma once

#include "sge_core/Geometry.h"
#include "sge_engine/Actor.h"
#include "sge_utils/math/mat4.h"

namespace sge {

DefineTypeIdExists(TraitRenderGeometry);
/// Represents a custom made geometry that is going to be rendered with
/// the default shaders. (TODO: custom shaders and materials).
struct SGE_ENGINE_API TraitRenderGeometry : public Trait {
	SGE_TraitDecl_Full(TraitRenderGeometry);

	/// @brief A Single geometry to be rendered.
	struct Element {
		const Geometry* pGeom = nullptr; ///< The geometry to be rendered. The pointer must be managed manually.
		const Material* pMtl = nullptr; ///< The material to be used. The pointer must be managed manually.
		mat4f tform = mat4f::getIdentity();
		/// If true @tform should be used as if it specified the world space
		/// Ignoring the owning actor transform.
		/// otherwise it is in object space of the owning actor.
		bool isTformInWorldSpace = false;
		bool isRenderable = true;
	};

	std::vector<Element> geoms;
};

} // namespace sge
