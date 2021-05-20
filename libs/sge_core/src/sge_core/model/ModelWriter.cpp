#include <sge_utils/utils/FileStream.h>
#include <sge_utils/utils/json.h>

#include "Model.h"
#include "ModelWriter.h"

namespace sge {

int ModelWriter::NewChunkFromPtr(const void* const ptr, const size_t sizeBytes) {
	const int newChunkId = (dataChunks.size() == 0) ? 0 : dataChunks.back().id + 1;
	dataChunks.emplace_back(DataChunk(newChunkId, ptr, sizeBytes));
	return newChunkId;
}

JsonValue* ModelWriter::WriteParamBlock(const ParameterBlock& paramBlock) {
	JsonValue* jParamBlock = jvb(JID_ARRAY);

	for (const auto& itr : paramBlock) {
		const std::string& paramName = itr.first;
		const Parameter& param = itr.second;

		JsonValue* jParam = jParamBlock->arrPush(jvb(JID_MAP));
		jParam->setMember("name", jvb(paramName));
		jParam->setMember("type", jvb(ParameterType::info(param.GetType()).name));

		switch (param.GetType()) {
			case ParameterType::Float: {
				jParam->setMember("staticValue", jvb(*(const float*)param.GetStaticValue()));
			} break;
			case ParameterType::Float2: {
				const float* const f = (const float*)param.GetStaticValue();
				jParam->setMember("staticValue", jvb(f, 2));
			} break;
			case ParameterType::Float3: {
				const float* const f = (const float*)param.GetStaticValue();
				jParam->setMember("staticValue", jvb(f, 3));
			} break;
			case ParameterType::Float4:
			case ParameterType::Quaternion: {
				const float* f = (const float*)param.GetStaticValue();
				jParam->setMember("staticValue", jvb(f, 4));
			} break;
			case ParameterType::String: {
				jParam->setMember("staticValue", jvb((const char*)param.GetStaticValue()));
			} break;
			default: {
				sgeAssert(false && "Unknown parameter type");
			}
		}

		// Do not export an empty array for the curves.
		if (param.GetNumCurves() > 0) {
			JsonValue* jCurves = jParam->setMember("curves", jvb(JID_ARRAY));
			for (int iCurve = 0; iCurve < param.GetNumCurves(); ++iCurve) {
				const ParameterCurve* curve = param.GetCurve(iCurve);

				sgeAssert(curve->debug_VerifyData());

				// Duplicate the curve keyframes and values into the data chunks.
				int chunkKeysId = NewChunkFromStdVector(curve->keys);
				int chunkValuesId = NewChunkFromStdVector(curve->data);

				// Create the json for the curve.
				JsonValue* jCurve = jCurves->arrPush(jvb(JID_MAP));
				jCurve->setMember("name", jvb(param.GetCurveName(iCurve)));
				jCurve->setMember("keyframesChunkId", jvb(chunkKeysId));
				jCurve->setMember("valuesChunkId", jvb(chunkValuesId));
			}
		}
	}

	return jParamBlock;
}

void ModelWriter::GenerateAnimations() {
	if (model->m_animations.size()) {
		auto jAnimations = root->setMember("animations", jvb(JID_ARRAY_BEGIN));
		for (auto& animation : model->m_animations) {
			auto jAnim = jvb(JID_MAP_BEGIN);

			jAnim->setMember("curve", jvb(animation.curveName));
			jAnim->setMember("timeOffset", jvb(animation.startTime)); // will be used in the future
			jAnim->setMember("duration", jvb(animation.duration));

			jAnimations->arrPush(jAnim);
		}
	}

	return;
}

void ModelWriter::GenerateNodeHierarchy() {
	{
		JsonValue* jHierarchy = root->setMember("nodeHierarchy", jvb(JID_ARRAY));

		// The 1st elements is the root node ...
		jHierarchy->arrPush(jvb(model->m_rootNode->id));

		// ...and after that is the hierarchy desc
		// [<RootNodeName>, <NodeName0>, [Childs] ...]
		for (const auto& node : model->m_nodes) {
			// Skip the node if there are not childs.
			if (node->childNodes.empty()) {
				continue;
			}

			// Add the parent node.
			jHierarchy->arrPush(jvb(node->id));

			// Add the child nodes.
			JsonValue* jChildNodes = jHierarchy->arrPush(jvb(JID_ARRAY));
			for (const auto& childNode : node->childNodes) {
				jChildNodes->arrPush(jvb(childNode->id));
			}
		}
	}

	// The same as the code above, but using the names, this stucture should only be used to simplify the
	// debugging and for nothing else.
	{
		JsonValue* jHierarchy = root->setMember("debug_nodeHierarchyNames", jvb(JID_ARRAY));

		// The 1st elements is the name of the root node ...
		jHierarchy->arrPush(jvb(model->m_rootNode->name.c_str()));

		// ...and after that is the hierarchy desc
		// [<RootNodeName>, <NodeName0>, [Childs] ...]
		for (const auto& node : model->m_nodes) {
			// skip the node if there are not childs.
			if (node->childNodes.empty()) {
				continue;
			}

			// Add the node and than add the array with child names.
			jHierarchy->arrPush(jvb(node->name.c_str()));

			JsonValue* jChildNodes = jHierarchy->arrPush(jvb(JID_ARRAY));
			for (const auto& childNode : node->childNodes) {
				jChildNodes->arrPush(jvb(childNode->name.c_str()));
			}
		}
	}

	return;
}

void ModelWriter::GenerateNodes() {
	JsonValue* jNodes = root->setMember("nodes", jvb(JID_ARRAY));

	for (const auto& node : model->m_nodes) {
		JsonValue* jNode = jNodes->arrPush(jvb(JID_MAP));

		// Node id and name.
		jNode->setMember("id", jvb(node->id));
		jNode->setMember("name", jvb(node->name.c_str()));

		// Parameter block.
		jNode->setMember("paramBlock", WriteParamBlock(node->paramBlock));

		// Attached meshes.
		if (!node->meshAttachments.empty()) {
			auto jMeshes = jNode->setMember("meshes", jvb(JID_ARRAY));

			for (const Model::MeshAttachment attachmentMesh : node->meshAttachments) {
				JsonValue* const jAttachmentMesh = jvb(JID_MAP);
				jAttachmentMesh->setMember("mesh_id", jvb(attachmentMesh.mesh->id));
				if (attachmentMesh.material != nullptr) {
					jAttachmentMesh->setMember("material_id", jvb(attachmentMesh.material->id));
				}

				jMeshes->arrPush(jAttachmentMesh);
			}
		}
	}
}

void ModelWriter::GenerateMaterials() {
	JsonValue* const jMaterials = root->setMember("materials", jvb(JID_ARRAY));

	for (auto& mtl : model->m_materials) {
		JsonValue* const jMaterial = jMaterials->arrPush(jvb(JID_MAP));

		jMaterial->setMember("id", jvb(mtl->id));
		jMaterial->setMember("name", jvb(mtl->name));
		jMaterial->setMember("paramBlock", WriteParamBlock(mtl->paramBlock));
	}

	return;
}

void ModelWriter::GenerateMeshesData() {
	auto UnformType2String = [](const UniformType::Enum ut) -> const char* {
		switch (ut) {
			case UniformType::Uint16:
				return "uint16";
			case UniformType::Uint:
				return "uint32";
			case UniformType::Float2:
				return "float2";
			case UniformType::Float3:
				return "float3";
			case UniformType::Float4:
				return "float4";
			case UniformType::Int4:
				return "int4";
		}

		sgeAssert(false);
		return nullptr;
	};

	auto PrimitiveTopology2String = [](const PrimitiveTopology::Enum ut) -> const char* {
		switch (ut) {
			case PrimitiveTopology::TriangleList:
				return "TriangleList";
		}

		sgeAssert(false);
		return nullptr;
	};

	JsonValue* jMeshesData = root->setMember("meshesData", jvb(JID_ARRAY));

	for (auto const& meshData : model->m_meshesData) {
		JsonValue* jMeshData = jMeshesData->arrPush(jvb(JID_MAP));

		// Write the vertex/index buffers chunks.
		if (meshData->vertexBufferRaw.size()) {
			const int vertexBufferChunkId = NewChunkFromStdVector(meshData->vertexBufferRaw);
			jMeshData->setMember("vertexDataChunkId", jvb(vertexBufferChunkId));
		}

		if (meshData->indexBufferRaw.size()) {
			const int indexBufferChunkId = NewChunkFromStdVector(meshData->indexBufferRaw);
			jMeshData->setMember("indexDataChunkId", jvb(indexBufferChunkId));
		}

		// Write the meshes that are described in this mesh data.
		auto jMeshes = jMeshData->setMember("meshes", jvb(JID_ARRAY));
		for (int t = 0; t < meshData->meshes.size(); ++t) {
			JsonValue* jMesh = jMeshes->arrPush(jvb(JID_MAP));

			// Draw call settings.
			const Model::Mesh* const mesh = meshData->meshes[t];
			jMesh->setMember("type", jvb("mesh"));
			jMesh->setMember("id", jvb(mesh->id));
			jMesh->setMember("name", jvb(mesh->name));
			jMesh->setMember("primitiveTopology", jvb(PrimitiveTopology2String(mesh->primTopo)));
			jMesh->setMember("vbByteOffset", jvb((int)mesh->vbByteOffset));

			jMesh->setMember("numElements", jvb((int)mesh->numElements));
			jMesh->setMember("numVertices", jvb((int)mesh->numVertices));

			if (mesh->ibFmt != UniformType::Unknown) {
				jMesh->setMember("ibByteOffset", jvb((int)mesh->ibByteOffset));
				jMesh->setMember("ibFormat", jvb(UnformType2String(mesh->ibFmt)));
			}

			// Material.
			if (mesh->pMaterial != NULL) {
				jMesh->setMember("material_id", jvb(mesh->pMaterial->id));
			}

			// Vertex declaration.
			JsonValue* jVertexDecl = jMesh->setMember("vertexDecl", jvb(JID_ARRAY));
			for (auto& v : mesh->vertexDecl) {
				JsonValue* jDecl = jVertexDecl->arrPush(jvb(JID_MAP));
				jDecl->setMember("semantic", jvb(v.semantic.c_str()));
				jDecl->setMember("byteOffset", jvb((int)v.byteOffset));
				jDecl->setMember("format", jvb(UnformType2String(v.format)));
			}

			// Bones(if any).
			if (mesh->bones.size() != 0) {
				auto jBones = jMesh->setMember("bones", jvb(JID_ARRAY_BEGIN));
				for (const auto& bone : mesh->bones) {
					const int boneOffsetChunkId = NewChunkFromPtr(&bone.offsetMatrix, sizeof(bone.offsetMatrix));

					auto jBone = jBones->arrPush(jvb(JID_MAP_BEGIN));
					jBone->setMember("node_id", jvb(bone.node->id));
					jBone->setMember("debug_node_name", jvb(bone.node->name));
					jBone->setMember("offsetMatrixChunkId", jvb(boneOffsetChunkId));
				}
			}

			// Axis aligned bounding box.
			jMesh->setMember("AABoxMin", jvb((float*)&mesh->aabox.min.x, 3));
			jMesh->setMember("AABoxMax", jvb((float*)&mesh->aabox.max.x, 3));
		}
	}

	return;
}

void ModelWriter::GenerateCollisionData() {
	const auto transf3dToJson = [&](const transf3d& tr) -> JsonValue* {
		JsonValue* j = jvb(JID_MAP);
		j->setMember("p", jvb(tr.p.data, 3));
		j->setMember("r", jvb(tr.r.data, 4));
		j->setMember("s", jvb(tr.s.data, 3));

		return j;
	};

	// Write the convex hull that are evaluated at the static moment.
	if (model->m_convexHulls.size()) {
		JsonValue* jStaticConvecHulls = root->setMember("staticConvexHulls", jvb(JID_ARRAY));

		for (int t = 0; t < model->m_convexHulls.size(); ++t) {
			JsonValue* const jHull = jStaticConvecHulls->arrPush(jvb(JID_MAP));
			const int hullVertsChunkId = NewChunkFromStdVector(model->m_convexHulls[t].vertices);
			const int hullIndsChunkId = NewChunkFromStdVector(model->m_convexHulls[t].indices);
			jHull->setMember("vertsChunkId", jvb(hullVertsChunkId));
			jHull->setMember("indicesChunkId", jvb(hullIndsChunkId));
		}
	}

	// Write the conave hull that are evaluated at the static moment.
	if (model->m_concaveHulls.size()) {
		JsonValue* jStaticConvecHulls = root->setMember("staticConcaveHulls", jvb(JID_ARRAY));

		for (int t = 0; t < model->m_concaveHulls.size(); ++t) {
			JsonValue* const jHull = jStaticConvecHulls->arrPush(jvb(JID_MAP));
			const int hullVertsChunkId = NewChunkFromStdVector(model->m_concaveHulls[t].vertices);
			const int hullIndsChunkId = NewChunkFromStdVector(model->m_concaveHulls[t].indices);
			jHull->setMember("vertsChunkId", jvb(hullVertsChunkId));
			jHull->setMember("indicesChunkId", jvb(hullIndsChunkId));
		}
	}

	// Write the boxes
	if (model->m_collisionBoxes.size()) {
		JsonValue* const jShapes = root->setMember("collisionBoxes", jvb(JID_ARRAY));
		for (int t = 0; t < model->m_collisionBoxes.size(); ++t) {
			const auto& shape = model->m_collisionBoxes[t];
			JsonValue* const jShape = jShapes->arrPush(jvb(JID_MAP));

			jShape->setMember("name", jvb(shape.name.c_str()));
			jShape->setMember("transform", transf3dToJson(shape.transform));
			jShape->setMember("halfDiagonal", jvb(shape.halfDiagonal.data, 3));
		}
	}

	// Write the capsules
	if (model->m_collisionCapsules.size()) {
		JsonValue* const jShapes = root->setMember("collisionCapsules", jvb(JID_ARRAY));
		for (int t = 0; t < model->m_collisionCapsules.size(); ++t) {
			const auto& shape = model->m_collisionCapsules[t];
			JsonValue* const jShape = jShapes->arrPush(jvb(JID_MAP));

			jShape->setMember("name", jvb(shape.name.c_str()));
			jShape->setMember("transform", transf3dToJson(shape.transform));
			jShape->setMember("halfHeight", jvb(shape.halfHeight));
			jShape->setMember("radius", jvb(shape.radius));
		}
	}

	// Write the cylinders
	if (model->m_collisionCylinders.size()) {
		JsonValue* const jShapes = root->setMember("collisionCylinders", jvb(JID_ARRAY));
		for (int t = 0; t < model->m_collisionCylinders.size(); ++t) {
			const auto& shape = model->m_collisionCylinders[t];
			JsonValue* const jShape = jShapes->arrPush(jvb(JID_MAP));

			jShape->setMember("name", jvb(shape.name.c_str()));
			jShape->setMember("transform", transf3dToJson(shape.transform));
			jShape->setMember("halfDiagonal", jvb(shape.halfDiagonal.data, 3));
		}
	}

	// Write the spheres.
	if (model->m_collisionSpheres.size()) {
		JsonValue* const jShapes = root->setMember("collisionSpheres", jvb(JID_ARRAY));
		for (int t = 0; t < model->m_collisionSpheres.size(); ++t) {
			const auto& shape = model->m_collisionSpheres[t];
			JsonValue* const jShape = jShapes->arrPush(jvb(JID_MAP));

			jShape->setMember("name", jvb(shape.name.c_str()));
			jShape->setMember("transform", transf3dToJson(shape.transform));
			jShape->setMember("radius", jvb(shape.radius));
		}
	}
}

bool ModelWriter::write(const Model::Model& modelToWrite, IWriteStream* iws) {
	if (iws == nullptr) {
		return false;
	}

	root = jvb(JID_MAP);
	this->model = &modelToWrite;

	GenerateAnimations();
	GenerateNodeHierarchy();
	GenerateNodes();
	GenerateMaterials();
	GenerateMeshesData();
	GenerateCollisionData();

	// Generate the json that describes the data chunks.
	JsonValue* const jDataChunkDesc = root->setMember("dataChunksDesc", jvb(JID_ARRAY));

	size_t offsetBytesAccum = 0;
	for (const DataChunk& chunk : dataChunks) {
		// Output layout [ ... id, offset, size, ... ]
		jDataChunkDesc->arrPush(jvb(chunk.id));
		jDataChunkDesc->arrPush(jvb((int)offsetBytesAccum));
		jDataChunkDesc->arrPush(jvb((int)chunk.sizeBytes));

		offsetBytesAccum += chunk.sizeBytes;
	}

	// Now write the json header.
	JsonWriter jsonWriter;
	jsonWriter.write(iws, root, true);

	// And now write the data chunks data.
	for (const DataChunk& chunk : dataChunks) {
		iws->write((char*)chunk.data, chunk.sizeBytes);
	}

	return true;
}

bool ModelWriter::write(const Model::Model& modelToWrite, const char* const filename) {
	if (filename == nullptr) {
		return false;
	}

	FileWriteStream fws;
	if (!fws.open(filename)) {
		sgeAssert(false);
		return false;
	}

	return write(modelToWrite, &fws);
}
} // namespace sge
