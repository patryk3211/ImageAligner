#pragma once

#include <fitsio.h>

#include <string>

namespace Img {

class Fits {
  fitsfile *m_fileptr;
  int m_status;

  int m_hduCount;

  class ErrorGuard {
    Fits& m_parent;

  public:
    ErrorGuard(Fits& parent);
    ~ErrorGuard();
  };

public:
  Fits(const std::string& filename);
  Fits(Fits&& other);

  // no copy constructor
  Fits(const Fits& other) = delete;

  ~Fits();

  int hduCount() const;

  void select(int index);

  int imageType();
  int imageDimensionCount();
  void imageSize(int dimCount, long *dimensions);

  void readPixelRect(int type, void *data, long *start, long length);
  void readPixelRect(int type, void *data, long *start, long *end, long *inc = 0);
};

}
