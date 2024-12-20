#pragma once

#include <gtkmm.h>
#include "objects/image.hpp"

namespace UI {
class State;

class SequenceView : public Gtk::ColumnView {
  Glib::RefPtr<Gio::ListStore<Obj::Image>> m_model;

  using Factory = Glib::RefPtr<Gtk::SignalListItemFactory>;
  using ListItem = Glib::RefPtr<Gtk::ListItem>;

  Factory m_idColFactory;
  Factory m_selectColFactory;
  Factory m_xOffsetFactory;
  Factory m_yOffsetFactory;

  int m_lastRefIndex;
  Gtk::SpinButton *m_refImageSelector;

public:
  SequenceView();
  SequenceView(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder);

  virtual ~SequenceView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

  Glib::RefPtr<Gio::ListStore<Obj::Image>>& model();

  uint getSelectedIndex();
  Glib::RefPtr<Obj::Image> getSelected();

  Glib::RefPtr<Obj::Image> getImage(int index);

  void prevImage();
  void nextImage();

private:
  void referenceChanged();

  static void labelColSetup(const ListItem& item);
  static void checkboxColSetup(const ListItem& item);
  static void idColBind(const ListItem& item);
  static void selectColBind(const ListItem& item);
  static void xColBind(const ListItem& item);
  static void yColBind(const ListItem& item);
};

} // namespace UI
