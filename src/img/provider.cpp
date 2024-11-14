#include "img/provider.hpp"

#include <spdlog/spdlog.h>

using namespace Img;

size_t DataType::dataSize(const EnumType& type) {
  switch(type) {
    case BYTE:
    case UBYTE:
      return 1;
    case SHORT:
    case USHORT:
      return 2;
    case INT:
    case UINT:
    case FLOAT:
      return 4;
    case LONG:
    case ULONG:
    case DOUBLE:
      return 8;
  }
}

size_t DataType::typeMin(const EnumType& type) {
  switch(type) {
    case UBYTE:
    case USHORT:
    case UINT:
    case ULONG:
      return 0;
    default:
      spdlog::error("Type {} not defined in typeMin function", (int)type);
      return 0;
  }
}

size_t DataType::typeMax(const EnumType& type) {
  switch(type) {
    case UBYTE:
      return (1 << 8) - 1;
    case USHORT:
      return (1 << 16) - 1;
    case UINT:
      return UINT_MAX;
    case ULONG:
      return ULONG_MAX;
    default:
      spdlog::error("Type {} not defined in typeMax function", (int)type);
      return 0;
  }
}

DataParameters::DataParameters(int index)
  : m_type(DataType::BYTE)
  , m_index(index)
  , m_dimCount(-1)
  , m_start(0)
  , m_end(0)
  , m_inc(0) {

}

DataParameters::DataParameters(int index, DataType::EnumType type, int dimCount, long *end)
  : m_type(type)
  , m_index(index)
  , m_dimCount(dimCount) {
  m_start = new long[dimCount];
  m_end = new long[dimCount];
  m_inc = new long[dimCount];

  for(int i = 0; i < dimCount; ++i) {
    m_end[i] = end[i];
    m_start[i] = 1;
    m_inc[i] = 1;
  }
}

DataParameters::DataParameters(int index, DataType::EnumType type, int dimCount, long *start, long *end, long *inc)
  : m_type(type)
  , m_index(index)
  , m_dimCount(dimCount) {
  m_start = new long[dimCount];
  m_end = new long[dimCount];
  m_inc = new long[dimCount];

  for(int i = 0; i < dimCount; ++i) {
    m_start[i] = start[i];
    m_end[i] = end[i];

    if(inc != 0) m_inc[i] = inc[i];
    else m_inc[i] = 1;
  }
}

DataParameters::DataParameters(const DataParameters& other)
  : m_type(other.m_type)
  , m_index(other.m_index)
  , m_dimCount(other.m_dimCount) {
  m_start = new long[m_dimCount];
  m_end = new long[m_dimCount];
  m_inc = new long[m_dimCount];

  for(int i = 0; i < m_dimCount; ++i) {
    m_start[i] = other.m_start[i];
    m_end[i] = other.m_end[i];
    m_inc[i] = other.m_inc[i];
  }
}

DataParameters::DataParameters(DataParameters&& other)
  : m_type(other.m_type)
  , m_index(other.m_index)
  , m_dimCount(other.m_dimCount) {
  m_start = other.m_start;
  m_end = other.m_end;
  m_inc = other.m_inc;

  other.m_start = 0;
  other.m_end = 0;
  other.m_inc = 0;
}

DataParameters::~DataParameters() {
  delete[] m_start;
  delete[] m_end;
  delete[] m_inc;
}

DataParameters::operator bool() const {
  return !!*this;
}

bool DataParameters::operator!() const {
  return !(m_dimCount > 0);
}

bool DataParameters::operator==(const DataParameters& other) const {
  if(m_type != other.m_type || m_dimCount != other.m_dimCount || m_index != other.m_index)
    return false;

  for(int i = 0; i < m_dimCount; ++i) {
    if(m_start[i] != other.m_start[i] || m_end[i] != other.m_end[i] || m_inc[i] != other.m_inc[i])
      return false;
  }

  return true;
}

size_t DataParamHash::operator()(const Img::DataParameters& obj) const {
  std::hash<long> hasher;

  size_t h = obj.m_type + obj.m_dimCount * 100 + obj.m_index * 1000;
  for(int i = 0; i < obj.m_dimCount; ++i) {
    h ^= hasher(obj.m_start[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(obj.m_end[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= hasher(obj.m_inc[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
  }

  return h;
}

DataType::EnumType DataParameters::type() const {
  return m_type;
}

int DataParameters::index() const {
  return m_index;
}

int DataParameters::dimCount() const {
  return m_dimCount;
}

const long *DataParameters::start() const {
  return m_start;
}

const long *DataParameters::end() const {
  return m_end;
}

const long *DataParameters::inc() const {
  return m_inc;
}

long *DataParameters::start() {
  return m_start;
}

long *DataParameters::end() {
  return m_end;
}

long *DataParameters::inc() {
  return m_inc;
}

long DataParameters::width() const {
  return m_dimCount >= 1 ? (m_end[0] - m_start[0] + 1) / m_inc[0] : 0;
}

long DataParameters::height() const {
  return m_dimCount >= 2 ? (m_end[1] - m_start[1] + 1) / m_inc[1] : 0;
}

long DataParameters::layerCount() const {
  return m_dimCount >= 3 ? (m_end[2] - m_start[2] + 1) / m_inc[2] : 0;
}

void DataParameters::setDimension(int dim, long start, long end, long inc) {
  if(dim < m_dimCount) {
    if(start > 0) m_start[dim] = start;
    if(end > 0) m_end[dim] = end;
    if(inc > 0) m_inc[dim] = inc;
  }
}

ImageProvider::ImageProvider()
  : m_imageCount(0) {

}

int ImageProvider::imageCount() {
  return m_imageCount;
}

CachedImageProvider::CachedImageProvider(ImageProvider* provider, size_t maxCachedEntries)
  : m_provider(provider)
  , m_cacheSize(maxCachedEntries) {

}

CachedImageProvider::~CachedImageProvider() {
  delete m_provider;
}

DataParameters CachedImageProvider::getImageParameters(int index) {
  return m_provider->getImageParameters(index);
}

std::shared_ptr<uint8_t[]> CachedImageProvider::getPixels(const DataParameters& params) {
  if(!params)
    return nullptr;

  auto iter = m_cache.find(params);
  if(iter == m_cache.end()) {
    auto data = m_provider->getPixels(params);
    if(!data)
      return nullptr;

    auto cacheEntry = m_cache.insert({ params, data });
    m_loadOrder.push_back(&cacheEntry.first->first);

    if(m_loadOrder.size() > m_cacheSize) {
      cleanupCache();
    }
    return data;
  }
  return iter->second;
}

void CachedImageProvider::cleanupCache() {
  // Try to remove oldest unused entry from cache
  auto iter = m_loadOrder.begin();
  while(iter != m_loadOrder.end() && m_loadOrder.size() > m_cacheSize) {
    auto& params = **iter;
    auto ptr = m_cache[params];
    if(ptr.use_count() > 1) {
      // Use count = 1 means that only the cache is referencing this pointer
      ++iter;
      continue;
    }
    iter = m_loadOrder.erase(iter);
    m_cache.erase(params);
  }
}

