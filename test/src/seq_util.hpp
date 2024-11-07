#pragma once

#include "img/sequence.hpp"

static int check_header(const Img::Sequence& seq, const std::string& name, int startImage,
                 int imageCount, int selectedCount, int fixedLength, int referenceImage,
                 int version, int variableSize, int fzFlag) {
  if(seq.name() != name)
    return 0;
  if(seq.startImage() != startImage)
    return 0;
  if(seq.imageCount() != imageCount)
    return 0;
  if(seq.selectedCount() != selectedCount)
    return 0;
  if(seq.fixedLength() != fixedLength)
    return 0;
  if(seq.referenceImage() != referenceImage)
    return 0;
  if(seq.version() != version)
    return 0;
  if(seq.variableSize() != variableSize)
    return 0;
  if(seq.fzFlag() != fzFlag)
    return 0;
  return 1;
}

static int check_image(const Img::SequenceImage& img, int index, bool selected) {
  if(img.m_fileIndex != index)
    return 0;
  if((!!img.m_included) != selected)
    return 0;
  return 1;
}

static int check_stats(const Img::ImageStats& stats, long pixels, long goodPixels, double mean,
                       double median, double sigma, double avgDev, double mad, double sqrtBWMV, double location,
                       double scale, double min, double max, double normValue, double bgNoise) {
  if(stats.m_totalPixels != pixels)
    return 0;
  if(stats.m_goodPixels != goodPixels)
    return 0;
  if(stats.m_mean != mean)
    return 0;
  if(stats.m_median != median)
    return 0;
  if(stats.m_sigma != sigma)
    return 0;
  if(stats.m_avgDev != avgDev)
    return 0;
  if(stats.m_mad != mad)
    return 0;
  if(stats.m_sqrtBWMV != sqrtBWMV)
    return 0;
  if(stats.m_location != location)
    return 0;
  if(stats.m_scale != scale)
    return 0;
  if(stats.m_min != min)
    return 0;
  if(stats.m_max != max)
    return 0;
  if(stats.m_normValue != normValue)
    return 0;
  if(stats.m_bgNoise != bgNoise)
    return 0;
  return 1;
}

