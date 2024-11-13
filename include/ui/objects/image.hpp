#pragma once

#include "glibmm/propertyproxy.h"
#include "img/provider.hpp"
#include "img/sequence.hpp"
#include <gtkmm.h>

namespace UI {

class Image : public Glib::Object {
  Glib::Property_ReadOnly<int> m_sequenceIndex;

  Glib::Property<bool> m_included;

  Glib::Property<double> m_xOffset;
  Glib::Property<double> m_yOffset;

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

  Glib::PropertyProxy_ReadOnly<int> propertyIndex() const;
  Glib::PropertyProxy<bool> propertyIncluded();
  Glib::PropertyProxy<double> propertyXOffset();
  Glib::PropertyProxy<double> propertyYOffset();

  void refresh();

private:
  void includedChanged();
};

}

