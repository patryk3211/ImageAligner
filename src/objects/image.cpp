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
  }
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
  return m_sequence->getReferenceImageIndex() == m_sequenceIndex.get_value();
}

void Image::notifyRedraw() {
  if(!m_notified) {
    m_redrawSignal.emit();
    m_notified = true;
  }
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

/*
Image::Image(int seqIndex, Sequence& sequence, ImageProvider& provider) 
  : ObjectBase("ImageObject")
  , m_sequenceIndex(*this, "index", seqIndex)
  , m_included(*this, "included", true)
  , m_minLevel(*this, "level-min", 0)
  , m_maxLevel(*this, "level-max", 0)
  , m_maxTypeValue(*this, "type-max", 0)
  , m_sequence(sequence)
  , m_imageProvider(provider) {
  m_notified = false;
  refresh();

  // Connect signals
  m_included.get_proxy().signal_changed().connect(sigc::mem_fun(*this, &Image::includedChanged));

  // TODO: I might move all these properties into separate object classes
  m_minLevel.get_proxy().signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_maxLevel.get_proxy().signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));

  m_homography.property(0).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(1).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(2).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(3).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(4).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(5).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(6).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(7).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
  m_homography.property(8).signal_changed().connect(sigc::mem_fun(*this, &Image::notifyRedraw));
}

ImageProvider& Image::getProvider() {
  return m_imageProvider;
}

SequenceImage& Image::getSequenceImage() {
  return m_sequence.image(m_sequenceIndex.get_value());
}

void Image::refresh() {
  auto& img = m_sequence.image(m_sequenceIndex.get_value());
  m_included.set_value(img.m_included);

  auto params = m_imageProvider.getImageParameters(img.m_fileIndex);
  // Idk about this...
  m_maxTypeValue.set_value(DataType::typeMax(params.type()));

  if(img.m_registration) {
    auto& reg = img.m_registration;
    m_homography.write(reg->m_homographyMatrix);
  }

  if(img.m_stats.size() > 0) {
    // TODO: Currently min/max level uses stats from red channel, this is wrong for multichannel images
    m_minLevel.set_value(img.m_stats[0].m_min);
    m_maxLevel.set_value(img.m_stats[0].m_max);
  } else {
    m_minLevel.set_value(0);
    m_maxLevel.set_value(m_maxTypeValue.get_value());
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

  notifyRedraw();
}

int Image::getIndex() const {
  return m_sequenceIndex.get_value();
}

bool Image::getIncluded() const {
  return m_included.get_value();
}

float Image::getScaledMinLevel() const {
  return (float)m_minLevel.get_value() / m_maxTypeValue.get_value();
}

float Image::getScaledMaxLevel() const {
  return (float)m_maxLevel.get_value() / m_maxTypeValue.get_value();
}

double Image::getTypeMax() const {
  return m_maxTypeValue.get_value();
}

double Image::getLevelMin() const {
  return m_minLevel.get_value();
}

double Image::getLevelMax() const {
  return m_maxLevel.get_value();
}

Glib::PropertyProxy_ReadOnly<int> Image::propertyIndex() const {
  return m_sequenceIndex.get_proxy();
}

Glib::PropertyProxy<bool> Image::propertyIncluded() {
  return m_included.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyXOffset() {
  return m_homography.property(2, 0);
}

Glib::PropertyProxy<double> Image::propertyYOffset() {
  return m_homography.property(2, 1);
}

Glib::PropertyProxy<double> Image::propertyLevelMin() {
  return m_minLevel.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyLevelMax() {
  return m_maxLevel.get_proxy();
}

Glib::PropertyProxy<double> Image::propertyTypeMax() {
  return m_maxTypeValue.get_proxy();
}

HomographyMatrix& Image::homography() {
  return m_homography;
}

Image::redraw_signal_type Image::signalRedraw() {
  return m_redrawSignal;
}

Glib::RefPtr<Image> Image::create(int seqIndex, Img::Sequence& sequence, Img::ImageProvider& provider) {
  return Glib::make_refptr_for_instance(new Image(seqIndex, sequence, provider));
}
*/
