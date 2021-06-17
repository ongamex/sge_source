#include "renderer.h"
#include "HLSLTranslator.h"

namespace sge {

#if 0
template<typename T>
T* SGEDevice::requestResource()
{
	sgeAssert(false);
	return nullptr;
}
#endif

#define SGE_Impl_request_resource(T, RT)             \
	template <>                                      \
	T* SGEDevice::requestResource<T>() {             \
		return static_cast<T*>(requestResource(RT)); \
	}

SGE_Impl_request_resource(Buffer, ResourceType::Buffer);
SGE_Impl_request_resource(Texture, ResourceType::Texture);
SGE_Impl_request_resource(FrameTarget, ResourceType::FrameTarget);
SGE_Impl_request_resource(Shader, ResourceType::Shader);
SGE_Impl_request_resource(ShadingProgram, ResourceType::ShadingProgram);
SGE_Impl_request_resource(RasterizerState, ResourceType::RasterizerState);
SGE_Impl_request_resource(DepthStencilState, ResourceType::DepthStencilState);
SGE_Impl_request_resource(BlendState, ResourceType::BlendState);
SGE_Impl_request_resource(SamplerState, ResourceType::Sampler);
SGE_Impl_request_resource(Query, ResourceType::Query);

CreateShaderResult
    ShadingProgram::createFromCustomHLSL(const char* const pVSCode, const char* const pPSCode, std::set<std::string>* outIncludedFiles) {
	std::string compilationErrors;

	std::string vsTranslated;
	if (!translateHLSL(pVSCode, ShadingLanguage::ApiNative, ShaderType::VertexShader, vsTranslated, compilationErrors, outIncludedFiles)) {
		return CreateShaderResult(false, compilationErrors);
	}

	std::string psTranslated;
	if (!translateHLSL(pPSCode, ShadingLanguage::ApiNative, ShaderType::PixelShader, psTranslated, compilationErrors, outIncludedFiles)) {
		return CreateShaderResult(false, compilationErrors);
	}

	return createFromNativeCode(vsTranslated.c_str(), psTranslated.c_str());
}

} // namespace sge
