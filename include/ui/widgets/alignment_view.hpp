#pragma once

#include "gtkmm/spinbutton.h"
#include "ui/widgets/gl/program.hpp"
#include "ui/widgets/gl/texture.hpp"
#include "ui/widgets/gl_area_plus.hpp"
#include "ui/widgets/main_view.hpp"
#include "ui/widgets/sequence_list.hpp"

namespace UI {
class State;

class AlignmentView : public GLAreaPlus {
  std::shared_ptr<UI::State> m_state;

  std::shared_ptr<GL::Program> m_program;
  std::shared_ptr<GL::Texture> m_refTexture;
  std::shared_ptr<GL::Texture> m_alignTexture;

  SequenceView *m_sequenceView;
  MainView *m_mainView;
  Gtk::AspectFrame *m_aspectFrame;

  Gtk::SpinButton *m_refImgBtn;
  Gtk::SpinButton *m_viewTypeBtn;
  Gtk::SpinButton *m_viewParamBtn;

  float m_refAspect;
  float m_viewSection[4];
  
public:
  AlignmentView(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
  virtual ~AlignmentView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

protected:
  virtual void realize();
  virtual bool render(const Glib::RefPtr<Gdk::GLContext>& context);

  void referenceChanged();
  void sequenceViewSelectionChanged(uint position, uint nitems);
  void viewTypeChanged();
  void pickArea();
};

}

