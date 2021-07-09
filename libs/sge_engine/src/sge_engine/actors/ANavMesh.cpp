#include "ANavMesh.h"
#include "ANavMesh_btCollisionShapeToTriangles.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "IconsForkAwesome/IconsForkAwesome.h"
#include "Recast.h"
#include "RecastDetourWrapper.h"
#include "sge_core/ICore.h"
#include "sge_engine/GameWorld.h"
#include "sge_engine/traits/TraitRigidBody.h"
#include "sge_utils/utils/ScopeGuard.h"

#include "sge_engine/windows/PropertyEditorWindow.h"

namespace sge {

struct ABlockingObstacle;
struct AStaticObstacle;

// clang-format off
DefineTypeId(NavMeshBuildSets, 20'05'10'0001);
DefineTypeId(ANavMesh, 20'05'10'0002);

ReflBlock() {
	ReflAddType(NavMeshBuildSets)
		ReflMemberNamed(NavMeshBuildSets, cellXZSize, "cellXZSize")
		ReflMemberNamed(NavMeshBuildSets, cellYSize, "cellYSize")
		ReflMemberNamed(NavMeshBuildSets, climbableSlopeAngle, "climbableSlopeAngle").addMemberFlag(MFF_FloatAsDegrees)
		ReflMemberNamed(NavMeshBuildSets, cimbableStairHeight, "cimbableStairHeight")
		ReflMemberNamed(NavMeshBuildSets, minRoomHeight, "minRoomHeight")
		ReflMemberNamed(NavMeshBuildSets, agentRadius, "agentRadius")
		ReflMemberNamed(NavMeshBuildSets, agentHeight, "agentHeight")
	;

	ReflAddActor(ANavMesh)
		ReflMemberNamed(ANavMesh, m_buildSettings, "buildSettings")
		ReflMember(ANavMesh, m_typesToUse)
	;

}
// clang-format on


//--------------------------------------------------------
// ANavMesh
//--------------------------------------------------------
void ANavMesh::create() {
	registerTrait(m_traitViewportIcon);
	registerTrait(static_cast<IActorCustomAttributeEditorTrait&>(*this));
	m_traitViewportIcon.setTexture("assets/editor/textures/icons/obj/ANavMesh.png", true);

	m_typesToUse.push_back(sgeTypeId(AStaticObstacle));
	m_typesToUse.push_back(sgeTypeId(ABlockingObstacle));

	onWorldLoadedCBHandle = GameObject::getWorld()->onWorldLoaded.subscribe([this]() {
		build();
		onWorldLoadedCBHandle.unsubscribe();
	});
}

AABox3f ANavMesh::getBBoxOS() const {
	return AABox3f::getFromHalfDiagonal(vec3f(1.f));
}

void ANavMesh::doAttributeEditor(GameInspector* inspector) {
	MemberChain chain;

	chain.clear();
	chain.add(typeLib().find<ANavMesh>()->findMember(&ANavMesh::m_buildSettings));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	chain.add(typeLib().find<ANavMesh>()->findMember(&ANavMesh::m_typesToUse));
	ProperyEditorUIGen::doMemberUI(*inspector, this, chain);
	chain.pop();

	if (ImGui::Button(ICON_FK_REFRESH " Build...")) {
		build();
	}
}

bool ANavMesh::findPath(std::vector<vec3f>& outPath,
                        const vec3f& startPos,
                        const vec3f& targetEndPos,
                        const vec3f nearestPointSearchHalfDiagonal) {
	const int kMaxPathPolyCount = 256; // TODO: Fine tune this variable.

	outPath.clear();

	if (m_detourNavMeshQuery.object == nullptr || m_detourNavMeshQuery->getNodePool() == nullptr) {
		sgeAssert(false && "ANavMesh isn't initialzied yet!");
		return false;
	}

	dtQueryFilter queryPolyFilter;

	dtPolyRef startPolyRef = 0;
	m_detourNavMeshQuery->findNearestPoly(startPos.data, nearestPointSearchHalfDiagonal.data, &queryPolyFilter, &startPolyRef, 0);

	dtPolyRef endPolyRef = 0;
	m_detourNavMeshQuery->findNearestPoly(targetEndPos.data, nearestPointSearchHalfDiagonal.data, &queryPolyFilter, &endPolyRef, 0);

	if (startPolyRef == 0 || endPolyRef == 0) {
		return false; // No path could be found.
	}

	dtPolyRef polygonsAlongPath[kMaxPathPolyCount];
	int numPolygonsAlongPath = 0;
	m_detourNavMeshQuery->findPath(startPolyRef, endPolyRef, startPos.data, targetEndPos.data, &queryPolyFilter, polygonsAlongPath,
	                               &numPolygonsAlongPath, SGE_ARRSZ(polygonsAlongPath));

	if (numPolygonsAlongPath > 0) {
		outPath.resize(kMaxPathPolyCount);
		// In case of partial path, make sure the end point is clamped to the last polygon.
		// Partial path means that the points isn't reachable, but the library generated a path which
		// takes you closer to the end point.
		vec3f acutualEndPos = targetEndPos;
		if (polygonsAlongPath[numPolygonsAlongPath - 1] != endPolyRef) {
			m_detourNavMeshQuery->closestPointOnPoly(polygonsAlongPath[numPolygonsAlongPath - 1], targetEndPos.data, acutualEndPos.data, 0);
		}

		int numPointsInPath = 0;
		const int streightPathOptions = 0;
		unsigned char straightPathFlags[kMaxPathPolyCount];
		m_detourNavMeshQuery->findStraightPath(startPos.data, acutualEndPos.data, polygonsAlongPath, numPolygonsAlongPath,
		                                       (float*)outPath.data(), straightPathFlags, nullptr, &numPointsInPath, kMaxPathPolyCount,
		                                       streightPathOptions);

		outPath.resize(numPointsInPath);
		return true;
	}

	return false;
}


void ANavMesh::update(const GameUpdateSets& UNUSED(updateSets)) {
}

void ANavMesh::build() {
	// Generate triangles representing each object that could be used as a walkable area by the navmesh.
	std::vector<vec3f> trianglesVerticesWorldSpace;
	std::vector<int> trianglesIndices;

	for (const TypeId actorType : m_typesToUse) {
		const std::vector<GameObject*>* pAllObjsOfType = GameObject::getWorld()->getObjects(actorType);
		if (pAllObjsOfType == nullptr) {
			continue;
		}

		for (const GameObject* const object : *GameObject::getWorld()->getObjects(actorType)) {
			const TraitRigidBody* const traitRb = getTrait<TraitRigidBody>(object);
			if (traitRb != nullptr && traitRb->getRigidBody()->isValid()) {
				const btTransform bodyWorldTransform = traitRb->m_rigidBody.getBulletRigidBody()->getWorldTransform();
				const btCollisionShape* const shape = traitRb->m_rigidBody.getBulletRigidBody()->getCollisionShape();
				bulletCollisionShapeToTriangles(shape, bodyWorldTransform, trianglesVerticesWorldSpace, trianglesIndices);
			}
		}
	}

	const int numTriangles = int(trianglesIndices.size()) / 3;

	if (numTriangles == 0) {
		SGE_DEBUG_WAR("NavMesh did not find any triangles to be used for building the navmesh!");
		return;
	}

	// Generate the navmesh itself.
	rcConfig recastCfg;

	memset(&recastCfg, 0, sizeof(recastCfg));
	recastCfg.cs = m_buildSettings.cellXZSize;
	recastCfg.ch = m_buildSettings.cellYSize;
	recastCfg.walkableSlopeAngle = rad2deg(m_buildSettings.climbableSlopeAngle);
	recastCfg.walkableHeight = (int)ceilf(m_buildSettings.minRoomHeight / recastCfg.ch);
	recastCfg.walkableClimb = (int)floorf(m_buildSettings.cimbableStairHeight / recastCfg.ch);
	recastCfg.walkableRadius = (int)ceilf(m_buildSettings.agentRadius / recastCfg.cs);
	recastCfg.maxEdgeLen = (int)(m_buildSettings.polygonsMaxEdgeLength / recastCfg.cs); // Why does this exists?
	recastCfg.maxSimplificationError = 1.3f;                                            // What is a good default?
	recastCfg.minRegionArea = (int)rcSqr(8);                                            // Note: area = size*size // WTF is this?
	recastCfg.mergeRegionArea = (int)rcSqr(20);                                         // Note: area = size*size // WTF is this?
	recastCfg.maxVertsPerPoly = (int)6;                                                 // Why does this exists?
	recastCfg.detailSampleDist = 6.f < 0.9f ? 0 : recastCfg.cs * 6.f;                   // WTF is this?
	recastCfg.detailSampleMaxError = 1.f;                                               // WTF is this?

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	AABox3f navMeshBBox = getBBoxOS().getTransformed(getTransformMtx());

	recastCfg.bmin[0] = navMeshBBox.min.x;
	recastCfg.bmin[1] = navMeshBBox.min.y;
	recastCfg.bmin[2] = navMeshBBox.min.z;

	recastCfg.bmax[0] = navMeshBBox.max.x;
	recastCfg.bmax[1] = navMeshBBox.max.y;
	recastCfg.bmax[2] = navMeshBBox.max.z;

	rcCalcGridSize(recastCfg.bmin, recastCfg.bmax, recastCfg.cs, &recastCfg.width, &recastCfg.height);
	rcHeightfieldWrapper recastHeightFiled;
	rcContext recastLogging = rcContext(false);

	if (!rcCreateHeightfield(&recastLogging, recastHeightFiled.ref(), recastCfg.width, recastCfg.height, recastCfg.bmin, recastCfg.bmax,
	                         recastCfg.cs, recastCfg.ch)) {
		sgeAssert(false);
		return;
	}

	// Mark all walkable triangles.
	std::vector<unsigned char> recastPerTriangleFlags(numTriangles, 0);
	rcMarkWalkableTriangles(&recastLogging, recastCfg.walkableSlopeAngle, (float*)trianglesVerticesWorldSpace.data(),
	                        int(trianglesVerticesWorldSpace.size()), trianglesIndices.data(), numTriangles, recastPerTriangleFlags.data());

	// Voxelize the triangles.
	if (!rcRasterizeTriangles(&recastLogging, (float*)trianglesVerticesWorldSpace.data(), int(trianglesVerticesWorldSpace.size()),
	                          trianglesIndices.data(), recastPerTriangleFlags.data(), numTriangles, recastHeightFiled.ref(),
	                          recastCfg.walkableClimb)) {
		sgeAssert(false);
	}

	// Filter walkables surfaces.
	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(&recastLogging, recastCfg.walkableClimb, recastHeightFiled.ref());
	rcFilterLedgeSpans(&recastLogging, recastCfg.walkableHeight, recastCfg.walkableClimb, recastHeightFiled.ref());
	rcFilterWalkableLowHeightSpans(&recastLogging, recastCfg.walkableHeight, recastHeightFiled.ref());

	rcCompactHeightfieldWrapper compactHeightField;
	if (!rcBuildCompactHeightfield(&recastLogging, recastCfg.walkableHeight, recastCfg.walkableClimb, recastHeightFiled.ref(),
	                               compactHeightField.ref())) {
		sgeAssert(false);
	}

	// Clean-up the height field as we aren't going to use it anymore.
	recastHeightFiled.freeExisting();

	if (!rcErodeWalkableArea(&recastLogging, recastCfg.walkableRadius, compactHeightField.ref())) {
		sgeAssert(false);
	}

	// Here I've skipped a step where we mark different area types

	// Partition the walkable surface into simple regions without holes.
	if (!rcBuildDistanceField(&recastLogging, compactHeightField.ref())) {
		sgeAssert(false);
	}

	if (!rcBuildRegions(&recastLogging, compactHeightField.ref(), 0, recastCfg.minRegionArea, recastCfg.mergeRegionArea)) {
		sgeAssert(false);
	}

	rcContourSetWrapper contourSet;
	if (!rcBuildContours(&recastLogging, compactHeightField.ref(), recastCfg.maxSimplificationError, recastCfg.maxEdgeLen,
	                     contourSet.ref())) {
		sgeAssert(false);
	}

	// Build the recast poly mesh.
	m_recastPolyMesh.createNew();
	if (!rcBuildPolyMesh(&recastLogging, contourSet.ref(), recastCfg.maxVertsPerPoly, m_recastPolyMesh.ref())) {
		sgeAssert(false);
	}

	rcPolyMeshDetailWrapper recastDetailMesh;
	if (!rcBuildPolyMeshDetail(&recastLogging, m_recastPolyMesh.ref(), compactHeightField.ref(), recastCfg.detailSampleDist,
	                           recastCfg.detailSampleMaxError, recastDetailMesh.ref())) {
		sgeAssert(false);
	}

	// Mark all resulting polygons as walkable
	for (int iPoly = 0; iPoly < m_recastPolyMesh->npolys; ++iPoly) {
		m_recastPolyMesh->areas[iPoly] = RC_WALKABLE_AREA;
		m_recastPolyMesh->flags[iPoly] = 1;
	}

	// Build the path-finding mesh for Detour.
	if_checked(recastCfg.maxVertsPerPoly <= DT_VERTS_PER_POLYGON) {
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = m_recastPolyMesh->verts;
		params.vertCount = m_recastPolyMesh->nverts;
		params.polys = m_recastPolyMesh->polys;
		params.polyAreas = m_recastPolyMesh->areas;
		params.polyFlags = m_recastPolyMesh->flags;
		params.polyCount = m_recastPolyMesh->npolys;
		params.nvp = m_recastPolyMesh->nvp;
		params.detailMeshes = recastDetailMesh->meshes;
		params.detailVerts = recastDetailMesh->verts;
		params.detailVertsCount = recastDetailMesh->nverts;
		params.detailTris = recastDetailMesh->tris;
		params.detailTriCount = recastDetailMesh->ntris;
		// params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		// params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		// params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		// params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		// params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		// params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		// params.offMeshConCount = m_geom->getOffMeshConnectionCount();
		params.walkableHeight = m_buildSettings.agentHeight;
		params.walkableRadius = m_buildSettings.agentRadius;
		params.walkableClimb = m_buildSettings.cimbableStairHeight;
		rcVcopy(params.bmin, m_recastPolyMesh->bmin);
		rcVcopy(params.bmax, m_recastPolyMesh->bmax);
		params.cs = recastCfg.cs;
		params.ch = recastCfg.ch;
		params.buildBvTree = true;

		unsigned char* navData = nullptr;
		int navDataSize = 0;
		if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
			dtFree(navData); // If the function succeedes the bookkeeping of the data is done with m_detourNavMesh
			sgeAssert(false);
		}

		m_detourNavMesh.createNew();
		[[maybe_unused]] dtStatus status = m_detourNavMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		m_detourNavMeshQuery.createNew();
		m_detourNavMeshQuery->init(m_detourNavMesh.object, 2048); // TODO: Why 2048?
	}

	// Build the debug draw mesh.
	{
		const int nvp = m_recastPolyMesh->nvp;
		const float cs = m_recastPolyMesh->cs;
		const float ch = m_recastPolyMesh->ch;
		const vec3f orig = vec3f(m_recastPolyMesh->bmin[0], m_recastPolyMesh->bmin[1], m_recastPolyMesh->bmin[2]);

		m_debugDrawNavMeshTriListWs.clear();

		for (int iPoly = 0; iPoly < m_recastPolyMesh->npolys; ++iPoly) {
			const unsigned short* p = &m_recastPolyMesh->polys[iPoly * nvp * 2];

			unsigned short vi[3];
			for (int j = 2; j < nvp; ++j) {
				if (p[j] == RC_MESH_NULL_IDX) {
					break;
				}
				vi[0] = p[0];
				vi[1] = p[j - 1];
				vi[2] = p[j];
				for (int k = 0; k < 3; ++k) {
					const unsigned short* v = &m_recastPolyMesh->verts[vi[k] * 3];
					const float x = orig[0] + v[0] * cs;
					const float y = orig[1] + (v[1] + 1) * ch;
					const float z = orig[2] + v[2] * cs;
					m_debugDrawNavMeshTriListWs.push_back(vec3f(x, y, z));
				}
			}
		}
	}

	m_debugDrawNavMeshBuildTriListWs.clear();
	for (int iTri = 0; iTri < numTriangles; iTri++) {
		m_debugDrawNavMeshBuildTriListWs.push_back(trianglesVerticesWorldSpace[trianglesIndices[iTri * 3 + 0]]);
		m_debugDrawNavMeshBuildTriListWs.push_back(trianglesVerticesWorldSpace[trianglesIndices[iTri * 3 + 1]]);
		m_debugDrawNavMeshBuildTriListWs.push_back(trianglesVerticesWorldSpace[trianglesIndices[iTri * 3 + 2]]);
	}
}

} // namespace sge
