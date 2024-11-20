#pragma once

#include "objects/image.hpp"
#include "io/provider.hpp"

#include <opencv2/features2d.hpp>

namespace OpenCV {

class Context {
  using ImgPtr = Glib::RefPtr<Obj::Image>;

  struct ImgData {
    ImgPtr m_image;
    int m_width;
    int m_height;
    bool m_reference;

    std::vector<cv::KeyPoint> m_keypoints;
    cv::Mat m_descriptors;
  };

  cv::Ptr<cv::Feature2D> m_detector;
  cv::Ptr<cv::DescriptorMatcher> m_matcher;

  IO::ImageProvider& m_provider;

  std::list<std::shared_ptr<ImgData>> m_referenceImages;

  std::unordered_map<ImgPtr, std::shared_ptr<ImgData>> m_imageData;

public:
  Context(IO::ImageProvider& provider, cv::Ptr<cv::Feature2D> detector = nullptr, cv::Ptr<cv::DescriptorMatcher> matcher = nullptr);
  ~Context() = default;

  void addReference(const ImgPtr& image);

  std::optional<std::reference_wrapper<const std::vector<cv::KeyPoint>>> getKeypoints(const ImgPtr& image);
  void findKeypoints(const ImgPtr& image, bool reprocess = false);

  void matchFeatures(const ImgPtr& image);

private:
  std::shared_ptr<ImgData> processImage(const ImgPtr& image, bool force = false);
  std::shared_ptr<ImgData> getData(const ImgPtr& image);
};

}

