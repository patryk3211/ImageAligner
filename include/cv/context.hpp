#pragma once

#include "objects/image.hpp"
#include "io/provider.hpp"

#include <opencv2/features2d.hpp>

namespace OpenCV {

class Context {
  using ImgPtr = Glib::RefPtr<Obj::Image>;

  struct ImgData {
    ImgPtr m_image;

    std::vector<cv::KeyPoint> m_keypoints;
    cv::Mat m_descriptors;
  };

  cv::Ptr<cv::Feature2D> m_detector;
  cv::Ptr<cv::DescriptorMatcher> m_matcher;

  IO::ImageProvider& m_provider;

  std::list<ImgData> m_referenceImages;

public:
  Context(IO::ImageProvider& provider);
  ~Context() = default;

  void addReference(const ImgPtr& image);

  void matchFeatures(const ImgPtr& image);

private:
  ImgData detect(const ImgPtr& image);
};

}

