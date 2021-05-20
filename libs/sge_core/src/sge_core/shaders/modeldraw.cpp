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
	};

	enum : int { uWorld, uProjView, uPointLightPositionWs, uPointLightFarPlaneDistance };

	if (shadingPermutFWDBuildShadowMaps.isValid() == false) {
		shadingPermutFWDBuildShadowMaps = ShadingProgramPermuator();

		static const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_LightType,
		     "OPT_LightType",
		     {SGE_MACRO_STR(FWDDBSM_OPT_LightType_SpotOrDirectional), SGE_MACRO_STR(FWDDBSM_OPT_LightType_Point)}},
		};

		static const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uWorld, "world"},
		    {uProjView, "projView"},
		    {uPointLightPositionWs, "uPointLightPositionWs"},
		    {uPointLightFarPlaneDistance, "uPointLightFarPlaneDistance"},
		};

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermutFWDBuildShadowMaps->createFromFile(sgedev, "core_shaders/FWDDefault_buildShadowMaps.shader", compileTimeOptions,
		                                                uniformsToCache);
	}

	const OptionPermuataor::OptionChoice optionChoice[] = {
	    {OPT_LightType, generalMods.isShadowMapForPointLight ? FWDDBSM_OPT_LightType_Point : FWDDBSM_OPT_LightType_SpotOrDirectional},
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
	enum : int {
		OPT_UseNormalMap,
		OPT_DiffuseColorSrc,
		OPT_Lighting,
		OPT_HasVertexSkinning,
		kNumOptions,
	};

	enum : int {
		uiHighLightColor,
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
		uColor,
		uCameraPositionWs,
		uCameraLookDirWs,
		uWorld,
		uProjView,
		uUVWTransform,
		uGameTime,
		uAmbientLightColor,
		uRimLightColorWWidth,
		uLightPosition,
		uLightSpotDirAndCosAngle,
		uLightColorWFlag,
		uLightShadowMap,
		uLightShadowMapProjView,
		uLightShadowRange,
		uPointLightShadowMap,
		uFluidColor0,
		uFluidColor1,
		uTexMetalness,
		uTexMetalnessSampler,
		uTexRoughness,
		uTexRoughnessSampler,
		uMetalness,
		uRoughness,
		uPBRMtlFlags,
		uSkinningBones,
	};

	if (shadingPermutFWDShading.isValid() == false) {
		shadingPermutFWDShading = ShadingProgramPermuator();

		static const std::vector<OptionPermuataor::OptionDesc> compileTimeOptions = {
		    {OPT_UseNormalMap, "OPT_UseNormalMap", {"0", "1"}},
		    {OPT_DiffuseColorSrc, "OPT_DiffuseColorSrc", {"0", "1", "2", "3", "4"}},
		    {OPT_Lighting, "OPT_Lighting", {SGE_MACRO_STR(kLightingShaded), SGE_MACRO_STR(kLightingForceNoLighting)}},
		    {OPT_HasVertexSkinning, "OPT_HasVertexSkinning", {SGE_MACRO_STR(kHasVertexSkinning_No), SGE_MACRO_STR(kHasVertexSkinning_Yes)}},
		};

		// clang-format off

		// Caution: It is important that the order of the elements here MATCHES the order in the enum above.
		static const std::vector<ShadingProgramPermuator::Unform> uniformsToCache = {
		    {uiHighLightColor, "uiHighLightColor"},
		    {uTexDiffuse, "texDiffuse"},
		    {uTexDiffuseSampler, "texDiffuse_sampler"},
		    {uTexNormalMap, "uTexNormalMap"},
		    {uTexNormalMapSampler, "uTexNormalMap_sampler"},
		    {uTexDiffuseX, "texDiffuseX"},
		    {uTexDiffuseXSampler, "texDiffuseX_sampler"},
		    {uTexDiffuseY, "texDiffuseY"},
		    {uTexDiffuseYSampler, "texDiffuseY_sampler"},
		    {uTexDiffuseZ, "texDiffuseZ"},
		    {uTexDiffuseZSampler, "texDiffuseZ_sampler"},
		    {uTexDiffuseXYZScaling, "texDiffuseXYZScaling"},
		    {uColor, "uDiffuseColorTint"},
		    {uCameraPositionWs, "cameraPositionWs"},
		    {uCameraLookDirWs, "uCameraLookDirWs"},
		    {uWorld, "world"},
		    {uProjView, "projView"},
		    {uUVWTransform, "uvwTransform"},
		    {uGameTime, "gameTime"},
		    {uAmbientLightColor, "ambientLightColor"},
		    {uRimLightColorWWidth, "uRimLightColorWWidth"},
		    {uLightPosition, "lightPosition"},
		    {uLightSpotDirAndCosAngle, "lightSpotDirAndCosAngle"},
		    {uLightColorWFlag, "lightColorWFlag"},
		    {uLightShadowMap, "lightShadowMap"},
		    {uLightShadowMapProjView, "lightShadowMapProjView"},
		    {uLightShadowRange, "lightShadowRange"},
		    {uPointLightShadowMap, "uPointLightShadowMap"},
		    {uFluidColor0, "uFluidColor0"},
		    {uFluidColor1, "uFluidColor1"},
		    {uTexMetalness, "uTexMetalness"},
		    {uTexMetalnessSampler, "uTexMetalness_sampler"},
		    {uTexRoughness, "uTexRoughness"},
		    {uTexRoughnessSampler, "uTexRoughness_sampler"},
		    {uMetalness, "uMetalness"},
		    {uRoughness, "uRoughness"},
		    {uPBRMtlFlags, "uPBRMtlFlags"},
			{uSkinningBones, "uSkinningBones"},
		};
		// clang-format on

		SGEDevice* const sgedev = rdest.getDevice();
		shadingPermutFWDShading->createFromFile(sgedev, "core_shaders/FWDDefault_shading.shader", compileTimeOptions, uniformsToCache);
	}

	SGEDevice* const sgedev = rdest.getDevice();

	const std::vector<VertexDecl>& vertexDecl = sgedev->getVertexDeclFromIndex(geometry->vertexDeclIndex);

	int optDiffuseColorSrc = kDiffuseColorSrcConstant;
	if (material.special == Material::special_none) {
		if (!material.diffuseTexture) {
			bool hasVertexColor = false;
			for (const VertexDecl& decl : vertexDecl) {
				if (decl.semantic == "a_color") {
					hasVertexColor = true;
					break;
				}
			}
			if (hasVertexColor) {
				optDiffuseColorSrc = kDiffuseColorSrcVertex;
			}
		}
		if (material.diffuseTexture)
			optDiffuseColorSrc = kDiffuseColorSrcTexture;
		if (material.diffuseTextureX && material.diffuseTextureY && material.diffuseTextureZ) {
			optDiffuseColorSrc = kDiffuseColorSrcTriplanarTex;
		}
	} else if (material.special == Material::special_fluid) {
		optDiffuseColorSrc = kDiffuseColorSrcFluid;
	}

	const int optLighting = (mods.forceNoLighting ? kLightingForceNoLighting : kLightingShaded);

	const int optUseNormalMap = !!(geometry->vertexDeclHasTangentSpace && material.texNormalMap);
	const int optHasVertexSkinning = (geometry->skinningBoneTransforms != nullptr) ? kHasVertexSkinning_Yes : kHasVertexSkinning_No;

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

	shaderPerm.bind<64>(uniforms, uWorld, (void*)&world);
	shaderPerm.bind<64>(uniforms, uCameraPositionWs, (void*)camPos.data);
	shaderPerm.bind<64>(uniforms, uCameraLookDirWs, (void*)camLookDir.data);
	shaderPerm.bind<64>(uniforms, uProjView, (void*)&projView);

	shaderPerm.bind<64>(uniforms, uiHighLightColor, (void*)&generalMods.selectionTint);

	shaderPerm.bind<64>(uniforms, uColor, (void*)&material.diffuseColor);

	mat4f combinedUVWTransform = mods.uvwTransform * material.uvwTransform;
	shaderPerm.bind<64>(uniforms, uUVWTransform, (void*)&combinedUVWTransform);

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
	} else if (optDiffuseColorSrc == kDiffuseColorSrcFluid) {
		shaderPerm.bind<64>(uniforms, uGameTime, (void*)&mods.gameTime);
		shaderPerm.bind<64>(uniforms, uFluidColor0, (void*)&material.fluidColor0.data);
		shaderPerm.bind<64>(uniforms, uFluidColor1, (void*)&material.fluidColor1.data);
	} else {
		sgeAssert(false);
	}

	int pbrFlags = 0;

	shaderPerm.bind<64>(uniforms, uMetalness, (void*)&material.metalness);
	if (material.texMetalness != nullptr) {
		pbrFlags |= kPBRMtl_Flags_HasMetalnessMap;
		shaderPerm.bind<64>(uniforms, uTexMetalness, (void*)material.texMetalness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexMetalnessSampler, (void*)material.texMetalness->getSamplerState());
#endif
	}

	shaderPerm.bind<64>(uniforms, uRoughness, (void*)&material.roughness);
	if (material.texRoughness != nullptr) {
		pbrFlags |= kPBRMtl_Flags_HasRoughnessMap;
		shaderPerm.bind<64>(uniforms, uTexRoughness, (void*)material.texRoughness);
#ifdef SGE_RENDERER_D3D11
		shaderPerm.bind<64>(uniforms, uTexRoughnessSampler, (void*)material.texRoughness->getSamplerState());
#endif
	}

	shaderPerm.bind<64>(uniforms, uPBRMtlFlags, (void*)&pbrFlags);

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

	if (shaderPerm.uniformLUT[uSkinningBones].isNull() == false) {
		uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uSkinningBones], (geometry->skinningBoneTransforms)));
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
				shaderPerm.bind<64>(uniforms, uAmbientLightColor, (void*)&generalMods.ambientLightColor);
				shaderPerm.bind<64>(uniforms, uRimLightColorWWidth, (void*)&generalMods.uRimLightColorWWidth);
			} else {
				vec4f zeroColor(0.f);
				shaderPerm.bind<64>(uniforms, uAmbientLightColor, (void*)&zeroColor);
				shaderPerm.bind<64>(uniforms, uRimLightColorWWidth, (void*)&zeroColor);
			}
		}

		if (mods.forceNoLighting == false) {
			shaderPerm.bind<64>(uniforms, (int)uLightPosition, (void*)&shadingLight.lightPositionAndType);
			shaderPerm.bind<64>(uniforms, (int)uLightSpotDirAndCosAngle, (void*)&shadingLight.lightSpotDirAndCosAngle);
			shaderPerm.bind<64>(uniforms, (int)uLightColorWFlag, (void*)&shadingLight.lightColorWFlags);

			if (shadingLight.shadowMap != nullptr && shaderPerm.uniformLUT[uLightShadowMap].isNull() == false) {
				if (shadingLight.lightPositionAndType.w == 0.f) {
					uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uPointLightShadowMap], (shadingLight.shadowMap)));
					sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
				} else {
					uniforms.push_back(BoundUniform(shaderPerm.uniformLUT[uLightShadowMap], (shadingLight.shadowMap)));
					sgeAssert(uniforms.back().bindLocation.isNull() == false && uniforms.back().bindLocation.uniformType != 0);
				}
			}

			shaderPerm.bind<64>(uniforms, uLightShadowMapProjView, (void*)&shadingLight.shadowMapProjView);
			shaderPerm.bind<64>(uniforms, uLightShadowRange, (void*)&shadingLight.lightXShadowRange);
		}

		if (mods.forceAdditiveBlending) {
			stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
			                          getCore()->getGraphicsResources().BS_backToFrontAlpha);
		} else {
			stateGroup.setRenderState(rasterState, getCore()->getGraphicsResources().DSS_default_lessEqual,
			                          (iLight == 0) ? getCore()->getGraphicsResources().BS_backToFrontAlpha
			                                        : getCore()->getGraphicsResources().BS_addativeColor);
		}



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
		shaderPerm.bind<64>(uniforms, uAmbientLightColor, (void*)&generalMods.ambientLightColor);
		shaderPerm.bind<64>(uniforms, uRimLightColorWWidth, (void*)&generalMods.uRimLightColorWWidth);

		vec4f colorWFlags(0.f);
		colorWFlags.w = float(kLightFlt_DontLight);
		shaderPerm.bind<64>(uniforms, uLightColorWFlag, (void*)&colorWFlags);

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
                          const EvaluatedModel& model,
                          const InstanceDrawMods& mods,
                          const std::vector<MaterialOverride>* mtlOverrides) {
	for (int iNode = 0; iNode < model.m_nodes.size(); ++iNode) {
		const EvaluatedNode& evalNode = model.m_nodes.valueAtIdx(iNode);

		for (int iMesh = 0; iMesh < evalNode.attachedMeshes.size(); ++iMesh) {
			const EvaluatedMeshAttachment& meshAttachment = evalNode.attachedMeshes[iMesh];
			Model::Mesh* const mesh = evalNode.attachedMeshes[iMesh].pMesh->pReferenceMesh;
			mat4f const finalTrasform = (mesh->bones.size() == 0) ? preRoot * evalNode.evalGlobalTransform : preRoot;

			Material material;

			if (meshAttachment.pMaterial) {
				auto itr = mtlOverrides ? std::find_if(mtlOverrides->begin(), mtlOverrides->end(),
				                                       [&meshAttachment](const MaterialOverride& v) -> bool {
					                                       return v.name == meshAttachment.pMaterial->name;
				                                       })
				                        : std::vector<MaterialOverride>::iterator();

				if (!mtlOverrides || itr == mtlOverrides->end()) {
					material.diffuseColor = meshAttachment.pMaterial->diffuseColor;
					material.metalness = meshAttachment.pMaterial->metallic;
					material.roughness = meshAttachment.pMaterial->roughness;

					material.diffuseTexture =
					    isAssetLoaded(meshAttachment.pMaterial->diffuseTexture) && meshAttachment.pMaterial->diffuseTexture->asTextureView()
					        ? meshAttachment.pMaterial->diffuseTexture->asTextureView()->tex.GetPtr()
					        : nullptr;

					material.texNormalMap =
					    isAssetLoaded(meshAttachment.pMaterial->texNormalMap) && meshAttachment.pMaterial->texNormalMap->asTextureView()
					        ? meshAttachment.pMaterial->texNormalMap->asTextureView()->tex.GetPtr()
					        : nullptr;

					material.texMetalness =
					    isAssetLoaded(meshAttachment.pMaterial->texMetallic) && meshAttachment.pMaterial->texMetallic->asTextureView()
					        ? meshAttachment.pMaterial->texMetallic->asTextureView()->tex.GetPtr()
					        : nullptr;

					material.texRoughness =
					    isAssetLoaded(meshAttachment.pMaterial->texRoughness) && meshAttachment.pMaterial->texRoughness->asTextureView()
					        ? meshAttachment.pMaterial->texRoughness->asTextureView()->tex.GetPtr()
					        : nullptr;
				} else {
					material = itr->mtl;
				}
			}

			drawGeometry(rdest, camPos, camLookDir, projView, finalTrasform, generalMods, &meshAttachment.pMesh->geom, material, mods);
		}
	}
}
