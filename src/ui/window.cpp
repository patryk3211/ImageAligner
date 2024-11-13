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
}

void Window::setState(const std::shared_ptr<UI::State>& state) {
  m_sequenceView->connectState(state);
  m_mainView->connectState(state);
  m_alignmentView->connectState(state);
}

