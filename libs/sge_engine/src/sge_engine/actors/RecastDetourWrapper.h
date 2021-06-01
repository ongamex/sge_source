#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "Recast.h"

#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/basetypes.h"

#include <string>

namespace sge {

/// As Recast is build in C it is quite tedous and error-prone to do the memory management manually.
/// So, to workaround this class should be used instead. It automatically calls m_allocator/free by default.
template <typename T, T* (*TAlloc)(), void (*TFree)(T*)>
struct RecastObject : public Noncopyable {
	RecastObject() { createNew(); }
	~RecastObject() { freeExisting(); }

	void createNew() {
		freeExisting();
		object = TAlloc();
	}

	void freeExisting() {
		if (object != nullptr) {
			TFree(object);
			object = nullptr;
		}
	}

	T& ref() {
		sgeAssert(object);
		return *object;
	}

	T* operator->() {
		sgeAssert(object);
		return object;
	}

	const T* operator->() const {
		sgeAssert(object);
		return object;
	}


	T* object = nullptr;
};

#define SGE_CREATE_RECAST_WRAPPER(type, allocFn, freeFn) using type##Wrapper = RecastObject<type, allocFn, freeFn>;

SGE_CREATE_RECAST_WRAPPER(rcHeightfield, rcAllocHeightfield, rcFreeHeightField)
SGE_CREATE_RECAST_WRAPPER(rcCompactHeightfield, rcAllocCompactHeightfield, rcFreeCompactHeightfield)
SGE_CREATE_RECAST_WRAPPER(rcContourSet, rcAllocContourSet, rcFreeContourSet)
SGE_CREATE_RECAST_WRAPPER(rcPolyMesh, rcAllocPolyMesh, rcFreePolyMesh)
SGE_CREATE_RECAST_WRAPPER(rcPolyMeshDetail, rcAllocPolyMeshDetail, rcFreePolyMeshDetail)
SGE_CREATE_RECAST_WRAPPER(dtNavMesh, dtAllocNavMesh, dtFreeNavMesh)
SGE_CREATE_RECAST_WRAPPER(dtNavMeshQuery, dtAllocNavMeshQuery, dtFreeNavMeshQuery)

#undef SGE_CREATE_RECAST_WRAPPER

} // namespace sge
