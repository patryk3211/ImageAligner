#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include <memory>

namespace Img {

struct ImageStats {
  long m_totalPixels;
  long m_goodPixels;

  double m_mean;
  double m_median;
  double m_sigma;
  double m_avgDev;
  double m_mad;
  double m_sqrtBWMV;
  double m_location;
  double m_scale;
  double m_min;
  double m_max;

  double m_normValue;
  double m_bgNoise;
};

struct ImageRegistration {
  long m_transX;
  long m_transY;
};

struct SequenceImage {
  int m_fileIndex;
  int m_included;
  int m_width;
  int m_height;

  std::vector<ImageStats> m_stats;
  std::optional<ImageRegistration> m_registration;
};

class Sequence {
  std::string m_sequenceName;
  int m_startImage;
  int m_imageCount;
  int m_selectedCount;
  int m_fixedLength;
  int m_referenceImage;
  int m_version;
  int m_variableSize;
  int m_flag;

  char m_imageFormat;

  int m_layerCount;

  bool m_initialized;

  std::vector<SequenceImage> m_images;

  Sequence();

public:
  ~Sequence() = default;

  void writeStream(std::ostream& stream);

  const std::string& name() const;

  int startImage() const;
  int imageCount() const;
  int selectedCount() const;
  int fixedLength() const;
  int referenceImage() const;
  int version() const;
  bool variableSize() const;
  int fzFlag() const;
  char imageFormat() const;
  int layerCount() const;

  const SequenceImage& image(int index) const;

  static std::shared_ptr<Sequence> readSequence(const std::filesystem::path& file);
  static std::shared_ptr<Sequence> readStream(std::istream& stream);
};

} // namespace Img

