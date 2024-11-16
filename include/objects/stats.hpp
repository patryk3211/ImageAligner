#pragma once

#include <gtkmm.h>

namespace Obj {

class Stats : public Glib::Object {
  Glib::Property<long> m_totalPixels;
  Glib::Property<long> m_goodPixels;

  Glib::Property<double> m_mean;
  Glib::Property<double> m_median;
  Glib::Property<double> m_sigma;
  Glib::Property<double> m_avgDev;
  Glib::Property<double> m_mad;
  Glib::Property<double> m_sqrtBWMV;
  Glib::Property<double> m_location;
  Glib::Property<double> m_scale;
  Glib::Property<double> m_min;
  Glib::Property<double> m_max;

  Glib::Property<double> m_normValue;
  Glib::Property<double> m_bgNoise;

public:
  using modified_signal_type = sigc::signal<void()>;

private:
  modified_signal_type m_signalModified;
  void emitModified();

public:
  Stats();
  virtual ~Stats() = default;

  modified_signal_type signalModified();

  // Use this method to store modified data back
  // in the underlying IO sequence entry.
  // void writeBack();

  Glib::PropertyProxy<long> propertyTotalPixels();
  Glib::PropertyProxy<long> propertyGoodPixels();
  Glib::PropertyProxy<double> propertyMean();
  Glib::PropertyProxy<double> propertyMedian();
  Glib::PropertyProxy<double> propertySigma();
  Glib::PropertyProxy<double> propertyAvgDev();
  Glib::PropertyProxy<double> propertyMad();
  Glib::PropertyProxy<double> propertySqrtBWMV();
  Glib::PropertyProxy<double> propertyLocation();
  Glib::PropertyProxy<double> propertyScale();
  Glib::PropertyProxy<double> propertyMin();
  Glib::PropertyProxy<double> propertyMax();
  Glib::PropertyProxy<double> propertyNormValue();
  Glib::PropertyProxy<double> propertyBgNoise();

  long getTotalPixels() const;
  long getGoodPixels() const;
  double getMean() const;
  double getMedian() const;
  double getSigma() const;
  double getAvgDev() const;
  double getMad() const;
  double getSqrtBWMV() const;
  double getLocation() const;
  double getScale() const;
  double getMin() const;
  double getMax() const;
  double getNormValue() const;
  double getBgNoise() const;

  void setTotalPixels(long value);
  void setGoodPixels(long value);
  void setMean(double value);
  void setMedian(double value);
  void setSigma(double value);
  void setAvgDev(double value);
  void setMad(double value);
  void setSqrtBWMV(double value);
  void setLocation(double value);
  void setScale(double value);
  void setMin(double value);
  void setMax(double value);
  void setNormValue(double value);
  void setBgNoise(double value);

  static Glib::RefPtr<Stats> create();
};

} // namespace Obj

