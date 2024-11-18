#pragma once

#include <unordered_map>
#include <memory>
#include <cstdint>
#include <list>

#include <opencv2/core/mat.hpp>

namespace IO {

struct DataType {
  enum EnumType {
    BYTE,
    UBYTE,
    SHORT,
    USHORT,
    INT,
    UINT,
    LONG,
    ULONG,
    FLOAT,
    DOUBLE
  };

  static size_t dataSize(const EnumType& type);

  static size_t typeMin(const EnumType& type);
  static size_t typeMax(const EnumType& type);
};


class DataParameters {
  DataType::EnumType m_type;
  int m_index;
  int m_dimCount;

  long *m_start;
  long *m_end;
  long *m_inc;

public:
  DataParameters(int index);
  DataParameters(int index, DataType::EnumType type, int dimCount, long *end);
  DataParameters(int index, DataType::EnumType type, int dimCount, long *start, long *end, long *inc = 0);
  DataParameters(const DataParameters& other);
  DataParameters(DataParameters&& other);

  ~DataParameters();

  operator bool() const;
  bool operator!() const;

  bool operator==(const DataParameters& other) const;

  DataType::EnumType type() const;
  int index() const;
  int dimCount() const;

  const long *start() const;
  const long *end() const;
  const long *inc() const;

  long *start();
  long *end();
  long *inc();

  long width() const;
  long height() const;
  long layerCount() const;

  void setDimension(int dim, long start = -1, long end = -1, long inc = -1);

  friend struct DataParamHash;
};

struct DataParamHash {
  size_t operator()(const IO::DataParameters& obj) const;
};

class ImageProvider {

protected:
  int m_imageCount;

public:
  ImageProvider();
  virtual ~ImageProvider() = default;

  int imageCount();

  cv::Mat getImageMatrix(int index);

  virtual std::shared_ptr<uint8_t[]> getPixels(const DataParameters& params);

  virtual double maxTypeValue() = 0;
  virtual DataParameters getImageParameters(int index) = 0;
  virtual bool readPixels(const DataParameters& params, void *ptr) = 0;
};

class CachedImageProvider : public ImageProvider {
  ImageProvider *m_provider;

  std::unordered_map<DataParameters, std::shared_ptr<uint8_t[]>, DataParamHash> m_cache;
  std::list<const DataParameters*> m_loadOrder;
  size_t m_cacheSize;

public:
  CachedImageProvider(ImageProvider* provider, size_t maxCachedEntries);
  virtual ~CachedImageProvider();

  virtual DataParameters getImageParameters(int index) override;
  virtual std::shared_ptr<uint8_t[]> getPixels(const DataParameters& params) override;

  virtual double maxTypeValue() override;
private:
  void cleanupCache();
};

} // namespace IO

