#include "io/sequence.hpp"
#include "objects/image.hpp"
#include "objects/registration.hpp"
#include "objects/stats.hpp"

#include <ctime>
#include <fstream>
#include <iostream>
#include <format>

#include <spdlog/spdlog.h>

using namespace IO;

Sequence::Sequence()
  : ObjectBase("SequenceObject")
  , m_sequenceName(*this, "name")
  , m_fileIndexFirst(*this, "first-file-index")
  , m_imageCount(*this, "image-count")
  , m_selectedCount(*this, "selected-count")
  , m_fileIndexFixedLength(*this, "file-index-fixed-length")
  , m_referenceImageIndex(*this, "reference-image-index")
  , m_version(*this, "version")
  , m_variableSizeImages(*this, "variable-size")
  , m_fzFlag(*this, "fz-flag")
  , m_layerCount(*this, "layer-count")
  , m_sequenceType(*this, "sequence-type", SequenceType::MULTI_FITS)
  , m_registrationLayer(*this, "registration-layer")
  , m_dirty(*this, "dirty", false) {
}

Sequence::~Sequence() {
  spdlog::trace("(Sequence) dtor entry");
}

Glib::RefPtr<Sequence> Sequence::create() {
  return Glib::make_refptr_for_instance(new Sequence());
}

// Siril sequence reader based on https://gitlab.com/free-astro/siril/-/blob/master/src/io/seqfile.c
Glib::RefPtr<Sequence> Sequence::readSequence(const std::filesystem::path& filepath) {
  spdlog::info("Reading sequence file '{}'", filepath.c_str());

  std::ifstream file(filepath);
  if(!file.is_open()) {
    spdlog::error("Failed to open file '{}'", filepath.c_str());
    return nullptr;
  }

  return readStream(file);
}

Glib::RefPtr<Sequence> Sequence::readStream(std::istream& stream) {
  Glib::RefPtr<Sequence> sequence;

  char line[512];
  const char* scanfmt = 0;
  int imageIndex = 0, statIndex = 0, regIndex = 0;
  while(stream.good()) {
    stream.getline(line, 512);
    switch(line[0]) {
      case '#':
        continue;
      case 'S': {
        if(line[2] == '"') {
          spdlog::error("Sequence doesn't have a name and will not be loaded!");
          return nullptr;
        }
        if(line[2] == '\'')
          scanfmt = "'%511[^']' %d %d %d %d %d %d %d %d";
        else
          scanfmt = "%511s %d %d %d %d %d %d %d %d";

        if(sequence) {
          spdlog::error("Sequence contains multiple header definitions!");
          return nullptr;
        }

        sequence = Sequence::create();
        char seqName[512];

        int start, count, selCount, fixed, reference, version, variable, fzFlag;

        if(sscanf(line + 2, scanfmt, seqName,
                  &start, &count, &selCount, &fixed,
                  &reference, &version, &variable, &fzFlag) < 6) {
          spdlog::error("Sequence header error");
          return nullptr;
        }

        sequence->m_sequenceName.set_value(seqName);
        sequence->m_fileIndexFirst.set_value(start);
        sequence->m_imageCount.set_value(count);
        sequence->m_selectedCount.set_value(selCount);
        sequence->m_fileIndexFixedLength.set_value(fixed);
        sequence->m_referenceImageIndex.set_value(reference);
        sequence->m_version.set_value(version);
        sequence->m_variableSizeImages.set_value(variable);
        sequence->m_fzFlag.set_value(fzFlag);

        if(version <= 3) {
          spdlog::error("Sequence versions below or equal to 3 are unsupported!");
          return nullptr;
        }
        break;
      }
      case 'L':
        if(line[1] == ' ') {
          int layerCount;
          if(sscanf(line + 2, "%d", &layerCount) != 1) {
            spdlog::error("Sequence file format error");
            return nullptr;
          }
          sequence->m_layerCount.set_value(layerCount);
        }
        break;
      case 'I': {
        int fileIndex, included, width, height;
        int tokenCount = sscanf(line + 2, "%d %d %d,%d",
                           &fileIndex, &included, &width, &height);
        if((tokenCount != 4 && sequence->getVariableSizeImages()) ||
            (tokenCount != 2 && !sequence->getVariableSizeImages())) {
          spdlog::error("Sequence file format error");
          return nullptr;
        }

        auto img = Obj::Image::create(imageIndex++, sequence->getLayerCount(), sequence);
        img->setFileIndex(fileIndex);
        img->setIncluded(included);
        if(tokenCount > 2) {
          img->setWidth(width);
          img->setHeight(height);
        }
        sequence->m_images.push_back(img);
        break;
      }
      case 'T':
        if(line[1] != 'F') {
          spdlog::error("Sequence type not supported! (only FITS sequences are currently supported)");
          return nullptr;
        }
        // Type F = Single fits file sequence (I think?)
        sequence->m_sequenceType.set_value(SequenceType::SINGLE_FITS);
        break;
      case 'M': {
        int layer = 0;
        if(line[1] >= '0' && line[1] <= '9') {
          // Regular (demosaiced) channel
          layer = line[1] - '0';
        } else if(line[1] == '*') {
          // CFA channel
        } else {
          spdlog::error("Invalid M line layer index!");
          return nullptr;
        }
        if(line[2] != '-') {
          spdlog::error("Invalid M line layer index!");
          return nullptr;
        }
        int image, consumed;
        if(sscanf(line + 3, "%d%n", &image, &consumed) != 1) {
          spdlog::error("Invalid or missing M line image index!");
          return nullptr;
        }
        auto img = sequence->m_images[image];
        if(!img) {
          spdlog::error("Stats defined for non-existant image");
          return nullptr;
        }
        long totalPixels, goodPixels;
        double mean, median, sigma, avgDev, mad, sqrtBWMV, location, scale, min, max, normValue, bgNoise;
        int tokenCount = sscanf(line + 4 + consumed, "%ld %ld %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                &totalPixels, &goodPixels,
                                &mean, &median,
                                &sigma, &avgDev,
                                &mad, &sqrtBWMV,
                                &location, &scale,
                                &min, &max,
                                &normValue, &bgNoise);
        if(tokenCount != 14) {
          spdlog::error("Malformed M line, file loading terminated.");
          return nullptr;
        }
        if(img->getStats(layer)) {
          spdlog::error("Redefinition of stats on an image layer");
          return nullptr;
        }
        auto stats = Obj::Stats::create();
        stats->setTotalPixels(totalPixels);
        stats->setGoodPixels(goodPixels);
        stats->setMean(mean);
        stats->setMedian(median);
        stats->setSigma(sigma);
        stats->setAvgDev(avgDev);
        stats->setMad(mad);
        stats->setSqrtBWMV(sqrtBWMV);
        stats->setLocation(location);
        stats->setScale(scale);
        stats->setMin(min);
        stats->setMax(max);
        stats->setNormValue(normValue);
        stats->setBgNoise(bgNoise);
        img->setStats(layer, stats);
        break;
      }
      // case 'D':
      //   std::cout << "Warning! D line is currently unsupported in sequences!" << std::endl;
      //   break;
      case 'R': {
        int layer;
        if(line[1] == '*') {
          layer = -1;
        } else if(line[1] >= '0' && line[1] <= '9') {
          layer = line[1] - '0';
        } else {
          spdlog::error("Sequence registration error");
          return nullptr;
        }

        if(regIndex == 0) {
          sequence->m_registrationLayer.set_value(layer);
        } else {
          if(sequence->m_registrationLayer.get_value() != layer) {
            spdlog::error("Sequence registers more than one layer, this is currently not supported");
            return nullptr;
          }
        }

        float FWHM, weightedFWHM, roundness, backgroundLevel;
        double quality;
        int numberOfStars;
        double homographyMatrix[9];
        int tokenCount = sscanf(line + 3, "%g %g %g %lg %g %d H %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                &FWHM, &weightedFWHM, &roundness,
                                &quality, &backgroundLevel, &numberOfStars,
                                &homographyMatrix[0], &homographyMatrix[1], &homographyMatrix[2],
                                &homographyMatrix[3], &homographyMatrix[4], &homographyMatrix[5],
                                &homographyMatrix[6], &homographyMatrix[7], &homographyMatrix[8]);

        if(tokenCount != 15) {
          spdlog::error("Sequence registration error");
          return nullptr;
        }

        auto reg = Obj::Registration::create();
        reg->setFWHM(FWHM);
        reg->setWeightedFWHM(weightedFWHM);
        reg->setRoundness(roundness);
        reg->setQuality(quality);
        reg->setBackgroundLevel(backgroundLevel);
        reg->setNumberOfStars(numberOfStars);
        reg->matrix().write(homographyMatrix);

        auto img = sequence->m_images[regIndex++];
        img->setRegistration(reg);
        break;
      }
      case 0:
        // Ignore empty line
        break;
      default:
        spdlog::warn("Unsupported line '{}' in sequence!", line[0]);
        break;
    }
  }

  sequence->validate();
  sequence->markClean();
  return sequence;
}

void Sequence::markDirty() {
  if(!m_dirty.get_value())
    m_dirty.set_value(true);
}

void Sequence::markClean() {
  if(m_dirty.get_value())
    m_dirty.set_value(false);
}

bool Sequence::isDirty() {
  return m_dirty.get_value();
}

Glib::PropertyProxy_ReadOnly<bool> Sequence::propertyDirty() {
  return const_cast<const Glib::Property<bool>&>(m_dirty).get_proxy();
}

void Sequence::validate() {
  if(m_images.size() != m_imageCount.get_value()) {
    spdlog::warn("Read more images than specified in the headers, correcting header information");
    m_imageCount.set_value(m_images.size());
  }
  int selected = 0;
  for(auto& img : m_images) {
    if(img->getIncluded())
      ++selected;
  }

  if(selected != m_selectedCount.get_value()) {
    spdlog::warn("Header selected image count doesn't match the actual selected image count, correcting");
    m_selectedCount.set_value(selected);
  }
}

void Sequence::prepareWrite(IO::ImageProvider& provider) {
  // Make sure that there are no gaps in registration and stats.
  bool calcRegistration = false;
  bool calcStats = false;
  for(auto iter = m_images.rbegin(); iter != m_images.rend(); ++iter) {
    auto& img = *iter;

    if(img->getRegistration()) {
      // Make sure all other registrations get filled
      calcRegistration = true;
    } else if(calcRegistration) {
      // Set with empty registration
      img->setRegistration(Obj::Registration::create());
    }

    bool hasAny = false;
    bool hasAll = true;
    for(uint l = 0; l < m_layerCount.get_value(); ++l) {
      if(img->getStats(l))
        hasAny = true;
      if(!img->getStats(l))
        hasAll = false;
    }

    if(hasAny) {
      calcStats = true;
    }

    if((hasAny && !hasAll) || (!hasAny && calcStats)) {
      // Calculate missing stats
      img->calculateStats(provider);
    }
  }
}

void Sequence::writeStream(std::ostream& stream) {
  // Start off with the header
  stream << "S '";
  stream << m_sequenceName.get_value();
  stream << "' ";
  stream << m_fileIndexFirst.get_value() << ' ';
  stream << m_imageCount.get_value() << ' ';
  stream << m_selectedCount.get_value() << ' ';
  stream << m_fileIndexFixedLength.get_value() << ' ';
  stream << m_referenceImageIndex.get_value() << ' ';
  stream << m_version.get_value() << ' ';
  stream << m_variableSizeImages.get_value() << ' ';
  stream << m_fzFlag.get_value() << '\n';

  // Emit sequence type
  switch(m_sequenceType.get_value()) {
    case SequenceType::SINGLE_FITS:
      stream << "TF\n";
      break;
    case SequenceType::MULTI_FITS:
      // No additional info for this type
      break;
  }
  // Layer count
  stream << "L " << m_layerCount.get_value() << '\n';
  
  // Write all images
  for(auto& img : m_images) {
    stream << std::format("I {} {}", img->getFileIndex(), img->getIncluded() ? 1 : 0);
    if(m_variableSizeImages.get_value()) {
      stream << std::format(" {},{}", img->getWidth(), img->getWidth());
    }
    stream << '\n';
  }

  // Write layer data
  for(uint l = 0; l < m_layerCount.get_value(); ++l) {
    if(m_registrationLayer.get_value() == l) {
      // Write registration info for layer
      for(uint i = 0; i < m_imageCount.get_value(); ++i) {
        auto& img = m_images[i];
        if(!img->getRegistration()) {
          // Registration info is written without gaps so the first image
          // without it means that all images above also don't have it.
          break;
        }
        auto& reg = *img->getRegistration();

        // Write registration line
        double matrix[9];
        reg.matrix().read(matrix);
        stream << std::format("R{} {} {} {} {} {} {} H {} {} {} {} {} {} {} {} {}\n",
                              l,
                              reg.getFWHM(),
                              reg.getWeightedFWHM(),
                              reg.getRoundness(),
                              reg.getQuality(),
                              reg.getBackgroundLevel(),
                              reg.getNumberOfStars(),
                              matrix[0],
                              matrix[1],
                              matrix[2],
                              matrix[3],
                              matrix[4],
                              matrix[5],
                              matrix[6],
                              matrix[7],
                              matrix[8]);
      }
    }

    // Write all image stats for this layer
    for(uint i = 0; i < m_imageCount; ++i) {
      auto& img = m_images[i];
      if(!img->getStats(l)) {
        // Image stats are written without gaps so the first image
        // without them means that all images above also don't have them.
        break;
      }

      // Write the M line
      auto& stats = *img->getStats(l);

      stream << std::format("M{}-{} {} {} {} {} {} {} {} {} {} {} {} {} {} {}\n",
                            l, i,
                            stats.getTotalPixels(),
                            stats.getGoodPixels(),
                            stats.getMean(),
                            stats.getMedian(),
                            stats.getSigma(),
                            stats.getAvgDev(),
                            stats.getMad(),
                            stats.getSqrtBWMV(),
                            stats.getLocation(),
                            stats.getScale(),
                            stats.getMin(),
                            stats.getMax(),
                            stats.getNormValue(),
                            stats.getBgNoise());
    }
  }
}

Glib::PropertyProxy<Glib::ustring> Sequence::propertySequenceName() {
  return m_sequenceName.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyFileIndexFirst() {
  return m_fileIndexFirst.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyImageCount() {
  return m_imageCount.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertySelectedCount() {
  return m_selectedCount.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyFileIndexFixedLength() {
  return m_fileIndexFixedLength.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyReferenceImageIndex() {
  return m_referenceImageIndex.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyVersion() {
  return m_version.get_proxy();
}

Glib::PropertyProxy<bool> Sequence::propertyVariableSizeImages() {
  return m_variableSizeImages.get_proxy();
}

Glib::PropertyProxy<bool> Sequence::propertyFzFlag() {
  return m_fzFlag.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyLayerCount() {
  return m_layerCount.get_proxy();
}

Glib::PropertyProxy<SequenceType> Sequence::propertySequenceType() {
  return m_sequenceType.get_proxy();
}

Glib::PropertyProxy<int> Sequence::propertyRegistrationLayer() {
  return m_registrationLayer.get_proxy();
}

bool Sequence::getVariableSizeImages() const {
  return m_variableSizeImages.get_value();
}

int Sequence::getLayerCount() const {
  return m_layerCount.get_value();
}

Glib::ustring Sequence::getSequenceName() const {
  return m_sequenceName.get_value();
}

int Sequence::getFileIndexFirst() const {
  return m_fileIndexFirst.get_value();
}

int Sequence::getImageCount() const {
  return m_imageCount.get_value();
}

int Sequence::getSelectedCount() const {
  return m_selectedCount.get_value();
}

int Sequence::getFileIndexFixedLength() const {
  return m_fileIndexFixedLength.get_value();
}

int Sequence::getReferenceImageIndex() const {
  return m_referenceImageIndex.get_value();
}

int Sequence::getVersion() const {
  return m_version.get_value();
}

bool Sequence::getFzFlag() const {
  return m_fzFlag.get_value();
}

SequenceType Sequence::getSequenceType() const {
  return m_sequenceType.get_value();
}

int Sequence::getRegistrationLayer() const {
  return m_registrationLayer.get_value();
}

Glib::RefPtr<Obj::Image> Sequence::image(int index) const {
  return m_images[index];
}

// GType Sequence::get_item_type_vfunc() {
//   return Obj::Image::get_type();
// }

// guint Sequence::get_n_items_vfunc() {
//   return m_images.size();
// }

// gpointer Sequence::get_item_vfunc(guint position) {
//   return m_images[position]->gobj();
// }

