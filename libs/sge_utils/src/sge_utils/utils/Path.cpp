#include <filesystem>

#include "Path.h"

#include "common.h"

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <ShlObj.h>
#include <Windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace sge {

std::string extractFileNameWithExt(const char* filepath) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	int p = -1; // the position of the last point
	int d = -1; // the position of the last slash

	for (int t = 0; filepath[t] != '\0'; ++t) {
		if (filepath[t] == '.')
			p = t;
		if (filepath[t] == '/' || filepath[t] == '\\')
			d = t;
	}

	// Check if the input is actually a filename.
	if (d == -1) {
		return filepath;
	}

	// Only things with extension are counted as files here.
	if (p < d) {
		return std::string();
	}

	// Check if there is a file at all.
	if (filepath[d + 1] == '\0') {
		return std::string();
	}

	return std::string(filepath + d + 1);
}


std::string extractFileDir(const char* filepath, const bool includeSlashInResult) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	int d = -1; // the position of the last slash

	for (int t = 0; filepath[t] != '\0'; ++t) {
		if (filepath[t] == '/' || filepath[t] == '\\')
			d = t;
	}

	if (d == -1) {
		return std::string();
	}

	return std::string(filepath, filepath + d + (includeSlashInResult ? 1 : 0));
}

std::string removeFileExtension(const char* filepath) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	int p = -1;
	for (int t = 0; filepath[t] != '\0'; ++t) {
		if (filepath[t] == '.')
			p = t;
	}

	if (p == -1)
		return std::string(filepath);
	return std::string(filepath, filepath + p);
}

std::string extractFileExtension(const char* filepath) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	int dotLocation = -1; // The index of the last found dot.
	for (int t = 0; filepath[t] != '\0'; ++t) {
		if (filepath[t] == '.') {
			dotLocation = t;
		}
	}

	if (dotLocation == -1)
		return std::string();

	std::string retval = std::string(filepath + dotLocation + 1);

	for (auto& c : retval) {
		c = char(tolower(c));
	}

	return retval;
}

std::string replaceExtension(const char* const filepath, const char* const ext) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	int dotLocation = -1; // The index of the last found dot.
	for (int t = 0; filepath[t] != '\0'; ++t) {
		if (filepath[t] == '.')
			dotLocation = t;
	}

	if (dotLocation == -1)
		filepath;

	std::string result(filepath, filepath + dotLocation + 1);
	if (ext != nullptr) {
		result += ext;
	}

	return result;
}

std::string currentWorkingDir() {
#if defined WIN32
	char currentDir[1024];
	if (GetCurrentDirectoryA(ARRAYSIZE(currentDir), currentDir) == 0) {
		sgeAssert(false);
		return std::string();
	}

	return std::string(currentDir);
#else

	char currentDir[1024];
	if (!getcwd(currentDir, SGE_ARRSZ(currentDir))) {
		sgeAssert(false);
		return std::string();
	}

	return std::string(currentDir);
#endif
}

std::string canonizePathRespectOS(const char* filepath) {
	if (filepath == NULL) {
		sgeAssert(false);
		return std::string();
	}

	std::string result = filepath;

#if defined _WIN32
	for (int t = 0; t < (int)result.size(); ++t) {
		char& ch = result[t];
		if (ch == '\\') {
			if (t == 2 && result[1] == ':') {
			} else {
				ch = '/';
			}
		}
	}
#endif

	// remove the redundand slashes.
	auto const isSlash = [](const char ch) -> bool {
#if defined _WIN32
		return ch == '/' || ch == '\\';
#else
		return ch == '/';
#endif
	};

	for (int t = 0; t < (int)result.size() - 1; ++t) {
		if (isSlash(result[t]) && isSlash(result[t + 1])) {
			result.erase(result.begin() + t);
			t = t - 1;
		} else if (t != 0 && result[t] == '.' && result[t + 1] == '/') {
			// Check if this is back one dir "../" if so, skip it.
			const bool isBackOneDir = (t - 1) >= 0 && result[t - 1] == '.';
			if (isBackOneDir == false) {
				result.erase(t, 2);
				t = t - 1;
			}
		}
	}

	return result;
}

bool isPathAbsolute(const char* const cpath) {
	std::filesystem::path path(cpath);
	return path.is_absolute();
}

std::string relativePathTo(const char* path, const char* base) {
	try {
		std::error_code ec;
		return std::filesystem::proximate(std::filesystem::path(path), std::filesystem::path(base)).string();
	} catch (...) {
		return std::string();
	}
}

std::string relativePathToCwd(const char* path) {
	try {
		std::error_code ec;
		return std::filesystem::proximate(std::filesystem::path(path), std::filesystem::current_path()).string();
	} catch (...) {
		return std::string();
	}
}

std::string relativePathToCwd(const std::string& path) {
	return relativePathToCwd(path.c_str());
}

std::string relativePathToCwdCanoize(const std::string& path) {
	return canonizePathRespectOS(relativePathToCwd(path));
}

std::string absoluteOf(const char* const path) {
	try {
		return std::filesystem::absolute(std::filesystem::path(path)).string();
	} catch (...) {
		return std::string();
	}
}


void createDirectory(const char* const path) {
	std::filesystem::create_directories(std::filesystem::path(path));
	return;
#if 0
#ifdef WIN32
	//SHCreateDirectoryExA(NULL, path, NULL); // attempt 1
	//CreateDirectoryA(path, NULL); // attempt 2
	const std::string cmd = string_format("mkdir -p \"%s\" > NUL", path);
	system(cmd.c_str());
#else
	const std::string cmd = string_format("mkdir -p \"%s\"", path);
	system(cmd.c_str());
#endif
#endif
}

void copyFile(const char* srcFile, const char* destFile) {
#ifdef WIN32
	[[maybe_unused]] BOOL succeeded = CopyFileA(srcFile, destFile, FALSE);
	sgeAssert(succeeded != 0);
#else
	// const std::string cmd = string_format("cp \"%s\" \"%s\"", srcFile, destFile);
	// system(cmd.c_str());
	try {
		std::filesystem::copy(srcFile, destFile);
	} catch (...) {
	}
#endif
}

} // namespace sge
