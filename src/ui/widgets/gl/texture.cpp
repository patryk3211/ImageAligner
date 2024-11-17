#include "ui/widgets/gl/texture.hpp"

#include <GL/gl.h>

using namespace UI::GL;

Texture::Texture(const Glib::RefPtr<Gdk::GLContext>& ctx) : Object(ctx) {
  glGenTextures(1, &m_id);

  glBindTexture(GL_TEXTURE_2D, m_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

Texture::~Texture() {
  destroy();
}

void Texture::destroy() {
  if(m_id != NULL_ID) {
    Object::destroy();
    glDeleteTextures(1, &m_id);
    m_id = NULL_ID;
  }
}

void Texture::bind() {
  // Bind is expecting the context to be correct
  glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::load(uint width, uint height, int srcFormat, int srcType, const void *data, int dstFormat) {
  prepare_context();
  bind();
  glTexImage2D(GL_TEXTURE_2D, 0, dstFormat, width, height, 0, srcFormat, srcType, data);
}

