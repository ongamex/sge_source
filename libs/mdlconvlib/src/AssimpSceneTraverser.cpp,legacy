
#include "SceneTraverser.h"

#include <sstream>

#include "sge/utils/FileStream.h"
#include "sge/model/Parameter.h"
#include "sge/math/mat4.h"
#include "sge/math/Box.h"
#include "sge/utils/Path.h"

#include "IAssetRelocationPolicy.h"

namespace
{
	bool fltcmp_radius(float a, float b, float epsilonRadius)
	{
		const float t = a - b;
		return t >= -epsilonRadius && t <= epsilonRadius;
	}
}

namespace sge {

//---------------------------------------------------------------------
//
//---------------------------------------------------------------------
void AssimpSceneParser::parse(
	Model* result, 
	const aiScene* scene, 
	const ModelParseSettings& settings,
	std::vector<std::string>* pReferencedTextures)
{
	sgeAssert(result != NULL && scene != NULL);

	model = result;
	impScene = scene;
	this->settings = settings;

	// Clear the old state if any.
	cachedWholeSceneMeshData = NULL;
	iaMeshToMesh.clear();
	iaMaterialToMaterial.clear();
	meshNamesInAssimpScene.clear();
	bonesToResolve.clear();

	// Prase the scene.
	// Note the order of those function calls matters.
	printf("Parsing materials...\n");
	ParseMaterials(pReferencedTextures);

	printf("Parsing meshes...\n");
	for(int t = 0; t < impScene->mNumMeshes; ++t) {
		ParseMesh(impScene->mMeshes[t]);
	}

	// Parse all nodes.
	printf("Parsing nodes...\n");
	model->m_rootNode = ParseNodesRecursive(scene->mRootNode);

	printf("Resolving bones pointers... \n");
	ResolveBonesNodePointer();

	// Parse the animations.
	if(settings.shouldExportAnimations) {
		printf("Parsing animations...\n");
		ParseAnimations();
	}

	printf("Scene Parser Done.\n");
}

//---------------------------------------------------------------------
void AssimpSceneParser::ParseMaterials(std::vector<std::string>* pReferencedTextures)
{
	for(int t = 0; t < impScene->mNumMaterials; ++t)
	{
		aiMaterial* const impMtl = impScene->mMaterials[t];

		// Allocate the material.
		model->m_materials.push_back(model->m_containerMaterial.new_element());
		Model::Material* material = model->m_materials.back();

		// Keep track of what materials we've imported.
		iaMaterialToMaterial[impMtl] = material;

		// [TODO] Implmeent a better way of naming materials.
		material->id = getNextId();
		material->name = string_format("mtl_%d", t);

		printf("\tGenerated material: %s\n", material->name.c_str());

		// Obtain the diffuse color if any.
		aiColor4D impDiffColor;
		if(aiReturn_SUCCESS == impMtl->Get(AI_MATKEY_COLOR_DIFFUSE, impDiffColor))
		{
			Parameter* const diffuseColor = material->paramBlock.FindParameter("diffuseColor", ParameterType::Float4);
			diffuseColor->SetStaticValue(&vec4f(impDiffColor.r, impDiffColor.g, impDiffColor.b, impDiffColor.a));
		}

		// Obtain the diffuse texture if any.
		const int numDiffuseTextures = impMtl->GetTextureCount(aiTextureType_DIFFUSE);
		if(numDiffuseTextures != 0)
		{
			aiString texFilePath;
			impMtl->GetTexture(aiTextureType_DIFFUSE, 0, &texFilePath);

			const std::string requestPath = settings.pRelocaionPolicy->whatWillBeTheAssetNameOf(settings.fileDirectroy, texFilePath.C_Str());
			Parameter* const texDiffuse = material->paramBlock.FindParameter("texDiffuse", ParameterType::String);
			texDiffuse->SetStaticValue(requestPath.c_str());

			if(pReferencedTextures) {
				pReferencedTextures->push_back(texFilePath.C_Str());
			}
		}
	}
}

//---------------------------------------------------------------------
void AssimpSceneParser::ParseMesh(aiMesh* impMesh)
{
	printf("\tParsing mesh'%s'\n", impMesh->mName.C_Str());

	unsigned stride = 0; // Single vertex size accumulator.
	std::vector<void*> channelsData;
	std::vector<int> impChannelByteStep; // ASSIMP stores UV coordinates as a 3D UVW coordinates, this is why that variables exists

	// Find the MeshData storage and allocate a new mesh.
	Model::MeshData* meshData = GetBestSuitableMeshData(impMesh);
	meshData->meshes.push_back(model->m_containerMesh.new_element());
	Model::Mesh* mesh = meshData->meshes.back();

	// Keep track of the described assimp meshes.
	iaMeshToMesh[impMesh] = mesh;

	// Load the skinning data.
	mesh->bones.resize(impMesh->mNumBones);
	for(int iBone = 0; iBone < impMesh->mNumBones; ++iBone)
	{
		const auto& impBone = *impMesh->mBones[iBone];
		Model::Bone& bone = mesh->bones[iBone];

		bone.node = NULL; // Has to be resolved later since the nodes aren't loaded yet.
		bonesToResolve[&bone] = impBone.mName.C_Str();

		bone.vertexIds.resize(impBone.mNumWeights);
		bone.weights.resize(impBone.mNumWeights);

		// Copy the vertex ids and infuence weights.
		for(int t = 0; t < impBone.mNumWeights; ++t) {
			bone.vertexIds[t] = impBone.mWeights[t].mVertexId;
			bone.weights[t] = impBone.mWeights[t].mWeight;
		}

		// Copy the offset matrix. Additionally transpose the matrix as ASSIMP is using row major matrices.
		static_assert(sizeof(bone.offsetMatrix) == sizeof(impBone.mOffsetMatrix), "");
		memcpy(&bone.offsetMatrix, &impBone.mOffsetMatrix, sizeof(bone.offsetMatrix));
		bone.offsetMatrix = bone.offsetMatrix.transposed();
	}

	// Compose a name for the mesh. ASSIMP may use one name for multiple meshes, and assimp may not give a name to the mesh.
	mesh->id = getNextId();
	mesh->name = (impMesh->mName.length>0) ? impMesh->mName.C_Str() : "ImporterAutoAssignedMeshName";
	

	// Compose the vertex buffer, vertex declaration and stride size.
	int positionDeclIndex = -1;
	if(impMesh->HasPositions())
	{
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_position";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		impChannelByteStep.push_back(12);
		stride += 12;
		mesh->vertexDecl.push_back(decl);
		channelsData.push_back(impMesh->mVertices);
		
		positionDeclIndex = mesh->vertexDecl.size() - 1;
	}

	if(impMesh->HasVertexColors(0))
	{
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_color";
		decl.format = sge::UniformType::Float4;
		decl.byteOffset = -1;

		impChannelByteStep.push_back(12);
		stride += 12;
		mesh->vertexDecl.push_back(decl);
		channelsData.push_back(impMesh->mColors[0]);
	}

	if(impMesh->HasNormals())
	{
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_normal";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		impChannelByteStep.push_back(12);
		stride += 12;
		mesh->vertexDecl.push_back(decl);
		channelsData.push_back(impMesh->mNormals);
	}
	
	if(impMesh->HasTextureCoords(0))
	{
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_uv";
		decl.format = sge::UniformType::Float2;
		decl.byteOffset = -1;

		impChannelByteStep.push_back(12); // Yes 12 and not 8 !!! ASSIMP stores this as vec3f but we need vec2f!
		stride += 8;

		mesh->vertexDecl.push_back(decl);
		channelsData.push_back(impMesh->mTextureCoords[0]);
	}

	// Postprocess the primitive topology and vertex declaration.
	mesh->primTopo = PrimitiveTopology::TriangleList;
	mesh->vertexDecl = sge::VertexDecl::NormalizeDecl(mesh->vertexDecl.data(), mesh->vertexDecl.size());

	const int strideSize = mesh->vertexDecl.back().byteOffset + sge::UniformType::GetSizeBytes(mesh->vertexDecl.back().format);	
	const int bufferSizeBytes = strideSize * impMesh->mNumVertices;
	unsigned const numVerts = impMesh->mNumVertices;

	// Extend the vertex and index buffers with the data for this mesh.
	std::vector<char>& vbChunkData = meshData->vertexBufferRaw;
	vbChunkData.resize(vbChunkData.size() + bufferSizeBytes);
	char* const vbData = vbChunkData.data() + vbChunkData.size() - bufferSizeBytes; // The start location of our data.

	// Cache up vertex buffer offset into the shader MeshData.
	mesh->vbByteOffset = vbChunkData.size() - bufferSizeBytes;

	// Fill the vertex buffers data. and compute the axis aligned bounding box.
	mesh->aabox.setEmpty();
	// For each vertex for each variable/element...
	for(unsigned vertIdx = 0; vertIdx < numVerts; ++vertIdx)
	for(unsigned declIdx = 0; declIdx < mesh->vertexDecl.size(); ++declIdx)
	{
		// Insert the vertex data into the buffers.
		const VertexDecl& vertexDecl = mesh->vertexDecl[declIdx];
		const size_t writeOffset = strideSize * vertIdx + vertexDecl.byteOffset; // The data offset location form the begining of our buffer.
		
		// Copy the data to the result buffer.
		char* destLoc = (char*)vbData + writeOffset;
		const char* readLoc = (char*)channelsData[declIdx] + impChannelByteStep[declIdx] * vertIdx;
		const size_t bytes = UniformType::GetSizeBytes(vertexDecl.format);

		memcpy(destLoc, readLoc, bytes);

		// Accumulate the position data into AABox.
		if(positionDeclIndex == declIdx) {
			mesh->aabox.expand(*(vec3f*)(readLoc));
		}
	}
	
	// Extend the index buffer and choose a format for the index buffer.
	const uint32 numIndices = impMesh->mNumFaces * 3;
	const bool useShortIb = (numIndices < 65000);
	const size_t ibSizeBytes = numIndices * ((useShortIb) ? sizeof(uint16) : sizeof(uint32));
	mesh->ibFmt = (useShortIb) ? UniformType::Ushort : UniformType::Uint;
	
	// Allocate additional memory in order to store the indices for this mesh.
	std::vector<char>& ibChunkData = meshData->indexBufferRaw;
	ibChunkData.resize(ibChunkData.size() + ibSizeBytes);

	char* const ibData = ibChunkData.data() + ibChunkData.size() - ibSizeBytes;
	mesh->ibByteOffset = ibChunkData.size() - ibSizeBytes;

	// The number of vertices/indices used by this mesh. 
	// [NOTE] Currently we only export INDEXED meshes.
	mesh->numElements = numIndices;

	// Used for checking if the vertex buffer is just 1,2,3,4,5,6....N
	bool isIbSequental = true; 
	int ibSeqCnt = 0; 
	
	// Fill the index buffer data.
	for(int t = 0; t < impMesh->mNumFaces; ++t)
	{
		const aiFace& face = impMesh->mFaces[t];
		
		// This function support only triangles so far.
		if(face.mNumIndices != 3) {
			printf("Cannot import mesh '%s'. The number of indices is not 3!\n", impMesh->mName.C_Str());
			sgeAssert(false);
			// [TODO] Do something....
		}

		isIbSequental &= 
			(face.mIndices[0] == ibSeqCnt) &&
			(face.mIndices[1] == face.mIndices[0] + 1) &&
			(face.mIndices[2] == face.mIndices[1] + 1);

		ibSeqCnt += 3; 

		if(mesh->ibFmt == UniformType::Uint)
		{
			uint32* const data = (uint32*)ibData;
			data[t*3] = face.mIndices[0];
			data[t*3 + 1] = face.mIndices[1];
			data[t*3 + 2] = face.mIndices[2];
		}
		else if(mesh->ibFmt == UniformType::Ushort)
		{
			uint16* const data = (uint16*)ibData;
			data[t*3] = face.mIndices[0];
			data[t*3 + 1] = face.mIndices[1];
			data[t*3 + 2] = face.mIndices[2];
		}
		else
		{
			// Unknown mesh index buffer data format!
			printf("FATAL ERROR: Uknown mesh index buffer data format for assimp mesh: '%s'!\n", impMesh->mName.C_Str());
			sgeAssert(false);
		}
	}

	// If the index buffer is sequental (1,2,3,4,5,6...N) the index buffer could be skipped.
	if(isIbSequental) {
		sgeAssert(numVerts == numIndices); // The sequental check should be wrong if this assert fails.

		printf("The index buffer is sequental - cutting of the indices...\n");
		mesh->ibFmt = UniformType::Unknown;
		mesh->ibByteOffset = 0;
		ibChunkData.resize(ibChunkData.size() - ibSizeBytes);
	}

	printf("\t\tnumVerts = %d\n", numVerts);
	if(!isIbSequental)
	{
		printf("\t\tnumIndices = %d\n", numIndices);
	}

	// Find the attached material(if any).
	if(impMesh->mMaterialIndex >= 0)
	{
		auto itr = iaMaterialToMaterial.find(impScene->mMaterials[impMesh->mMaterialIndex]);
		if(itr != std::end(iaMaterialToMaterial))
		{
			mesh->pMaterial = itr->second;
		}
	}
}

//---------------------------------------------------------------------
Model::Node* AssimpSceneParser::ParseNodesRecursive(const aiNode* const impNode)
{
	// Allocate a new node.
	Model::Node* const node = model->m_containerNode.new_element();
	model->m_nodes.push_back(node);

	// Set the name of the node.
	node->id = getNextId();
	node->name = impNode->mName.C_Str();

	printf("\tParsing node '%s' with assigned id %d\n", impNode->mName.C_Str(), node->id);

	// Obtain the static moment node trasform.
	aiVector3D translation;
	aiVector3D scaling;
	aiQuaternion rotation;

	// [TODO] Is decomposition the best thing we could do?
	impNode->mTransformation.Decompose(scaling, rotation, translation);

	// Translation.
	Parameter* const translParam = node->paramBlock.FindParameter("translation", ParameterType::Float3);
	translParam->SetStaticValue(&translation);

	// Scaling.
	Parameter* const scaleParam = node->paramBlock.FindParameter("scaling", ParameterType::Float3);
	scaleParam->SetStaticValue(&scaling);

	// Rotation.
	// CAUTION: ASSIMP stores quaternions in (w, x,y,z) order!
	Parameter* const rotationParam = node->paramBlock.FindParameter("rotation", ParameterType::Quaternion);
	const quatf q = normalized(quatf(rotation.x, rotation.y, rotation.z, rotation.w));
	rotationParam->SetStaticValue(&q);

	// Find the attached meshes.
	for(unsigned t = 0; t < impNode->mNumMeshes; ++t)
	{
		const int impMeshIdx = impNode->mMeshes[t];
		printf("\t\t Attached mesh'%s'\n", impScene->mMeshes[impMeshIdx]->mName.C_Str());

		aiMesh* const impMesh = impScene->mMeshes[impMeshIdx];
		aiMaterial* const impMaterial = (impMesh->mMaterialIndex >= 0)
			? impScene->mMaterials[impMesh->mMaterialIndex]
			: nullptr;

		Model::MeshAttachment meshAttachment;
		meshAttachment.mesh = FindMesh(impScene->mMeshes[impMeshIdx]);
		meshAttachment.material = impMaterial ? iaMaterialToMaterial[impMaterial] : nullptr;

		sgeAssert(meshAttachment.mesh); // A mesh should always be attached.

		node->meshAttachments.push_back(meshAttachment);
	}

	// Parse the child nodes.
	for(unsigned t = 0; t < impNode->mNumChildren; ++t)
	{
		Model::Node* const childNode = ParseNodesRecursive(impNode->mChildren[t]);
		sgeAssert(childNode != NULL);
		node->childNodes.push_back(childNode);
	}

	return node;
}

//---------------------------------------------------------------------
void AssimpSceneParser::ResolveBonesNodePointer()
{
	for(const auto& itr : bonesToResolve)
	{
		Model::Bone* const bone = itr.first; sgeAssert(bone != NULL);
		bone->node = FindNodeByName(itr.second.c_str());

		if(bone->node == NULL) {
			sgeAssert(false);
		}
	}
}

//---------------------------------------------------------------------
void AssimpSceneParser::ParseAnimations()
{
	static_assert(sizeof(aiVector3D) == 3 * sizeof(float), "");

	// Store all found animations(or as I call them 'curves'). This is used to later define all the animation in the model.
	std::map<std::string, float> curvesDurations;

	// [KEYFRAME_REDUCE] This function tries to eleminate keyframes that doens't have any meaning for the animation(aka dfdx for frame T is the same as the one for T-1)
	// Currently the algorighm is clearly wrong for quaternions, but it's useful becase it removes duplicate keyframes.
	// [TODO] Find a way to remove the duplicated code...

	// [TODO] add these as parameters they are scene dependant.
	const float dfdx_epsilon = 1e-2f; // not the best constant...
	const float quat_diff_epsion = 1e-2f;

	int numAvoidedKeyFrames = 0; // Just debug statistics has nothing to do with the export.


	for(int animIdx = 0; animIdx < impScene->mNumAnimations; ++animIdx)
	{
		const aiAnimation* const anim = impScene->mAnimations[animIdx];

		const float timeScale = 1.f / (anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 1.f);
		std::string animatioName = impScene->mAnimations[animIdx]->mName.C_Str();
		const float animationDuration = impScene->mAnimations[animIdx]->mDuration * timeScale;

		// If an animation name wasn't provided rename it to something.
		if(animatioName == "")
		{
			char defaultAnimation[64];
			sprintf(defaultAnimation, "default_animation_%d", animIdx);
			animatioName=defaultAnimation;
		}

		curvesDurations[animatioName] = animationDuration; 

		printf("Parsing animation: '%s' ...", animatioName.c_str());

		for(int chidx = 0; chidx < anim->mNumChannels; ++chidx)
		{
			const aiNodeAnim* const nodeAnim = anim->mChannels[chidx];
			Model::Node* const node = FindNodeByName(nodeAnim->mNodeName.C_Str());

			ParameterBlock& block = node->paramBlock;

			// Translation
			{
				// [KEYFRAME_REDUCE]
				struct
				{
					bool chain = false;
					vec3f ddx = vec3f(0.f);
					float time;
					vec3f data;
				}prev;

				Parameter* const parameter = block.FindParameter("translation", ParameterType::Float3);
				
				if(parameter->CreateCurve(animatioName.c_str()) == false) {
					sgeAssert(false);
				}
				
				ParameterCurve* const dataCurve = parameter->GetCurve(animatioName.c_str());

				for(int t = 0; t < nodeAnim->mNumPositionKeys; ++t)
				{
					const float time = nodeAnim->mPositionKeys[t].mTime * timeScale;
					const vec3f data = *(vec3f*)&nodeAnim->mPositionKeys[t].mValue;
					
					// Skip the keyframe if the derivative is the same as the previous one.
					const float dx = (t != 0) ? time - prev.time : 0.f;
					const vec3f ddx = (t != 0) ? (data - prev.data) / dx : vec3f(0.f);

					if(t != 0) {

						const bool x = fltcmp_radius(ddx.x, prev.ddx.x, dfdx_epsilon);
						const bool y = fltcmp_radius(ddx.y, prev.ddx.y, dfdx_epsilon);
						const bool z = fltcmp_radius(ddx.z, prev.ddx.z, dfdx_epsilon);

						if(x && y && z) {
							// Move the point to the current location...
							prev.chain = false;
							prev.ddx = ddx;
							prev.time = time;
							prev.data = data;
							
							numAvoidedKeyFrames++;
							continue;
						}
					}
					
					// Add the prev point if it was on a chain of points with same derivative.
					if(prev.chain) {
						dataCurve->Add(prev.time, &prev.data);
						sgeAssert(dataCurve->debug_VerifyData());
					}

					dataCurve->Add(time, &data);
					sgeAssert(dataCurve->debug_VerifyData());

					prev.chain = false;
					prev.ddx = ddx;
					prev.time = time;
					prev.data = data;
				}
			}

			// Rotation
			{
				// [KEYFRAME_REDUCE]
				// Just initialize these to something invalid.
				struct
				{
					bool chain = false;
					quatf data = quatf(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
					float time = FLT_MAX;
				}prev;

				Parameter* const parameter = block.FindParameter("rotation", ParameterType::Quaternion);
				
				if(parameter->CreateCurve(animatioName.c_str()) == false) {
					sgeAssert(false);
				}
				
				ParameterCurve* const dataCurve = parameter->GetCurve(animatioName.c_str());

				for(int t = 0; t < nodeAnim->mNumRotationKeys; ++t)
				{
					// [CAUTION] Assimp quaternions has "w xyz" memory layout, so we need to shuffle it a bit.
					const aiQuaternion& assimpQ = nodeAnim->mRotationKeys[t].mValue;
				
					const quatf data = quatf(assimpQ.x, assimpQ.y, assimpQ.z, assimpQ.w).normalized();
					const float time = nodeAnim->mRotationKeys[t].mTime * timeScale;

					// Skip the keyframe if the data is the same.
					if(t == 0)
					{
						/*const bool x = fltcmp_radius(data.x, prev.data.x, quat_diff_epsion);
						const bool y = fltcmp_radius(data.y, prev.data.y, quat_diff_epsion);
						const bool z = fltcmp_radius(data.z, prev.data.z, quat_diff_epsion);
						const bool w = fltcmp_radius(data.w, prev.data.w, quat_diff_epsion);
						*/
						if(data == prev.data)
						{
							prev.chain = true;
							prev.data = data;
							prev.time = time;
							continue;
						}
					}

					if(prev.chain) {
						dataCurve->Add(time, &data);
						sgeAssert(dataCurve->debug_VerifyData());
					}

					dataCurve->Add(time, &data);
					sgeAssert(dataCurve->debug_VerifyData());

					prev.chain = false;
					prev.data = data;
					prev.time = time;
				}
			}

			// Scaling
			{
				// [KEYFRAME_REDUCE]
				struct
				{
					bool chain = false;
					vec3f ddx = vec3f(0.f);
					float time;
					vec3f data;
				}prev;

				Parameter* const parameter = block.FindParameter("scaling", ParameterType::Float3);
				
				if(parameter->CreateCurve(animatioName.c_str()) == false) {
					sgeAssert(false);
				}
				
				ParameterCurve* const dataCurve = parameter->GetCurve(animatioName.c_str());

				for(int t = 0; t < nodeAnim->mNumScalingKeys; ++t)
				{
					const float time = nodeAnim->mScalingKeys[t].mTime * timeScale;
					const vec3f data = *(vec3f*)&nodeAnim->mScalingKeys[t].mValue;
					
					// Skip the keyframe if the derivative is the same as the previous one.
					const float dx = (t != 0) ? time - prev.time : 0.f;
					const vec3f ddx = (t != 0) ? (data - prev.data) / dx : vec3f(0.f);

					if(t != 0) {

						const bool x = fltcmp_radius(ddx.x, prev.ddx.x, dfdx_epsilon);
						const bool y = fltcmp_radius(ddx.y, prev.ddx.y, dfdx_epsilon);
						const bool z = fltcmp_radius(ddx.z, prev.ddx.z, dfdx_epsilon);

						if(x && y && z) {
							// Move the point to the current location...
							prev.chain = false;
							prev.ddx = ddx;
							prev.time = time;
							prev.data = data;
							
							numAvoidedKeyFrames++;
							continue;
						}
					}
					
					// Add the prev point if it was on a chain of points with same derivative.
					if(prev.chain) {
						dataCurve->Add(prev.time, &prev.data);
						sgeAssert(dataCurve->debug_VerifyData());
					}

					dataCurve->Add(time, &data);
					sgeAssert(dataCurve->debug_VerifyData());

					prev.chain = false;
					prev.ddx = ddx;
					prev.time = time;
					prev.data = data;
				}
			}
		}
	}

	for(auto& curve : curvesDurations)
	{
		model->m_animations.push_back(Model::AnimationInfo(curve.first.c_str(), 0.f, curve.second));
	}

	printf(" Done!\n"); // relates to "Parsing animation: ..."
	printf("Num avoided keyframes: %d\n", numAvoidedKeyFrames);

}

//---------------------------------------------------------------------
Model::MeshData* AssimpSceneParser::GetBestSuitableMeshData(aiMesh* impMesh)
{
	// 1st resolve skinned meshes.
	const bool hasBones = (impMesh->mNumBones != 0);

	//
	if(settings.meshPacking == MeshPacking::NoPacking || hasBones)
	{
		char meshDataName[512];
		sprintf(meshDataName, "%s%d", impMesh->mName.C_Str(), impMesh->mMaterialIndex);
		
		// Currently we store all skinned meshes in a spearate pack.
		model->m_meshesData.push_back(model->m_containerMeshData.new_element());
		return model->m_meshesData.back();
	}

	//
	if(settings.meshPacking == MeshPacking::PackPerMesh)
	{
		for(Model::MeshData* meshData : model->m_meshesData)
		for(Model::Mesh* mesh : meshData->meshes)
		{
			// Sometimes a model could be split in few different parts because of the different material binding.
			// In order to reduce the context switching while drawing the packing policy stores these together.
			if(mesh->name == impMesh->mName.C_Str())
			{
				return meshData;
			}
		}
	}

	//
	if(settings.meshPacking == MeshPacking::PackWholeScene)
	{
		if(cachedWholeSceneMeshData == NULL)
		{
			model->m_meshesData.push_back(model->m_containerMeshData.new_element());
			cachedWholeSceneMeshData = model->m_meshesData.back();
		}

		return cachedWholeSceneMeshData;
	}

	// No suitable meshData found. We must allocate a new one.
	char meshDataName[512];
	sprintf(meshDataName, "%s%d", impMesh->mName.C_Str(), impMesh->mMaterialIndex);
		
	// Currently we store all skinned meshes in a spearate pack.
	model->m_meshesData.push_back(model->m_containerMeshData.new_element());
	return model->m_meshesData.back();
}

//---------------------------------------------------------------------
Model::Mesh* AssimpSceneParser::FindMesh(aiMesh* impMesh)
{
	sgeAssert(impMesh);

	const auto itr = iaMeshToMesh.find(impMesh);

	if(itr == std::end(iaMeshToMesh)) 
	{
		// Should never happen.
		printf("FATAL ERROR: Failing to find mesh with name : '%s'!\n", impMesh->mName.C_Str());
		sgeAssert(false);
		return NULL;
	}

	return itr->second;
}

//---------------------------------------------------------------------
Model::Node* AssimpSceneParser::FindNodeByName(const char const* nodeName)
{
	sgeAssert(nodeName);

	for(auto& node : model->m_nodes) {
		if(node->name == nodeName) {
			return node;
		}
	}

	return nullptr;
}

}
