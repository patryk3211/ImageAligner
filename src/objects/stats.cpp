#include "objects/stats.hpp"
#include "glibmm/refptr.h"

using namespace Obj;

Stats::Stats()
  : ObjectBase("StatsObject")
  , m_totalPixels(*this, "total-pixels")
  , m_goodPixels(*this, "good-pixels")
  , m_mean(*this, "mean")
  , m_median(*this, "median")
  , m_sigma(*this, "sigma")
  , m_avgDev(*this, "average-deviation")
  , m_mad(*this, "mad")
  , m_sqrtBWMV(*this, "sqrt-bwmv")
  , m_location(*this, "location")
  , m_scale(*this, "scale")
  , m_min(*this, "min")
  , m_max(*this, "max")
  , m_normValue(*this, "normalization-value")
  , m_bgNoise(*this, "background-noise") {
  auto slot = sigc::mem_fun(*this, &Stats::emitModified);
  m_totalPixels.get_proxy().signal_changed().connect(slot);
  m_goodPixels.get_proxy().signal_changed().connect(slot);
  m_mean.get_proxy().signal_changed().connect(slot);
  m_median.get_proxy().signal_changed().connect(slot);
  m_sigma.get_proxy().signal_changed().connect(slot);
  m_avgDev.get_proxy().signal_changed().connect(slot);
  m_mad.get_proxy().signal_changed().connect(slot);
  m_sqrtBWMV.get_proxy().signal_changed().connect(slot);
  m_location.get_proxy().signal_changed().connect(slot);
  m_scale.get_proxy().signal_changed().connect(slot);
  m_min.get_proxy().signal_changed().connect(slot);
  m_max.get_proxy().signal_changed().connect(slot);
  m_normValue.get_proxy().signal_changed().connect(slot);
  m_bgNoise.get_proxy().signal_changed().connect(slot);
}

Glib::RefPtr<Stats> Stats::create() {
  return Glib::make_refptr_for_instance(new Stats());
}

Stats::modified_signal_type Stats::signalModified() {
  return m_signalModified;
}

void Stats::emitModified() {
  m_signalModified.emit();
}

Glib::PropertyProxy<long> Stats::propertyTotalPixels() {
  return m_totalPixels.get_proxy();
}

Glib::PropertyProxy<long> Stats::propertyGoodPixels() {
  return m_goodPixels.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyMean() {
  return m_mean.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyMedian() {
  return m_median.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertySigma() {
  return m_sigma.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyAvgDev() {
  return m_avgDev.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyMad() {
  return m_mad.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertySqrtBWMV() {
  return m_sqrtBWMV.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyLocation() {
  return m_location.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyScale() {
  return m_scale.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyMin() {
  return m_min.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyMax() {
  return m_max.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyNormValue() {
  return m_normValue.get_proxy();
}

Glib::PropertyProxy<double> Stats::propertyBgNoise() {
  return m_bgNoise.get_proxy();
}

long Stats::getTotalPixels() const {
  return m_totalPixels.get_value();
}

long Stats::getGoodPixels() const {
  return m_goodPixels.get_value();
}

double Stats::getMean() const {
  return m_mean.get_value();
}

double Stats::getMedian() const {
  return m_median.get_value();
}

double Stats::getSigma() const {
  return m_sigma.get_value();
}

double Stats::getAvgDev() const {
  return m_avgDev.get_value();
}

double Stats::getMad() const {
  return m_mad.get_value();
}

double Stats::getSqrtBWMV() const {
  return m_sqrtBWMV.get_value();
}

double Stats::getLocation() const {
  return m_location.get_value();
}

double Stats::getScale() const {
  return m_scale.get_value();
}

double Stats::getMin() const {
  return m_min.get_value();
}

double Stats::getMax() const {
  return m_max.get_value();
}

double Stats::getNormValue() const {
  return m_normValue.get_value();
}

double Stats::getBgNoise() const {
  return m_bgNoise.get_value();
}


void Stats::setTotalPixels(long value) {
  m_totalPixels.set_value(value);
}

void Stats::setGoodPixels(long value) {
  m_goodPixels.set_value(value);
}

void Stats::setMean(double value) {
  m_mean.set_value(value);
}

void Stats::setMedian(double value) {
  m_median.set_value(value);
}

void Stats::setSigma(double value) {
  m_sigma.set_value(value);
}

void Stats::setAvgDev(double value) {
  m_avgDev.set_value(value);
}

void Stats::setMad(double value) {
  m_mad.set_value(value);
}

void Stats::setSqrtBWMV(double value) {
  m_sqrtBWMV.set_value(value);
}

void Stats::setLocation(double value) {
  m_location.set_value(value);
}

void Stats::setScale(double value) {
  m_scale.set_value(value);
}

void Stats::setMin(double value) {
  m_min.set_value(value);
}

void Stats::setMax(double value) {
  m_max.set_value(value);
}

void Stats::setNormValue(double value) {
  m_normValue.set_value(value);
}

void Stats::setBgNoise(double value) {
  m_bgNoise.set_value(value);
}

