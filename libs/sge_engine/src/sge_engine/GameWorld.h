#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Actor.h"
#include "Camera.h"
#include "PhysicsDebugDraw.h"
#include "sge_core/application/input.h"
#include "sge_engine/Physics.h"
#include "sge_renderer/renderer/renderer.h"
#include "sge_utils/utils/Event.h"
#include "sge_utils/utils/vector_set.h"

namespace sge {

struct Asset;
struct QuickDraw;

struct GameWorld;
struct GameInspector;
struct IGameDrawer;

struct RigidBody;
struct BulletPhysicsDebugDraw;

struct SGE_ENGINE_API IPostSceneUpdateTask {
	IPostSceneUpdateTask() = default;
	virtual ~IPostSceneUpdateTask() = default;
};

struct SGE_ENGINE_API PostSceneUpdateTaskSetWorldState final : public IPostSceneUpdateTask {
	PostSceneUpdateTaskSetWorldState() = default;
	PostSceneUpdateTaskSetWorldState(std::string json, bool noPauseNoEditorCamera)
	    : newWorldStateJson(std::move(json))
	    , noPauseNoEditorCamera(noPauseNoEditorCamera) {}

	std::string newWorldStateJson;
	bool noPauseNoEditorCamera = false;
};

struct SGE_ENGINE_API PostSceneUpdateTaskLoadWorldFormFile final : public IPostSceneUpdateTask {
	PostSceneUpdateTaskLoadWorldFormFile() = default;
	PostSceneUpdateTaskLoadWorldFormFile(std::string filename, bool noPauseNoEditorCamera)
	    : filename(std::move(filename))
	    , noPauseNoEditorCamera(noPauseNoEditorCamera) {}

	std::string filename;
	bool noPauseNoEditorCamera = false;
};

/// @brief A struct describing the settings to be passed to the GameWorld when updating the game.
/// Keep in mind that the world is always getting updated, even when paused even in edit mode.
/// The game objects need to be aware that they will get updated and the editor might suddenly change them.
struct GameUpdateSets {
	GameUpdateSets() = default;
	GameUpdateSets(float const dt, bool const isPaused, InputState const is /*, SGEContext* const sgecon*/)
	    : dt(dt)
	    , isPaused(isPaused)
	    , is(is) {}

	/// @brief Returns true if the game is paused for some reason.
	bool isGamePaused() const { return isPaused; }

	/// @brief Returns true if the game is not paused.
	bool isPlaying() const { return !isGamePaused(); }

	float dt = 0.f;       ///< The delta time to be used when updateing.
	bool isPaused = true; ///< True if the game is paused for any reason (usually we are in the editor and the game is paused there).
	InputState is;        ///< The input state to be used when updateing the scene.
};

/// @brief GameWorld is the main class that hold all alocated GameObject (Material, Scripts, Actors and so on),
/// Maintains their lifetime, as well as stepping the game simulation.
struct SGE_ENGINE_API GameWorld {
	GameWorld() {
		userProjectionSettings.fov = deg2rad(60.f);
		userProjectionSettings.aspectRatio = 1.f; // Cannot be determined here.
		userProjectionSettings.near = 0.1f;
		userProjectionSettings.far = 10000.f;
	}

	~GameWorld() { clear(); }

	void create();

  public:
	void clear();

	/// Steps the game simulation. GameInspectors may intercept the execution of that function.
	void update(const GameUpdateSets& updateSets);

	/// Returns an unique ID for the current scene.
	ObjectId getNewId();

	/// Checks is an object already uses the specified id.
	bool isIdTaken(ObjectId const id) const;

	/// Create a new object of the specified type.
	GameObject* allocObject(TypeId const type, ObjectId const specificId = ObjectId(), const char* name = nullptr);
	Actor* allocActor(TypeId const type, ObjectId const specificId = ObjectId(), const char* name = nullptr);

	/// @brief Type-safe allocation of a GameObject.
	template <typename T>
	T* m_allocator(ObjectId const specificId = ObjectId(), const char* name = nullptr) {
		return dynamic_cast<T*>(allocObject(sgeTypeId(T), specificId, name));
	}

	/// Permanently deletes the specified object, wthout giving it a chance of recovery.
	/// Example usage: in gameplay when destroying bullets or killing enemies.
	/// The object isn't going to be deleted immediatley, instead it is going to get added to a list of object that want to get killed.
	/// At the begining of the next update() they are going to get deleted.
	void objectDelete(const ObjectId& id);

	GameObject* getObjectById(const ObjectId& id);

	/// @brief Retrieves an Actor object by id,
	/// if the object exist but it is not an actor the function will return nullptr.
	Actor* getActorById(const ObjectId& id);

	/// Searches for an actor by name, the first actor with the specified name gets returned.
	GameObject* getObjectByName(const char* name);
	/// Searches for an actor by name, the first actor with the specified name gets returned.
	/// If the object exists, but it is not an Actor the function will return nullptr.
	Actor* getActorByName(const char* name);

	/// Iterates over all playing and awaiting creation game objects.
	/// @param [in] lambda the lambda-function to get called to get called for each game object.
	/// The lambda should return true if it wants to get called for the next object.
	void iterateOverPlayingObjects(const std::function<bool(GameObject*)>& lambda, bool includeAwaitCreationObject);

	/// Iterates over all playing and awaiting creation game objects.
	/// @param [in] lambda the lambda-function to get called to get called for each game object.
	/// The lambda should return true if it wants to get called for the next object.
	void iterateOverPlayingObjects(const std::function<bool(const GameObject*)>& lambda, bool includeAwaitCreationObject) const;

	/// @brief Retrieves a list of all playing object of the specified type. May be nullptr.
	const std::vector<GameObject*>* getObjects(TypeId type) const {
		const auto itr = playingObjects.find(type);
		if (itr == playingObjects.end()) {
			return nullptr;
		}

		return &itr->second;
	}

	/// @brief Retrieves an object with the specified id.
	template <typename T>
	T* getObject(const ObjectId& id) {
		GameObject* const go = getActorById(id);
		if (!go || go->getType() != sgeTypeId(T)) {
			return nullptr;
		}

		return static_cast<T*>(go);
	}

	/// @brief Retrieves an actor with the specified id.
	/// If the object exists but if it is not actor the function will return nullptr.
	template <typename T>
	T* getActor(const ObjectId& id) {
		Actor* const actor = getActorById(id);
		if (!actor || actor->getType() != sgeTypeId(T)) {
			return nullptr;
		}

		return static_cast<T*>(actor);
	}


	/// Sets the parent of the specified actor.
	/// @param [in] child the id of the child object
	/// @param [in] newParent the id of the actor that is going to be the parent of @child
	/// @param [in] doNotAssert - Usually when parenting one object to another, both object should exist.
	///             if they don't then some error might have occured. However when we do object deletion with command histroy.
	///             Having multiple deletes in one command might result in objects not being restored yet (as they were added to the
	///             compound command eariler). In that case we do not assert (and setParentOf will just fail silently). When the missing
	///             object is resotred it, the command will try to resore the original hierarchy and will succeeded if all objects exist in
	///             the scene.
	bool setParentOf(ObjectId const child, ObjectId const newParent, bool doNotAssert = false);

	/// @brief Retrives the parent object id of the specified object (by its id).
	ObjectId getParentId(ObjectId const child) const;

	/// @brief Retrieves the parent actor of the specified actor.
	Actor* getParentActor(ObjectId const child);

	/// @brief Returns the root parent (parent of the parent of the parent and so on) of the specified object.
	ObjectId getRootParentId(ObjectId child) const;

	/// @brief Retrieves all childres of the specified object.
	const vector_set<ObjectId>* getChildensOf(ObjectId const parent) const;
	vector_set<ObjectId> getChildensOfAsList(ObjectId const parent) const {
		const vector_set<ObjectId>* pList = getChildensOf(parent);
		return pList ? *pList : vector_set<ObjectId>();
	}

	/// @brief Returns all childrens and their childrens of the specified actor.
	///        The values are appended to the list.
	void getAllChildren(vector_set<ObjectId>& result, ObjectId const parent) const;

	/// @brief Returns a list of all parents (and their parents) of the specified actor (without the specified actor itself)!
	/// The values are appended to the list.
	void getAllParents(vector_set<ObjectId>& result, ObjectId actorId) const;

	/// @brief Returns the parent and its parents and all childrens and their childrens of the specified actor.
	/// The values are appended to the list.
	void getAllRelativesOf(vector_set<ObjectId>& result, ObjectId actorId) const;

	/// @brief Instantients the specified world into the current world.
	/// @param [in] prefabPath a path the world file to be instantiated.
	/// @param [in] createHistory pass true if the changes should be added to undo/redo history.
	void instantiatePrefab(const char* prefabPath, bool createHistory, bool shouldGenerateNewObjectIds);

	/// Instantients the specified world into the current world.
	/// @param [in] prefabPath a path the world file to be instantiated.
	/// @param [in] createHistory pass true if the changes should be added to undo/redo history.
	void instantiatePrefabFromJsonString(const char* prefabJson, bool createHistory, bool shouldGenerateNewObjectIds);

	/// Instantients the specified prefabWorld into the current world.
	/// In result new objects will be generated in the current world, however for obvious reasions
	/// they will have different ids.
	/// @praam [in] prefabWorld the world to be instantiated in this world.
	/// @param [in] oblectsToInstantiate if the nullptr, all objects will be instantiated, otherwise
	///                             only the objects in the specified list will be instantiated/
	void instantiatePrefab(const GameWorld& prefabWorld,
	                       bool createHistory,
	                       bool shouldGenerateNewObjectIds,
	                       const vector_set<ObjectId>* const pOblectsToInstantiate,
	                       vector_set<ObjectId>* const newObjectIds = nullptr);

	/// Creates a prefab world based on the current world.
	/// @praam [out] prefabWorld the prefab world that is going to be created.
	/// @param [in] oblectsToInstantiate if the nullptr, all objects will be instantiated, otherwise
	///                             only the objects in the specified list will be instantiated in the prefab world.
	void createPrefab(GameWorld& prefabWorld,
	                  bool shouldKeepOriginalObjectIds,
	                  const vector_set<ObjectId>* const pOblectsToInstantiate) const;

	/// @brief Used for giving object unique-ish names. However the GameWorld still supports objects with same name.
	int getNextNameIndex();

	/// @brief Returns the attached inspector (if any).
	GameInspector* getInspector() { return inspector; }

	/// @brief Retrieves the amount of time passed while not being paused.
	float getGameTime() const { return timeSpendPlaying; }

	/// @brief Returns true if the scene is in edit mode. Could be true only in the SGEEditor.
	bool isInEditMode() const { return isEdited; }

	void toggleEditMode() { isEdited = !isEdited; }

	/// @brief Adds a task to be executed after the scene update has finished.
	/// @param task A pointer to DYNAMICALLY allocated with new to task to be executed.
	///        Once done this funcion will call delete on that pointer.
	void addPostSceneTask(IPostSceneUpdateTask* const task);

	/// @brief A shortcut for addPostSceneTask. Useful for changeing the levels.
	void addPostSceneTaskLoadWorldFormFile(const char* filename);

	/// @brief Removes all manifold for the specified rigid body.
	///        Used if for some reason the rigid body is invalidated during updates.
	void removeRigidBodyManifold(RigidBody* rb);

	/// @brief Changes the gravity for all objects currently playing in the scene.
	void setDefaultGravity(const vec3f& gravity);

	ICamera* getRenderCamera();

  public:
	/// The projection settings specified by the user. (Some of them are window dependad and we update them manully).
	/// TODO: This is an old idea, and no longer has its place in the game world.
	CameraProjectionSettings userProjectionSettings;

	/// Game camera for gameplay.
	ObjectId m_cameraPovider = ObjectId();
	bool m_useEditorCamera = true;
	EditorCamera m_editorCamera;

	// Scene default ambient lighting.
	vec3f m_ambientLight = vec3f(0.25f);
	vec3f m_rimLight = vec3f(0.1f);
	float m_rimCosineWidth = 0.3f;

	/// A pointer to the attached inspector(if any).
	GameInspector* inspector = nullptr;

	std::vector<GameObject*> objectsAwaitingCreation; // A set of object ready to start playing at the beginning of the next step.
	std::unordered_map<TypeId, std::vector<GameObject*>> playingObjects; // All playing game object sorted by type.
	vector_set<ObjectId> objectsWantingPermanentKill; // A set of actors that are going to be compleatley deleted for the game world.
	std::unordered_map<ObjectId, GameObject*> m_gameObjectByIdLUT; // A look up table for fast searching for an object with a specific id.

	/// Hierarchical relationship between actors.
	/// These two are deeply connected to one another!
	std::unordered_map<ObjectId, vector_set<ObjectId>> m_childernOf;
	std::unordered_map<ObjectId, ObjectId> m_parentOf;

	/// Physics
	PhysicsWorld physicsWorld;
	BulletPhysicsDebugDraw m_physicsDebugDraw;

	/// Per frame physics contact manifold list.
	std::unordered_map<const RigidBody*, std::vector<const btPersistentManifold*>> m_physicsManifoldList;

	/// The next free game object id.
	int m_nextObjectId = 1;

	/// When creating new nodes or duplicating existing we use these indices for naming new nodes.
	int m_nextNameIndex = 0;

	int totalStepsTaken = 0;      ///< The number of updates done, both paused and playing.
	float timeSpendPlaying = 0.f; ///< The total time spend playing in seconds.

	int m_physicsSimNumSubSteps = 3;
	vec3f m_defaultGravity = vec3f(0.f, -10.f, 0.f);

	/// Called when a level has just been loaded after deserializing is done.
	EventEmitter<> onWorldLoaded;

	std::vector<std::unique_ptr<IPostSceneUpdateTask>> m_postSceneUpdateTasks;

	/// If set this is the file that we are currently working with. Otherwise we are not working with a saved level.
	std::string m_workingFilePath;

	/// Script objects to get called.
	std::vector<ObjectId> m_scriptObjects;

	/// True if the game is in edit mode
	bool isEdited = true;

	// Editing Grid.
	bool gridShouldDraw = true;
	vec2i gridNumSegments = vec2i(10);
	float gridSegmentsSpacing = 1.f;

	/// Debugging variables.
	mutable struct {
		/// Forces the update loop to sleep for the specified amount of miliseconds before upadating.
		/// Useful for checking if game logic works for any timestep.
		int forceSleepMs = 0;
	} debug;

	// Audio stuff
	float m_masterVolume = 1.0f;
};

} // namespace sge
