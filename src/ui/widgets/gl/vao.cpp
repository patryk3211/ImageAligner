#include "ui/widgets/gl/vao.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

using namespace UI::GL;

VAO::VAO(const Glib::RefPtr<Gdk::GLContext>& ctx)
  : Object(ctx) {
  glGenVertexArrays(1, &m_id);
}

VAO::~VAO() {
  destroy();
}

void VAO::bind() {
  glBindVertexArray(m_id);
}

void VAO::unbind() {
  glBindVertexArray(0);
}

void VAO::attribPointer(int index, int count, int type, bool normalized, int stride, uintptr_t offset) {
  glEnableVertexAttribArray(index);
  glVertexAttribPointer(index, count, type, normalized ? GL_TRUE : GL_FALSE, stride, (void *)offset);
}

void VAO::destroy() {
  if(m_id != NULL_ID) {
    Object::destroy();
    glDeleteVertexArrays(1, &m_id);
    m_id = NULL_ID;
  }
}

