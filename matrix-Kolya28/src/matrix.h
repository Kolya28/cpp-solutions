#pragma once

#include <algorithm>
#include <cassert>
#include <compare>
#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>

template <class T>
class matrix {
private:
  template <class U>
  class basic_col_iterator {
    friend matrix;

  public:
    using value_type = T;
    using reference = U&;
    using pointer = U*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

  private:
    pointer _data;
    size_t _col_index;
    size_t _cols;

    basic_col_iterator(pointer data, size_t col_index, size_t cols) : _data(data), _col_index(col_index), _cols(cols) {}

  public:
    basic_col_iterator() = default;

    operator basic_col_iterator<const U>() const {
      return basic_col_iterator<const U>(_data, _col_index, _cols);
    }

    basic_col_iterator& operator++() {
      _data += _cols;
      return *this;
    }

    basic_col_iterator& operator--() {
      _data -= _cols;
      return *this;
    }

    basic_col_iterator operator++(int) {
      basic_col_iterator copy(*this);
      ++*this;
      return copy;
    }

    basic_col_iterator operator--(int) {
      basic_col_iterator copy(*this);
      --*this;
      return copy;
    }

    basic_col_iterator operator+(difference_type diff) const {
      return basic_col_iterator(*this) += diff;
    }

    friend basic_col_iterator operator+(difference_type diff, const basic_col_iterator& it) {
      return it + diff;
    }

    basic_col_iterator operator-(difference_type diff) const {
      return basic_col_iterator(*this) -= diff;
    }

    friend difference_type operator-(const basic_col_iterator& left, const basic_col_iterator& right) {
      assert(left._cols == right._cols && left._col_index == right._col_index);
      return (left._data - right._data) / static_cast<difference_type>(left._cols);
    }

    basic_col_iterator& operator+=(difference_type diff) {
      _data += static_cast<difference_type>(_cols) * diff;
      return *this;
    }

    basic_col_iterator& operator-=(difference_type diff) {
      _data -= static_cast<difference_type>(_cols) * diff;
      return *this;
    }

    reference operator*() const {
      return _data[_col_index];
    }

    pointer operator->() const {
      return _data + _col_index;
    }

    reference operator[](difference_type offset) const {
      offset *= _cols;
      return (_data + offset)[_col_index];
    }

    friend auto operator<=>(const basic_col_iterator& left, const basic_col_iterator& right) {
      assert(left._col_index == right._col_index && left._cols == right._cols);
      return left._data <=> right._data;
    }

    friend bool operator==(const basic_col_iterator& left, const basic_col_iterator& right) {
      return std::is_eq(left <=> right);
    }

    friend bool operator!=(const basic_col_iterator& left, const basic_col_iterator& right) = default;
  };

public:
  using value_type = T;

  using reference = value_type&;
  using const_reference = const value_type&;

  using pointer = value_type*;
  using const_pointer = const value_type*;

  using iterator = pointer;
  using const_iterator = const_pointer;

  using row_iterator = pointer;
  using const_row_iterator = const_pointer;

  using col_iterator = basic_col_iterator<value_type>;
  using const_col_iterator = basic_col_iterator<const value_type>;

private:
  value_type* _data;
  size_t _rows;
  size_t _cols;

public:
  matrix() : _data(nullptr), _rows(0), _cols(0) {}

  matrix(size_t rows, size_t cols) : _rows(rows), _cols(cols) {
    size_t s = size();
    if (s == 0) {
      _data = nullptr;
      _rows = _cols = 0;
    } else {
      _data = new value_type[s]{};
    }
  }

  template <size_t Rows, size_t Cols>
  matrix(const value_type (&init)[Rows][Cols]) : _data(new value_type[Rows * Cols]),
                                                 _rows(Rows),
                                                 _cols(Cols) {
    // array of size 0 doesn't exist
    iterator it = begin();
    for (const value_type(&row)[Cols] : init) {
      it = std::copy_n(row, cols(), it);
    }
  }

  matrix(const matrix& other) : _rows(other.rows()), _cols(other.cols()) {
    if (other.empty()) {
      _data = nullptr;
    } else {
      _data = new value_type[size()];
      std::copy(other.begin(), other.end(), begin());
    }
  }

  matrix(matrix&& other)
      : _data(std::exchange(other._data, nullptr)),
        _rows(std::exchange(other._rows, 0)),
        _cols(std::exchange(other._cols, 0)) {}

  matrix& operator=(const matrix& other) {
    if (&other != this) {
      *this = matrix(other);
    }
    return *this;
  }

  matrix& operator=(matrix&& other) {
    if (&other != this) {
      std::swap(_data, other._data);
      std::swap(_rows, other._rows);
      std::swap(_cols, other._cols);
    }
    return *this;
  }

  ~matrix() {
    delete[] data();
  }

  // Iterators

  iterator begin() {
    return data();
  }

  const_iterator begin() const {
    return data();
  }

  iterator end() {
    return data() + size();
  }

  const_iterator end() const {
    return data() + size();
  }

  row_iterator row_begin(size_t row) {
    assert(row < rows());
    return data() + row * cols();
  }

  const_row_iterator row_begin(size_t row) const {
    assert(row < rows());
    return data() + row * cols();
  }

  row_iterator row_end(size_t row) {
    assert(row < rows());
    return row_begin(row) + cols();
  }

  const_row_iterator row_end(size_t row) const {
    assert(row < rows());
    return row_begin(row) + cols();
  }

  col_iterator col_begin(size_t col) {
    assert(col < cols());
    return col_iterator(begin(), col, cols());
  }

  const_col_iterator col_begin(size_t col) const {
    assert(col < cols());
    return const_col_iterator(begin(), col, cols());
  }

  col_iterator col_end(size_t col) {
    assert(col < cols());
    return col_iterator(end(), col, cols());
  }

  const_col_iterator col_end(size_t col) const {
    assert(col < cols());
    return const_col_iterator(end(), col, cols());
  }

  // Size

  size_t rows() const {
    return _rows;
  }

  size_t cols() const {
    return _cols;
  }

  size_t size() const {
    return rows() * cols();
  }

  bool empty() const {
    return data() == nullptr;
  }

  // Elements access

  reference operator()(size_t row, size_t col) {
    return row_begin(row)[col];
  }

  const_reference operator()(size_t row, size_t col) const {
    return row_begin(row)[col];
  }

  pointer data() {
    return _data;
  }

  const_pointer data() const {
    return _data;
  }

  // Comparison

  friend bool operator==(const matrix& left, const matrix& right) {
    return left.cols() == right.cols() && left.rows() == right.rows() &&
           std::equal(left.begin(), left.end(), right.begin());
  }

  friend bool operator!=(const matrix& left, const matrix& right) = default;

  // Arithmetic operations

  // helpers:

private:
  template <class BinaryOperation>
  requires std::is_invocable_r_v<value_type, BinaryOperation, const_reference, const_reference>
  matrix& transform(const matrix& other, matrix& output, BinaryOperation operation) const {
    assert(cols() == other.cols() && rows() == other.rows() && "Matrices should have equal dimensions");
    std::transform(begin(), end(), other.begin(), output.begin(), operation);
    return output;
  }

  template <class BinaryOperation>
  requires std::is_invocable_r_v<value_type, BinaryOperation, const_reference, const_reference>
  matrix(const matrix& left, const matrix& right, BinaryOperation operation)
      : _data(new value_type[left.size()]),
        _rows(left.rows()),
        _cols(left.cols()) {
    left.transform(right, *this, operation);
  }

  matrix multiply(const matrix& other) const {
    assert(cols() == other.rows());
    matrix result(rows(), other.cols());
    for (size_t i = 0; i < rows(); ++i) {
      for (size_t j = 0; j < other.cols(); ++j) {
        result(i, j) = std::inner_product(row_begin(i), row_end(i), other.col_begin(j), value_type{});
      }
    }
    return result;
  }

  // operations:

public:
  matrix& operator+=(const matrix& other) {
    return transform(other, *this, std::plus<>());
  }

  matrix& operator-=(const matrix& other) {
    return transform(other, *this, std::minus<>());
  }

  matrix& operator*=(const matrix& other) {
    return *this = multiply(other);
  }

  matrix& operator*=(const_reference factor) {
    return transform(*this, *this, [&factor](const_reference x, const_reference) { return x * factor; });
  }

  friend matrix operator+(const matrix& left, const matrix& right) {
    return matrix(left, right, std::plus<>());
  }

  friend matrix operator-(const matrix& left, const matrix& right) {
    return matrix(left, right, std::minus<>());
  }

  friend matrix operator*(const matrix& left, const matrix& right) {
    return left.multiply(right);
  }

  friend matrix operator*(const matrix& left, const_reference right) {
    return matrix(left, left, [&right](const_reference x, const_reference) { return x * right; });
  }

  friend matrix operator*(const_reference left, const matrix& right) {
    return operator*(right, left);
  }
};
