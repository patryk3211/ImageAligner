#include "ui/widgets/sequence_list.hpp"
#include "ui/state.hpp"

#include "sigc++/functors/ptr_fun.h"

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
}

Glib::RefPtr<Gio::ListStore<SequenceListItem>>& SequenceView::model() {
  return m_model;
}

void SequenceView::populateModel(const UI::State& state) {
  // Clear the model
  m_model->remove_all();

  // Populate with new sequence
  for(int i = 0; i < state.m_sequence->imageCount(); ++i) {
    auto& img = state.m_sequence->image(i);
    m_model->append(SequenceListItem::create(img.m_fileIndex, img.m_included));
  }
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

