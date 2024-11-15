#pragma once

#include "ui/widgets/gl/object.hpp"

namespace UI::GL {

class VAO : public Object {
  uint m_id;

public:
  VAO(const Glib::RefPtr<Gdk::GLContext>& ctx);
  virtual ~VAO();

  void bind();
  static void unbind();

  void attribPointer(int index, int count, int type, bool normalized, int stride, uintptr_t offset);

  virtual void destroy() override;
};

} // namespace UI::GL

