#pragma once

#include "FileStream.h"
#include "sge_utils/utils/timer.h"
#include <set>

namespace sge {

struct FileWatcher {
	FileWatcher() = default;
	FileWatcher(const char* const filenameToWatch) {
		*this = FileWatcher();
		initilize(filenameToWatch);
	}

	// Returns true if the file exists.
	bool initilize(const char* const filenameToWatch) {
		*this = FileWatcher(); // reset to defaults.
		if (filenameToWatch) {
			this->filename = filenameToWatch;
			return update();
		}

		return false;
	}

	/// Returns true if the file was updated.
	bool update() {
		lastUpdateModtime = modtime;
		modtime = FileReadStream::getFileModTime(filename.c_str());
		isFileExisting = modtime != 0;
		return lastUpdateModtime != modtime;
	}

	bool doesFileExists() const { return isFileExisting; }

	bool hasChangedSinceLastUpdate() {
		if (isFileExisting == false) {
			return false;
		}

		sgeAssert(modtime >= lastUpdateModtime);
		return modtime > lastUpdateModtime;
	}

	const std::string& getFilename() const { return filename; }

  private:
	bool isFileExisting = false;
	sint64 modtime = 0;
	sint64 lastUpdateModtime = 0;
	std::string filename;
};

struct FilesWatcher {
	void initialize(const std::set<std::string>& filesToWatch, float delayedCheckIntervalsSeconds = 0) {
		timeIntervalBetweenActualChecks = delayedCheckIntervalsSeconds;
		for (const std::string& filePath : filesToWatch) {
			watchers.emplace_back(FileWatcher(filePath.c_str()));
		}
	}

	/// Returns true if any file was updated.
	bool update() {
		if (timeIntervalBetweenActualChecks != 0) {
			const float now = Timer::now_seconds();
			if (now - lastCheckTime < timeIntervalBetweenActualChecks) {
				// Not enough time has passed do not check.
				return false;
			}

			lastCheckTime = now;
		}

		bool updated = false;

		for (FileWatcher& fw : watchers) {
			updated |= fw.update();
		}

		return updated;
	}

  private:
	Timer timer;
	float timeIntervalBetweenActualChecks = 0;
	float lastCheckTime = 0;
	std::vector<FileWatcher> watchers;
};

} // namespace sge
