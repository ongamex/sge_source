#if defined(WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include "DLLHandler.h"
#include "sge_utils/sge_utils.h"
#include <string>

namespace sge {

bool DLLHandler::load(const char* const path) {
	unload();

#if defined(WIN32)
	HMODULE hMod = LoadLibraryA(path);
	static_assert(sizeof(m_nativeHandle) == sizeof(hMod), "DLL Handle size int correct.");
	m_nativeHandle = hMod;
	return m_nativeHandle != NULL;
#else
	m_nativeHandle = dlopen(path, RTLD_NOW);
	return m_nativeHandle != nullptr;
#endif
}

bool DLLHandler::loadNoExt(const char* pPath) {
	std::string filename = pPath;
#if defined(WIN32)
	filename += ".dll";
#else
	filename += ".so";
#endif

	return load(filename.c_str());
}

void DLLHandler::unload() {
#if defined(WIN32)
	if (m_nativeHandle) {
		[[maybe_unused]] BOOL success = FreeLibrary((HMODULE)m_nativeHandle);
		sgeAssert(success);
	}
	m_nativeHandle = nullptr;
#else
	if (m_nativeHandle != nullptr) {
		dlclose(m_nativeHandle);
		m_nativeHandle = nullptr;
	}
#endif
}

void* DLLHandler::getProcAdress(const char* const proc) {
#if defined(WIN32)
	if (m_nativeHandle == nullptr)
		return nullptr;

	return GetProcAddress((HMODULE)m_nativeHandle, proc);
#else
	if (m_nativeHandle == nullptr)
		return nullptr;

	return dlsym(m_nativeHandle, proc);
#endif
}

} // namespace sge
