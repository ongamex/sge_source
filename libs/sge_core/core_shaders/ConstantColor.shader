
#include "ShadeCommon.h"

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	#include "lib_skinning.shader"
#endif

//--------------------------------------------------------------------
// Uniforms.
//--------------------------------------------------------------------
uniform float4x4 uProjView;
uniform float4x4 uWorld;

uniform float4 uColor;

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	uniform sampler2D uSkinningBones;
	uniform int uSkinningFirstBoneOffsetInTex;
#endif

//--------------------------------------------------------------------
// Vertex Shader.
//--------------------------------------------------------------------
struct VS_INPUT
{
	float3 a_position : a_position;
	
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	int4 a_bonesIds : a_bonesIds;
	float4 a_bonesWeights : a_bonesWeights;	
#endif
};

struct VS_OUTPUT {
	float4 SV_Position : SV_Position;
};

VS_OUTPUT vsMain(VS_INPUT vsin)
{
	VS_OUTPUT res;

	float3 vertexPosOs = vsin.a_position;
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes	
	float4x4 skinMtx = libSkining_getSkinningTransform(vsin.a_bonesIds, uSkinningFirstBoneOffsetInTex, vsin.a_bonesWeights, uSkinningBones);
	vertexPosOs = mul(skinMtx, float4(vertexPosOs, 1.0)).xyz;
#endif

	const float4 worldPos = mul(uWorld, float4(vertexPosOs, 1.0));
	const float4 posProjSpace = mul(uProjView, worldPos);
	
	res.SV_Position = posProjSpace;
	
	return res;
}

//--------------------------------------------------------------------
// Pixel Shader.
//--------------------------------------------------------------------
float4 psMain(VS_OUTPUT IN) : SV_Target0
{
	return uColor;
}
