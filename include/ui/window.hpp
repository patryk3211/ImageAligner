#pragma once

#include <gtkmm.h>

#include "ui/widgets/main_view.hpp"
#include "ui/widgets/sequence_list.hpp"

namespace UI {

class Window : public Gtk::Window {
  Glib::RefPtr<Gtk::FileDialog> m_dialog;

  SequenceView* m_sequenceView;
  MainView* m_mainView;

  std::shared_ptr<UI::State> m_state;

public:
  Window();
  virtual ~Window() = default;

  void setState(const std::shared_ptr<UI::State>& state);

private:
  void openFileDialog();
  void openFileFinish(Glib::RefPtr<Gio::AsyncResult>& result);
};

} // namespace UI
