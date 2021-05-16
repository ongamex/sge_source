#pragma once

#include <new>
#include <utility>

#include "sge_utils/sge_utils.h"
#include "sge_utils/utils/AlignedStorage.h"
#include "sge_utils/utils/TypeTraits.h"

namespace sge {

/// A helper struct to construct empty Optional variable.
/// example return NullOptional();
struct NullOptional {};

template <class T>
struct Optional {
  private:
	typename AlignedStorage<sizeof(T), alignof(T)>::type m_storage;
	bool m_isValid = false;

  public:
	using TNoRef = typename RemoveReference<T>::type;
	static_assert(std::is_reference<T>::value == false, "References aren't supported, use pointers!");

	Optional() = default;

	~Optional() { destroyStorage(); }

	Optional(const NullOptional&) {}

	Optional(const T& other) { copyConstructStorage(other); }

	Optional(T&& other) { moveConstructStorage(std::move(other)); }

	Optional(const Optional& other) {
		if (other.isValid()) {
			copyConstructStorage(other.getStorageTyped());
		}
	}

	Optional(Optional&& other) {
		if (other.isValid()) {
			moveConstructStorage(std::move(other.getStorageTyped()));
			other.m_isValid = false;
		}
	}

	Optional& operator=(const T& other) {
		setStorage(other);
		return *this;
	}

	Optional& operator=(T&& other) {
		setStorage(std::move(other));
		return *this;
	}

	Optional& operator=(const Optional& other) {
		if (other.isValid()) {
			setStorage(other.getStorageTyped());
		} else {
			destroyStorage();
		}
		return *this;
	}

	Optional& operator=(Optional&& other) {
		destroyStorage();
		if (other.isValid()) {
			setStorage(std::move(other.getStorageForMoving()));
			other.m_isValid = false;
		} else {
			destroyStorage();
		}
		return *this;
	}

	bool operator==(T& other) const { return m_isValid && get() == other; }

	bool operator!=(T& other) const { return !m_isValid || (get() != other); }

	void reset() { *this = Optional(); }

	TNoRef* operator->() {
		sgeAssert(isValid());
		T& result = *reinterpret_cast<T*>(m_storage.data);
		return &result;
	}

	const TNoRef* operator->() const {
		sgeAssert(isValid());
		const T& result = *reinterpret_cast<const T*>(m_storage.data);
		return &result;
	}

	operator bool() const { return m_isValid; }

	bool isValid() const { return (bool)(*this); }

	bool hasValue() const { return (bool)(*this); }

	T& get() {
		sgeAssert(isValid());
		T& result = *reinterpret_cast<T*>(m_storage.data);
		return result;
	}

	const T& get() const {
		sgeAssert(isValid());
		const T& result = *reinterpret_cast<const T*>(m_storage.data);
		return result;
	}

	T& operator*() { return get(); }
	const T& operator*() const { return get(); }

  private:
	// void defaultConstructStorage() {
	//	new (m_storage.data) T();
	//}

	void copyConstructStorage(const T& value) {
		new (m_storage.data) T(value);
		m_isValid = true;
	}

	void moveConstructStorage(T&& value) {
		new (m_storage.data) T(std::move(value));
		m_isValid = true;
	}

	T& getStorageTyped() {
		T& result = *reinterpret_cast<T*>(m_storage.data);
		return result;
	}

	const T& getStorageTyped() const {
		const T& result = *reinterpret_cast<const T*>(m_storage.data);
		return result;
	}

	T&& getStorageForMoving() {
		T& result = *reinterpret_cast<T*>(m_storage.data);
		return std::move(result);
	}

	void setStorage(const T& value) {
		if constexpr (std::is_const<T>::value) {
			reset();
			copyConstructStorage(value);
		} else {
			if (isValid()) {
				getStorageTyped() = value;
			} else {
				copyConstructStorage(value);
			}
		}
	}

	void setStorage(TNoRef&& value) {
		if constexpr (std::is_const<T>::value) {
			reset();
			moveConstructStorage(std::move(value));
		} else {
			if (isValid()) {
				getStorageTyped() = std::move(value);
			} else {
				if constexpr (std::is_move_constructible_v<T>) {
					moveConstructStorage(std::move(value));
				} else {
					copyConstructStorage(value);
				}
			}
		}
	}

	void destroyStorage() {
		if (isValid()) {
			T& result = *reinterpret_cast<T*>(m_storage.data);
			result.~T();
			m_isValid = false;
		}
	}
};

} // namespace sge
