#include "modeldraw.h"
#include "sge_core/AssetLibrary.h"
#include "sge_core/ICore.h"
#include "sge_core/model/EvaluatedModel.h"
#include "sge_core/model/Model.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/FileStream.h"
#include <sge_utils/math/mat4.h>

// Caution:
// this include is an exception do not include anything else like it.
#include "../core_shaders/FWDDefault_buildShadowMaps.h"
#include "../core_shaders/ShadeCommon.h"

using namespace sge;

// https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules?redirectedfrom=MSDN
struct ParamsCbFWDDefaultShading {
	mat4f projView;
	mat4f world;
	mat4f uvwTransform;
	vec4f cameraPositionWs;
	vec4f uCameraLookDirWs;

	vec4f uiHighLightColor;
	vec4f uDiffuseColorTint;
	vec3f texDiffuseXYZScaling;
	float uMetalness;
	float uRoughness;

	int uPBRMtlFlags;
	int uPBRMtlFlags_padding[2];


	vec4f uRimLightColorWWidth;
	vec3f ambientLightColor;
	float ambientLightColor_padding;

	// Lights uniforms.
	vec4f lightPosition;           // Position, w encodes the type of the light.
	vec4f lightSpotDirAndCosAngle; // all Used in spot lights :( other lights do not use it
	vec4f lightColorWFlag;         // w used for flags.

	mat4f lightShadowMapProjView;
	vec4f lightShadowRange;

	// Skinning.
	int uSkinningFirstBoneOffsetInTex; ///< The row (integer) in @uSkinningBones of the fist bone for the mesh that is being drawn.
	int uSkinningFirstBoneOffsetInTex_padding[3];
};

//-----------------------------------------------------------------------------
// BasicModelDraw
//-----------------------------------------------------------------------------
void BasicModelDraw::drawGeometry(const RenderDestination& rdest,
                                  const vec3f& camPos,
                                  const vec3f& camLookDir,
                                  const mat4f& projView,
                                  const mat4f& world,
                                  const GeneralDrawMod& generalMods,
                                  const Geometry* geometry,
                                  const Material& material,
                                  const InstanceDrawMods& mods) {
	if (generalMods.isRenderingShadowMap) {
		drawGeometry_FWDBuildShadowMap(rdest, camPos, camLookDir, projView, world, generalMods, geometry, material, mods);
	} else {
		drawGeometry_FWDShading(rdest, camPos, camLookDir, projView, world, generalMods, geometry, material, mods);
	}
}


void BasicModelDraw::drawGeometry_FWDBuildShadowMap(const RenderDestination& rdest,
                                                    const vec3f& camPos,
                                                    const vec3f& UNUSED(camLookDir),
                                                    const mat4f& projView,
                                                    const mat4f& world,
                                                    const GeneralDrawMod& generalMods,
                                                    const Geometry* geometry,
                                                    const Material& UNUSED(material),
                                                    const InstanceDrawMods& mods) {
	enum {
		OPT_LightType,
		OPT_HasVertexSkinning,
		kNumOptions,
	};

	enum : int { uWorld, uProjView, uPointLightPositionWs, uPointLightFarPlaneDistance, uSkinningBones, uSkinningFirstBoneOffsetInTex };

	if (shadingPermutFWDBuildShadowMaps.isValid() == false) {
		shadingPermutFWDBuildShadowMaps = ShadingProgramPermuator();

		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_LightType,
		     "OPT_LightType",
		     {SGE_MACRO_STR(FWDDBSM_OPT_LightType_SpotOrDirectional), SGE_MACRO_STR(FWDDBSM_OPT_LightType_Point)}},
		    {OPT_HasVertexSkinning, "OPT_HasVertexSkinning", {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		};

		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uWorld, "world"},
		    {uProjView, "projView"},
		    {uPointLightPositionWs, "uPointLightPositionWs"},
		    {uPointLightFarPlaneDistance, "uPointLightFarPlaneDistance"},
		    {uSkinningBones, "uSkinningBones"},
		    {uSkinningFirstBoneOffsetInTex, "uSkinningFirstBoneOffsetInTex"}};

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermutFWDBuildShadowMaps->createFromFile(sgedev, "core_shaders/FWDDefault_buildShadowMaps.shader", compileTimeOptions,
		                                                uniformsToCache);
	}

	const int optHasVertexSkinning = (geometry->hasVertexSkinning()) ? kHasVertexSkinning_Yes : kHasVertexSkinning_No;

	const OptionPermuataor::OptionChoice optionChoice[kNumOptions] = {
	    {OPT_LightType, generalMods.isShadowMapForPointLight ? FWDDBSM_OPT_LightType_Point : FWDDBSM_OPT_LightType_SpotOrDirectional},
	    {OPT_HasVertexSkinning, optHasVertexSkinning},
	};

	const int iShaderPerm =
	    shadingPermutFWDBuildShadowMaps->getCompileTimeOptionsPerm().computePermutationIndex(optionChoice, SGE_ARRSZ(optionChoice));
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermutFWDBuildShadowMaps->getShadersPerPerm()[iShaderPerm];

	StaticArray<BoundUniform, 8> uniforms;

	shaderPerm.bind<8>(uniforms, (int)uWorld, (void*)&world);
	shaderPerm.bind<8>(uniforms, (int)uProjView, (void*)&projView);

	if (generalMods.isShadowMapForPointLight) {
		vec3f pointLightPositionWs = camPos;
		shaderPerm.bind<8>(uniforms, uPointLightPositionWs, (void*)&pointLightPositionWs);
		shaderPerm.bind<8>(uniforms, uPointLightFarPlaneDistance, (void*)&generalMods.shadowMapPointLightDepthRange);
	}

	if (optHasVertexSkinning == kHasVertexSkinning_Yes) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningBones], (geometry->skinningBoneTransforms)));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningFirstBoneOffsetInTex], (void*)&geometry->firstBoneOffset));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	// Feed the draw call data to the state group.
	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
	stateGroup.setVBDeclIndex(geometry->vertexDeclIndex);
	stateGroup.setVB(0, geometry->vertexBuffer, uint32(geometry->vbByteOffset), geometry->stride);

	RasterizerState* rasterState = nullptr;
	if (mods.forceNoCulling) {
		rasterState = getCore()->getGraphicsResources().RS_noCulling;
	} else {
		// We are baking shadow maps and we want to render the backfaces
		// *opposing to the regular rendering which uses front faces... duh).
		// This is done to avoid the Shadow Acne artifacts caused by floating point
		// innacuraties introduced by the depth texture.
		bool flipCulling = determinant(world) > 0.f;

		// Caution: [POINT_LIGHT_SHADOWMAP_TRIANGLE_WINING_FLIP]
		// Triangle winding would need an aditional flip based on the rendering API.
		if (generalMods.isShadowMapForPointLight && kIsTexcoordStyleD3D) {
			flipCulling = !flipCulling;
		}

		rasterState = flipCulling ? getCore()->getGraphicsResources().RS_defaultBackfaceCCW : getCore()->getGraphicsResources().RS_default;
	}

	stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual);

	if (geometry->ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry->indexBuffer, geometry->ibFmt, geometry->ibByteOffset);
	} else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	DrawCall dc;
	dc.setUniforms(uniforms.data(), uniforms.size());
	dc.setStateGroup(&stateGroup);

	if (geometry->ibFmt != UniformType::Unknown) {
		dc.drawIndexed(geometry->numElements, 0, 0);
	} else {
		dc.draw(geometry->numElements, 0);
	}

	// Exexute the draw call.
	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void BasicModelDraw::drawGeometry_FWDShading(const RenderDestination& rdest,
                                             const vec3f& camPos,
                                             const vec3f& camLookDir,
                                             const mat4f& projView,
                                             const mat4f& world,
                                             const GeneralDrawMod& generalMods,
                                             const Geometry* geometry,
                                             const Material& material,
                                             const InstanceDrawMods& mods) {
	SGEDevice* const sgedev = rdest.getDevice();
	if (!paramsBuffer.IsResourceValid()) {
		BufferDesc bd = BufferDesc::GetDefaultConstantBuffer(1024, ResourceUsage::Dynamic);
		paramsBuffer = sgedev->requestResource<Buffer>();
		paramsBuffer->create(bd, nullptr);
	}

	enum : int {
		OPT_UseNormalMap,
		OPT_DiffuseColorSrc,
		OPT_Lighting,
		OPT_HasVertexSkinning,
		kNumOptions,
	};

	enum : int {
		uTexDiffuse,
		uTexDiffuseSampler,
		uTexNormalMap,
		uTexNormalMapSampler,
		uTexDiffuseX,
		uTexDiffuseXSampler,
		uTexDiffuseY,
		uTexDiffuseYSampler,
		uTexDiffuseZ,
		uTexDiffuseZSampler,
		uTexDiffuseXYZScaling,
		uLightShadowMap,
		uPointLightShadowMap,
		uTexMetalness,
		uTexMetalnessSampler,
		uTexRoughness,
		uTexRoughnessSampler,
		uTexSkinningBones,
		uParamsCbFWDDefaultShading_vertex,
		uParamsCbFWDDefaultShading_pixel,
	};

	ParamsCbFWDDefaultShading paramsCb;
	memset(&paramsCb, 0, sizeof(paramsCb));

	if (shadingPermutFWDShading.isValid() == false) {
		shadingPermutFWDShading = ShadingProgramPermuator();

		const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_UseNormalMap, "OPT_UseNormalMap", {"0", "1"}},
		    {OPT_DiffuseColorSrc, "OPT_DiffuseColorSrc", {"0", "1", "2", "3", "4"}},
		    {OPT_Lighting, "OPT_Lighting", {SGE_MACRO_STR(kLightingShaded), SGE_MACRO_STR(kLightingForceNoLighting)}},
		    {OPT_HasVertexSkinning, "OPT_HasVertexSkinning", {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		};

		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uTexDiffuse, "texDiffuse", ShaderType::PixelShader},
		    {uTexDiffuseSampler, "texDiffuse_sampler", ShaderType::PixelShader},
		    {uTexNormalMap, "uTexNormalMap", ShaderType::PixelShader},
		    {uTexNormalMapSampler, "uTexNormalMap_sampler", ShaderType::PixelShader},
		    {uTexDiffuseX, "texDiffuseX", ShaderType::PixelShader},
		    {uTexDiffuseXSampler, "texDiffuseX_sampler", ShaderType::PixelShader},
		    {uTexDiffuseY, "texDiffuseY", ShaderType::PixelShader},
		    {uTexDiffuseYSampler, "texDiffuseY_sampler", ShaderType::PixelShader},
		    {uTexDiffuseZ, "texDiffuseZ", ShaderType::PixelShader},
		    {uTexDiffuseZSampler, "texDiffuseZ_sampler", ShaderType::PixelShader},
		    {uTexDiffuseXYZScaling, "texDiffuseXYZScaling", ShaderType::PixelShader},
		    {uLightShadowMap, "lightShadowMap", ShaderType::PixelShader},
		    {uPointLightShadowMap, "uPointLightShadowMap", ShaderType::PixelShader},
		    {uTexMetalness, "uTexMetalness", ShaderType::PixelShader},
		    {uTexMetalnessSampler, "uTexMetalness_sampler", ShaderType::PixelShader},
		    {uTexRoughness, "uTexRoughness", ShaderType::PixelShader},
		    {uTexRoughnessSampler, "uTexRoughness_sampler", ShaderType::PixelShader},
		    {uTexSkinningBones, "uSkinningBones", ShaderType::VertexShader},
		    {uParamsCbFWDDefaultShading_vertex, "ParamsCbFWDDefaultShading", ShaderType::VertexShader},
		    {uParamsCbFWDDefaultShading_pixel, "ParamsCbFWDDefaultShading", ShaderType::PixelShader},
		};


		shadingPermutFWDShading->createFromFile(sgedev, "core_shaders/FWDDefault_shading.shader", compileTimeOptions, uniformsToCache);
	}

	int optDiffuseColorSrc = kDiffuseColorSrcConstant;
	{
		if (!material.diffuseTexture) {
			if (geometry->vertexDeclHasVertexColor) {
				optDiffuseColorSrc = kDiffuseColorSrcVertex;
			}
		}
		if (material.diffuseTexture)
			optDiffuseColorSrc = kDiffuseColorSrcTexture;
		if (material.diffuseTextureX && material.diffuseTextureY && material.diffuseTextureZ) {
			optDiffuseColorSrc = kDiffuseColorSrcTriplanarTex;
		}
	}

	const int optLighting = (mods.forceNoLighting ? kLightingForceNoLighting : kLightingShaded);

	const int optUseNormalMap = !!(geometry->vertexDeclHasTangentSpace && material.texNormalMap);
	const int optHasVertexSkinning = (geometry->hasVertexSkinning()) ? kHasVertexSkinning_Yes : kHasVertexSkinning_No;

	const OptionPermuataor::OptionChoice optionChoice[kNumOptions] = {{OPT_UseNormalMap, optUseNormalMap},
	                                                                  {OPT_DiffuseColorSrc, optDiffuseColorSrc},
	                                                                  {OPT_Lighting, optLighting},
	                                                                  {OPT_HasVertexSkinning, optHasVertexSkinning}};

	const int iShaderPerm =
	    shadingPermutFWDShading->getCompileTimeOptionsPerm().computePermutationIndex(optionChoice, SGE_ARRSZ(optionChoice));
	const ShadingProgramPermuator::Permutation& shaderPerm = shadingPermutFWDShading->getShadersPerPerm()[iShaderPerm];

	DrawCall dc;

	stateGroup.setProgram(shaderPerm.shadingProgram.GetPtr());
	stateGroup.setVBDeclIndex(geometry->vertexDeclIndex);
	stateGroup.setVB(0, geometry->vertexBuffer, uint32(geometry->vbByteOffset), geometry->stride);
	stateGroup.setPrimitiveTopology(geometry->topology);
	if (geometry->ibFmt != UniformType::Unknown) {
		stateGroup.setIB(geometry->indexBuffer, geometry->ibFmt, geometry->ibByteOffset);
	} else {
		stateGroup.setIB(nullptr, UniformType::Unknown, 0);
	}

	RasterizerState* rasterState = nullptr;
	if (mods.forceNoCulling) {
		rasterState = getCore()->getGraphicsResources().RS_noCulling;
	} else {
		// We are baking shadow maps and we want to render the backfaces
		// *opposing to the regular rendering which uses front faces... duh).
		// This is done to avoid the Shadow Acne artifacts caused by floating point
		// innacuraties introduced by the depth texture.
		bool flipCulling = determinant(world) > 0.f;

		// Caution: [POINT_LIGHT_SHADOWMAP_TRIANGLE_WINING_FLIP]
		// Triangle winding would need an aditional flip based on the rendering API.
		if (generalMods.isShadowMapForPointLight && kIsTexcoordStyleD3D) {
			flipCulling = !flipCulling;
		}

		rasterState = flipCulling ? getCore()->getGraphicsResources().RS_default : getCore()->getGraphicsResources().RS_defaultBackfaceCCW;
	}

	StaticArray<BoundUniform, 64> uniforms;

	mat4f combinedUVWTransform = mods.uvwTransform * material.uvwTransform;

	paramsCb.world = world;
	paramsCb.cameraPositionWs = vec4f(camPos, 1.f);
	paramsCb.uCameraLookDirWs = vec4f(camLookDir, 0.f);
	paramsCb.projView = projView;
	paramsCb.uiHighLightColor = generalMods.selectionTint;
	paramsCb.uDiffuseColorTint = material.diffuseColor;
	paramsCb.uvwTransform = combinedUVWTransform;
	paramsCb.texDiffuseXYZScaling = material.diffuseTexXYZScaling;
	paramsCb.uMetalness = material.metalness;
	paramsCb.uRoughness = material.roughness;
	paramsCb.uSkinningFirstBoneOffsetInTex = geometry->firstBoneOffset;

	if (optDiffuseColorSrc == kDiffuseColorSrcConstant) {
		// Nothing, uColor is used here.
	} else if (optDiffuseColorSrc == kDiffuseColorSrcVertex) {
		// Nothing.
	} else if (optDiffuseColorSrc == kDiffuseColorSrcTexture) {
		shaderPerm.bind<64>(uniforms, uTexDiffuse, (void*)material.diffuseTexture);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseSampler, (void*)material.diffuseTexture->getSamplerState());
#endif
	} else if (optDiffuseColorSrc == kDiffuseColorSrcTriplanarTex) {
		shaderPerm.bind<64>(uniforms, uTexDiffuseX, (void*)material.diffuseTextureX);
		shaderPerm.bind<64>(uniforms, uTexDiffuseY, (void*)material.diffuseTextureY);
		shaderPerm.bind<64>(uniforms, uTexDiffuseZ, (void*)material.diffuseTextureZ);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexDiffuseXSampler, (void*)material.diffuseTextureX->getSamplerState());
		shaderPerm.bind<64>(uniforms, uTexDiffuseYSampler, (void*)material.diffuseTextureY->getSamplerState());
		shaderPerm.bind<64>(uniforms, uTexDiffuseZSampler, (void*)material.diffuseTextureZ->getSamplerState());
#endif
		shaderPerm.bind<64>(uniforms, uTexDiffuseXYZScaling, (void*)&material.diffuseTexXYZScaling);
	} else {
		sgeAssert(false);
	}

	int pbrFlags = 0;

	if (material.texMetalness != nullptr) {
		pbrFlags |= kPBRMtl_Flags_HasMetalnessMap;
		shaderPerm.bind<64>(uniforms, uTexMetalness, (void*)material.texMetalness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexMetalnessSampler, (void*)material.texMetalness->getSamplerState());
#endif
	}

	if (material.texRoughness != nullptr) {
		pbrFlags |= kPBRMtl_Flags_HasRoughnessMap;
		shaderPerm.bind<64>(uniforms, uTexRoughness, (void*)material.texRoughness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexRoughnessSampler, (void*)material.texRoughness->getSamplerState());
#endif
	}

	paramsCb.uPBRMtlFlags = pbrFlags;

	if (optUseNormalMap) {
		shaderPerm.bind<64>(uniforms, uTexNormalMap, (void*)material.texNormalMap);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexNormalMapSampler, (void*)material.texNormalMap->getSamplerState());
#endif
	}

	if (emptyCubeShadowMap.IsResourceValid() == false) {
		TextureDesc texDesc;
		texDesc.textureType = UniformType::TextureCube;
		texDesc.format = TextureFormat::D24_UNORM_S8_UINT;
		texDesc.usage = TextureUsage::DepthStencilResource;
		texDesc.textureCube.width = 16;
		texDesc.textureCube.height = 16;
		texDesc.textureCube.arraySize = 1;
		texDesc.textureCube.numMips = 1;
		texDesc.textureCube.sampleQuality = 0;
		texDesc.textureCube.numSamples = 1;

		emptyCubeShadowMap = getCore()->getDevice()->requestResource<Texture>();
		[[maybe_unused]] const bool succeeded = emptyCubeShadowMap->create(texDesc, nullptr);
	}

	if (shaderPerm.uniformLUT[uPointLightShadowMap].isNull() == false) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uPointLightShadowMap], (emptyCubeShadowMap.GetPtr())));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	if (optHasVertexSkinning == kHasVertexSkinning_Yes) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uTexSkinningBones], (geometry->skinningBoneTransforms)));
		sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
	}

	// Lights and draw call.
	const int preLightsNumUnuforms = uniforms.size();
	for (int iLight = 0; iLight < generalMods.lightsCount; ++iLight) {
		const ShadingLightData& shadingLight = *generalMods.ppLightData[iLight];
		// Delete the uniforms form the previous light.
		uniforms.resize(preLightsNumUnuforms);

		// Do the ambient lighting only with the 1st light.
		if (optLighting == kLightingShaded) {
			if (iLight == 0) {
				paramsCb.ambientLightColor = generalMods.ambientLightColor;
				paramsCb.uRimLightColorWWidth = generalMods.uRimLightColorWWidth;
			} else {
				vec4f zeroColor(0.f);
				paramsCb.ambientLightColor = vec3f(0.f);
				paramsCb.uRimLightColorWWidth = vec4f(0.f);
			}
		}

		if (mods.forceNoLighting == false) {
			paramsCb.lightPosition = shadingLight.lightPositionAndType;
			paramsCb.lightSpotDirAndCosAngle = shadingLight.lightSpotDirAndCosAngle;
			paramsCb.lightColorWFlag = shadingLight.lightColorWFlags;

			if (shadingLight.shadowMap != nullptr && shaderPerm.uniformLUT[uLightShadowMap].isNull() == false) {
				if (shadingLight.lightPositionAndType.w == 0.f) {
					uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uPointLightShadowMap], (shadingLight.shadowMap)));
					sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
				} else {
					uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uLightShadowMap], (shadingLight.shadowMap)));
					sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
				}
			}

			paramsCb.lightShadowMapProjView = shadingLight.shadowMapProjView;
			paramsCb.lightShadowRange = shadingLight.lightXShadowRange;
		}

		if (mods.forceAdditiveBlending) {
			stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
			                          getCore()->getGraphicsResources().BS_backToFrontAlpha);
		} else {
			stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
			                          (iLight == 0) ? getCore()->getGraphicsResources().BS_backToFrontAlpha
			                                        : getCore()->getGraphicsResources().BS_addativeColor);
		}

		void* paramsMappedData = sgedev->getContext()->map(paramsBuffer, Map::WriteDiscard);
		memcpy(paramsMappedData, &paramsCb, sizeof(paramsCb));
		sgedev->getContext()->unMap(paramsBuffer);

		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_vertex], paramsBuffer.GetPtr()));
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_pixel], paramsBuffer.GetPtr()));

		dc.setUniforms(uniforms.data(), uniforms.size());
		dc.setStateGroup(&stateGroup);

		if (geometry->ibFmt != UniformType::Unknown) {
			dc.drawIndexed(geometry->numElements, 0, 0);
		} else {
			dc.draw(geometry->numElements, 0);
		}

		rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
	}

	// if the number of lights affecting the object is zero,
	// then there were no draw call created. However we need to draw the object
	// in order for it to affect the z-depth or even get light by the ambient lighting.
	if (generalMods.lightsCount == 0) {
		paramsCb.ambientLightColor = generalMods.ambientLightColor;
		paramsCb.uRimLightColorWWidth = generalMods.uRimLightColorWWidth;

		vec4f colorWFlags(0.f);
		colorWFlags.w = float(kLightFlt_DontLight);
		paramsCb.lightColorWFlag = colorWFlags;

		void* paramsMappedData = sgedev->getContext()->map(paramsBuffer, Map::WriteDiscard);
		memcpy(paramsMappedData, &paramsCb, sizeof(paramsCb));
		sgedev->getContext()->unMap(paramsBuffer);

		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_vertex], paramsBuffer.GetPtr()));
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uParamsCbFWDDefaultShading_pixel], paramsBuffer.GetPtr()));

		stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);
		stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
		                          getCore()->getGraphicsResources().BS_backToFrontAlpha);

		dc.setUniforms(uniforms.data(), uniforms.size());
		dc.setStateGroup(&stateGroup);

		if (geometry->ibFmt != UniformType::Unknown) {
			dc.drawIndexed(geometry->numElements, 0, 0);
		} else {
			dc.draw(geometry->numElements, 0);
		}

		rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
	}
}

void BasicModelDraw::draw(const RenderDestination& rdest,
                          const vec3f& camPos,
                          const vec3f& camLookDir,
                          const mat4f& projView,
                          const mat4f& preRoot,
                          const GeneralDrawMod& generalMods,
                          const EvaluatedModel& evalModel,
                          const InstanceDrawMods& mods,
                          const std::vector<MaterialOverride>* mtlOverrides) {
	for (int iNode = 0; iNode < evalModel.getNumEvalNodes(); ++iNode) {
		const EvaluatedNode& evalNode = evalModel.getEvalNode(iNode);
		const ModelNode* rawNode = evalModel.m_model->nodeAt(iNode);

		for (int iMesh = 0; iMesh < rawNode->meshAttachments.size(); ++iMesh) {
			const MeshAttachment& meshAttachment = rawNode->meshAttachments[iMesh];
			const EvaluatedMesh& evalMesh = evalModel.getEvalMesh(meshAttachment.attachedMeshIndex);
			mat4f const finalTrasform = (evalMesh.geometry.hasVertexSkinning()) ? preRoot : preRoot * evalNode.evalGlobalTransform;

			Material material;

			if (meshAttachment.attachedMaterialIndex >= 0) {
				const EvaluatedMaterial& mtl = evalModel.getEvalMaterial(meshAttachment.attachedMaterialIndex);
				const std::string mtlName = evalModel.m_model->materialAt(meshAttachment.attachedMaterialIndex)->name;

				auto itr = mtlOverrides ? std::find_if(mtlOverrides->begin(), mtlOverrides->end(),
				                                       [&mtlName](const MaterialOverride& v) -> bool { return v.name == mtlName; })
				                        : std::vector<MaterialOverride>::iterator();

				if (!mtlOverrides || itr == mtlOverrides->end()) {
					material.diffuseColor = mtl.diffuseColor;
					material.metalness = mtl.metallic;
					material.roughness = mtl.roughness;

					material.diffuseTexture = isAssetLoaded(mtl.diffuseTexture) && mtl.diffuseTexture->asTextureView()
					                              ? mtl.diffuseTexture->asTextureView()->tex.GetPtr()
					                              : nullptr;

					material.texNormalMap = isAssetLoaded(mtl.texNormalMap) && mtl.texNormalMap->asTextureView()
					                            ? mtl.texNormalMap->asTextureView()->tex.GetPtr()
					                            : nullptr;

					material.texMetalness = isAssetLoaded(mtl.texMetallic) && mtl.texMetallic->asTextureView()
					                            ? mtl.texMetallic->asTextureView()->tex.GetPtr()
					                            : nullptr;

					material.texRoughness = isAssetLoaded(mtl.texRoughness) && mtl.texRoughness->asTextureView()
					                            ? mtl.texRoughness->asTextureView()->tex.GetPtr()
					                            : nullptr;
				} else {
					material = itr->mtl;
				}
			}

			drawGeometry(rdest, camPos, camLookDir, projView, finalTrasform, generalMods, &evalMesh.geometry, material, mods);
		}
	}
}
