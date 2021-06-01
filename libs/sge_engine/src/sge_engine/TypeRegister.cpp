#include "TypeRegister.h"
#include "sge_utils/stl_algorithm_ex.h"
#include <cctype>
#include <functional>

template class std::vector<sge::MemberDesc>;
template class std::map<sge::TypeId, sge::TypeDesc>;
template class std::map<sge::TypeId, sge::TypeDesc*>;

namespace sge {

TypeLib& typeLib() {
	static TypeLib s;
	return s;
}

int addFunctionThatDefinesTypesToTypeLibrary(void (*fnPtr)()) {
	typeLib().functionsToBeCalledThatWillRegisterTypes.push_back(fnPtr);
	return 0;
}

std::string TypeDesc::computePrettyName(const char* const name) {
	const int nameStrLen = int(strlen(name));
	if (nameStrLen <= 2) {
		return name;
	}

	const bool hasMprefix = name[0] == 'm' && name[1] == '_';

	std::string prettyName;

	bool isFirstIter = true;
	int i = hasMprefix ? 2 : 0;
	for (; i < nameStrLen; ++i) {
		if (isFirstIter && std::isalpha(name[i])) {
			prettyName.push_back(char(std::toupper(name[i])));
		} else {
			if (std::isupper(name[i])) {
				prettyName.push_back(' ');
			}

			prettyName.push_back(name[i]);
		}

		isFirstIter = false;
	}

	return prettyName;
}

TypeDesc& TypeDesc::uiRange(float vmin, float vmax, float sliderSpeed) {
	members.back().min_float = vmin;
	members.back().max_float = vmax;
	members.back().sliderSpeed_float = sliderSpeed;

	return *this;
}

TypeDesc& TypeDesc::uiRange(int vmin, int vmax, float sliderSpeed) {
	members.back().min_int = vmin;
	members.back().max_int = vmax;
	members.back().sliderSpeed_float = sliderSpeed;

	return *this;
}

//----------------------------------------------------------------------------------------
// TypeDesc
//----------------------------------------------------------------------------------------
TypeDesc& TypeDesc::addEnumMember(int member, const char* name) {
	sgeAssert(enumUnderlayingType.isNull() == false);
	enumValueToNameLUT[member] = name;
	return *this;
}

const MemberDesc* TypeDesc::findMemberByName(const char* const memberName) const {
	for (int t = 0; t < members.size(); ++t) {
		if (strcmp(members[t].name, memberName) == 0) {
			return &members[t];
		}
	}

	// Should never happen.
	sgeAssert(false);
	return NULL;
}

bool TypeDesc::doesInherits(const TypeId parentClass) const {
	for (const SuperClassData superClass : superclasses) {
		if (superClass.id == parentClass) {
			return true;
		} else {
			// If the current superClass is not a the pearent we are seeking, seach it's parents.
			const TypeDesc* const superClassTypeDesc = typeLib().find(superClass.id);
			if (superClassTypeDesc == nullptr) {
				sgeAssert(false && "Found a super class without a typedesc!");
				continue;
			}

			if (superClassTypeDesc->doesInherits(parentClass)) {
				return true;
			}
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------
// MemberChain
//----------------------------------------------------------------------------------------
bool MemberChain::add(const MemberFieldChainKnot& knot) {
	if (!knot.mfd) {
		sgeAssert(false);
		return false;
	}

	if (knots.size() == 0 && knot.arrayIdx == -1) {
		knots.emplace_back(knot);
		return true;
	} else {
		TypeId ownerTypeId = knots.back().mfd->owner->typeId;
		if_checked(ownerTypeId != knot.mfd->typeId) {
			knots.emplace_back(knot);
			return true;
		}

		return false;
	}
}

void MemberChain::pop() {
	knots.pop_back();
}

void MemberChain::clear() {
	knots.clear();
}

void* MemberChain::follow(void* root) const {
	char* dest = (char*)root;
	for (const auto& knot : knots) {
		if (knot.mfd->byteOffset < 0) {
			return nullptr;
		}

		const TypeDesc* const type = typeLib().find(knot.mfd->typeId);

		if (type == nullptr) {
			sgeAssert(false); // Should never happen.
			return nullptr;
		}

		if (type->stdVectorUnderlayingType.isValid()) {
			if (knot.arrayIdx == -1) {
				// In that case we mean the std::vector itself.
				dest += knot.mfd->byteOffset;
			} else {
				// In that case we are indexing the std::vector.
				char* pStdVector = dest + knot.mfd->byteOffset;
				dest = (char*)type->stdVectorGetElement(pStdVector, knot.arrayIdx);
			}
		} else {
			dest += knot.mfd->byteOffset;
		}
	}

	return dest;
}

void MemberChain::forEachMember(void* root, std::function<void(void* root, const MemberChain&)>& lambda) {
	if (root == nullptr || lambda == nullptr) {
		sgeAssert(false);
		return;
	}

	forEachMemberInternal(root, *this, lambda);
}

void MemberChain::forEachMemberInternal(void* root, MemberChain chain, std::function<void(void* root, const MemberChain&)>& lambda) {
	if (root == nullptr || lambda == nullptr) {
		sgeAssert(false);
		return;
	}

	const TypeDesc* const typeDesc = chain.getType();
	if (!typeDesc) {
		return;
	}

	lambda(root, chain);

	// Check if this is a struct.
	for (const MemberDesc& mfd : typeDesc->members) {
		MemberChain toMemberChain = chain;
		toMemberChain.add(&mfd);
		forEachMemberInternal(root, std::move(toMemberChain), lambda);
	}

	// Check if this is a std::vector
	if (typeDesc->stdVectorUnderlayingType.isValid()) {
		int numElements = int(typeDesc->stdVectorSize(chain.follow(root)));
		for (int t = 0; t < numElements; ++t) {
			MemberChain toMemberChain = chain;
			toMemberChain.knots.back().arrayIdx = t;
			forEachMemberInternal(root, std::move(toMemberChain), lambda);
		}
	}
}

void TypeLib::performRegistration() {
	for (auto& fn : functionsToBeCalledThatWillRegisterTypes) {
		fn();
	}
	functionsToBeCalledThatWillRegisterTypes.clear();

	while (true) {
		bool areAllCompleted = true;
		for (auto& typeItr : this->m_registeredTypes) {
			TypeDesc& type = typeItr.second;

			if (isCompleted.count(type.typeId) == 0) {
				isCompleted[type.typeId] = false;
			}

			if (type.superclasses.empty() || isCompleted[type.typeId]) {
				isCompleted[type.typeId] = true;
				continue;
			}

			areAllCompleted = false;

			bool areAllSuperClassesComplete = true;
			for (TypeDesc::SuperClassData superData : type.superclasses) {
				areAllSuperClassesComplete &= isCompleted[superData.id];
			}

			if (areAllSuperClassesComplete) {
				for (TypeDesc::SuperClassData superData : type.superclasses) {
					const TypeDesc* const superTypeDesc = typeLib().find(superData.id);
					sgeAssert(superTypeDesc);

					// Copy the inherited values desc. The function maintains the original superclass.
					for (const MemberDesc& member : superTypeDesc->members) {
						push_front(type.members, member);
						type.members.front().owner = &type;
						type.members.front().byteOffset += superData.byteOffset;
						if (member.inheritedForm.isNull()) {
							type.members.front().inheritedForm = superData.id;
						}
					}
				}
				isCompleted[type.typeId] = true;
			}
		}

		if (areAllCompleted) {
			break;
		}
	}

	// Finally find all types that inherit Actor and cache them.
	for (TypeLib::MapTypes::iterator itr = typeLib().m_registeredTypes.begin(); itr != typeLib().m_registeredTypes.end(); ++itr) {
		// TODO: add that Object and Actor must be at 0 byte offset.
		if (itr->second.doesInherits(sgeTypeId(GameObject)) && itr->second.newFn != nullptr) {
			m_gameObjectTypes.insert(itr->first);
		}
	}
}

} // namespace sge
