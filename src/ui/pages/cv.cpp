#include "ui/pages/cv.hpp"
#include "ui/state.hpp"
#include "ui/window.hpp"

#include <spdlog/spdlog.h>
#include <opencv2/features2d.hpp>

using namespace UI::Pages;

CV::CV(const Glib::RefPtr<Gtk::Builder>& builder, Window& window)
  : Page("cv") {
  m_actionGroup = Gio::SimpleActionGroup::create();

  m_threshold = builder->get_widget<Gtk::SpinButton>("threshold");
  m_descriptorSize = builder->get_widget<Gtk::SpinButton>("descriptor_size");
  m_descriptorChannels = builder->get_widget<Gtk::SpinButton>("descriptor_channels");
  m_octaves = builder->get_widget<Gtk::SpinButton>("octaves");
  m_octaveLayers = builder->get_widget<Gtk::SpinButton>("octave_layers");
  m_onlySelected = builder->get_widget<Gtk::CheckButton>("only_selected");

  assert(m_threshold != nullptr);
  assert(m_descriptorSize != nullptr);
  assert(m_descriptorChannels != nullptr);
  assert(m_octaves != nullptr);
  assert(m_octaveLayers != nullptr);

  m_sequenceList = window.m_sequenceView;

  // Create actions
  auto actionKeypoints = Gio::SimpleAction::create("keypoints");
  actionKeypoints->signal_activate().connect(sigc::mem_fun(*this, &CV::findKeypoints));
  m_actionGroup->add_action(actionKeypoints);
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

  return std::make_shared<OpenCV::Context>(provider, detector, matcher);
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

  spdlog::info("Finding keypoints in {} images", processImages.size());
  for(auto& img : processImages) {
    m_cvContext->findKeypoints(img);
  }

  spdlog::info("Finished keypoint detection!");
}

