#pragma once

#include "ui/widgets/gl/object.hpp"

namespace UI::GL {

class Buffer : public Object {
  uint m_id;

public:
  Buffer(const Glib::RefPtr<Gdk::GLContext>& ctx);
  virtual ~Buffer();

  void bind();
  void store(uint length, const void *data);

  virtual void destroy() override;
};

} // namespace UI::GL

