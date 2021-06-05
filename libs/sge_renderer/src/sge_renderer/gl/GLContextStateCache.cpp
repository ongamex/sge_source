#include "GLContextStateCache.h"
#include "GraphicsCommon_gl.h"

namespace sge {

//------------------------------------------------
// state accum helper function
// returns true if
//------------------------------------------------
namespace {
	template <typename T>
	bool UPDATE_ON_DIFF(T& Variable, const T& Value) {
		if (Variable == Value)
			return false;
		Variable = Value;
		return true;
	}
} // namespace

////////////////////////////////////////////////////////////////////
// GLContextStateCache
////////////////////////////////////////////////////////////////////
void* GLContextStateCache::MapBuffer(const GLenum target, const GLenum access) {
#if !defined(__EMSCRIPTEN__)
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(target);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(target);

	if (m_boundBuffers[freq].buffer == 0) {
		sgeAssert(false && "Trying to call glMapBuffer on slot with no bound buffer!");
		return nullptr;
	}

	m_boundBuffers[freq].isMapped = true;
	void* result = glMapBuffer(target, access);
	DumpAllGLErrors();
	return result;
#endif
}

void GLContextStateCache::UnmapBuffer(const GLenum target) {
#if !defined(__EMSCRIPTEN__)
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(target);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(target);

	if (m_boundBuffers[freq].buffer == 0) {
		sgeAssert(false && "Trying to call glMapBuffer on slot with no bound buffer!");
	}

	sgeAssert(m_boundBuffers[freq].isMapped == true);

	m_boundBuffers[freq].isMapped = false;
	glUnmapBuffer(target);
#endif
}

// void* GLContextStateCache::MapNamedBuffer(const GLuint buffer, const GLenum access)
//{
//	return glMapNamedBufferEXT(buffer, access);
//}
//
// void GLContextStateCache::UnmapNamedBuffer(const GLuint buffer)
//{
//	glUnmapNamedBufferEXT(buffer);
//}

//---------------------------------------------------------------------
void GLContextStateCache::BindBuffer(const GLenum bufferTarget, const GLuint buffer) {
	// add some debug error checking because
	// not all buffer targets are supported
	IsBufferTargetSupported(bufferTarget);

	//
	const BUFFER_FREQUENCY freq = GetBufferTargetByFrequency(bufferTarget);

#if SGE_GL_CONTEXT_STRICT
	if (m_boundBuffers[freq].isMapped) {
		SGE_DEBUG_ERR("SGE GLContext API PROHIBITS Buffer Binding when currently bound buffer on that slot is mapped!");
	}
#endif

	if (UPDATE_ON_DIFF(m_boundBuffers[freq].buffer, buffer)) {
		glBindBuffer(bufferTarget, buffer);
		DumpAllGLErrors();
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::SetVertexAttribSlotState(const bool bEnabled,
                                                   const GLuint index,
                                                   const GLuint buffer,
                                                   const GLuint size,
                                                   const GLenum type,
                                                   const GLboolean normalized,
                                                   const GLuint stride,
                                                   const GLuint byteOffset) {
	VertexAttribSlotDesc& currentState = m_vertAttribPointers[index];

	// If currently the slot is enabled just disable it and bypass the call to
	// glVertexAttribPointer.
	// Calling glDisableVertexAttribArray("index") will invalidate the previous glVertexAttribPointer on "index"-th slot.
	bool justEnabled = false;

	if (bEnabled != currentState.isEnabled) {
		currentState.isEnabled = bEnabled;
		if (currentState.isEnabled) {
			justEnabled = true;
			glEnableVertexAttribArray(index);
		} else {
			glDisableVertexAttribArray(index);
		}

		DumpAllGLErrors();
	}

	if (currentState.isEnabled) {
		const bool stateDiff = (currentState.buffer != buffer) || (currentState.size != size) || (currentState.type != type) ||
		                       (currentState.normalized != normalized) || (currentState.stride != stride) ||
		                       (currentState.byteOffset != byteOffset);

		BindBuffer(GL_ARRAY_BUFFER, buffer);
		DumpAllGLErrors();

		if (stateDiff || justEnabled) {
			currentState.buffer = buffer;
			currentState.size = size;
			currentState.type = type;
			currentState.normalized = normalized;
			currentState.stride = stride;
			currentState.byteOffset = byteOffset;

			// Caution:
			// Integer vertex attributes needs to be called
			// with glVertexAttribIPointer and not with
			// with glVertexAttribPointer
			// Even if we specify GL_INT as a type. glGetError will not report any errors,
			// but in shader we will not get the integers we've specified.
			if (currentState.type == GL_INT || currentState.type == GL_UNSIGNED_INT) {
				glVertexAttribIPointer(index, currentState.size, currentState.type, currentState.stride,
				                       (GLvoid*)(std::ptrdiff_t(currentState.byteOffset)));
			} else {
				glVertexAttribPointer(index, currentState.size, currentState.type, currentState.normalized, currentState.stride,
				                      (GLvoid*)(std::ptrdiff_t(currentState.byteOffset)));
			}
			DumpAllGLErrors();
		}
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::UseProgram(const GLuint program) {
	if (UPDATE_ON_DIFF(m_program, program)) {
		glUseProgram(program);
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::BindUniformBuffer(const GLuint index, const GLuint buffer) {
	if (UPDATE_ON_DIFF(m_uniformBuffers[index], buffer)) {
		glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::SetActiveTexture(const GLenum activeSlot) {
	sgeAssert(activeSlot >= GL_TEXTURE0 && activeSlot <= GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);

	if (m_activeTexture != activeSlot) {
		m_activeTexture = activeSlot;
		glActiveTexture(activeSlot);
		DumpAllGLErrors();
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::BindTexture(const GLenum texTarget, const GLuint texture) {
	BoundTexture* pSlotData = nullptr;

	for (int t = 0; t < m_textures.size(); ++t) {
		auto& tex = m_textures[t];

		if (tex.activeSlot == m_activeTexture && tex.type == texTarget) {
			if (tex.resource == texture)
				return;
			pSlotData = &tex;
		}
	}

	if (pSlotData) // the binding location(slot, texTarget) is found but the resource is different
	{
		pSlotData->resource = texture;
		glBindTexture(texTarget, texture);
	} else {
		// the binding location isn't found
		BoundTexture boundTex;
		boundTex.activeSlot = m_activeTexture;
		boundTex.type = texTarget;
		boundTex.resource = texture;

		// add the bound slot
		const bool success = m_textures.push_back(boundTex);

		//[nODE]that assert mean thet the m_textures hasn't enough elemenets
		// increase the array size and recompile or something
		sgeAssert(success);

		glBindTexture(texTarget, texture);
		DumpAllGLErrors();
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::BindTextureEx(const GLenum texTarget, const GLenum activeSlot, const GLuint texture) {
	SetActiveTexture(activeSlot);
	BindTexture(texTarget, texture);
	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::BindFBO(const GLuint fbo) {
	if (m_frameBuffer == fbo)
		return;
	m_frameBuffer = fbo;

	// GL_FRAMEBUFFER is the only possible argument.... currently!
	// https://www.khronos.org/opengles/sdk/docs/man/xhtml/glBindFramebuffer.xml
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::setViewport(const sge::GLViewport& vp) {
	if (UPDATE_ON_DIFF(m_viewport.second, vp) || !m_viewport.first) {
		glViewport(vp.x, vp.y, vp.width, vp.height);
		m_viewport.first = true;
		DumpAllGLErrors();
	}
}

//---------------------------------------------------------------------
void GLContextStateCache::ApplyRasterDesc(const RasterDesc& desc) {
	// Backface culling.
	if (UPDATE_ON_DIFF(m_rasterDesc.cullMode, desc.cullMode)) {
		switch (desc.cullMode) {
			case CullMode::Back:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;

			case CullMode::Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;

			case CullMode::None:
				glDisable(GL_CULL_FACE);
				break;

			default:
				// Unknown cull mode
				sgeAssert(false);
		}
	}

	if (UPDATE_ON_DIFF(m_rasterDesc.backFaceCCW, desc.backFaceCCW)) {
		if (m_rasterDesc.backFaceCCW)
			glFrontFace(GL_CW);
		else
			glFrontFace(GL_CCW);
	}

	DumpAllGLErrors();

	// Fillmode.
#if !defined(__EMSCRIPTEN__) // WebGL 2 does't support fill mode.
	if (UPDATE_ON_DIFF(m_rasterDesc.fillMode, desc.fillMode)) {
		switch (desc.fillMode) {
			case FillMode::Solid:
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				break;

			case FillMode::Wireframe:
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				break;

			default:
				// Unknown fill mode
				sgeAssert(false);
		}
	}
#endif

	// Scissors.
	if (UPDATE_ON_DIFF(m_rasterDesc.useScissor, desc.useScissor)) {
		if (desc.useScissor)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}

	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::ApplyScissorsRect(GLint x, GLint y, GLsizei width, GLsizei height) {
	const bool diff = m_scissorsRect.x != x || m_scissorsRect.y != y || m_scissorsRect.width != width || m_scissorsRect.height != height;

	if (diff == false) {
		return;
	}

	m_scissorsRect.x = x;
	m_scissorsRect.y = y;
	m_scissorsRect.width = width;
	m_scissorsRect.height = height;

	glScissor(x, y, width, height);
	DumpAllGLErrors();
}

void GLContextStateCache::DepthMask(const GLboolean enabled) {
	if (UPDATE_ON_DIFF(m_depthStencilDesc.depthWriteEnabled, enabled == GL_TRUE)) {
		if (enabled)
			glDepthMask(GL_TRUE);
		else
			glDepthMask(GL_FALSE);
		DumpAllGLErrors();
	}
}

void GLContextStateCache::ApplyDepthStencilDesc(const DepthStencilDesc& desc) {
	if (UPDATE_ON_DIFF(m_depthStencilDesc.depthTestEnabled, desc.depthTestEnabled)) {
		if (desc.depthTestEnabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
		DumpAllGLErrors();
	}

	DepthMask(desc.depthWriteEnabled ? GL_TRUE : GL_FALSE);

	if (UPDATE_ON_DIFF(m_depthStencilDesc.comparisonFunc, desc.comparisonFunc)) {
		glDepthFunc(DepthComparisonFunc_GetGLNative(desc.comparisonFunc));
		DumpAllGLErrors();
	}
}

void GLContextStateCache::ApplyBlendState(const BlendDesc& blendDesc) {
	if (UPDATE_ON_DIFF(m_blendDesc, blendDesc)) {
		if (m_blendDesc.enabled)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);
		//}

		// if(m_blendDesc != blendDesc)
		//{
		glBlendFuncSeparate(Blend_GetGLNative(m_blendDesc.srcBlend), Blend_GetGLNative(m_blendDesc.destBlend),
		                    Blend_GetGLNative(m_blendDesc.alphaSrcBlend), Blend_GetGLNative(m_blendDesc.alphaDestBlend));

		glBlendEquationSeparate(BlendOp_GetGLNative(m_blendDesc.blendOp), BlendOp_GetGLNative(m_blendDesc.alphaBlendOp));

		m_blendDesc = blendDesc;
	}

	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::DrawElements(const GLenum primTopology,
                                       const GLuint numIndices,
                                       const GLenum elemArrayBufferFormat,
                                       const GLvoid* indices,
                                       const GLsizei instanceCount) {
	if (instanceCount == 1)
		glDrawElements(primTopology, numIndices, elemArrayBufferFormat, indices);
	else
		glDrawElementsInstanced(primTopology, numIndices, elemArrayBufferFormat, indices, instanceCount);

	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::DrawArrays(const GLenum primTopology,
                                     const GLuint startVertex,
                                     const GLuint numVerts,
                                     const GLsizei instanceCount) {
	if (instanceCount == 1)
		glDrawArrays(primTopology, startVertex, numVerts);
	else
		glDrawArraysInstanced(primTopology, startVertex, numVerts, instanceCount);

	DumpAllGLErrors();
}

//---------------------------------------------------------------------
void GLContextStateCache::GenBuffers(const GLsizei numBuffers, GLuint* const buffers) {
	sgeAssert(buffers != nullptr && numBuffers > 0);
	glGenBuffers(numBuffers, buffers);
}

void GLContextStateCache::DeleteBuffers(const GLsizei numBuffers, GLuint* const buffers) {
	sgeAssert(buffers != nullptr && numBuffers > 0);

	// The tricky part. remove all mentions of any buffer from buffers
	// from the context state cache(aka. unbind everything).
	for (int iBuffer = 0; iBuffer < numBuffers; ++iBuffer) {
		const GLuint buffer = buffers[iBuffer];

		// Active state.
		for (BoundBufferState& boundBuffer : m_boundBuffers) {
			if (boundBuffer.buffer == buffer) {
				// Its illigal to unbind a mapped buffer.  Your logic is probably wrong.
				sgeAssert(boundBuffer.isMapped == false);
				boundBuffer.buffer = 0;
			}
		}

		// Input assembler
		for (int t = 0; t < m_vertAttribPointers.size(); ++t) {
			auto& vad = m_vertAttribPointers[t];
			if (vad.buffer == buffer) {
				SetVertexAttribSlotState(false, t, 0, 1, GL_FLOAT, GL_FALSE, 0, 0);
			}
		}

		// uniform buffers.
		for (GLuint& ubuffer : m_uniformBuffers) {
			if (ubuffer == buffer) {
				ubuffer = 0;
			}
		}
	}

	// And finally delete the buffers.
	glDeleteBuffers(numBuffers, buffers);
}

//---------------------------------------------------------------------
void GLContextStateCache::GenTextures(const GLsizei numTextures, GLuint* const textures) {
	sgeAssert(textures != nullptr && numTextures > 0);
	glGenTextures(numTextures, textures);
	DumpAllGLErrors();
}

void GLContextStateCache::DeleteTextures(const GLsizei numTextures, GLuint* const textures) {
	// "Unbind" the texture form the state cache.
	for (int iTexture = 0; iTexture < numTextures; ++iTexture) {
		const GLuint tex = textures[iTexture];

		for (int t = 0; t < m_textures.size(); ++t) {
			BoundTexture& boundTex = m_textures[t];
			if (boundTex.resource == tex) {
				SetActiveTexture(boundTex.activeSlot);
				BindTextureEx(boundTex.type, boundTex.activeSlot, 0);
			}
		}
	}

	glDeleteTextures(numTextures, textures);
}

//---------------------------------------------------------------------
void GLContextStateCache::GenFrameBuffers(const GLsizei n, GLuint* ids) {
	sgeAssert(n > 0 && ids != NULL);
	glGenFramebuffers(n, ids);
}

//---------------------------------------------------------------------
void GLContextStateCache::DeleteFrameBuffers(const GLsizei n, GLuint* ids) {
	for (int t = 0; t < n; ++t) {
		// HACK: if the currently bound fbo is deleted, I really don't know what happens to the OpenGL state.
		// In order to make the next fbo call successful we imagine that a fbo with index max(GLuint) is bound.
		if (ids[t] == m_frameBuffer) {
			m_frameBuffer = std::numeric_limits<GLuint>::max();
		}
	}

	sgeAssert(n > 0 && ids != NULL);
	glDeleteFramebuffers(n, ids);
}
//---------------------------------------------------------------------
void GLContextStateCache::DeleteProgram(GLuint program) {
	if (program == m_program) {
		m_program = 0;
	}

	glDeleteProgram(program);
}

//---------------------------------------------------------------------
GLContextStateCache::BUFFER_FREQUENCY GLContextStateCache::GetBufferTargetByFrequency(const GLenum bufferTarget) {
	switch (bufferTarget) {
		case GL_ARRAY_BUFFER:
			return BUFFER_FREQUENCY_ARRAY;
		case GL_ELEMENT_ARRAY_BUFFER:
			return BUFFER_FREQUENCY_ELEMENT_ARRAY;
		case GL_UNIFORM_BUFFER:
			return BUFFER_FREQUENCY_UNIFORM;
	}

	// unimplemented frequency type
	sgeAssert(false);

	// make the compiler happy
	return BUFFER_FREQUENCY_ARRAY;
}

bool GLContextStateCache::IsBufferTargetSupported(const GLenum bufferTarget) {
	switch (bufferTarget) {
		case GL_ARRAY_BUFFER:
		case GL_ELEMENT_ARRAY_BUFFER:
		case GL_UNIFORM_BUFFER:
			return true;
	}
#ifdef SGE_USE_DEBUG
	sgeAssert(false && "Unsupported buffer target encountered!");
#endif
	return false;
}

struct BoundBufferState {
	bool isMapped = false;
	GLuint buffer = 0;
};

} // namespace sge
