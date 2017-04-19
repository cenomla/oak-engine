#pragma once

#include "entity_id.h"

namespace oak {

	class ComponentStorage;

	using ComponentHandleStorage = TypeHandleStorage<detail::BaseComponent, config::MAX_COMPONENTS>;

	class Scene {
	public:

		void terminate();

		EntityId createEntity(uint64_t idx);

		void destroyEntity(EntityId entity);
		void activateEntity(EntityId entity);
		void deactivateEntity(EntityId entity);

		bool isEntityAlive(EntityId entity) const;
		bool isEntityActive(EntityId entity) const;

		void addComponent(EntityId entity, size_t tid);
		void addComponent(EntityId entity, size_t tid, const void* ptr);
		void removeComponent(EntityId entity, size_t tid);

		void* getComponent(EntityId entity, size_t tid);
		const void* getComponent(EntityId entity, size_t tid) const;
		bool hasComponent(EntityId entity, size_t tid) const;
		const std::bitset<config::MAX_COMPONENTS>& getComponentFilter(EntityId entity);

		void update();

		void addComponentStorage(size_t tid, ComponentStorage& storage);

		size_t getEntityCount() const { return entities_.size(); }
	private:
		oak::vector<EntityId> entities_;
		oak::vector<EntityId> killed_;

		oak::vector<std::bitset<config::MAX_COMPONENTS>> componentMasks_;
		oak::vector<std::bitset<4>> flags_;

		//stores the generation of each entity
		oak::vector<uint32_t> generations_;
		//stores the indices free for reuse
		oak::deque<uint32_t> freeIndices_;

		std::vector<ComponentStorage*> componentPools_;
		
		void ensureSize(size_t size);
		void removeAllComponents(EntityId entity);
	};

}