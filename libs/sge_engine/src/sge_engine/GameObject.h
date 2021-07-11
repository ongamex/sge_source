#pragma once

#include "TypeRegister.h"
#include "sge_engine/typelibHelper.h"
#include "sge_engine_api.h"
#include "sge_utils/utils/basetypes.h"
#include "sge_utils/utils/vector_map.h"
#include <string>

namespace sge {

struct GameWorld;
struct GameUpdateSets;
struct GameObject;
struct Actor;
struct Trait;

enum EditMode : signed char { editMode_actors = 0, editMode_points, editMode_count };

using ObjectAEMemberFilterFn = bool (*)(GameObject* actor, const MemberDesc& mdf, void* pValueToFIlter);

/// ObjectId is a type specifying an id for a specific game object.
/// This id should be unique per game object, in any specific instance of the @GameWorld.
/// To reference other objects in the scene, use a ObjectId and call GameWorld::getObjectById().
/// An id of 0 (or just default ObjectId()) refers ot an invalid id, that never points to any game object.
struct ObjectId {
	// Caution:
	// A std::hash implmentation below!
	// Update it if you add any members!

	explicit ObjectId(int const id = 0)
	    : id(id) {}

	/// @brief Returns true if the id is invalid and cannot possibly point to any object.
	bool isNull() const { return id == 0; }

	bool operator<(const ObjectId& r) const { return id < r.id; }
	bool operator>(const ObjectId& r) const { return id > r.id; }
	bool operator==(const ObjectId& r) const { return id == r.id; }
	bool operator!=(const ObjectId& r) const { return id != r.id; }

  public:
	int id;
};

/// @brief GameObject is the base class for all game object that participate in a @GameWorld.
/// This is just a base class that can be inherited to create specific kinds of game objects.
/// The most notable GameObject kind is @Actor - this game object has a transformation and paricipates in the 3D scene.
struct SGE_ENGINE_API GameObject : public NoncopyableMovable {
	GameObject() = default;
	virtual ~GameObject() = default;

	/// @brief To be called only by @GameWorld. The function sets the common properties of the game object.
	/// Never call this manually.
	void private_GameWorld_performInitialization(GameWorld* const world, const ObjectId id, const TypeId typeId, std::string displayName);

	bool isActor() const;
	Actor* getActor();
	const Actor* getActor() const;

	ObjectId getId() const { return m_id; }
	TypeId getType() const { return m_type; }
	const std::string& getDisplayName() const { return m_displayName; }
	const char* getDisplayNameCStr() const { return m_displayName.c_str(); }
	void setDisplayName(std::string name) { m_displayName = std::move(name); }
	GameWorld* getWorld() { return m_world; }
	const GameWorld* getWorld() const { return m_world; }
	GameWorld* getWorldMutable() const { return m_world; }

	// Composes a debug display name that contains the display name + the id of the object (as the names aren't unique).
	void composeDebugDisplayName(std::string& result) const;

	// Called when the object has to be created.
	virtual void create() = 0;

	// Regular update, the player may only read and partially modify itself.
	// However this shouldn't break other game object that rely on this one.
	// For example during update() the object shouldn't delete it's rigid bodies
	// as other object may rely on the contact manifolds with that object.
	// use the postUpdate() for such manipulations.
	virtual void update(const GameUpdateSets& UNUSED(updateSets)) {}

	// This is the place where the object can freely modify themselves,
	// you cannot rely on other objects, as they can modify themselves here.
	virtual void postUpdate(const GameUpdateSets& UNUSED(updateSets)) {}

	/// Called when the object enters or leaves the game.
	/// If you override this, make sure you've called this method as well.
	virtual void onPlayStateChanged(bool const isStartingToPlay);

	/// Called when THIS object was create by duplicating another.
	/// Called at the end of the duplication process.
	virtual void onDuplocationComplete() {}

	/// Called when a member of the object has been changed.
	/// Caution: Not really. The changer needs to call this, it is not automatic.
	/// This is a legacy method (but still used) of notifying other object that something has changed.
	virtual void onMemberChanged() {}

	/// This method just registers the pointer to the trait, it is only a book keeping.
	void registerTrait(Trait& trait);

	/// @brief Searches for a registered trait of the specified family in the current object.
	/// It would be better to use the global @getTrait function, it is typesafe and can search
	/// not just by family, but also by type.
	Trait* findTraitByFamily(const TypeId family) {
		for (const TraitRegistration& traitReg : m_traits) {
			if (traitReg.traitFamilyType == family) {
				return traitReg.pointerToTrait;
			}
		}

		return nullptr;
	}

	/// @brief Searches for a registered trait of the specified family in the current object.
	/// It would be better to use the global @getTrait function, it is typesafe and can search
	/// not just by family, but also by type.
	const Trait* findTraitByFamily(const TypeId family) const {
		for (const TraitRegistration& traitReg : m_traits) {
			if (traitReg.traitFamilyType == family) {
				return traitReg.pointerToTrait;
			}
		}

		return nullptr;
	}

	int getDirtyIndex() const { return m_dirtyIndex; }
	void makeDirtyExternal() { makeDirty(); }

  protected:
	void makeDirty() { m_dirtyIndex++; }

  public: // This should be private, however because of the reflection system it needs to be public.
	/// The id of the game object. No other object can have the same id in the current GameWorld that owns that game object.
	ObjectId m_id;
	/// The exact type id of the GameObject.
	TypeId m_type;
	/// @brief A way to notify other objects that something important in this object has been changed.
	/// this is an old apporach to solve the problem with dependencies between objects and will get deleted.
	int m_dirtyIndex = 0;

	std::string m_displayName;
	GameWorld* m_world = nullptr;

	struct TraitRegistration {
		TypeId traitFamilyType;
		Trait* pointerToTrait = nullptr;
	};

	std::vector<TraitRegistration> m_traits;
};

/// @brief A structure describing a selection in SGEEditor.
struct SelectedItem {
	SelectedItem() = default;

	SelectedItem(EditMode const editMode, const ObjectId& objectId, int const index)
	    : editMode(editMode)
	    , objectId(objectId)
	    , index(index) {}

	explicit SelectedItem(const ObjectId& objectId)
	    : editMode(editMode_actors)
	    , objectId(objectId) {}

	bool operator==(const SelectedItem& ref) const { return objectId == ref.objectId && index == ref.index && editMode == ref.editMode; }

	bool operator<(const SelectedItem& ref) const { return ref.objectId > objectId || ref.index > index || ref.editMode > editMode; }

	EditMode editMode = editMode_actors; // The mode the item was selected.
	ObjectId objectId;
	int index = 0; // The index of the item. Depends on the edit mode.
};

struct SelectedItemDirect {
	EditMode editMode = editMode_actors; // The mode the item was selected.
	GameObject* gameObject = nullptr;
	int index = 0; // The index of the item. Depends on the edit mode.

	static SelectedItemDirect formSelectedItem(const SelectedItem& item, GameWorld& world);
};

/// @brief Traits are properties that can be attached to any game object.
/// These properties are a way to provide reusable functionallity between different game objects.
/// These functionallities migtht be, an engine functionallity - Rigid Bodies, Renderable 3D Models/Sprites, Viewport Icons and others.
/// They also might be game specific - Health, Inventory, Interactables and others.
/// A trait usually has a family, this familiy is basically the base interface type of the Trait.
///
/// For example if we want to add an interactable trait, that could be used for multiple interactions,
/// we can create a family for that trait using @SGE_TraitDecl_BaseFamily and then inheirit it and implement
/// the actual traits by using @SGE_TraitDecl_Final.
///
/// If the trait does not need any specific implementations (is not an interface basically) we can use
/// @SGE_TraitDecl_Full to declate a trait familiy and a trait type at the same time for the specified type
/// that is going to be used as a triat.
struct SGE_ENGINE_API Trait {
	Trait() = default;

	virtual ~Trait() { m_owner = nullptr; }

	Trait(const Trait&) = delete;
	const Trait& operator==(const Trait&) = delete;

	/// @brief Returns the GameObject that owns that trait.
	/// Should never return nullptr.
	GameObject* getObject() {
		sgeAssert(m_owner != nullptr);
		return m_owner;
	}

	/// @brief Returns the GameObject that owns that trait.
	/// Should never return nullptr.
	const GameObject* getObject() const {
		sgeAssert(m_owner != nullptr);
		return m_owner;
	}

	/// @brief If the owning game object is an actor, this function returns the Actor, otherwise returns nullptr.
	Actor* getActor();

	/// @brief If the owning game object is an actor, this function returns the Actor, otherwise returns nullptr.
	const Actor* getActor() const;

	/// @brief Retrieves the GameWorld that owns the game object that owns that trait.
	GameWorld* getWorld() { return m_owner ? m_owner->getWorld() : nullptr; }

	/// @brief Retrieves the GameWorld that owns the game object that owns that trait.
	const GameWorld* getWorld() const { return m_owner ? m_owner->getWorld() : nullptr; }

	/// @brief Returns the familiy of the trait.
	virtual TypeId getFamily() const = 0;

	/// @brief Do not call this manuall.
	/// This function get called when GameObject::registerTrait, registers a new trait to the object.
	/// The function provides a pointer to the game object that owns the trait.
	/// Traits must always be owned by exactly one object.
	void private_GameObject_register(GameObject* const owner) {
		sgeAssert(owner);
		m_owner = owner;
	}

	virtual void onPlayStateChanged(bool const UNUSED(isStartingToPlay)) {}

	Trait& operator=(const Trait&) { return *this; }

  public:
	/// A pointer to the game object the owns this trait instance.
	GameObject* m_owner = nullptr;
};

/// Defines a trait that is going to be inherited and extended
/// Enabling to have different kinds of the same trait.
/// @BaseTrait is the type of the trait family to be defined.
/// Search the code to see how it is used.
///
/// Note:
/// A friendly reminded that this macro uses the type reflection library,
/// if you get any linger errors because of this it might because you need to
/// call DefineTypeIdExists(BaseTrait) before defining the triat.
#define SGE_TraitDecl_BaseFamily(BaseTrait) \
	typedef BaseTrait TraitFamily;          \
	TypeId getFamily() const final { return sgeTypeId(BaseTrait); }

/// Defines that this is the last override of the trait that we've extended.
#define SGE_TraitDecl_Final(TraitSelf) \
	typedef TraitSelf TraitType;       \
	//TypeId getExactType() const final { return sgeTypeId(TraitSelf); } \

/// Defines a trait that is not going to be extendable ready to be used directly in game objects.
/// The type is going to be both trait family and trait type.
#define SGE_TraitDecl_Full(TraitSelf)                               \
	typedef TraitSelf TraitFamily;                                  \
	typedef TraitSelf TraitType;                                    \
	TypeId getFamily() const final { return sgeTypeId(TraitSelf); } \
	//TypeId getExactType() const final { return sgeTypeId(TraitSelf); } \

/// @brief Searches for a trait of a familly or a type in the specified game object.
/// Returns nullptr if the trait was not found.
template <typename TTrait>
TTrait* getTrait(GameObject* const object) {
	if (!object) {
		return nullptr;
	}

	Trait* const trait = object->findTraitByFamily(sgeTypeId(typename TTrait::TraitFamily));
	if (!trait) {
		return nullptr;
	}

	return dynamic_cast<TTrait*>(trait);
}

/// @brief Searches for a trait of a familly or a type in the specified game object.
/// Returns nullptr if the trait was not found.
template <typename TTrait>
const TTrait* getTrait(const GameObject* const object) {
	if (!object) {
		return nullptr;
	}

	const Trait* const trait = object->findTraitByFamily(sgeTypeId(typename TTrait::TraitFamily));
	if (!trait) {
		return nullptr;
	}

	return dynamic_cast<const TTrait*>(trait);
}

} // namespace sge

// The std::hash implementation of the @ObjectId.
namespace std {
template <>
struct hash<sge::ObjectId> {
	size_t operator()(const sge::ObjectId& x) const { return std::hash<decltype(x.id)>{}(x.id); }
};
} // namespace std
