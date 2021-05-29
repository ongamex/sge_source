#include <set>

#include "FBXSDKParser.h"
#include "IAssetRelocationPolicy.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/range_loop.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

struct FBXParseError : public std::logic_error {
	FBXParseError(const char* const error = "Unknown FBXParseError")
	    : std::logic_error(error) {}
};

FbxManager* g_fbxManager = nullptr;

bool InitializeFBXSDK() {
	if (!g_fbxManager) {
		g_fbxManager = FbxManager::Create();
	}

	if (g_fbxManager == nullptr) {
		printf("Failed to initialize FBX SDK!\n");
		sgeAssert(false);
		return false;
	}
	return true;
}

Model::CollisionMesh fbxMeshToCollisionMesh(fbxsdk::FbxMesh* const fbxMesh) {
	// Extract all vertices form all polygons and then remove the duplicate vertices and generate indices.
	const int numPolygons = fbxMesh->GetPolygonCount();
	const int numVerts = numPolygons * 3;

	std::vector<vec3f> trianglesVeticesWithDuplicated(numVerts);

	for (int const iPoly : range_int(numPolygons)) {
		for (int const iVertex : range_int(3)) {
			int const globalVertexIndex = iPoly * 3 + iVertex;
			int const ctrlPtIndex = fbxMesh->GetPolygonVertex(iPoly, iVertex);

			const fbxsdk::FbxVector4 fbxPosition = fbxMesh->GetControlPointAt(ctrlPtIndex);
			trianglesVeticesWithDuplicated[globalVertexIndex] =
			    vec3f((float)fbxPosition.mData[0], (float)fbxPosition.mData[1], (float)fbxPosition.mData[2]);
		}
	}

	// Remove the duplicates and create an index buffer.
	std::vector<vec3f> trianglesVetices;
	std::vector<int> trianglesIndices;

	for (int iVertex = 0; iVertex < trianglesVeticesWithDuplicated.size(); iVertex++) {
		int foundIndex = -1;
		for (int t = 0; t < trianglesVetices.size(); ++t) {
			if (trianglesVetices[t] == trianglesVeticesWithDuplicated[iVertex]) {
				foundIndex = t;
				break;
			}
		}

		if (foundIndex == -1) {
			trianglesVetices.push_back(trianglesVeticesWithDuplicated[iVertex]);
			foundIndex = int(trianglesVetices.size()) - 1;
		}

		trianglesIndices.push_back(foundIndex);
	}

	sgeAssert(trianglesIndices.size() % 3 == 0);

	return Model::CollisionMesh{std::move(trianglesVetices), std::move(trianglesIndices)};
}

// CAUTION: FBX SDK uses DEGREES this functon expects DEGREES!
// Converts an Euler angle rotation to quaternion.
quatf quatFromFbx(const fbxsdk::FbxEuler::EOrder rotationOrder, const fbxsdk::FbxDouble3& euler) {
	const auto make_quat = [](int _1, int _2, int _3, const fbxsdk::FbxDouble3& euler) {
		return quatf::getAxisAngle(vec3f::getAxis(_3), deg2rad((float)euler[_3])) *
		       quatf::getAxisAngle(vec3f::getAxis(_2), deg2rad((float)euler[_2])) *
		       quatf::getAxisAngle(vec3f::getAxis(_1), deg2rad((float)euler[_1]));
	};

	quatf result = quatf::getIdentity();

	switch (rotationOrder) {
		case fbxsdk::FbxEuler::eOrderXYZ: {
			result = make_quat(0, 1, 2, euler);
		} break;
		case fbxsdk::FbxEuler::eOrderXZY: {
			result = make_quat(0, 2, 1, euler);
		} break;
		case fbxsdk::FbxEuler::eOrderYZX: {
			result = make_quat(1, 2, 0, euler);
		} break;
		case fbxsdk::FbxEuler::eOrderYXZ: {
			result = make_quat(1, 0, 2, euler);
		} break;
		case fbxsdk::FbxEuler::eOrderZXY: {
			result = make_quat(2, 0, 1, euler);
		} break;
		case fbxsdk::FbxEuler::eOrderZYX: {
			result = make_quat(2, 1, 0, euler);
		} break;

		default: {
			throw FBXParseError("Unknown FBX rotation order!");
		} break;
	}

	return result;
}

transf3d transf3DFromFbx(const FbxAMatrix& fbxTr, const fbxsdk::FbxEuler::EOrder rotationOrder) {
	const FbxDouble3 fbxTranslation = fbxTr.GetT();
	const FbxDouble3 fbxRotationEuler = fbxTr.GetR();
	const FbxDouble3 fbxScaling = fbxTr.GetS();

	transf3d result;

	result.s = vec3f((float)fbxScaling[0], (float)fbxScaling[1], (float)fbxScaling[2]);
	result.r = quatFromFbx(rotationOrder, fbxRotationEuler);
	result.p = vec3f((float)fbxTranslation[0], (float)fbxTranslation[1], (float)fbxTranslation[2]);

	return result;
}

vec3f vec3fFromFbx(const fbxsdk::FbxVector4& fbxVec) {
	return vec3f((float)fbxVec.mData[0], (float)fbxVec.mData[1], (float)fbxVec.mData[2]);
}

template <class TFBXLayerElement, int TDataArity>
void readGeometryElement(TFBXLayerElement* const element, int const controlPointIndex, int const vertexIndex, float* const result) {
	fbxsdk::FbxGeometryElement::EMappingMode const mappingMode = element->GetMappingMode();
	fbxsdk::FbxGeometryElement::EReferenceMode const referenceMode = element->GetReferenceMode();

	if (mappingMode == FbxGeometryElement::eByControlPoint) {
		if (referenceMode == FbxGeometryElement::eDirect) {
			for (int t = 0; t < TDataArity; ++t) {
				result[t] = (float)(element->GetDirectArray().GetAt(controlPointIndex)[t]);
			}
		} else if (referenceMode == FbxGeometryElement::eIndexToDirect || referenceMode == FbxGeometryElement::eIndex) {
			int const index = element->GetIndexArray().GetAt(controlPointIndex);

			for (int t = 0; t < TDataArity; ++t) {
				result[t] = (float)(element->GetDirectArray().GetAt(index)[t]);
			}
		} else {
			throw FBXParseError("Unknown reference mode.");
		}
	} else if (mappingMode == FbxGeometryElement::eByPolygonVertex) {
		if (referenceMode == FbxGeometryElement::eDirect) {
			for (int t = 0; t < TDataArity; ++t) {
				result[t] = (float)(element->GetDirectArray().GetAt(vertexIndex)[t]);
			}

		} else if (referenceMode == FbxGeometryElement::eIndexToDirect || referenceMode == FbxGeometryElement::eIndex) {
			int const index = element->GetIndexArray().GetAt(vertexIndex);
			for (int t = 0; t < TDataArity; ++t) {
				result[t] = (float)(element->GetDirectArray().GetAt(index)[t]);
			}
		} else {
			throw FBXParseError("Unknown reference mode.");
		}
	} else {
		throw FBXParseError("Unknown mapping mode.");
	}
}

//---------------------------------------------------------------
// FBXSDKParser
//---------------------------------------------------------------
bool FBXSDKParser::parse(Model::Model* result,
                         std::vector<std::string>* pReferencedTextures,
                         fbxsdk::FbxScene* scene,
                         FbxNode* enforcedRootNode,
                         const ModelParseSettings& parseSettings) {
	try {
		// Make sure that the FBX SDK is initialized.
		InitializeFBXSDK();

		m_model = result;
		m_parseSettings = parseSettings;
		m_fbxScene = scene;
		m_pReferencedTextures = pReferencedTextures;

		// Do not change the axis system, it is better to do it during the export of the FBX file in the DCC.
#if 0
		// Change the up Axis to Y.
		FbxAxisSystem::OpenGL.ConvertScene(m_fbxScene);
#endif

		// Make sure that each mesh has only 1 material attached and that it is triangulated.
		fbxsdk::FbxGeometryConverter fbxGeomConv(g_fbxManager);
		if (!fbxGeomConv.SplitMeshesPerMaterial(m_fbxScene, true)) {
			return false;
		}

		if (!fbxGeomConv.Triangulate(m_fbxScene, true)) {
			return false;
		}

		// Discover all the nodes that need to be parsed. Allocate them and store their hierarchy.
		// This step also dicovers all meshes and materials that need to be imported.
		fbxsdk::FbxNode* const rootNodeToBeUsed = enforcedRootNode != nullptr ? enforcedRootNode : m_fbxScene->GetRootNode();
		const int importedRootNodeIndex = discoverNodesRecursive(rootNodeToBeUsed);

		// Caution:
		// I've encounteded a file that had a few nodes that weren't parented to any node.
		// As a workaround for them, if we aren't exporting a sub-tree we attach them to root of the imported node.
		if (enforcedRootNode == nullptr) {
			Model::Node* rootNode = m_model->nodeAt(importedRootNodeIndex);
			for (int const iFbxNode : range_int(m_fbxScene->GetNodeCount())) {
				// If the node hasn't been discovered yet, that means that is is not parented to the root node.
				FbxNode* const fbxNode = m_fbxScene->GetNode(iFbxNode);
				if (m_fbxNode2NodeIndex.find(fbxNode) == m_fbxNode2NodeIndex.end()) {
					// Discover the node, and then attach it to the root node.
					int newlyDiscoveredNodeIndex = discoverNodesRecursive(fbxNode);
					rootNode->childNodes.push_back(newlyDiscoveredNodeIndex);
				}
			}
		}

		importMaterials();
		importMeshes(enforcedRootNode == nullptr);
		importNodes();
		// Importing animations when we only import a sub-tree of the fbx scene isn't supported yet.
		if (enforcedRootNode == nullptr) {
			importAnimations();
		}

		// When importing a sub-tree reset the transformation of the root node,
		// we want the sub tree to be centered.
		if (enforcedRootNode != nullptr) {
			m_model->getRootNode()->staticLocalTransform.p = vec3f(0.f);

			// Since we are going to reposition the root node location, we aloso need to reposition
			// the collision geometries.
			fbxsdk::FbxAMatrix tr = enforcedRootNode->EvaluateGlobalTransform();
			tr.SetR(FbxVector4(0.f, 0.f, 0.f));
			tr.SetS(FbxVector4(1.f, 1.f, 1.f));
			m_collision_transfromCorrection = transf3DFromFbx(tr.Inverse(), FbxEuler::eOrderXYZ);
		}

		importCollisionGeometry();

		return true;
	} catch ([[maybe_unused]] const std::exception& except) {
		return false;
	}
}

int FBXSDKParser::discoverNodesRecursive(fbxsdk::FbxNode* const fbxNode) {
	const int nodeIndex = m_model->makeNewNode();
	Model::Node* node = m_model->nodeAt(nodeIndex);
	m_fbxNode2NodeIndex[fbxNode] = nodeIndex;

	node->name = fbxNode->GetName();

	// [FBX_PARSING_NODE_ATTRIBS]
	// Read the node's attrbiutes. Attributes are something that specifies what the node is and that data does it reference.
	// It could be if the node is used for a bone some parameters of that bone (for example it's length).
	// It could be a mesh and/or material.
	int const attribCount = fbxNode->GetNodeAttributeCount();
	for (const int iAttrib : range_int(attribCount)) {
		fbxsdk::FbxNodeAttribute* const fbxNodeAttrib = fbxNode->GetNodeAttributeByIndex(iAttrib);

		const fbxsdk::FbxNodeAttribute::EType fbxAttriuteType = fbxNodeAttrib->GetAttributeType();
		if (fbxAttriuteType == fbxsdk::FbxNodeAttribute::eSkeleton) {
		} else if (fbxAttriuteType == fbxsdk::FbxNodeAttribute::eMesh) {
			// Caution:
			// Skip all non triangle meshes. There should be an import option that
			// triangualates all the mehses and it must be specified.
			fbxsdk::FbxMesh* const fbxMesh = fbxsdk::FbxCast<fbxsdk::FbxMesh>(fbxNodeAttrib);
			if (fbxMesh && fbxMesh->IsTriangleMesh()) {
				// Check if the geometry attached to this node should
				// be used for as collsion geometry and not for rendering.
				// This is guessed by the prefix in the node name.
				bool const isCollisionGeometryTriMeshNode = node->name.find("SCConcave_") == 0 || node->name.find("SCTriMesh_") == 0;
				bool const isCollisionGeometryConvexNode = node->name.find("SCConvex_") == 0;
				bool const isCollisionGeometryBoxNode = node->name.find("SCBox_") == 0;
				bool const isCollisionGeometryCapsuleNode = node->name.find("SCCapsule_") == 0;
				bool const isCollisionGeometryCylinderNode = node->name.find("SCCylinder_") == 0;
				bool const isCollisionGeometrySphereNode = node->name.find("SCSphere_") == 0;

				// clang-format off
				bool const isCollisionGeometryNode = 
					   isCollisionGeometryTriMeshNode 
					|| isCollisionGeometryConvexNode 
					|| isCollisionGeometryBoxNode 
					|| isCollisionGeometryCapsuleNode 
					|| isCollisionGeometryCylinderNode 
					|| isCollisionGeometrySphereNode;
				// clang-format on

				if (isCollisionGeometryNode) {
					// A mesh that must be used for collision only.
					transf3d const globalTransformBindMoment = transf3DFromFbx(fbxNode->EvaluateGlobalTransform(), FbxEuler::eOrderXYZ);

					if (isCollisionGeometryConvexNode) {
						m_collision_ConvexHullMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else if (isCollisionGeometryTriMeshNode) {
						m_collision_BvhTriMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else if (isCollisionGeometryBoxNode) {
						m_collision_BoxMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else if (isCollisionGeometryCapsuleNode) {
						m_collision_CaplsuleMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else if (isCollisionGeometryCylinderNode) {
						m_collision_CylinderMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else if (isCollisionGeometrySphereNode) {
						m_collision_SphereMeshes[fbxMesh].push_back(globalTransformBindMoment);
					} else {
						sgeAssert(false);
					}
				} else {
					// The mesh is going to be used for rendering.

					// If this is the 1st time we dicover that mesh,
					// allocated memory for our imported one and save it for later import.
					if (m_fbxMesh2MeshIndex.count(fbxMesh) == 0) {
						const int meshIndex = m_model->makeNewMesh();
						m_fbxMesh2MeshIndex[fbxMesh] = meshIndex;
					}

					// Find the attached material to that mesh.
					// I'm not sure if this is the corrent way to handle it.
					// We are assuming that we have 1 material per mesh.
					fbxsdk::FbxSurfaceMaterial* const fbxSurfMtl = fbxNode->GetMaterial(iAttrib);
					if (fbxSurfMtl) {
						// If this is the 1st time we dicover this material,
						// allocated memory for our imported one and save it for later import.
						if (m_fbxMtl2MtlIndex.count(fbxSurfMtl) == 0) {
							int materialIndex = m_model->makeNewMaterial();
							m_fbxMtl2MtlIndex[fbxSurfMtl] = materialIndex;
						}
					}
				}
			}
		}
	}

	// Parse the child nodes and store the hierarchy.
	int const childCount = fbxNode->GetChildCount();
	for (int iChild = 0; iChild < childCount; ++iChild) {
		fbxsdk::FbxNode* const fbxChildNode = fbxNode->GetChild(iChild);
		node->childNodes.push_back(discoverNodesRecursive(fbxChildNode));
	}

	return nodeIndex;
}

void FBXSDKParser::importMaterials() {
	for (auto& pair : m_fbxMtl2MtlIndex) {
		FbxSurfaceMaterial* const fSurfMtl = pair.first;
		Model::Material* const material = m_model->materialAt(pair.second);

#if 0
		// A loop useful for debugging while importing new types of materials.
		// This loop can show you all available properies and their names.
		auto prop = fSurfMtl->GetFirstProperty();
		while (prop.IsValid()) {
			[[maybe_unused]] const char* name = prop.GetNameAsCStr();
			prop = fSurfMtl->GetNextProperty(prop);
		}
#endif

		material->name = fSurfMtl->GetName();

		// By reverse engineering the FBX file containing Stingray PBS material
		// exported from Autodesk Maya it seems that the attributes for
		// it are exported as properties under the compound property named "Maya".
		const FbxProperty propMaya = fSurfMtl->FindProperty("Maya", false);

		// If the Maya property is available assume that this is Stingray PBS material.
		if (propMaya.IsValid()) {
			// Try to parse this as if it is a Stingray PBS material.

			// Load the specified texture (if exists) in the current material parameter block under the name @outParamName.
			// Returns true if a texture was assigned.
			const auto loadTextureFromFbxPropertyByName = [&](const FbxProperty& rootProp, const char* subPropName) -> std::string {
				const FbxProperty prop = rootProp.Find(subPropName);
				if (prop.IsValid()) {
					if (const FbxFileTexture* fFileTex = prop.GetSrcObject<FbxFileTexture>()) {
						const char* const texFilename = fFileTex->GetRelativeFileName();
						const std::string requestPath =
						    m_parseSettings.pRelocaionPolicy->whatWillBeTheAssetNameOf(m_parseSettings.fileDirectroy, texFilename);

						if (m_pReferencedTextures && !requestPath.empty()) {
							m_pReferencedTextures->push_back(requestPath);
						}

						return requestPath;
					}
				}

				// No texture was found.
				return std::string();
			};

			// Check for existing attached textures to the material.
			// If a texture is found, reset the numeric value to 1.f, as Maya doesn't use them to multiply the texture values as we do
			// when textures as used.
			material->diffuseTextureName = loadTextureFromFbxPropertyByName(propMaya, "TEX_color_map");
			material->normalTextureName = loadTextureFromFbxPropertyByName(propMaya, "TEX_normal_map");
			material->metallicTextureName = loadTextureFromFbxPropertyByName(propMaya, "TEX_metallic_map");
			material->roughnessTextureName = loadTextureFromFbxPropertyByName(propMaya, "TEX_roughness_map");

			// Write the numeric diffuse color.
			if (material->diffuseTextureName.empty()) {
				fbxsdk::FbxDouble3 fBaseColor = propMaya.Find("base_color").Get<fbxsdk::FbxVector4>();
				material->diffuseColor = vec4f((float)fBaseColor.mData[0], (float)fBaseColor.mData[1], (float)fBaseColor.mData[2], 1.f);
			} else {
				material->diffuseColor = vec4f(1.f);
			}

			if (material->metallicTextureName.empty()) {
				material->metallic = propMaya.Find("metallic").Get<float>();
			} else {
				material->metallic = 1.f;
			}

			if (material->roughnessTextureName.empty()) {
				material->roughness = propMaya.Find("roughness").Get<float>();
			} else {
				material->roughness = 1.f;
			}
		} else {
			// Searches for the 1st texture in the specified property.
			const auto findTextureFromFbxProperty = [](const FbxProperty& property) -> fbxsdk::FbxFileTexture* {
				// Search in the layered textures.
				int const layeredTexCount = property.GetSrcObjectCount<fbxsdk::FbxLayeredTexture>();
				for (int const iLayer : range_int(layeredTexCount)) {
					fbxsdk::FbxLayeredTexture* const fbxLayeredTex =
					    FbxCast<fbxsdk::FbxLayeredTexture>(property.GetSrcObject<fbxsdk::FbxLayeredTexture>(iLayer));
					int const textureCount = fbxLayeredTex->GetSrcObjectCount<fbxsdk::FbxTexture>();
					for (int const iTex : range_int(textureCount)) {
						fbxsdk::FbxFileTexture* const fFileTex =
						    fbxsdk::FbxCast<fbxsdk::FbxFileTexture>(fbxLayeredTex->GetSrcObject<FbxTexture>(iTex));
						if (fFileTex != nullptr) {
							return fFileTex;
						}
					}
				}

				int const textureCount = property.GetSrcObjectCount<FbxTexture>();
				for (int const iTex : range_int(textureCount)) {
					fbxsdk::FbxFileTexture* const fFileTex = fbxsdk::FbxCast<fbxsdk::FbxFileTexture>(property.GetSrcObject<FbxTexture>(0));
					if (fFileTex) {
						return fFileTex;
					}
				}

				// No textures were found.
				return nullptr;
			};

			const FbxProperty propNormal = fSurfMtl->FindProperty(FbxSurfaceMaterial::sNormalMap);
			const FbxProperty propBump = fSurfMtl->FindProperty(FbxSurfaceMaterial::sBump);
			const FbxProperty propDiffuse = fSurfMtl->FindProperty(FbxSurfaceMaterial::sDiffuse);
			const FbxProperty propDiffuseFactor = fSurfMtl->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);
			const FbxProperty propSpecular = fSurfMtl->FindProperty(FbxSurfaceMaterial::sSpecular);
			const FbxProperty propSpecularFactor = fSurfMtl->FindProperty(FbxSurfaceMaterial::sSpecularFactor);

			fbxsdk::FbxFileTexture* const fFileDiffuseTex = findTextureFromFbxProperty(propDiffuse);
			if (fFileDiffuseTex) {
				const char* const texFilename = fFileDiffuseTex->GetRelativeFileName();
				const std::string requestPath =
				    m_parseSettings.pRelocaionPolicy->whatWillBeTheAssetNameOf(m_parseSettings.fileDirectroy, texFilename);
				material->diffuseTextureName = requestPath;
				if (m_pReferencedTextures) {
					m_pReferencedTextures->push_back(requestPath);
				}
			}

			fbxsdk::FbxFileTexture* const fFileNormalTex = findTextureFromFbxProperty(propNormal);
			if (fFileNormalTex) {
				const char* const texFilename = fFileNormalTex->GetRelativeFileName();
				const std::string requestPath =
				    m_parseSettings.pRelocaionPolicy->whatWillBeTheAssetNameOf(m_parseSettings.fileDirectroy, texFilename);
				material->normalTextureName = requestPath;
				if (m_pReferencedTextures) {
					m_pReferencedTextures->push_back(requestPath);
				}
			} else {
				// if there is no normal map, then most likely it is going to be reported as a bump map.
				fbxsdk::FbxFileTexture* const fFileBumpTex = findTextureFromFbxProperty(propBump);
				if (fFileBumpTex) {
					const char* const texFilename = fFileBumpTex->GetRelativeFileName();
					const std::string requestPath =
					    m_parseSettings.pRelocaionPolicy->whatWillBeTheAssetNameOf(m_parseSettings.fileDirectroy, texFilename);
					material->normalTextureName = requestPath;
					if (m_pReferencedTextures) {
						m_pReferencedTextures->push_back(requestPath);
					}
				}
			}

			FbxProperty fbxDiffuseProp = fSurfMtl->FindProperty(FbxSurfaceMaterial::sDiffuse);
			if (fbxDiffuseProp.IsValid()) {
				FbxDouble3 const cd = fSurfMtl->FindProperty(FbxSurfaceMaterial::sDiffuse).Get<FbxDouble3>();
				material->diffuseColor = vec4f((float)cd.mData[0], (float)cd.mData[1], (float)cd.mData[2], 1.f);
			}
		}
	}
}

void FBXSDKParser::importMeshes(const bool importSkinningData) {
	for (auto& pair : m_fbxMesh2MeshIndex) {
		FbxMesh* const fbxMesh = pair.first;
		const int meshIndex = pair.second;
		importMeshes_singleMesh(fbxMesh, meshIndex, importSkinningData);
	}
}

void FBXSDKParser::importMeshes_singleMesh(FbxMesh* fbxMesh, int importedMeshIndex, const bool importSkinningData) {
	// I have not seen a geometry with non-identity pivot (at least for games) ever, this is why it is not handled.
	{
		FbxAMatrix fbxPivot;
		fbxMesh->GetPivot(fbxPivot);
		if (!fbxPivot.IsIdentity()) {
			printf("Non identity pivot on geometry %s. These are not supported (yet!). Identity will be used!\n", fbxMesh->GetName());
		}
	}

	Model::Mesh& mesh = *m_model->getMeshByIndex(importedMeshIndex);

	mesh.name = fbxMesh->GetName();

	// We assume that the meshes are separated by material already.
	const int materialCount = fbxMesh->GetElementMaterialCount();
	if (materialCount != 1) {
		sgeAssert(false);
		throw FBXParseError("Failed to load mesh. More than one materials are attached!");
	}

	mesh.name = fbxMesh->GetName();

	// Remember, we are assuming that the meshes are already triangulated.
	// Compute the vertex layout.

	// Get the avaiables vertex data channels(called layers in FBX) in that fbxMesh.
	// Currently we support only one layer.
	int stride = 0;

	// Vertex positions.
	{
		VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_position";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		mesh.vertexDecl.push_back(decl);

		stride += 12;
	}

	// Color.
	FbxGeometryElementVertexColor* const evemVertexColor0 = fbxMesh->GetElementVertexColor(0);
	int color0ByteOffset = -1;
	if (evemVertexColor0 != nullptr) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_color";
		decl.format = sge::UniformType::Float4;
		decl.byteOffset = -1;

		color0ByteOffset = stride;
		stride += 16;
		mesh.vertexDecl.push_back(decl);
	}

	// Normals.
	FbxGeometryElementNormal* elemNormals0 = fbxMesh->GetElementNormal(0);
	if (elemNormals0 == nullptr) {
		fbxMesh->GenerateNormals();
		elemNormals0 = fbxMesh->GetElementNormal(0);
	}

	int normals0ByteOffset = -1;
	if (elemNormals0 != nullptr) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_normal";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		normals0ByteOffset = stride;
		stride += 12;
		mesh.vertexDecl.push_back(decl);
	}

	// Tangents.
	FbxGeometryElementTangent* const elemTangets0 = fbxMesh->GetElementTangent(0);
	int tangents0ByteOffset = -1;
	if (elemTangets0 != nullptr) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_tangent";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		tangents0ByteOffset = stride;
		stride += 12;
		mesh.vertexDecl.push_back(decl);
	}

	// Binormals.
	FbxGeometryElementBinormal* const elemBinormal0 = fbxMesh->GetElementBinormal(0);
	int binormals0ByteOffset = -1;
	if (elemTangets0 != nullptr) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_binormal";
		decl.format = sge::UniformType::Float3;
		decl.byteOffset = -1;

		binormals0ByteOffset = stride;
		stride += 12;
		mesh.vertexDecl.push_back(decl);
	}

	// UVs
	FbxGeometryElementUV* const elemUV0 = fbxMesh->GetElementUV(0);
	int UV0ByteOffset = -1;
	if (elemUV0 != nullptr) {
		sge::VertexDecl decl;
		decl.bufferSlot = 0;
		decl.semantic = "a_uv";
		decl.format = sge::UniformType::Float2;
		decl.byteOffset = -1;

		UV0ByteOffset = stride;
		stride += 8;
		mesh.vertexDecl.push_back(decl);
	}

	struct BoneInfluence {
		BoneInfluence() = default;
		BoneInfluence(int boneIndex, float boneWeight)
		    : boneIndex(boneIndex)
		    , boneWeight(boneWeight) {}

		int boneIndex = -1;
		float boneWeight = 0.f;
	};

	const int kMaxBonesPerVertex = 4;
	std::map<int, std::vector<BoneInfluence>> perControlPointBoneInfluence;

	// Find if there is a skinning data and if so, load the bones for mesh skinning (skeletal animation).
	if (importSkinningData) {
		for (int const iDeformer : range_int(fbxMesh->GetDeformerCount())) {
			fbxsdk::FbxDeformer* const fDeformer = fbxMesh->GetDeformer(iDeformer);
			fbxsdk::FbxSkin* const fSkin = fbxsdk::FbxCast<fbxsdk::FbxSkin>(fDeformer);

			if (fSkin == nullptr) {
				continue;
			}

			// Read the clusters from the skin. Clusters are what they call bones in FBX.
			int const clustersCount = fSkin->GetClusterCount();

			mesh.bones.reserve(clustersCount);

			for (const int iCluster : range_int(clustersCount)) {
				fbxsdk::FbxCluster* const fCluster = fSkin->GetCluster(iCluster);
				fbxsdk::FbxNode* const fNodeBone = fCluster->GetLink();

				// Find the node that represents the bone. The animation on that node will represent the movement of the bone.
				const auto& itrFindBoneNode = m_fbxNode2NodeIndex.find(fNodeBone);
				if (itrFindBoneNode == m_fbxNode2NodeIndex.end()) {
					throw FBXParseError();
				}

				const int boneNodeIndex = itrFindBoneNode->second;

				// Compute the offset matrix. The offset matrix repsents the location of the bone when it was bound the the mesh.
				// It is used to transform the vertices to the bones space and apply it's infuence there and then to return it back to the
				// meshes "object space".

				// auto t = fNodeBone->GetGeometricTranslation(fbxsdk::FbxNode::eSourcePivot);
				// auto r = fNodeBone->GetGeometricRotation(fbxsdk::FbxNode::eSourcePivot);
				// auto s = fNodeBone->GetGeometricScaling(fbxsdk::FbxNode::eSourcePivot);

				FbxAMatrix transformMatrix;
				FbxAMatrix transformLinkMatrix;

				// The transformation of the mesh at binding time.
				fCluster->GetTransformMatrix(transformMatrix);

				// The transformation of the cluster(joint, bone) at binding time from joint space to world space.
				// TODO: Geometric transform. The internet applies it here, but it should be stored in the MeshAttachment IMHO.
				fCluster->GetTransformLinkMatrix(transformLinkMatrix);
				FbxAMatrix const globalBindposeInverseMatrix = transformLinkMatrix.Inverse() * transformMatrix;

				mat4f boneOffseMtx = mat4f::getIdentity();
				for (int const t : range_int(16)) {
					boneOffseMtx.data[t / 4][t % 4] = (float)(globalBindposeInverseMatrix.mData[t / 4].mData[t % 4]);
				}

				// Read the affected vertex indices by the bone and store their affection weight.
				int const affectedPointsCount = fCluster->GetControlPointIndicesCount();
				for (int const iPt : range_int(affectedPointsCount)) {
					const float weight = float(fCluster->GetControlPointWeights()[iPt]);
					const int ctrlPt = fCluster->GetControlPointIndices()[iPt];

					perControlPointBoneInfluence[ctrlPt].push_back(BoneInfluence(iCluster, weight));
				}

				// Finally Add the bone to the mesh.
				mesh.bones.push_back(Model::Bone(boneOffseMtx, boneNodeIndex));
			}

			// We support so far only skin deformers. And we support only one skin deformer.
			// This is why we break here.
			break;
		}
	}
	int boneIdsByteOffset = -1;
	int boneWeightsByteOffset = -1;

	// If there are bones in the mesh. Create the vertex attributres for them.
	if (perControlPointBoneInfluence.empty() == false) {
		for (auto& pair : perControlPointBoneInfluence) {
			// Sort by weigths.
			for (int i = 0; i < pair.second.size(); ++i) {
				for (int j = i + 1; j < pair.second.size(); ++j) {
					if (pair.second[j].boneWeight > pair.second[i].boneWeight) {
						std::swap(pair.second[j], pair.second[i]);
					}
				}
			}

			// Some control point might be infuenced by more then kMaxBonesPerVertex,
			// if reduce the bones that amount and renormalize the weigths so they sum to 1.
			if (pair.second.size() > kMaxBonesPerVertex) {
				pair.second.resize(kMaxBonesPerVertex);

				float weigthSum = 0.f;
				for (int t = 0; t < kMaxBonesPerVertex; ++t) {
					weigthSum += pair.second[t].boneWeight;
				}

				if (weigthSum > 1e-6f) {
					float invSum = 1.f / weigthSum;
					for (int t = 0; t < kMaxBonesPerVertex; ++t) {
						pair.second[t].boneWeight = pair.second[t].boneWeight * invSum;
					}
				}
			}

			{
				sge::VertexDecl decl;
				decl.bufferSlot = 0;
				decl.semantic = "a_bonesIds";
				decl.format = sge::UniformType::Int4;
				decl.byteOffset = -1;

				boneIdsByteOffset = stride;
				stride += 16;
				mesh.vertexDecl.push_back(decl);
			}

			{
				sge::VertexDecl decl;
				decl.bufferSlot = 0;
				decl.semantic = "a_boneWeights";
				decl.format = sge::UniformType::Float4;
				decl.byteOffset = -1;

				boneWeightsByteOffset = stride;
				stride += 16;
				mesh.vertexDecl.push_back(decl);
			}
		}
	}

	mesh.primTopo = PrimitiveTopology::TriangleList;
	mesh.vertexDecl = sge::VertexDecl::NormalizeDecl(mesh.vertexDecl.data(), int(mesh.vertexDecl.size()));

	auto const findVertexChannelByteOffset = [](const char* semantic, const std::vector<VertexDecl>& decl) -> int {
		for (const VertexDecl& vertexDecl : decl) {
			if (vertexDecl.semantic == semantic) {
				return vertexDecl.byteOffset;
			}
		}

		return -1;
	};

	// An asset to check if we computed correctly vertex layout.
	sgeAssert(stride == mesh.vertexDecl.back().byteOffset + sge::UniformType::GetSizeBytes(mesh.vertexDecl.back().format));

	// Get the number of polygons.
	const int numPolygons = fbxMesh->GetPolygonCount();
	int const numVertsBeforeIBGen = numPolygons * 3;

	// An separated buffers of all data needed to every vertex.
	// these arrays (if used) should always have the same size.
	// Together they form a complete triangle list representing the gemeotry.
	std::vector<vec4f> vtxColors(numVertsBeforeIBGen);
	std::vector<vec3f> positions(numVertsBeforeIBGen);
	std::vector<vec3f> normals(numVertsBeforeIBGen);
	std::vector<vec3f> tangents(numVertsBeforeIBGen);
	std::vector<vec3f> binormals(numVertsBeforeIBGen);
	std::vector<vec2f> uvs(numVertsBeforeIBGen);
	std::vector<vec4i> boneIds(numVertsBeforeIBGen);
	std::vector<vec4f> bonesWeights(numVertsBeforeIBGen);

	AABox3f geomBBoxObjectSpace; // Store the bounding box of the geometry in object space.

	// The control point of each interleaved vertex.
	// Provides a way to get the control point form a vertex.
	std::vector<int> vert2controlPoint_BeforeInterleave(numVertsBeforeIBGen);

	for (int const iPoly : range_int(numPolygons)) {
		[[maybe_unused]] const int debugOnly_polyVertexCountInFbx = fbxMesh->GetPolygonSize(iPoly);
		sgeAssert(debugOnly_polyVertexCountInFbx == 3);
		for (int const iVertex : range_int(3)) {
			int const globalVertexIndex = iPoly * 3 + iVertex;
			int const ctrlPtIndex = fbxMesh->GetPolygonVertex(iPoly, iVertex);

			vert2controlPoint_BeforeInterleave[globalVertexIndex] = ctrlPtIndex;

			fbxsdk::FbxVector4 const fbxPosition = fbxMesh->GetControlPointAt(ctrlPtIndex);
			positions[globalVertexIndex] = vec3f((float)fbxPosition.mData[0], (float)fbxPosition.mData[1], (float)fbxPosition.mData[2]);

			geomBBoxObjectSpace.expand(positions[globalVertexIndex]);

			// Read the vertex attributes.
			if (evemVertexColor0) {
				readGeometryElement<FbxGeometryElementVertexColor, 4>(evemVertexColor0, ctrlPtIndex, globalVertexIndex,
				                                                      vtxColors[globalVertexIndex].data);
			}

			if (elemNormals0) {
				readGeometryElement<FbxGeometryElementNormal, 3>(elemNormals0, ctrlPtIndex, globalVertexIndex,
				                                                 normals[globalVertexIndex].data);
			}

			if (elemTangets0) {
				readGeometryElement<FbxGeometryElementTangent, 3>(elemTangets0, ctrlPtIndex, globalVertexIndex,
				                                                  tangents[globalVertexIndex].data);
			}

			if (elemBinormal0) {
				readGeometryElement<FbxGeometryElementBinormal, 3>(elemBinormal0, ctrlPtIndex, globalVertexIndex,
				                                                   binormals[globalVertexIndex].data);
			}

			if (elemUV0) {
				// Convert to DirectX style UVs.
				readGeometryElement<FbxGeometryElementUV, 2>(elemUV0, ctrlPtIndex, globalVertexIndex, uvs[globalVertexIndex].data);
				uvs[globalVertexIndex].y = 1.f - uvs[globalVertexIndex].y;
			}

			// Per vertex skinning data.
			if (perControlPointBoneInfluence.empty() == false) {
				vec4i boneIdsForVertex = vec4i(-1);
				vec4f boneWeightsForVertex = vec4f(0.f);

				const auto& boneInfluences = perControlPointBoneInfluence[ctrlPtIndex];
				const int maxArity = std::min(int(boneInfluences.size()), 4);
				for (int iInfluence = 0; iInfluence < maxArity; ++iInfluence) {
					boneIdsForVertex[iInfluence] = boneInfluences[iInfluence].boneIndex;
					boneWeightsForVertex[iInfluence] = boneInfluences[iInfluence].boneWeight;
				}

				boneIds[globalVertexIndex] = boneIdsForVertex;
				bonesWeights[globalVertexIndex] = boneWeightsForVertex;
			}
		}
	}

	// Interleave the read data.
	std::vector<char> interleavedVertexData(numVertsBeforeIBGen * stride);
	sgeAssert(positions.size() == normals.size() && positions.size() == uvs.size());

	for (int t = 0; t < positions.size(); ++t) {
		char* const vertexData = interleavedVertexData.data() + t * stride;

		vec3f& position = *(vec3f*)vertexData;
		vec4f& rgba = *(vec4f*)(vertexData + color0ByteOffset);
		vec3f& normal = *(vec3f*)(vertexData + normals0ByteOffset);
		vec3f& tangent = *(vec3f*)(vertexData + tangents0ByteOffset);
		vec3f& binormal = *(vec3f*)(vertexData + binormals0ByteOffset);
		vec2f& uv = *(vec2f*)(vertexData + UV0ByteOffset);
		vec4i& vertexBoneIds = *(vec4i*)(vertexData + boneIdsByteOffset);
		vec4f& vertexBoneWeights = *(vec4f*)(vertexData + boneWeightsByteOffset);

		position = positions[t];

		if (color0ByteOffset >= 0)
			rgba = vtxColors[t];
		if (normals0ByteOffset >= 0)
			normal = normals[t];
		if (tangents0ByteOffset >= 0)
			tangent = tangents[t];
		if (binormals0ByteOffset >= 0)
			binormal = binormals[t];
		if (UV0ByteOffset >= 0)
			uv = uvs[t];

		if (boneIdsByteOffset >= 0)
			vertexBoneIds = boneIds[t];
		if (boneWeightsByteOffset >= 0)
			vertexBoneWeights = bonesWeights[t];
	}

	// Clear the separate vertices as they are no longer needed
	vtxColors = std::vector<vec4f>();
	positions = std::vector<vec3f>();
	normals = std::vector<vec3f>();
	tangents = std::vector<vec3f>();
	binormals = std::vector<vec3f>();
	uvs = std::vector<vec2f>();
	boneIds = std::vector<vec4i>();
	bonesWeights = std::vector<vec4f>();

	// Generate the index buffer and remove duplidated vertices.
	std::vector<char> indexBufferData;
	std::vector<char> vertexBufferData;

	// Stores the control point used to for each vertex. This data is needed
	// when we remove duplicated vertices. Duplicated vertices colud happen in two ways
	// 1st one is with the code above where we create a flat buffer of all vertices (representing a triangle list).
	// 2nd one is that the geometry had two or more control points at the same location because the geometry (produced by the artist)
	// had duplicated faces. In this situation it feel tempting to just ignore that the vertices come from different control points
	// however when skinning is involved this vertex will end up not being assigned to any bone, this is why we take the source control
	// point of the vertex when we remove duplicates.
	std::vector<int> perVertexControlPoint;
	std::vector<std::set<int>> controlPoint2verts(fbxMesh->GetControlPointsCount());

	indexBufferData.reserve(sizeof(int) * numVertsBeforeIBGen);

	[[maybe_unused]] bool isDuplicatedFacesMessageShown = false;

	for (int iSrcVert = 0; iSrcVert < numVertsBeforeIBGen; ++iSrcVert) {
		const char* const srcVertexData = &interleavedVertexData[size_t(iSrcVert) * size_t(stride)];

		ptrdiff_t vertexIndexWithSameData = -1;

		// Check if the same data has already been used. If so, use that one for the vertex.
#if 1
		for (int byteOffset = int(vertexBufferData.size()) - stride; byteOffset >= 0; byteOffset -= stride) {
			const bool doesDataMatch = memcmp(srcVertexData, &vertexBufferData[byteOffset], stride) == 0;
			if (doesDataMatch) {
				const bool doesSourceControlPointMatch =
				    vert2controlPoint_BeforeInterleave[iSrcVert] == vert2controlPoint_BeforeInterleave[byteOffset / stride];

				// Sometimes the artists might create duplicated geometry, if so the data will match but not the
				// control point that generated that vertex. As a result, the bones skinning later will not get applied properly.
				if (doesSourceControlPointMatch) {
					vertexIndexWithSameData = byteOffset / stride;
					break;
				} else {
					if (isDuplicatedFacesMessageShown == false) {
						isDuplicatedFacesMessageShown = true;
						printf("Found duplicate faces! The duplicated faces will remain in the output!\n");
					}
				}
			}
		}
#endif

		// If the vertex doesn't exists create a new one.
		if (vertexIndexWithSameData == -1) {
			vertexIndexWithSameData = int(vertexBufferData.size()) / stride;
			vertexBufferData.insert(vertexBufferData.end(), srcVertexData, srcVertexData + stride);
			perVertexControlPoint.push_back(vert2controlPoint_BeforeInterleave[iSrcVert]);
		}

		// Insert the index in the index buffer.
		indexBufferData.resize(indexBufferData.size() + 4);
		*(uint32*)(&indexBufferData.back() - 3) = uint32(vertexIndexWithSameData);
		controlPoint2verts[vert2controlPoint_BeforeInterleave[iSrcVert]].insert(int(vertexIndexWithSameData));
	}

	// Clear the interleaved vertex data as it is no longer needed.
	interleavedVertexData = std::vector<char>();

	// Caution: In the computation of numElements we ASSUME that ibFmt is UniformType::Uint!
	mesh.ibFmt = UniformType::Uint;

	sgeAssert(mesh.ibFmt == UniformType::Uint);
	mesh.numElements = int(indexBufferData.size()) / 4;
	mesh.numVertices = int(vertexBufferData.size()) / stride;
	mesh.ibByteOffset = 0;
	mesh.indexBufferRaw = std::move(indexBufferData);
	mesh.vbByteOffset = 0;
	mesh.vertexBufferRaw = std::move(vertexBufferData);
	mesh.aabox = geomBBoxObjectSpace;

	mesh.vbVertexColorOffsetBytes = color0ByteOffset;
	mesh.vbPositionOffsetBytes = 0;
	mesh.vbNormalOffsetBytes = normals0ByteOffset;
	mesh.vbTangetOffsetBytes = tangents0ByteOffset;
	mesh.vbBinormalOffsetBytes = binormals0ByteOffset;
	mesh.vbUVOffsetBytes = UV0ByteOffset;
	mesh.vbBonesIdsBytesOffset = boneIdsByteOffset;
	mesh.vbBonesWeightsByteOffset = boneWeightsByteOffset;
}

void FBXSDKParser::importNodes() {
	for (const auto& pair : m_fbxNode2NodeIndex) {
		FbxNode* fbxNode = pair.first;
		importNodes_singleNode(fbxNode, pair.second);
	}
}

void FBXSDKParser::importNodes_singleNode(FbxNode* fbxNode, int importNodeIndex) {
	Model::Node* node = m_model->nodeAt(importNodeIndex);

	node->name = fbxNode->GetName();

	// http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html?url=files/GUID-10CDD63C-79C1-4F2D-BB28-AD2BE65A02ED.htm,topicNumber=d30e8997
	// http://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html?url=cpp_ref/class_fbx_node.html,topicNumber=cpp_ref_class_fbx_node_html6b73528d-77ae-4781-b04c-da4af82b4a08
	// (see Pivot Management) From FBX SDK docs: Rotation offset (Roff) Rotation pivot (Rp) Pre-rotation (Rpre) Post-rotation (Rpost)
	// Scaling offset (Soff)
	// Scaling pivot (Sp)
	// Geometric translation (Gt)
	// Geometric rotation (Gr)
	// Geometric scaling (Gs)
	// World = ParentWorld * T * Roff * Rp * Rpre * R * Rpost * Rp-1 * Soff * Sp * S * Sp-1

	// FbxDouble3 const fbxScalingOffset = fbxNode->GetScalingOffset(FbxNode::eSourcePivot);
	// FbxDouble3 const fbxScalingPivot = fbxNode->GetScalingPivot(FbxNode::eSourcePivot);

	// FbxDouble3 const fbxRotationOffset = fbxNode->GetRotationOffset(FbxNode::eSourcePivot);
	// FbxDouble3 const fbxRotationPivot = fbxNode->GetRotationPivot(FbxNode::eSourcePivot);

	// FbxDouble3 const fbxPreRotation = fbxNode->GetPreRotation(FbxNode::eSourcePivot);
	// FbxDouble3 const fbxPostRotation = fbxNode->GetPostRotation(FbxNode::eSourcePivot);

	// const FbxDouble3 fbxGeometricTranslation = fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot);
	// const FbxDouble3 fbxGeometricRotation = fbxNode->GetGeometricRotation(FbxNode::eSourcePivot);
	// const FbxDouble3 fbxGeometricScaling = fbxNode->GetGeometricScaling(FbxNode::eSourcePivot);

	// Load the static moment transform.
	//
	// Caution:
	// In order not to introduce a separate variable for the rotation offset we embed it in the node's
	// translation. THIS MAY BE INCORRECT.

	node->staticLocalTransform = transf3DFromFbx(fbxNode->EvaluateLocalTransform(), FbxEuler::eOrderXYZ);

	// [FBX_PARSING_NODE_ATTRIBS]
	// Read the node's attrbiutes. Attributes are something that specifies what the node is and that data does it reference.
	// It could be if the node is used for a bone some parameters of that bone (for example it's length).
	// It could be a mesh and/or material.
	int const attribCount = fbxNode->GetNodeAttributeCount();
	for (const int iAttrib : range_int(attribCount)) {
		fbxsdk::FbxNodeAttribute* const fbxNodeAttrib = fbxNode->GetNodeAttributeByIndex(iAttrib);

		const fbxsdk::FbxNodeAttribute::EType fbxAttriuteType = fbxNodeAttrib->GetAttributeType();
		if (fbxAttriuteType == fbxsdk::FbxNodeAttribute::eSkeleton) {
			// Nothing to do nere.
		} else if (fbxAttriuteType == fbxsdk::FbxNodeAttribute::eMesh) {
			if (fbxsdk::FbxMesh* const fbxMesh = fbxsdk::FbxCast<fbxsdk::FbxMesh>(fbxNodeAttrib)) {
				// Attach the imported mesh to the imported node.
				// A reminder that here we care only about meshes for rendering.
				// Meshes for collisions are not attached to any node.
				fbxsdk::FbxSurfaceMaterial* const fbxSurfMtl = fbxNode->GetMaterial(iAttrib);

				const auto& itrFindMeshIndex = m_fbxMesh2MeshIndex.find(fbxMesh);
				const auto& itrFindMaterialIndex = m_fbxMtl2MtlIndex.find(fbxSurfMtl);

				if (itrFindMeshIndex != m_fbxMesh2MeshIndex.end() && itrFindMaterialIndex != m_fbxMtl2MtlIndex.end()) {
					node->meshAttachments.push_back(Model::MeshAttachment(itrFindMeshIndex->second, itrFindMaterialIndex->second));
				}
			}
		}
	}
}

void FBXSDKParser::importAnimations() {
	struct sge_FbxTime_hasher {
		std::size_t operator()(const fbxsdk::FbxTime& k) const {
			// When I wrote this the FbxTime storage was basically just a long long integer.
			// And it was returned by that function.
			return k.Get();
		}
	};

	using FbxTimeToTransformLut = std::unordered_map<fbxsdk::FbxTime, FbxAMatrix, sge_FbxTime_hasher>;
	std::unordered_map<fbxsdk::FbxNode*, FbxTimeToTransformLut> perNodeLclTransformLUT;

	const auto getNodeTransform = [&perNodeLclTransformLUT](fbxsdk::FbxNode* fbxNode, const fbxsdk::FbxTime& time) -> FbxAMatrix {
		FbxTimeToTransformLut& timeToTransformLUT = perNodeLclTransformLUT[fbxNode];

		// Check if we have cached values for EvaluateLocalTransform at this time.
		// If so use it, else add it to the cache.
		const auto& itrFindTransform = timeToTransformLUT.find(time);
		if (itrFindTransform == timeToTransformLUT.end()) {
			FbxAMatrix lclTransformToCache = fbxNode->EvaluateLocalTransform(time);
			timeToTransformLUT[time] = lclTransformToCache;
			return lclTransformToCache;
		} else {
			return itrFindTransform->second;
		}
	};

	// FBX file format supports multiple animations per file.
	// Each animation is called "Animation Stack" previously they were called "Takes",
	// this is why you see a mixed usage of terms.
	int const animStackCount = m_fbxScene->GetSrcObjectCount<fbxsdk::FbxAnimStack>();
	for (int const iStack : range_int(animStackCount)) {
		// Clear the cache form the previous node.
		fbxsdk::FbxAnimStack* const fbxAnimStack = m_fbxScene->GetSrcObject<fbxsdk::FbxAnimStack>(iStack);
		const char* const animationName = fbxAnimStack->GetName();

		// Set this to current animation stack so our evaluator could use it.
		m_fbxScene->SetCurrentAnimationStack(fbxAnimStack);

		const fbxsdk::FbxTakeInfo* const takeInfo = m_fbxScene->GetTakeInfo(animationName);

		// Take the start and end time of the animation. Offset the keyframes so in the imported animation
		// the animation starts from 0 seconds.
		const float animationStart = (float)takeInfo->mLocalTimeSpan.GetStart().GetSecondDouble();
		const float animationEnd = (float)takeInfo->mLocalTimeSpan.GetStop().GetSecondDouble();
		const float animationDuration = animationEnd - animationStart;

		// Per node keyframes for this animation. They keyframes are in node local space.
		std::map<int, Model::KeyFrames> perNodeKeyFrames;

		// Each stack constist of multiple layers. This abstaction is used by the artist in Maya/Max/ect. to separate the different type
		// of keyframes while animating. This us purely used to keep the animation timeline organized and has no functionally when
		// playing the animation in code.
		int const layerCount = fbxAnimStack->GetMemberCount<fbxsdk::FbxAnimLayer>();
		for (int const iLayer : range_int(layerCount)) {
			fbxsdk::FbxAnimLayer* const fbxAnimLayer = fbxAnimStack->GetMember<fbxsdk::FbxAnimLayer>(iLayer);

			// Read each the animated values for each node on that curve.
			for (const auto& itr : m_fbxNode2NodeIndex) {
				fbxsdk::FbxNode* const fbxNode = itr.first;
				Model::KeyFrames nodeKeyFrames = perNodeKeyFrames[itr.second];

				if (!fbxNode) {
					sgeAssert(false && "Failed to find nodes for importing their animations.");
					continue;
				}

				// Check if the node's translation is animated.
				if (fbxsdk::FbxAnimCurve* const fbxTranslCurve = fbxNode->LclTranslation.GetCurve(fbxAnimLayer, false)) {
					const int keyFramesCount = fbxTranslCurve->KeyGetCount();

					for (int const iKey : range_int(keyFramesCount)) {
						const FbxAnimCurveKey fKey = fbxTranslCurve->KeyGet(iKey);
						const fbxsdk::FbxTime fbxKeyTime = fKey.GetTime();

						const FbxAMatrix localTransform = getNodeTransform(fbxNode, fbxKeyTime);
						fbxsdk::FbxVector4 fbxPos = localTransform.GetT();

						// Convert the keyframe to our own format and save it.
						const float keyTimeSeconds = (float)fbxKeyTime.GetSecondDouble() - animationStart;
						vec3f const position = vec3f((float)fbxPos.mData[0], (float)fbxPos.mData[1], (float)fbxPos.mData[2]);
						nodeKeyFrames.positionKeyFrames[keyTimeSeconds] = position;
					}
				}

				// Check if the node's rotation is animated.
				if (fbxsdk::FbxAnimCurve* const fbxRotationCurve = fbxNode->LclRotation.GetCurve(fbxAnimLayer, false)) {
					const int keyFramesCount = fbxRotationCurve->KeyGetCount();
					for (int const iKey : range_int(keyFramesCount)) {
						fbxsdk::FbxAnimCurveKey const fKey = fbxRotationCurve->KeyGet(iKey);
						fbxsdk::FbxTime const fbxKeyTime = fKey.GetTime();

						const FbxAMatrix localTransform = getNodeTransform(fbxNode, fbxKeyTime);
						fbxsdk::FbxVector4 const fRotation = localTransform.GetR();

						// Convert the keyframe to our own format and save it.
						float const keyTimeSeconds = (float)fbxKeyTime.GetSecondDouble() - animationStart;
						quatf const rotation = quatFromFbx(FbxEuler::eOrderXYZ, fRotation);
						nodeKeyFrames.rotationKeyFrames[keyTimeSeconds] = rotation;
					}
				}

				// Check if the node's scaling is animated.
				if (fbxsdk::FbxAnimCurve* const fbxScalingCurve = fbxNode->LclScaling.GetCurve(fbxAnimLayer, false)) {
					const int keyFramesCount = fbxScalingCurve->KeyGetCount();
					for (int const iKey : range_int(keyFramesCount)) {
						fbxsdk::FbxAnimCurveKey const fKey = fbxScalingCurve->KeyGet(iKey);
						fbxsdk::FbxTime const fbxKeyTime = fKey.GetTime();

						const FbxAMatrix localTransform = getNodeTransform(fbxNode, fbxKeyTime);
						fbxsdk::FbxVector4 const fRotation = localTransform.GetS();

						// Convert the keyframe to our own format and save it.
						float const keyTimeSeconds = (float)fbxKeyTime.GetSecondDouble() - animationStart;
						fbxsdk::FbxVector4 const fScaling = localTransform.GetS();
						vec3f const scaling = vec3f((float)fScaling.mData[0], (float)fScaling.mData[1], (float)fScaling.mData[2]);
						nodeKeyFrames.scalingKeyFrames[keyTimeSeconds] = scaling;
					}
				}
			} // End for each node loop.
		}     // End for each layer loop.

		// Add the animation to the model.
		const int newAnimIndex = m_model->makeNewAnim();

		*(m_model->getAnimation(newAnimIndex)) = Model::Animation(animationName, animationDuration, perNodeKeyFrames);
	}
}

void FBXSDKParser::importCollisionGeometry() {
	// Convex hulls.
	for (const auto& itrFbxMeshInstantiations : m_collision_ConvexHullMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrFbxMeshInstantiations.first;

		Model::CollisionMesh collisionMeshObjectSpace = fbxMeshToCollisionMesh(fbxMesh);

		if (collisionMeshObjectSpace.indices.size() % 3 == 0) {
			// For every instance transform the vertices to model (or in this context world space).
			for (int const iInstance : range_int(int(itrFbxMeshInstantiations.second.size()))) {
				std::vector<vec3f> verticesWS = collisionMeshObjectSpace.vertices;

				mat4f const n2w = m_collision_transfromCorrection.toMatrix() * itrFbxMeshInstantiations.second[iInstance].toMatrix();
				for (vec3f& v : verticesWS) {
					v = mat_mul_pos(n2w, v);
				}

				m_model->m_convexHulls.emplace_back(Model::CollisionMesh{std::move(verticesWS), collisionMeshObjectSpace.indices});
			}
		} else {
			printf("Invalid collision geometry '%s'!\n", fbxMesh->GetName());
		}
	}

	// Triangle meshes.
	for (const auto& itrFbxMeshInstantiations : m_collision_BvhTriMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrFbxMeshInstantiations.first;

		Model::CollisionMesh collisionMeshObjectSpace = fbxMeshToCollisionMesh(fbxMesh);

		if (collisionMeshObjectSpace.indices.size() % 3 == 0) {
			// For every instance transform the vertices to model (or in this context world space).
			for (int const iInstance : range_int(int(itrFbxMeshInstantiations.second.size()))) {
				std::vector<vec3f> verticesWS = collisionMeshObjectSpace.vertices;

				mat4f const n2w = m_collision_transfromCorrection.toMatrix() * itrFbxMeshInstantiations.second[iInstance].toMatrix();
				for (vec3f& v : verticesWS) {
					v = mat_mul_pos(n2w, v);
				}

				m_model->m_concaveHulls.emplace_back(Model::CollisionMesh{std::move(verticesWS), collisionMeshObjectSpace.indices});
			}
		} else {
			printf("Invalid collision geometry '%s'!\n", fbxMesh->GetName());
		}
	}

	// Boxes
	for (const auto& itrBoxInstantiations : m_collision_BoxMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrBoxInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		AABox3f bbox;
		for (int const iVert : range_int(fbxMesh->GetControlPointsCount())) {
			bbox.expand(vec3fFromFbx(fbxMesh->GetControlPointAt(iVert)));
		}

		for (int const iInstance : range_int(int(itrBoxInstantiations.second.size()))) {
			transf3d n2w = m_collision_transfromCorrection * itrBoxInstantiations.second[iInstance];
			n2w.p += bbox.center();
			m_model->m_collisionBoxes.push_back(
			    Model::CollisionShapeBox{std::string(fbxMesh->GetNode()->GetName()), n2w, bbox.halfDiagonal()});
		}
	}

	// Capsules.
	for (const auto& itrBoxInstantiations : m_collision_CaplsuleMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrBoxInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		AABox3f bbox;
		for (int const iVert : range_int(fbxMesh->GetControlPointsCount())) {
			bbox.expand(vec3fFromFbx(fbxMesh->GetControlPointAt(iVert)));
		}

		vec3f const halfDiagonal = bbox.halfDiagonal();
		vec3f const ssides = halfDiagonal.getSorted();
		float halfHeight = ssides[0];
		float const radius = maxOf(ssides[1], ssides[2]);

		if_checked(2.f * radius <= halfHeight) { printf("ERROR: Invalid capsule buonding box.\n"); }

		halfHeight -= radius;

		for (int const iInstance : range_int(int(itrBoxInstantiations.second.size()))) {
			transf3d n2w = m_collision_transfromCorrection * itrBoxInstantiations.second[iInstance];
			n2w.p += bbox.center();
			m_model->m_collisionCapsules.push_back(
			    Model::CollisionShapeCapsule{std::string(fbxMesh->GetNode()->GetName()), n2w, halfHeight, radius});
		}
	}

	// Cylinders.
	for (const auto& itrCylinderInstantiations : m_collision_CylinderMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrCylinderInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		AABox3f bboxOS;
		for (int const iVert : range_int(fbxMesh->GetControlPointsCount())) {
			bboxOS.expand(vec3fFromFbx(fbxMesh->GetControlPointAt(iVert)));
		}

		vec3f const halfDiagonal = bboxOS.halfDiagonal();
		for (int const iInstance : range_int(int(itrCylinderInstantiations.second.size()))) {
			transf3d const n2w = m_collision_transfromCorrection * itrCylinderInstantiations.second[iInstance];

			m_model->m_collisionCylinders.push_back(
			    Model::CollisionShapeCylinder{std::string(fbxMesh->GetNode()->GetName()), n2w, halfDiagonal});
		}
	}

	// Spheres.
	for (const auto& itrSphereInstantiations : m_collision_SphereMeshes) {
		fbxsdk::FbxMesh* const fbxMesh = itrSphereInstantiations.first;

		// CAUTION: The code assumes that the mesh vertices are untoched.
		AABox3f bboxOS;
		for (int const iVert : range_int(fbxMesh->GetControlPointsCount())) {
			bboxOS.expand(vec3fFromFbx(fbxMesh->GetControlPointAt(iVert)));
		}

		vec3f const halfDiagonal = bboxOS.halfDiagonal();
		float const radius = halfDiagonal.getSorted().x;
		for (int const iInstance : range_int(int(itrSphereInstantiations.second.size()))) {
			transf3d const n2w = m_collision_transfromCorrection * itrSphereInstantiations.second[iInstance];

			m_model->m_collisionSpheres.push_back(Model::CollisionShapeSphere{std::string(fbxMesh->GetNode()->GetName()), n2w, radius});
		}
	}
}

} // namespace sge
