#pragma once

#include <gtkmm.h>
#include "objects/image.hpp"

#include <filesystem>
#include <vector>

namespace IO {

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

  std::vector<Glib::RefPtr<Obj::Image>> m_images;

  Glib::Property<bool> m_dirty;

public:
  Sequence();
  virtual ~Sequence();

  static Glib::RefPtr<Sequence> create();

  void validate();
  void prepareWrite(IO::ImageProvider& provider);

  void writeStream(std::ostream& stream);

  void markDirty();
  void markClean();
  bool isDirty();
  Glib::PropertyProxy_ReadOnly<bool> propertyDirty();

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

  Glib::RefPtr<Obj::Image> image(int index) const;

  // virtual GType get_item_type_vfunc() override;
  // virtual guint get_n_items_vfunc() override;
  // virtual gpointer get_item_vfunc(guint position) override;

  static Glib::RefPtr<Sequence> readSequence(const std::filesystem::path& file);
  static Glib::RefPtr<Sequence> readStream(std::istream& stream);
};

} // namespace IO

