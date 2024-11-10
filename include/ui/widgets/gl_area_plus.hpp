#pragma once

#include <gtkmm.h>

#include "ui/widgets/gl/buffer.hpp"
#include "ui/widgets/gl/program.hpp"
#include "ui/widgets/gl/texture.hpp"

namespace UI {

class GLAreaPlus : public Gtk::GLArea {
  // static GLAreaPlus* s_contextHolder;

  std::list<std::weak_ptr<GL::Object>> m_objects;

public:
  GLAreaPlus(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~GLAreaPlus();

  std::shared_ptr<GL::Texture> createTexture();
  std::shared_ptr<GL::Buffer> createBuffer();
  std::shared_ptr<GL::Program> createProgram(const char *vert, const char *frag);

protected:
  Glib::RefPtr<Gdk::GLContext> createContext();
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context) = 0;
  virtual void unrealize();

  template<typename T> std::shared_ptr<T> wrap(T* ptr) {
    std::shared_ptr<T> newPtr(ptr, [this](T* toDelete) {
      // Destructor manages the gl context
      delete toDelete;

      // Remove all expired pointers
      auto iter = m_objects.begin();
      while(iter != m_objects.end()) {
        if(iter->expired()) {
          iter = m_objects.erase(iter);
        } else {
          ++iter;
        }
      }
    });
    m_objects.push_back(newPtr);
    return newPtr;
  }
};

} // namespace UI

