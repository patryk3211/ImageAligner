#include "objects/image.hpp"
#include "io/sequence.hpp"
#include "io/provider.hpp"

#include <spdlog/spdlog.h>

using namespace Obj;
using namespace IO;

Image::Image(int seqIndex, int layerCount, const Glib::RefPtr<IO::Sequence>& sequence)
  : ObjectBase("ImageObject")
  , m_sequenceIndex(*this, "sequence-index", seqIndex)
  , m_fileIndex(*this, "file-index")
  , m_included(*this, "included")
  , m_width(*this, "width")
  , m_height(*this, "height")
  , m_stats(layerCount)
  , m_sequence(sequence)
  , m_connStats(layerCount)
  , m_xOffset(*this, "x-offset")
  , m_yOffset(*this, "y-offset") {
  m_notified = false;

  auto slot = sigc::mem_fun(*this, &Image::notifyRedraw);
  m_fileIndex.get_proxy().signal_changed().connect(slot);
  m_included.get_proxy().signal_changed().connect(slot);
}

Glib::RefPtr<Image> Image::create(int seqIndex, int layerCount, const Glib::RefPtr<IO::Sequence>& sequence) {
  return Glib::make_refptr_for_instance(new Image(seqIndex, layerCount, sequence));
}

void Image::setRegistration(const Glib::RefPtr<Registration>& value) {
  m_registration = value;
  
  if(!m_connRegistration.empty())
    m_connRegistration.disconnect();

  if(m_xBind) {
    m_xBind->unbind();
    m_xBind = nullptr;
  }
  if(m_yBind) {
    m_yBind->unbind();
    m_yBind = nullptr;
  }

  if(value) {
    m_connRegistration = value->signalModified().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
    m_xBind = Glib::Binding::bind_property(value->matrix().property(2), m_xOffset.get_proxy(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
    m_yBind = Glib::Binding::bind_property(value->matrix().property(5), m_yOffset.get_proxy(), Glib::Binding::Flags::SYNC_CREATE | Glib::Binding::Flags::BIDIRECTIONAL);
  }

  notifyRedraw();
}

void Image::setStats(int layer, const Glib::RefPtr<Stats>& value) {
  m_stats[layer] = value;
  
  if(!m_connStats[layer].empty())
    m_connStats[layer].disconnect();
  if(value) {
    m_connStats[layer] = value->signalModified().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  }

  notifyRedraw();
}

Glib::RefPtr<Registration> Image::getRegistration() {
  return m_registration;
}

Glib::RefPtr<Stats> Image::getStats(int layer) {
  return m_stats[layer];
}

void Image::calculateStats(ImageProvider& provider) {
  auto params = provider.getImageParameters(m_fileIndex.get_value());
  bool changed = false;

  for(int i = 0; i < m_stats.size(); ++i) {
    if(m_stats[i])
      continue;

    auto stats = Stats::create();
    stats->setTotalPixels(params.width() * params.height());
    stats->setGoodPixels(-1);
    stats->setMean(-999999);
    stats->setMedian(-999999);
    stats->setSigma(-999999);
    stats->setAvgDev(-999999);
    stats->setMad(-999999);
    stats->setSqrtBWMV(-999999);
    stats->setLocation(-999999);
    stats->setScale(-999999);
    stats->setNormValue(provider.maxTypeValue());
    stats->setBgNoise(-999999);
    
    // TODO: Calculate these
    stats->setMin(0);
    stats->setMax(provider.maxTypeValue());
    
    m_stats[i] = stats;
    m_connStats[i] = stats->signalModified().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
    changed = true;
  }

  if(changed)
    notifyRedraw();
}

Glib::PropertyProxy_ReadOnly<int> Image::propertySequenceIndex() {
  return m_sequenceIndex.get_proxy();
}

Glib::PropertyProxy<int> Image::propertyFileIndex() {
  return m_fileIndex.get_proxy();
}

Glib::PropertyProxy<bool> Image::propertyIncluded() {
  return m_included.get_proxy();
}

Glib::PropertyProxy<int> Image::propertyWidth() {
  return m_width.get_proxy();
}

Glib::PropertyProxy<int> Image::propertyHeight() {
  return m_height.get_proxy();
}

int Image::getSequenceIndex() const {
  return m_sequenceIndex.get_value();
}

int Image::getFileIndex() const {
  return m_fileIndex.get_value();
}

bool Image::getIncluded() const {
  return m_included.get_value();
}

int Image::getWidth() const {
  return m_width.get_value();
}

int Image::getHeight() const {
  return m_height.get_value();
}

void Image::setFileIndex(int value) {
  m_fileIndex.set_value(value);
}

void Image::setIncluded(bool value) {
  m_included.set_value(value);
}

void Image::setWidth(int value) {
  m_width.set_value(value);
}

void Image::setHeight(int value) {
  m_height.set_value(value);
}

Image::redraw_signal_type Image::signalRedraw() {
  return m_redrawSignal;
}

bool Image::isReference() {
  return m_sequence.lock()->getReferenceImageIndex() == m_sequenceIndex.get_value();
}

void Image::notifyRedraw() {
  if(!m_notified) {
    m_redrawSignal.emit();
    m_notified = true;
  }

  // Any change to a sequence's object marks it as dirty.
  m_sequence.lock()->markDirty();
}

void Image::clearRedrawFlag() {
  m_notified = false;
}

Glib::PropertyProxy<double> Image::propertyXOffset() {
  return m_xOffset.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyYOffset() {
  return m_yOffset.get_proxy();
}

