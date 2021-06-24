#pragma once

#include "ShadingProgram_d3d11.h"

namespace sge {

struct ShadingProgramD3D11 : public ShadingProgram {
	ShadingProgramD3D11() {}
	~ShadingProgramD3D11() { destroy(); }

	bool create(Shader* vertShdr, Shader* pixelShdr) final;
	CreateShaderResult createFromNativeCode(const char* const pVSCode, const char* const pPSCode) override;

	void destroy() override;
	bool isValid() const override;

	// Resource access.
	Shader* getVertexShader() const final { return m_vertShdr.GetPtr(); }
	Shader* getPixelShader() const final { return m_pixShadr.GetPtr(); }


	const ShadingProgramRefl& getReflection() const final { return m_reflection; }

	ID3D11VertexShader* D3D11_GetVertexShader() const;
	ID3D11PixelShader* D3D11_GetPixelShader() const;

	// Returns the slot of the $Globals cbuffer.
	// returns -1 if not present.
	int getGlobalCBufferSlot(ShaderType::Enum shaderType) const { return m_globalCBufferSlot[shaderType]; }

  private:
	GpuHandle<Shader> m_vertShdr;
	GpuHandle<Shader> m_pixShadr;

	// Reflections.
	ShadingProgramRefl m_reflection;
	int m_globalCBufferSlot[ShaderType::NumElems];
};

} // namespace sge
