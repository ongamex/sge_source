#include <functional>

#include "sge_core/AssetLibrary.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/range_loop.h"

#include "EvaluatedModel.h"
#include "Model.h"

namespace sge {

const char s_DiffuseTextureParamName[] = "texDiffuse";
const char s_TexNormalMap[] = "texNormal";

void EvaluatedModel::initialize(AssetLibrary* const assetLibrary, Model::Model* model) {
	sgeAssert(assetLibrary != NULL);
	sgeAssert(model);

	// Reset the object's state.
	*this = EvaluatedModel();

	m_assetLibrary = assetLibrary;
	m_model = model;
}

void EvaluatedModel::buildNodeRemappingLUT(const Model::Model* otherModel) {
	if (!otherModel) {
		return;
	}

	// Skip if the remapping is already there.
	if (m_nodeRemapping.find_element(otherModel) != nullptr) {
		return;
	}

	vector_map<const Model::Node*, const Model::Node*>& nodeRemap = m_nodeRemapping[otherModel];

	// Find the equvalent node in the otherModel node and cache it.
	for (Model::Node* node : m_model->m_nodes) {
		// CAUTION: Currently the retargeting is mapping node-to-node using names.
		// This is highly unreliable as there may be multiple nodes with the same name in a single model.
		// A possible fix is to search these nodes using the hierarchy
		// for example if we are looking for a node named "hand":
		// "upper_torso|left_arm|hand".
		const Model::Node* const foundNode = otherModel->FindFirstNodeByName(node->name.c_str());

		if (foundNode != nullptr) {
			nodeRemap[node] = foundNode;
		}
	}
}

bool EvaluatedModel::evaluate(const char* const curveName, float const time) {
	std::vector<EvalMomentSets> evalSets;
	evalSets.push_back(EvalMomentSets{m_model, std::string(curveName ? curveName : ""), time, 1.f});

	return evaluate(evalSets);
}

bool EvaluatedModel::evaluate(const std::vector<EvalMomentSets>& evalSets) {
	if (evalSets.size() == 0)
		return false;

	for (auto& moment : evalSets) {
		buildNodeRemappingLUT(moment.model);
	}

	evaluateNodesFromMoments(evalSets);
	evaluateMaterials();
	evaluateSkinning();

	return true;
}

bool EvaluatedModel::evaluate(vector_map<const Model::Node*, mat4f>& boneOverrides) {
	evaluateNodesFromExternalBones(boneOverrides);
	evaluateMaterials();
	evaluateSkinning();
	return true;
}

bool EvaluatedModel::evaluateNodes_common() {
	aabox.setEmpty();
	m_nodes.clear();

	// Initialize the attachments to the node.
	for (const Model::Node* const originalNode : m_model->m_nodes) {
		EvaluatedNode& evalNode = m_nodes[originalNode];
		evalNode.name = originalNode->name.empty() ? "" : originalNode->name.c_str();

		// Obtain the inital bounding box by getting the unevaluated attached meshes bounding boxes.
		for (const Model::MeshAttachment& meshAttachment : originalNode->meshAttachments) {
			evalNode.aabb.expand(meshAttachment.mesh->aabox);
		}
	}

	return true;
}

bool EvaluatedModel::evaluateNodesFromMoments(const std::vector<EvalMomentSets>& evalSets) {
	evaluateNodes_common();

	// Evaluates the nodes. They may be effecte by multiple models (stealing animations and blending them)
	for (int const iMoment : range_int(int(evalSets.size()))) {
		const EvalMomentSets& moment = evalSets[iMoment];
		auto& nodeRemap = m_nodeRemapping[moment.model];

		const Model::AnimationInfo* const animInfo = moment.model->findAnimation(moment.animationName);
		const float evalTime = animInfo ? moment.time + animInfo->startTime : moment.time;

		for (const Model::Node* const originalNode : m_model->m_nodes) {
			// Use the node form the specified Model in the node, if such node doesn't exists, fallback to the originalNode.
			const Model::Node** ppFoundNode = nodeRemap.find_element(originalNode);
			const Model::Node* nodeToUse = ppFoundNode ? *ppFoundNode : originalNode;

			const Parameter* const scalingPrm = nodeToUse->paramBlock.FindParameter("scaling");
			const Parameter* const rotationPrm = nodeToUse->paramBlock.FindParameter("rotation");
			const Parameter* const translationPrm = nodeToUse->paramBlock.FindParameter("translation");

			EvaluatedNode& evalNode = m_nodes[originalNode];

			transf3d transf;
			scalingPrm->Evalute(&transf.s, moment.animationName.c_str(), evalTime);
			rotationPrm->Evalute(&transf.r, moment.animationName.c_str(), evalTime);
			translationPrm->Evalute(&transf.p, moment.animationName.c_str(), evalTime);

			mat4f const transfMtx = transf.toMatrix();

			// CAUTION: We assume that all transforms in evalNode are initialized to zero!
			evalNode.evalLocalTransform += transfMtx * moment.weight; // Hmm... is this at least semi correct?
		}
	}

	// Evaluate the node global transform by traversing the node hierarchy using the local transform computed above.
	// Evaluate attached meshes to the evaluated nodes.
	std::function<void(Model::Node*, mat4f)> traverseGlobalTransform;
	traverseGlobalTransform = [&](Model::Node* node, const mat4f& parentTransfrom) -> void {
		EvaluatedNode* evalNode = m_nodes.find_element(node);
		evalNode->evalGlobalTransform = parentTransfrom * evalNode->evalLocalTransform;

		for (auto& childNode : node->childNodes) {
			traverseGlobalTransform(childNode, evalNode->evalGlobalTransform);
		}

		if (evalNode->aabb.IsEmpty() == false) {
			aabox.expand(evalNode->aabb.getTransformed(evalNode->evalGlobalTransform));
		}
	};

	traverseGlobalTransform(m_model->m_rootNode, mat4f::getIdentity());


	return true;
}

bool EvaluatedModel::evaluateNodesFromExternalBones(vector_map<const Model::Node*, mat4f>& boneGlobalTrasnformOverrides) {
	evaluateNodes_common();

	for (const Model::Node* const originalNode : m_model->m_nodes) {
		EvaluatedNode& evalNode = m_nodes[originalNode];
		// evalNode.evalLocalTransform is not computed as it isn't needed by skinning.
		evalNode.evalGlobalTransform = boneGlobalTrasnformOverrides[originalNode];
		aabox.expand(evalNode.aabb.getTransformed(evalNode.evalGlobalTransform));
	}

	return true;
}

bool EvaluatedModel::evaluateMaterials() {
	m_materials.clear();
	m_materials.reserve(m_model->m_materials.size());


	// Evaluate the materials.
	// TODO:
	// The animation if the materials is now broken, as I was to lazy to fix it, as this isn't really that used.
	{
		std::string texPath;

		for (Model::Material* mtl : m_model->m_materials) {
			EvaluatedMaterial& evalMtl = m_materials[mtl];

			evalMtl.name = mtl->name;

			// Check if there is a diffuse color attached here.

			if (const Parameter* diffuseColorPrm = mtl->paramBlock.FindParameter("diffuseColor");
			    diffuseColorPrm && diffuseColorPrm->GetType() == ParameterType::Float4) {
				diffuseColorPrm->Evalute(&evalMtl.diffuseColor, "", 0.f);
			}

			if (const Parameter* roughnessPrm = mtl->paramBlock.FindParameter("roughness");
			    roughnessPrm && roughnessPrm->GetType() == ParameterType::Float) {
				roughnessPrm->Evalute(&evalMtl.roughness, "", 0.f);
			} else {
				evalMtl.roughness = 1.f; // If not specified set to some sensible default;
			}

			if (const Parameter* metallicPrm = mtl->paramBlock.FindParameter("metallic");
			    metallicPrm && metallicPrm->GetType() == ParameterType::Float) {
				metallicPrm->Evalute(&evalMtl.metallic, "", 0.f);
			} else {
				evalMtl.metallic = 0.f;
			}

			// Check if there is a diffuse texture attached here.
			if (const Parameter* const tex = mtl->paramBlock.FindParameter(s_DiffuseTextureParamName);
			    tex && tex->GetType() == ParameterType::String) {
				tex->Evalute(&texPath, "", 0.f);
				texPath = m_model->m_loadSets.assetDir + texPath;
				evalMtl.diffuseTexture = m_assetLibrary->getAsset(AssetType::TextureView, texPath.c_str(), true);

				// If there is a texture force the diffuse color to be 1, as Maya doesn't respet it when there is a texture involved.
				evalMtl.diffuseColor = vec4f(1.f);
			}

			// Normal map.
			if (const Parameter* const tex = mtl->paramBlock.FindParameter(s_TexNormalMap);
			    tex && tex->GetType() == ParameterType::String) {
				tex->Evalute(&texPath, "", 0.f);
				texPath = m_model->m_loadSets.assetDir + texPath;
				evalMtl.texNormalMap = m_assetLibrary->getAsset(AssetType::TextureView, texPath.c_str(), true);
			}

			// Metallic map.
			if (const Parameter* const tex = mtl->paramBlock.FindParameter("texMetallic"); tex && tex->GetType() == ParameterType::String) {
				tex->Evalute(&texPath, "", 0.f);
				texPath = m_model->m_loadSets.assetDir + texPath;
				evalMtl.texMetallic = m_assetLibrary->getAsset(AssetType::TextureView, texPath.c_str(), true);
			}

			// Roughness map.
			if (const Parameter* const tex = mtl->paramBlock.FindParameter("texRoughness");
			    tex && tex->GetType() == ParameterType::String) {
				tex->Evalute(&texPath, "", 0.f);
				texPath = m_model->m_loadSets.assetDir + texPath;
				evalMtl.texRoughness = m_assetLibrary->getAsset(AssetType::TextureView, texPath.c_str(), true);
			}
		}
	}

	return true;
}

bool EvaluatedModel::evaluateSkinning() {
	SGEContext* const context = m_assetLibrary->getDevice()->getContext();

	// Evaluate the meshes.
	for (Model::MeshData* const meshData : m_model->m_meshesData)
		for (Model::Mesh* const mesh : meshData->meshes) {
			EvaluatedMesh& evalMesh = meshes[mesh];
			evalMesh.pReferenceMesh = mesh;

			evalMesh.vertexDeclIndex = context->getDevice()->getVertexDeclIndex(mesh->vertexDecl.data(), int(mesh->vertexDecl.size()));
			bool vertexDeclHasVertexColor = false;
			bool vertexDeclHasUv = false;
			bool vertexDeclHasNormals = false;
			bool vertexDeclHasTangentSpace = false;
			int tangetSpaceCounter = 0;
			for (const VertexDecl& decl : mesh->vertexDecl) {
				if (decl.semantic == "a_color") {
					vertexDeclHasVertexColor = true;
				}

				if (decl.semantic == "a_uv") {
					vertexDeclHasUv = true;
					tangetSpaceCounter++; // Tangents space is kind of useless for shading without UVs.
				}

				if (decl.semantic == "a_normal") {
					vertexDeclHasNormals = true;
					tangetSpaceCounter++;
				}

				if (decl.semantic == "a_tangent") {
					tangetSpaceCounter++;
				}

				if (decl.semantic == "a_binormal") {
					tangetSpaceCounter++;
				}
			}

			vertexDeclHasTangentSpace = (tangetSpaceCounter == 4);

			evalMesh.indexBuffer = meshData->indexBuffer;
			evalMesh.vertexBuffer = meshData->vertexBuffer;
			if (mesh->bones.empty() == false) {
				// If the mesh has software skinning perform CPU skinning.
				const int posByteOffset = mesh->vbPositionOffsetBytes;
				const int normalByteOffset = mesh->vbNormalOffsetBytes;

				// Duplicate the raw vertex buffer data and zero the vertex position.
				std::vector<char> vbdata(meshData->vertexBufferRaw.size());
				std::vector<mat4f> bonesTransformTexData(mesh->bones.size());
				for (int iBone = 0; iBone < mesh->bones.size(); ++iBone) {
					const Model::Bone& bone = mesh->bones[iBone];
					const mat4f boneTransformWithOffsetModelObjectSpace =
					    m_nodes.find_element(bone.node)->evalGlobalTransform * bone.offsetMatrix;
					bonesTransformTexData[iBone] = boneTransformWithOffsetModelObjectSpace;
				}


				{
					TextureData data = TextureData(bonesTransformTexData.data(), sizeof(vec4f) * 4);

					if (evalMesh.skinningBoneTransfsTex.HasResource() == false) {
						TextureDesc td;
						td.textureType = UniformType::Texture2D;
						td.usage = TextureUsage::DynamicResource;
						td.format = TextureFormat::R32G32B32A32_FLOAT;
						td.texture2D.arraySize = 1;
						td.texture2D = Texture2DDesc(4, int(mesh->bones.size()));

						SamplerDesc sd;
						sd.filter = TextureFilter::Min_Mag_Mip_Point;

						evalMesh.skinningBoneTransfsTex = context->getDevice()->requestResource<Texture>();
						evalMesh.skinningBoneTransfsTex->create(td, &data, sd);
					} else {
						context->updateTextureData(evalMesh.skinningBoneTransfsTex.GetPtr(), data);
					}

					
				}

#if 0
				sgeAssert((vbdata.size() % mesh->stride) == 0);
				const int numVerts = int(vbdata.size() / mesh->stride);
				for (size_t t = 0; t < numVerts; ++t) {
					const char* const srcVertex = meshData->vertexBufferRaw.data() + mesh->stride * t;
					char* const destVertex = vbdata.data() + mesh->stride * t;

					memcpy(destVertex, srcVertex, mesh->stride);

					vec3f& pos_w = *(vec3f*)(destVertex + posByteOffset);
					pos_w = vec3f(0);

					if (normalByteOffset >= 0) {
						vec3f& normal_w = *(vec3f*)(destVertex + normalByteOffset);
						normal_w = vec3f(0);
					}
				}

				for (const auto& bone : mesh->bones) {
					const mat4f m = m_nodes.find_element(bone.node)->evalGlobalTransform * bone.offsetMatrix;

					for (size_t t = 0; t < bone.vertexIds.size(); ++t) {
						// CAUTION:
						// With ASSIMP converted models this line was :
						//
						// bool const hasIndices = (mesh->ibFmt != UniformType::Unknown);
						// const int vid = hasIndices ? ibd[bone.vertexIds[t]] : bone.vertexIds[t];
						//
						// However after implementing the index buffer generation in FBX SDK converter
						// the index buffer here really did not make any sense.
						const int vid = bone.vertexIds[t];

						// Accumulate the transformed vertex.
						const size_t posOffset = mesh->stride * vid + posByteOffset;
						const vec3f pos_r = *(vec3f*)(mesh->pMeshData->vertexBufferRaw.data() + posOffset);
						vec3f& pos_w = *(vec3f*)(vbdata.data() + mesh->vbByteOffset + posOffset);
						pos_w += mat_mul_pos(m, pos_r) * bone.weights[t];

						if (normalByteOffset >= 0) {
							const size_t normalOffset = mesh->stride * vid + normalByteOffset;
							const vec3f norm_r = *(vec3f*)(mesh->pMeshData->vertexBufferRaw.data() + normalOffset);
							vec3f& normal_w = *(vec3f*)(vbdata.data() + mesh->vbByteOffset + normalOffset);
							normal_w += mat_mul_dir(m, norm_r) * bone.weights[t];
						}

						// TODO: TBN in the future.
					}
				}

				// Update the vertex buffers.
				if (evalMesh.vertexBuffer.IsResourceValid() == false) {
					evalMesh.vertexBuffer = context->getDevice()->requestResource<Buffer>();
				}

				if (evalMesh.vertexBuffer->isValid() == false || evalMesh.vertexBuffer->getDesc().sizeBytes < vbdata.size()) {
					BufferDesc bd = BufferDesc::GetDefaultVertexBuffer((uint32)vbdata.size(), ResourceUsage::Dynamic);
					evalMesh.vertexBuffer->create(bd, NULL);
				}

				void* const pMappedData = context->map(evalMesh.vertexBuffer, Map::WriteDiscard);
				memcpy(pMappedData, vbdata.data(), vbdata.size());
				context->unMap(evalMesh.vertexBuffer);
#endif
			}

			// Finally fill the geometry structure.
			evalMesh.geom = Geometry(evalMesh.vertexBuffer.GetPtr(), evalMesh.indexBuffer.GetPtr(),
			                         evalMesh.skinningBoneTransfsTex.GetPtr(), evalMesh.vertexDeclIndex, vertexDeclHasVertexColor,
			                         vertexDeclHasUv, vertexDeclHasNormals, vertexDeclHasTangentSpace, evalMesh.pReferenceMesh->primTopo,
			                         evalMesh.pReferenceMesh->vbByteOffset, evalMesh.pReferenceMesh->ibByteOffset,
			                         evalMesh.pReferenceMesh->stride, evalMesh.pReferenceMesh->ibFmt, evalMesh.pReferenceMesh->numElements);
		}

	// Attach the meshes to the evaluated nodes.
	std::function<void(Model::Node*)> meshTraverse = [&](Model::Node* node) -> void {
		EvaluatedNode* evalNode = m_nodes.find_element(node);

		if (evalNode->attachedMeshes.size() != node->meshAttachments.size()) {
			evalNode->attachedMeshes.clear();
			for (const Model::MeshAttachment& attachmentMesh : node->meshAttachments) {
				EvaluatedMeshAttachment evalMeshAttachment;
				evalMeshAttachment.pMesh = meshes.find_element(attachmentMesh.mesh);
				evalMeshAttachment.pMaterial = (attachmentMesh.material) ? m_materials.find_element(attachmentMesh.material) : nullptr;

				evalNode->attachedMeshes.push_back(evalMeshAttachment);
			}
		}

		for (auto& childNode : node->childNodes) {
			meshTraverse(childNode);
		}
	};

	meshTraverse(m_model->m_rootNode);

	return true;
}

float EvaluatedModel::Raycast(const Ray& ray, Model::Node** ppNode, const char* const positionSemantic) const {
	float mint = FLT_MAX;
	Model::Node* minNode = NULL;

	for (Model::Node* const node : m_model->m_nodes) {
		mat4f const invTransf = inverse(m_nodes.find_element(node)->evalGlobalTransform);

		Ray invRay;
		invRay.pos = mat_mul_pos(invTransf, ray.pos);
		invRay.dir = mat_mul_dir(invTransf, ray.dir);

		for (const Model::MeshAttachment& meshAttachment : node->meshAttachments) {
			const float t = meshAttachment.mesh->Raycast(invRay, positionSemantic);
			if (t < mint) {
				mint = t;
				minNode = node;
			}
		}
	}

	if (ppNode != NULL) {
		*ppNode = minNode;
	}

	return mint;
}

void Model_FindReferencedResources(std::vector<std::string>& referencedTextures, const Model::Model& model) {
	for (const auto& mtl : model.m_materials) {
		const Parameter* const prmTexDiffuse = mtl->paramBlock.FindParameter(s_DiffuseTextureParamName);
		if ((prmTexDiffuse != nullptr) && (prmTexDiffuse->GetType() == ParameterType::String)) {
			const std::string texture = (const char*)prmTexDiffuse->GetStaticValue();
			referencedTextures.emplace_back(std::move(texture));
		}

		const Parameter* const prmTexNormal = mtl->paramBlock.FindParameter(s_TexNormalMap);
		if ((prmTexNormal != nullptr) && (prmTexNormal->GetType() == ParameterType::String)) {
			const std::string texture = (const char*)prmTexNormal->GetStaticValue();
			referencedTextures.emplace_back(std::move(texture));
		}
	}

	return;
}

} // namespace sge
