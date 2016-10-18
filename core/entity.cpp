#include "entity.h"

#include <algorithm>

#include "engine.h"

namespace oak {

	EntityManager::EntityManager(Engine &engine) : System{ engine, "entity_manager" } {}

	void EntityManager::init() {
		engine_.getTaskManager().addTask({ [this](){ update(); }, Task::LOOP_BIT });
	}

	void EntityManager::destroy() {
		reset();
		for (const auto &block : componentHandles_) {
			if (block.ptr != nullptr) {
				static_cast<TypeHandleBase*>(block.ptr)->~TypeHandleBase();
				MemoryManager::inst().deallocate(block.ptr, block.size);
			}
		}
	}

	Entity EntityManager::createEntity(uint8_t layer) {
		uint32_t idx;
		if (freeIndices_.size() > 1024) {
			idx = freeIndices_.front();
			freeIndices_.pop_front();
		} else {
			idx = generation_.size();
			generation_.push_back(0);
		}

		entities_.alive.emplace_back(idx, generation_[idx], layer, this);
		entityAttributes_.resize(entities_.alive.size());
		return entities_.alive.back();
	}

	void EntityManager::destroyEntity(const Entity &entity) {
		entities_.deactivated.push_back(entity);
		entities_.killed.push_back(entity);
	}

	void EntityManager::activateEntity(const Entity &entity) {
		entities_.activated.push_back(entity);
	}

	void EntityManager::deactivateEntity(const Entity &entity) {
		entities_.deactivated.push_back(entity);
	}

	bool EntityManager::isEntityActive(const Entity &entity) const {
		return entityAttributes_[entity.index()].flags[0];
	}

	void* EntityManager::addComponent(uint32_t idx, uint32_t tid, size_t size) {
		auto &attribute = entityAttributes_[idx];
		attribute.componentMask[tid] = true;
		if (attribute.components[tid].ptr == nullptr) {
			attribute.components[tid] = { MemoryManager::inst().allocate(size), size };
		}
		return attribute.components[tid].ptr;
	}

	void EntityManager::removeComponent(uint32_t idx, uint32_t tid) {
		auto &attribute = entityAttributes_[idx];
		attribute.componentMask[tid] = false;
		if (attribute.components[tid].ptr != nullptr) {
			static_cast<TypeHandleBase*>(componentHandles_[tid].ptr)->destruct(attribute.components[tid].ptr);
		}
	}

	void EntityManager::deleteComponent(uint32_t idx, uint32_t tid) {
		auto const& attribute = entityAttributes_[idx];
		if (attribute.components[tid].ptr != nullptr) {
			const Block &block = attribute.components[tid];
			MemoryManager::inst().deallocate(block.ptr, block.size);
		}
	}

	void EntityManager::removeAllComponents(uint32_t idx) {
		for (size_t i = 0; i < MAX_COMPONENTS; i++) {
			removeComponent(idx, i);
			deleteComponent(idx, i);
		}
	}

	void* EntityManager::getComponent(uint32_t idx, uint32_t tid) {
		return entityAttributes_[idx].components[tid].ptr;
	}

	const void* EntityManager::getComponent(uint32_t idx, uint32_t tid) const {
		return entityAttributes_[idx].components[tid].ptr;
	}

	bool EntityManager::hasComponent(uint32_t idx, uint32_t tid) const {
		return entityAttributes_[idx].componentMask[tid];
	}

	void EntityManager::update() {
		std::unordered_map<size_t, EntityCache*> cachesToSort;
		//remove all entities to remove
		for (auto &e : entities_.activated) {
			auto &attribute = entityAttributes_[e.index()];
			attribute.flags[0] = true;

			//add entity to relavent caches
			for (auto &s : caches_) {
				size_t sysType = s.first;

				EntityCache &sys = *s.second;
				//if the entity has all the components that the system requires then add it to the system
				if ((sys.getFilter() & attribute.componentMask) == sys.getFilter()) {
					if (sysType >= attribute.caches.size() || !attribute.caches[sysType]) {
						sys.addEntity(e);
						cachesToSort.insert(s);
						if (sysType >= attribute.caches.size()) {
							attribute.caches.resize(sysType + 1);
						}
						attribute.caches[sysType] = true;
					}
				}
				//if the entity doesn't match the component requirement but still is in the system remove it
				else if (attribute.caches.size() > sysType && attribute.caches[sysType]) {
					sys.removeEntity(e);
					attribute.caches[sysType] = false;
				}
			}

		}

		for (auto &sys : cachesToSort) {
			sys.second->sort();
		}

		for (auto &e : entities_.deactivated) {
			auto &attribute = entityAttributes_[e.index()];
			attribute.flags[0] = false;

			for (auto &s : caches_) {
				size_t sysType = s.first;

				EntityCache &sys = *s.second;
				//if the entity is apart of a system then remove it
				if (attribute.caches.size() > sysType && attribute.caches[sysType]) {
					sys.removeEntity(e);
					attribute.caches[sysType] = false;
				}
			}
		}

		for (auto &e : entities_.killed) {
			size_t idx = e.index();
			//remove all the entities components
			removeAllComponents(idx);

			//remove the entity from the alive list and recycle the index
			entities_.alive.erase(std::remove(std::begin(entities_.alive), std::end(entities_.alive), e), std::end(entities_.alive));

			++generation_[idx];
			freeIndices_.push_back(idx);
		}


		entities_.clear();
	}

	void EntityManager::reset() {
		for (auto &e : entities_.alive) {
			removeAllComponents(e.index());
		}
		for (auto &s : caches_) {
			s.second->clearEntities();
		}
		for (auto &a : entityAttributes_) {
			a.flags.reset();
			a.caches.clear();
		}
		entities_.reset();
		generation_.clear();
		freeIndices_.clear();
	}

	void EntityManager::enable() {
		for (auto &entity : entities_.alive) {
			auto &attribute = entityAttributes_[entity.index()];
			//if the entity is supposed to be active then add it to the appropriate caches
			if (attribute.flags[0]) {
				for (auto &system : caches_) {
					if (attribute.caches[system.first]) {
						system.second->addEntity(entity);
					}
				}
			}
		}
	}

	void EntityManager::disable() {
		for (auto &entity : entities_.alive) {
			auto &attribute = entityAttributes_[entity.index()];
			for (auto &system : caches_) {
				//if the entity is apart of the system remove it but do not set its inactive flag
				if (attribute.caches[system.first]) {
					system.second->removeEntity(entity);
				}
			}
		}
	}

	void EntityManager::addCache(EntityCache *cache) {
		caches_.insert({caches_.size(), cache});
	}

	void EntityCache::setOnAdd(const std::function<void (const Entity &e)> &func) {
		onAdd_ = func;
	}

	void EntityCache::setOnRemove(const std::function<void (const Entity &e)> &func) {
		onRemove_ = func;
	}

	void EntityCache::addEntity(const Entity &e) {
		entities_.push_back(e);
		if (onAdd_) {
			onAdd_(e);
		}
	}

	void EntityCache::removeEntity(const Entity &e) {
		if (onRemove_) {
			onRemove_(e);
		}
		entities_.erase(std::remove(std::begin(entities_), std::end(entities_), e), std::end(entities_));
	}

	void EntityCache::sort() {
		std::sort(std::begin(entities_), std::end(entities_), [](const Entity &first, const Entity &second) -> bool {
			return first.index() < second.index();
		});
	}

}