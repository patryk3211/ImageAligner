#include "ui/pages/cv.hpp"
#include "ui/state.hpp"
#include "ui/widgets/util.hpp"
#include "ui/window.hpp"

#include <spdlog/spdlog.h>
#include <opencv2/features2d.hpp>

using namespace UI::Pages;

CV::CV(const Glib::RefPtr<Gtk::Builder>& builder, Window& window)
  : Page("cv")
  , m_keypointModel(Gio::ListStore<KeypointObject>::create()) {
  m_actionGroup = Gio::SimpleActionGroup::create();

  m_threshold = builder->get_widget<Gtk::SpinButton>("threshold");
  m_descriptorSize = builder->get_widget<Gtk::SpinButton>("descriptor_size");
  m_descriptorChannels = builder->get_widget<Gtk::SpinButton>("descriptor_channels");
  m_octaves = builder->get_widget<Gtk::SpinButton>("octaves");
  m_octaveLayers = builder->get_widget<Gtk::SpinButton>("octave_layers");
  m_onlySelected = builder->get_widget<Gtk::CheckButton>("only_selected");
  m_keypointView = builder->get_widget<Gtk::ColumnView>("keypoint_list");

  assert(m_threshold != nullptr);
  assert(m_descriptorSize != nullptr);
  assert(m_descriptorChannels != nullptr);
  assert(m_octaves != nullptr);
  assert(m_octaveLayers != nullptr);

  m_sequenceList = window.m_sequenceView;
  m_sequenceList->get_model()->signal_selection_changed().connect(sigc::mem_fun(*this, &CV::selectionChanged));

  m_keypointView->set_model(Gtk::SingleSelection::create(m_keypointModel));
  addIntColumn(m_keypointView, "Index", "index");
  addFloatColumn(m_keypointView, "Position X", "x");
  addFloatColumn(m_keypointView, "Position Y", "y");
  addFloatColumn(m_keypointView, "Scale", "scale");
  addFloatColumn(m_keypointView, "Angle", "angle");

  // Create actions
  m_actionKeypoints = Gio::SimpleAction::create("keypoints");
  m_actionKeypoints->signal_activate().connect(sigc::mem_fun(*this, &CV::findKeypoints));
  m_actionKeypoints->set_enabled(false);
  m_actionGroup->add_action(m_actionKeypoints);

  m_actionFeatures = Gio::SimpleAction::create("match-features");
  m_actionFeatures->signal_activate().connect(sigc::mem_fun(*this, &CV::matchFeatures));
  m_actionFeatures->set_enabled(false);
  m_actionGroup->add_action(m_actionFeatures);

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
}

void CV::selectionChanged(uint pos, uint nitems) {
  // Remove old keypoints
  m_keypointModel->remove_all();
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

  std::list<Glib::RefPtr<Obj::Image>> processImages = getImageList();
  spdlog::info("Matching features in {} images", processImages.size());
  for(auto& img : processImages) {
    m_cvContext->matchFeatures(img);
  }

  spdlog::info("Finished feature matching!");
}

void CV::setupLabel(const Glib::RefPtr<Gtk::ListItem>& item) {
  auto label = Gtk::make_managed<Gtk::Label>();
  item->set_child(*label);
}

void CV::bindLabelFloat(const Glib::RefPtr<Gtk::ListItem>& item, const char *property) {
  // auto& itemRef = dynamic_cast<KeypointObject&>(*item->get_item());
  auto& label = dynamic_cast<Gtk::Label&>(*item->get_child());
  Glib::PropertyProxy<float> proxy(item->get_item().get(), property);
  Glib::Binding::bind_property(proxy, label.property_label(), Glib::Binding::Flags::SYNC_CREATE, [](const float& value) {
    return std::format("{:.2f}", value);
  });
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

