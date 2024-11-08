#include "img/fits.hpp"
#include <cassert>
#include <iostream>

#define GUARD() ErrorGuard _guard(*this)

Img::Fits::ErrorGuard::ErrorGuard(Fits& parent)
  : m_parent(parent) {
  assert(m_parent.m_status == 0);
  m_parent.m_status = 0;
}

Img::Fits::ErrorGuard::~ErrorGuard() {
  if(m_parent.m_status) {
    fits_report_error(stderr, m_parent.m_status);
    m_parent.m_status = 0;
  }
}

Img::Fits::Fits(const std::string& filename)
  : m_status(0) {
  GUARD();

  fits_open_file(&m_fileptr, filename.c_str(), READONLY, &m_status);

  fits_get_num_hdus(m_fileptr, &m_hduCount, &m_status);

  if(m_status) {
    // An error has occurred
    m_hduCount = -1;
    std::cerr << "Failed to open FITS file '" << filename << "'" << std::endl;
  } else {
    std::cout << "Opened FITS file ('" << filename << "') with " << m_hduCount << " HDUs" << std::endl;
  }

  int hdupos, nkeys;
  char card[FLEN_CARD];
  fits_get_hdu_num(m_fileptr, &hdupos);

  for (; !m_status; hdupos++)  /* Main loop through each extension */
  {
    fits_get_hdrspace(m_fileptr, &nkeys, NULL, &m_status); /* get # of keywords */

    printf("Header listing for HDU #%d:\n", hdupos);

    for (int i = 1; i <= nkeys; i++) { /* Read and print each keywords */

       if (fits_read_record(m_fileptr, i, card, &m_status))break;
      // fits_get
       printf("%s\n", card);
    }
    printf("END\n\n");  /* terminate listing with END */

    fits_movrel_hdu(m_fileptr, 1, NULL, &m_status);  /* try to move to next HDU */
    break;
  }

  if (m_status == END_OF_FILE)  m_status = 0; /* Reset after normal error */

  if (m_status) fits_report_error(stderr, m_status); /* print any error message */
}

Img::Fits::Fits(Fits&& other)
  : m_fileptr(other.m_fileptr)
  , m_status(other.m_status)
  , m_hduCount(other.m_hduCount) {
  other.m_fileptr = nullptr;
}

Img::Fits::~Fits() {
  if(m_fileptr) {
    GUARD();

    fits_close_file(m_fileptr, &m_status);
  }
}

int Img::Fits::hduCount() const {
  return m_hduCount;
}

void Img::Fits::select(int index) {
  GUARD();

  fits_movabs_hdu(m_fileptr, index, NULL, &m_status);
}

int Img::Fits::imageType() {
  GUARD();

  int type = 0;
  fits_get_img_type(m_fileptr, &type, &m_status);
  return type;
}

int Img::Fits::imageDimensionCount() {
  GUARD();

  int count = 0;
  fits_get_img_dim(m_fileptr, &count, &m_status);
  return count;
}

void Img::Fits::imageSize(int dimCount, long *dimensions) {
  GUARD();

  fits_get_img_size(m_fileptr, dimCount, dimensions, &m_status);
}

void Img::Fits::readPixelRect(int type, void *data, long *start, long length) {
  GUARD();

  fits_read_pix(m_fileptr, type, start, length, nullptr, data, nullptr, &m_status);  
}

void Img::Fits::readPixelRect(int type, void *data, long *start, long *end, long *inc) {
  GUARD();

  if(inc == 0) {
    int count = imageDimensionCount();
    long incDef[count];
    for(int i = 0; i < count; ++i)
      incDef[i] = 1;
    fits_read_subset(m_fileptr, type, start, end, incDef, nullptr, data, nullptr, &m_status);
    return;
  }

  fits_read_subset(m_fileptr, type, start, end, inc, nullptr, data, nullptr, &m_status);
}

