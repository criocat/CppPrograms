#pragma once
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>

template <class T>
struct matrix {
private:
  template <typename G>
  struct base_col_iterator {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = G*;
    using reference = G&;
    using iterator_category = std::random_access_iterator_tag;

  private:
    base_col_iterator(pointer current, size_t col, size_t cols) : _cur_data(current), _col(col), _cols(cols) {}

    friend matrix;

  public:
    base_col_iterator() = default;

    reference operator*() const {
      return _cur_data[_col];
    }

    operator base_col_iterator<const G>() const {
      return {_cur_data, _col, _cols};
    }

    pointer operator->() const {
      return _cur_data + _col;
    }

    base_col_iterator& operator++() {
      _cur_data += _cols;
      return *this;
    }

    base_col_iterator operator++(int) {
      base_col_iterator tmp = *this;
      ++*this;
      return tmp;
    }

    base_col_iterator& operator--() {
      _cur_data -= _cols;
      return *this;
    }

    base_col_iterator operator--(int) {
      base_col_iterator tmp = *this;
      --*this;
      return tmp;
    }

    friend base_col_iterator operator+(const difference_type left, const base_col_iterator& right) {
      return base_col_iterator(right._cur_data + static_cast<difference_type>(right._cols) * left, right._col,
                               right._cols);
    }

    friend base_col_iterator operator+(const base_col_iterator& left, const difference_type right) {
      return base_col_iterator(left._cur_data + static_cast<difference_type>(left._cols) * right, left._col,
                               left._cols);
    }

    friend base_col_iterator operator-(const base_col_iterator& left, const difference_type right) {
      return base_col_iterator(left._cur_data - static_cast<difference_type>(left._cols) * right, left._col,
                               left._cols);
    }

    friend difference_type operator-(const base_col_iterator& left, const base_col_iterator& right) {
      return (left._cur_data - right._cur_data) / static_cast<difference_type>(left._cols);
    }

    friend bool operator<(const base_col_iterator& left, const base_col_iterator& right) {
      return right - left > 0;
    }

    friend bool operator>(const base_col_iterator& left, const base_col_iterator& right) {
      return left - right > 0;
    }

    friend bool operator>=(const base_col_iterator& left, const base_col_iterator& right) {
      return !(left < right);
    }

    friend bool operator<=(const base_col_iterator& left, const base_col_iterator& right) {
      return !(left > right);
    }

    base_col_iterator& operator+=(const difference_type diff) {
      *this = *this + diff;
      return *this;
    }

    base_col_iterator& operator-=(const difference_type diff) {
      *this = *this - diff;
      return *this;
    }

    reference operator[](const difference_type pos) const {
      return _cur_data[pos * static_cast<difference_type>(_cols) + static_cast<difference_type>(_col)];
    }

    friend bool operator==(const base_col_iterator& lhs, const base_col_iterator& rhs) {
      return lhs._cur_data == rhs._cur_data;
    }

    friend bool operator!=(const base_col_iterator& lhs, const base_col_iterator& rhs) {
      return !(lhs == rhs);
    }

  private:
    pointer _cur_data;
    size_t _col;
    size_t _cols;
  };

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

  using row_iterator = pointer;
  using const_row_iterator = const_pointer;

  using col_iterator = base_col_iterator<value_type>;
  using const_col_iterator = base_col_iterator<const value_type>;

private:
  size_t _cols;
  size_t _rows;
  pointer _data;

public:
  matrix() : _cols(0), _rows(0), _data(nullptr) {}

  matrix(const size_t rows, const size_t cols) {
    if (rows * cols != 0) {
      _rows = rows;
      _cols = cols;
      _data = new value_type[rows * cols]();
    } else {
      _rows = 0;
      _cols = 0;
      _data = nullptr;
    }
  }

  template <size_t Rows, size_t Cols>
  matrix(const value_type (&arr)[Rows][Cols]) : matrix(Rows, Cols) {
    for (size_t row = 0; row < Rows; ++row) {
      std::copy_n(arr[row], Cols, _data + row * Cols);
    }
  }

  matrix(const matrix& other) : matrix(other._rows, other._cols) {
    std::copy(other.data(), other.data() + size(), data());
  }

  ~matrix() {
    delete[] _data;
  }

  matrix& operator=(const matrix& other) {
    if (this == &other) {
      return *this;
    }
    matrix copy(other);
    swap(*this, copy);
    return *this;
  }

  friend void swap(matrix& left, matrix& right) {
    std::swap(left._cols, right._cols);
    std::swap(left._rows, right._rows);
    std::swap(left._data, right._data);
  }

  size_t rows() const {
    return _rows;
  }

  size_t cols() const {
    return _cols;
  }

  size_t size() const {
    return cols() * rows();
  }

  bool empty() const {
    return cols() == 0 && rows() == 0;
  }

  reference operator()(const size_t row, const size_t col) {
    return _data[row * cols() + col];
  }

  const_reference operator()(const size_t row, const size_t col) const {
    return _data[row * cols() + col];
  }

  pointer data() {
    return _data;
  }

  const_pointer data() const {
    return _data;
  }

  friend bool operator==(const matrix& left, const matrix& right) {
    return left.cols() == right.cols() && left.rows() == right.rows() &&
           std::equal(left.data(), left.data() + left.size(), right.data());
  }

  friend bool operator!=(const matrix& left, const matrix& right) {
    return !(left == right);
  }

  matrix& operator+=(const matrix& other) {
    std::transform(begin(), end(), other.begin(), begin(), std::plus());
    return *this;
  }

  matrix& operator-=(const matrix& other) {
    std::transform(begin(), end(), other.begin(), begin(), std::minus());
    return *this;
  }

  matrix& operator*=(const matrix& other) {
    matrix res = *this * other;
    swap(*this, res);
    return *this;
  }

  matrix& operator*=(const_reference factor) {
    std::transform(begin(), end(), begin(), [&factor](value_type val) { return val * factor; });
    return *this;
  }

  friend matrix operator+(const matrix& left, const matrix& right) {
    return matrix(left) += right;
  }

  friend matrix operator-(const matrix& left, const matrix& right) {
    return matrix(left) -= right;
  }

  friend matrix operator*(const matrix& left, const matrix& right) {
    matrix res(left.rows(), right.cols());
    for (size_t row = 0; row < left.rows(); ++row) {
      for (size_t col = 0; col < right.cols(); ++col) {
        res(row, col) = std::inner_product(left.row_begin(row), left.row_end(row), right.col_begin(col), value_type());
      }
    }
    return res;
  }

  friend matrix operator*(const_reference factor, const matrix& right) {
    return matrix(right) *= factor;
  }

  friend matrix operator*(const matrix& left, const_reference factor) {
    return matrix(left) *= factor;
  }

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  iterator end() {
    return begin() + size();
  }

  const_iterator end() const {
    return begin() + size();
  }

  row_iterator row_begin(const size_t row) {
    return data() + row * cols();
  }

  const_row_iterator row_begin(const size_t row) const {
    return data() + row * cols();
  }

  row_iterator row_end(const size_t row) {
    return row_begin(row) + cols();
  }

  const_row_iterator row_end(const size_t row) const {
    return row_begin(row) + cols();
  }

  col_iterator col_begin(const size_t col) {
    return col_iterator(begin(), col, cols());
  }

  const_col_iterator col_begin(const size_t col) const {
    return const_col_iterator(begin(), col, cols());
  }

  col_iterator col_end(const size_t col) {
    return col_begin(col) + rows();
  }

  const_col_iterator col_end(const size_t col) const {
    return col_begin(col) + rows();
  }
};
