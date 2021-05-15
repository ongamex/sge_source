#pragma once

#include "sge_utils/sge_utils.h"
#include <chrono>

namespace sge {

struct Timer {
	typedef std::chrono::high_resolution_clock clock;
	typedef clock::time_point time_point;

	static const time_point application_start_time;

	Timer() { reset(); }

	void reset() {
		lastUpdate = clock::now();
		dtSeconds = 0.f;
	}

	static uint64 now_nanoseconds_int() {
		uint64 ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - application_start_time).count();
		return ns;
	}

	// Returns the current time since the application start in seconds.
	static float now_seconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - application_start_time).count() * 1e-6f;
	}

	static float currentPointTimeSeconds() {
		return std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count() * 1e-6f;
	}

#if 0
	static double now_seconds_double() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now() - application_start_time).count() * 1e-9;
	}
#endif

	bool tick() {
		float minThickLength = maxTicksPerSeconds ? 1.f / maxTicksPerSeconds : 0.f;

		const time_point now = clock::now();
		const auto diff = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count();
		dtSeconds = diff * 1e-6f; // microseconds -> seconds

		if (dtSeconds < minThickLength)
			return false;

		lastUpdate = now;
		return true;
	}

	float diff_seconds() const { return dtSeconds; }

	void setFPSCap(float fps) { maxTicksPerSeconds = fps; }

  private:
	time_point lastUpdate;
	float dtSeconds;

	float maxTicksPerSeconds = 0.f;
};

} // namespace sge
