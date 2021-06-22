#pragma once

#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/math/Box.h"
#include "sge_utils/math/mat4.h"
#include "sge_utils/math/primitives.h"
#include "sgecore_api.h"
#include <stb_truetype.h>

#include "GeomGen.h"

#include <string>

namespace sge {

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
struct SGE_CORE_API DebugFont {
  public:
	DebugFont() { Destroy(); }
	~DebugFont() { Destroy(); }

	bool Create(SGEDevice* sgedev, const char* const ttfFilename, float heightPixels);
	void Destroy();

	vec2f computeTextDimensions(const char* text, float textHeight) const;

	float getScalingForHeight(float targetHeightPixels) const { return height / targetHeightPixels; }

	GpuHandle<Texture> texture;

	// Every value here is in pixels.
	// Explanation: https://www.microsoft.com/typography/otspec/TTCH01.htm
	float height = 0.f; // The height of the baked font.
	float maxAscent = 0.f;
	float maxDescent = 0.f;
	float maxHorizontalAdvance = 0.f;

	stbtt_bakedchar cdata[256];
};

//////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////
struct SGE_CORE_API QuickDraw {
	struct Vertex2D {
		Vertex2D() = default;

		Vertex2D(sge::vec2f p, sge::vec2f uv)
		    : p(p)
		    , uv(uv) {}

		sge::vec2f p;
		sge::vec2f uv;
	};

	QuickDraw() = default;

	bool initialize(SGEContext* context);

	// 2D and Text drawing.
	void drawRect(const RenderDestination& rdest, const AABox2f& boxPixels, const vec4f& rgba, BlendState* blendState = nullptr);
	void drawRect(const RenderDestination& rdest,
	              float xPixels,
	              float yPixels,
	              float width,
	              float height,
	              const vec4f& rgba,
	              BlendState* blendState = nullptr);
	void drawTriLeft(
	    const RenderDestination& rdest, const AABox2f& boxPixels, float rotation, const vec4f& rgba, BlendState* blendState = nullptr);

	void drawRectTexture(const RenderDestination& rdest,
	                     float xPixels,
	                     float yPixels,
	                     float width,
	                     float height,
	                     Texture* texture,
	                     BlendState* blendState = nullptr,
	                     vec2f topUV = vec2f(0),
	                     vec2f bottomUV = vec2f(1.f),
	                     float alphaMult = 1.f);


	void drawRectTexture(const RenderDestination& rdest,
	                     const AABox2f& boxPixels,
	                     Texture* texture,
	                     BlendState* blendState = nullptr,
	                     vec2f topUV = vec2f(0),
	                     vec2f bottomUV = vec2f(1.f),
	                     float alphaMult = 1.f);

	void drawTexture(
	    const RenderDestination& rdest,
	    float xPixels,
	    float yPixels,
	    float width,
	    Texture* texture,
	    BlendState* blendState = nullptr,
	    vec2f topUV = vec2f(0),
	    vec2f bottomUV = vec2f(1.f),
	    float alphaMult = 1.f); // Draws a textured quad using width and auto picking width based on the aspect ratio of the image.

	/// @param [in] posPixels the position of the 1st letter bottom left corner.
	void drawTextLazy(const RenderDestination& rdest,
	                  DebugFont& font,
	                  vec2f posPixels,
	                  const vec4f& rgba,
	                  const char* text,
	                  float height = -1.f,
	                  const Rect2s* scissors = nullptr);

	// These methods add the specified primitive to the rendering queue.
	// The queue is executed(and then cleared by) drawWired_Execute
	void drawWiredAdd_Line(const vec3f& a, const vec3f& b, const uint32 rgba);
	void drawWiredAdd_Arrow(const vec3f& from, const vec3f& to, const uint32 rgba);
	void drawWiredAdd_Box(const mat4f& world, const uint32 rgba);
	void drawWiredAdd_Box(const AABox3f& aabb, const uint32 rgba);
	void drawWiredAdd_Box(const mat4f& world, const AABox3f& aabb, const uint32 rgba);
	void drawWiredAdd_Sphere(const mat4f& world, const uint32 rgba, float radius, int numSides = 3);
	void drawWiredAdd_Capsule(const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_Cylinder(const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_ConeBottomAligned(const mat4f& world, const uint32 rgba, float height, float radius, int numSides = 3);
	void drawWiredAdd_Basis(const mat4f& world);
	void drawWiredAdd_Bone(const mat4f& n2w, float length, float radius, const vec4f& color = vec4f(1.f));
	void drawWiredAdd_Grid(const vec3f& origin,
	                       const vec3f& xAxis,
	                       const vec3f& zAxis,
	                       const int xLines,
	                       const int yLines, // Then number of lines in x and z axis.
	                       const int color = 0x000000);
	void drawWiredAdd_triangle(const vec3f& a, const vec3f& b, const vec3f& c, const int color = 0x000000);

	// Removes all curretly queued primitives for drawing.
	void drawWired_Clear();
	void drawWired_Execute(const RenderDestination& rdest,
	                       const mat4f& projViewWorld,
	                       BlendState* blendState = nullptr,
	                       DepthStencilState* dss = nullptr);

	void drawSolidAdd_Triangle(const vec3f a, const vec3f b, const vec3f c, const uint32 rgba);
	void drawSolidAdd_Quad(const vec3f& origin, const vec3f& ex, const vec3f& ey, const uint32 rgba);
	void drawSolidAdd_QuadCentered(const vec3f& center, const vec3f& exHalf, const vec3f& eyHalf, const uint32 rgba);

	void drawSolid_Execute(const RenderDestination& rdest,
	                       const mat4f& projViewWorld,
	                       bool shouldUseCulling = true,
	                       BlendState* blendState = nullptr);

  private:
	void initalize2DDrawResources(SGEContext* context);
	void initalize3DDrawResources(SGEContext* context);

	//------------------------------------------------------
	// Common
	//------------------------------------------------------
	GpuHandle<DepthStencilState> dssLessEqual;
	GpuHandle<RasterizerState> rsDefault;
	GpuHandle<RasterizerState> rsNoCullUseScissors;
	GpuHandle<RasterizerState> rsNoCulling;
	GpuHandle<RasterizerState> rsScissors;

	GpuHandle<SamplerState> m_textSamplerState;
	GpuHandle<BlendState> m_textBlendState;

	//------------------------------------------------------
	// 2D rendering resources
	//------------------------------------------------------
	VertexDeclIndex vertexDeclIndex_pos2d_uv;
	GpuHandle<Buffer> m_vb2d;
	GpuHandle<Buffer> m_triangleBuffer;
	GpuHandle<ShadingProgram> m_effect2DColored, m_effect2DTextured, m_effect2DText;

	// Text rendering. in order to accaelerate the text rendering this structure is used
	// to cache text vertex buffer.
	struct TextDrawingData {
		GpuHandle<Buffer> vb;
		std::vector<Vertex2D> cpuDataCache;
		DebugFont* font = NULL; // Font to be used for cached rendering.
		vec4f rgba = vec4f(0.f);
	};

	struct Shape2DInfo {
		int vertexOffset = 0;
		int numPoints = 0;
	};

	TextDrawingData m_textDrawing;
	Shape2DInfo m_rect2DShapeInfo;
	Shape2DInfo m_triLeftShapeInfo;
	Shape2DInfo m_circle2DShapeInfo;

	GpuHandle<Buffer> m_vb2dReusable;

	//------------------------------------------------------
	// 3D Rendering resources
	//------------------------------------------------------
	enum { VB_MAX_TRI_CNT = 100 };
	VertexDeclIndex vertexDeclIndex_pos3d;
	VertexDeclIndex vertexDeclIndex_pos3d_rgba_int;
	GpuHandle<Buffer> m_vb3d;
	GpuHandle<ShadingProgram> m_effect3DVertexColored;

	GpuHandle<Buffer> m_vbSphere;
	int m_vbSphereNumPoints;

	GpuHandle<Buffer> m_vbCylinder;
	int m_vbCylinderNumPoints;

	// Resources for the wireframe geometry of the whole scene.
	std::vector<GeomGen::PosColorVert> m_wireframeVerts;
	GpuHandle<Buffer> m_vbWiredGeometry;

	// Resources for the wireframe geometry of the whole scene.
	std::vector<GeomGen::PosColorVert> m_solidColorVerts;
	GpuHandle<Buffer> m_vbSolidColorGeometry;

	StateGroup stateGroup;
};

} // namespace sge
