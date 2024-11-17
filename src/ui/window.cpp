#include "ui/window.hpp"
#include "ui/state.hpp"

using namespace UI;

Window::Window() {
  auto builder = Gtk::Builder::create_from_file("ui/main.blp.ui");

  auto root = builder->get_widget<Gtk::Widget>("root");
  this->set_child(*root);

  m_sequenceView = Gtk::Builder::get_widget_derived<SequenceView>(builder, "sequence_view");
  m_mainView = Gtk::Builder::get_widget_derived<MainView>(builder, "main_gl_area");
  m_mainView->set_size_request(400, 400);

  m_alignmentView = Gtk::Builder::get_widget_derived<AlignmentView>(builder, "alignment_view");

  m_saveChangesDialog = 0;

  signal_close_request().connect(sigc::mem_fun(*this, &Window::closeRequest), false);
}

void Window::setState(const std::shared_ptr<UI::State>& state) {
  m_state = state;

  m_sequenceView->connectState(state);
  m_mainView->connectState(state);
  m_alignmentView->connectState(state);
}

bool Window::closeRequest() {
  if(m_state && m_state->m_sequence->isDirty()) {
    // Trying to close with unsaved changes
    m_saveChangesDialog = adw_alert_dialog_new("Save Changes?", "Open sequence has unsaved changes. Changes which are not saved will be permanently lost.");
    adw_alert_dialog_add_responses(ADW_ALERT_DIALOG(m_saveChangesDialog),
                                   "cancel", "Cancel",
                                   "discard", "Discard",
                                   "save", "Save",
                                   NULL);
    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(m_saveChangesDialog), "discard", ADW_RESPONSE_DESTRUCTIVE);
    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(m_saveChangesDialog), "save", ADW_RESPONSE_SUGGESTED);

    adw_alert_dialog_set_default_response(ADW_ALERT_DIALOG(m_saveChangesDialog), "cancel");
    adw_alert_dialog_set_close_response(ADW_ALERT_DIALOG(m_saveChangesDialog), "cancel");

    adw_alert_dialog_choose(ADW_ALERT_DIALOG(m_saveChangesDialog),
                            GTK_WIDGET(this->gobj()),
                            nullptr,
                            [](GObject *obj, GAsyncResult *result, void *udata) { static_cast<Window *>(udata)->saveChangesFinish(result); },
                            this);
    return true;
  }

  return false;
}

void Window::saveChangesFinish(GAsyncResult *result) {
  std::string response = adw_alert_dialog_choose_finish(ADW_ALERT_DIALOG(m_saveChangesDialog), result);
  m_saveChangesDialog = 0;

  if(response == "cancel") {
    // Do nothing
    return;
  } else if(response == "discard") {
    // Mark sequence as clean and try to close again
    m_state->m_sequence->markClean();
    this->close();
  } else if(response == "save") {
    // Save sequence and close again
    m_state->saveSequence();
    this->close();
  }
}

