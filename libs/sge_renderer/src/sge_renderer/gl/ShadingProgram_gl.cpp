#include <algorithm>
#include <stdio.h>

#include "GraphicsInterface_gl.h"
#include "Shader_gl.h"
#include "ShadingProgram_gl.h"

namespace sge {

bool ShadingProgramGL::create(Shader* vertShdr, Shader* pixelShdr) {
	if (!vertShdr || !pixelShdr) {
		return false;
	}

	// Cleanup the current state.
	destroy();

	m_vertShdr = vertShdr;
	m_pixShadr = pixelShdr;

	// Create the program and attach the shaders.
	m_glProgram = glCreateProgram();

	glAttachShader(m_glProgram, ((ShaderGL*)vertShdr)->GL_GetShader()); // attach vertex shader
	glAttachShader(m_glProgram, ((ShaderGL*)pixelShdr)->GL_GetShader());

	// Link the program
	glLinkProgram(m_glProgram);

	// Check for errors
	GLint linkingStatus;
	glGetProgramiv(m_glProgram, GL_LINK_STATUS, &linkingStatus);

	if (linkingStatus == GL_FALSE) {
		GLint logLenght;
		glGetProgramiv(m_glProgram, GL_INFO_LOG_LENGTH, &logLenght);

		if (logLenght) {
			std::vector<GLchar> infoLog(logLenght);
			glGetProgramInfoLog(m_glProgram, logLenght, &logLenght, &infoLog[0]);
			sgeAssert(false);
			// Display the error message.
			// SGE_DEBUG_ERR((char*)infoLog.data());
		}

		// clean up
		destroy();
		return false;
	}

	bool const reflectonSucceeded = m_reflection.create(this);
	return reflectonSucceeded;
}

bool ShadingProgramGL::create(const char* const pVSCode, const char* const pPSCode, std::set<std::string>* outIncludedFiles = nullptr) {
	GpuHandle<ShaderGL> vs = getDevice()->requestResource<Shader>();
	bool r = vs->create(ShaderType::VertexShader, pVSCode, nullptr);
	if (r == false) {
		return r;
	}

	GpuHandle<ShaderGL> ps = getDevice()->requestResource<Shader>();
	r = ps->create(ShaderType::PixelShader, pPSCode, nullptr);

	if (r == false) {
		return r;
	}

	return create(vs, ps);
}

void ShadingProgramGL::destroy() {
	// drop shader usage
	m_vertShdr.Release();
	m_pixShadr.Release();

	if (m_glProgram != 0) {
		GLContextStateCache* glcon = getDevice<SGEDeviceImpl>()->GL_GetContextStateCache();
		glcon->DeleteProgram(m_glProgram);
		m_glProgram = 0;
	}
}

bool ShadingProgramGL::isValid() const {
	return m_glProgram != 0;
}

VertexMapperGL* ShadingProgramGL::GetVertexMapper(const VertexDeclIndex vertexDeclIndex) {
	// if the requester mapper already exists return it!
	for (size_t t = 0; t < m_vertMappers.size(); ++t) {
		const bool isMatching = vertexDeclIndex == m_vertMappers[t]->getVertexDeclIndex();

		// suitable vertex mapping exists!
		if (isMatching)
			return m_vertMappers[t];
	}

	// create a new mapper
	GpuHandle<VertexMapperGL> vertMapper;

	vertMapper = getDevice()->requestResource(ResourceType::VertexMapper);
	if (!vertMapper->create(this, vertexDeclIndex)) {
		sgeAssert(false && "Failed Creating InputLayout Resource!\n");
		return nullptr;
	}

	m_vertMappers.push_back(vertMapper);
	return m_vertMappers.back();
}

} // namespace sge
