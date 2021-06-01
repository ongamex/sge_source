#include "sge_utils/utils/vector_map.h"
#include <sge_utils/utils/FileStream.h>
#include <sge_utils/utils/json.h>
#include <stdexcept>

#include "Model.h"
#include "ModelReader.h"

namespace sge {

struct ModelParseExcept : public std::logic_error {
	ModelParseExcept(const char* msg)
	    : std::logic_error(msg) {}
};


PrimitiveTopology::Enum PrimitiveTolologyFromString(const char* str) {
	if (strcmp(str, "TriangleList") == 0)
		return PrimitiveTopology::TriangleList;
	if (strcmp(str, "TriangleStrip") == 0)
		return PrimitiveTopology::TriangleStrip;
	if (strcmp(str, "LineList") == 0)
		return PrimitiveTopology::LineList;
	if (strcmp(str, "LineStrip") == 0)
		return PrimitiveTopology::LineStrip;
	if (strcmp(str, "PointList") == 0)
		return PrimitiveTopology::PointList;

	sgeAssert(false);
	throw ModelParseExcept("Unknown primitive topology!");
}

UniformType::Enum UniformTypeFromString(const char* str) {
	if (strcmp(str, "uint16") == 0)
		return UniformType::Uint16;
	if (strcmp(str, "uint32") == 0)
		return UniformType::Uint;
	if (strcmp(str, "float") == 0)
		return UniformType::Float;
	if (strcmp(str, "float2") == 0)
		return UniformType::Float2;
	if (strcmp(str, "float3") == 0)
		return UniformType::Float3;
	if (strcmp(str, "float4") == 0)
		return UniformType::Float4;
	if (strcmp(str, "int") == 0)
		return UniformType::Int;
	if (strcmp(str, "int2") == 0)
		return UniformType::Int2;
	if (strcmp(str, "int3") == 0)
		return UniformType::Int3;
	if (strcmp(str, "int4") == 0)
		return UniformType::Int4;

	sgeAssert(false);
	throw ModelParseExcept("Unknown uniform type!");
}

template <typename T>
void ModelReader::loadDataChunk(std::vector<T>& resultBuffer, const int chunkId) {
	const DataChunkDesc* chunkDesc = nullptr;
	for (const auto& desc : dataChunksDesc) {
		if (desc.chunkId == chunkId) {
			chunkDesc = &desc;
			break;
		}
	}
	if (!chunkDesc) {
		sgeAssert(false);
		throw ModelParseExcept("Chunk desc not found!");
	}

	if (chunkDesc->sizeBytes % sizeof(T)) {
		sgeAssert(false);
		throw ModelParseExcept("ChunkSize % sizeof(T) != 0!");
	}

	const size_t numElems = chunkDesc->sizeBytes / sizeof(T);
	resultBuffer.resize(numElems);

	const size_t firstChunkDataLocation = irs->pointerOffset();
	irs->seek(SeekOrigin::Current, chunkDesc->byteOffset);
	irs->read(resultBuffer.data(), chunkDesc->sizeBytes);

	// Seek back to the 1st chunk.
	irs->seek(SeekOrigin::Begining, firstChunkDataLocation);
}

void ModelReader::loadDataChunkRaw(void* const ptr, const size_t ptrExpectedSize, const int chunkId) {
	const DataChunkDesc* chunkDesc = nullptr;
	for (const auto& desc : dataChunksDesc) {
		if (desc.chunkId == chunkId) {
			chunkDesc = &desc;
			break;
		}
	}

	if (!chunkDesc) {
		sgeAssert(false);
		throw ModelParseExcept("Chunk desc not found!");
	}

	if (ptrExpectedSize != chunkDesc->sizeBytes) {
		sgeAssert(false);
		throw ModelParseExcept("Could not load data chunk, because the preallocated memory isn't enough!");
	}

	const size_t firstChunkDataLocation = irs->pointerOffset();
	irs->seek(SeekOrigin::Current, chunkDesc->byteOffset);
	irs->read(ptr, chunkDesc->sizeBytes);

	// Seek back to the 1st chunk.
	irs->seek(SeekOrigin::Begining, firstChunkDataLocation);
}

const ModelReader::DataChunkDesc& ModelReader::FindDataChunkDesc(const int chunkId) const {
	for (const auto& desc : dataChunksDesc) {
		if (desc.chunkId == chunkId) {
			return desc;
		}
	}

	throw ModelParseExcept("Chunk desc not found!");
}

bool ModelReader::loadModel(const ModelLoadSettings loadSets, IReadStream* const iReadStream, Model& model) {
	try {
		dataChunksDesc.clear();
		irs = iReadStream;

		model = Model();
		model.setModelLoadSettings(loadSets);

		JsonParser jsonParser;
		if (!jsonParser.parse(irs)) {
			throw ModelParseExcept("Parsing the json header failed!");
		}

		// This point here is pretty important.
		// The irs pointer currently points at the beggining of the
		// data chunks. This pointer will jump around that section
		// in order to load the specific buffer data(like mesh data parameter data ect...)

		// [CAUTION] From this point DO NOT use directly irs
		// If you want to load a data chunk use LoadDataChunk

		const JsonValue* const jRoot = jsonParser.getRoot();

		// Load the data chunk desc.
		{
			const JsonValue* const jDataChunksDesc = jRoot->getMember("dataChunksDesc");
			dataChunksDesc.reserve(jDataChunksDesc->arrSize() / 3);

			for (size_t t = 0; t < jDataChunksDesc->arrSize(); t += 3) {
				DataChunkDesc chunkDesc;

				chunkDesc.chunkId = jDataChunksDesc->arrAt(t)->getNumberAs<int>();
				chunkDesc.byteOffset = jDataChunksDesc->arrAt(t + 1)->getNumberAs<int>();
				chunkDesc.sizeBytes = jDataChunksDesc->arrAt(t + 2)->getNumberAs<int>();

				dataChunksDesc.push_back(chunkDesc);
			}
		}

		// Load the animations.

		if (auto jAnimations = jRoot->getMember("animations")) {
			for (size_t t = 0; t < jAnimations->arrSize(); ++t) {
				auto jAnimation = jAnimations->arrAt(t);

				int animantionIndex = model.makeNewAnim();
				ModelAnimation& animation = *model.animationAt(animantionIndex);

				animation.animationName = std::string(jAnimation->getMember("animationName")->GetString());
				animation.durationSec = jAnimation->getMember("durationSec")->getNumberAs<float>();

				const JsonValue* jAllNodesKeyFrames = jAnimation->getMember("perNodeKeyFrames");

				const int numAnimatedNodes = int(jAllNodesKeyFrames->arrSize());

				// Load the position keyframes.
				for (int iAnimatedNode = 0; iAnimatedNode < numAnimatedNodes; ++iAnimatedNode) {
					const JsonValue* jNodeKeyFrames = jAllNodesKeyFrames->arrAt(iAnimatedNode);

					int nodeIndex = jNodeKeyFrames->getMember("nodeIndex")->getNumberAs<int>();
					const JsonValue* jKeyFrames = jNodeKeyFrames->getMember("keyFrames");

					if (const JsonValue* jKeyFramesPos = jKeyFrames->getMember("positionKeyFrames_chunkId")) {
						const int chunkId = jKeyFramesPos->getNumberAs<int>();
						const DataChunkDesc& chunkDesc = FindDataChunkDesc(chunkId);

						std::vector<char> chunkMemory(chunkDesc.sizeBytes);
						loadDataChunkRaw(chunkMemory.data(), chunkMemory.size() * sizeof(chunkMemory[0]), chunkId);

						const int valueTypeSizeBytes = sizeof(animation.perNodeKeyFrames[0].positionKeyFrames[0.f]);

						const int numPairsInChunk = int(chunkDesc.sizeBytes / (sizeof(float) + valueTypeSizeBytes));
						const char* readPtr = chunkMemory.data();
						for (int iPair = 0; iPair < numPairsInChunk; ++iPair) {
							const float keyTime = *(float*)(readPtr);
							readPtr += sizeof(float);
							const vec3f keyData = *(vec3f*)(readPtr);
							readPtr += valueTypeSizeBytes;

							animation.perNodeKeyFrames[nodeIndex].positionKeyFrames[keyTime] = keyData;
						}
					}

					// Load the rotation keyframes.
					if (const JsonValue* jKeyFramesRot = jKeyFrames->getMember("rotationKeyFrames_chunkId")) {
						const int chunkId = jKeyFramesRot->getNumberAs<int>();
						const DataChunkDesc& chunkDesc = FindDataChunkDesc(chunkId);

						std::vector<char> chunkMemory(chunkDesc.sizeBytes);
						loadDataChunkRaw(chunkMemory.data(), chunkMemory.size() * sizeof(chunkMemory[0]), chunkId);

						const int valueTypeSizeBytes = sizeof(animation.perNodeKeyFrames[0].rotationKeyFrames[0.f]);

						const int numPairsInChunk = int(chunkDesc.sizeBytes / (sizeof(float) + valueTypeSizeBytes));
						const char* readPtr = chunkMemory.data();
						for (int iPair = 0; iPair < numPairsInChunk; ++iPair) {
							const float keyTime = *(float*)(readPtr);
							readPtr += sizeof(float);
							const quatf keyData = *(quatf*)(readPtr);
							readPtr += valueTypeSizeBytes;

							animation.perNodeKeyFrames[nodeIndex].rotationKeyFrames[keyTime] = keyData;
						}
					}


					// Load the scaling keyframes.

					if (const JsonValue* jKeyFramesScale = jKeyFrames->getMember("scalingKeyFrames_chunkId")) {
						const int chunkId = jKeyFramesScale->getNumberAs<int>();
						const DataChunkDesc& chunkDesc = FindDataChunkDesc(chunkId);

						std::vector<char> chunkMemory(chunkDesc.sizeBytes);
						loadDataChunkRaw(chunkMemory.data(), chunkMemory.size() * sizeof(chunkMemory[0]), chunkId);

						const int valueTypeSizeBytes = sizeof(animation.perNodeKeyFrames[0].scalingKeyFrames[0.f]);

						const int numPairsInChunk = int(chunkDesc.sizeBytes / (sizeof(float) + valueTypeSizeBytes));
						const char* readPtr = chunkMemory.data();
						for (int iPair = 0; iPair < numPairsInChunk; ++iPair) {
							const float keyTime = *(float*)(readPtr);
							readPtr += sizeof(float);
							const vec3f keyData = *(vec3f*)(readPtr);
							readPtr += valueTypeSizeBytes;

							animation.perNodeKeyFrames[nodeIndex].scalingKeyFrames[keyTime] = keyData;
						}
					}
				}
			}
		}

		// Load the materials.
		auto jMaterials = jRoot->getMember("materials");
		if (jMaterials) {
			for (size_t t = 0; t < jMaterials->arrSize(); ++t) {
				const JsonValue* const jMaterial = jMaterials->arrAt(t);

				int newMaterialIndex = model.makeNewMaterial();
				ModelMaterial* material = model.materialAt(newMaterialIndex);

				material->name = jMaterial->getMember("name")->GetString();
				jMaterial->getMember("diffuseColor")->getNumberArrayAs<float>(material->diffuseColor.data, 4);
				jMaterial->getMember("emissionColor")->getNumberArrayAs<float>(material->emissionColor.data, 4);
				material->metallic = jMaterial->getMember("metallic")->getNumberAs<float>();
				material->roughness = jMaterial->getMember("roughness")->getNumberAs<float>();

				if (const JsonValue* jTex = jMaterial->getMember("diffuseTextureName")) {
					material->diffuseTextureName = jTex->GetString();
				}

				if (const JsonValue* jTex = jMaterial->getMember("emissionTextureName")) {
					material->emissionTextureName = jTex->GetString();
				}

				if (const JsonValue* jTex = jMaterial->getMember("metallicTextureName")) {
					material->metallicTextureName = jTex->GetString();
				}

				if (const JsonValue* jTex = jMaterial->getMember("roughnessTextureName")) {
					material->roughnessTextureName = jTex->GetString();
				}
			}
		}

		// Load the MeshData.
		if (auto jMeshes = jRoot->getMember("meshes")) {
			for (size_t t = 0; t < jMeshes->arrSize(); ++t) {
				auto jMesh = jMeshes->arrAt(t);

				const int newMeshIndex = model.makeNewMesh();
				ModelMesh* mesh = model.meshAt(newMeshIndex);

				const int vertexDataChunkID = jMesh->getMember("vertexDataChunkId")->getNumberAs<int>();
				const JsonValue* jIndexBufferChunkID = jMesh->getMember("indexDataChunkId");

				loadDataChunk(mesh->vertexBufferRaw, vertexDataChunkID);
				const int indexDataChunkID = jIndexBufferChunkID ? jIndexBufferChunkID->getNumberAs<int>() : -1;
				if (indexDataChunkID > 0) {
					loadDataChunk(mesh->indexBufferRaw, indexDataChunkID);
				}

				mesh->name = jMesh->getMember("name")->GetString();
				mesh->primitiveTopology = PrimitiveTolologyFromString(jMesh->getMember("primitiveTopology")->GetString());
				mesh->vbByteOffset = jMesh->getMember("vbByteOffset")->getNumberAs<uint32>();
				mesh->numElements = jMesh->getMember("numElements")->getNumberAs<uint32>();
				mesh->numVertices = jMesh->getMember("numVertices")->getNumberAs<uint32>();

				// Load the index buffer. Note that there may be no index buffer.
				if (jMesh->getMember("ibByteOffset") && jMesh->getMember("ibFormat")) {
					mesh->ibByteOffset = jMesh->getMember("ibByteOffset")->getNumberAs<uint32>();
					mesh->ibFmt = UniformTypeFromString(jMesh->getMember("ibFormat")->GetString());
				}

				// The AABB of the mesh.
				jMesh->getMember("AABoxMin")->getNumberArrayAs<float>(mesh->aabox.min.data, 3);
				jMesh->getMember("AABoxMax")->getNumberArrayAs<float>(mesh->aabox.max.data, 3);

				// The vertex declaration.
				auto jVertexDecl = jMesh->getMember("vertexDecl");
				for (size_t iDecl = 0; iDecl < jVertexDecl->arrSize(); ++iDecl) {
					auto jDecl = jVertexDecl->arrAt(iDecl);

					VertexDecl decl;
					decl.bufferSlot = 0;
					decl.semantic = jDecl->getMember("semantic")->GetString();
					decl.byteOffset = jDecl->getMember("byteOffset")->getNumberAs<int>();
					decl.format = UniformTypeFromString(jDecl->getMember("format")->GetString());

					mesh->vertexDecl.push_back(decl);

					// Cache some commonly used semantics offsets.
					if (decl.semantic == "a_position") {
						mesh->vbPositionOffsetBytes = (int)decl.byteOffset;
					} else if (decl.semantic == "a_normal") {
						mesh->vbNormalOffsetBytes = (int)decl.byteOffset;
					} else if (decl.semantic == "a_uv") {
						mesh->vbUVOffsetBytes = (int)decl.byteOffset;
					}
				}

				// Bake the vertex stride.
				mesh->stride = int(mesh->vertexDecl.back().byteOffset) + UniformType::GetSizeBytes(mesh->vertexDecl.back().format);

				// The bones.
				if (const JsonValue* const jBones = jMesh->getMember("bones")) {
					mesh->bones.resize(jBones->arrSize());
					for (size_t iBone = 0; iBone < jBones->arrSize(); ++iBone) {
						auto jBone = jBones->arrAt(iBone);
						ModelMeshBone& bone = mesh->bones[iBone];

						// Becase nodes are loded before meshes, we must do that gymnastic.
						bone.nodeIdx = jBone->getMember("boneIndex")->getNumberAs<int>();
						loadDataChunkRaw(&bone.offsetMatrix, sizeof(bone.offsetMatrix),
						                 jBone->getMember("offsetMatrixChunkId")->getNumberAs<int>());
					}
				}

				// Finally Create the GPU resources.
				// const ResourceUsage::Enum usage = (hasBones) ? ResourceUsage::Dynamic : ResourceUsage::Immutable;

				// meshData.vertexBuffer = sgedev->requestResource<Buffer>();
				// const BufferDesc vbd = BufferDesc::GetDefaultVertexBuffer((uint32)meshData.vertexBufferRaw.size(), usage);
				// meshData.vertexBuffer->create(vbd, meshData.vertexBufferRaw.data());

				//// The index buffer is any.
				// if (meshData.indexBufferRaw.size() != 0) {
				//	meshData.indexBuffer = sgedev->requestResource<Buffer>();
				//	const BufferDesc ibd = BufferDesc::GetDefaultIndexBuffer((uint32)meshData.indexBufferRaw.size(), usage);
				//	meshData.indexBuffer->create(ibd, meshData.indexBufferRaw.data());
				//}

				// const bool shouldKeepCPUBuffers = (loadSets.cpuMeshData == ModelLoadSettings::KeepMeshData_Skin && hasBones) ||
				//                                  (loadSets.cpuMeshData == ModelLoadSettings::KeepMeshData_All);

				// if (shouldKeepCPUBuffers == false) {
				//	meshData.vertexBufferRaw = std::vector<char>();
				//	meshData.indexBufferRaw = std::vector<char>();
				//}
			}
		}

		// Load the nodes.
		auto jNodes = jRoot->getMember("nodes");
		if (jNodes) {
			for (size_t t = 0; t < jNodes->arrSize(); ++t) {
				auto jNode = jNodes->arrAt(t);

				const int newNodeIndex = model.makeNewNode();
				ModelNode* node = model.nodeAt(newNodeIndex);

				node->name = jNode->getMember("name")->GetString();

				if (auto jTranslation = jNode->getMember("translation")) {
					jTranslation->getNumberArrayAs<float>(node->staticLocalTransform.p.data, 3);
				}

				if (auto jRotation = jNode->getMember("rotation")) {
					jRotation->getNumberArrayAs<float>(node->staticLocalTransform.r.data, 4);
				}

				if (auto jScaling = jNode->getMember("scaling")) {
					jScaling->getNumberArrayAs<float>(node->staticLocalTransform.s.data, 3);
				}


				if (auto jlimbLength = jNode->getMember("limbLength")) {
					node->limbLength = jlimbLength->getNumberAs<float>();
				}

				

				// Read the mesh attachments.
				if (auto jMeshes = jNode->getMember("meshes")) {
					for (size_t iAttachment = 0; iAttachment < jMeshes->arrSize(); ++iAttachment) {
						const JsonValue* const jAttachmentMesh = jMeshes->arrAt(iAttachment);

						MeshAttachment attachmentMesh;
						attachmentMesh.attachedMeshIndex = jAttachmentMesh->getMember("meshIndex")->getNumberAs<int>();
						attachmentMesh.attachedMaterialIndex = jAttachmentMesh->getMember("materialIndex")->getNumberAs<int>();

						node->meshAttachments.push_back(attachmentMesh);
					}
				}

				// Nodes.
				if (const JsonValue* const jChildNodes = jNode->getMember("childNodes")) {
					node->childNodes.reserve(jChildNodes->arrSize());
					for (int iChild = 0; iChild < jChildNodes->arrSize(); ++iChild) {
						node->childNodes.push_back(jChildNodes->arrAt(iChild)->getNumberAs<int>());
					}
				}
			}
		}
		auto jRootNodeIndex = jRoot->getMember("rootNodeIndex");
		const int rootNodeIndex = jRootNodeIndex->getNumberAs<int>();
		model.setRootNodeIndex(rootNodeIndex);


		// Read the collision geometry:

		// Convex hulls.
		const JsonValue* const jStaticConvexHulls = jRoot->getMember("staticConvexHulls");
		if (jStaticConvexHulls) {
			int const numHulls = int(jStaticConvexHulls->arrSize());
			for (int t = 0; t < numHulls; ++t) {
				const int hullVertsChunkId = jStaticConvexHulls->arrAt(t)->getMember("vertsChunkId")->getNumberAs<int>();
				const int hullIndicesChunkId = jStaticConvexHulls->arrAt(t)->getMember("indicesChunkId")->getNumberAs<int>();

				std::vector<vec3f> verts;
				loadDataChunk(verts, hullVertsChunkId);

				std::vector<int> indices;
				loadDataChunk(indices, hullIndicesChunkId);

				model.m_convexHulls.emplace_back(ModelCollisionMesh(std::move(verts), std::move(indices)));
			}
		}

		// Concave hulls
		const JsonValue* const jStaticConcaveHulls = jRoot->getMember("staticConcaveHulls");
		if (jStaticConcaveHulls) {
			int const numHulls = int(jStaticConcaveHulls->arrSize());
			for (int t = 0; t < numHulls; ++t) {
				const int hullVertsChunkId = jStaticConcaveHulls->arrAt(t)->getMember("vertsChunkId")->getNumberAs<int>();
				const int hullIndicesChunkId = jStaticConcaveHulls->arrAt(t)->getMember("indicesChunkId")->getNumberAs<int>();

				std::vector<vec3f> verts;
				loadDataChunk(verts, hullVertsChunkId);

				std::vector<int> indices;
				loadDataChunk(indices, hullIndicesChunkId);

				model.m_concaveHulls.emplace_back(ModelCollisionMesh(std::move(verts), std::move(indices)));
			}
		}

		const auto jsonToTransf3d = [](const JsonValue* const j) -> transf3d {
			transf3d tr = transf3d::getIdentity();

			j->getMember("p")->getNumberArrayAs<float>(tr.p.data, 3);
			j->getMember("r")->getNumberArrayAs<float>(tr.r.data, 4);
			j->getMember("s")->getNumberArrayAs<float>(tr.s.data, 3);

			return tr;
		};

		// Collision boxes
		{
			const JsonValue* const jShapes = jRoot->getMember("collisionBoxes");
			if (jShapes) {
				for (int t = 0; t < jShapes->arrSize(); ++t) {
					const JsonValue* const jShape = jShapes->arrAt(t);
					Model_CollisionShapeBox shape;
					shape.name = jShape->getMember("name")->GetString();
					shape.transform = jsonToTransf3d(jShape->getMember("transform"));
					jShape->getMember("halfDiagonal")->getNumberArrayAs<float>(shape.halfDiagonal.data, 3);

					model.m_collisionBoxes.push_back(shape);
				}
			}
		}

		// Collision capsules
		{
			const JsonValue* const jShapes = jRoot->getMember("collisionCapsules");
			if (jShapes) {
				for (int t = 0; t < jShapes->arrSize(); ++t) {
					const JsonValue* const jShape = jShapes->arrAt(t);

					Model_CollisionShapeCapsule shape;
					shape.name = jShape->getMember("name")->GetString();
					shape.transform = jsonToTransf3d(jShape->getMember("transform"));
					shape.halfHeight = jShape->getMember("halfHeight")->getNumberAs<float>();
					shape.halfHeight = jShape->getMember("radius")->getNumberAs<float>();

					model.m_collisionCapsules.push_back(shape);
				}
			}
		}

		// Collision cylinders
		{
			const JsonValue* const jShapes = jRoot->getMember("collisionCylinders");
			if (jShapes) {
				for (int t = 0; t < jShapes->arrSize(); ++t) {
					const JsonValue* const jShape = jShapes->arrAt(t);

					Model_CollisionShapeCylinder shape;
					shape.name = jShape->getMember("name")->GetString();
					shape.transform = jsonToTransf3d(jShape->getMember("transform"));
					jShape->getMember("halfDiagonal")->getNumberArrayAs<float>(shape.halfDiagonal.data, 3);

					model.m_collisionCylinders.push_back(shape);
				}
			}
		}

		// Collision spheres
		{
			const JsonValue* const jShapes = jRoot->getMember("collisionSpheres");
			if (jShapes) {
				for (int t = 0; t < jShapes->arrSize(); ++t) {
					const JsonValue* const jShape = jShapes->arrAt(t);

					Model_CollisionShapeSphere shape;
					shape.name = jShape->getMember("name")->GetString();
					shape.transform = jsonToTransf3d(jShape->getMember("transform"));
					shape.radius = jShape->getMember("radius")->getNumberAs<float>();

					model.m_collisionSpheres.push_back(shape);
				}
			}
		}
	} catch (const ModelParseExcept& UNUSED(except)) {
		// SGE_DEBUG_ERR("%s: Failed with exception:\n", __func__);
		// SGE_DEBUG_ERR(except.what());

		return false;
	} catch (...) {
		// SGE_DEBUG_ERR("%s: Failed with unknown exception:\n", __func__);
		return false;
	}

	return true;
}

} // namespace sge
