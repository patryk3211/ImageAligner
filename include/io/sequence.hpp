#pragma once

#include <gtkmm.h>
#include "objects/image.hpp"

#include <filesystem>
#include <vector>
#include <memory>

namespace IO {

// struct ImageStats {
//   long m_totalPixels;
//   long m_goodPixels;

//   double m_mean;
//   double m_median;
//   double m_sigma;
//   double m_avgDev;
//   double m_mad;
//   double m_sqrtBWMV;
//   double m_location;
//   double m_scale;
//   double m_min;
//   double m_max;

//   double m_normValue;
//   double m_bgNoise;
// };

// struct ImageRegistration {
//   // int m_layer;

//   float m_FWHM;
//   float m_weightedFWHM;
//   float m_roundness;
//   double m_quality;
//   float m_backgroundLevel;
//   int m_numberOfStars;
//   double m_homographyMatrix[9];
// };

// struct SequenceImage {
//   int m_fileIndex;
//   int m_included;
//   int m_width;
//   int m_height;

//   std::vector<ImageStats> m_stats;
//   std::optional<ImageRegistration> m_registration;
// };

enum class SequenceType {
  MULTI_FITS = 0,
  SINGLE_FITS = 1,
};

class Sequence : public Glib::Object {
  Glib::Property<Glib::ustring> m_sequenceName;
  Glib::Property<int> m_fileIndexFirst;
  Glib::Property<int> m_imageCount;
  Glib::Property<int> m_selectedCount;
  Glib::Property<int> m_fileIndexFixedLength;
  Glib::Property<int> m_referenceImageIndex;
  Glib::Property<int> m_version;
  Glib::Property<bool> m_variableSizeImages;
  Glib::Property<bool> m_fzFlag;

  Glib::Property<int> m_layerCount;
  Glib::Property<SequenceType> m_sequenceType;

  Glib::Property<int> m_registrationLayer;
  // std::string m_sequenceName;
  // int m_startImage;
  // int m_imageCount;
  // int m_selectedCount;
  // int m_fixedLength;
  // int m_referenceImage;
  // int m_version;
  // int m_variableSize;
  // int m_flag;

  // char m_imageFormat;

  // int m_layerCount;

  // bool m_initialized;

  // int m_registrationLayer;

  std::vector<Glib::RefPtr<Obj::Image>> m_images;
  // std::vector<SequenceImage> m_images;

public:
  Sequence();
  virtual ~Sequence() = default;

  static Glib::RefPtr<Sequence> create();

  void validate();
  void prepareWrite(IO::ImageProvider& provider);

  void writeStream(std::ostream& stream);

  Glib::PropertyProxy<Glib::ustring> propertySequenceName();
  Glib::PropertyProxy<int> propertyFileIndexFirst();
  Glib::PropertyProxy<int> propertyImageCount();
  Glib::PropertyProxy<int> propertySelectedCount();
  Glib::PropertyProxy<int> propertyFileIndexFixedLength();
  Glib::PropertyProxy<int> propertyReferenceImageIndex();
  Glib::PropertyProxy<int> propertyVersion();
  Glib::PropertyProxy<bool> propertyVariableSizeImages();
  Glib::PropertyProxy<bool> propertyFzFlag();
  Glib::PropertyProxy<int> propertyLayerCount();
  Glib::PropertyProxy<SequenceType> propertySequenceType();
  Glib::PropertyProxy<int> propertyRegistrationLayer();

  Glib::ustring getSequenceName() const;
  int getFileIndexFirst() const;
  int getImageCount() const;
  int getSelectedCount() const;
  int getFileIndexFixedLength() const;
  int getReferenceImageIndex() const;
  int getVersion() const;
  bool getVariableSizeImages() const;
  bool getFzFlag() const;
  int getLayerCount() const;
  SequenceType getSequenceType() const;
  int getRegistrationLayer() const;

  // const std::string& name() const;

  // int startImage() const;
  // int imageCount() const;
  // int selectedCount() const;
  // int fixedLength() const;
  // int referenceImage() const;
  // int version() const;
  // bool variableSize() const;
  // int fzFlag() const;
  // char imageFormat() const;
  // int layerCount() const;

  // int& selectedCount();

  // const SequenceImage& image(int index) const;
  // SequenceImage& image(int index);
  Glib::RefPtr<Obj::Image> image(int index) const;

  // virtual GType get_item_type_vfunc() override;
  // virtual guint get_n_items_vfunc() override;
  // virtual gpointer get_item_vfunc(guint position) override;

  static Glib::RefPtr<Sequence> readSequence(const std::filesystem::path& file);
  static Glib::RefPtr<Sequence> readStream(std::istream& stream);
};

} // namespace IO

