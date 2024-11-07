#include "ui/window.hpp"
#include "sigc++/functors/mem_fun.h"
#include "ui/state.hpp"

#include <filesystem>
#include <iostream>

using namespace UI;

Window::Window()
  : m_dialog(Gtk::FileDialog::create()) {
  auto builder = Gtk::Builder::create_from_file("ui/main.blp.ui");

  auto root = builder->get_widget<Gtk::Widget>("root");
  this->set_child(*root);

  auto btn_open_file = builder->get_widget<Gtk::Button>("btn_open_file");
  btn_open_file->signal_clicked().connect(sigc::mem_fun(*this, &Window::openFileDialog));

  m_sequence_view = Gtk::Builder::get_widget_derived<SequenceView>(builder, "sequence_view");
}

void Window::setState(const std::shared_ptr<UI::State>& state) {
  m_state = state;
  m_sequence_view->populateModel(*m_state);
}

void Window::openFileDialog() {
  m_dialog->open(*this, sigc::mem_fun(*this, &Window::openFileFinish), nullptr);
}

void Window::openFileFinish(Glib::RefPtr<Gio::AsyncResult>& result) {
  try {
    auto file = m_dialog->open_finish(result);

    auto path = std::filesystem::path(file->get_path());
    setState(UI::State::fromSequenceFile(path));
  } catch(Gtk::DialogError e) {
    // No file was selected
  }
}

