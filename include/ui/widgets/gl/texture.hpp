#pragma once

#include "ui/widgets/gl/object.hpp"

namespace UI::GL {

class Texture : public Object {
  uint m_id;

public:
  Texture(const Glib::RefPtr<Gdk::GLContext>& ctx);
  virtual ~Texture();

  virtual void destroy() override;

  void bind();

  void load(uint width, uint height, int srcFormat, int srcType, const void *data, int dstFormat);
};

} // namespace UI::GL

