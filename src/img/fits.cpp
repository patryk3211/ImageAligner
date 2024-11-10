#include "img/fits.hpp"

#include <cassert>

#include <spdlog/spdlog.h>

#define GUARD() ErrorGuard _guard(*this)

using namespace Img;

static int fitsDataType(const DataType::EnumType& type) {
  switch(type) {
    case DataType::BYTE:
      return TSBYTE;
    case DataType::UBYTE:
      return TBYTE;
    case DataType::SHORT:
      return TSHORT;
    case DataType::USHORT:
      return TUSHORT;
    case DataType::INT:
      return TINT;
    case DataType::UINT:
      return TUINT;
    case DataType::LONG:
      return TLONG;
    case DataType::ULONG:
      return TULONG;
    case DataType::FLOAT:
      return TFLOAT;
    case DataType::DOUBLE:
      return TDOUBLE;
  }
}

Fits::ErrorGuard::ErrorGuard(Fits& parent)
  : m_parent(parent) {
  assert(m_parent.m_status == 0);
  m_parent.m_status = 0;
}

Fits::ErrorGuard::~ErrorGuard() {
  if(m_parent.m_status) {
    fits_report_error(stderr, m_parent.m_status);
    m_parent.m_status = 0;
  }
}

Fits::Fits(const std::filesystem::path& filename)
  : m_status(0) {
  GUARD();

  fits_open_file(&m_fileptr, filename.c_str(), READONLY, &m_status);

  fits_get_num_hdus(m_fileptr, &m_imageCount, &m_status);

  if(m_status) {
    // An error has occurred
    m_imageCount = -1;
    spdlog::error("Failed to open FITS file {}", filename.c_str());
  } else {
    spdlog::info("Opened FITS file {} with {} HDUs", filename.c_str(), m_imageCount);
  }

  // int hdupos, nkeys;
  // char card[FLEN_CARD];
  // fits_get_hdu_num(m_fileptr, &hdupos);

  // for (; !m_status; hdupos++)  /* Main loop through each extension */
  // {
  //   fits_get_hdrspace(m_fileptr, &nkeys, NULL, &m_status); /* get # of keywords */

  //   printf("Header listing for HDU #%d:\n", hdupos);

  //   for (int i = 1; i <= nkeys; i++) { /* Read and print each keywords */

  //      if (fits_read_record(m_fileptr, i, card, &m_status))break;
  //     // fits_get
  //      printf("%s\n", card);
  //   }
  //   printf("END\n\n");  /* terminate listing with END */

  //   fits_movrel_hdu(m_fileptr, 1, NULL, &m_status);  /* try to move to next HDU */
  //   break;
  // }

  // if (m_status == END_OF_FILE)  m_status = 0; /* Reset after normal error */

  // if (m_status) fits_report_error(stderr, m_status); /* print any error message */
}

Fits::Fits(Fits&& other)
  : m_fileptr(other.m_fileptr)
  , m_status(other.m_status) {
  m_imageCount = other.m_imageCount;
  other.m_fileptr = nullptr;
}

Fits::~Fits() {
  if(m_fileptr) {
    GUARD();

    fits_close_file(m_fileptr, &m_status);
  }
}

void Fits::select(int index) {
  GUARD();

  fits_movabs_hdu(m_fileptr, index, NULL, &m_status);
}

int Fits::imageType() {
  GUARD();

  int type = 0;
  fits_get_img_type(m_fileptr, &type, &m_status);
  return type;
}

int Fits::imageDimensionCount() {
  GUARD();

  int count = 0;
  fits_get_img_dim(m_fileptr, &count, &m_status);
  return count;
}

void Fits::imageSize(int dimCount, long *dimensions) {
  GUARD();

  fits_get_img_size(m_fileptr, dimCount, dimensions, &m_status);
}

DataParameters Fits::getImageParameters(int index) {
  GUARD();
  select(index + 1);
  
  // int hdupos, nkeys;
  // char card[FLEN_CARD];
  // fits_get_hdu_num(m_fileptr, &hdupos);
  // fits_get_hdrspace(m_fileptr, &nkeys, NULL, &m_status); /* get # of keywords */
  // printf("Header listing for HDU #%d:\n", hdupos);
  // for (int i = 1; i <= nkeys; i++) { /* Read and print each keywords */
  //    if (fits_read_record(m_fileptr, i, card, &m_status))break;
  //    printf("%s\n", card);
  // }
  // printf("END\n\n");  /* terminate listing with END */


  int dimCount = imageDimensionCount();
  long dims[dimCount];
  imageSize(dimCount, dims);

  int equivType;
  fits_get_img_equivtype(m_fileptr, &equivType, &m_status);

  DataType::EnumType type;
  switch(equivType) {
    case BYTE_IMG: type = DataType::UBYTE; break;
    case SHORT_IMG: type = DataType::SHORT; break;
    case USHORT_IMG: type = DataType::USHORT; break;
    case LONG_IMG: type = DataType::INT; break;
    case LONGLONG_IMG: type = DataType::LONG; break;
    case FLOAT_IMG: type = DataType::FLOAT; break;
    case DOUBLE_IMG: type = DataType::DOUBLE; break;
    default:
      spdlog::error("Unknown equivType received from FITSIO: {}", equivType);
      assert(false);
      // return DataParameters(index);
  }

  if(m_status == 0) {
    return DataParameters(index, type, dimCount, dims);
  } else {
    // Invalid parameters
    return DataParameters(index);
  }
}

std::shared_ptr<uint8_t[]> Fits::getPixels(const DataParameters& params) {
  if(!params)
    return nullptr;

  GUARD();

  select(params.index() + 1);
  int dimCount = imageDimensionCount();
  if(dimCount != params.dimCount())
    return nullptr;

  long start[dimCount], end[dimCount], inc[dimCount];
  size_t pixelCount = 1;
  for(int i = 0; i < dimCount; ++i) {
    start[i] = params.start()[i];
    end[i] = params.end()[i];
    inc[i] = params.inc()[i];

    size_t length = (end[i] - start[i] + 1) / inc[i];
    pixelCount *= length;
  }

  std::shared_ptr<uint8_t[]> data(new uint8_t[pixelCount * DataType::dataSize(params.type())]);
  fits_read_subset(m_fileptr, fitsDataType(params.type()), start, end, inc, nullptr, data.get(), nullptr, &m_status);

  if(m_status != 0)
    return nullptr;

  return data;
}

