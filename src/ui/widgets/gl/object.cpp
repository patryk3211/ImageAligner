#include "gdkmm/glcontext.h"
#include <ui/widgets/gl/object.hpp>

using namespace UI::GL;

Object::Object(const Glib::RefPtr<Gdk::GLContext>& ctx)
  : m_context(ctx) {
  prepare_context();
}

void Object::prepare_context() {
  if(Gdk::GLContext::get_current() != m_context) {
    m_context->make_current();
  }
}

