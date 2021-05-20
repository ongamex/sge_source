#pragma once

#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct Geometry {
	Geometry() = default;
	Geometry(Buffer* vertexBuffer,
	         Buffer* indexBuffer,
	         Texture* skinningBoneTransforms,
	         VertexDeclIndex vertexDeclIndex,
	         bool vertexDeclHasVertexColor,
	         bool vertexDeclHasUv,
	         bool vertexDeclHasNormals,
	         bool vertexDeclHasTangentSpace,
	         PrimitiveTopology::Enum topology,
	         int vbByteOffset,
	         int ibByteOffset,
	         int stride,
	         UniformType::Enum ibFmt,
	         int numElements)
	    : vertexBuffer(vertexBuffer)
	    , indexBuffer(indexBuffer)
	    , skinningBoneTransforms(skinningBoneTransforms)
	    , vertexDeclIndex(vertexDeclIndex)
	    , vertexDeclHasVertexColor(vertexDeclHasVertexColor)
	    , vertexDeclHasUv(vertexDeclHasUv)
	    , vertexDeclHasNormals(vertexDeclHasNormals)
	    , vertexDeclHasTangentSpace(vertexDeclHasTangentSpace)
	    , topology(topology)
	    , vbByteOffset(vbByteOffset)
	    , ibByteOffset(ibByteOffset)
	    , stride(stride)
	    , ibFmt(ibFmt)
	    , numElements(numElements) {}

	bool hasData() const { return vertexBuffer != nullptr; }

	Buffer* vertexBuffer = nullptr;
	Buffer* indexBuffer = nullptr;
	Texture* skinningBoneTransforms = nullptr;
	VertexDeclIndex vertexDeclIndex = VertexDeclIndex_Null;
	bool vertexDeclHasVertexColor = false;
	bool vertexDeclHasUv = false;
	bool vertexDeclHasNormals = false;
	bool vertexDeclHasTangentSpace = false;
	PrimitiveTopology::Enum topology = PrimitiveTopology::Unknown;

	uint32 vbByteOffset = 0;                        // 1st vertex byte offset into the vertex buffer
	uint32 ibByteOffset = 0;                        // 1st index byte offse int the index buffer
	int stride = 0;                                 // The size of a whole vertex in bytes.
	UniformType::Enum ibFmt = UniformType::Unknown; // The format the index buffer, if unknown this mesh doesn't use index buffers.
	uint32 numElements = 0;                         // The number of vertices/indices used by this mesh.
};

struct Material {
	enum Special {
		special_none,
		special_fluid,
	};

	mat4f uvwTransform = mat4f::getIdentity();

	Special special = special_none;
	vec4f diffuseColor = vec4f(1.f, 1.f, 1.f, 1.f);
	Texture* texNormalMap = nullptr;
	Texture* diffuseTexture = nullptr;
	Texture* diffuseTextureX = nullptr; // Triplanar x-axis texture.
	Texture* diffuseTextureY = nullptr; // Triplanar y-axis texture.
	Texture* diffuseTextureZ = nullptr; // Triplanar z-axis texture.

	Texture* texMetalness = nullptr;
	Texture* texRoughness = nullptr;

	vec3f diffuseTexXYZScaling = vec3f(1.f); // Triplanar scaling for each axis.

	vec4f fluidColor0 = vec4f(1.f);
	vec4f fluidColor1 = vec4f(1.f);

	float metalness = 0.f;
	float roughness = 0.30f;
};

} // namespace sge
