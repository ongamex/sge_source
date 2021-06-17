#include "QuickDraw.h"

#include "sge_core/ICore.h"
#include "sge_utils/math/color.h"
#include <sge_utils/math/Box.h>
#include <sge_utils/utils/FileStream.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace {

const char EFFECT_2D_UBERSHADER[] = R"(
//----------------------------------------
//
//----------------------------------------

uniform float4x4 projViewWorld;
uniform float alphaMult;

#ifdef COLOR_TEXTURE
	uniform float4 uvRegion; // xy - top-left, zw - bottom-right.
	uniform sampler2D colorTexture;
	#ifdef COLOR_TEXTURE_TEXT_MODE
		uniform float4 colorText;
	#endif
#else
		uniform float4 color;
#endif

struct VERTEX_IN {
	float2 a_position : a_position;
#ifdef COLOR_TEXTURE
	float2 a_uv : a_uv;
#endif
};

struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
#ifdef COLOR_TEXTURE
	float2 v_uv : v_uv;
#endif
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

#ifdef COLOR_TEXTURE
	OUT.v_uv = IN.a_uv;
#endif
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 0.0, 1.0));

	return OUT;
}

//----------------------------------------
//
//----------------------------------------
float4 psMain(VERTEX_OUT IN) : COLOR {
#ifdef COLOR_TEXTURE
	float2 sampleUV;
	sampleUV.x = uvRegion.x + (uvRegion.z - uvRegion.x) * IN.v_uv.x;
	sampleUV.y = uvRegion.y + (uvRegion.w - uvRegion.y) * IN.v_uv.y;
	float4 c0 = tex2D(colorTexture, sampleUV);

	#ifdef COLOR_TEXTURE_TEXT_MODE
		c0.xyzw = c0.xxxx;
		c0.w = c0.w * alphaMult;
		return colorText * c0;
	#else
		c0.w = c0.w * alphaMult;
		return c0;
	#endif
#else
	float4 c0 = color;
	c0.w = c0.w * alphaMult;
	return c0;
#endif
}

#endif
)";

const char EFFECT_3D_VERTEX_COLOR[] = R"(
//----------------------------------------
//
//----------------------------------------
uniform float4x4 projViewWorld;

struct VERTEX_IN {
	float3 a_position : a_position;
	float4 a_color : a_color;
};


struct VERTEX_OUT {
	float4 SV_Position : SV_Position;
	float4 v_color : v_color;
};

VERTEX_OUT vsMain(VERTEX_IN IN)
{
	VERTEX_OUT OUT;

	OUT.v_color = IN.a_color;
	OUT.SV_Position = mul(projViewWorld, float4(IN.a_position, 1.0));

	return OUT;
}

//----------------------------------------
//
//----------------------------------------
float4 psMain(VERTEX_OUT IN) : COLOR {
	return IN.v_color;
}
)";
} // namespace

namespace sge {

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
bool DebugFont::Create(SGEDevice* sgedev, const char* const ttfFilename, float heightPixels) {
	Destroy();

	// Read the ttf file data.
	FileReadStream frs;
	frs.open(ttfFilename);
	std::vector<unsigned char> ttfData(frs.remainingBytes());
	frs.read(ttfData.data(), ttfData.size());

	// Cache the font height.
	height = heightPixels;

	// Bake the font image.
	const int kTextureDim = 2048; // the size(in every axis) of the baked font texture.
	std::vector<unsigned char> textureData;
	textureData.resize(kTextureDim * kTextureDim);

	// [TODO] Find a way to pass a correct value for the "offset" argument
	// currently this is wrong and may crash!
	if (stbtt_BakeFontBitmap(ttfData.data(), 0, (float)heightPixels, textureData.data(), kTextureDim, kTextureDim, 0, 255, cdata) < 0) {
		Destroy();
		return false;
	}

	// Create a the texture of the font.
	TextureDesc td;
	td.textureType = UniformType::Texture2D;
	td.format = TextureFormat::R8_UNORM;
	td.usage = TextureUsage::ImmutableResource;
	td.texture2D = Texture2DDesc(kTextureDim, kTextureDim);

	TextureData texData;
	texData.data = textureData.data();
	texData.rowByteSize = kTextureDim * 1;

	texture = sgedev->requestResource<Texture>();
	if (!texture->create(td, &texData)) {
		Destroy();
		return false;
	}

	return true;
}

vec2f DebugFont::computeTextDimensions(const char* text, float textHeight) const {
	float kLineStartX = 0.f;

	float x = kLineStartX;
	float y = textHeight;
	const float sizeScaling = (textHeight >= 0.f) ? textHeight / (float)height : 1.f;

	AABox2f bbox;

	while (*text != '\0') {
		if (*text == '\n') {
			// Move the cursor to a new line and reset the x position.
			x = kLineStartX;
			y += height * sizeScaling;
			text += 1;
			continue;
		}

		const stbtt_bakedchar& ch = cdata[*text];
		const float halfWidth = (float)(ch.x1 - ch.x0) * 0.5f * sizeScaling;
		const float halfHeight = (float)(ch.y1 - ch.y0) * 0.5f * sizeScaling;

		mat4f world;
		if (*text != ' ') {
			world.data[0] = vec4f(halfWidth, 0.f, 0.f, 0.f);
			world.data[1] = vec4f(0.f, halfHeight, 0.f, 0.f);
			world.data[2] = vec4f(0.f, 0.f, 1.f, 0.f);
			world.data[3] = vec4f(x + halfWidth, y + halfHeight + ch.yoff * sizeScaling, 0.f, 1.f);
		}

		bbox.expand(mat_mul_pos(world, vec3f(0.f, 0.f, 0.f)).xy());
		bbox.expand(mat_mul_pos(world, vec3f(1.f, 1.f, 0.f)).xy());


		x += sizeScaling * ch.xadvance; // advance to the start point of the next character...

		text++;
	}

	return bbox.size();
}

void DebugFont::Destroy() {
	texture.Release();
}

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
bool QuickDraw::initialize(SGEContext* sgecon) {
	sgeAssert(sgecon);

	initalize2DDrawResources(sgecon);
	initalize3DDrawResources(sgecon);

	// Create the depth stencil states
	DepthStencilDesc dssDesc;
	dssDesc.comparisonFunc = DepthComparisonFunc::LessEqual;
	dssLessEqual = sgecon->getDevice()->requestResource<DepthStencilState>();
	dssLessEqual->create(dssDesc);

	return true;
}


void QuickDraw::initalize2DDrawResources(SGEContext* context) {
	SGEDevice* sgedev = context->getDevice();

	std::vector<Vertex2D> vbData;
	vbData.reserve(16);

	// Define the vertices for rectangle TriList CCW (bot-right, top-left)
	{
		m_rect2DShapeInfo.vertexOffset = (int)vbData.size();

		vbData.push_back(Vertex2D(vec2f(-1.f, 1.f), vec2f(0.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, 1.f), vec2f(1.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, -1.f), vec2f(1.f, 0.f)));

		vbData.push_back(Vertex2D(vec2f(1.f, -1.f), vec2f(1.f, 0.f)));
		vbData.push_back(Vertex2D(vec2f(-1.f, -1.f), vec2f(0.f, 0.f)));
		vbData.push_back(Vertex2D(vec2f(-1.f, 1.f), vec2f(0.f, 1.f)));

		m_rect2DShapeInfo.numPoints = (int)vbData.size() - m_rect2DShapeInfo.vertexOffset;
	}

	// Define the vertices for equilateral triangle pointing toward +X
	{
		m_triLeftShapeInfo.vertexOffset = (int)vbData.size();

		vbData.push_back(Vertex2D(vec2f(0.f, 1.f) - vec2f(0.5f), vec2f(0.f, 1.f)));
		vbData.push_back(Vertex2D(vec2f(1.f, 0.5f) - vec2f(0.5f), vec2f(1.f, 0.5f)));
		vbData.push_back(Vertex2D(vec2f(0.f, 0.f) - vec2f(0.5f), vec2f(0.f, 0.f)));

		m_triLeftShapeInfo.numPoints = (int)vbData.size() - m_triLeftShapeInfo.vertexOffset;
	}

	// Define the vertices for a circle. TriFan CCW
	{
		m_circle2DShapeInfo.vertexOffset = int(vbData.size());

		const int CIRCLE_SEGMENTS = 32;
		const float SEGMENT_THETHA = two_pi() / CIRCLE_SEGMENTS;

		// +1 for the last segments that is the same as the 1st one
		for (int t = 0; t < CIRCLE_SEGMENTS + 1; ++t) {
			float s0, c0;
			SinCos(SEGMENT_THETHA * (float)t, s0, c0);
			float s1, c1;
			SinCos(SEGMENT_THETHA * (float)(t + 1), s1, c1);

			vbData.push_back(Vertex2D(vec2f(0.f, 0.f), vec2f(0.f, 0.f)));
			vbData.push_back(Vertex2D(vec2f(c1, s1), vec2f((c1 + 1.f) * .5f, (s1 - 1.f) * .5f)));
			vbData.push_back(Vertex2D(vec2f(c0, s0), vec2f((c0 + 1.f) * .5f, (s0 - 1.f) * .5f)));
		}

		m_circle2DShapeInfo.numPoints = int(vbData.size()) - m_circle2DShapeInfo.vertexOffset;
	}

	// The vertex buffer for draw triangle
	m_triangleBuffer = sgedev->requestResource<Buffer>();
	m_triangleBuffer->create(BufferDesc::GetDefaultVertexBuffer(sizeof(Vertex2D) * 3, ResourceUsage::Dynamic), NULL);

	// Create the vertex buffer.
	m_vb2d = sgedev->requestResource<Buffer>();
	const BufferDesc vbd2D = BufferDesc::GetDefaultVertexBuffer(vbData.size() * sizeof(Vertex2D));
	m_vb2d->create(vbd2D, vbData.data());

	// Create the vertex buffer for the text rendering.
	m_textDrawing.vb = sgedev->requestResource<Buffer>();
	const BufferDesc vbdText =
	    BufferDesc::GetDefaultVertexBuffer(2048 * 6 * sizeof(Vertex2D), ResourceUsage::Dynamic); // 256 characters cache.
	m_textDrawing.vb->create(vbdText, NULL);

	// Create the shading programs.
	m_effect2DColored = sgedev->requestResource<ShadingProgram>();
	m_effect2DColored->createFromCustomHLSL(EFFECT_2D_UBERSHADER, EFFECT_2D_UBERSHADER);

	m_effect2DTextured = sgedev->requestResource<ShadingProgram>();
	const std::string effect2DTexturedCode = std::string("#define COLOR_TEXTURE\n") + EFFECT_2D_UBERSHADER;
	m_effect2DTextured->createFromCustomHLSL(effect2DTexturedCode.c_str(), effect2DTexturedCode.c_str());

	m_effect2DText = sgedev->requestResource<ShadingProgram>();
	const std::string effect2DTextCode = std::string("#define COLOR_TEXTURE\n#define COLOR_TEXTURE_TEXT_MODE\n") + EFFECT_2D_UBERSHADER;
	m_effect2DText->createFromCustomHLSL(effect2DTextCode.c_str(), effect2DTextCode.c_str());


	rsDefault = sgedev->requestResource<RasterizerState>();
	rsDefault->create(RasterDesc());

	{
		rsNoCullUseScissors = sgedev->requestResource<RasterizerState>();
		RasterDesc rd;
		rd.useScissor = true;
		rsNoCullUseScissors->create(rd);
	}

	RasterDesc rsNoCullingDesc;
	rsNoCullingDesc.cullMode = CullMode::None;
	rsNoCulling = sgedev->requestResource<RasterizerState>();
	rsNoCulling->create(rsNoCullingDesc);

	RasterDesc rdScissorsDesc;
	rdScissorsDesc.useScissor = true;
	rsScissors = sgedev->requestResource<RasterizerState>();
	rsScissors->create(rdScissorsDesc);

	// Create the alpha blending state.
	BlendDesc blendDesc;
	blendDesc.enabled = true;
	blendDesc.srcBlend = Blend::SrcColor;
	blendDesc.destBlend = Blend::InvSrcColor;
	blendDesc.blendOp = BlendOp::Add;
	blendDesc.alphaSrcBlend = Blend::Alpha_Src;
	blendDesc.alphaDestBlend = Blend::Alpha_InvSrc;
	blendDesc.blendOp = BlendOp::Add;

	m_textBlendState = sgedev->requestResource<BlendState>();
	m_textBlendState->create(blendDesc);

	// The 2D vertex declaration
	const VertexDecl vtxDecl_pos2d_uv[] = {
	    {0, "a_position", UniformType::Float2, 0},
	    {0, "a_uv", UniformType::Float2, 8},
	};

	vertexDeclIndex_pos2d_uv = sgedev->getVertexDeclIndex(vtxDecl_pos2d_uv, SGE_ARRSZ(vtxDecl_pos2d_uv));
}

void QuickDraw::initalize3DDrawResources(SGEContext* context) {
	SGEDevice* sgedev = context->getDevice();

	m_effect3DVertexColored = sgedev->requestResource<ShadingProgram>();
	m_effect3DVertexColored->createFromCustomHLSL(EFFECT_3D_VERTEX_COLOR, EFFECT_3D_VERTEX_COLOR);

	m_vb3d = sgedev->requestResource<Buffer>();
	m_vb3d->create(BufferDesc::GetDefaultVertexBuffer(VB_MAX_TRI_CNT * 3 * sizeof(vec3f), ResourceUsage::Dynamic), NULL);

	m_vbSphere = sgedev->requestResource<Buffer>();
	m_vbSphereNumPoints = GeomGen::sphere(m_vbSphere, 32, 32);

	m_vbCylinder = sgedev->requestResource<Buffer>();
	m_vbCylinderNumPoints = GeomGen::cylinder(m_vbCylinder, vec3f(0.0f, 0.0f, 0.0f), 100, 10.f, 2);

	// [CAUTION] Allocating big chunk of memory (almost 1MB).
	const int wiredVertexBufferSizeBytes = 65534 * sizeof(GeomGen::PosColorVert); // Around 1MB.

	m_vbWiredGeometry = sgedev->requestResource<Buffer>();
	const BufferDesc vbdWiredGeom = BufferDesc::GetDefaultVertexBuffer(wiredVertexBufferSizeBytes, ResourceUsage::Dynamic);
	m_vbWiredGeometry->create(vbdWiredGeom, NULL);

	m_vbSolidColorGeometry = sgedev->requestResource<Buffer>();
	const BufferDesc vbdSolidGeom = BufferDesc::GetDefaultVertexBuffer(wiredVertexBufferSizeBytes, ResourceUsage::Dynamic);
	m_vbSolidColorGeometry->create(vbdSolidGeom, NULL);

	// Vertex Declaration
	const VertexDecl vtxDecl_pos3d[] = {
	    {0, "a_position", UniformType::Float3, 0},
	};

	vertexDeclIndex_pos3d = sgedev->getVertexDeclIndex(vtxDecl_pos3d, SGE_ARRSZ(vtxDecl_pos3d));

	const VertexDecl vtxDecl_pos3d_rgba_int[] = {
	    {0, "a_position", UniformType::Float3, 0},
	    {0, "a_color", UniformType::Int_RGBA_Unorm_IA, 12},
	};

	vertexDeclIndex_pos3d_rgba_int = sgedev->getVertexDeclIndex(vtxDecl_pos3d_rgba_int, SGE_ARRSZ(vtxDecl_pos3d_rgba_int));
}

void QuickDraw::drawRect(const RenderDestination& rdest, const AABox2f& boxPixels, const vec4f& rgba, BlendState* blendState) {
	const vec2f boxSize = boxPixels.size();
	drawRect(rdest, boxPixels.min.x, boxPixels.min.y, boxSize.x, boxSize.y, rgba, blendState);
}

void QuickDraw::drawRect(
    const RenderDestination& rdest, float xPixels, float yPixels, float width, float height, const vec4f& rgba, BlendState* blendState) {
	const mat4f sizeScaling = mat4f::getScaling(width / 2.f, height / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(xPixels + width / 2.f, yPixels + height / 2.f, 0.f);
	const mat4f world = transl * sizeScaling;
	const mat4f ortho = mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;
	float alphaMult = 1.f;

	stateGroup.setRenderState(rsDefault, dssLessEqual, blendState);
	stateGroup.setProgram(m_effect2DColored);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("color", ShaderType::PixelShader), (void*)&rgba),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	};

	DrawCall dc;

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_rect2DShapeInfo.numPoints, m_rect2DShapeInfo.vertexOffset);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void QuickDraw::drawTriLeft(
    const RenderDestination& rdest, const AABox2f& boxPixels, float rotation, const vec4f& rgba, BlendState* blendState) {
	const vec2f size = boxPixels.size();

	const mat4f sizeScaling = mat4f::getScaling(size.x / 2.f, size.y / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(boxPixels.min.x + size.x / 2.f, boxPixels.min.y + size.y / 2.f, 0.f);
	const mat4f world = transl * mat4f::getRotationZ(rotation) * sizeScaling;
	const mat4f ortho = mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;
	float alphaMult = 1.f;

	stateGroup.setRenderState(rsDefault, dssLessEqual, blendState);
	stateGroup.setProgram(m_effect2DColored);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("color", ShaderType::PixelShader), (void*)&rgba),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	};

	DrawCall dc;

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_triLeftShapeInfo.numPoints, m_triLeftShapeInfo.vertexOffset);

	rdest.executeDrawCall(dc);
}

void QuickDraw::drawRectTexture(const RenderDestination& rdest,
                                float xPixels,
                                float yPixels,
                                float width,
                                float height,
                                Texture* texture,
                                BlendState* blendState,
                                vec2f topUV,
                                vec2f bottomUV,
                                float alphaMult) {
	if (texture && !texture->isValid()) {
		return;
	}

	vec4f region(topUV, bottomUV);

	const mat4f sizeScaling = mat4f::getScaling(width / 2.f, height / 2.f, 1.f);
	const mat4f transl = mat4f::getTranslation(xPixels + width / 2.f, yPixels + height / 2.f, 0.f);
	const mat4f world = transl * sizeScaling;
	const mat4f ortho = mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);
	const mat4f transf = ortho * world;

	stateGroup.setRenderState(rsDefault, dssLessEqual, blendState);
	stateGroup.setProgram(m_effect2DTextured);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	DrawCall dc;
	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();

	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
	    BoundUniform(refl.numericUnforms.findUniform("uvRegion", ShaderType::PixelShader), (void*)&region),
	    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
	    BoundUniform(refl.textures.findUniform("colorTexture", ShaderType::PixelShader), texture),
	};

	dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
	dc.setStateGroup(&stateGroup);
	dc.draw(m_rect2DShapeInfo.numPoints, m_rect2DShapeInfo.vertexOffset);

	rdest.sgecon->executeDrawCall(dc, rdest.frameTarget, &rdest.viewport);
}

void QuickDraw::drawRectTexture(const RenderDestination& rdest,
                                const AABox2f& boxPixels,
                                Texture* texture,
                                BlendState* blendState,
                                vec2f topUV,
                                vec2f bottomUV,
                                float alphaMult) {
	vec2f size = boxPixels.size();
	drawRectTexture(rdest, boxPixels.min.x, boxPixels.min.y, size.x, size.y, texture, blendState, topUV, bottomUV, alphaMult);
}

void QuickDraw::drawTexture(const RenderDestination& rdest,
                            float xPixels,
                            float yPixels,
                            float width,
                            Texture* texture,
                            BlendState* blendState,
                            vec2f topUV,
                            vec2f bottomUV,
                            float alphaMult) {
	if (!texture || !texture->isValid()) {
		return;
	}

	const TextureDesc desc = texture->getDesc();

	float aspectRatio = 0.f;
	if (desc.textureType == UniformType::Texture1D)
		aspectRatio = desc.texture1D.width / 1.f;
	if (desc.textureType == UniformType::Texture2D)
		aspectRatio = (float)desc.texture2D.width / (float)desc.texture2D.height;
	else {
		sgeAssert(false);
	}

	const float height = floorf(width / aspectRatio);

	drawRectTexture(rdest, xPixels, yPixels, width, height, texture, blendState, topUV, bottomUV, alphaMult);
}

void QuickDraw::drawTextLazy(const RenderDestination& rdest,
                             DebugFont& font,
                             vec2f posPixels,
                             const vec4f& rgba,
                             const char* text,
                             float height,
                             const Rect2s* scissors) {
	float alphaMult = 1.f;

	RasterizerState* rs = scissors == nullptr ? rsDefault : rsNoCullUseScissors;
	stateGroup.setRenderState(rs, dssLessEqual, m_textBlendState);

	stateGroup.setProgram(m_effect2DText);
	stateGroup.setVB(0, m_vb2d, 0, sizeof(Vertex2D));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos2d_uv);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const mat4f ortho = mat4f::getOrthoRH(rdest.viewport.width, rdest.viewport.height, 0.f, 1000.f, kIsTexcoordStyleD3D);

	const float textureWidth = (float)font.texture->getDesc().texture2D.width;
	const float textureHeight = (float)font.texture->getDesc().texture2D.height;

	float x = posPixels.x;
	float y = posPixels.y;
	const float sizeScaling = (height >= 0.f) ? height / (float)font.height : 1.f;

	mat4f world;
	while ((*text != '\0')) {
		if (*text == '\n') {
			// Move the cursor to a new line and reset the x position.
			x = posPixels.x;
			y += font.height * sizeScaling;
			text += 1;
			continue;
		}

		auto const& cdata = font.cdata[*text];
		const float halfWidth = (float)(cdata.x1 - cdata.x0) * 0.5f * sizeScaling;
		const float halfHeight = (float)(cdata.y1 - cdata.y0) * 0.5f * sizeScaling;

		if (*text != ' ') {
			world.data[0] = vec4f(halfWidth, 0.f, 0.f, 0.f);
			world.data[1] = vec4f(0.f, halfHeight, 0.f, 0.f);
			world.data[2] = vec4f(0.f, 0.f, 1.f, 0.f);
			world.data[3] = vec4f(x + halfWidth, y + halfHeight + cdata.yoff * sizeScaling, 0.f, 1.f);

			// Previously the code above looked like this:
			// const mat4f sizing = mat4f::getScaling(halfWidth, halfHeight, 0.f);
			// const mat4f align = mat4f::getTranslation(halfWidth, halfHeight - ascent, 0.f);
			// const mat4f scaling = mat4f::getScaling(sizeScaling, sizeScaling, 1.f);
			// const mat4f transl = mat4f::getTranslation(q.x0 , y, 0.f);
			// const mat4f transf = ortho * transl * scaling * align * sizing;

			const mat4f transf = ortho * world;

			const vec4f uvRegion(cdata.x0 / textureWidth, cdata.y0 / textureHeight, cdata.x1 / textureWidth, cdata.y1 / textureHeight);

			const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
			BoundUniform uniforms[] = {
			    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&transf),
			    BoundUniform(refl.numericUnforms.findUniform("uvRegion", ShaderType::PixelShader), (void*)&uvRegion),
			    BoundUniform(refl.numericUnforms.findUniform("colorText", ShaderType::PixelShader), (void*)&rgba),
			    BoundUniform(refl.numericUnforms.findUniform("alphaMult", ShaderType::PixelShader), (void*)&alphaMult),
			    BoundUniform(refl.textures.findUniform("colorTexture", ShaderType::PixelShader), font.texture),
			};

			DrawCall dc;
			dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
			dc.setStateGroup(&stateGroup);
			dc.draw(m_rect2DShapeInfo.numPoints, m_rect2DShapeInfo.vertexOffset);

			rdest.executeDrawCall(dc, scissors);
		}

		x += sizeScaling * font.cdata[*text].xadvance; // advance to the start point of the next character...

		text++;
	}
}

void QuickDraw::drawWiredAdd_Line(const vec3f& a, const vec3f& b, const uint32 rgba) {
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, rgba));
}

void QuickDraw::drawWiredAdd_Arrow(const vec3f& from, const vec3f& to, const uint32 rgba) {
	const vec3f diff = to - from;
	vec3f ax;
	if (isEpsEqual(diff.x, diff.z)) {
		ax = vec3f(diff.x, diff.z, -diff.y);
	} else {
		ax = vec3f(-diff.z, diff.y, diff.x);
	}

	const vec3f perp0 = cross(diff, ax).normalized() * diff.length() * 0.15f;
	const vec3f perp1 = quatf::getAxisAngle(diff.normalized(), half_pi()).transformDir(perp0);

	const vec3f toBack = diff * -0.15f;

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from + perp0, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from - perp0, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from + perp1, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(from, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(from - perp1, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to + perp0 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to - perp0 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to + perp1 + toBack, rgba));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(to, rgba));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(to - perp1 + toBack, rgba));
}

void QuickDraw::drawWiredAdd_Box(const mat4f& world, const uint32 rgba) {
	GeomGen::wiredBox(m_wireframeVerts, world, rgba);
}

void QuickDraw::drawWiredAdd_Box(const AABox3f& aabb, const uint32 rgba) {
	GeomGen::wiredBox(m_wireframeVerts, aabb, rgba);
}

void QuickDraw::drawWiredAdd_Box(const mat4f& world, const AABox3f& aabb, const uint32 rgba) {
	const size_t newBoxStart = m_wireframeVerts.size();

	GeomGen::wiredBox(m_wireframeVerts, aabb, rgba);
	for (size_t t = newBoxStart; t < m_wireframeVerts.size(); ++t) {
		m_wireframeVerts[t].pt = (world * vec4f(m_wireframeVerts[t].pt, 1.f)).xyz();
	}
}

void QuickDraw::drawWiredAdd_Capsule(const mat4f& world, const uint32 rgba, float height, float radius, int numSides) {
	GeomGen::wiredCapsule(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::center);
}

void QuickDraw::drawWiredAdd_Sphere(const mat4f& world, const uint32 rgba, float radius, int numSides) {
	GeomGen::wiredSphere(m_wireframeVerts, world, rgba, radius, numSides);
}

void QuickDraw::drawWiredAdd_Cylinder(const mat4f& world, const uint32 rgba, float height, float radius, int numSides) {
	GeomGen::wiredCylinder(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::center);
}

void QuickDraw::drawWiredAdd_ConeBottomAligned(const mat4f& world, const uint32 rgba, float height, float radius, int numSides) {
	GeomGen::wiredCone(m_wireframeVerts, world, rgba, height, radius, numSides, GeomGen::Bottom);
}

void QuickDraw::drawWiredAdd_Basis(const mat4f& world) {
	GeomGen::wiredBasis(m_wireframeVerts, world);
}

void QuickDraw::drawWiredAdd_Bone(const mat4f& n2w, float length, float radius, const vec4f& color) {
	const int startVertex = int(m_wireframeVerts.size());

	int intColor = colorToIntRgba(color);
	// create the Bone oriented along the X-axis.

	const float midX = length * 0.3f;

	// 1st small pyramid
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(0.f), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));

	// The middle ring.
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));

	// 2nd bigger pyramid.
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, -radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, -radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(midX, radius, radius), intColor));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(vec3f(length, 0.f, 0.f), intColor));

	// Now transform vertices to world space using the provide matrix.
	for (int t = startVertex; t < int(m_wireframeVerts.size()); ++t) {
		m_wireframeVerts[t].pt = mat_mul_pos(n2w, m_wireframeVerts[t].pt);
	}
}

void QuickDraw::drawWiredAdd_Grid(
    const vec3f& origin, const vec3f& xAxis, const vec3f& zAxis, const int xLines, const int yLines, const int color) {
	GeomGen::wiredGrid(m_wireframeVerts, origin, xAxis, zAxis, xLines, yLines, color);
}

void QuickDraw::drawWiredAdd_triangle(const vec3f& a, const vec3f& b, const vec3f& c, const int color) {
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(b, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(c, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(c, color));
	m_wireframeVerts.push_back(GeomGen::PosColorVert(a, color));
}

void QuickDraw::drawWired_Clear() {
	m_wireframeVerts.clear();
}

void QuickDraw::drawWired_Execute(const RenderDestination& rdest, const mat4f& projView, BlendState* blendState, DepthStencilState* dss) {
	if (m_wireframeVerts.size() == 0) {
		return;
	}

	sgeAssert(m_wireframeVerts.size() % 2 == 0);

	stateGroup.setRenderState(rsDefault, dss ? dss : dssLessEqual.GetPtr(), blendState);
	stateGroup.setProgram(m_effect3DVertexColored);
	stateGroup.setVB(0, m_vbWiredGeometry, 0, sizeof(GeomGen::PosColorVert));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos3d_rgba_int);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::LineList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&projView),
	};

	const uint32 vbSizeVerts = uint32(m_vbWiredGeometry->getDesc().sizeBytes) / (sizeof(GeomGen::PosColorVert));

	while (m_wireframeVerts.size()) {
		const uint32 numVertsToCopy = (m_wireframeVerts.size() > vbSizeVerts) ? vbSizeVerts : uint32(m_wireframeVerts.size());

		GeomGen::PosColorVert* const vbdata = (GeomGen::PosColorVert*)rdest.sgecon->map(m_vbWiredGeometry, Map::WriteDiscard);
		std::copy(m_wireframeVerts.begin(), m_wireframeVerts.begin() + numVertsToCopy, vbdata);
		rdest.sgecon->unMap(m_vbWiredGeometry);

		m_wireframeVerts.erase(m_wireframeVerts.begin(), m_wireframeVerts.begin() + numVertsToCopy);

		DrawCall dc;

		dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
		dc.setStateGroup(&stateGroup);
		dc.draw(numVertsToCopy, 0);

		rdest.executeDrawCall(dc);
	}

	drawWired_Clear();
}

void QuickDraw::drawSolidAdd_Triangle(const vec3f a, const vec3f b, const vec3f c, const uint32 rgba) {
	m_solidColorVerts.push_back(GeomGen::PosColorVert(a, rgba));
	m_solidColorVerts.push_back(GeomGen::PosColorVert(b, rgba));
	m_solidColorVerts.push_back(GeomGen::PosColorVert(c, rgba));
}

void QuickDraw::drawSolidAdd_Quad(const vec3f& origin, const vec3f& ex, const vec3f& ey, const uint32 rgba) {
	drawSolidAdd_Triangle(origin, origin + ex, origin + ey, rgba);
	drawSolidAdd_Triangle(origin + ey, origin + ex, origin + ex + ey, rgba);
}

void QuickDraw::drawSolidAdd_QuadCentered(const vec3f& center, const vec3f& exHalf, const vec3f& eyHalf, const uint32 rgba) {
	drawSolidAdd_Quad(center - exHalf - eyHalf, 2.f * exHalf, 2.f * eyHalf, rgba);
}

void QuickDraw::drawSolid_Execute(const RenderDestination& rdest,
                                  const mat4f& projViewWorld,
                                  bool shouldUseCulling,
                                  BlendState* blendState) {
	if (m_solidColorVerts.size() == 0) {
		return;
	}

	// As we are drawing trianges, these must be multiple of 3.
	sgeAssert(m_solidColorVerts.size() % 3 == 0);

	stateGroup.setRenderState(shouldUseCulling ? rsDefault : rsNoCulling, dssLessEqual, blendState);
	stateGroup.setProgram(m_effect3DVertexColored);
	stateGroup.setVB(0, m_vbSolidColorGeometry, 0, sizeof(GeomGen::PosColorVert));
	stateGroup.setVBDeclIndex(vertexDeclIndex_pos3d_rgba_int);
	stateGroup.setPrimitiveTopology(PrimitiveTopology::TriangleList);

	const ShadingProgramRefl& refl = stateGroup.m_shadingProg->getReflection();
	BoundUniform uniforms[] = {
	    BoundUniform(refl.numericUnforms.findUniform("projViewWorld", ShaderType::VertexShader), (void*)&projViewWorld),
	};

	// CAUTION: clamp the actual size to something multiple of 3
	// as we are going to draw triangles and the loop below assumes it.
	const unsigned vbSizeVerts = ((uint32(m_vbSolidColorGeometry->getDesc().sizeBytes) / (sizeof(GeomGen::PosColorVert))) / 3) * 3;
	sgeAssert(vbSizeVerts % 3 == 0);

	while (m_solidColorVerts.size()) {
		const unsigned numVertsToCopy = (m_solidColorVerts.size() > vbSizeVerts) ? vbSizeVerts : uint32(m_solidColorVerts.size());

		sgeAssert(numVertsToCopy % 3 == 0);

		GeomGen::PosColorVert* const vbdata = (GeomGen::PosColorVert*)rdest.sgecon->map(m_vbSolidColorGeometry, Map::WriteDiscard);
		std::copy(m_solidColorVerts.begin(), m_solidColorVerts.begin() + numVertsToCopy, vbdata);
		rdest.sgecon->unMap(m_vbSolidColorGeometry);

		m_solidColorVerts.erase(m_solidColorVerts.begin(), m_solidColorVerts.begin() + numVertsToCopy);

		DrawCall dc;

		dc.setUniforms(uniforms, SGE_ARRSZ(uniforms));
		dc.setStateGroup(&stateGroup);
		dc.draw(numVertsToCopy, 0);

		rdest.executeDrawCall(dc);
	}
}

} // namespace sge
