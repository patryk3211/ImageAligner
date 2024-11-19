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

Context::Context(ImageProvider& provider)
  : m_provider(provider) {
  m_detector = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_KAZE, 256, 4, 0.0005, 6, 6, cv::KAZE::DIFF_PM_G2);
  //cv::SIFT::create(0, 5, 0.06, 10, 1.3);
  m_matcher = 
  //cv::BFMatcher::create(cv::NORM_HAMMING);
  cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
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

  auto& trainMat = refMat;
  auto& queryMat = imgMat;
  auto& trainImg = ref;
  auto& queryImg = data;

  std::vector<std::vector<cv::DMatch>> knnMatches;
  m_matcher->knnMatch(queryImg.m_descriptors, trainImg.m_descriptors, knnMatches, 2);

  std::vector<cv::DMatch> goodMatches;
  // m_matcher->match(ref.m_descriptors, data.m_descriptors, goodMatches);
  const float ratio_thresh = 0.7f;
  for(size_t i = 0; i < knnMatches.size(); i++) {
    spdlog::info("i = {}, ratio = {}", i, knnMatches[i][0].distance / knnMatches[i][1].distance);
    if(knnMatches[i][0].distance < ratio_thresh * knnMatches[i][1].distance) {
      goodMatches.push_back(knnMatches[i][0]);
    }
  }

  std::vector<cv::Point2f> refPoints;
  std::vector<cv::Point2f> alignPoints;
  for(size_t i = 0; i < goodMatches.size(); ++i) {
    auto& match = goodMatches[i];

    auto refPt = trainImg.m_keypoints[match.trainIdx].pt;
    auto aliPt = queryImg.m_keypoints[match.queryIdx].pt;

    refPoints.push_back(refPt);
    alignPoints.push_back(aliPt);

    spdlog::info("Match ref = ({}, {}), ali = ({}, {})", refPt.x, refPt.y, aliPt.x, aliPt.y);
  }

  std::vector<char> mask;
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
  homography.at<double>(1, 2) = -affine.at<double>(1, 2) * ref.m_width / ref.m_height;
  homography.at<double>(2, 0) = 0;
  homography.at<double>(2, 1) = 0;
  homography.at<double>(2, 2) = 1;

  for(int i = 0; i < mask.size(); ++i) {
    spdlog::info("Mask i = {}, value: {}", i, (int) mask[i]);
  }

  image->getRegistration()->matrix().write(homography);

  // spdlog::info("mask {}", mask.size());

  spdlog::debug("{:9.3f} {:9.3f} {:9.3f}", homography.at<double>(0), homography.at<double>(1), homography.at<double>(2));
  spdlog::debug("{:9.3f} {:9.3f} {:9.3f}", homography.at<double>(3), homography.at<double>(4), homography.at<double>(5));
  spdlog::debug("{:9.3f} {:9.3f} {:9.3f}", homography.at<double>(6), homography.at<double>(7), homography.at<double>(8));

  // cv::Mat imgMatches;
  // cv::drawMatches(queryMat, queryImg.m_keypoints, trainMat, trainImg.m_keypoints, goodMatches, imgMatches, cv::Scalar::all(-1), cv::Scalar::all(-1), { }, cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  // cv::imwrite("img.png", imgMatches);

  // cv::Mat warped;
  // cv::warpPerspective(imgMat, warped, homography, { imgMat.cols, imgMat.rows });
  // cv::imwrite("warp.png", (warped * 0.5) + (refMat * 0.5));
}

Context::ImgData Context::detect(const ImgPtr& image) {
  cv::Mat raw;
  m_provider.getImageMatrix(image->getFileIndex()).convertTo(raw, CV_32F);
  raw = ((cv::min(cv::max(raw, 990.0), 3900.0) - 990.0) / (3900.0 - 990.0));

  cv::Mat mat;
  raw.convertTo(mat, CV_8U, 255);

  ImgData data = { image, raw.cols, raw.rows };
  m_detector->detectAndCompute(mat, cv::noArray(), data.m_keypoints, data.m_descriptors);

  spdlog::info("Found {} keypoints in image (file index = {})", data.m_keypoints.size(), image->getFileIndex());

  // cv::Mat outImg;
  // cv::drawKeypoints(mat, data.m_keypoints, outImg, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
  // cv::imwrite("key.png", outImg);

  return data;
}

