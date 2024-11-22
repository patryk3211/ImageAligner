#include "cv/context.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/features2d.hpp>
#include <spdlog/spdlog.h>

#include <opencv2/opencv.hpp>

using namespace OpenCV;
using namespace IO;
using namespace Obj;

Context::Context(ImageProvider& provider, cv::Ptr<cv::Feature2D> detector, cv::Ptr<cv::DescriptorMatcher> matcher, float matchThreshold)
  : m_provider(provider)
  , m_detector(detector)
  , m_matcher(matcher)
  , m_matchThreshold(matchThreshold) {
  if(!m_detector) {
    m_detector = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_KAZE, 256, 4, 0.0002, 6, 6, cv::KAZE::DIFF_PM_G2);
    //cv::SIFT::create(0, 5, 0.06, 10, 1.3);
  }
  if(!m_matcher) {
    m_matcher = 
    //cv::BFMatcher::create(cv::NORM_HAMMING);
    cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
  }
}

void Context::addReference(const ImgPtr& image) {
  auto data = processImage(image);
  data->m_reference = true;
  m_referenceImages.push_back(data);
}

const std::vector<cv::KeyPoint> *Context::getKeypoints(const ImgPtr& image) {
  auto data = getData(image);
  return data ? &data->m_keypoints : nullptr;
}

void Context::findKeypoints(const ImgPtr& image, bool reprocess) {
  static_cast<void>(processImage(image, reprocess));
}

void Context::matchFeatures(const ImgPtr& image) {
  auto& ref = m_referenceImages.front();
  auto align = getData(image);
  align->m_matches.clear();

  std::vector<std::vector<cv::DMatch>> knnMatches;
  m_matcher->knnMatch(align->m_descriptors, ref->m_descriptors, knnMatches, 2);

  for(size_t i = 0; i < knnMatches.size(); i++) {
    spdlog::trace("KNN Match: i = {}, ratio = {}", i, knnMatches[i][0].distance / knnMatches[i][1].distance);

    if(knnMatches[i][0].distance < m_matchThreshold * knnMatches[i][1].distance) {
      auto& match = knnMatches[i][0];
      align->m_matches.push_back(match);
    }
  }

  spdlog::debug("Found {} matches between sequence images (reference index = {}) and (image index = {})",
                align->m_matches.size(), ref->m_image->getSequenceIndex(), align->m_image->getSequenceIndex());
  image->notifyRedraw();
}

void Context::alignFeatures(const ImgPtr& image) {
  auto& ref = m_referenceImages.front();
  auto align = getData(image);

  // Get points from matches
  std::vector<cv::Point2f> refPoints;
  std::vector<cv::Point2f> alignPoints;
  for(auto& match : align->m_matches) {
    auto refPt = ref->m_keypoints[match.trainIdx].pt;
    auto aliPt = align->m_keypoints[match.queryIdx].pt;

    refPoints.push_back(refPt);
    alignPoints.push_back(aliPt);
  }

  // Estimate matrix
  cv::Mat affine = cv::estimateAffine2D(alignPoints, refPoints, cv::noArray(), cv::RANSAC, 4.0);
  // cv::Mat homography = cv::findHomography(alignPoints, refPoints, cv::RHO, 2.0, mask);

  cv::Mat homography;
  homography.create(3, 3, CV_64F);
  homography.at<double>(0, 0) = affine.at<double>(0, 0);
  homography.at<double>(0, 1) = affine.at<double>(0, 1);
  homography.at<double>(0, 2) = affine.at<double>(0, 2);
  homography.at<double>(1, 0) = affine.at<double>(1, 0);
  homography.at<double>(1, 1) = affine.at<double>(1, 1);
  // To make this matrix work in Siril we need to transform the Y translation a bit
  homography.at<double>(1, 2) = -affine.at<double>(1, 2) * ref->m_width / ref->m_height;
  homography.at<double>(2, 0) = 0;
  homography.at<double>(2, 1) = 0;
  homography.at<double>(2, 2) = 1;

  spdlog::debug("Calculated homography matrix:");
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(0), homography.at<double>(1), homography.at<double>(2));
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(3), homography.at<double>(4), homography.at<double>(5));
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(6), homography.at<double>(7), homography.at<double>(8));

  image->getRegistration()->matrix().write(homography);
}

void Context::matchAndAlignFeatures(const ImgPtr& image) {
  auto& ref = m_referenceImages.front();
  auto align = getData(image);
  align->m_matches.clear();

  std::vector<std::vector<cv::DMatch>> knnMatches;
  m_matcher->knnMatch(align->m_descriptors, ref->m_descriptors, knnMatches, 2);

  std::vector<cv::Point2f> refPoints;
  std::vector<cv::Point2f> alignPoints;
  for(size_t i = 0; i < knnMatches.size(); i++) {
    spdlog::trace("KNN Match: i = {}, ratio = {}", i, knnMatches[i][0].distance / knnMatches[i][1].distance);

    if(knnMatches[i][0].distance < m_matchThreshold * knnMatches[i][1].distance) {
      auto& match = knnMatches[i][0];
      align->m_matches.push_back(match);

      auto refPt = ref->m_keypoints[match.trainIdx].pt;
      auto aliPt = align->m_keypoints[match.queryIdx].pt;

      refPoints.push_back(refPt);
      alignPoints.push_back(aliPt);
    }
  }

  spdlog::debug("Found {} matches between sequence images (reference index = {}) and (image index = {})",
                refPoints.size(), ref->m_image->getSequenceIndex(), align->m_image->getSequenceIndex());

  cv::Mat affine = cv::estimateAffine2D(alignPoints, refPoints, cv::noArray(), cv::RANSAC, 4.0);
  // cv::Mat homography = cv::findHomography(alignPoints, refPoints, cv::RHO, 2.0, mask);

  cv::Mat homography;
  homography.create(3, 3, CV_64F);
  homography.at<double>(0, 0) = affine.at<double>(0, 0);
  homography.at<double>(0, 1) = affine.at<double>(0, 1);
  homography.at<double>(0, 2) = affine.at<double>(0, 2);
  homography.at<double>(1, 0) = affine.at<double>(1, 0);
  homography.at<double>(1, 1) = affine.at<double>(1, 1);
  // To make this matrix work in Siril we need to transform the Y translation a bit
  homography.at<double>(1, 2) = -affine.at<double>(1, 2) * ref->m_width / ref->m_height;
  homography.at<double>(2, 0) = 0;
  homography.at<double>(2, 1) = 0;
  homography.at<double>(2, 2) = 1;

  spdlog::debug("Calculated homography matrix:");
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(0), homography.at<double>(1), homography.at<double>(2));
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(3), homography.at<double>(4), homography.at<double>(5));
  spdlog::debug("  {:9.3f} {:9.3f} {:9.3f}", homography.at<double>(6), homography.at<double>(7), homography.at<double>(8));

  image->getRegistration()->matrix().write(homography);
}

std::shared_ptr<Context::ImgData> Context::getData(const ImgPtr& image) {
  auto iter = m_imageData.find(image);
  if(iter != m_imageData.end()) {
    // Found in storage
    return iter->second;
  } else {
    // Not found
    return nullptr;
  }
}

std::shared_ptr<Context::ImgData> Context::processImage(const ImgPtr& image, bool force) {
  auto iter = m_imageData.find(image);
  if(iter != m_imageData.end()) {
    // Found in storage
    if(force) {
      // Remove and process again
      m_imageData.erase(iter);
    } else {
      // Return available data
      return iter->second;
    }
  }

  // Process image
  cv::Mat raw;
  m_provider.getImageMatrix(image->getFileIndex()).convertTo(raw, CV_32F);
  // TODO: Use levels from image object
  raw = ((cv::min(cv::max(raw, 990.0), 3900.0) - 990.0) / (3900.0 - 990.0));

  cv::Mat mat;
  raw.convertTo(mat, CV_8U, 255);

  std::shared_ptr<ImgData> data(new ImgData(image, raw.cols, raw.rows));

  data->m_keypoints.clear();
  m_detector->detectAndCompute(mat, cv::noArray(), data->m_keypoints, data->m_descriptors);

  spdlog::debug("Found {} keypoints in image (sequence index = {})",
                data->m_keypoints.size(), image->getSequenceIndex());

  // Insert into storage
  m_imageData.insert({ image, data });
  image->notifyRedraw();
  return data;
}

Context::ImgData::ImgData(const ImgPtr& image, int width, int height)
  : m_image(image)
  , m_width(width)
  , m_height(height)
  , m_reference(false)
  , m_keypoints()
  , m_descriptors()
  , m_matches() {
  // Associate key point array with the image object
  m_image->set_data("keypoints", &m_keypoints);
  m_image->set_data("matches", &m_matches);
}

Context::ImgData::~ImgData() {
  // Make sure there are no dangling pointers.
  m_image->remove_data("keypoints");
  m_image->remove_data("matches");
}

