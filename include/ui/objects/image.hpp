#pragma once

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

  Glib::Property<double> m_minLevel;
  Glib::Property<double> m_maxLevel;
  Glib::Property<double> m_maxTypeValue;

  Img::Sequence& m_sequence;
  Img::ImageProvider& m_imageProvider;

public:
  using redraw_signal_type = sigc::signal<void()>;

private:
  redraw_signal_type m_redrawSignal;
  bool m_notified;

  void notifyRedraw();

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

  double getTypeMax() const;
  double getLevelMin() const;
  double getLevelMax() const;

  Glib::PropertyProxy_ReadOnly<int> propertyIndex() const;
  Glib::PropertyProxy<bool> propertyIncluded();
  Glib::PropertyProxy<double> propertyXOffset();
  Glib::PropertyProxy<double> propertyYOffset();

  Glib::PropertyProxy<double> propertyLevelMin();
  Glib::PropertyProxy<double> propertyLevelMax();
  Glib::PropertyProxy<double> propertyTypeMax();

  redraw_signal_type signalRedraw();
  void clearRedrawFlag();

  HomographyMatrix& homography();
  
  // Forces the properties to be updated
  // from the underlying image sequence entry
  void refresh();

private:
  void includedChanged();
};

}

