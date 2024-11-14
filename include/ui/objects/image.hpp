#pragma once

#include "glibmm/propertyproxy.h"
#include "img/provider.hpp"
#include "img/sequence.hpp"
#include "ui/objects/matrix.hpp"
#include <gtkmm.h>

namespace UI {

class Image : public Glib::Object {
  Glib::Property_ReadOnly<int> m_sequenceIndex;

  Glib::Property<bool> m_included;

  // Homography is stored like in the sequence file,
  // it needs to be transposed to use it in the rendering pipeline.
  HomographyMatrix m_homography;

  Glib::Property<long> m_minLevel;
  Glib::Property<long> m_maxLevel;
  Glib::Property<long> m_maxTypeValue;

  // Glib::Property<double> m_xOffset;
  // Glib::Property<double> m_yOffset;

  Img::Sequence& m_sequence;
  Img::ImageProvider& m_imageProvider;

public:
  Image(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider);
  virtual ~Image() = default;

  static Glib::RefPtr<Image> create(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider);

  Img::ImageProvider& getProvider();
  Img::SequenceImage& getSequenceImage();
  bool isReference();

  int getIndex() const;
  bool getIncluded() const;

  float getScaledMinLevel() const;
  float getScaledMaxLevel() const;

  Glib::PropertyProxy_ReadOnly<int> propertyIndex() const;
  Glib::PropertyProxy<bool> propertyIncluded();
  Glib::PropertyProxy<double> propertyXOffset();
  Glib::PropertyProxy<double> propertyYOffset();

  HomographyMatrix& homography();

  void refresh();

private:
  void includedChanged();
};

}

