#include "ui/objects/image.hpp"

#include <spdlog/spdlog.h>

using namespace UI;
using namespace Img;

Image::Image(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider) 
  : ObjectBase("ImageObject")
  , m_sequenceIndex(*this, "index", seqIndex)
  , m_included(*this, "included", true)
  , m_minLevel(*this, "level-min", 0)
  , m_maxLevel(*this, "level-max", 0)
  , m_maxTypeValue(*this, "type-max", 0)
  , m_sequence(sequence)
  , m_imageProvider(provider) {
  m_included.get_proxy().signal_changed().connect(sigc::mem_fun(*this, &Image::includedChanged));
  refresh();
}

Img::ImageProvider& Image::getProvider() {
  return m_imageProvider;
}

Img::SequenceImage& Image::getSequenceImage() {
  return m_sequence.image(m_sequenceIndex.get_value());
}

bool Image::isReference() {
  return m_sequence.referenceImage() == m_sequenceIndex.get_value();
}

void Image::refresh() {
  auto& img = m_sequence.image(m_sequenceIndex.get_value());
  m_included.set_value(img.m_included);

  auto params = m_imageProvider.getImageParameters(img.m_fileIndex);
  // Idk about this...
  m_maxTypeValue.set_value(DataType::typeMax(params.type()));

  if(img.m_registration) {
    auto& reg = img.m_registration;
    m_homography.write(reg->m_homographyMatrix);
  }

  if(img.m_stats.size() > 0) {
    // TODO: Currently min/max level uses stats from red channel, this is wrong for multichannel images
    m_minLevel.set_value(img.m_stats[0].m_min);
    m_maxLevel.set_value(img.m_stats[0].m_max);
  } else {
    m_minLevel.set_value(0);
    m_maxLevel.set_value(m_maxTypeValue.get_value());
  }
}

void Image::includedChanged() {
  int& seqIncluded = m_sequence.image(m_sequenceIndex.get_value()).m_included;

  // Force into a bool before comparison
  if((!!seqIncluded) != m_included.get_value()) {
    seqIncluded = m_included.get_value();
    if(seqIncluded) {
      m_sequence.selectedCount()++;
    } else {
      m_sequence.selectedCount()--;
    }
  }
}

int Image::getIndex() const {
  return m_sequenceIndex.get_value();
}

bool Image::getIncluded() const {
  return m_included.get_value();
}

float Image::getScaledMinLevel() const {
  return (float)m_minLevel.get_value() / m_maxTypeValue.get_value();
}

float Image::getScaledMaxLevel() const {
  return (float)m_maxLevel.get_value() / m_maxTypeValue.get_value();
}

Glib::PropertyProxy_ReadOnly<int> Image::propertyIndex() const {
  return m_sequenceIndex.get_proxy();
}

Glib::PropertyProxy<bool> Image::propertyIncluded() {
  return m_included.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyXOffset() {
  return m_homography.property(2, 0);
}

Glib::PropertyProxy<double> Image::propertyYOffset() {
  return m_homography.property(2, 1);
}

HomographyMatrix& Image::homography() {
  return m_homography;
}

Glib::RefPtr<Image> Image::create(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider) {
  return Glib::make_refptr_for_instance(new Image(seqIndex, sequence, provider));
}

