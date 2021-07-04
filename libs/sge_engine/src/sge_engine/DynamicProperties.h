#pragma once

#include "sge_engine/typelibHelper.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace sge {

struct DynamicProperties {
	struct Property {
		TypeId type;
		std::unique_ptr<char[]> propData;

		bool isEmpty() const { return type.isNull() || propData.get() == nullptr; }

		template <typename T>
		T& getRef() {
			sgeAssert(type == sgeTypeId(T));
			sgeAssert(isEmpty() == false);
			return *(T*)propData.get();
		}

		template <typename T>
		T* get() {
			if (type == sgeTypeId(T) && !isEmpty()) {
				return (T*)propData.get();
			}

			return nullptr;
		}
	};

	void* addProperty(std::string name, TypeId type);

	Property* find(const std::string& name);
	void* find(const std::string& name, TypeId type);

	template <typename T>
	T* findTyped(const char* const name) {
		void* ptr = find(name, sgeTypeId(T));
		return ptr ? reinterpret_cast<T*>(ptr) : nullptr;
	}

	std::unordered_map<std::string, Property> m_properties;
};

DefineTypeIdInline(DynamicProperties, 21'06'21'0001);

} // namespace sge
