#pragma once

#include "objects/matrix.hpp"
#include <gtkmm.h>

namespace Obj {

class Registration : public Glib::Object {
  Glib::Property<float> m_FWHM;
  Glib::Property<float> m_weightedFWHM;
  Glib::Property<float> m_roundness;
  Glib::Property<double> m_quality;
  Glib::Property<float> m_backgroundLevel;
  Glib::Property<int> m_numberOfStars;

  HomographyMatrix m_matrix;

public:
  using modified_signal_type = sigc::signal<void()>;

private:
  modified_signal_type m_signalModified;
  void emitModified();

public:
  Registration();
  virtual ~Registration() = default;

  modified_signal_type signalModified();

  HomographyMatrix& matrix();
  const HomographyMatrix& matrix() const;

  Glib::PropertyProxy<float> propertyFWHM();
  Glib::PropertyProxy<float> propertyWeightedFWHM();
  Glib::PropertyProxy<float> propertyRoundness();
  Glib::PropertyProxy<double> propertyQuality();
  Glib::PropertyProxy<float> propertyBackgroundLevel();
  Glib::PropertyProxy<int> propertyNumberOfStars();

  float getFWHM() const;
  float getWeightedFWHM() const;
  float getRoundness() const;
  double getQuality() const;
  float getBackgroundLevel() const;
  int getNumberOfStars() const;

  void setFWHM(float value);
  void setWeightedFWHM(float value);
  void setRoundness(float value);
  void setQuality(double value);
  void setBackgroundLevel(float value);
  void setNumberOfStars(int value);

  static Glib::RefPtr<Registration> create();
};

} // namespace Obj

