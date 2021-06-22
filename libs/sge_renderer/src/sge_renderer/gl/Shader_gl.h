#pragma once

#include "opengl_include.h"
#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct ShadingProgramGL;

//----------------------------------------------------------------------------
// VertexMapperGL
//----------------------------------------------------------------------------
struct VertexMapperGL : public RAIResource {
	struct GL_AttribLayout {
		// This is not really needed by OpenGL. But vertex buffer binding
		// in 'SGE API' tries to replicate the direct3d 11 behaviour.
		int bufferSlot;

		GLuint index;
		GLint byteOffset;
		UniformType::Enum type;
	};

	VertexMapperGL() { destroy(); }
	~VertexMapperGL() { destroy(); }

	bool create(const ShadingProgram* shadingProgram, const VertexDeclIndex vertexDeclIndex);

	virtual void destroy() final;
	virtual bool isValid() const final;
	virtual ResourceType::Enum getResourceType() const override { return ResourceType::VertexMapper; }

	int getVertexDeclIndex() const { return m_vertexDeclIndex; }

	const std::vector<GL_AttribLayout>& GL_GetVertexLayout() const { return m_glVertexLayout; }

  protected:
	VertexDeclIndex m_vertexDeclIndex;
	std::vector<GL_AttribLayout> m_glVertexLayout;
};

//----------------------------------------------------------------------------
// ShaderGL
//----------------------------------------------------------------------------
struct ShaderGL : public Shader {
	ShaderGL() {}
	~ShaderGL() { destroy(); }

	// Creates the shader using the native language for the API.
	CreateShaderResult createNative(const ShaderType::Enum type, const char* pCode, const char* const entryPoint);

	virtual void destroy() override;
	virtual bool isValid() const override;

	const ShaderType::Enum getShaderType() const final { return m_shaderType; }

	GLuint GL_GetShader() { return m_glShader; }

  private:
	ShaderType::Enum m_shaderType;
	std::string m_cachedCode;

	GLuint m_glShader = 0;
};

} // namespace sge
