#include "ui/widgets/gl_area_plus.hpp"

#include <iostream>

using namespace UI;

GLAreaPlus::GLAreaPlus(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : GLArea(cobject) {

  signal_realize().connect(sigc::mem_fun(*this, &GLAreaPlus::realize), false);
  signal_render().connect(sigc::mem_fun(*this, &GLAreaPlus::render), false);
  signal_unrealize().connect(sigc::mem_fun(*this, &GLAreaPlus::unrealize), false);

}

std::shared_ptr<GL::Texture> GLAreaPlus::createTexture() {
  return make_pointer(new GL::Texture(get_context()));
}

std::shared_ptr<GL::Buffer> GLAreaPlus::createBuffer() {
  return make_pointer(new GL::Buffer(get_context()));
}

std::shared_ptr<GL::Program> GLAreaPlus::createProgram(const char *vert, const char *frag) {
  return make_pointer(new GL::Program(get_context(), vert, frag));
}

void GLAreaPlus::realize() {
  int major, minor;
  get_context()->get_version(major, minor);
  std::cout << "OpenGL context version " << major << "." << minor << std::endl;
}

void GLAreaPlus::unrealize() {
  // Delete all allocated GL objects
  for(auto& obj : m_objects) {
    if(obj.expired())
      continue;
    auto locked = obj.lock();
    locked->destroy();
  }
}

