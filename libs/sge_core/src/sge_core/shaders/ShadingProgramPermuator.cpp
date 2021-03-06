#include "ShadingProgramPermuator.h"
#include "sge_core/ICore.h"
#include "sge_utils/utils/FileStream.h"

namespace sge {

//-----------------------------------------------------------------------------
// ShadingProgramPermuator
//-----------------------------------------------------------------------------
bool ShadingProgramPermuator::createFromFile(SGEDevice* sgedev,
                                             const char* const filename,
                                             const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                             const std::vector<Unform>& uniformsToCacheInLUT,
                                             std::set<std::string>* outIncludedFiles) {
	std::vector<char> fileContents;
	if (FileReadStream::readFile(filename, fileContents)) {
		if (outIncludedFiles != nullptr) {
			outIncludedFiles->insert(filename);
		}

		fileContents.push_back('\0');
		return create(sgedev, fileContents.data(), compileTimeOptions, uniformsToCacheInLUT, outIncludedFiles);
	}

	return false;
}

bool ShadingProgramPermuator::create(SGEDevice* sgedev,
                                     const char* const shaderCode,
                                     const std::vector<OptionPermuataor::OptionDesc>& compileTimeOptions,
                                     const std::vector<Unform>& uniformsToCacheInLUT,
                                     std::set<std::string>* outIncludedFiles) {
	*this = ShadingProgramPermuator();

	// Verify the safety indices.
	for (int t = 0; t < uniformsToCacheInLUT.size(); ++t) {
		if (uniformsToCacheInLUT[t].safetyIndex != t) {
			sgeAssert(false);
			return false;
		}
	}

	compileTimeOptionsPermutator.build(compileTimeOptions);

	const int numPerm = int(compileTimeOptionsPermutator.getAllPermunations().size());
	if (numPerm == 0) {
		return false;
	}

	// Compile the shaders and cache the unforms LUT.
	std::string shaderCodeFull;
	perPermutationShadingProg.resize(numPerm);
	for (int iPerm = 0; iPerm < compileTimeOptionsPermutator.getAllPermunations().size(); ++iPerm) {
		const auto& perm = compileTimeOptionsPermutator.getAllPermunations()[iPerm];

		std::string macrosToPreapend;
		for (int iOpt = 0; iOpt < compileTimeOptions.size(); ++iOpt) {
			const OptionPermuataor::OptionDesc& desc = compileTimeOptions[iOpt];
			macrosToPreapend += "#define " + desc.name + " " + desc.possibleValues[perm[iOpt]] + "\n";
		}

		perPermutationShadingProg[iPerm].shadingProgram = sgedev->requestResource<ShadingProgram>();

		shaderCodeFull.clear();
		shaderCodeFull += macrosToPreapend;
		shaderCodeFull += shaderCode;

		const CreateShaderResult programCreateResult = perPermutationShadingProg[iPerm].shadingProgram->createFromCustomHLSL(
		    shaderCodeFull.c_str(), shaderCodeFull.c_str(), outIncludedFiles);

		if (programCreateResult.succeeded == false) {
			SGE_DEBUG_ERR("Shader Compilation Failed:\n%s", programCreateResult.errors.c_str());
			*this = ShadingProgramPermuator();
			return false;
		}

		const ShadingProgramRefl& refl = perPermutationShadingProg[iPerm].shadingProgram->getReflection();
		perPermutationShadingProg[iPerm].uniformLUT.reserve(uniformsToCacheInLUT.size());
		for (int t = 0; t < uniformsToCacheInLUT.size(); ++t) {
			BindLocation bindLoc = refl.findUniform(uniformsToCacheInLUT[t].uniformName, uniformsToCacheInLUT[t].shaderStage);
			perPermutationShadingProg[iPerm].uniformLUT.push_back(bindLoc);
		}
	}

	return true;
}
} // namespace sge
