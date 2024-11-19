#include "objects/matrix.hpp"

#include <spdlog/spdlog.h>

using namespace Obj;

HomographyMatrix::HomographyMatrix()
  : ObjectBase("HomographyMatrix")
  , m_h00(*this, "h00", 0)
  , m_h01(*this, "h01", 0)
  , m_h02(*this, "h02", 0)
  , m_h10(*this, "h10", 0)
  , m_h11(*this, "h11", 0)
  , m_h12(*this, "h12", 0)
  , m_h20(*this, "h20", 0)
  , m_h21(*this, "h21", 0)
  , m_h22(*this, "h22", 0) {
  m_accessArray[0] = &m_h00;
  m_accessArray[1] = &m_h01;
  m_accessArray[2] = &m_h02;
  m_accessArray[3] = &m_h10;
  m_accessArray[4] = &m_h11;
  m_accessArray[5] = &m_h12;
  m_accessArray[6] = &m_h20;
  m_accessArray[7] = &m_h21;
  m_accessArray[8] = &m_h22;
}

void HomographyMatrix::reset() {
  for(int i = 0; i < 9; ++i)
    set(i, 0);
  set(0, 1);
  set(4, 1);
  set(8, 1);
}

cv::Mat HomographyMatrix::read() const {
  cv::Mat mat;
  mat.create(3, 3, CV_64F);
  for(int i = 0; i < 9; ++i)
    mat.at<double>(i) = get(i);
  return mat;
}

void HomographyMatrix::write(const cv::Mat& matrix) {
  if(matrix.depth() != CV_64F) {
    spdlog::error("Invalid matrix depth in HomographyMatrix::write, got {}", matrix.depth());
    return;
  }
    
  for(int i = 0; i < 9; ++i)
    set(i, matrix.at<double>(i));
}

Glib::PropertyProxy<double> HomographyMatrix::property(int i) {
  return m_accessArray[i]->get_proxy();
}

Glib::PropertyProxy<double> HomographyMatrix::property(int col, int row) {
  return m_accessArray[col + row * 3]->get_proxy();
}

Glib::PropertyProxy_ReadOnly<double> HomographyMatrix::property(int i) const {
  return const_cast<const Glib::Property<double>*>(m_accessArray[i])->get_proxy();
}

Glib::PropertyProxy_ReadOnly<double> HomographyMatrix::property(int col, int row) const {
  return const_cast<const Glib::Property<double>*>(m_accessArray[col + row * 3])->get_proxy();
}

double HomographyMatrix::get(int i) const {
  return m_accessArray[i]->get_value();
}

double HomographyMatrix::get(int col, int row) const {
  return m_accessArray[col + row * 3]->get_value();
}

void HomographyMatrix::set(int i, double val) {
  m_accessArray[i]->set_value(val);
}

void HomographyMatrix::set(int col, int row, double val) {
  m_accessArray[col + row * 3]->set_value(val);
}

