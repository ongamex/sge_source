#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "basetypes.h"
#include "sge_utils/sge_utils.h"

namespace sge {

/// A class used for managing lifetime of callbacks registered in EventEmitter.
/// When a callback is registered to a list a EventSubscription is created and
/// when this object gets destoryed the callback is unregistered from the EventEmitter.
struct EventSubscription : public NoncopyableMovable {
	EventSubscription() = default;

	EventSubscription(std::function<void()> unsubscribeFn)
	    : unsubscribeFn(std::move(unsubscribeFn)) {}

	~EventSubscription() { unsubscribe(); }

	/// Unregisters the callback for the owning EventEmitter.
	void unsubscribe() {
		if (unsubscribeFn != nullptr) {
			unsubscribeFn();
			unsubscribeFn = nullptr;
		}
	}

	/// If called the lifetime of the callback will no longer be maintained
	/// and it will get called until the owning EventEmitter exists.
	void abandon() { unsubscribeFn = nullptr; }

	EventSubscription(EventSubscription&& other) noexcept {
		this->unsubscribeFn = std::move(other.unsubscribeFn);
		other.unsubscribeFn = nullptr;
	}

	EventSubscription& operator=(EventSubscription&& other) noexcept {
		// Unsubscribe from the current event, as this EventSubscription
		// will start taking care of another one.
		this->unsubscribe();

		this->unsubscribeFn = std::move(other.unsubscribeFn);
		other.unsubscribeFn = nullptr;

		return *this;
	}

  private:
	std::function<void()> unsubscribeFn;
};

/// EventEmitter holds callbacks which are subscribed to it.
/// These callbacks could be invoked by the EventEmitter, with EventEmitter::invokeEvent.
template <typename... TArgs>
struct EventEmitter : public NoncopyableMovable {
  private:
	struct Internal {
		/// @callbacks maps the id of the sucbscriber to its callback function.
		std::unordered_map<int, std::function<void(TArgs...)>> callbacks;
		int nextFreeId = 0;
		std::mutex dataLock;
	};

	std::shared_ptr<Internal> data;

  public:
	EventEmitter()
	    : data(std::make_shared<Internal>()) {}

	EventEmitter(EventEmitter&& ref) noexcept
	    : data(std::move(ref.data)) {
		ref.data = std::make_shared<Internal>();
	}

	EventEmitter& operator=(EventEmitter&& ref) noexcept {
		data = std::move(ref.data);
		ref.data = std::make_shared<Internal>();
		return *this;
	}

	/// Add a new callback to be called.
	/// @retval a EventSubscription used to manage the lifetime of the
	///         registered callback, can be used to unsubscribe.
	///         Basically the event provider needs to hold this token as long
	///         as it the callback is callable. Once the object dies the EventSubscription
	/// Will kick in and unregister the subscription.
	[[nodiscard]] EventSubscription subscribe(std::function<void(TArgs...)> fn) {
		const std::lock_guard<std::mutex> g(data->dataLock);
		const int id = data->nextFreeId++;
		sgeAssert(data->callbacks.count(id) == 0);
		data->callbacks[id] = std::move(fn);

		std::weak_ptr<Internal> weakData = data;
		return EventSubscription(
		    // This is the unregister function tall will be held by the EventSubscription
		    // which will get called when EventSubscription gets destroyed.
		    [id, weakData]() -> void {
			    if (std::shared_ptr<Internal> strongData = weakData.lock()) {
				    auto itr = strongData->callbacks.find(id);
				    if_checked(itr != strongData->callbacks.end()) { strongData->callbacks.erase(itr); }
			    }
		    });
	}

	void invokeEvent(TArgs... args) const {
		// If you crash here, then this mutex is probably locked for a second time.
		const std::lock_guard<std::mutex> g(data->dataLock);

		for (auto& callback : data->callbacks) {
			if_checked(callback.second) { callback.second(args...); }
		}
	}

	/// @brief Returns true if there aren't any subscribes to the event.
	bool isEmpty() const {
		const std::lock_guard<std::mutex> g(data->dataLock);
		return data->callbacks.empty();
	}

	void discardAllCallbacks() {
		const std::lock_guard<std::mutex> g(data->dataLock);
		data->callbacks.clear();
	}
};

} // namespace sge
