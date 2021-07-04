#include "DynamicProperties.h"

namespace sge {

void* DynamicProperties::addProperty(std::string name, TypeId type) {
	const TypeDesc* const td = typeLib().find(type);
	if (td == nullptr || td->constructorFn == nullptr || td->sizeBytes <= 0) {
		// The type does not exist or it is not constuctable, we cannot make this property.
		return nullptr;
	}
	Property& prop = m_properties[name];

	if (!prop.isEmpty()) {
		// The property already exists, we cannot add it.
		return nullptr;
	}

	// Allocate the property and call the default constructor.
	prop.type = td->typeId;
	prop.propData.reset(new char[td->sizeBytes]);
	td->constructorFn(prop.propData.get());

	return prop.propData.get();
}

DynamicProperties::Property* DynamicProperties::find(const std::string& name) {
	auto itr = m_properties.find(name);
	if (itr != m_properties.end()) {
		return &itr->second;
	}

	return nullptr;
}

void* DynamicProperties::find(const std::string& name, TypeId type) {
	auto findItr = m_properties.find(name);
	if (findItr != m_properties.end()) {
		if (type == findItr->second.type) {
			void* const propertyAddress = findItr->second.propData.get();
			sgeAssert(propertyAddress != nullptr);
			return propertyAddress;
		} else {
			sgeAssert(false);
		}
	}

	return nullptr;
}
} // namespace sge
