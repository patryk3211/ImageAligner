#include "cv/context.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/features2d.hpp>
#include <spdlog/spdlog.h>

#include <opencv2/opencv.hpp>

using namespace OpenCV;
using namespace IO;
using namespace Obj;

Context::Context(ImageProvider& provider)
  : m_provider(provider) {
  m_detector = cv::SIFT::create(0, 10);
  m_matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
}

void Context::addReference(const ImgPtr& image) {
  m_referenceImages.push_back(detect(image));
}

void Context::matchFeatures(const ImgPtr& image) {
  auto& ref = m_referenceImages.front();

  cv::Mat refMat;
  m_provider.getImageMatrix(ref.m_image->getFileIndex()).convertTo(refMat, CV_8UC1, 20.0 / 256.0);
  cv::Mat imgMat;
  m_provider.getImageMatrix(image->getFileIndex()).convertTo(imgMat, CV_8UC1, 20.0 / 256.0);

  auto data = detect(image);

  std::vector<std::vector<cv::DMatch>> knnMatches;
  m_matcher->knnMatch(ref.m_descriptors, data.m_descriptors, knnMatches, 2);

  std::vector<cv::DMatch> goodMatches;
  // m_matcher->match(ref.m_descriptors, data.m_descriptors, goodMatches);
  const float ratio_thresh = 0.7f;
  for(size_t i = 0; i < knnMatches.size(); i++) {
    if(knnMatches[i][0].distance < ratio_thresh * knnMatches[i][1].distance) {
      goodMatches.push_back(knnMatches[i][0]);
    }
  }

  std::vector<cv::Point2f> refPoints;
  std::vector<cv::Point2f> alignPoints;
  for(size_t i = 0; i < goodMatches.size(); ++i) {
    auto& match = goodMatches[i];

    auto refPt = ref.m_keypoints[match.queryIdx].pt;
    auto aliPt = data.m_keypoints[match.trainIdx].pt;
    refPoints.push_back(refPt);
    alignPoints.push_back(aliPt);

    spdlog::info("Match ref = ({}, {}), ali = ({}, {})", refPt.x, refPt.y, aliPt.x, aliPt.y);
  }

  std::vector<char> mask;
  cv::Mat homography = cv::findHomography(alignPoints, refPoints, cv::RANSAC, 5.0, mask);
  // homography.at<float>(2) *= -1;
  // homography.at<float>(1) *= -1;
  // homography.at<float>(4) *= -1;
  // homography.at<float>(7) *= -1;
  // homography.at<float>(5) *= -1;

  // cv::Matx<double, 3, 3> scale = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
  // homography *= scale;
  image->getRegistration()->matrix().write(homography);

  // spdlog::info("mask {}", mask.size());

  spdlog::info("{} {} {}", homography.at<float>(0), homography.at<float>(1), homography.at<float>(2));
  spdlog::info("{} {} {}", homography.at<float>(3), homography.at<float>(4), homography.at<float>(5));
  spdlog::info("{} {} {}", homography.at<float>(6), homography.at<float>(7), homography.at<float>(8));

  // cv::Mat imgMatches;
  // cv::drawMatches(refMat, ref.m_keypoints, imgMat, data.m_keypoints, goodMatches, imgMatches, cv::Scalar::all(-1), cv::Scalar::all(-1), mask, cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
  // cv::imwrite("img.png", imgMatches);
}

Context::ImgData Context::detect(const ImgPtr& image) {
  cv::Mat mat;
  m_provider.getImageMatrix(image->getFileIndex()).convertTo(mat, CV_8U, 20.0 / 256.0);

  ImgData data = { image };
  m_detector->detectAndCompute(mat, cv::noArray(), data.m_keypoints, data.m_descriptors);

  spdlog::info("Found {} keypoints in image (file index = {})", data.m_keypoints.size(), image->getFileIndex());

  return data;
}

