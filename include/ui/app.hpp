#pragma once

#include <gtkmm.h>
#include "ui/window.hpp"

namespace UI {

class App : public Gtk::Application {
  Window *m_window;
  std::shared_ptr<UI::State> m_state;

  Glib::RefPtr<Gtk::FileDialog> m_fileDialog;

  Glib::RefPtr<Gio::SimpleAction> m_saveAction;

public:
  App();
  virtual ~App() = default;

  static Glib::RefPtr<App> create();

protected:
  void startup();

  void openFile(const Glib::VariantBase& variant);
  void openFileFinish(Glib::RefPtr<Gio::AsyncResult>& result);

  void saveFile(const Glib::VariantBase& variant);
};

}

