#include "ui/widgets/util.hpp"
#include "ui/widgets/sequence_list.hpp"

#include <format>

void UI::initCustomWidgets() {
  // Make sure GTK knows about our custom widgets by calling their dummy constructors
  static_cast<void>(UI::SequenceView());
}

static void setupLabel(const Glib::RefPtr<Gtk::ListItem>& item) {
  auto label = Gtk::make_managed<Gtk::Label>();
  item->set_child(*label);
}

static void bindLabelInt(const Glib::RefPtr<Gtk::ListItem>& item, const char *property) {
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());
  Glib::PropertyProxy<int> proxy(item->get_item().get(), property);
  Glib::Binding::bind_property(proxy, label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const int& value) {
    return std::format("{}", value);
  });
}

static void bindLabelFloat(const Glib::RefPtr<Gtk::ListItem>& item, const char *property) {
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());
  Glib::PropertyProxy<float> proxy(item->get_item().get(), property);
  Glib::Binding::bind_property(proxy, label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const float& value) {
    return std::format("{:.2f}", value);
  });
}

void UI::addIntColumn(Gtk::ColumnView *view, const char *title, const char *propertyName) {
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect(sigc::ptr_fun(&setupLabel));
  factory->signal_bind().connect(sigc::bind(sigc::ptr_fun(&bindLabelInt), propertyName));
  view->append_column(Gtk::ColumnViewColumn::create(title, factory));
}

void UI::addFloatColumn(Gtk::ColumnView *view, const char *title, const char *propertyName) {
  auto factory = Gtk::SignalListItemFactory::create();
  factory->signal_setup().connect(sigc::ptr_fun(&setupLabel));
  factory->signal_bind().connect(sigc::bind(sigc::ptr_fun(&bindLabelFloat), propertyName));
  view->append_column(Gtk::ColumnViewColumn::create(title, factory));
}

