#include "test_renderer.h"

#include "opengl/gl_renderer.h"
#include "components.h"

namespace oak::graphics {

	TestRenderer::TestRenderer(Scene& scene) : scene_{ &scene } {}

	void TestRenderer::init() {
		renderer_ = new GLRenderer{};
		renderer_->init();

		cache_.requireComponent<TransformComponent>();
		cache_.requireComponent<MeshComponent>();
	}

	void TestRenderer::run() {
		batcher_.run();

		const char *data = static_cast<const char*>(batcher_.getData());

		for (const auto& batch : batcher_.getBatches()) {
			renderer_->render(data + batch.offset, batch);
		}

	}

}