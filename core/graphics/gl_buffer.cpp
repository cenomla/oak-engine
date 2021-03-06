#include "gl_buffer.h"

#include <glad/glad.h>

#include "buffer.h"

namespace oak::graphics::GLBuffer {

	static const GLenum types[] = {
		GL_ARRAY_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER,
		GL_UNIFORM_BUFFER,
		GL_PIXEL_PACK_BUFFER,
		GL_PIXEL_UNPACK_BUFFER
	};

	static const GLenum hints[] = {
		GL_STATIC_DRAW,
		GL_STREAM_DRAW
	};

	static const GLenum buffer_access[] = {
		GL_READ_ONLY,
		GL_WRITE_ONLY,
		GL_READ_WRITE
	};

	void bind(const Buffer& buffer) {
		glBindBuffer(types[static_cast<int>(buffer.info.type)], buffer.id);
	}

	void unbind(const Buffer& buffer) {
		glBindBuffer(types[static_cast<int>(buffer.info.type)], 0);
	}

	Buffer create(const BufferInfo& info) {
		Buffer buffer;
		glGenBuffers(1, &buffer.id);

		if (info.base != -1) {
			glBindBufferBase(types[static_cast<int>(info.type)], info.base, buffer.id);
		}
		buffer.info = info;
		return buffer;
	}

	void destroy(Buffer& buffer) {
		if (buffer.id) {
			glDeleteBuffers(1, &buffer.id);
			buffer.id = 0;
		}
	}

	void *map(const Buffer& buffer, BufferAccess access) {
		return glMapBuffer(types[static_cast<int>(buffer.info.type)], buffer_access[static_cast<int>(access)]);
	}

	void unmap(const Buffer& buffer) {
		glUnmapBuffer(types[static_cast<int>(buffer.info.type)]);
	}

	void data(const Buffer& buffer, size_t size, const void *data) {
		glBufferData(types[static_cast<int>(buffer.info.type)], size, data, hints[static_cast<int>(buffer.info.hint)]);
	}

	void data(const Buffer& buffer, size_t offset, size_t size, const void *data) {
		glBufferSubData(types[static_cast<int>(buffer.info.type)], offset, size, data);
	}

}
