#include <cstring>
#include <mutex>
#include <unordered_map>
// HLSLParser +  MCPP
#include "HLSLTranslator.h"
#include "sge_mcpp.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/strings.h"
#include <Engine.h>
#include <GLSLGenerator.h>
#include <HLSLGenerator.h>
#include <HLSLParser.h>

constexpr const char* const SGE_FAKE_SHADER_SRC_FILE = "<NO-FILE>";

namespace preprocessor {
struct Args {
	const char* input; // the inut code
	int strlen_input;
	int ptr;             // an index pointer pointing in input (may point at input.size() because output_fn must return '\0'
	std::string& output; // the result code
	std::string& errors; // [TODO]
};

// This function must return '\0'!
char* input_fn(char* buffer, int size, void* userdata) {
	Args* args = (Args*)userdata;

	int sz = args->strlen_input + 1; // in order to return '\0'
	size -= 1;                       // the manually added null terminator
	size = ((sz - args->ptr) > size) ? size : (sz - args->ptr);

	if (size != 0) {
		memcpy(buffer, args->input + args->ptr, size);
		args->ptr += size;
		buffer[size] = 0;
	}

	return size ? buffer : 0;
}

void output_fn(int c, void* userdata) {
	Args* const args = static_cast<Args*>(userdata);
	args->output += char(c);
}

void error_fn(void* UNUSED(userdata), char* UNUSED(format), va_list UNUSED(arg)) {
	// This is not really called only when error accure
	// SGE_DEBUG_WAR(format, arg);
}

std::string preprocess(const char* code, const char* const* macros, const int numMacros, std::set<std::string>* outIncludedFiles) {
	// Not that anyone is going to do it but:
	// mcpp uses some global state, so it isn't thread safe!
	static std::mutex lock;
	const std::lock_guard<std::mutex> lockGuard(lock);

	std::vector<std::string> args;
	std::vector<const char*> argsCFormat;

	args.push_back("mcpp");
	args.push_back(SGE_FAKE_SHADER_SRC_FILE);

	std::string compileOptionsForMCpp;
	for (int t = 0; t < numMacros; ++t) {
		args.push_back(string_format("-D%s", macros[t]));
	}

	for (const std::string& arg : args) {
		argsCFormat.push_back(arg.c_str());
	}

	std::unordered_map<std::string, std::vector<char>> includeFiles;

	struct UserData {
		const char* code = nullptr;
		std::unordered_map<std::string, std::vector<char>>& includeFiles;
	};

	UserData udata = {code, includeFiles};

	const auto loadFileFn = [](const char* filename, const char** out_contents, size_t* out_contents_size, void* user_data) -> int {
		UserData& udata = *static_cast<UserData*>(user_data);

		if (strcmp(filename, SGE_FAKE_SHADER_SRC_FILE) == 0) {
			if (out_contents) {
				*out_contents = udata.code;
			}

			if (out_contents_size) {
				*out_contents_size = strlen(udata.code);
			}
			return 1;
		}
		const std::string fileToLoad = std::string("core_shaders/") + filename;
		auto itrExisting = udata.includeFiles.find(fileToLoad.c_str());
		std::vector<char>* pFileData = nullptr;
		if (itrExisting != udata.includeFiles.end()) {
			pFileData = &itrExisting->second;
		} else {
			std::vector<char> data;
			if (sge::FileReadStream::readFile(fileToLoad.c_str(), data) == false) {
				return 0;
			}
			data.emplace_back('\0');

			udata.includeFiles[fileToLoad] = std::move(data);
			pFileData = &udata.includeFiles[fileToLoad];
		}

		if (pFileData) {
			if (out_contents) {
				*out_contents = pFileData->data();
			}

			if (out_contents_size) {
				*out_contents_size = pFileData->size();
			}
			return 1;
		}

		return 0;
	};

	// These two are redirected to some global mcpp values.
	char* compilationResult = nullptr;
	char* colpilationErrors = nullptr;

	int const res =
	    sge_mcpp_preprocess(&compilationResult, &colpilationErrors, argsCFormat.data(), int(argsCFormat.size()), loadFileFn, &udata);
	std::string result = compilationResult ? compilationResult : "";

	if (outIncludedFiles != nullptr) {
		for (auto& itr : includeFiles) {
			outIncludedFiles->insert(itr.first);
		}
	}

	return result;
}
} // namespace preprocessor

namespace sge {

bool translateHLSL(const char* const pCode,
                   const ShadingLanguage::Enum shadingLanguage,
                   const ShaderType::Enum shaderType,
                   std::string& result,
                   std::string& compilationErrors,
                   std::set<std::string>* outIncludedFiles) {
	M4::g_hlslParserErrors.clear();

	const char* const mOpenGL = "OpenGL";

	std::vector<const char*> macros;

	if (shadingLanguage == ShadingLanguage::GLSL) {
		macros.push_back(mOpenGL);
	}

	if (shaderType == ShaderType::PixelShader) {
		macros.push_back("SGE_PIXEL_SHADER");
	}


	const std::string processsedCode = preprocessor::preprocess(pCode, macros.data(), int(macros.size()), outIncludedFiles);

	M4::Allocator m4Alloc;
	M4::HLSLParser hlslParser(&m4Alloc, SGE_FAKE_SHADER_SRC_FILE, processsedCode.c_str(), processsedCode.size());
	M4::HLSLTree tree(&m4Alloc);

	if (hlslParser.Parse(&tree) == false) {
		compilationErrors = M4::g_hlslParserErrors;
		return false;
	}

	if (shadingLanguage == ShadingLanguage::GLSL) {
		auto target =
		    shaderType == ShaderType::VertexShader ? M4::GLSLGenerator::Target_VertexShader : M4::GLSLGenerator::Target_FragmentShader;
		auto mainFnName = shaderType == ShaderType::VertexShader ? "vsMain" : "psMain";

		M4::GLSLGenerator gen;
#if defined(__EMSCRIPTEN__)
		if (gen.Generate(&tree, target, M4::GLSLGenerator::Version_300_ES, mainFnName) == false) {
#else
		if (gen.Generate(&tree, target, M4::GLSLGenerator::Version_150, mainFnName) == false) {
#endif
			compilationErrors = M4::g_hlslParserErrors;
			return false;
		}
		result = gen.GetResult();
	} else if (shadingLanguage == ShadingLanguage::HLSL) {
		auto target =
		    shaderType == ShaderType::VertexShader ? M4::HLSLGenerator::Target_VertexShader : M4::HLSLGenerator::Target_PixelShader;
		auto mainFnName = shaderType == ShaderType::VertexShader ? "vsMain" : "psMain";

		M4::HLSLGenerator gen;
		if (gen.Generate(&tree, target, mainFnName, false) == false) {
			compilationErrors = M4::g_hlslParserErrors;
			return false;
		}
		result = gen.GetResult();
	}

	return true;
}

} // namespace sge
