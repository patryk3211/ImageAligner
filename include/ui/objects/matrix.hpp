#pragma once

#include <gtkmm.h>

namespace UI {

class HomographyMatrix : public Glib::Object {
  // Property name = h[row][column]
  // Data inside of this class is stored in a row major order
  Glib::Property<double> m_h00;
  Glib::Property<double> m_h01;
  Glib::Property<double> m_h02;
  Glib::Property<double> m_h10;
  Glib::Property<double> m_h11;
  Glib::Property<double> m_h12;
  Glib::Property<double> m_h20;
  Glib::Property<double> m_h21;
  Glib::Property<double> m_h22;

  Glib::Property<double>* m_accessArray[9];

public:
  enum OrderEnum {
    ROW_MAJOR,
    COLUMN_MAJOR
  };

public:
  HomographyMatrix();
  virtual ~HomographyMatrix() = default;

  Glib::PropertyProxy<double> property(int i);
  Glib::PropertyProxy<double> property(int col, int row);
  Glib::PropertyProxy_ReadOnly<double> property(int i) const;
  Glib::PropertyProxy_ReadOnly<double> property(int col, int row) const;

  double get(int i) const;
  double get(int col, int row) const;

  void set(int i, double val);
  void set(int col, int row, double val);

  // Changing ordering to COLUMN_MAJOR effectively transposes the matrix
  void read(double *data, OrderEnum ordering = ROW_MAJOR) const;
  void write(const double *data, OrderEnum ordering = ROW_MAJOR);
};

}
