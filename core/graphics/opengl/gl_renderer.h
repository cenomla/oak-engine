#pragma once

#include "system.h"

#include <vector>

#include "graphics/renderable.h"
#include "graphics/sprite.h"
#include "gl_buffer.h"
#include "gl_material.h"
#include "gl_vertex_array.h"

namespace oak::graphics {

	class GLRenderer : public System {
	public:
		GLRenderer(Engine &engine);

		void init() override;

		void addObject(const glm::vec2 &position, float depth, uint32_t layer, float rotation, float scale, const Renderable *object);

		void render();

	private:
		struct ObjectPos {
			const Renderable *object;
			glm::vec2 position;
			float depth; 
			uint32_t layer;
			float rotation, scale;

			inline bool operator<(const ObjectPos &other) const { 
				return layer == other.layer ? 
					depth < other.depth : 
					layer < other.layer; 
			}
		};

		struct Batch {
			const GLMaterial *material;
			size_t start, count;
			uint32_t layer;
		};

		std::vector<ObjectPos> objects_;
		std::vector<Batch> batches_;

		GLVertexArray vao_;
		GLBuffer vbo_;
		GLBuffer ibo_;
		
		size_t vertexCount_;
		size_t maxVertices_;
	};

}