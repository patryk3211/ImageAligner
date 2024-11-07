#include <gtkmm/application.h>
#include <adwaita.h>
#include "ui/window.hpp"
#include "ui/widgets/util.hpp"

int main(int argc, char **argv) {
  adw_init();
  auto app = Gtk::Application::create("org.patryk3211.imagealign");
  UI::initCustomWidgets();
  return app->make_window_and_run<UI::Window>(argc, argv);
}

