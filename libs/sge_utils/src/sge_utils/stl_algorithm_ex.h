#pragma once

#include <algorithm>

namespace sge {

/// @brief A simple helper function for std::vector that adds element at the front of the specified vector.
template <typename TStdVector>
void push_front(TStdVector& vec, const typename TStdVector::value_type& v) {
	if (vec.size() == 0)
		vec.push_back(v);
	else
		vec.insert(vec.begin(), v);
}

template <class STL_Like_Type, class TPredicate>
typename STL_Like_Type::iterator find_if(STL_Like_Type& container, TPredicate predicate) {
	return std::find_if(std::begin(container), std::end(container), predicate);
}

template <class STL_Like_Type, class TPredicate>
typename STL_Like_Type::const_iterator find_if(const STL_Like_Type& container, TPredicate predicate) {
	return std::find_if(std::begin(container), std::end(container), predicate);
}

} // namespace sge
