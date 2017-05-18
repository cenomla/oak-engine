#include "entity_cache.h"

#include <algorithm>

#include "event_manager.h"
#include "event_queue.h"
#include "scene.h"
#include "scene_events.h"
#include "scene_util.h"
#include "event_comp.h"

namespace oak {

	void EntityCache::update(const Scene& scene) {
		bool needsSort = false;

		for (const auto& evt : EventManager::inst().getQueue<EntityActivateEvent>()) {
			ensureSize(evt.entity.index);
			//check if the entity matches the filter
			const auto& compFilter = scene.getComponentFilter(evt.entity);
			if (filter(evt.entity, compFilter, scene)) {
				if (!contains_[evt.entity]) {
					addEntity(evt.entity);
					needsSort = true;
				}
			} else {
				if (contains_[evt.entity]) {
					removeEntity(evt.entity);
					needsSort = true;
				}
			}
		}

		for (const auto& evt : EventManager::inst().getQueue<EntityDeactivateEvent>()) {
			ensureSize(evt.entity.index);
			//if the entity is contained then remove it
			if (contains_[evt.entity]) {
				removeEntity(evt.entity);
				needsSort = true;
			}
		}

		if (needsSort) {
			sort();
		}
	}

	void EntityCache::addEntity(EntityId entity) {
		entities_.push_back(entity);
		contains_[entity] = true;
	}

	void EntityCache::removeEntity(EntityId entity) {
		entities_.erase(std::remove(std::begin(entities_), std::end(entities_), entity), std::end(entities_));
		contains_[entity] = false;
	}

	void EntityCache::sort() {
		std::sort(std::begin(entities_), std::end(entities_), [](const EntityId& first, const EntityId& second) {
			return first.index < second.index;
		});
	}

	void EntityCache::ensureSize(size_t size) {
		if (contains_.size() <= size) {
			contains_.resize(size + 1);
		}
	}

	bool EntityCache::filter(EntityId entity, const std::bitset<config::MAX_COMPONENTS>& compFilter, const Scene& scene) {
		if (!scene.hasComponent(entity, util::type_id<detail::BaseComponent, EventComponent>::id)) {
			return (compFilter & componentFilter_) == componentFilter_;
		} else {
			const auto& ec = getComponent<const EventComponent>(getComponentStorage<EventComponent>(scene), entity);
			return ((compFilter & componentFilter_) == componentFilter_ && (ec.filter & eventFilter_) == eventFilter_);
		}
	}
}