#pragma once

namespace sge {

struct DLLHandler {
	DLLHandler() = default;
	~DLLHandler() { unload(); }

	/// @brief Load a dll/shared library from the specified path.
	/// @return True if the loading for successful.
	bool load(const char* const path);

	/// @brief Load a dll/shared library from the specified path.
	/// The path should not include any extension (including the '.'). The function will guess
	/// the extension of the library based on the current operating system.
	/// @return True if the loading for successful.
	bool loadNoExt(const char* pPath);

	void unload();

	/// @brief Searches for the specified symbol name, and returns a pointer to it.
	/// @param [in] proc the name of the symbol.
	/// @return the pointer to the symbol or nullptr if the failed.
	void* getProcAdress(const char* const proc);

	/// @brief Returns true if there there is a loaded library.
	bool isLoaded() { return m_nativeHandle != nullptr; }

  private:
	void* m_nativeHandle = nullptr;
};

} // namespace sge
