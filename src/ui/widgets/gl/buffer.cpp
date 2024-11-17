#include "ui/widgets/gl/buffer.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

using namespace UI::GL;

Buffer::Buffer(const Glib::RefPtr<Gdk::GLContext>& ctx) : Object(ctx) {
  glGenBuffers(1, &m_id);
}

Buffer::~Buffer() {
  destroy();
}

void Buffer::destroy() {
  if(m_id != NULL_ID) {
    Object::destroy();
    glDeleteBuffers(1, &m_id);
    m_id = NULL_ID;
  }
}

void Buffer::bind() {
  // Bind is expecting the context to be correct
  glBindBuffer(GL_ARRAY_BUFFER, m_id);
}

void Buffer::unbind() {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Buffer::store(uint length, const void *data) {
  prepare_context();
  bind();
  glBufferData(GL_ARRAY_BUFFER, length, data, GL_STATIC_DRAW);
}

