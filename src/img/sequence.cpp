#include "img/sequence.hpp"

#include <fstream>
#include <iostream>

#include <spdlog/spdlog.h>

Img::Sequence::Sequence() {
  m_initialized = false;

  m_imageFormat = 0;
}

// Siril sequence reader based on https://gitlab.com/free-astro/siril/-/blob/master/src/io/seqfile.c
std::shared_ptr<Img::Sequence> Img::Sequence::readSequence(const std::filesystem::path& filepath) {
  spdlog::info("Reading sequence file '{}'", filepath.c_str());

  std::ifstream file(filepath);
  if(!file.is_open()) {
    spdlog::error("Failed to open file '{}'", filepath.c_str());
    return nullptr;
  }

  return readStream(file);
}

std::shared_ptr<Img::Sequence> Img::Sequence::readStream(std::istream& stream) {
  Img::Sequence sequence;

  char line[512];
  const char* scanfmt = 0;
  int imageIndex = 0, statIndex = 0;
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

        if(sequence.m_initialized) {
          spdlog::error("Sequence contains multiple header definitions!");
          return nullptr;
        }

        char seqName[512];

        if(sscanf(line + 2, scanfmt, seqName,
                  &sequence.m_startImage, &sequence.m_imageCount,
                  &sequence.m_selectedCount, &sequence.m_fixedLength,
                  &sequence.m_referenceImage, &sequence.m_version,
                  &sequence.m_variableSize, &sequence.m_flag) < 6) {
          spdlog::error("Sequence header error");
          return nullptr;
        }

        sequence.m_sequenceName = std::string(seqName);
        sequence.m_images.resize(sequence.m_imageCount);
        sequence.m_initialized = true;

        if(sequence.m_version <= 3) {
          spdlog::error("Sequence versions below or equal to 3 are unsupported!");
          return nullptr;
        }
        break;
      }
      case 'L':
        if(line[1] == ' ') {
          if(sscanf(line + 2, "%d", &sequence.m_layerCount) != 1) {
            spdlog::error("Sequence file format error");
            return nullptr;
          }
        }
        break;
      case 'I': {
    //     if(sequence.m_version <= 3) {
    //       SequenceImage& imgRef = sequence.m_images[imageIndex++];
				// 	int tokenCount = sscanf(line + 2, "%d %d %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
    //                          &imgRef.m_fileIndex, &imgRef.m_included,
    //                          &imgRef.m_mean, &imgRef.m_median, &imgRef.m_sigma,
    //                          &imgRef.m_avgDev, &imgRef.m_mad, &imgRef.m_sqrtBWMV,
    //                          &imgRef.m_location, &imgRef.m_scale, &imgRef.m_min,
    //                          &imgRef.m_max);
				// 	if(tokenCount != 12 || tokenCount != 2) {
    //         std::cout << "Sequence file format error" << std::endl;
    //         return nullptr;
				// 	}
				// } else {
        SequenceImage& imgRef = sequence.m_images[imageIndex++];
        int tokenCount = sscanf(line + 2, "%d %d %d,%d",
                           &imgRef.m_fileIndex, &imgRef.m_included,
                           &imgRef.m_width, &imgRef.m_height);
        if((tokenCount != 4 && sequence.m_variableSize) ||
            (tokenCount != 2 && !sequence.m_variableSize)) {
          spdlog::error("Sequence file format error");
          return nullptr;
        }
				// }
        break;
      }
      case 'T':
        if(line[1] != 'F') {
          spdlog::error("Sequence type not supported! (only FITS sequences are currently supported)");
          return nullptr;
        }
        sequence.m_imageFormat = 'F';
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
        SequenceImage& ref = sequence.m_images[image];
        if(ref.m_stats.empty())
          ref.m_stats.resize(sequence.m_layerCount);
        ImageStats& stats = ref.m_stats[layer];
        int tokenCount = sscanf(line + 4 + consumed, "%ld %ld %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg",
                                &stats.m_totalPixels, &stats.m_goodPixels,
                                &stats.m_mean, &stats.m_median,
                                &stats.m_sigma, &stats.m_avgDev,
                                &stats.m_mad, &stats.m_sqrtBWMV,
                                &stats.m_location, &stats.m_scale,
                                &stats.m_min, &stats.m_max,
                                &stats.m_normValue, &stats.m_bgNoise);
        if(tokenCount != 14) {
          spdlog::error("Malformed M line, file loading terminated.");
          return nullptr;
        }
        break;
      }
      // case 'D':
      //   std::cout << "Warning! D line is currently unsupported in sequences!" << std::endl;
      //   break;
      // case 'R':
      case 0:
        // Ignore empty line
        break;
      default:
        spdlog::warn("Unsupported line '{}' in sequence!", line[0]);
        break;
    }
  }

  return std::make_shared<Img::Sequence>(sequence);
}

void Img::Sequence::writeStream(std::ostream& stream) {
  // Start off with the header
  stream << "S '";
  stream << m_sequenceName;
  stream << "' ";
  stream << m_startImage << ' ';
  stream << m_imageCount << ' ';
  stream << m_selectedCount << ' ';
  stream << m_fixedLength << ' ';
  stream << m_referenceImage << ' ';
  stream << m_version << ' ';
  stream << m_variableSize << ' ';
  stream << m_flag << '\n';

  // Emit sequence type
  stream << 'T' << m_imageFormat << '\n';
  // Layer count
  stream << "L " << m_layerCount << '\n';
  
  // Write all images
  for(auto& img : m_images) {
    stream << "I ";
    stream << img.m_fileIndex << ' ' << img.m_included;
    if(m_variableSize) {
      stream << ' ' << img.m_width << ',' << img.m_height;
    }
    stream << '\n';
  }

  // Write all stats
  for(uint i = 0; i < m_imageCount; ++i) {
    SequenceImage& img = m_images[i];
    if(img.m_stats.empty())
      continue;

    for(uint j = 0; j < m_layerCount; ++j) {
      // Start the M line
      ImageStats& stats = img.m_stats[j];
      stream << 'M' << j << '-' << i << ' ';

      stream << stats.m_totalPixels << ' ';
      stream << stats.m_goodPixels << ' ';
      stream << stats.m_mean << ' ';
      stream << stats.m_median << ' ';
      stream << stats.m_sigma << ' ';
      stream << stats.m_avgDev << ' ';
      stream << stats.m_mad << ' ';
      stream << stats.m_sqrtBWMV << ' ';
      stream << stats.m_location << ' ';
      stream << stats.m_scale << ' ';
      stream << stats.m_min << ' ';
      stream << stats.m_max << ' ';
      stream << stats.m_normValue << ' ';
      stream << stats.m_bgNoise << '\n';
    }
  }
}

const std::string& Img::Sequence::name() const { return m_sequenceName; }
int Img::Sequence::startImage() const { return m_startImage; }
int Img::Sequence::imageCount() const { return m_imageCount; }
int Img::Sequence::selectedCount() const { return m_selectedCount; }
int Img::Sequence::fixedLength() const { return m_fixedLength; }
int Img::Sequence::referenceImage() const { return m_referenceImage; }
int Img::Sequence::version() const { return m_version; }
bool Img::Sequence::variableSize() const { return m_variableSize; }
int Img::Sequence::fzFlag() const { return m_flag; }
char Img::Sequence::imageFormat() const { return m_imageFormat; }
int Img::Sequence::layerCount() const { return m_layerCount; }

const Img::SequenceImage& Img::Sequence::image(int index) const {
  return m_images[index];
}

