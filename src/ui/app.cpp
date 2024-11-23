#include "ui/app.hpp"
#include "ui/widgets/util.hpp"
#include "ui/state.hpp"

#include <spdlog/spdlog.h>

using namespace UI;

App::App()
  : Application("org.patryk3211.imagealign")
  , m_fileDialog(Gtk::FileDialog::create()) {
  UI::initCustomWidgets();
  m_recentManager = Gtk::RecentManager::get_default();
  m_window = new Window();
  m_window->set_title("Image Aligner");

  auto filters = Gio::ListStore<Gtk::FileFilter>::create();
  auto seqFilter = Gtk::FileFilter::create();
  seqFilter->set_name("Sequence File");
  seqFilter->add_pattern("*.seq");
  filters->append(seqFilter);
  m_fileDialog->set_filters(filters);

  signal_startup().connect(sigc::mem_fun(*this, &App::startup));

  auto openAction = Gio::SimpleAction::create("open");
  openAction->signal_activate().connect(sigc::mem_fun(*this, &App::openFile), false);
  add_action(openAction);

  m_saveAction = Gio::SimpleAction::create("save");
  m_saveAction->signal_activate().connect(sigc::mem_fun(*this, &App::saveFile), false);
  m_saveAction->set_enabled(false);
  add_action(m_saveAction);
}

void App::startup() {
  add_window(*m_window);
  m_window->show();
}

void App::openFile(const Glib::VariantBase& variant) {
  m_fileDialog->open(*m_window, sigc::mem_fun(*this, &App::openFileFinish), nullptr);
}

void App::saveFile(const Glib::VariantBase& variant) {
  if(m_state) {
    m_state->saveSequence();
    spdlog::info("Sequence saved under the same name");
  }
}

void App::openFileFinish(Glib::RefPtr<Gio::AsyncResult>& result) {
  try {
    auto file = m_fileDialog->open_finish(result);

    m_state = UI::State::fromSequenceFile(file->get_path());
    if(m_state) {
      m_recentManager->add_item(file->get_uri());
      spdlog::info("{}", file->get_uri());
      m_window->setState(m_state);
      m_saveAction->set_enabled(true);
    } else {
      m_window->setState(nullptr);
      m_saveAction->set_enabled(false);
    }
  } catch(Gtk::DialogError e) {
    // No file was selected
  }
}

Glib::RefPtr<App> App::create() {
  return Glib::make_refptr_for_instance<App>(new App());
}

