#include "ui/widgets/sequence_list.hpp"
#include "gtkmm/singleselection.h"
#include "ui/state.hpp"

#include <string>

using namespace UI;

SequenceListItem::SequenceListItem(int id, bool selected)
  : ObjectBase("SequenceListItem")
  , m_id(*this, "id", id)
  , m_selected(*this, "selected", selected) { }

Glib::RefPtr<SequenceListItem> SequenceListItem::create(int id, bool selected) {
  return Glib::make_refptr_for_instance<SequenceListItem>(new SequenceListItem(id, selected));
}

SequenceView::SequenceView() : Glib::ObjectBase("SequenceView") {
  /* Dummy constructor for type registration */
}

SequenceView::SequenceView(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder>& builder)
  : Glib::ObjectBase("SequenceView")
  , Gtk::ColumnView(cobject)
  , m_model(Gio::ListStore<SequenceListItem>::create())
  , m_idColFactory(Gtk::SignalListItemFactory::create())
  , m_selectColFactory(Gtk::SignalListItemFactory::create()) {
  // Create column factories
  m_idColFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::labelColSetup));
  m_idColFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::idColBind));
  m_selectColFactory->signal_setup().connect(sigc::ptr_fun(&SequenceView::checkboxColSetup));
  m_selectColFactory->signal_bind().connect(sigc::ptr_fun(&SequenceView::selectColBind));

  // Append columns
  append_column(Gtk::ColumnViewColumn::create("Id", m_idColFactory));
  append_column(Gtk::ColumnViewColumn::create("Selected", m_selectColFactory));
  
  // Set selection model
  set_model(Gtk::SingleSelection::create(m_model));

  m_refImageSelector = builder->get_widget<Gtk::SpinButton>("ref_image_spin_btn");
}

Glib::RefPtr<Gio::ListStore<SequenceListItem>>& SequenceView::model() {
  return m_model;
}

void SequenceView::connectState(const std::shared_ptr<UI::State>& state) {
  // Clear the model
  m_model->remove_all();

  // Populate with new sequence
  for(int i = 0; i < state->m_sequence->imageCount(); ++i) {
    auto& img = state->m_sequence->image(i);
    m_model->append(SequenceListItem::create(img.m_fileIndex, img.m_included));
  }

  // Set reference image index
  auto adj = m_refImageSelector->get_adjustment();
  adj->set_lower(0);
  adj->set_step_increment(1);
  adj->set_upper(state->m_sequence->imageCount() - 1);
  adj->set_value(state->m_sequence->referenceImage());

  auto listView = get_children()[1];
  auto rows = listView->get_children();
  rows[state->m_sequence->referenceImage()]->add_css_class("refimg");

  // Select first image
  get_model()->select_item(0, true);
}

void SequenceView::prevImage() {
  uint selected = getSelected();
  if(selected > 0) {
    get_model()->select_item(selected - 1, true);
  }
}

void SequenceView::nextImage() {
  uint selected = getSelected();
  if(selected < m_model->get_n_items() - 1) {
    get_model()->select_item(selected + 1, true);
  }
}

uint SequenceView::getSelected() {
  return dynamic_cast<Gtk::SingleSelection&>(*get_model()).get_selected();
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
  auto& itemRef = dynamic_cast<SequenceListItem&>(*item->get_item());
  std::string text = std::to_string(itemRef.m_id.get_value());
  dynamic_cast<Gtk::Label&>(*item->get_child()).set_text(text);
}

void SequenceView::selectColBind(const ListItem& item) {
  auto& itemRef = dynamic_cast<SequenceListItem&>(*item->get_item());
  dynamic_cast<Gtk::CheckButton&>(*item->get_child()).set_active(itemRef.m_selected.get_value());
}

