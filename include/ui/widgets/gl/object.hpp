#pragma once

#include <cstdlib>
#include <gtkmm.h>

#define GL_GLEXT_PROTOTYPES

namespace UI::GL {

class Object {
  Glib::RefPtr<Gdk::GLContext> m_context;

protected:
  void prepare_context();

public:
  const static uint NULL_ID = -1;

  Object(const Glib::RefPtr<Gdk::GLContext>& ctx);
  virtual ~Object() = default;

  virtual void destroy() = 0;
};

} // namespace UI::GL

