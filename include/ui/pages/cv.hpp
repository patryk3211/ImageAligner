#pragma once

#include <gtkmm.h>

#include "ui/pages/page.hpp"
#include "cv/context.hpp"
#include "ui/widgets/sequence_list.hpp"
#include "ui/widgets/main_view.hpp"

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

class MatchObject : public Glib::Object {
  Glib::Property_ReadOnly<int> m_index;
  Glib::Property<float> m_x1;
  Glib::Property<float> m_y1;
  Glib::Property<float> m_x2;
  Glib::Property<float> m_y2;

public:
  MatchObject(int index, float x1, float y1, float x2, float y2);
  virtual ~MatchObject() = default;

  Glib::PropertyProxy_ReadOnly<int> propertyIndex();
  Glib::PropertyProxy<float> propertyX1();
  Glib::PropertyProxy<float> propertyY1();
  Glib::PropertyProxy<float> propertyX2();
  Glib::PropertyProxy<float> propertyY2();

  static Glib::RefPtr<MatchObject> create(int index, float x1, float y1, float x2, float y2);
};

class CV : public Page {
  Glib::RefPtr<Gio::SimpleActionGroup> m_actionGroup;

  SequenceView *m_sequenceList;
  MainView *m_mainView;

  Gtk::SpinButton *m_threshold;
  Gtk::SpinButton *m_descriptorSize;
  Gtk::SpinButton *m_descriptorChannels;
  Gtk::SpinButton *m_octaves;
  Gtk::SpinButton *m_octaveLayers;
  Gtk::SpinButton *m_matchThreshold;

  Gtk::CheckButton *m_onlySelected;

  Gtk::ToggleButton *m_keypointToggle;
  Gtk::ToggleButton *m_matchToggle;

  Gtk::ColumnView *m_keypointView;
  Glib::RefPtr<Gio::ListStore<KeypointObject>> m_keypointModel;

  Gtk::ColumnView *m_matchView;
  Glib::RefPtr<Gio::ListStore<MatchObject>> m_matchModel;

  std::shared_ptr<OpenCV::Context> m_cvContext;

  Glib::RefPtr<Gio::SimpleAction> m_actionKeypoints;
  Glib::RefPtr<Gio::SimpleAction> m_actionFeatures;
  Glib::RefPtr<Gio::SimpleAction> m_actionAlign;

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
  void alignFeatures(const Glib::VariantBase& variant);

  void toggleKeypoint();
  void toggleMatch();
};

}

