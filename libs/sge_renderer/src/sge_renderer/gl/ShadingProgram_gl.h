#pragma once

#include "opengl_include.h"

#include "sge_renderer/renderer/renderer.h"

namespace sge {

struct VertexMapperGL;

struct ShadingProgramGL : public ShadingProgram {
	ShadingProgramGL() {}
	~ShadingProgramGL() { destroy(); }

	bool create(Shader* vertShdr, Shader* pixelShdr) final;
	CreateShaderResult createFromNativeCode(const char* const pVSCode, const char* const pPSCode) final;
	//bool createFromNativeCode(const char* const pVSCode, const char* const pPSCode, std::set<std::string>* outIncludedFiles = nullptr) final;

	void destroy() override;
	bool isValid() const override;

	// Resource access.
	Shader* getVertexShader() const final { return m_vertShdr.GetPtr(); }
	Shader* getPixelShader() const final { return m_pixShadr.GetPtr(); }

	// Creates or returns already existing VertexMapperGL
	VertexMapperGL* GetVertexMapper(const VertexDeclIndex vertexDeclIndex);

	GLuint GL_GetProgram() const { return m_glProgram; }

	const ShadingProgramRefl& getReflection() const final { return m_reflection; }

  private:
	GpuHandle<Shader> m_vertShdr;
	GpuHandle<Shader> m_pixShadr;

	std::vector<GpuHandle<VertexMapperGL>> m_vertMappers; // Vertex shader input layouts.
	GLuint m_glProgram = 0;
	ShadingProgramRefl m_reflection;
};

} // namespace sge
