#include <functional>

#include "Model.h"

namespace sge {

//--------------------------------------------------------------
// Model
//--------------------------------------------------------------
void Model::createRenderingResources(SGEDevice& sgedev) {
	for (ModelMesh* const mesh : m_meshes) {
		const ResourceUsage::Enum usage = ResourceUsage::Immutable;

		if (mesh->vertexBufferRaw.size() != 0) {
			mesh->vertexBuffer = sgedev.requestResource<Buffer>();
			const BufferDesc vbd = BufferDesc::GetDefaultVertexBuffer((uint32)mesh->vertexBufferRaw.size(), usage);
			mesh->vertexBuffer->create(vbd, mesh->vertexBufferRaw.data());

			mesh->vertexDeclIndex = sgedev.getVertexDeclIndex(mesh->vertexDecl.data(), int(mesh->vertexDecl.size()));

			mesh->hasUsableTangetSpace = mesh->vbNormalOffsetBytes >= 0 && mesh->vbNormalOffsetBytes >= 0 &&
			                             mesh->vbTangetOffsetBytes >= 0 && mesh->vbBinormalOffsetBytes >= 0;

			if (mesh->indexBufferRaw.size() != 0) {
				mesh->indexBuffer = sgedev.requestResource<Buffer>();
				const BufferDesc ibd = BufferDesc::GetDefaultIndexBuffer((uint32)mesh->indexBufferRaw.size(), usage);
				mesh->indexBuffer->create(ibd, mesh->indexBufferRaw.data());
			}
		}
	}
}

int Model::makeNewNode() {
	ModelNode* node = m_containerNode.new_element();
	const int nodeIndex = int(m_nodes.size());
	m_nodes.push_back(node);

	return nodeIndex;
}

int Model::makeNewMaterial() {
	ModelMaterial* newMtl = m_containerMaterial.new_element();
	int materialIndex = int(m_materials.size());
	m_materials.push_back(newMtl);
	return materialIndex;
}

int Model::makeNewMesh() {
	ModelMesh* mesh = m_containerMesh.new_element();
	int meshIndex = int(m_meshes.size());
	m_meshes.push_back(mesh);
	return meshIndex;
}

int Model::makeNewAnim() {
	int animIndex = int(m_animations.size());
	m_animations.push_back(ModelAnimation());
	return animIndex;
}

inline void Model::setRootNodeIndex(const int newRootNodeIndex) {
	if (newRootNodeIndex >= 0 && newRootNodeIndex < numNodes()) {
		m_rootNodeIndex = newRootNodeIndex;
	} else {
		sgeAssert(false);
	}
}


ModelNode* Model::nodeAt(int nodeIndex) {
	if (nodeIndex >= 0 && nodeIndex < int(m_nodes.size())) {
		return m_nodes[nodeIndex];
	}
	sgeAssert(false);
	return nullptr;
}

const ModelNode* Model::nodeAt(int nodeIndex) const {
	if (nodeIndex >= 0 && nodeIndex < int(m_nodes.size())) {
		return m_nodes[nodeIndex];
	}
	sgeAssert(false);
	return nullptr;
}

int Model::findFistNodeIndexWithName(const std::string& name) const {
	for (int t = 0; t < int(m_nodes.size()); ++t) {
		if (m_nodes[t]->name == name) {
			return t;
		}
	}
	return -1;
}

ModelMaterial* Model::materialAt(int materialIndex) {
	if (materialIndex >= 0 && materialIndex < int(m_materials.size())) {
		return m_materials[materialIndex];
	}
	sgeAssert(false);
	return nullptr;
}

const ModelMaterial* Model::materialAt(int materialIndex) const {
	if (materialIndex >= 0 && materialIndex < int(m_materials.size())) {
		return m_materials[materialIndex];
	}
	sgeAssert(false);
	return nullptr;
}

ModelMesh* Model::meshAt(int meshIndex) {
	if (meshIndex >= 0 && meshIndex < int(m_meshes.size())) {
		return m_meshes[meshIndex];
	}
	sgeAssert(false);
	return nullptr;
}

const ModelMesh* Model::meshAt(int meshIndex) const {
	if (meshIndex >= 0 && meshIndex < int(m_meshes.size())) {
		return m_meshes[meshIndex];
	}
	sgeAssert(false);
	return nullptr;
}

const ModelAnimation* Model::animationAt(int iAnim) const {
	if (iAnim >= 0 && iAnim < int(m_animations.size())) {
		return &m_animations[iAnim];
	}
	return nullptr;
}

ModelAnimation* Model::animationAt(int iAnim) {
	if (iAnim >= 0 && iAnim < int(m_animations.size())) {
		return &m_animations[iAnim];
	}
	return nullptr;
}

const ModelAnimation* Model::getAnimationByName(const std::string& name) const {
	for (const ModelAnimation& a : m_animations) {
		if (a.animationName == name) {
			return &a;
		}
	}
	return nullptr;
}

int Model::getAnimationIndexByName(const std::string& name) const {
	for (int iAnimation = 0; iAnimation < m_animations.size(); ++iAnimation) {
		if (m_animations[iAnimation].animationName == name)
			return iAnimation;
	}
	return -1;
}

void KeyFrames::evaluate(transf3d& result, const float t) const {
	// Evaluate the translation.
	if (positionKeyFrames.empty() == false) {
		const auto& keyItr = positionKeyFrames.upper_bound(t);
		if (keyItr == positionKeyFrames.end()) {
			result.p = positionKeyFrames.rbegin()->second;
		} else {
			const auto& prevKeyItr = std::prev(keyItr);

			if (prevKeyItr == positionKeyFrames.end()) {
				result.p = keyItr->second;
			} else {
				const float t0 = prevKeyItr->first;
				const float t1 = keyItr->first;
				const float dt = t1 - t0;

				if (dt > 1e-6f) {
					result.p = lerp(prevKeyItr->second, keyItr->second, (t - t0) / dt);
				} else {
					result.p = keyItr->second;
				}
			}
		}
	}

	// Evaluate the translation.
	if (rotationKeyFrames.empty() == false) {
		const auto& keyItr = rotationKeyFrames.upper_bound(t);
		if (keyItr == rotationKeyFrames.end()) {
			result.r = rotationKeyFrames.rbegin()->second;
		} else {
			const auto& prevKeyItr = std::prev(keyItr);

			if (prevKeyItr == rotationKeyFrames.end()) {
				result.r = keyItr->second;
			} else {
				const float t0 = prevKeyItr->first;
				const float t1 = keyItr->first;
				const float dt = t1 - t0;

				if (dt > 1e-6f) {
					result.r = lerp(prevKeyItr->second, keyItr->second, (t - t0) / dt);
				} else {
					result.r = keyItr->second;
				}
			}
		}
	}


	// Evaluate the scaling.
	if (scalingKeyFrames.empty() == false) {
		const auto& keyItr = scalingKeyFrames.upper_bound(t);
		if (keyItr == scalingKeyFrames.end()) {
			result.s = scalingKeyFrames.rbegin()->second;
		} else {
			const auto& prevKeyItr = std::prev(keyItr);

			if (prevKeyItr == scalingKeyFrames.end()) {
				result.s = keyItr->second;
			} else {
				const float t0 = prevKeyItr->first;
				const float t1 = keyItr->first;
				const float dt = t1 - t0;

				if (dt > 1e-6f) {
					result.s = lerp(prevKeyItr->second, keyItr->second, (t - t0) / dt);
				} else {
					result.s = keyItr->second;
				}
			}
		}
	}
}

} // namespace sge
