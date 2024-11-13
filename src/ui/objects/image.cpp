#include "ui/objects/image.hpp"

#include <spdlog/spdlog.h>

using namespace UI;
using namespace Img;

Image::Image(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider) 
  : ObjectBase("ImageObject")
  , m_sequenceIndex(*this, "index", seqIndex)
  , m_included(*this, "included", true)
  , m_xOffset(*this, "x-offset", 0)
  , m_yOffset(*this, "y-offset", 0)
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

  if(img.m_registration) {
    auto& reg = img.m_registration;
    m_xOffset.set_value(reg->m_homographyMatrix[2]);
    m_yOffset.set_value(reg->m_homographyMatrix[5]);
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

Glib::PropertyProxy_ReadOnly<int> Image::propertyIndex() const {
  return m_sequenceIndex.get_proxy();
}

Glib::PropertyProxy<bool> Image::propertyIncluded() {
  return m_included.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyXOffset() {
  return m_xOffset.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyYOffset() {
  return m_yOffset.get_proxy();
}

Glib::RefPtr<Image> Image::create(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider) {
  return Glib::make_refptr_for_instance(new Image(seqIndex, sequence, provider));
}

