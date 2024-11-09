#pragma once

#include "ui/widgets/gl_area_plus.hpp"
#include "ui/widgets/sequence_list.hpp"
#include "img/provider.hpp"
#include <gtkmm.h>

namespace UI {
class State;

class ViewImage {
  std::shared_ptr<GL::Buffer> m_vertices;
  std::shared_ptr<GL::Texture> m_texture;

public:
  float m_matrix[9];
  float m_colorMultiplier;
  int m_sequenceImageIndex;

  ViewImage(GLAreaPlus& area);
  ~ViewImage() = default;

  void make_vertices(float scaleX, float scaleY);
  void load_texture(Img::ImageProvider& image, int index);

  void render(GL::Program& program);
};

class MainView : public GLAreaPlus {
  std::shared_ptr<GL::Program> m_program;
  std::shared_ptr<UI::State> m_state;
  SequenceView* m_sequenceView;

  // std::shared_ptr<Gtk::GestureDrag> m_dragGesture;

  std::list<std::shared_ptr<ViewImage>> m_images;

  float m_offset[2];
  float m_scale;

  float m_savedOffset[2];

public:
  MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

protected:
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  void dragBegin(double startX, double startY);
  void dragUpdate(double offsetX, double offsetY);
  bool scroll(double x, double y);

  void sequenceViewSelectionChanged(uint position, uint nitems);
};

}

