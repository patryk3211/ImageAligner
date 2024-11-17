#pragma once

#include <gtkmm.h>

#include "adwaita.h"
#include "ui/widgets/alignment_view.hpp"
#include "ui/widgets/main_view.hpp"
#include "ui/widgets/sequence_list.hpp"

namespace UI {

class Window : public Gtk::Window {
  std::shared_ptr<UI::State> m_state;

  // Glib::RefPtr<Glib::Object> m_saveChangesDialog;
  AdwDialog *m_saveChangesDialog;

public:
  SequenceView* m_sequenceView;
  MainView* m_mainView;
  AlignmentView* m_alignmentView;

public:
  Window();
  virtual ~Window() = default;

  void setState(const std::shared_ptr<UI::State>& state);

private:
  void showCloseDialog();
  bool closeRequest();

  void saveChangesFinish(GAsyncResult *result);
};

} // namespace UI

