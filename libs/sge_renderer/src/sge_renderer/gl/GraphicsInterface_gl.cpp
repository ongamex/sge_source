#include "sge_utils/utils/timer.h"
#include <iostream>

#include "GraphicsCommon_gl.h"
#include "GraphicsInterface_gl.h"

#if WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <GL/wglew.h>
#endif

namespace sge {

//////////////////////////////////////////////////////////////////////////////////////////
// Create/Destroy
//////////////////////////////////////////////////////////////////////////////////////////
bool SGEDeviceImpl::Create(const MainFrameTargetDesc& frameTargetDesc) {
#if defined(WIN32) && 0

	m_windowFrameTargetDesc = frameTargetDesc;

	const HWND hwnd = (HWND)frameTargetDesc.hWindow;
	HDC hdc = GetDC(hwnd);
	sgeAssert(hdc);

	const PIXELFORMATDESCRIPTOR pfd = {
	    sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
	    1,                             // Version Number
	    PFD_DRAW_TO_WINDOW |           // Format Must Support Window
	        PFD_SUPPORT_OPENGL |       // Format Must Support OpenGL
	        PFD_DOUBLEBUFFER,          // Must Support Double Buffering
	    PFD_TYPE_RGBA,                 // Request An RGBA Format
	    8,                             // Select Our Color Depth, 8 bits / channel
	    0,
	    0,
	    0,
	    0,
	    0,
	    0, // Color Bits Ignored
	    0, // No Alpha Buffer
	    0, // Shift Bit Ignored
	    0, // No Accumulation Buffer
	    0,
	    0,
	    0,
	    0,              // Accumulation Bits Ignored
	    24,             // 32 bit Z-Buffer (Depth Buffer)
	    8,              // No Stencil Buffer
	    0,              // No Auxiliary Buffer
	    PFD_MAIN_PLANE, // Main Drawing Layer
	    0,              // Reserved
	    0,
	    0,
	    0 // Layer Masks Ignored
	};

	const int iFormat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, iFormat, &pfd);

	// Create a temp context.
	HGLRC tempContext = wglCreateContext(hdc);
	wglMakeCurrent(hdc, tempContext);

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		sgeAssert(false);
		return false;
	}

	int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
	                 3,
	                 WGL_CONTEXT_MINOR_VERSION_ARB,
	                 2,
	                 WGL_CONTEXT_PROFILE_MASK_ARB,
	                 WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#ifdef SGE_USE_DEBUG
//	WGL_CONTEXT_FLAGS_ARB,  WGL_CONTEXT_DEBUG_BIT_ARB,
#else
	                 WGL_CONTEXT_FLAGS_ARB,
	                 0,
#endif
	                 0};

	HGLRC hrc = wglCreateContextAttribsARB(hdc, 0, attribs);
	// HGLRC hrc = wglCreateContext(hdc);

	//
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(tempContext);
	tempContext = 0;

	if (!wglMakeCurrent(hdc, hrc)) {
		return false;
	}

	m_gl_hdc = hdc;

#endif
	DumpAllGLErrors();

#if !defined(__EMSCRIPTEN__)
	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		sgeAssert(false);
		return false;
	}
#endif



	DumpAllGLErrors();

	//[[maybe_unused]] const GLubyte* glVersion = glGetString(GL_VERSION);
	// SGE_DEBUG_LOG("OpenGL Version = %s\n", glVersion);

	m_immContext = new SGEContextImmediate;
	m_immContext->SetSGEDevice(this);

	m_screenTarget = requestResource(ResourceType::FrameTarget);
	m_screenTarget.as<FrameTargetGL>()->GL_CreateWindowFrameTarget(frameTargetDesc.width, frameTargetDesc.height);

	setVsync(frameTargetDesc.vSync);

	//[[maybe_unused]] const GLubyte* vendor = glGetString(GL_VENDOR);     // Returns the vendor
	//[[maybe_unused]] const GLubyte* renderer = glGetString(GL_RENDERER); // Returns a hint to the model

	// SGE_DEBUG_LOG("Vendor = %s\nRenderer = %s\n", vendor, renderer);

	// The code uses only one VAO.
	GLuint globallyUsedDefaultVAO = -1;
	glGenVertexArrays(1, &globallyUsedDefaultVAO);
	glBindVertexArray(globallyUsedDefaultVAO);
	DumpAllGLErrors();

	return true;
}

void SGEDeviceImpl::resizeBackBuffer(int width, int height) {
	m_windowFrameTargetDesc.width = width;
	m_windowFrameTargetDesc.height = height;
	m_screenTarget.as<FrameTargetGL>()->GL_CreateWindowFrameTarget(m_windowFrameTargetDesc.width, m_windowFrameTargetDesc.height);
}

void SGEDeviceImpl::setVsync(const bool enabled) {
	m_VSyncEnabled = enabled;

#if defined(WIN32) && 0
	if (wglSwapIntervalEXT != NULL) {
		wglSwapIntervalEXT(m_VSyncEnabled ? 1 : 0);
	} else {
		sgeAssert(false);
	}
#endif
}

void SGEDeviceImpl::Destroy() {
}

void SGEDeviceImpl::present() {
#if defined(WIN32)
	glFinish();
	SwapBuffers((HDC)m_gl_hdc);
#else //[TODO]

#endif

	float const now = Timer::now_seconds();

	m_frameStatistics.Reset();
	m_frameStatistics.lastPresentDt = now - m_frameStatistics.lastPresentTime;
	m_frameStatistics.lastPresentTime = now;
}

void SGEContextImmediate::beginQuery(Query* const query) {
#ifndef __EMSCRIPTEN__
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return;
	}

	GLenum glNativeQueryType = QueryType_GetGLnative(query->getType());
	sgeAssert(glNativeQueryType != SGE_GL_UNKNOWN);

	QueryGL* const gl_query = (QueryGL*)query;
	GLuint glNativeQueryId = gl_query->GL_GetResource();

	glBeginQuery(glNativeQueryType, glNativeQueryId);
	DumpAllGLErrors();
#else
	sgeAssert(false && "Query is not supported on emscripten");
#endif
}

void SGEContextImmediate::endQuery(Query* const query) {
#ifndef __EMSCRIPTEN__
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return;
	}

	GLenum glNativeQueryType = QueryType_GetGLnative(query->getType());
	sgeAssert(glNativeQueryType != SGE_GL_UNKNOWN);

	QueryGL* const gl_query = (QueryGL*)query;
	[[maybe_unused]] GLuint debug_glNativeQueryId = gl_query->GL_GetResource();

	// CAUTION: TODO: multiple queries could be bound at the same time!
	// This is currently not handled!
	glEndQuery(glNativeQueryType);
	DumpAllGLErrors();
#else
	sgeAssert(false && "Query is not supported on emscripten");
#endif
}

bool SGEContextImmediate::isQueryReady(Query* const query) {
#ifndef __EMSCRIPTEN__
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return false;
	}

	QueryGL* const gl_query = (QueryGL*)query;
	GLuint glNativeQueryId = gl_query->GL_GetResource();

	GLint queryResult;
	glGetQueryObjectiv(glNativeQueryId, GL_QUERY_RESULT_AVAILABLE, &queryResult);
	DumpAllGLErrors();

	return queryResult != GL_FALSE;
#else
	sgeAssert(false && "Query is not supported on emscripten");
#endif
}

bool SGEContextImmediate::getQueryData(Query* const query, uint64& queryData) {
#ifndef __EMSCRIPTEN__
	if (!query || !query->isValid()) {
		sgeAssert(false);
		return false;
	}

	QueryGL* const gl_query = (QueryGL*)query;
	GLuint glNativeQueryId = gl_query->GL_GetResource();

	GLint queryResult;
	glGetQueryObjectiv(glNativeQueryId, GL_QUERY_RESULT, &queryResult);
	DumpAllGLErrors();

	queryData = queryResult;

	return true;
#else
	sgeAssert(false && "Query is not supported on emscripten");
	return false;
#endif
}


void SGEContextImmediate::updateTextureData(Texture* texture, const TextureData& texData) {
	if (texture && texture->getDesc().textureType == UniformType::Texture2D && texData.data) {
		GLContextStateCache* const glcon = GL_GetContextStateCache();

		TextureGL* textureGl = static_cast<TextureGL*>(texture);

		glcon->BindTexture(GL_TEXTURE_2D, textureGl->GL_GetResource());

		GLint glInternalFormat;
		GLenum glFormat, glType;
		TextureFormat_GetGLNative(textureGl->getDesc().format, glInternalFormat, glFormat, glType);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, textureGl->getDesc().texture2D.width, textureGl->getDesc().texture2D.height, 0,
		             glFormat, glType, texData.data);
		DumpAllGLErrors();
	}
}

void SGEContextImmediate::clearColor(FrameTarget* target, int index, const float rgba[4]) {
	if (target == NULL || !target->isValid()) {
		// Called with initialized or invalid frame target.
		sgeAssert(false);
		return;
	}

	// If loop idx is -1, all render targets must be cleared.
	int loopUpIdx = GraphicsCaps::kRenderTargetSlotsCount;
	if (index == -1) {
		index = 0;
		loopUpIdx = index + 1;
	}

	const bool isWindowFBO = ((FrameTargetGL*)target)->GL_IsWindowFrameBufferWrapper();
	const GLuint fbo = ((FrameTargetGL*)target)->GL_GetResource();

	// Bind the frame buffer.
	getDeviceImpl()->GL_GetContextStateCache()->ApplyRasterDesc(RasterDesc());
	getDeviceImpl()->GL_GetContextStateCache()->BindFBO(fbo);

	// [TODO] Could we skip all the checks and just call glClearBufferfv?
	// Clear each texture.
	for (; index < loopUpIdx; ++index) {
		sge::Texture* pTex = (isWindowFBO == false) ? target->getRenderTarget(index) : NULL;

		if (isWindowFBO || (pTex && pTex->isValid())) {
#if 1
			glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
			glClear(GL_COLOR_BUFFER_BIT);
			DumpAllGLErrors();
#else
			glClearBufferfv(GL_COLOR, index, rgba);
			DumpAllGLErrors();
#endif
		}
	}
}

//---------------------------------------------------------------------------------------
// SGEContextImmediate
//---------------------------------------------------------------------------------------
void SGEContextImmediate::clearDepth(FrameTarget* target, float depth) {
	// [CAUTION] Some nVidia clearing depth does nothing if the
	// depth write is not enabled !!!
	// As a workaround we always enable the depth write when clearing...
	GL_GetContextStateCache()->DepthMask(GL_TRUE);

	if (target == NULL || !target->isValid()) {
		sgeAssert(false); // Called with initialized frame target.
		return;
	}

	const GLuint fbo = ((FrameTargetGL*)target)->GL_GetResource();
	GL_GetContextStateCache()->BindFBO(fbo);

#if defined(__EMSCRIPTEN__)
	glClearDepthf(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
	DumpAllGLErrors();
#else
	glClearDepth(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
	DumpAllGLErrors();
#endif
}

void* SGEContextImmediate::map(Buffer* buffer, const Map::Enum map) {
	void* result = ((BufferGL*)buffer)->map(map);
	DumpAllGLErrors();
	return result;
}

void SGEContextImmediate::unMap(Buffer* buffer) {
	((BufferGL*)buffer)->unMap();
	DumpAllGLErrors();
}

void SGEContextImmediate::executeDrawCall(DrawCall& drawCall,
                                          FrameTarget* frameTarget,
                                          const Rect2s* const pViewport,
                                          const Rect2s* const pScissorsRect) {
	StateGroup* const stateGroup = drawCall.m_pStateGroup;

	// Check if the draw call is valid:
	// sgeAssert(drawCall.ValidateDrawCall() == true);

	GLContextStateCache* const glcon = getDeviceImpl()->GL_GetContextStateCache();

	// Vertex attributes and vertex buffers.
	sgeAssert(stateGroup->m_shadingProg);
	VertexMapperGL* const vertMapper = ((ShadingProgramGL*)stateGroup->m_shadingProg)->GetVertexMapper(stateGroup->m_vertDeclIndex);

	sgeAssert(vertMapper);
	{
		const std::vector<VertexMapperGL::GL_AttribLayout>& glAttribLayout = vertMapper->GL_GetVertexLayout();
		for (int t = 0; t < (int)glAttribLayout.size(); ++t) {
			GLenum attrbType;
			GLint attribAirty;
			GLboolean attibNormalized;
			UniformType_ToGLUniformType(glAttribLayout[t].type, attrbType, attribAirty, attibNormalized);

			GLuint const buffer = ((BufferGL*)(stateGroup->m_vertexBuffers[glAttribLayout[t].bufferSlot]))->GL_GetResource();
			GLuint const byteOffset = glAttribLayout[t].byteOffset;
			GLuint const stride = stateGroup->m_vbStrides[glAttribLayout[t].bufferSlot];

			// Due to the lack of "glDrawElementsBaseVertex" under OpenGL ES*
			// we are forced to add that offset here.
			int drawIndexedBaseVertexAdditionOffset = 0;
			if (drawCall.m_drawExec.GetType() == DrawExecDesc::Type_Indexed) {
				drawIndexedBaseVertexAdditionOffset = drawCall.m_drawExec.IndexedCall().startVertex * stride;
			}

			glcon->SetVertexAttribSlotState(buffer != 0, glAttribLayout[t].index, buffer, attribAirty, attrbType, attibNormalized, stride,
			                                byteOffset + drawIndexedBaseVertexAdditionOffset);
		}
	}

	// Index buffer.
	if (stateGroup->m_indexBuffer != nullptr) {
		const GLuint indexBuffer = ((BufferGL*)stateGroup->m_indexBuffer)->GL_GetResource();
		glcon->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	}

	// The shading program.
	glcon->UseProgram(((ShadingProgramGL*)stateGroup->m_shadingProg)->GL_GetProgram());

	// Depth stencil state.
	if (stateGroup->m_depthStencilState != nullptr) {
		glcon->ApplyDepthStencilDesc(stateGroup->m_depthStencilState->getDesc());
	} else {
		glcon->ApplyDepthStencilDesc(DepthStencilDesc());
	}

	// Bind the framebuffer.
	glcon->BindFBO(((FrameTargetGL*)frameTarget)->GL_GetResource());

	// Bounded resources.
	for (int iUniform = 0; iUniform < drawCall.numUniforms; ++iUniform) {
		const BoundUniform& binding = drawCall.uniforms[iUniform];
		const void* const boundData = binding.data;
		const UniformType::Enum uniformType = (UniformType::Enum)binding.bindLocation.uniformType;

		sgeAssert(binding.bindLocation.glArraySize >= 1);
		switch (uniformType) {
			// Numeric uniforms.
			case UniformType::Int: {
				glUniform1i(binding.bindLocation.bindLocation, *(int*)boundData);
			} break;
			case UniformType::Float: {
				glUniform1f(binding.bindLocation.bindLocation, *(float*)boundData);
			} break;
			case UniformType::Float2: {
				glUniform2fv(binding.bindLocation.bindLocation, binding.bindLocation.glArraySize, (float*)boundData);
			} break;
			case UniformType::Float3: {
				glUniform3fv(binding.bindLocation.bindLocation, binding.bindLocation.glArraySize, (float*)boundData);
			} break;
			case UniformType::Float4: {
				glUniform4fv(binding.bindLocation.bindLocation, binding.bindLocation.glArraySize, (float*)boundData);
			} break;
			case UniformType::Float4x4: {
				glUniformMatrix4fv(binding.bindLocation.bindLocation, binding.bindLocation.glArraySize, GL_FALSE, (float*)boundData);
			} break;
			case UniformType::Float3x3: {
				glUniformMatrix3fv(binding.bindLocation.bindLocation, binding.bindLocation.glArraySize, GL_FALSE, (float*)boundData);
			} break;
			// Uniform blocks.
			case UniformType::ConstantBuffer: {
				sgeAssert(binding.bindLocation.glArraySize == 1);
				glcon->BindUniformBuffer(binding.bindLocation.bindLocation, ((BufferGL*)(binding.buffer))->GL_GetResource());
			} break;

			// Textures.
			case UniformType::Texture1D:
			case UniformType::Texture2D:
			case UniformType::TextureCube:
			case UniformType::Texture3D: {
				GLenum textureTarget = GL_NONE;
				switch (uniformType) {
#if !defined(__EMSCRIPTEN__)
					case UniformType::Texture1D:
						textureTarget = GL_TEXTURE_1D;
						break;
#endif
					case UniformType::Texture2D:
						textureTarget = GL_TEXTURE_2D;
						break;
					case UniformType::TextureCube:
						textureTarget = GL_TEXTURE_CUBE_MAP;
						break;
					case UniformType::Texture3D:
						textureTarget = GL_TEXTURE_3D;
						break;
				}

				sgeAssert(textureTarget != GL_NONE);

				// Caution:
				// Currently if the array size is 1, we assume that a single texture has been bound via
				// uniform.texture and not an array with uniform.textures with 1 element specified.
				if (binding.bindLocation.glArraySize == 1) {
					// Bind the texture.
					TextureGL* const textureGL = ((TextureGL*)(binding.texture));
					const GLint texture = textureGL ? ((TextureGL*)(binding.texture))->GL_GetResource() : GL_NONE;
					glcon->BindTextureEx(textureTarget, GL_TEXTURE0 + binding.bindLocation.glTextureUnit, texture);
					DumpAllGLErrors();

					glUniform1i(binding.bindLocation.bindLocation, binding.bindLocation.glTextureUnit);
					DumpAllGLErrors();
				} else {
					for (int t = 0; t < binding.bindLocation.glArraySize; ++t) {
						// Bind the texture.
						TextureGL* const boundTextureGL = static_cast<TextureGL*>(binding.textures[t]);
						const GLint texture = boundTextureGL ? boundTextureGL->GL_GetResource() : GL_NONE;
						const GLenum texSlotIdx = binding.bindLocation.bindLocation + t;
						glcon->BindTextureEx(textureTarget, GL_TEXTURE0 + texSlotIdx, texture);
						DumpAllGLErrors();

						glUniform1i(texSlotIdx, texSlotIdx);
						DumpAllGLErrors();
					}
				}

			} break;

			case UniformType::SamplerState: {
				// Nothing to do here... In OpenGL these are embedded in the texture itself.
			} break;

			// Unknown:
			default: {
				sgeAssert(false && "Unabled to bind an attribute with tis uniform type");
			}
		}
	}

	// Rasterizer state. If in GL_Flip_Y mode, change the front facing triangles,
	// because the Y-flip in the projection matrix is also flipping the y-coord.
	RasterDesc rasterDesc = RasterDesc();
	if (stateGroup->m_rasterState != nullptr) {
		rasterDesc = stateGroup->m_rasterState->getDesc();
	}

	glcon->ApplyRasterDesc(rasterDesc);

	if (rasterDesc.useScissor) {
		// [NOTE] THE CORRECT FRAME BUFFER MUST BE BOUND AT THIS POINT!!! (And i fucking forgot to write why...)
		// The actual scissors rect to be applied.
		// OpenGL uses the bottom-left rect origin!
		// In order to flip the Y coordinate we need the current frame buffer height.
		const GLint frameTargetHeight = frameTarget->getHeight();

		sgeAssert(pScissorsRect);

		// TODO:
		// TODO:
		// TODO:
		// TODO:
		// TODO:
		// TODO:
		// TODO: THERE SHOULD BE NOT INTERNAL FLIPPING OF THE UP AXIS !!!
		// I'm not sure how to handle the differences of the rendering APIs in therms of texcoord and frame buffer origin.
		// I've tried "flipping" the viewport and the scissors rect, it did not scale well (AGAIN I FORGOT TO WRITE WHY...)
		if (pScissorsRect != nullptr) {
			const bool flipY = ((FrameTargetGL*)frameTarget)->GL_IsWindowFrameBufferWrapper();

			// Remember, SGE API uses the top-left corner as an origin for scissor rects!
			const GLint x = pScissorsRect->x;
			const GLint y = frameTargetHeight - pScissorsRect->y - pScissorsRect->height; // TODO: No Y axis flipping.
			const GLsizei width = pScissorsRect->width;
			const GLsizei height = pScissorsRect->height;

			glcon->ApplyScissorsRect(x, y, width, height);
		} else {
			sgeAssert(false);
			// In order to make this a bit debuggable just make big scissors rect.
			const GLint frameTargetWidth = frameTarget->getWidth();
			glcon->ApplyScissorsRect(0, 0, frameTargetWidth, frameTargetHeight);
		}
	}

	// Blending
	if (stateGroup->m_blendState && stateGroup->m_blendState->isValid()) {
		glcon->ApplyBlendState(stateGroup->m_blendState->getDesc().blendDesc[0]);
	} else {
		glcon->ApplyBlendState(BlendDesc());
	}

	// Viewport.
	const Rect2s viewport = pViewport != nullptr ? *pViewport : frameTarget->getViewport();
	const GLViewport gl_viewport(viewport.x, viewport.y, viewport.width, viewport.height);
	glcon->setViewport(gl_viewport);

	// Execute the draw call.
	size_t numPrimitivesDrawn = 0;

	if (drawCall.m_drawExec.GetType() == DrawExecDesc::Type_Indexed) {
		GLenum glType;
		GLint glArity;
		GLboolean normalized;

		UniformType_ToGLUniformType(stateGroup->m_indexBufferFormat, glType, glArity, normalized);

		const int ibFmtSizeBytes = UniformType::GetSizeBytes(stateGroup->m_indexBufferFormat);

		// [CAUTION] Faked with "glVertexAttribPointer".
		// glDrawElementsBaseVertex(
		//	PrimitiveTopology_GetGLNative(drawCall.m_primTopology),
		//	drawCall.m_drawExec.IndexedCall().numIndices,
		//	glType,
		//	(void*)(drawCall.m_drawExec.IndexedCall().startIndex*ibFmtSizeBytes),
		//	drawCall.m_drawExec.IndexedCall().startVertex);

		glcon->DrawElements(PrimitiveTopology_GetGLNative(stateGroup->m_primTopology), drawCall.m_drawExec.IndexedCall().numIndices, glType,
		                    (GLvoid*)(std::ptrdiff_t(drawCall.m_drawExec.IndexedCall().startIndex * ibFmtSizeBytes)),
		                    drawCall.m_drawExec.IndexedCall().numInstances);

		numPrimitivesDrawn +=
		    PrimitiveTopology::GetNumPrimitivesByPoints(stateGroup->m_primTopology, drawCall.m_drawExec.IndexedCall().numIndices) *
		    drawCall.m_drawExec.IndexedCall().numInstances;

	} else {
		glcon->DrawArrays(PrimitiveTopology_GetGLNative(stateGroup->m_primTopology), drawCall.m_drawExec.LinearCall().startVert,
		                  drawCall.m_drawExec.LinearCall().numVerts, drawCall.m_drawExec.LinearCall().numInstances);

		numPrimitivesDrawn += drawCall.m_drawExec.LinearCall().numInstances * drawCall.m_drawExec.LinearCall().numVerts;
	}

	getDeviceImpl()->m_frameStatistics.numDrawCalls += 1;
	getDeviceImpl()->m_frameStatistics.numPrimitiveDrawn += numPrimitivesDrawn;
}

#if !defined(__EMSCRIPTEN__)
void glDebugOutput(GLenum source,
                   GLenum type,
                   unsigned int id,
                   GLenum severity,
                   GLsizei UNUSED(length),
                   const char* message,
                   const void* UNUSED(userParam)) {
	// ignore non-significant error/warning codes
	// if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
		case GL_DEBUG_SOURCE_API:
			std::cout << "Source: API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			std::cout << "Source: Window System";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			std::cout << "Source: Shader Compiler";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			std::cout << "Source: Third Party";
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			std::cout << "Source: Application";
			break;
		case GL_DEBUG_SOURCE_OTHER:
			std::cout << "Source: Other";
			break;
	}
	std::cout << std::endl;

	switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			std::cout << "Type: Error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			std::cout << "Type: Deprecated Behaviour";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			std::cout << "Type: Undefined Behaviour";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			std::cout << "Type: Portability";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			std::cout << "Type: Performance";
			break;
		case GL_DEBUG_TYPE_MARKER:
			std::cout << "Type: Marker";
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			std::cout << "Type: Push Group";
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			std::cout << "Type: Pop Group";
			break;
		case GL_DEBUG_TYPE_OTHER:
			std::cout << "Type: Other";
			break;
	}
	std::cout << std::endl;

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			std::cout << "Severity: high";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			std::cout << "Severity: medium";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			std::cout << "Severity: low";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			std::cout << "Severity: notification";
			break;
	}
	std::cout << std::endl;
	std::cout << std::endl;
}
#endif

SGEDevice* SGEDevice::create(const MainFrameTargetDesc& frameTargetDesc) {
	SGEDeviceImpl* s = new SGEDeviceImpl();
	s->Create(frameTargetDesc);

#if !defined(__EMSCRIPTEN__)
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(glDebugOutput, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
#endif
	return s;
}

RAIResource* SGEDeviceImpl::requestResource(const ResourceType::Enum resourceType) {
	RAIResource* result = nullptr;

	if (resourceType == ResourceType::Buffer)
		result = new BufferGL;
	if (resourceType == ResourceType::Texture)
		result = new TextureGL;
	if (resourceType == ResourceType::Sampler)
		result = new SamplerStateGL;
	if (resourceType == ResourceType::FrameTarget)
		result = new FrameTargetGL;
	if (resourceType == ResourceType::Shader)
		result = new ShaderGL;
	if (resourceType == ResourceType::ShadingProgram)
		result = new ShadingProgramGL;
	if (resourceType == ResourceType::Query)
		result = new QueryGL;
	if (resourceType == ResourceType::RasterizerState)
		result = new RasterizerStateGL;
	if (resourceType == ResourceType::DepthStencilState)
		result = new DepthStencilStateGL;
	if (resourceType == ResourceType::BlendState)
		result = new BlendStateGL;
	if (resourceType == ResourceType::VertexMapper)
		result = new VertexMapperGL;

	if (!result) {
		sgeAssert(false && "Unknown resource type");
		return nullptr;
	}

	result->setDeviceInternal(this);

	return result;
}


RasterizerState* SGEDeviceImpl::requestRasterizerState(const RasterDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(rasterizerStateCache.begin(), rasterizerStateCache.end(),
	                        [&desc](const RasterizerState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(rasterizerStateCache)) {
		return *itr;
	}


	// Create the new resource;
	RasterizerState* const state = (RasterizerState*)requestResource(ResourceType::RasterizerState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	rasterizerStateCache.push_back(state);

	return state;
}

DepthStencilState* SGEDeviceImpl::requestDepthStencilState(const DepthStencilDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(depthStencilStateCache.begin(), depthStencilStateCache.end(),
	                        [&desc](const DepthStencilState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(depthStencilStateCache)) {
		return *itr;
	}

	// Create the new resource;
	DepthStencilState* const state = (DepthStencilState*)requestResource(ResourceType::DepthStencilState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	depthStencilStateCache.push_back(state);

	return state;
}

BlendState* SGEDeviceImpl::requestBlendState(const BlendStateDesc& desc) {
	// Search if the resource exists.
	auto itr = std::find_if(blendStateCache.begin(), blendStateCache.end(),
	                        [&desc](const BlendState* state) -> bool { return state->getDesc() == desc; });

	if (itr != std::end(blendStateCache)) {
		return *itr;
	}

	// Create the new resource;
	BlendState* const state = (BlendState*)requestResource(ResourceType::BlendState);
	state->create(desc);
	sgeAssert(state->isValid());

	// Add the 1 ref to the resource (this cointainer holds it).
	state->addRef();
	blendStateCache.push_back(state);

	return state;
}

VertexDeclIndex SGEDeviceImpl::getVertexDeclIndex(const VertexDecl* const declElems, const int declElemsCount) {
	const std::vector<VertexDecl> decl = VertexDecl::NormalizeDecl(declElems, declElemsCount);

	VertexDeclIndex& idx = m_vertexDeclIndexMap[decl];
	static_assert(VertexDeclIndex_Null == 0, "");
	if (idx == VertexDeclIndex_Null) {
		idx = static_cast<VertexDeclIndex>(m_vertexDeclIndexMap.size());
	}

	return idx;
}

const std::vector<VertexDecl>& SGEDeviceImpl::getVertexDeclFromIndex(const VertexDeclIndex index) const {
	for (const auto& e : m_vertexDeclIndexMap) {
		if (e.second == index) {
			return e.first;
		}
	}
	static std::vector<VertexDecl> empty = std::vector<VertexDecl>();
	return empty;
}

} // namespace sge
