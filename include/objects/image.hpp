#pragma once

#include "objects/registration.hpp"
#include "objects/stats.hpp"

namespace IO {
  class Sequence;
  class ImageProvider;
}

namespace Obj {

class Image : public Glib::Object {
  Glib::Property_ReadOnly<int> m_sequenceIndex;
  Glib::Property<int> m_fileIndex;
  Glib::Property<bool> m_included;
  Glib::Property<int> m_width;
  Glib::Property<int> m_height;

  std::vector<Glib::RefPtr<Stats>> m_stats;
  Glib::RefPtr<Registration> m_registration;

  Glib::RefPtr<IO::Sequence> m_sequence;

  sigc::connection m_connRegistration;
  std::vector<sigc::connection> m_connStats;

  // Dummy properties that get bound with the homography matrix,
  // they are here to make sequence list view easier to manage
  Glib::Property<double> m_xOffset;
  Glib::Property<double> m_yOffset;
  Glib::RefPtr<Glib::Binding> m_xBind;
  Glib::RefPtr<Glib::Binding> m_yBind;

public:
  using redraw_signal_type = sigc::signal<void()>;

private:
  redraw_signal_type m_redrawSignal;
  bool m_notified;

  void notifyRedraw();

public:
  Image(int index, int layerCount, const Glib::RefPtr<IO::Sequence>& sequence);
  virtual ~Image() = default;

  redraw_signal_type signalRedraw();
  void clearRedrawFlag();
  bool isReference();

  void setRegistration(const Glib::RefPtr<Registration>& value);
  void setStats(int layer, const Glib::RefPtr<Stats>& value);

  Glib::RefPtr<Registration> getRegistration();
  Glib::RefPtr<Stats> getStats(int layer);

  void calculateStats(IO::ImageProvider& provider);

  Glib::PropertyProxy_ReadOnly<int> propertySequenceIndex();
  Glib::PropertyProxy<int> propertyFileIndex();
  Glib::PropertyProxy<bool> propertyIncluded();
  Glib::PropertyProxy<int> propertyWidth();
  Glib::PropertyProxy<int> propertyHeight();
  
  int getSequenceIndex() const;
  int getFileIndex() const;
  bool getIncluded() const;
  int getWidth() const;
  int getHeight() const;
  
  void setFileIndex(int value);
  void setIncluded(bool value);
  void setWidth(int value);
  void setHeight(int value);

  Glib::PropertyProxy<double> propertyXOffset();
  Glib::PropertyProxy<double> propertyYOffset();

  static Glib::RefPtr<Image> create(int seqIndex, int layerCount, const Glib::RefPtr<IO::Sequence>& sequence);
};

/*class Image : public Glib::Object {
  Glib::Property_ReadOnly<int> m_sequenceIndex;

  Glib::Property<bool> m_included;

  // Homography is stored like in the sequence file,
  // it needs to be transposed to use it in the rendering pipeline.
  HomographyMatrix m_homography;

  // Glib::Property<double> m_minLevel;
  // Glib::Property<double> m_maxLevel;
  // Glib::Property<double> m_maxTypeValue;

  // Img::Sequence& m_sequence;
  // Img::ImageProvider& m_imageProvider;

public:
  using redraw_signal_type = sigc::signal<void()>;

private:
  redraw_signal_type m_redrawSignal;
  bool m_notified;

  void notifyRedraw();

public:
  // Image(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider);
  virtual ~Image() = default;

  // static Glib::RefPtr<Image> create(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider);

  // Img::ImageProvider& getProvider();
  // Img::SequenceImage& getSequenceImage();
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
};*/

} // namespace Obj

