#include "gl_vertex_array.h"

#include <glad/glad.h>

#include "attribute_layout.h"

namespace oak::graphics::GLVertexArray {

	static uint32_t bound;

	uint32_t create() {
		uint32_t id;
		glGenVertexArrays(1, &id);
		return id;
	}

	void destroy(uint32_t id) {
		glDeleteVertexArrays(1, &id);
	}

	void bind(uint32_t id) {
		if (id != bound) {
			glBindVertexArray(id);
			bound = id;
		}
	}

	void unbind() {
		if (bound != 0) {
			glBindVertexArray(0);
			bound = 0;
		}
	}

	static const GLenum gltype[] = {
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_UNSIGNED_BYTE,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_FLOAT,
		GL_UNSIGNED_BYTE
	};

	static const GLenum normalized[] = {
		GL_FALSE,
		GL_FALSE,
		GL_FALSE,
		GL_FALSE,
		GL_TRUE,
		GL_FALSE,		
		GL_FALSE,
		GL_FALSE,
		GL_FALSE,
		GL_FALSE,
		GL_TRUE
	};

	static const GLuint count[] = {
		3,
		2,
		3, 
		2, 
		4,
		16,
		3,
		2,
		3,
		2,
		4
	};

	static const GLuint divisor[] = {
		0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
	};

	static constexpr int glsize(GLenum type) {
		switch (type) {
			case GL_FLOAT: return 4;
			case GL_UNSIGNED_BYTE: return 1;
			default: return 0;
		}
	}

	void attribDescription(const AttributeLayout& layout) {
		const size_t stride = layout.stride();
		size_t offset = 0;
		for (const auto& attrib : layout.attributes) {
			int type = static_cast<int>(attrib);
			if (divisor[type] > 0) { continue; }
			glEnableVertexAttribArray(type);
			glVertexAttribPointer(type, count[type], gltype[type], normalized[type], stride, reinterpret_cast<void*>(offset));
			offset += glsize(gltype[type]) * count[type];
		}
	}

	void instanceAttribDescription(const AttributeLayout& layout, size_t offset) {
		const size_t stride = layout.instance_stride();
		for (const auto& attrib : layout.attributes) {
			int type = static_cast<int>(attrib);
			if (divisor[type] < 1) { continue; }
			glEnableVertexAttribArray(type);
			glVertexAttribPointer(type, count[type], gltype[type], normalized[type], stride, reinterpret_cast<void*>(offset));
			glVertexAttribDivisor(type, divisor[type]);
			offset += glsize(gltype[type]) * count[type];
		}
	}

}