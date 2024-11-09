#pragma once

#include "ui/widgets/gl_area_plus.hpp"
#include "img/provider.hpp"
#include <gtkmm.h>

namespace UI {

class ViewImage {
  std::shared_ptr<GL::Buffer> m_vertices;
  std::shared_ptr<GL::Texture> m_texture;
  float m_matrix[9];

public:
  float m_colorMultiplier;

  ViewImage(GLAreaPlus& area);
  ~ViewImage() = default;

  void make_vertices(float scaleX, float scaleY);
  void load_texture(Img::ImageProvider& image, int index);

  void render(GL::Program& program);
};

class MainView : public GLAreaPlus {
  std::shared_ptr<GL::Program> m_program;

  float m_offset[2];
  float m_scale;

public:
  ViewImage* m_image;

  MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainView() = default;

protected:
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context);
};

}

