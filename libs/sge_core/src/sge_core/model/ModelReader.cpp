#include "sge_utils/utils/vector_map.h"
#include <sge_utils/utils/FileStream.h>
#include <sge_utils/utils/json.h>
#include <stdexcept>

#include "Model.h"
#include "ModelReader.h"

namespace sge {

namespace Model {

	struct ModelParseExcept : public std::logic_error {
		ModelParseExcept(const char* msg)
		    : std::logic_error(msg) {}
	};

	template <typename T>
	void ModelReader::LoadDataChunk(std::vector<T>& resultBuffer, const int chunkId) {
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
			throw ModelParseExcept("ChunkSize / sizeof(T) != 0!");
		}

		const size_t numElems = chunkDesc->sizeBytes / sizeof(T);
		resultBuffer.resize(numElems);

		const size_t firstChunkDataLocation = irs->pointerOffset();
		irs->seek(SeekOrigin::Current, chunkDesc->byteOffset);
		irs->read(resultBuffer.data(), chunkDesc->sizeBytes);

		// Seek back to the 1st chunk.
		irs->seek(SeekOrigin::Begining, firstChunkDataLocation);
	}

	void ModelReader::LoadDataChunkRaw(void* const ptr, const size_t ptrExpectedSize, const int chunkId) {
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

	bool ModelReader::LoadParamBlock(const JsonValue* jParamBlock, ParameterBlock& paramBlock) {
		const size_t numParameters = jParamBlock->arrSize();
		for (size_t t = 0; t < numParameters; ++t) {
			auto jParam = jParamBlock->arrAt(t);

			const char* const name = jParam->getMember("name")->GetString();
			const ParameterType::Enum paramType = ParameterType::FromString(jParam->getMember("type")->GetString());

			// Some validation.
			if (paramType == ParameterType::FromStringError) {
				sgeAssert(false && "Failed to convert string to ParameterType!");
				throw ModelParseExcept("Unknown parameter type!"); // [TODO} Concider just skipping that parameter.
			}

			// Resolve the static value
			auto jStaticValue = jParam->getMember("staticValue");
			float fStaticValue[4];
			const void* pStaticValue = nullptr;
			if (paramType == ParameterType::Float) {
				fStaticValue[0] = jStaticValue->getNumberAs<float>();
				pStaticValue = fStaticValue;
			} else if (paramType == ParameterType::Float2 || paramType == ParameterType::Float3 || paramType == ParameterType::Float4 ||
			           paramType == ParameterType::Quaternion) {
				for (size_t i = 0; i < jStaticValue->arrSize(); ++i) {
					fStaticValue[i] = jStaticValue->arrAt(i)->getNumberAs<float>();
				}
				pStaticValue = fStaticValue;
			} else if (paramType == ParameterType::String) {
				pStaticValue = jStaticValue->GetString();
			} else {
				sgeAssert(false && "Failed to load parameter static value!\n");
				throw ModelParseExcept("Unsupported parameter type!"); // [TODO] Concider just skipping that parameter.
			}

			// Create the parameter in the param block and initialize it.
			Parameter* const param = paramBlock.FindParameter(name, paramType, pStaticValue);

			// Load curves if any.
			auto jCurves = jParam->getMember("curves");
			if (jCurves != nullptr) {
				for (size_t i = 0; i < jCurves->arrSize(); ++i) {
					auto jCurve = jCurves->arrAt(i);

					const char* const curveName = jCurve->getMember("name")->GetString();
					const int keysChunkId = jCurve->getMember("keyframesChunkId")->getNumberAs<int>();
					const int valuesChunkId = jCurve->getMember("valuesChunkId")->getNumberAs<int>();

					// Create the curve and load the keyframes and the values.
					if (param->CreateCurve(curveName)) {
						ParameterCurve& curve = *param->GetCurve(curveName);

						curve.type = param->GetType();
						LoadDataChunk(curve.keys, keysChunkId);
						LoadDataChunk(curve.data, valuesChunkId);

						sgeAssert(curve.debug_VerifyData());
					} else {
						sgeAssert(false);
					}
				}
			}
		}

		return true;
	}

	PrimitiveTopology::Enum ModelReader::PrimitiveTolologyFromString(const char* str) {
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

	UniformType::Enum ModelReader::UniformTypeFromString(const char* str) {
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

	bool ModelReader::Load(const LoadSettings loadSets, SGEDevice* sgedev, IReadStream* const iReadStream, Model& model) {
		try {
			dataChunksDesc.clear();
			irs = iReadStream;

			model = Model();
			model.m_loadSets = loadSets;

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
			auto jAnimations = jRoot->getMember("animations");
			if (jAnimations) {
				for (size_t t = 0; t < jAnimations->arrSize(); ++t) {
					auto jAnimation = jAnimations->arrAt(t);

					AnimationInfo animationInfo;

					animationInfo.curveName = std::string(jAnimation->getMember("curve")->GetString());
					animationInfo.startTime = jAnimation->getMember("timeOffset")->getNumberAs<float>();
					animationInfo.duration = jAnimation->getMember("duration")->getNumberAs<float>();

					model.m_animations.push_back(animationInfo);
				}
			}

			// Load the materials.
			auto jMaterials = jRoot->getMember("materials");
			if (jMaterials) {
				model.m_materials.reserve(jMaterials->arrSize());
				for (size_t t = 0; t < jMaterials->arrSize(); ++t) {
					const JsonValue* const jMaterial = jMaterials->arrAt(t);

					model.m_materials.push_back(model.m_containerMaterial.new_element());
					Material* const mtl = model.m_materials.back();

					mtl->id = jMaterial->getMember("id")->getNumberAs<int>();
					mtl->name = jMaterial->getMember("name")->GetString();
					LoadParamBlock(jMaterial->getMember("paramBlock"), mtl->paramBlock);
				}
			}

			// Load the MeshData.
			vector_map<Bone*, int> bone2nodeResolve;

			auto jMeshesData = jRoot->getMember("meshesData");
			if (jMeshesData) {
				model.m_meshesData.reserve(jMeshesData->arrSize());
				for (size_t t = 0; t < jMeshesData->arrSize(); ++t) {
					model.m_meshesData.push_back(model.m_containerMeshData.new_element());
					MeshData& meshData = *model.m_meshesData.back();

					auto jMeshData = jMeshesData->arrAt(t);

					const int vertexDataChunkID = jMeshData->getMember("vertexDataChunkId")->getNumberAs<int>();
					const JsonValue* jIndexBufferChunkID = jMeshData->getMember("indexDataChunkId");

					LoadDataChunk(meshData.vertexBufferRaw, vertexDataChunkID);
					const int indexDataChunkID = jIndexBufferChunkID ? jIndexBufferChunkID->getNumberAs<int>() : -1;
					if (indexDataChunkID > 0)
						LoadDataChunk(meshData.indexBufferRaw, indexDataChunkID);

					// Load the described meshes.

					// True if at least one mesh have bones. BTW currently only all Skinned meshes are stored into separate meshDatas.
					bool hasBones = false;

					auto jMeshes = jMeshData->getMember("meshes");

					meshData.meshes.reserve(jMeshes->arrSize());

					for (size_t s = 0; s < jMeshes->arrSize(); ++s) {
						meshData.meshes.push_back(model.m_containerMesh.new_element());
						Mesh& mesh = *meshData.meshes.back();

						auto jMesh = jMeshes->arrAt(s);

						mesh.pMeshData = &meshData;
						mesh.id = jMesh->getMember("id")->getNumberAs<int>();
						mesh.name = jMesh->getMember("name")->GetString();
						mesh.primTopo = PrimitiveTolologyFromString(jMesh->getMember("primitiveTopology")->GetString());
						mesh.vbByteOffset = jMesh->getMember("vbByteOffset")->getNumberAs<uint32>();
						mesh.numElements = jMesh->getMember("numElements")->getNumberAs<uint32>();
						mesh.numVertices = jMesh->getMember("numVertices")->getNumberAs<uint32>();

						// Load the index buffer. Note that there may be no index buffer.
						if (jMesh->getMember("ibByteOffset") && jMesh->getMember("ibFormat")) {
							mesh.ibByteOffset = jMesh->getMember("ibByteOffset")->getNumberAs<uint32>();
							mesh.ibFmt = UniformTypeFromString(jMesh->getMember("ibFormat")->GetString());
						}

						// The AABB of the mesh.
						jMesh->getMember("AABoxMin")->getNumberArrayAs<float>(mesh.aabox.min.data, 3);
						jMesh->getMember("AABoxMax")->getNumberArrayAs<float>(mesh.aabox.max.data, 3);

						// The vertex declaration.
						auto jVertexDecl = jMesh->getMember("vertexDecl");
						for (size_t iDecl = 0; iDecl < jVertexDecl->arrSize(); ++iDecl) {
							auto jDecl = jVertexDecl->arrAt(iDecl);

							VertexDecl decl;
							decl.bufferSlot = 0;
							decl.semantic = jDecl->getMember("semantic")->GetString();
							decl.byteOffset = jDecl->getMember("byteOffset")->getNumberAs<int>();
							decl.format = UniformTypeFromString(jDecl->getMember("format")->GetString());

							mesh.vertexDecl.push_back(decl);

							// Cache some commonly used semantics offsets.
							if (decl.semantic == "a_position") {
								mesh.vbPositionOffsetBytes = (int)decl.byteOffset;
							} else if (decl.semantic == "a_normal") {
								mesh.vbNormalOffsetBytes = (int)decl.byteOffset;
							} else if (decl.semantic == "a_uv") {
								mesh.vbUVOffsetBytes = (int)decl.byteOffset;
							}
						}

						// Bake the vertex stride.
						mesh.stride = int(mesh.vertexDecl.back().byteOffset) + UniformType::GetSizeBytes(mesh.vertexDecl.back().format);

						// The attached material (if any).
						if (jMesh->getMember("material_id")) {
							const int material_id = jMesh->getMember("material_id")->getNumberAs<int>();
							mesh.pMaterial = model.FindMaterial(material_id);
							sgeAssert(mesh.pMaterial);
						}

						// The bones.
						auto jBones = jMesh->getMember("bones");
						if (jBones) {
							hasBones = true;

							mesh.bones.resize(jBones->arrSize());
							for (size_t iBone = 0; iBone < jBones->arrSize(); ++iBone) {
								auto jBone = jBones->arrAt(iBone);
								Bone& bone = mesh.bones[iBone];

								// Becase nodes are loded before meshes, we must do that gymnastic.
								bone2nodeResolve[&bone] = jBone->getMember("node_id")->getNumberAs<int>();
								LoadDataChunkRaw(&bone.offsetMatrix, sizeof(bone.offsetMatrix),
								                 jBone->getMember("offsetMatrixChunkId")->getNumberAs<int>());
							}
						}
					}

					// Skinned meshes must be stored into separate meshData buffers
					if (hasBones && meshData.meshes.size() != 1) {
						sgeAssert(false);
						throw ModelParseExcept("Mesh data isn't stored into a separate mesh data!");
					}

					// Finally Create the GPU resources.
					const ResourceUsage::Enum usage = (hasBones) ? ResourceUsage::Dynamic : ResourceUsage::Immutable;

					meshData.vertexBuffer = sgedev->requestResource<Buffer>();
					const BufferDesc vbd = BufferDesc::GetDefaultVertexBuffer((uint32)meshData.vertexBufferRaw.size(), usage);
					meshData.vertexBuffer->create(vbd, meshData.vertexBufferRaw.data());

					// The index buffer is any.
					if (meshData.indexBufferRaw.size() != 0) {
						meshData.indexBuffer = sgedev->requestResource<Buffer>();
						const BufferDesc ibd = BufferDesc::GetDefaultIndexBuffer((uint32)meshData.indexBufferRaw.size(), usage);
						meshData.indexBuffer->create(ibd, meshData.indexBufferRaw.data());
					}

					const bool shouldKeepCPUBuffers = (loadSets.cpuMeshData == LoadSettings::KeepMeshData_Skin && hasBones) ||
					                                  (loadSets.cpuMeshData == LoadSettings::KeepMeshData_All);

					if (shouldKeepCPUBuffers == false) {
						meshData.vertexBufferRaw = std::vector<char>();
						meshData.indexBufferRaw = std::vector<char>();
					}
				}
			}

			// Load the nodes.
			auto jNodes = jRoot->getMember("nodes");
			if (jNodes) {
				model.m_nodes.reserve(jNodes->arrSize());
				for (size_t t = 0; t < jNodes->arrSize(); ++t) {
					auto jNode = jNodes->arrAt(t);
					auto jParamBlock = jNode->getMember("paramBlock");

					Node* node = model.m_containerNode.new_element();

					node->id = jNode->getMember("id")->getNumberAs<int>();
					node->name = jNode->getMember("name")->GetString();
					LoadParamBlock(jParamBlock, node->paramBlock);

					// Read the mesh attachments.
					auto jMeshes = jNode->getMember("meshes");
					if (jMeshes) {
						for (size_t iMesh = 0; iMesh < jMeshes->arrSize(); ++iMesh) {
							const JsonValue* const jAttachmentMesh = jMeshes->arrAt(iMesh);

							const JsonValue* const jMeshId = jAttachmentMesh->getMember("mesh_id");
							const JsonValue* const jMaterialId = jAttachmentMesh->getMember("material_id");

							const int meshId = jMeshId->getNumberAs<int>();
							MeshAttachment attachmentMesh;

							attachmentMesh.mesh = model.FindMesh(meshId);
							if (jMaterialId) {
								const int materialId = jMaterialId->getNumberAs<int>();
								attachmentMesh.material = model.FindMaterial(materialId);
							}

							if (attachmentMesh.mesh != nullptr) {
								node->meshAttachments.push_back(attachmentMesh);
							} else {
								sgeAssert(false); // Should never happen.
							}
						}
					}

					//
					model.m_nodes.push_back(node);
				}
			}

			// Resolve Bone Node pointers.
			{
				for (const auto& pair : bone2nodeResolve) {
					Bone* const bone = pair.key();
					Node* const node = model.FindNode(pair.value());
					sgeAssert(bone != nullptr && node != nullptr);

					bone->node = node;
				}
			}

			// Resolve Node hierarchy
			auto jHierarchy = jRoot->getMember("nodeHierarchy");
			if (jHierarchy) {
				// the 1st element of that arrays is the name of the root node.
				const int rootNodeId = jHierarchy->arrAt(0)->getNumberAs<int>();
				model.m_rootNode = model.FindNode(rootNodeId);

				for (size_t iNode = 1; iNode < jHierarchy->arrSize(); iNode += 2) {
					const int nodeId = jHierarchy->arrAt(iNode)->getNumberAs<int>();
					Node* const node = model.FindNode(nodeId);

					// Add the child nodes.
					const JsonValue* const jChilds = jHierarchy->arrAt(iNode + 1);
					for (size_t iChild = 0; iChild < jChilds->arrSize(); ++iChild) {
						const int childNodeId = jChilds->arrAt(iChild)->getNumberAs<int>();
						node->childNodes.push_back(model.FindNode(childNodeId));
					}
				}
			}

			// Read the collision geometry:

			// Convex hulls.
			const JsonValue* const jStaticConvexHulls = jRoot->getMember("staticConvexHulls");
			if (jStaticConvexHulls) {
				int const numHulls = int(jStaticConvexHulls->arrSize());
				for (int t = 0; t < numHulls; ++t) {
					const int hullVertsChunkId = jStaticConvexHulls->arrAt(t)->getMember("vertsChunkId")->getNumberAs<int>();
					const int hullIndicesChunkId = jStaticConvexHulls->arrAt(t)->getMember("indicesChunkId")->getNumberAs<int>();

					std::vector<vec3f> verts;
					LoadDataChunk(verts, hullVertsChunkId);

					std::vector<int> indices;
					LoadDataChunk(indices, hullIndicesChunkId);

					model.m_convexHulls.emplace_back(CollisionMesh(std::move(verts), std::move(indices)));
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
					LoadDataChunk(verts, hullVertsChunkId);

					std::vector<int> indices;
					LoadDataChunk(indices, hullIndicesChunkId);

					model.m_concaveHulls.emplace_back(CollisionMesh(std::move(verts), std::move(indices)));
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
						CollisionShapeBox shape;
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

						CollisionShapeCapsule shape;
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

						CollisionShapeCylinder shape;
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

						CollisionShapeSphere shape;
						shape.name = jShape->getMember("name")->GetString();
						shape.transform = jsonToTransf3d(jShape->getMember("transform"));
						shape.radius = jShape->getMember("radius")->getNumberAs<float>();

						model.m_collisionSpheres.push_back(shape);
					}
				}
			}
		} catch (const ModelParseExcept& except) {
			((void)except);
			// SGE_DEBUG_ERR("%s: Failed with exception:\n", __func__);
			// SGE_DEBUG_ERR(except.what());

			return false;
		} catch (...) {
			// SGE_DEBUG_ERR("%s: Failed with unknown exception:\n", __func__);
			return false;
		}

		return true;
	} // namespace Model

} // namespace Model

} // namespace sge
