#include "sgeImportFBXFile.h"
#include "FBXSDKParser.h"
#include "IAssetRelocationPolicy.h"
#include "ModelParseSettings.h"
#include "sge_utils/utils/Path.h"
#include "sge_utils/utils/strings.h"
#include <type_traits>

using namespace sge;

#if WIN32
#ifdef SGE_MDLCONVLIB_BUILDING_DLL
#define SGE_MDLCONVLIB_API __declspec(dllexport)
#else
#define SGE_MDLCONVLIB_API __declspec(dllimport)
#endif
#else
#ifdef SGE_MDLCONVLIB_BUILDING_DLL
#define SGE_MDLCONVLIB_API __attribute__((visibility("default")))
#else
#define SGE_MDLCONVLIB_API
#endif
#endif

extern "C" {

SGE_MDLCONVLIB_API bool
    m_sgeImportFBXFile(Model::Model& result, const char* fbxFilename, std::vector<std::string>* pOutReferencedTextures) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&m_sgeImportFBXFile), sgeImportFBXFileFn>::value);

	static fbxsdk::FbxManager* const fbxMngr = fbxsdk::FbxManager::Create();
	FbxImporter* const fbxImporter = FbxImporter::Create(fbxMngr, ""); // todo: do we need a name here?
	const bool fbxImporterInitialized = fbxImporter->Initialize(fbxFilename, -1, fbxMngr->GetIOSettings());

	if (!fbxImporterInitialized) {
		return false;
	}

	FbxScene* const fbxScene = FbxScene::Create(fbxMngr, "fbxsdkImportedScene");
	if (!fbxImporter->Import(fbxScene)) {
		return false;
	}

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;

	FBXSDKParser fbxsdkParser;
	if (!fbxsdkParser.parse(&result, pOutReferencedTextures, fbxScene, nullptr, mps)) {
		return false;
	}

	return true;
}

SGE_MDLCONVLIB_API bool m_sgeImportFBXFileAsMultiple(std::vector<MultiModelImportResult>& result,
                                                     const char* fbxFilename,
                                                     std::vector<std::string>* pOutReferencedTextures) {
	// Ensure that the typedef we provide in the header matches the actual function type.
	static_assert(std::is_same<decltype(&m_sgeImportFBXFileAsMultiple), sgeImportFBXFileAsMultipleFn>::value);

	static fbxsdk::FbxManager* const fbxMngr = fbxsdk::FbxManager::Create();
	FbxImporter* const fbxImporter = FbxImporter::Create(fbxMngr, ""); // todo: do we need a name here?
	const bool fbxImporterInitialized = fbxImporter->Initialize(fbxFilename, -1, fbxMngr->GetIOSettings());

	if (!fbxImporterInitialized) {
		return false;
	}

	FbxScene* const fbxScene = FbxScene::Create(fbxMngr, "fbxsdkImportedScene");
	if (!fbxImporter->Import(fbxScene)) {
		return false;
	}

	ModelParseSettings mps;
	NoneAssetRelocationPolicy relocationPolicy = NoneAssetRelocationPolicy();
	mps.pRelocaionPolicy = &relocationPolicy;

	// Find all nodes that are immediate children of the root and import them sepretly.
	FbxNode* const fbxRoot = fbxScene->GetRootNode();

	result.clear();
	result.resize(fbxRoot->GetChildCount());

	for (int t = 0; t < fbxRoot->GetChildCount(); ++t) {
		FbxNode* const fbxChild = fbxRoot->GetChild(t);

		// Append the name of the node in the result.
		const std::string newExt = string_format("%s.mdl", fbxChild->GetName());
		const std::string newOutName = extractFileNameWithExt(replaceExtension(fbxFilename, newExt.c_str()).c_str());

		FBXSDKParser fbxsdkParser;
		Model::Model importedModel;

		if (fbxsdkParser.parse(&result[t].importedModel, pOutReferencedTextures, fbxScene, fbxChild, mps)) {
			result[t].propsedFilename = newOutName;
		} else {
			printf("Something went wrong when converting %s!", fbxFilename);
		}
	}

	return true;
}

} // extern "C"
