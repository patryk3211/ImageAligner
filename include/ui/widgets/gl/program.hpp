#pragma once

#include "ui/widgets/gl/object.hpp"
#include <string>
#include <unordered_map>

namespace UI::GL {

class Program : public Object {
  uint m_id;

  std::unordered_map<std::string, uint> m_locations;

public:
  Program(const Glib::RefPtr<Gdk::GLContext>& ctx, const char* vertSource, const char* fragSource);
  virtual ~Program();

  virtual void destroy() override;

  void use();

  uint uniformLocation(const std::string& name);
};

} // namespace UI::GL

