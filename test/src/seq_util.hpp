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

static int check_registration(const Img::ImageRegistration& reg, int layer, float fwhm, float wfwhm, float roundness, double quality,
                              float backgroundLvl, int numberOfStarts, double h0, double h1, double h2, double h3, double h4,
                              double h5, double h6, double h7, double h8) {
  if(reg.m_layer != layer)
    return 0;
  if(reg.m_FWHM != fwhm)
    return 0;
  if(reg.m_weightedFWHM != wfwhm)
    return 0;
  if(reg.m_roundness != roundness)
    return 0;
  if(reg.m_quality != quality)
    return 0;
  if(reg.m_backgroundLevel != backgroundLvl)
    return 0;
  if(reg.m_numberOfStars != numberOfStarts)
    return 0;

  if(reg.m_homographyMatrix[0] != h0)
    return 0;
  if(reg.m_homographyMatrix[1] != h1)
    return 0;
  if(reg.m_homographyMatrix[2] != h2)
    return 0;
  if(reg.m_homographyMatrix[3] != h3)
    return 0;
  if(reg.m_homographyMatrix[4] != h4)
    return 0;
  if(reg.m_homographyMatrix[5] != h5)
    return 0;
  if(reg.m_homographyMatrix[6] != h6)
    return 0;
  if(reg.m_homographyMatrix[7] != h7)
    return 0;
  if(reg.m_homographyMatrix[8] != h8)
    return 0;

  return 1;
}

