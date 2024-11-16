#include "objects/registration.hpp"

using namespace Obj;

Registration::Registration()
  : ObjectBase("RegistrationObject")
  , m_FWHM(*this, "fwhm")
  , m_weightedFWHM(*this, "w-fwhm")
  , m_roundness(*this, "roundness")
  , m_quality(*this, "quality")
  , m_backgroundLevel(*this, "background-level")
  , m_numberOfStars(*this, "number-of-stars") {
  auto slot = sigc::mem_fun(*this, &Registration::emitModified);
  m_FWHM.get_proxy().signal_changed().connect(slot);
  m_weightedFWHM.get_proxy().signal_changed().connect(slot);
  m_roundness.get_proxy().signal_changed().connect(slot);
  m_quality.get_proxy().signal_changed().connect(slot);
  m_backgroundLevel.get_proxy().signal_changed().connect(slot);
  m_numberOfStars.get_proxy().signal_changed().connect(slot);

  for(int i = 0; i < 9; ++i)
    m_matrix.property(i).signal_changed().connect(slot);

  m_matrix.set(0, 1);
  m_matrix.set(4, 1);
  m_matrix.set(8, 1);
}

Glib::RefPtr<Registration> Registration::create() {
  return Glib::make_refptr_for_instance(new Registration());
}

Registration::modified_signal_type Registration::signalModified() {
  return m_signalModified;
}

void Registration::emitModified() {
  m_signalModified.emit();
}

HomographyMatrix& Registration::matrix() {
  return m_matrix;
}

const HomographyMatrix& Registration::matrix() const {
  return m_matrix;
}

// Property proxies

Glib::PropertyProxy<float> Registration::propertyFWHM() {
  return m_FWHM.get_proxy();
}

Glib::PropertyProxy<float> Registration::propertyWeightedFWHM() {
  return m_weightedFWHM.get_proxy();
}

Glib::PropertyProxy<float> Registration::propertyRoundness() {
  return m_roundness.get_proxy();
}

Glib::PropertyProxy<double> Registration::propertyQuality() {
  return m_quality.get_proxy();
}

Glib::PropertyProxy<float> Registration::propertyBackgroundLevel() {
  return m_backgroundLevel.get_proxy();
}

Glib::PropertyProxy<int> Registration::propertyNumberOfStars() {
  return m_numberOfStars.get_proxy();
}

// Getters

float Registration::getFWHM() const {
  return m_FWHM.get_value();
}

float Registration::getWeightedFWHM() const {
  return m_weightedFWHM.get_value();
}

float Registration::getRoundness() const {
  return m_roundness.get_value();
}

double Registration::getQuality() const {
  return m_quality.get_value();
}

float Registration::getBackgroundLevel() const {
  return m_backgroundLevel.get_value();
}

int Registration::getNumberOfStars() const {
  return m_numberOfStars.get_value();
}

// Setters

void Registration::setFWHM(float value) {
  m_FWHM.set_value(value);
}

void Registration::setWeightedFWHM(float value) {
  m_weightedFWHM.set_value(value);
}

void Registration::setRoundness(float value) {
  m_roundness.set_value(value);
}

void Registration::setQuality(double value) {
  m_quality.set_value(value);
}

void Registration::setBackgroundLevel(float value) {
  m_backgroundLevel.set_value(value);
}

void Registration::setNumberOfStars(int value) {
  m_numberOfStars.set_value(value);
}

