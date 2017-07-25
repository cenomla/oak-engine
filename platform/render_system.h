#pragma once

#include <graphics/buffer_storage.h>
#include <graphics/static_batcher.h>
#include <graphics/sprite_batcher.h>
#include <graphics/particle_system.h>
#include <graphics/pipeline.h>
#include <system.h>
#include <entity_cache.h>

namespace oak {
	class Scene;
}

namespace oak::graphics {
	class Api;
	class Renderer;
}

class RenderSystem : public oak::System {
public:
	RenderSystem(oak::Scene *scene, oak::graphics::Api *api);

	void pushLayerFront(oak::graphics::Renderer *renderer);
	void pushLayerBack(oak::graphics::Renderer *renderer);

	void removeLayer(oak::graphics::Renderer& renderer);

	void init() override;
	void terminate() override;
	void run() override;
	
private:
	oak::Scene *scene_;
	oak::EntityCache meshCache_;

	oak::graphics::Api *api_;
	oak::graphics::Pipeline pipeline_;
	oak::vector<oak::graphics::Renderer*> layers_;
	oak::vector<oak::graphics::Batch> batches_;
	
	oak::graphics::BufferStorage storageMesh_;
};