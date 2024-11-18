#pragma once

#include "io/provider.hpp"

#include <filesystem>
#include <fitsio.h>

namespace IO {

class Fits : public ImageProvider {
  fitsfile *m_fileptr;
  int m_status;

  class ErrorGuard {
    Fits& m_parent;

  public:
    ErrorGuard(Fits& parent);
    ~ErrorGuard();
  };

public:
  Fits(const std::filesystem::path& filename);
  Fits(Fits&& other);

  // no copy constructor
  Fits(const Fits& other) = delete;

  virtual ~Fits();

  void select(int index);

  int imageType();
  int imageDimensionCount();
  void imageSize(int dimCount, long *dimensions);

  virtual DataParameters getImageParameters(int index) override;
  // virtual std::shared_ptr<uint8_t[]> getPixels(const DataParameters& params) override;
  virtual bool readPixels(const DataParameters& params, void *ptr) override;
  virtual double maxTypeValue() override;
};

} // namespace IO

