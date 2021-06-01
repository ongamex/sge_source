
#include "ShadeCommon.h"

//--------------------------------------------------------------------
// Uniforms.
//--------------------------------------------------------------------
uniform float4x4 uProjView;
uniform float4x4 uWorld;

uniform float4 uColor;

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes

uniform sampler2D uSkinningBones;

#endif

//--------------------------------------------------------------------
// Common functions.
//--------------------------------------------------------------------
float4x4 getBoneTransform(int iBone, sampler2D bonesTransfTex) {
	const int2 boneTexSize = tex2Dsize(bonesTransfTex);
		
	const float mtxColumn_0_U = ((0.5 + 0.0) / 4.f);
	const float mtxColumn_1_U = ((0.5 + 1.0) / 4.f);
	const float mtxColumn_2_U = ((0.5 + 2.0) / 4.f);
	const float mtxColumn_3_U = ((0.5 + 3.0) / 4.f);	

	const float v = ((float)iBone + 0.5f) / (float)boneTexSize.y;

	const float4 c0 = tex2Dlod(bonesTransfTex, float4(mtxColumn_0_U, v, 0.f, 0.f));
	const float4 c1 = tex2Dlod(bonesTransfTex, float4(mtxColumn_1_U, v, 0.f, 0.f));
	const float4 c2 = tex2Dlod(bonesTransfTex, float4(mtxColumn_2_U, v, 0.f, 0.f));
	const float4 c3 = tex2Dlod(bonesTransfTex, float4(mtxColumn_3_U, v, 0.f, 0.f));

	// The matrix on CPU is column major, HLSL should be compiled with column major, but this is the syntax.
	const float4x4 mtx = float4x4(
		c0.x, c1.x, c2.x, c3.x,
		c0.y, c1.y, c2.y, c3.y,
		c0.z, c1.z, c2.z, c3.z,
		c0.w, c1.w, c2.w, c3.w);
		
	return mtx;
}

float4x4 getSkinningTransform(int4 bonesIndices, float4 bonesWeights, sampler2D bonesTransfTex) {
	const float4x4 skinMtx = 
		  getBoneTransform(bonesIndices.x, bonesTransfTex) * bonesWeights.x
		+ getBoneTransform(bonesIndices.y, bonesTransfTex) * bonesWeights.y
		+ getBoneTransform(bonesIndices.z, bonesTransfTex) * bonesWeights.z
		+ getBoneTransform(bonesIndices.w, bonesTransfTex) * bonesWeights.w;
		
	return skinMtx;
}

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
	float4x4 skinMtx = getSkinningTransform(vsin.a_bonesIds, vsin.a_bonesWeights, uSkinningBones);
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
