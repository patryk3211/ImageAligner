#pragma once

#include <gtkmm.h>

#include "ui/pages/page.hpp"
#include "cv/context.hpp"
#include "ui/widgets/sequence_list.hpp"

namespace UI::Pages {

class KeypointObject : public Glib::Object {
  Glib::Property_ReadOnly<int> m_index;
  Glib::Property<float> m_x;
  Glib::Property<float> m_y;
  Glib::Property<float> m_scale;
  Glib::Property<float> m_angle;

public:
  KeypointObject(int index, float x, float y, float scale, float angle);
  virtual ~KeypointObject() = default;

  Glib::PropertyProxy_ReadOnly<int> propertyIndex();
  Glib::PropertyProxy<float> propertyX();
  Glib::PropertyProxy<float> propertyY();
  Glib::PropertyProxy<float> propertyScale();
  Glib::PropertyProxy<float> propertyAngle();

  static Glib::RefPtr<KeypointObject> create(int index, float x, float y, float scale, float angle);
};

class CV : public Page {
  Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;

  SequenceView *m_sequenceList;

  Gtk::SpinButton *m_threshold;
  Gtk::SpinButton *m_descriptorSize;
  Gtk::SpinButton *m_descriptorChannels;
  Gtk::SpinButton *m_octaves;
  Gtk::SpinButton *m_octaveLayers;

  Gtk::CheckButton *m_onlySelected;

  Gtk::ColumnView *m_keypointView;
  Glib::RefPtr<Gio::ListStore<KeypointObject>> m_keypointModel;

  std::shared_ptr<OpenCV::Context> m_cvContext;

  Glib::RefPtr<Gio::SimpleAction> m_actionKeypoints;
  Glib::RefPtr<Gio::SimpleAction> m_actionFeatures;

private:
  CV(const Glib::RefPtr<Gtk::Builder>& builder, Window& window);

public:
  std::shared_ptr<OpenCV::Context> createCVContext(IO::ImageProvider& provider);

  virtual ~CV() = default;
  
  virtual void connectState(const std::shared_ptr<State>& state) override;
  virtual Glib::RefPtr<Gio::ActionGroup> actionGroup() override;

  static std::unique_ptr<CV> load(Gtk::Stack *stack, Window& window) { return Page::create<CV>(stack, window, "ui/cv.blp.ui", "Auto-alignment"); }
  friend class Page;

private:
  std::list<Glib::RefPtr<Obj::Image>> getImageList();
  void dropContext();

  void selectionChanged(uint pos, uint nitems);

  void findKeypoints(const Glib::VariantBase& variant);
  void matchFeatures(const Glib::VariantBase& variant);

  static void setupLabel(const Glib::RefPtr<Gtk::ListItem>& item);
  static void bindLabelFloat(const Glib::RefPtr<Gtk::ListItem>& item, const char *property);
};

}

