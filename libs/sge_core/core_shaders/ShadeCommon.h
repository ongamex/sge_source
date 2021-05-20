//
// A file written in C containing commonalities between C++ and HLSL
//
#ifndef SHADECOMMON_H
#define SHADECOMMON_H

// Settings for skinned meshes OPT_HasVertexSkinning
#define kHasVertexSkinning_No 0
#define kHasVertexSkinning_Yes 1

// Setting for OPT_HasVertexColor, vertex color can be used as diffuse source or for tinting.
#define kHasVertexColor_No 0
#define kHasVertexColor_YesFloat3 1
#define kHasVertexColor_YesFloat4 2

// Settings for OPT_DiffuseColorSrc
#define kDiffuseColorSrcConstant 0
#define kDiffuseColorSrcVertex 1
#define kDiffuseColorSrcTexture 2
#define kDiffuseColorSrcTriplanarTex 3
#define kDiffuseColorSrcFluid 4

// Settings for OPT_Lighting
#define kLightingShaded 0
#define kLightingForceNoLighting 1

// Lights flags encoded as float use up to 23
// These are going to be casted as float in the shader BTW.
#define kLightFlt_DontLight 1 // user for objects that have no light affecting them besides the amibient one.
#define kLightFlg_HasShadowMap 2

// Material flags.
#define kPBRMtl_Flags_HasNormalMap 1
#define kPBRMtl_Flags_HasMetalnessMap 2
#define kPBRMtl_Flags_HasRoughnessMap 4

#endif
