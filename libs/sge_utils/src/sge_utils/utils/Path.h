#pragma once

#include "sge_utils/sge_utils.h"
#include <string>

namespace sge {

// DISCLAIMER:

// Due to the lack of <filesystem> in the current version of C++
// these functions were written. They are an approximation of
// the reality sinice it makes a few assumtions:
//
// UNC paths are not handled at all.
// '\' charracter cannot be used in filenames in Linux.
// UTF-8 support is in mind but I;m not sure if there is still something to handle.
//
// Sometimes I wonder if I should use boost or not.

// Extracts the filename form the input path.
std::string extractFileNameWithExt(const char* filepath);

// Extracts the directroy of a given file.
std::string extractFileDir(const char* filepath, const bool includeSlashInResult);

// Extracts everything but the last dot and the extension
std::string removeFileExtension(const char* filepath);

// Returns the extraction of the specified file. The letters in the extension are lowered.
std::string extractFileExtension(const char* filepath);

//
std::string replaceExtension(const char* const filepath, const char* const ext);

// Convets the path to the "cacnonical" form.
// Windows Only: replaced \ with /  only when it is not used for things like X:\ .
// Removes rendundand slashes (again it is OS dependand).
std::string canonizePathRespectOS(const char* filepath);
inline std::string canonizePathRespectOS(const std::string& filepath) {
	return canonizePathRespectOS(filepath.c_str());
}

// Returns the current working directory.
std::string currentWorkingDir();

// Returns true if a path is absolute.
bool isPathAbsolute(const char* const path);

std::string relativePathTo(const char* path, const char* base);
std::string relativePathToCwd(const char* path);
std::string relativePathToCwd(const std::string& path);

std::string relativePathToCwdCanoize(const std::string& path);

std::string absoluteOf(const char* const path);

//
void createDirectory(const char* const path);

//
void copyFile(const char* srcFile, const char* destFile);

} // namespace sge
