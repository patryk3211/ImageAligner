#include <gtkmm/application.h>
#include <adwaita.h>

#include "ui/app.hpp"

#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::trace);

  adw_init();
  auto app = UI::App::create();

  auto display = Gdk::Display::get_default();
  auto cssProvider = Gtk::CssProvider::create();
  cssProvider->load_from_path("ui/main.scss");
  Gtk::StyleProvider::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_USER);

  return app->run();
}

