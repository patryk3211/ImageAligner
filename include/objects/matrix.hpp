#pragma once

#include <gtkmm.h>

namespace Obj {

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

  template<typename T>
  static void identity(T *data) {
    memset(data, 0, sizeof(T) * 9);
    data[0] = 1;
    data[4] = 1;
    data[8] = 1;
  }

  // Changing ordering to COLUMN_MAJOR effectively transposes the matrix
  template<typename T>
  void read(T *data, OrderEnum ordering = ROW_MAJOR) const {
    static_assert(std::is_convertible_v<double, T>);

    for(uint i = 0; i < 9; ++i) {
      if(ordering == ROW_MAJOR)
        data[i] = static_cast<T>(get(i));
      else {
        uint col = i % 3;
        uint row = i / 3;
        data[row + col * 3] = static_cast<T>(get(i));
      }
    }
  }

  template<typename T>
  void write(const T *data, OrderEnum ordering = ROW_MAJOR) {
    static_assert(std::is_convertible_v<T, double>);

    for(uint i = 0; i < 9; ++i) {
      if(ordering == ROW_MAJOR)
        set(i, static_cast<double>(data[i]));
      else {
        uint col = i % 3;
        uint row = i / 3;
        set(i, static_cast<double>(data[row + col * 3]));
      }
    }
  }
};

} // namespace Obj
