#pragma once

#include "sge_utils/sge_utils.h"

namespace sge {

// A for loop helper that iterates in [a, b)
template <typename T>
struct rng {
	struct const_iterator {
		T i;

		const_iterator(T i)
		    : i(i) {}

		bool operator!=(const const_iterator& ref) const { return i != ref.i; }
		T operator*() const { return i; }
		const_iterator operator++() {
			++i;
			return *this;
		}
	};

	// a is assumed 0. the interval is [0, _end)
	rng(const T _end)
	    : _begin(0)
	    , _end(_end) {}

	// [_begin, _end)
	rng(const T _begin, const T _end)
	    : _begin(_begin)
	    , _end(_end) {
		sgeAssert(_begin <= _end);
	}

	const_iterator begin() const { return _begin; }
	const_iterator end() const { return _end; }

	T _begin, _end;
};

typedef rng<int> rng_int;
typedef rng<unsigned int> rng_uint;
typedef rng<size_t> rng_size;

} // namespace sge
