#pragma once

#include <gtkmm.h>

namespace UI {
  void initCustomWidgets();

  void addIntColumn(Gtk::ColumnView *view, const char *title, const char *propertyName);
  void addFloatColumn(Gtk::ColumnView *view, const char *title, const char *propertyName);
}

