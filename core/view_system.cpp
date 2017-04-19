#include "view_system.h"

#include <glm/gtc/matrix_transform.hpp>

namespace oak {

	void ViewSystem::defineView(size_t viewId, const oak::vector<uint32_t>& layers) {
		for (auto layer : layers) {
			if (layer >= layerToView_.size()) {
				layerToView_.resize(layer + 1);
			}
			layerToView_[layer] = viewId;
		}
		if (viewId >= views_.size()) {
			views_.resize(viewId + 1);
		}
		auto &view = views_[viewId];
		view.ubo.create();
		view.ubo.bindBufferBase(viewId);
	}

	void ViewSystem::setView(size_t viewId, const View& view) {
		auto &it = views_[viewId];
		it.view = view;
		it.matrixBlock.proj = glm::ortho(0.0f, it.view.width, it.view.height, 0.0f, -1.0f, 1.0f);
		it.matrixBlock.view = glm::translate(glm::mat4{ 1.0f }, glm::vec3{ -it.view.x, -it.view.y, 0.0f });
		it.matrixBlock.model = glm::mat4{ 1.0f };

		it.ubo.bind();
		it.ubo.bufferData(sizeof(it.matrixBlock), &it.matrixBlock, GL_STREAM_DRAW);
		it.ubo.unbind();
	}

	glm::vec2 ViewSystem::transformPoint(size_t viewId, const glm::vec2& point) {
		auto &it = views_[viewId];

		return glm::vec2{ point.x + it.view.x, point.y + it.view.y };
	}

	size_t ViewSystem::getViewId(uint32_t layer) const {
		return layerToView_[layer];
	}

	const View& ViewSystem::getViewByLayer(uint32_t layer) const {
		return views_[layerToView_[layer]].view;
	}

	const View& ViewSystem::getView(size_t viewId) const {
		return views_[viewId].view;
	}

}