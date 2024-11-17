#include "ui/widgets/sequence_list.hpp"
#include "ui/state.hpp"

#include <string>
#include <format>

using namespace UI;
using namespace Obj;

SequenceView::SequenceView() : Glib::ObjectBase("SequenceView") {
  /* Dummy constructor for type registration */
}

SequenceView::SequenceView(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : Glib::ObjectBase("SequenceView")
  , Gtk::ColumnView(cobject)
  , m_model(Gio::ListStore<Image>::create())
  , m_idColFactory(Gtk::SignalListItemFactory::create())
  , m_selectColFactory(Gtk::SignalListItemFactory::create())
  , m_xOffsetFactory(Gtk::SignalListItemFactory::create())
  , m_yOffsetFactory(Gtk::SignalListItemFactory::create()) {
  // Create column factories
  m_idColFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::labelColSetup));
  m_idColFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::idColBind));
  m_selectColFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::checkboxColSetup));
  m_selectColFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::selectColBind));

  m_xOffsetFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::labelColSetup));
  m_xOffsetFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::xColBind));
  m_yOffsetFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::labelColSetup));
  m_yOffsetFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::yColBind));

  // Append columns
  append_column(Gtk::ColumnViewColumn::create("Id", m_idColFactory));
  append_column(Gtk::ColumnViewColumn::create("Selected", m_selectColFactory));
  append_column(Gtk::ColumnViewColumn::create("X Offset", m_xOffsetFactory));
  append_column(Gtk::ColumnViewColumn::create("Y Offset", m_yOffsetFactory));
  
  // Set selection model
  set_model(Gtk::SingleSelection::create(m_model));

  m_refImageSelector = builder->get_widget<Gtk::SpinButton>("ref_image_spin_btn");
  m_refImageSelector->signal_value_changed().connect(sigc::mem_fun(*this, &SequenceView::referenceChanged));

  m_lastRefIndex = -1;
}

void SequenceView::referenceChanged() {
  auto listView = get_children()[1];
  auto rows = listView->get_children();
  if(m_lastRefIndex >= 0) {
    rows[m_lastRefIndex]->remove_css_class("refimg");
  }
  m_lastRefIndex = static_cast<int>(m_refImageSelector->get_value());
  rows[m_lastRefIndex]->add_css_class("refimg");
}

Glib::RefPtr<Gio::ListStore<Image>>& SequenceView::model() {
  return m_model;
}

void SequenceView::connectState(const std::shared_ptr<UI::State>& state) {
  // Clear the model
  m_model->remove_all();

  // Populate with new sequence
  // TODO: Turn IO::Sequence into a valid ListModel
  for(int i = 0; i < state->m_sequence->getImageCount(); ++i) {
    m_model->append(state->m_sequence->image(i));
  }

  // Set reference image index
  auto adj = m_refImageSelector->get_adjustment();
  adj->set_lower(0);
  adj->set_step_increment(1);
  adj->set_upper(state->m_sequence->getImageCount() - 1);

  Glib::Binding::bind_property(state->m_sequence->propertyReferenceImageIndex(), m_refImageSelector->property_value(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  m_lastRefIndex = -1;
  referenceChanged();

  // Select first image
  get_model()->select_item(0, true);
}

void SequenceView::prevImage() {
  uint selected = getSelectedIndex();
  if(selected > 0) {
    get_model()->select_item(selected - 1, true);
  }
}

void SequenceView::nextImage() {
  uint selected = getSelectedIndex();
  if(selected < m_model->get_n_items() - 1) {
    get_model()->select_item(selected + 1, true);
  }
}

uint SequenceView::getSelectedIndex() {
  return dynamic_cast<Gtk::SingleSelection&>(*get_model()).get_selected();
}

Glib::RefPtr<Image> SequenceView::getSelected() {
  return m_model->get_item(getSelectedIndex());
}

Glib::RefPtr<Image> SequenceView::getImage(int index) {
  return m_model->get_item(index);
}

void SequenceView::labelColSetup(const ListItem& item) {
  auto label = Gtk::make_managed<Gtk::Label>();
  item->set_child(*label);
}

void SequenceView::checkboxColSetup(const ListItem& item) {
  auto checkbox = Gtk::make_managed<Gtk::CheckButton>();
  item->set_child(*checkbox);
}

void SequenceView::idColBind(const ListItem& item) {
  auto& itemRef = dynamic_cast<Image&>(*item->get_item());
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());

  Glib::Binding::bind_property(itemRef.propertySequenceIndex(), label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const int& index) -> std::optional<Glib::ustring> {
    return std::to_string(index);
  });
}

void SequenceView::selectColBind(const ListItem& item) {
  auto& itemRef = dynamic_cast<Image&>(*item->get_item());
  auto& check = dynamic_cast<Gtk::CheckButton&>(*item->get_child());
  Glib::Binding::bind_property(itemRef.propertyIncluded(), check.property_active(), Glib::Binding::Flags::BIDIRECTIONAL | Glib::Binding::Flags::SYNC_CREATE);
}

void SequenceView::xColBind(const ListItem& item) {
  auto& itemRef = dynamic_cast<Image&>(*item->get_item());
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());

  Glib::Binding::bind_property(itemRef.propertyXOffset(), label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const double& value) {
    return std::format("{:.2f}", value);
  });
}

void SequenceView::yColBind(const ListItem& item) {
  auto& itemRef = dynamic_cast<Image&>(*item->get_item());
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());

  Glib::Binding::bind_property(itemRef.propertyYOffset(), label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const double& value) {
    return std::format("{:.2f}", value);
  });
}

