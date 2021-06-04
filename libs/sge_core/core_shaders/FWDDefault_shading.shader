#include "ShadeCommon.h"

#define PI 3.14159265f

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	#include "lib_skinning.shader"
#endif

#if OPT_Lighting != kLightingForceNoLighting
	#include "lib_pbr.shader"
#endif

//--------------------------------------------------------------------
// Uniforms
//--------------------------------------------------------------------
uniform float4x4 projView;
uniform float4x4 world;
uniform float4x4 uvwTransform;
uniform float4 cameraPositionWs;
uniform float4 uCameraLookDirWs;

uniform float4 uiHighLightColor;

uniform sampler2D uTexNormalMap;

uniform float4 uDiffuseColorTint;

#if OPT_DiffuseColorSrc == kDiffuseColorSrcTexture
uniform sampler2D texDiffuse;
#elif OPT_DiffuseColorSrc == kDiffuseColorSrcConstant
// nothing, we use the color
#elif OPT_DiffuseColorSrc == kDiffuseColorSrcTriplanarTex
uniform sampler2D texDiffuseX;
uniform sampler2D texDiffuseY;
uniform sampler2D texDiffuseZ;
uniform float3 texDiffuseXYZScaling;
#endif

uniform sampler2D uTexMetalness;
uniform sampler2D uTexRoughness;

uniform float uMetalness;
uniform float uRoughness;
#ifdef SGE_PIXEL_SHADER
uniform int uPBRMtlFlags;
#endif

uniform float4 uRimLightColorWWidth;

uniform float3 ambientLightColor;

// Lights uniforms.
uniform float4 lightPosition;           // Position, w encodes the type of the light.
uniform float4 lightSpotDirAndCosAngle; // all Used in spot lights :( other lights do not use it
uniform float4 lightColorWFlag;         // w used for flags.
// uniform float4 lightExtra2;
uniform sampler2D lightShadowMap;
uniform float4x4 lightShadowMapProjView;
uniform float4 lightShadowRange;

uniform samplerCUBE uPointLightShadowMap;

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	uniform sampler2D uSkinningBones;
	uniform int uSkinningFirstBoneOffsetInTex; ///< The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.
#endif

//--------------------------------------------------------------------
//
//--------------------------------------------------------------------
float2 directionToUV_Spherical(float3 dir)
{
	const float2 invAtan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(dir.z, dir.x), asin(-dir.y));
    uv *= invAtan;
    uv += float2(0.5, 0.5);
    return uv;
}

//--------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------
struct VS_INPUT {
	float3 a_position : a_position;

#if (OPT_UseNormalMap == 1) || (OPT_DiffuseColorSrc == kDiffuseColorSrcTexture)
	float2 a_uv : a_uv;
#endif

	float3 a_normal : a_normal;
#if OPT_UseNormalMap == 1
	float3 a_tangent : a_tangent;
	float3 a_binormal : a_binormal;
#endif

#if OPT_DiffuseColorSrc == kDiffuseColorSrcVertex
	float4 a_color : a_color;
#endif

#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	int4 a_bonesIds : a_bonesIds;
	float4 a_bonesWeights : a_bonesWeights;
#endif
};

struct VS_OUTPUT {
	float4 SV_Position : SV_Position;
	float3 v_posWS : v_posWS;
	float3 v_normal : v_normal;
#if (OPT_UseNormalMap == 1) || (OPT_DiffuseColorSrc == kDiffuseColorSrcTexture)
	float2 v_uv : v_uv;
#endif

#if OPT_UseNormalMap == 1
	float3 v_tangent : v_tangent;
	float3 v_binormal : v_binormal;
#endif

#if OPT_DiffuseColorSrc == kDiffuseColorSrcVertex
	float4 v_vertexDiffuse : v_vertexDiffuse;
#endif
};

VS_OUTPUT vsMain(VS_INPUT vsin) {
	VS_OUTPUT res;
	
	float3 vertexPosOs = vsin.a_position;
	float3 normalOs = vsin.a_normal;

	// If there is a skinning avilable apply it to the vertex in object space.
#if OPT_HasVertexSkinning == kHasVertexSkinning_Yes
	float4x4 skinMtx = libSkining_getSkinningTransform(vsin.a_bonesIds, uSkinningFirstBoneOffsetInTex, vsin.a_bonesWeights, uSkinningBones);
	vertexPosOs = mul(skinMtx, float4(vertexPosOs, 1.0)).xyz;
	normalOs = mul(skinMtx, float4(normalOs, 0.0)).xyz; // TODO: Proper normal transfrom by inverse transpose.
#endif
	
	// Pass the varyings to the next shader.
	const float4 worldPos = mul(world, float4(vertexPosOs, 1.0));
	const float4 worldNormal = mul(world, float4(normalOs, 0.0)); // TODO: Proper normal transfrom by inverse transpose.

	res.v_normal = worldNormal.xyz;
	res.v_posWS = worldPos.xyz;
	res.SV_Position = mul(projView, worldPos);

#if OPT_UseNormalMap == 1
	res.v_tangent = mul(world, float4(vsin.a_tangent, 0.0)).xyz;
	res.v_binormal = mul(world, float4(vsin.a_binormal, 0.0)).xyz;
#endif

#if (OPT_UseNormalMap == 1) || (OPT_DiffuseColorSrc == kDiffuseColorSrcTexture)
	res.v_uv = mul(uvwTransform, float4(vsin.a_uv, 0.0, 1.0)).xy;
#endif

#if OPT_DiffuseColorSrc == kDiffuseColorSrcVertex
	res.v_vertexDiffuse = vsin.a_color;
#endif

	return res;
}

//--------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------
#ifdef SGE_PIXEL_SHADER
float4 psMain(VS_OUTPUT IN) : SV_Target0 {
	float4 diffuseColor = pow(uDiffuseColorTint, 2.2f);

#if OPT_DiffuseColorSrc == kDiffuseColorSrcVertex
	diffuseColor = IN.v_vertexDiffuse;
#elif OPT_DiffuseColorSrc == kDiffuseColorSrcTexture
	diffuseColor *= tex2D(texDiffuse, IN.v_uv);
	diffuseColor.xyz = pow(diffuseColor.xyz, 2.2f);
#elif OPT_DiffuseColorSrc == kDiffuseColorSrcConstant
	// Nothing
#elif OPT_DiffuseColorSrc == kDiffuseColorSrcTriplanarTex
	float3 blendWeights = abs(IN.v_normal).xyz;
	blendWeights = blendWeights - float3(0.2679f, 0.2679f, 0.2679f);
	blendWeights.x = max(blendWeights.x, 0.0f);
	blendWeights.y = max(blendWeights.y, 0.0f);
	blendWeights.z = max(blendWeights.z, 0.0f);

	// Force sum to 1.0
	const float blendWeightDenom = 1.0f / (blendWeights.x + blendWeights.y + blendWeights.z);
	blendWeights = blendWeights * blendWeightDenom;

	float4 colorTexX = tex2D(texDiffuseX, (IN.v_posWS.zy * texDiffuseXYZScaling.x));
	float4 colorTexY = tex2D(texDiffuseY, (IN.v_posWS.xz * texDiffuseXYZScaling.y));
	float4 colorTexZ = tex2D(texDiffuseZ, (IN.v_posWS.xy * texDiffuseXYZScaling.z));

	diffuseColor *= (colorTexX * blendWeights.xxxx + colorTexY * blendWeights.yyyy + colorTexZ * blendWeights.zzzz);
	diffuseColor.xyz = pow(diffuseColor.xyz, 2.2f);
#else
	#error Not implemented.
#endif

	if (diffuseColor.w <= 0.0f) {
		discard;
	}

	// DOING SHADING HERE

#if OPT_UseNormalMap == 1
	float3 normalMapNorm = 2.f * tex2D(uTexNormalMap, IN.v_uv).xyz - float3(1.f, 1.f, 1.f);
	const float3 normalFromNormalMap =
	    normalize(IN.v_tangent) * normalMapNorm.x + normalize(IN.v_binormal) * normalMapNorm.y + normalize(IN.v_normal) * normalMapNorm.z;
	const float3 normal = normalize(normalFromNormalMap);
#else
	const float3 normal = normalize(IN.v_normal);
#endif

	// GGX Crap:
	const float3 N = normalize(normal);
	const float3 V = normalize(cameraPositionWs.xyz - IN.v_posWS);

	float3 lighting = float3(0.f, 0.f, 0.f);
#if OPT_Lighting != kLightingForceNoLighting
	// Lighting.
	const int fLightFlags = (int)lightColorWFlag.w;
	const bool dontLight = (fLightFlags & kLightFlt_DontLight) != 0;
	if (dontLight == false) {
		// GGX Material crap:
		float metallic = uMetalness;
#if (OPT_UseNormalMap == 1) || (OPT_DiffuseColorSrc == kDiffuseColorSrcTexture)
		if ((uPBRMtlFlags & kPBRMtl_Flags_HasMetalnessMap) != 0) {
			metallic *= pow(tex2D(uTexMetalness, IN.v_uv).r, 2.2f);
		}
#endif

		float roughness = uRoughness;
#if (OPT_UseNormalMap == 1) || (OPT_DiffuseColorSrc == kDiffuseColorSrcTexture)
		if ((uPBRMtlFlags & kPBRMtl_Flags_HasRoughnessMap) != 0) {
			roughness *= pow(tex2D(uTexRoughness, IN.v_uv).r, 2.2f);
		}
#endif

		const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), diffuseColor.xyz, metallic);

		float3 lightRadiance = float3(0.f, 0.f, 0.f);
		float3 L = float3(0.f, 0.f, 0.f);

		const float4 lightPosF4 = lightPosition;
		const float3 lightColor = lightColorWFlag.xyz;
		const float range2 = lightShadowRange.x * lightShadowRange.x;


		if (lightPosF4.w == 0.f) {
			// Point Light.
			const float3 toLightWs = lightPosF4.xyz - IN.v_posWS;
			float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
			lightRadiance.xyz = lightColor * saturate(k * k);
			L = normalize(toLightWs);
		} else if (lightPosF4.w == 1.f) {
			// Direction Light.
			lightRadiance.xyz = lightColor;
			L = lightPosF4.xyz;
		} else if (lightPosF4.w == 2.f) {
			// Spot Light.
			const float3 toLightWs = lightPosF4.xyz - IN.v_posWS;
			const float3 revSpotLightDirWs = -lightSpotDirAndCosAngle.xyz;
			const float spotLightAngleCosine = lightSpotDirAndCosAngle.w;
			const float visibilityCosine = saturate(dot(normalize(toLightWs), revSpotLightDirWs));
			const float c = saturate(visibilityCosine - spotLightAngleCosine);
			const float range = 1.f - spotLightAngleCosine;
			const float scale = saturate(c / range);
			const float k = 1.f - lerp(0.f, 1.f, saturate(dot(toLightWs, toLightWs) / range2));
			lightRadiance.xyz = lightColor * (scale * saturate(k * k));
			L = normalize(toLightWs);
		}

		const float NdotL = max(dot(N, L), 0.0);

		const bool useShadowsMap = (fLightFlags & kLightFlg_HasShadowMap) != 0;

		float shadowScale = 1.f;
#if 1
		if (useShadowsMap != 0) {
			if (lightPosF4.w == 0.f) {
				// Point light.
				// [FWDDEF_POINTLIGHT_LINEAR_ZDEPTH]
				float3 lightToVertexWs = IN.v_posWS - lightPosF4.xyz;
				float lightToVertexWsLength = length(lightToVertexWs);
				const float shadowSampleDistanceToLight =
				    lightShadowRange.x * texCUBE(uPointLightShadowMap, lightToVertexWs / lightToVertexWsLength).x;
				const float currentFragmentDistanceToLight = lightToVertexWsLength;

				if (shadowSampleDistanceToLight < currentFragmentDistanceToLight || NdotL <= 0.0) {
					shadowScale = 0.f;
				}
			} else {
				// Direction and Spot lights.
				const float4 pixelShadowProj = mul(lightShadowMapProjView, float4(IN.v_posWS, 1.f));
				const float4 pixelShadowNDC = pixelShadowProj / pixelShadowProj.w;

				float2 shadowMapSampleLocation = pixelShadowNDC.xy;
#ifndef OpenGL
				shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, -0.5) + float2(0.5, 0.5);
#else
				shadowMapSampleLocation = shadowMapSampleLocation * float2(0.5, 0.5) + float2(0.5, 0.5);
#endif

				const float2 pixelSizeUVShadow = (1.0 / tex2Dsize(lightShadowMap));

				float samplesWeights = 0.f;
				const int pcfWidth = 1;
				const int pcfTotalSamples = (2 * pcfWidth + 1) * (2 * pcfWidth + 1);
				for (int ix = -pcfWidth; ix <= pcfWidth; ix += 1) {
					for (int iy = -pcfWidth; iy <= pcfWidth; iy += 1) {
						const float2 sampleUv =
						    shadowMapSampleLocation + float2(float(ix) * pixelSizeUVShadow.x, float(iy) * pixelSizeUVShadow.y);

#ifndef OpenGL
						const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
						const float currentZ = pixelShadowNDC.z;
#else
						const float shadowZ = tex2D(lightShadowMap, sampleUv).x;
						const float currentZ = (pixelShadowNDC.z + 1.0) * 0.5;
#endif

						float fadeOutMult = 1.f;

						// Make directional light fade as they capture some limited area, and when the shadow map ends in the distance
						// this will hide the rough edge.
						if (lightPosF4.w == 1.f) {
							const float distanceFromCamera = min(length(IN.v_posWS - cameraPositionWs), lightShadowRange.x);
							const float fadeStart = lightShadowRange.x * 0.90f;
							if (distanceFromCamera > fadeStart) {
								fadeOutMult = lerp(1.f, 0.f, (distanceFromCamera - fadeStart) / (lightShadowRange.x * 0.1f));
							}
						}

						if (shadowZ < currentZ || NdotL <= 0.0) {
							samplesWeights += fadeOutMult;
						}
					}
				}

				shadowScale = 1.f - (samplesWeights / (float)(pcfTotalSamples));
			}
		}
#endif // shadow map enabled

		if (NdotL > 1e-6f && shadowScale > 1e-6f) {
			const float3 H = normalize(V + L);

			// cook-torrance brdf
#if 1
			const float NDF = DistributionGGX(N, H, roughness);
			const float G = GeometrySmith(N, V, L, roughness);
			const float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			const float3 kS = F;
			const float3 kD = (float3(1.f, 1.f, 1.f) - kS) * (1.0 - metallic);

			const float3 numerator = NDF * G * F;
			const float denominator = 4.f * max(dot(N, V), 0.f) * max(dot(N, L), 0.f);
			const float3 specular = numerator / max(denominator, 0.001f);

			lighting += shadowScale * (kD * diffuseColor.xyz / PI + specular) * lightRadiance * NdotL;
#else
			const float3 lightLighting = shadowScale * diffuseColor.xyz * lightRadiance * NdotL;
			lighting += lightLighting;
#endif
		}

		if (NdotL < 0.f) {
			const float3 ambience = lightColor * 0.5f;
			lighting += ((normal.y * 0.5f + 0.5f) * ambience + ambience * 0.05f) * diffuseColor.xyz;
		}
	}
#endif

#if OPT_Lighting == kLightingShaded
	const float3 ambientLightColorLinear = ambientLightColor; // pow(ambientLightColor, 2.2f);
	const float3 fakeAmbientDetail =
	    ((normal.y * 0.5f + 0.5f) * ambientLightColorLinear + ambientLightColorLinear * 0.05f) * diffuseColor.xyz;
	const float3 rimLighting =
	    smoothstep(1.f - uRimLightColorWWidth.w, 1.f, (1.f - dot(uCameraLookDirWs, N))) * diffuseColor.xyz * uRimLightColorWWidth.xyz;

	float4 finalColor = float4(lighting, diffuseColor.w) + float4(fakeAmbientDetail + rimLighting, 0.f);
#elif OPT_Lighting == kLightingForceNoLighting
	float4 finalColor = diffuseColor;
#else
#error OPT_Lighting unexpected
#endif

	//// Saturate the color, to make the selection highlight visiable for really brightly lit objects.
	//finalColor.x = saturate(finalColor.x);
	//finalColor.y = saturate(finalColor.y);
	//finalColor.z = saturate(finalColor.z);

	// Apply tone mapping.
	//finalColor.xyz = finalColor / (finalColor + float3(1.f, 1.f, 1.f));

	// Back to linear color space.
	finalColor.xyz = pow(finalColor, 1.0f / 2.2f);

	return finalColor;
}
#endif