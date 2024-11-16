#include "objects/matrix.hpp"

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

