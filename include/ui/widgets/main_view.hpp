#pragma once

#include "objects/image.hpp"
#include "ui/widgets/gl/vao.hpp"
#include "ui/widgets/gl_area_plus.hpp"
#include "ui/widgets/sequence_list.hpp"
#include "io/provider.hpp"
#include <functional>
#include <gtkmm.h>

namespace UI {
class State;
class MainView;

class ViewImage {
  Glib::RefPtr<Obj::Image> m_imageObject;
  std::shared_ptr<GL::Buffer> m_vertices;
  std::shared_ptr<GL::Texture> m_texture;
  std::shared_ptr<GL::VAO> m_vao;

  double m_pixelSize;
  double m_maxValue;

public:
  ViewImage(MainView& area, const Glib::RefPtr<Obj::Image>& image);
  ~ViewImage() = default;

private:
  void makeVertices(float scaleX, float scaleY);
  void loadTexture(IO::ImageProvider& image, int index);

public:
  Glib::RefPtr<Obj::Image> imageObject();
  void render(GL::Program& program);
};

class MainView : public GLAreaPlus {
  std::shared_ptr<GL::Program> m_imgProgram;
  std::shared_ptr<GL::Program> m_selectProgram;
  std::shared_ptr<UI::State> m_state;
  SequenceView* m_sequenceView;

  Gtk::CheckButton *m_hideUnselected;

  Gtk::SpinButton *m_minLevelBtn;
  Gtk::SpinButton *m_maxLevelBtn;
  Gtk::Scale *m_minLevelScale;
  Gtk::Scale *m_maxLevelScale;

  std::list<std::shared_ptr<ViewImage>> m_images;

  float m_offset[2];
  float m_scale;
  float m_savedOffset[2];

  float m_selection[4];
  bool m_makeSelection;
  std::optional<std::function<void(float, float, float, float)>> m_selectCallback;

  double m_pixelSize;

  Glib::RefPtr<Glib::Binding> m_levelBindings[2];

public:
  MainView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~MainView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

  void requestSelection(const std::function<void(float, float, float, float)>& callback, float forceAspect = 0);

  std::shared_ptr<ViewImage> getView(int seqIndex);

  double pixelSize() const;
  std::shared_ptr<UI::State> state();

protected:
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  void dragBegin(double startX, double startY);
  void dragUpdate(double offsetX, double offsetY);
  void dragEnd(double endX, double endY);
  bool scroll(double x, double y);

  void sequenceViewSelectionChanged(uint position, uint nitems);
};

}

