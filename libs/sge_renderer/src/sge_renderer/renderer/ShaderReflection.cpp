#include "ShaderReflection.h"

namespace sge {

BindLocation ShadingProgramRefl::findUniform(const char* const uniformName, ShaderType::Enum shaderType) const {
	{
		BindLocation bl;
		bl = numericUnforms.findUniform(uniformName, shaderType);
		if (bl.isNull() == false)
			return bl;
	}

	{
		BindLocation bl;
		bl = cbuffers.findUniform(uniformName, shaderType);
		if (bl.isNull() == false)
			return bl;
	}

	{
		BindLocation bl;
		bl = textures.findUniform(uniformName, shaderType);
		if (bl.isNull() == false)
			return bl;
	}

	{
		BindLocation bl;
		bl = samplers.findUniform(uniformName, shaderType);
		if (bl.isNull() == false)
			return bl;
	}

	return BindLocation();
}

} // namespace sge