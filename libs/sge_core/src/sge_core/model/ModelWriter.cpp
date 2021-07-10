#include "ModelWriter.h"
#include "Model.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/json.h"
#include "sge_utils/utils/range_loop.h"

namespace sge {

int ModelWriter::newDataChunkFromPtr(const void* const ptr, const size_t sizeBytes) {
	const int newChunkId = (dataChunks.size() == 0) ? 0 : dataChunks.back().id + 1;

	dataChunks.emplace_back(DataChunk(newChunkId, ptr, sizeBytes));
	return newChunkId;
}

char* ModelWriter::newDataChunkWithSize(size_t sizeBytes, int& outChunkId) {
	const int newChunkId = (dataChunks.size() == 0) ? 0 : dataChunks.back().id + 1;

	std::vector<char> memory(sizeBytes);
	DataChunk chunk;
	chunk.data = memory.data();
	chunk.id = newChunkId;
	chunk.sizeBytes = sizeBytes;

	dataChunks.push_back(chunk);

	// [MODERL_WRITER_MOVE_ASSUME]
	m_dynamicallyAlocatedPointersToDelete.emplace_back(std::move(memory));
	// memory is now empty becuase of the move.
	sgeAssert(memory.empty());
	sgeAssert(m_dynamicallyAlocatedPointersToDelete.back().data() == chunk.data);

	outChunkId = newChunkId;
	return (char*)chunk.data;
}

JsonValue* ModelWriter::generateKeyFrames(const KeyFrames& keyfames) {
	JsonValue* jKeyFrames = jvb(JID_MAP);

	if (!keyfames.positionKeyFrames.empty()) {
		const int numKeyFrames = int(keyfames.positionKeyFrames.size());
		const size_t chunkSizeBytes = numKeyFrames * (sizeof(float) + sizeof(vec3f));

		int chunkId = -1;
		char* chunkData = newDataChunkWithSize(chunkSizeBytes, chunkId);

		for (const auto& itr : keyfames.positionKeyFrames) {
			*(float*)(chunkData) = itr.first;
			chunkData += sizeof(float);

			*(vec3f*)(chunkData) = itr.second;
			chunkData += sizeof(itr.second);
		}

		jKeyFrames->setMember("positionKeyFrames_chunkId", jvb(chunkId));
	}

	if (!keyfames.rotationKeyFrames.empty()) {
		const int numKeyFrames = int(keyfames.rotationKeyFrames.size());
		const size_t chunkSizeBytes = numKeyFrames * (sizeof(float) + sizeof(quatf));

		int chunkId = -1;
		char* chunkData = newDataChunkWithSize(chunkSizeBytes, chunkId);

		for (const auto& itr : keyfames.rotationKeyFrames) {
			*(float*)(chunkData) = itr.first;
			chunkData += sizeof(float);

			*(quatf*)(chunkData) = itr.second;
			chunkData += sizeof(itr.second);
		}

		jKeyFrames->setMember("rotationKeyFrames_chunkId", jvb(chunkId));
	}

	if (!keyfames.scalingKeyFrames.empty()) {
		const int numKeyFrames = int(keyfames.scalingKeyFrames.size());
		const size_t chunkSizeBytes = numKeyFrames * (sizeof(float) + sizeof(vec3f));

		int chunkId = -1;
		char* chunkData = newDataChunkWithSize(chunkSizeBytes, chunkId);

		for (const auto& itr : keyfames.scalingKeyFrames) {
			*(float*)(chunkData) = itr.first;
			chunkData += sizeof(float);

			*(vec3f*)(chunkData) = itr.second;
			chunkData += sizeof(itr.second);
		}

		jKeyFrames->setMember("scalingKeyFrames_chunkId", jvb(chunkId));
	}


	return jKeyFrames;
}

void ModelWriter::writeAnimations() {
	if (model->numAnimations() == 0) {
		return;
	}

	auto jAnimations = root->setMember("animations", jvb(JID_ARRAY_BEGIN));

	for (int iAnim : range_int(model->numAnimations())) {
		const ModelAnimation& animation = *model->animationAt(iAnim);

		auto jAnim = jAnimations->arrPush(jvb(JID_MAP_BEGIN));

		jAnim->setMember("animationName", jvb(animation.animationName));
		jAnim->setMember("durationSec", jvb(animation.durationSec)); // will be used in the future

		// Serialize the actual key frames.
		JsonValue* jAllNodesKeyFrames = jAnim->setMember("perNodeKeyFrames", jvb(JID_ARRAY));

		for (const auto& itrPerNodeKeyframes : animation.perNodeKeyFrames) {
			JsonValue* jNodeKeyFrames = jvb(JID_MAP);

			jNodeKeyFrames->setMember("nodeIndex", jvb(itrPerNodeKeyframes.first));
			JsonValue* jKeyFrames = generateKeyFrames(itrPerNodeKeyframes.second);
			jNodeKeyFrames->setMember("keyFrames", jKeyFrames);

			jAllNodesKeyFrames->arrPush(jNodeKeyFrames);
		}
	}
}

void ModelWriter::writeNodes() {
	JsonValue* jNodes = root->setMember("nodes", jvb(JID_ARRAY));
	root->setMember("rootNodeIndex", jvb(model->getRootNodeIndex()));

	for (int iNode : range_int(model->numNodes())) {
		const ModelNode* node = model->nodeAt(iNode);

		JsonValue* jNode = jNodes->arrPush(jvb(JID_MAP));

		jNode->setMember("name", jvb(node->name));

		if (node->staticLocalTransform.p != vec3f(0.f)) {
			jNode->setMember("translation", jvb(node->staticLocalTransform.p.data, 3)); // An array of 3 floats.
		}

		if (node->staticLocalTransform.r != quatf::getIdentity()) {
			jNode->setMember("rotation", jvb(node->staticLocalTransform.r.data, 4)); // An array of 4 floats.
		}

		if (node->staticLocalTransform.s != vec3f(1.f)) {
			jNode->setMember("scaling", jvb(node->staticLocalTransform.s.data, 3)); // An array of 3 floats.
		}

		if (node->limbLength > 0.f) {
			jNode->setMember("limbLength", jvb(node->limbLength));
		}

		// Attached meshes.
		if (!node->meshAttachments.empty()) {
			auto jMeshes = jNode->setMember("meshes", jvb(JID_ARRAY));

			for (const MeshAttachment attachmentMesh : node->meshAttachments) {
				JsonValue* const jAttachmentMesh = jvb(JID_MAP);
				jAttachmentMesh->setMember("meshIndex", jvb(attachmentMesh.attachedMeshIndex));
				jAttachmentMesh->setMember("materialIndex", jvb(attachmentMesh.attachedMaterialIndex));

				jMeshes->arrPush(jAttachmentMesh);
			}
		}

		// Child indices.
		if (!node->childNodes.empty()) {
			JsonValue* const jChildNode = jNode->setMember("childNodes", jvb(JID_ARRAY));
			for (int childIndex : node->childNodes) {
				jChildNode->arrPush(jvb(childIndex));
			}
		}
	}
}

void ModelWriter::writeMaterials() {
	JsonValue* const jMaterials = root->setMember("materials", jvb(JID_ARRAY));

	for (const int iMtl : range_int(model->numMaterials())) {
		JsonValue* const jMaterial = jMaterials->arrPush(jvb(JID_MAP));

		const ModelMaterial* mtl = model->materialAt(iMtl);

		jMaterial->setMember("name", jvb(mtl->name));

		static_assert(sizeof(mtl->diffuseColor) == sizeof(vec4f));
		jMaterial->setMember("diffuseColor", jvb(mtl->diffuseColor.data, 4)); // An array of 4 floats rgba.
		static_assert(sizeof(mtl->emissionColor) == sizeof(vec4f));
		jMaterial->setMember("emissionColor", jvb(mtl->emissionColor.data, 4)); // An array of 4 floats rgba.
		jMaterial->setMember("metallic", jvb(mtl->metallic));
		jMaterial->setMember("roughness", jvb(mtl->roughness));

		if (mtl->diffuseTextureName.empty() == false)
			jMaterial->setMember("diffuseTextureName", jvb(mtl->diffuseTextureName));

		if (mtl->emissionTextureName.empty() == false)
			jMaterial->setMember("emissionTextureName", jvb(mtl->emissionTextureName));

		if (mtl->metallicTextureName.empty() == false)
			jMaterial->setMember("metallicTextureName", jvb(mtl->metallicTextureName));

		if (mtl->roughnessTextureName.empty() == false)
			jMaterial->setMember("roughnessTextureName", jvb(mtl->roughnessTextureName));
	}

	return;
}

void ModelWriter::writeMeshes() {
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

	JsonValue* const jMeshes = root->setMember("meshes", jvb(JID_ARRAY));

	for (const int iMesh : range_int(model->numMeshes())) {
		JsonValue* jMesh = jMeshes->arrPush(jvb(JID_MAP));

		const ModelMesh* mesh = model->meshAt(iMesh);

		// Write the vertex/index buffers chunks.
		if (mesh->vertexBufferRaw.size()) {
			const int vertexBufferChunkId = newChunkFromStdVector(mesh->vertexBufferRaw);
			jMesh->setMember("vertexDataChunkId", jvb(vertexBufferChunkId));
		}

		if (mesh->indexBufferRaw.size()) {
			const int indexBufferChunkId = newChunkFromStdVector(mesh->indexBufferRaw);
			jMesh->setMember("indexDataChunkId", jvb(indexBufferChunkId));
		}

		// Write the meshes that are described in this mesh data.

		// Draw call settings.
		jMesh->setMember("name", jvb(mesh->name));
		jMesh->setMember("primitiveTopology", jvb(PrimitiveTopology2String(mesh->primitiveTopology)));
		jMesh->setMember("vbByteOffset", jvb((int)mesh->vbByteOffset));
		jMesh->setMember("numElements", jvb((int)mesh->numElements));
		jMesh->setMember("numVertices", jvb((int)mesh->numVertices));

		if (mesh->ibFmt != UniformType::Unknown) {
			jMesh->setMember("ibByteOffset", jvb((int)mesh->ibByteOffset));
			jMesh->setMember("ibFormat", jvb(UnformType2String(mesh->ibFmt)));
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
				const int boneOffsetChunkId = newDataChunkFromPtr(&bone.offsetMatrix, sizeof(bone.offsetMatrix));

				auto jBone = jBones->arrPush(jvb(JID_MAP_BEGIN));
				jBone->setMember("boneIndex", jvb(bone.nodeIdx));
				jBone->setMember("offsetMatrixChunkId", jvb(boneOffsetChunkId));
			}
		}

		// Axis aligned bounding box.
		jMesh->setMember("AABoxMin", jvb((float*)&mesh->aabox.min.x, 3));
		jMesh->setMember("AABoxMax", jvb((float*)&mesh->aabox.max.x, 3));
	}

	return;
}

void ModelWriter::writeCollisionData() {
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
			const int hullVertsChunkId = newChunkFromStdVector(model->m_convexHulls[t].vertices);
			const int hullIndsChunkId = newChunkFromStdVector(model->m_convexHulls[t].indices);
			jHull->setMember("vertsChunkId", jvb(hullVertsChunkId));
			jHull->setMember("indicesChunkId", jvb(hullIndsChunkId));
		}
	}

	// Write the conave hull that are evaluated at the static moment.
	if (model->m_concaveHulls.size()) {
		JsonValue* jStaticConvecHulls = root->setMember("staticConcaveHulls", jvb(JID_ARRAY));

		for (int t = 0; t < model->m_concaveHulls.size(); ++t) {
			JsonValue* const jHull = jStaticConvecHulls->arrPush(jvb(JID_MAP));
			const int hullVertsChunkId = newChunkFromStdVector(model->m_concaveHulls[t].vertices);
			const int hullIndsChunkId = newChunkFromStdVector(model->m_concaveHulls[t].indices);
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

bool ModelWriter::write(const Model& modelToWrite, IWriteStream* iws) {
	if (iws == nullptr) {
		return false;
	}

	this->model = &modelToWrite;

	root = jvb(JID_MAP);

	// The version of the file format.
	// 0 is the starting version.
	root->setMember("version", jvb(0));

	writeNodes();
	writeMaterials();
	writeMeshes();
	writeAnimations();
	writeCollisionData();

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

bool ModelWriter::write(const Model& modelToWrite, const char* const filename) {
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
