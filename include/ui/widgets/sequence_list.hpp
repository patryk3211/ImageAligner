#pragma once

#include "gtkmm/spinbutton.h"
#include <gtkmm.h>

namespace UI {
class State;

class SequenceListItem : public Glib::Object {

public:
  Glib::Property<int> m_id;
  Glib::Property<bool> m_selected;

  SequenceListItem(int id, bool selected);
  virtual ~SequenceListItem() = default;

  static Glib::RefPtr<SequenceListItem> create(int id, bool selected);
};

class SequenceView : public Gtk::ColumnView {
  Glib::RefPtr<Gio::ListStore<SequenceListItem>> m_model;

  using Factory = Glib::RefPtr<Gtk::SignalListItemFactory>;
  using ListItem = Glib::RefPtr<Gtk::ListItem>;

  Factory m_idColFactory;
  Factory m_selectColFactory;

  Gtk::SpinButton *m_refImageSelector;

public:
  SequenceView();
  SequenceView(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder);

  virtual ~SequenceView() = default;

  void connectState(const std::shared_ptr<UI::State>& state);

  Glib::RefPtr<Gio::ListStore<SequenceListItem>>& model();

  uint getSelected();
  void prevImage();
  void nextImage();

private:
  static void labelColSetup(const ListItem& item);
  static void checkboxColSetup(const ListItem& item);
  static void idColBind(const ListItem& item);
  static void selectColBind(const ListItem& item);
};

} // namespace UI
