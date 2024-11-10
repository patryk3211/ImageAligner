#pragma once

#include "ui/widgets/gl/object.hpp"
#include <string>
#include <filesystem>
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

  void uniform1i(const std::string& name, int value);
  
  void uniform1f(const std::string& name, float value);
  void uniform2f(const std::string& name, const float *value);
  void uniform2f(const std::string& name, float x, float y);
  void uniform3f(const std::string& name, const float *value);
  void uniform3f(const std::string& name, float x, float y, float z);
  void uniform4f(const std::string& name, const float *value);
  void uniform4f(const std::string& name, float x, float y, float z, float w);


  void uniformMat3fv(const std::string& name, int count, bool transpose, const float *data);
  void uniformMat4fv(const std::string& name, int count, bool transpose, const float *data);

  static Program *from_source(const Glib::RefPtr<Gdk::GLContext>& ctx, const std::string& vert, const std::string& frag);
  static Program *from_stream(const Glib::RefPtr<Gdk::GLContext>& ctx, std::istream& vert, std::istream& frag);
  static Program *from_path(const Glib::RefPtr<Gdk::GLContext>& ctx, const std::filesystem::path& vert, const std::filesystem::path& frag);

};

} // namespace UI::GL

