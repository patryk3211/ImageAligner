#pragma once

#include "ui/widgets/gl_area_plus.hpp"
#include "ui/widgets/sequence_list.hpp"
#include "img/provider.hpp"
#include <functional>
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
  void registrationHomography(const double *homography, double refPixelSize);
  void resetHomography();

  void render(GL::Program& program);
};

class MainView : public GLAreaPlus {
  std::shared_ptr<GL::Program> m_imgProgram;
  std::shared_ptr<GL::Program> m_selectProgram;
  std::shared_ptr<UI::State> m_state;
  SequenceView* m_sequenceView;

  std::list<std::shared_ptr<ViewImage>> m_images;

  float m_offset[2];
  float m_scale;
  float m_savedOffset[2];

  float m_selection[4];
  bool m_makeSelection;
  std::optional<std::function<void(float, float, float, float)>> m_selectCallback;

public:
  MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

  void requestSelection(const std::function<void(float, float, float, float)>& callback, float forceAspect = 0);

protected:
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  void dragBegin(double startX, double startY);
  void dragUpdate(double offsetX, double offsetY);
  void dragEnd(double endX, double endY);
  bool scroll(double x, double y);

  void enableVertexAttribs();
  void disableVertexAttribs();

  void sequenceViewSelectionChanged(uint position, uint nitems);
};

}

