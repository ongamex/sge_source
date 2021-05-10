#include "GameSerialization.h"
#include "GameInspector.h"
#include "GameWorld.h"
#include "sge_core/ICore.h"
#include "sge_utils/math/transform.h"
#include "sge_utils/utils/FileStream.h"
#include "sge_utils/utils/json.h"

namespace sge {

enum SceneVersion {
	sceneVersion_inital = 0,

	sceneVersion_enumEnd,
	sceneVersion_count = sceneVersion_enumEnd,
};

JsonValue* serializeVariable(const TypeDesc* const typeDesc, const char* const data, JsonValueBuffer& jvb) {
	if (typeDesc == NULL) {
		SGE_DEBUG_ERR("[SERIALIZATION] No TypeDesc was specified to %s\n", __func__);
		sgeAssert(false);
		return nullptr;
	}

	if (data == NULL) {
		SGE_DEBUG_ERR("[SERIALIZATION] No data was specified to %s for type %s\n", __func__, typeDesc->name);
		sgeAssert(false);
		return nullptr;
	}

	// Check if this is a primitive type.
	if (typeDesc->typeId == sgeTypeId(unsigned)) {
		return jvb(*(unsigned*)data);
	} else if (typeDesc->typeId == sgeTypeId(int)) {
		return jvb(*(int*)data);
	} else if (typeDesc->typeId == sgeTypeId(float)) {
		return jvb(*(float*)data);
	} else if (typeDesc->typeId == sgeTypeId(char)) {
		return jvb(*(char*)data);
	} else if (typeDesc->typeId == sgeTypeId(bool)) {
		return jvb(*(bool*)data);
	} else if (typeDesc->typeId == sgeTypeId(std::string)) {
		return jvb(*(std::string*)data);
	} else if (typeDesc->typeId == sgeTypeId(transf3d)) {
		// Caution:
		// [TRANSF3D_GAME_SERIALIZATION]
		// As a lot of comoments in a trasform are usually default (particularly rotation or scaling),
		// we could save a lot of space by not serializing them and using the defaults for them.
		// Each object has at least two transforms so this is a quite benefitial optimization.
		const transf3d& transf = *reinterpret_cast<const transf3d*>(data);
		JsonValue* const jTransform = jvb(JID_MAP);
		if (transf.p != vec3f(0.f)) {
			jTransform->setMember("p", serializeVariable(typeLib().find(sgeTypeId(decltype(transf.p))), (char*)&transf.p, jvb));
		}
		if (transf.r != quatf::getIdentity()) {
			jTransform->setMember("r", serializeVariable(typeLib().find(sgeTypeId(decltype(transf.r))), (char*)&transf.r, jvb));
		}
		if (transf.s != vec3f(1.f)) {
			jTransform->setMember("s", serializeVariable(typeLib().find(sgeTypeId(decltype(transf.s))), (char*)&transf.s, jvb));
		}

		return jTransform;
	} else if (typeDesc->enumUnderlayingType.isValid()) {
		// This is an enum. Pass this to be serailized as its underlying type.
		// TODO: Concider adding the text in some way.
		return serializeVariable(typeLib().find(typeDesc->enumUnderlayingType), data, jvb);
	} else if (typeDesc->stdVectorUnderlayingType.isValid()) {
		// The type is std::vector
		JsonValue* result = jvb(JID_ARRAY);

		size_t const numElements = typeDesc->stdVectorSize(data);
		for (int t = 0; t < numElements; ++t) {
			const char* const elementData = (const char*)typeDesc->stdVectorGetElementConst(data, t);
			result->arrPush(serializeVariable(typeLib().find(typeDesc->stdVectorUnderlayingType), elementData, jvb));
		}

		return result;
	} else if (typeDesc->stdMapKeyType.isValid() && typeDesc->stdMapKeyType.isValid()) {
		const TypeDesc* keyTd = typeLib().find(typeDesc->stdMapKeyType);
		const TypeDesc* ValueTd = typeLib().find(typeDesc->stdMapValueType);

		// The type is std::map
		JsonValue* jMapAsArray = jvb(JID_ARRAY);

		const size_t mapSize = typeDesc->stdMapSize((void*)data);

		void* tempKey = keyTd->newFn();
		void* tempValue = ValueTd->newFn();

		for (size_t t = 0; t < mapSize; ++t) {
			if (t != 0) {
				// the new, call already called the constructor.
				keyTd->constructorFn(tempKey);
				ValueTd->constructorFn(tempValue);
			}

			typeDesc->stdMapGetNthPair((void*)data, t, tempKey, tempValue);

			JsonValue* jKey = serializeVariable(typeLib().find(typeDesc->stdMapKeyType), (char*)tempKey, jvb);
			JsonValue* jValue = serializeVariable(typeLib().find(typeDesc->stdMapValueType), (char*)tempValue, jvb);

			JsonValue* jMapEntry = jvb(JID_MAP);
			jMapEntry->setMember("key", jKey);
			jMapEntry->setMember("value", jValue);

			jMapAsArray->arrPush(jMapEntry);

			// Call the destructor.
			keyTd->destructorFn(tempKey);
			ValueTd->destructorFn(tempValue);
		}

		// Delete the allocated memory.
		keyTd->deleteFn(tempKey);
		tempKey = nullptr;
		ValueTd->deleteFn(tempValue);
		tempValue = nullptr;

		return jMapAsArray;
	}

	// Assume that this is a struct.
	if (typeDesc->members.size() == 0) {
		SGE_DEBUG_ERR("[SERIALIZATION] Unknown type type %s\n", typeDesc->name);
		sgeAssert(false);
		return nullptr;
	}

	JsonValue* const jResult = jvb(JID_MAP);

	for (int iMember = 0; iMember < typeDesc->members.size(); ++iMember) {
		const MemberDesc& mfd = typeDesc->members[iMember];
		TypeDesc* const memberTypeDesc = typeLib().find(mfd.typeId);
		if (mfd.isSaveable()) {
			if (memberTypeDesc != nullptr) {
				if (mfd.byteOffset >= 0) {
					const char* const memberData = (char*)(data) + mfd.byteOffset;
					JsonValue* const serializedMember = serializeVariable(memberTypeDesc, memberData, jvb);

					if (serializedMember) {
						jResult->setMember(mfd.name, serializedMember);
					} else {
						SGE_DEBUG_ERR("[SERIALIZATION] Failed to serialize member %s::%s\n", typeDesc->name, mfd.name);
						sgeAssert(false);
					}
				} else if (mfd.getDataFn != nullptr) {
					char* const memberData = (char*)alloca(mfd.sizeBytes);
					memberTypeDesc->constructorFn(memberData);
					mfd.getDataFn((void*)data, memberData);

					JsonValue* const serializedMember = serializeVariable(memberTypeDesc, memberData, jvb);

					if (serializedMember) {
						jResult->setMember(mfd.name, serializedMember);
					} else {
						SGE_DEBUG_ERR("[SERIALIZATION] Failed to serialize member %s::%s\n", typeDesc->name, mfd.name);
						sgeAssert(false);
					}
				}

			} else {
				SGE_DEBUG_ERR("[SERIALIZATION] Found a member without TypeDesc in type %s\n", typeDesc->name);
				sgeAssert(false);
			}
		}
	}

	return jResult;
}

bool deserializeVariable(char* const valueData, const JsonValue* jValue, const TypeDesc* const typeDesc) {
	if (typeDesc == nullptr || jValue == nullptr || valueData == nullptr) {
		return false;
	}

	if (typeDesc->typeId == sgeTypeId(unsigned)) {
		*(unsigned*)(valueData) = jValue->getNumberAs<unsigned>();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(int)) {
		*(int*)(valueData) = jValue->getNumberAs<int>();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(float)) {
		*(float*)(valueData) = jValue->getNumberAs<float>();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(char)) {
		*(char*)(valueData) = jValue->getNumberAs<char>();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(bool)) {
		*(bool*)(valueData) = jValue->getNumberAs<bool>();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(std::string)) {
		*(std::string*)(valueData) = jValue->GetString();
		return true;
	} else if (typeDesc->typeId == sgeTypeId(transf3d)) {
		// Caution:
		// [TRANSF3D_GAME_SERIALIZATION]
		// As a lot of comoments in a trasform are usually default (particularly rotation or scaling),
		// we could save a lot of space by not serializing them and using the defaults for them.
		// Each object has at least two transforms so this is a quite benefitial optimization.
		const JsonValue* jPosition = jValue->getMember("p");
		const JsonValue* jRotation = jValue->getMember("r");
		const JsonValue* jScale = jValue->getMember("s");

		const transf3d& transf = *reinterpret_cast<const transf3d*>(valueData);
		bool succeeded = true;
		if (jPosition != nullptr) {
			succeeded &= deserializeVariable((char*)&transf.p, jPosition, typeLib().find(sgeTypeId(decltype(transf.p))));
		}
		if (jRotation != nullptr) {
			succeeded &= deserializeVariable((char*)&transf.r, jRotation, typeLib().find(sgeTypeId(decltype(transf.r))));
		}
		if (jScale != nullptr) {
			succeeded &= deserializeVariable((char*)&transf.s, jScale, typeLib().find(sgeTypeId(decltype(transf.s))));
		}

		return succeeded;
	} else if (typeDesc->enumUnderlayingType.isValid()) {
		// This is an enum. Pass this to be deserialized as its underlying type.
		// TODO: Concider adding the text in some way.
		return deserializeVariable(valueData, jValue, typeLib().find(typeDesc->enumUnderlayingType));
	} else if (typeDesc->stdVectorUnderlayingType.isValid()) {
		// this is a std::vector<T>
		int const numElements = int(jValue->arrSize());
		typeDesc->stdVectorResize(valueData, numElements);

		for (int t = 0; t < numElements; ++t) {
			bool succeeded = deserializeVariable((char*)typeDesc->stdVectorGetElement(valueData, t), jValue->arrAt(t),
			                                     typeLib().find(typeDesc->stdVectorUnderlayingType));
			if (!succeeded) {
				return false;
			}
		}

		return true;
	} else if (typeDesc->stdMapKeyType.isValid() && typeDesc->stdMapKeyType.isValid()) {
		// The type is std::map
		int const numMapPairs = int(jValue->arrSize());

		const TypeDesc* keyTd = typeLib().find(typeDesc->stdMapKeyType);
		const TypeDesc* ValueTd = typeLib().find(typeDesc->stdMapValueType);

		if (keyTd && ValueTd) {
			void* tempKey = keyTd->newFn();
			void* tempValue = ValueTd->newFn();
			for (int iPair = 0; iPair < numMapPairs; ++iPair) {
				const JsonValue* const jPair = jValue->arrAt(iPair);

				if (iPair != 0) {
					// the new, call already called the constructor.
					keyTd->constructorFn(tempKey);
					ValueTd->constructorFn(tempValue);
				}

				deserializeVariable((char*)tempKey, jPair->getMember("key"), keyTd);
				deserializeVariable((char*)tempValue, jPair->getMember("value"), ValueTd);

				// Call the destructor.
				keyTd->destructorFn(tempKey);
				ValueTd->destructorFn(tempValue);

				typeDesc->stdMapInsert(valueData, tempKey, tempValue);
			}

			keyTd->deleteFn(tempKey);
			tempKey = nullptr;
			ValueTd->deleteFn(tempValue);
			tempValue = nullptr;
			return true;
		} else {
			sgeAssert(false && "std::map types not defined");
		}
	}


	// Assume that this is a struct.
	if (jValue->jid != JID_MAP) {
		return false;
	}

	for (int iMember = 0; iMember < typeDesc->members.size(); ++iMember) {
		const MemberDesc& mfd = typeDesc->members[iMember];

		if (mfd.isSaveable() == false) {
			continue;
		}

		const JsonValue* const jMember = jValue->getMember(mfd.name);

		if (jMember == nullptr) {
			SGE_DEBUG_ERR("[SERIALIZATION] A member is missing %s::%s. This that is going to be skipped and left as it is.\n",
			              typeDesc->name, mfd.name);
			// sgeAssert(false);
			continue;
		} else {
			bool succeeded = false;
			if (mfd.byteOffset >= 0) {
				succeeded = deserializeVariable(valueData + mfd.byteOffset, jMember, typeLib().find(mfd.typeId));
			} else if (mfd.setDataFn != nullptr) {
				const TypeDesc* const memberTypeDesc = typeLib().find(mfd.typeId);

				if (memberTypeDesc) {
					char* const memberData = (char*)alloca(mfd.sizeBytes);
					memberTypeDesc->constructorFn(memberData);

					succeeded = deserializeVariable(memberData, jMember, memberTypeDesc);
					mfd.setDataFn(valueData, memberData);
				}
			}

			if (!succeeded) {
				SGE_DEBUG_ERR("[SERIALIZATION] Failed to deserialize %s::%s\n", typeDesc->name, mfd.name);
				return false;
			}
		}
	}

	return true;
}

JsonValue* serializeObject(const GameObject* object, JsonValueBuffer& jvb) {
	const TypeDesc* const typeDesc = typeLib().find(object->getType());

	if (!typeDesc) {
		SGE_DEBUG_ERR("GameObject of unregistered type!\n");
		sgeAssert(false);
		return nullptr;
	}

	JsonValue* const jObject = jvb(JID_MAP);

	// Write the type of the object.
	// [TODO] Save the id in a prettier way maybe.
	jObject->setMember("type", jvb(typeLib().find(object->getType())->name));
	jObject->setMember("id", jvb(object->getId().id));
	static_assert(sizeof(ObjectId) == sizeof(int), "");

	JsonValue* const jMembers = jObject->setMember("members", jvb(JID_MAP));

	for (const MemberDesc& mfd : typeDesc->members) {
		const char* const objBytes = (char*)object;
		const char* const memberBytes = objBytes + mfd.byteOffset;

		if (mfd.isSaveable()) {
			JsonValue* const jMember = serializeVariable(typeLib().find(mfd.typeId), memberBytes, jvb);

			if (jMember) {
				jMembers->setMember(mfd.name, jMember);
			} else {
				sgeAssert(false);
			}
		}
	}

	return jObject;
}
std::string serializeObject(const GameObject* object) {
	JsonValueBuffer jvb;
	JsonValue* jActor = serializeObject(object, jvb);

	if (jActor == nullptr) {
		return std::string();
	}

	WriteStdStringStream wss;
	JsonWriter jw;
	if (!jw.write(&wss, jActor, false)) {
		return std::string();
	}

	return wss.serializedString;
}

GameObject*
    deserializeObject(GameWorld* const world, const JsonValue* const jObject, const bool shouldGenerateNewId, ObjectId* outOriginalId) {
	// Get the id of the object.
	ObjectId id;

	const JsonValue* const jId = jObject->getMember("id");
	static_assert(sizeof(id) == sizeof(int), "");
	id.id = jId->getNumberAs<int>();

	if (outOriginalId != nullptr)
		*outOriginalId = id;

	if (shouldGenerateNewId) {
		id = ObjectId();
	}


	// Get the type of the object.
	const char* objectTypeName = jObject->getMember("type")->GetString();
	const TypeDesc* const actorTypeDesc = typeLib().findByName(objectTypeName);

	if (!actorTypeDesc) {
		sgeAssert(false);
		return nullptr;
	}

	// Create the game object itself.
	GameObject* const object = world->allocObject(actorTypeDesc->typeId, id);

	if (!object) {
		sgeAssert(false);
		return nullptr;
	}

	if (shouldGenerateNewId == false) {
		sgeAssert(object->getId() == id);
	}

	// Load the members
	const JsonValue* const jMembers = jObject->getMember("members");

	for (int iMember = 0; iMember < actorTypeDesc->members.size(); ++iMember) {
		const MemberDesc& mfd = actorTypeDesc->members[iMember];

		if (mfd.isSaveable() == false) {
			continue;
		}

		if (shouldGenerateNewId) {
			if (mfd.flags & MFF_PrefabDontCopy) {
				continue;
			}
		}

		const JsonValue* const jMember = jMembers->getMember(mfd.name);

		if (jMember != nullptr) {
			bool succeeded = false;
			bool const isLogicTransform = mfd.is(&Actor::m_logicTransform);

			if (isLogicTransform) {
				transf3d logicTransform;
				succeeded = deserializeVariable((char*)&logicTransform, jMember, typeLib().find(mfd.typeId));
				Actor* actor = object->getActor();
				if_checked(actor) { actor->setTransform(logicTransform); }
			} else if (mfd.byteOffset >= 0) {
				char* const memberData = (char*)(object) + mfd.byteOffset;
				succeeded = deserializeVariable(memberData, jMember, typeLib().find(mfd.typeId));
			} else if (mfd.setDataFn != nullptr) {
				const TypeDesc* const memberTypeDesc = typeLib().find(mfd.typeId);

				if (memberTypeDesc) {
					char* const memberData = (char*)alloca(mfd.sizeBytes);
					memberTypeDesc->constructorFn(memberData);

					succeeded = deserializeVariable(memberData, jMember, memberTypeDesc);
					mfd.setDataFn(object, memberData);
				}
			}

			if (!succeeded) {
				sgeAssert(false);
				return nullptr;
			}
		} else {
			SGE_DEBUG_ERR("[SERIALIZATION] Member not found %s::%s. The member is skipped.\n", actorTypeDesc->name, mfd.name);
		}
	}

	object->makeDirtyExternal();
	object->onMemberChanged();

	return object;
}

GameObject*
    deserializeObjectFromJson(GameWorld* const world, const std::string& json, const bool shouldGenerateNewId, ObjectId* outOriginalId) {
	JsonParser parser;
	ReadCStringStream crs(json.c_str());
	if (!parser.parse(&crs)) {
		return nullptr;
	}

	return deserializeObject(world, parser.getRoot(), shouldGenerateNewId, outOriginalId);
}

JsonValue* serializeGameWorld(const GameWorld* world, JsonValueBuffer& jvb) {
	JsonValue* const jWorld = jvb(JID_MAP);

	// Serialize the version of the scheme format.
	jWorld->setMember("version", jvb(sceneVersion_inital));

	// Serialize the state of the world.
	jWorld->setMember("nextNameIndex", jvb(world->m_nextNameIndex));

	// Serialize the state of the world.
	jWorld->setMember("nextActorId", jvb(world->m_nextObjectId));
	JsonValue* const jGameObjects = jWorld->setMember("actors", jvb(JID_ARRAY)); // TODO: rename from "actors" to "objects"

	// The object that provides the game camera.
	jWorld->setMember("cameraProvider", jvb(world->m_cameraPovider.id));

	jWorld->setMember("ambientLightColor", serializeVariableT(world->m_ambientLight, jvb));
	jWorld->setMember("rimLightColor", serializeVariableT(world->m_rimLight, jvb));
	jWorld->setMember("rimCosineWidth", serializeVariableT(world->m_rimCosineWidth, jvb));

	jWorld->setMember("skyColorBottom", serializeVariableT(world->m_skyColorBottom, jvb));
	jWorld->setMember("skyColorTop", serializeVariableT(world->m_skyColorTop, jvb));

	jWorld->setMember("gridShouldDraw", serializeVariableT(world->gridShouldDraw, jvb));
	jWorld->setMember("gridNumSegments", serializeVariableT(world->gridNumSegments, jvb));
	jWorld->setMember("gridSegmentsSpacing", serializeVariableT(world->gridSegmentsSpacing, jvb));

	jWorld->setMember("defaultGravity", serializeVariableT(world->m_defaultGravity, jvb));
	jWorld->setMember("physicsSimNumSubSteps", serializeVariableT(world->m_physicsSimNumSubSteps, jvb));

	jWorld->setMember("worldScripts", serializeVariableT(world->m_scriptObjects, jvb));

	// Hierarchical relationships.
	// Do not serialize the m_parentOf as we can restore it by using this information.
	JsonValue* const jHierarchy = jWorld->setMember("hierarchy", jvb(JID_ARRAY));
	for (auto itr_parent2childs : world->m_childernOf) {
		jHierarchy->arrPush(jvb(itr_parent2childs.first.id));
		JsonValue* const jChilds = jHierarchy->arrPush(jvb(JID_ARRAY));

		for (const ObjectId id : itr_parent2childs.second) {
			jChilds->arrPush(jvb(id.id));
		}
	}

	// Serialize the game objects.
	world->iterateOverPlayingObjects(
	    [&](const GameObject* object) -> bool {
		    JsonValue* const jGameObject = serializeObject(object, jvb);
		    sgeAssert(jGameObject);
		    if (jGameObject) {
			    jGameObjects->arrPush(jGameObject);
		    }

		    return true;
	    },
	    true);

	return jWorld;
}

std::string serializeGameWorld(const GameWorld* world) {
	JsonValueBuffer jvb;
	const JsonValue* const jWorld = serializeGameWorld(world, jvb);
	JsonWriter jw;
	WriteStdStringStream ss;
	jw.write(&ss, jWorld, false);

	return std::move(ss.serializedString);
}

bool loadGameWorldFromStream(GameWorld* world, IReadStream* stream, const char* const workingFilename) {
	if (!world || !stream) {
		return false;
	}

	world->clear();
	world->create();

	if (world->inspector) {
		world->inspector->m_disableAutoStepping = true;
	}

	JsonParser jsonParser;
	jsonParser.parse(stream);

	const JsonValue* const jWorld = jsonParser.getRoot();

	if (!jWorld) {
		sgeAssert(false);
		return false;
	}

	auto deserializeWorldMember = [jWorld](void* data, const char* member, TypeId typeId) -> void {
		const JsonValue* jMember = jWorld->getMember(member);
		if (jMember) {
			const TypeDesc* td = typeLib().find(typeId);
			if (td) {
				deserializeVariable((char*)data, jMember, td);
			}
		}
	};

	const auto jfmtVersion = jWorld->getMember("fmtVersion");
	const int version = jfmtVersion ? jfmtVersion->getNumberAs<int>() : -1;

	//
	const JsonValue* jNextNameIdx = jWorld->getMember("nextNameIndex");
	if (jNextNameIdx)
		world->m_nextNameIndex = jNextNameIdx->getNumberAs<int>();

	// Load the state of the world.
	world->m_nextObjectId = jWorld->getMember("nextActorId")->getNumberAs<int>();

	// Camera provider.

	if (const JsonValue* const jCameraProvider = jWorld->getMember("cameraProvider")) {
		world->m_cameraPovider.id = jCameraProvider->getNumberAs<int>();
	}

	if (const JsonValue* const jAmbientLightColor = jWorld->getMember("ambientLightColor")) {
		deserializeVariable((char*)&world->m_ambientLight, jAmbientLightColor, typeLib().find(sgeTypeId(vec3f)));
	}

	if (const JsonValue* const jRimLightColor = jWorld->getMember("rimLightColor")) {
		deserializeVariable((char*)&world->m_rimLight, jRimLightColor, typeLib().find(sgeTypeId(vec3f)));
	}

	if (const JsonValue* const jRimWidth = jWorld->getMember("rimCosineWidth")) {
		deserializeVariable((char*)&world->m_rimCosineWidth, jRimWidth, typeLib().find(sgeTypeId(float)));
	}

	if (const JsonValue* const jSkyColor = jWorld->getMember("skyColorBottom")) {
		deserializeVariable((char*)&world->m_skyColorBottom, jSkyColor, typeLib().find(sgeTypeId(vec3f)));
	}

	if (const JsonValue* const jSkyColor = jWorld->getMember("skyColorTop")) {
		deserializeVariable((char*)&world->m_skyColorTop, jSkyColor, typeLib().find(sgeTypeId(vec3f)));
	}

	deserializeWorldMember(&world->gridShouldDraw, "gridShouldDraw", sgeTypeId(decltype(world->gridShouldDraw)));
	deserializeWorldMember(&world->gridNumSegments, "gridNumSegments", sgeTypeId(decltype(world->gridNumSegments)));
	deserializeWorldMember(&world->gridSegmentsSpacing, "gridSegmentsSpacing", sgeTypeId(decltype(world->gridSegmentsSpacing)));

	if (const JsonValue* const jDefaultGravity = jWorld->getMember("defaultGravity")) {
		deserializeVariable((char*)&world->m_defaultGravity, jDefaultGravity, typeLib().find(sgeTypeId(vec3f)));
	}

	deserializeWorldMember(&world->m_physicsSimNumSubSteps, "physicsSimNumSubSteps", sgeTypeId(decltype(world->m_physicsSimNumSubSteps)));

	deserializeWorldMember(&world->m_scriptObjects, "worldScripts", sgeTypeId(decltype(world->m_scriptObjects)));

	// Load the playing objects.
	const JsonValue* const jActors = jWorld->getMember("actors");
	for (int t = 0; t < jActors->arrSize(); ++t) {
		const JsonValue* const jActor = jActors->arrAt(t);
		deserializeObject(world, jActor, false, nullptr);
	}

	// Set each trasnform again in order to enforce it to the physics.
	// This is a bit hacky IMHO.
	for (int t = 0; t < world->objectsAwaitingCreation.size(); ++t) {
		Actor* actor = dynamic_cast<Actor*>(world->objectsAwaitingCreation[t]);
		if (actor) {
			actor->setTransform(actor->getTransform());
		}
	}

	// Restore the hierarchial relationships between actors.
	const JsonValue* const jHierarchy = jWorld->getMember("hierarchy");
	if (jHierarchy) {
		for (int iParent = 0; iParent < jHierarchy->arrSize(); iParent += 2) {
			ObjectId const parentId(jHierarchy->arrAt(iParent)->getNumberAs<int>());

			auto& childList = world->m_childernOf[parentId];
			const JsonValue* const jChildren = jHierarchy->arrAt(iParent + 1);
			for (int iChild = 0; iChild < jChildren->arrSize(); ++iChild) {
				ObjectId const childId(jChildren->arrAt(iChild)->getNumberAs<int>());
				childList.add(childId);
				world->m_parentOf[childId] = parentId;
			}
		}
	}

	// Save the filename that we are working with.
	world->m_workingFilePath = workingFilename;
	world->physicsWorld.setGravity(world->m_defaultGravity);
	world->update(GameUpdateSets(0.f, true, InputState()));
	world->onWorldLoaded.invokeEvent();

	return true;
}

bool loadGameWorldFromString(GameWorld* world, const char* const levelJson, const char* const workingFilename) {
	ReadCStringStream rs(levelJson);
	return loadGameWorldFromStream(world, &rs, workingFilename);
}

bool loadGameWorldFromFile(GameWorld* world, const char* const filename) {
	if (!filename || filename[0] == '\0') {
		return false;
	}

	if (world->inspector) {
		world->inspector->m_disableAutoStepping = true;
	}

	// Load and parse the json.
	FileReadStream frs;
	if (!frs.open(filename)) {
		SGE_DEBUG_ERR("Unable to open world file '%s'\n", filename);
		sgeAssert(false);
		return false;
	}

	return loadGameWorldFromStream(world, &frs, filename);
}

} // namespace sge
