#pragma once

#include "io/sequence.hpp"
#include "objects/image.hpp"

using namespace IO;
using namespace Obj;

static int check_header(const Sequence& seq, const std::string& name, int startImage,
                 int imageCount, int selectedCount, int fixedLength, int referenceImage,
                 int version, int variableSize, int fzFlag) {
  if(seq.getSequenceName().raw() != name)
    return 0;
  if(seq.getFileIndexFirst() != startImage)
    return 0;
  if(seq.getImageCount() != imageCount)
    return 0;
  if(seq.getSelectedCount() != selectedCount)
    return 0;
  if(seq.getFileIndexFixedLength() != fixedLength)
    return 0;
  if(seq.getReferenceImageIndex() != referenceImage)
    return 0;
  if(seq.getVersion() != version)
    return 0;
  if(seq.getVariableSizeImages() != variableSize)
    return 0;
  if(seq.getFzFlag() != fzFlag)
    return 0;
  return 1;
}

static int check_image(const Image& img, int index, bool selected) {
  if(img.getFileIndex() != index)
    return 0;
  if(img.getIncluded() != selected)
    return 0;
  return 1;
}

static int check_stats(const Stats& stats, long pixels, long goodPixels, double mean,
                       double median, double sigma, double avgDev, double mad, double sqrtBWMV, double location,
                       double scale, double min, double max, double normValue, double bgNoise) {
  if(stats.getTotalPixels() != pixels)
    return 0;
  if(stats.getGoodPixels() != goodPixels)
    return 0;
  if(stats.getMean() != mean)
    return 0;
  if(stats.getMedian() != median)
    return 0;
  if(stats.getSigma() != sigma)
    return 0;
  if(stats.getAvgDev() != avgDev)
    return 0;
  if(stats.getMad() != mad)
    return 0;
  if(stats.getSqrtBWMV() != sqrtBWMV)
    return 0;
  if(stats.getLocation() != location)
    return 0;
  if(stats.getScale() != scale)
    return 0;
  if(stats.getMin() != min)
    return 0;
  if(stats.getMax() != max)
    return 0;
  if(stats.getNormValue() != normValue)
    return 0;
  if(stats.getBgNoise() != bgNoise)
    return 0;
  return 1;
}

static int check_registration(const Registration& reg, float fwhm, float wfwhm, float roundness, double quality,
                              float backgroundLvl, int numberOfStarts, double h0, double h1, double h2, double h3, double h4,
                              double h5, double h6, double h7, double h8) {
  if(reg.getFWHM() != fwhm)
    return 0;
  if(reg.getWeightedFWHM() != wfwhm)
    return 0;
  if(reg.getRoundness() != roundness)
    return 0;
  if(reg.getQuality() != quality)
    return 0;
  if(reg.getBackgroundLevel() != backgroundLvl)
    return 0;
  if(reg.getNumberOfStars() != numberOfStarts)
    return 0;

  if(reg.matrix().get(0) != h0)
    return 0;
  if(reg.matrix().get(1) != h1)
    return 0;
  if(reg.matrix().get(2) != h2)
    return 0;
  if(reg.matrix().get(3) != h3)
    return 0;
  if(reg.matrix().get(4) != h4)
    return 0;
  if(reg.matrix().get(5) != h5)
    return 0;
  if(reg.matrix().get(6) != h6)
    return 0;
  if(reg.matrix().get(7) != h7)
    return 0;
  if(reg.matrix().get(8) != h8)
    return 0;

  return 1;
}

