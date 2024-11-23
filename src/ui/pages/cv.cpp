#include "ui/pages/cv.hpp"
#include "glibmm/refptr.h"
#include "ui/state.hpp"
#include "ui/widgets/util.hpp"
#include "ui/window.hpp"

#include <spdlog/spdlog.h>
#include <opencv2/features2d.hpp>

using namespace UI::Pages;

CV::CV(const Glib::RefPtr<Gtk::Builder>& builder, Window& window)
  : Page("cv")
  , m_keypointModel(Gio::ListStore<KeypointObject>::create())
  , m_matchModel(Gio::ListStore<MatchObject>::create()) {
  m_actionGroup = Gio::SimpleActionGroup::create();

  m_threshold = builder->get_widget<Gtk::SpinButton>("threshold");
  m_descriptorSize = builder->get_widget<Gtk::SpinButton>("descriptor_size");
  m_descriptorChannels = builder->get_widget<Gtk::SpinButton>("descriptor_channels");
  m_octaves = builder->get_widget<Gtk::SpinButton>("octaves");
  m_octaveLayers = builder->get_widget<Gtk::SpinButton>("octave_layers");
  m_onlySelected = builder->get_widget<Gtk::CheckButton>("only_selected");
  m_keypointView = builder->get_widget<Gtk::ColumnView>("keypoint_list");
  m_keypointToggle = builder->get_widget<Gtk::ToggleButton>("keypoint_toggle");
  m_matchToggle = builder->get_widget<Gtk::ToggleButton>("match_toggle");
  m_matchView = builder->get_widget<Gtk::ColumnView>("match_list");
  m_matchThreshold = builder->get_widget<Gtk::SpinButton>("match_threshold");

  m_mainView = window.m_mainView;
  m_sequenceList = window.m_sequenceView;
  m_sequenceList->get_model()->signal_selection_changed().connect(sigc::mem_fun(*this, &CV::selectionChanged));

  m_keypointView->set_model(Gtk::SingleSelection::create(m_keypointModel));
  addIntColumn(m_keypointView, "Index", "index");
  addFloatColumn(m_keypointView, "Position X", "x");
  addFloatColumn(m_keypointView, "Position Y", "y");
  addFloatColumn(m_keypointView, "Scale", "scale");
  addFloatColumn(m_keypointView, "Angle", "angle");

  m_matchView->set_model(Gtk::SingleSelection::create(m_matchModel));
  addIntColumn(m_matchView, "Index", "index");
  addFloatColumn(m_matchView, "Ref X", "x1");
  addFloatColumn(m_matchView, "Ref Y", "y1");
  addFloatColumn(m_matchView, "Image X", "x2");
  addFloatColumn(m_matchView, "Image Y", "y2");

  m_keypointToggle->signal_toggled().connect(sigc::mem_fun(*this, &CV::toggleKeypoint));
  m_matchToggle->signal_toggled().connect(sigc::mem_fun(*this, &CV::toggleMatch));

  // Create actions
  m_actionKeypoints = Gio::SimpleAction::create("keypoints");
  m_actionKeypoints->signal_activate().connect(sigc::mem_fun(*this, &CV::findKeypoints));
  m_actionKeypoints->set_enabled(false);
  m_actionGroup->add_action(m_actionKeypoints);

  m_actionFeatures = Gio::SimpleAction::create("match-features");
  m_actionFeatures->signal_activate().connect(sigc::mem_fun(*this, &CV::matchFeatures));
  m_actionFeatures->set_enabled(false);
  m_actionGroup->add_action(m_actionFeatures);

  m_actionAlign = Gio::SimpleAction::create("align");
  m_actionAlign->signal_activate().connect(sigc::mem_fun(*this, &CV::alignFeatures));
  m_actionAlign->set_enabled(false);
  m_actionGroup->add_action(m_actionAlign);

  // Bind parameter changes to context invalidation
  auto slot = sigc::mem_fun(*this, &CV::dropContext);
  m_threshold->property_value().signal_changed().connect(slot);
  m_descriptorSize->property_value().signal_changed().connect(slot);
  m_descriptorChannels->property_value().signal_changed().connect(slot);
  m_octaves->property_value().signal_changed().connect(slot);
  m_octaveLayers->property_value().signal_changed().connect(slot);
}

void CV::connectState(const std::shared_ptr<State>& state) {
  Page::connectState(state);

  m_actionKeypoints->set_enabled(state != nullptr);
  m_actionFeatures->set_enabled(false);
  m_actionAlign->set_enabled(false);
}

void CV::selectionChanged(uint pos, uint nitems) {
  // Remove old keypoints
  m_keypointModel->remove_all();
  m_matchModel->remove_all();
  if(!m_cvContext)
    return;

  // Put keypoints into the list
  auto img = m_sequenceList->getSelected();
  auto keypoints = m_cvContext->getKeypoints(img);
  if(!keypoints)
    return;
  int index = 0;
  for(auto& kp : *keypoints) {
    m_keypointModel->append(KeypointObject::create(index++, kp.pt.x, kp.pt.y, kp.size, kp.angle));
  }

  // Get reference keypoints
  auto refImg = m_state->m_sequence->image(m_state->m_sequence->getReferenceImageIndex());
  auto refKeypoints = m_cvContext->getKeypoints(refImg);
  if(!refKeypoints)
    return;

  // Put matches into the list
  auto matches = m_cvContext->getMatches(img);
  if(!matches)
    return;
  index = 0;
  for(auto& m : *matches) {
    auto refPt = (*refKeypoints)[m.trainIdx].pt;
    auto imgPt = (*keypoints)[m.queryIdx].pt;
    m_matchModel->append(MatchObject::create(index++, refPt.x, refPt.y, imgPt.x, imgPt.y));
  }
}

Glib::RefPtr<Gio::ActionGroup> CV::actionGroup() {
  return m_actionGroup;
}

std::shared_ptr<OpenCV::Context> CV::createCVContext(IO::ImageProvider& provider) {
  auto detector = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_KAZE, 
                                    static_cast<int>(m_descriptorSize->get_value()),
                                    static_cast<int>(m_descriptorChannels->get_value()),
                                    static_cast<float>(m_threshold->get_value()),
                                    static_cast<int>(m_octaves->get_value()),
                                    static_cast<int>(m_octaveLayers->get_value()),
                                    cv::KAZE::DIFF_PM_G2);
  auto matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
  m_actionFeatures->set_enabled(true);
  m_actionAlign->set_enabled(true);

  return std::make_shared<OpenCV::Context>(provider, detector, matcher);
}

void CV::dropContext() {
  if(m_cvContext) {
    spdlog::trace("OpenCV context dropped with use count = {}", m_cvContext.use_count());
    m_cvContext = nullptr;
  }
}

std::list<Glib::RefPtr<Obj::Image>> CV::getImageList() {
  std::list<Glib::RefPtr<Obj::Image>> processImages;
  if(m_onlySelected->get_active()) {
    for(uint i = 0; i < m_state->m_sequence->getImageCount(); ++i) {
      auto img = m_state->m_sequence->image(i);
      if(!img->getIncluded() || img->isReference())
        continue;
      processImages.push_back(img);
    }
  } else {
    // All images get processed
    for(uint i = 0; i < m_state->m_sequence->getImageCount(); ++i) {
      auto img = m_state->m_sequence->image(i);
      if(img->isReference())
        continue;
      processImages.push_back(img);
    }
  }

  return processImages;
}

void CV::findKeypoints(const Glib::VariantBase& variant) {
  if(!m_state)
    return;
  if(!m_cvContext) {
    m_cvContext = createCVContext(m_state->m_imageFile);
    spdlog::debug("Created new OpenCV context");
  }

  // Process reference image first
  spdlog::info("Finding keypoints in reference image...");
  auto refImg = m_state->m_sequence->image(m_state->m_sequence->getReferenceImageIndex());
  m_cvContext->addReference(refImg);

  // Process other images
  std::list<Glib::RefPtr<Obj::Image>> processImages = getImageList();
  spdlog::info("Finding keypoints in {} images", processImages.size());
  for(auto& img : processImages) {
    m_cvContext->findKeypoints(img);
  }

  // TODO: Add a suggestion if reference image has less keypoints than any 
  // image processed. An image with more keypoints will work better for alignment.

  spdlog::info("Finished keypoint detection!");
  selectionChanged(0, 0);
}

void CV::matchFeatures(const Glib::VariantBase& variant) {
  if(!m_state || !m_cvContext)
    return;

  m_cvContext->setMatchThreshold(m_matchThreshold->get_value());

  std::list<Glib::RefPtr<Obj::Image>> processImages = getImageList();
  spdlog::info("Matching features in {} images", processImages.size());
  for(auto& img : processImages) {
    m_cvContext->matchFeatures(img);
  }

  spdlog::info("Finished feature matching!");
  selectionChanged(0, 0);
}

void CV::toggleKeypoint() {
  if(m_keypointToggle->get_active()) {
    m_matchToggle->set_active(false);
    m_mainView->setRenderMode(MainView::RenderMode::KEYPOINTS);
  } else {
    m_mainView->setRenderMode(MainView::RenderMode::DEFAULT);
  }
}

void CV::toggleMatch() {
  if(m_matchToggle->get_active()) {
    m_keypointToggle->set_active(false);
    m_mainView->setRenderMode(MainView::RenderMode::MATCHES);
  } else {
    m_mainView->setRenderMode(MainView::RenderMode::DEFAULT);
  }
}

void CV::alignFeatures(const Glib::VariantBase& variant) {
  if(!m_state || !m_cvContext)
    return;

  std::list<Glib::RefPtr<Obj::Image>> processImages = getImageList();
  spdlog::info("Aligning features in {} images", processImages.size());
  for(auto& img : processImages) {
    m_cvContext->alignFeatures(img);
  }

  spdlog::info("Finished feature alignment!");
}

KeypointObject::KeypointObject(int index, float x, float y, float scale, float angle)
  : ObjectBase("KeypointObject")
  , m_index(*this, "index", index)
  , m_x(*this, "x", x)
  , m_y(*this, "y", y)
  , m_scale(*this, "scale", scale)
  , m_angle(*this, "angle", angle) {

}

Glib::PropertyProxy_ReadOnly<int> KeypointObject::propertyIndex() {
  return m_index.get_proxy();
}

Glib::PropertyProxy<float> KeypointObject::propertyX() {
  return m_x.get_proxy();
}

Glib::PropertyProxy<float> KeypointObject::propertyY() {
  return m_y.get_proxy();
}

Glib::PropertyProxy<float> KeypointObject::propertyScale() {
  return m_scale.get_proxy();
}

Glib::PropertyProxy<float> KeypointObject::propertyAngle() {
  return m_angle.get_proxy();
}

Glib::RefPtr<KeypointObject> KeypointObject::create(int index, float x, float y, float scale, float angle) {
  return Glib::make_refptr_for_instance(new KeypointObject(index, x, y, scale, angle));
}

MatchObject::MatchObject(int index, float x1, float y1, float x2, float y2)
  : ObjectBase("MatchObject")
  , m_index(*this, "index", index)
  , m_x1(*this, "x1", x1)
  , m_y1(*this, "y1", y1)
  , m_x2(*this, "x2", x2)
  , m_y2(*this, "y2", y2) {
}

Glib::PropertyProxy_ReadOnly<int> MatchObject::propertyIndex() {
  return m_index.get_proxy();
}

Glib::PropertyProxy<float> MatchObject::propertyX1() {
  return m_x1.get_proxy();
}

Glib::PropertyProxy<float> MatchObject::propertyY1() {
  return m_y1.get_proxy();
}

Glib::PropertyProxy<float> MatchObject::propertyX2() {
  return m_x2.get_proxy();
}

Glib::PropertyProxy<float> MatchObject::propertyY2() {
  return m_y2.get_proxy();
}

Glib::RefPtr<MatchObject> MatchObject::create(int index, float x1, float y1, float x2, float y2) {
  return Glib::make_refptr_for_instance(new MatchObject(index, x1, y1, x2, y2));
}

