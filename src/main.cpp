#include <gtkmm/application.h>
#include <adwaita.h>

#include "ui/window.hpp"
#include "ui/widgets/util.hpp"

int main(int argc, char **argv) {
  adw_init();
  auto app = Gtk::Application::create("org.patryk3211.imagealign");
  UI::initCustomWidgets();

  auto display = Gdk::Display::get_default();
  auto cssProvider = Gtk::CssProvider::create();
  cssProvider->load_from_path("ui/main.scss");
  Gtk::StyleProvider::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  return app->make_window_and_run<UI::Window>(argc, argv);
}

