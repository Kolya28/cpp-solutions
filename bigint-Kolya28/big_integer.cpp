#include "big_integer.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>

big_integer::big_integer() : _negative(false) {}

big_integer::big_integer(const big_integer& other) = default;

big_integer::big_integer(unsigned long long val, bool negative) : _negative(negative) {
  if (val != 0) {
    _data.push_back(static_cast<digit_type>(val));

    val >>= DIGIT_BITS;
    if (val != 0) {
      _data.push_back(static_cast<digit_type>(val));
    }
  }
}

big_integer::big_integer(long long val)
    : big_integer(val == std::numeric_limits<long long>::min()
                      ? static_cast<uint64_t>(std::numeric_limits<long long>::max()) + 1
                      : static_cast<uint64_t>(std::abs(val)),
                  val < 0) {}

big_integer::big_integer(long val) : big_integer(static_cast<long long>(val)) {}

big_integer::big_integer(unsigned long val) : big_integer(static_cast<unsigned long long>(val)) {}

big_integer::big_integer(int val) : big_integer(static_cast<long long>(val)) {}

big_integer::big_integer(unsigned int val) : big_integer(static_cast<unsigned long long>(val)) {}

big_integer::big_integer(short val) : big_integer(static_cast<long long>(val)) {}

big_integer::big_integer(unsigned short val) : big_integer(static_cast<unsigned long long>(val)) {}

big_integer::big_integer(const std::string& str) : big_integer() {
  size_t index = 0;
  if (!str.empty() && str[index] == '-') {
    negate();
    ++index;
  }

  if (index == str.size()) {
    throw std::invalid_argument("Empty number in big_integer from string constructor");
  }

  while (index != str.size()) {
    size_t end = std::min(index + DIGITS_10, str.size());
    digit_type digit = 0;
    auto res = std::from_chars(&str[index], &str[end], digit);
    if (res.ptr != &str[end]) {
      throw std::invalid_argument("String parse error in big_integer constructor");
    }
    abs_mul_int(static_cast<digit_type>(std::pow(10, end - index)));
    abs_add_int(digit);
    index = end;
  }

  trim();
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(const big_integer& other) {
  if (&other != this) {
    big_integer(other).swap(*this);
  }
  return *this;
}

big_integer& big_integer::operator+=(const big_integer& rhs) {
  if (is_negative() == rhs.is_negative()) {
    abs_add(rhs, *this);
  } else {
    if (_data.size() == rhs._data.size()) {
      while (!_data.empty() && _data.back() == rhs._data[_data.size() - 1]) {
        _data.pop_back();
      }
      if (_data.empty()) {
        return *this;
      } else if (_data.back() > rhs._data[_data.size() - 1]) {
        abs_subtract(rhs, *this, _data.size());
      } else {
        rhs.abs_subtract(*this, *this, _data.size());
      }
    } else if (_data.size() > rhs._data.size()) {
      abs_subtract(rhs, *this);
    } else {
      rhs.abs_subtract(*this, *this);
    }
  }

  return *this;
}

big_integer& big_integer::operator-=(const big_integer& rhs) {
  return (negate() += rhs).negate();
}

big_integer& big_integer::operator*=(const big_integer& rhs) {
  _negative ^= rhs.is_negative();
  return abs_mul(rhs);
}

big_integer& big_integer::operator/=(const big_integer& rhs) {
  div_rem(rhs);
  return *this;
}

big_integer& big_integer::operator%=(const big_integer& rhs) {
  div_rem(rhs).swap(*this);
  return *this;
}

big_integer& big_integer::operator&=(const big_integer& rhs) {
  return bitwise_operation(rhs, std::bit_and<>());
}

big_integer& big_integer::operator|=(const big_integer& rhs) {
  return bitwise_operation(rhs, std::bit_or<>());
}

big_integer& big_integer::operator^=(const big_integer& rhs) {
  return bitwise_operation(rhs, std::bit_xor<>());
}

big_integer& big_integer::operator<<=(int rhs) {
  _data.insert(_data.begin(), rhs / DIGIT_BITS, 0);
  abs_mul_int(static_cast<digit_type>(1) << (rhs % DIGIT_BITS));
  return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
  ptrdiff_t to_erase = rhs / DIGIT_BITS;
  if (to_erase > _data.size()) {
    _data.clear();
    return *this;
  }

  _data.erase(_data.begin(), _data.begin() + to_erase);
  abs_divide_int(static_cast<digit_type>(1) << (rhs % DIGIT_BITS));

  if (is_negative()) {
    abs_add_int(1);
  }
  return *this;
}

big_integer big_integer::operator+() const {
  return *this;
}

big_integer big_integer::operator-() const {
  return big_integer(*this).negate();
}

big_integer big_integer::operator~() const {
  return --big_integer(*this).negate();
}

big_integer& big_integer::operator++() {
  if (!is_negative()) {
    return abs_add_int(1);
  } else {
    return abs_sub_int(1);
  }
}

big_integer big_integer::operator++(int) {
  big_integer copy(*this);
  ++*this;
  return copy;
}

big_integer& big_integer::operator--() {
  if (is_zero()) {
    _negative = true;
    _data.push_back(1);
    return *this;
  }
  return (++negate()).negate();
}

big_integer big_integer::operator--(int) {
  big_integer copy(*this);
  --*this;
  return copy;
}

big_integer operator+(const big_integer& a, const big_integer& b) {
  return big_integer(a) += b;
}

big_integer operator-(const big_integer& a, const big_integer& b) {
  return big_integer(a) -= b;
}

big_integer operator*(const big_integer& a, const big_integer& b) {
  return big_integer(a) *= b;
}

big_integer operator/(const big_integer& a, const big_integer& b) {
  return big_integer(a) /= b;
}

big_integer operator%(const big_integer& a, const big_integer& b) {
  return big_integer(a) %= b;
}

big_integer operator&(const big_integer& a, const big_integer& b) {
  return big_integer(a) &= b;
}

big_integer operator|(const big_integer& a, const big_integer& b) {
  return big_integer(a) |= b;
}

big_integer operator^(const big_integer& a, const big_integer& b) {
  return big_integer(a) ^= b;
}

big_integer operator<<(const big_integer& a, int b) {
  return big_integer(a) <<= b;
}

big_integer operator>>(const big_integer& a, int b) {
  return big_integer(a) >>= b;
}

bool operator==(const big_integer& a, const big_integer& b) {
  return a.is_negative() == b.is_negative() && a._data == b._data;
}

bool operator!=(const big_integer& a, const big_integer& b) {
  return !operator==(a, b);
}

bool operator<(const big_integer& a, const big_integer& b) {
  return operator>(b, a);
}

bool operator>(const big_integer& a, const big_integer& b) {
  if (!a.is_negative() && b.is_negative()) {
    return true;
  } else if (a.is_negative() && !b.is_negative()) {
    return false;
  }

  if (a._data.size() != b._data.size()) {
    return (a._data.size() > b._data.size()) ^ a.is_negative();
  }

  if (a.is_negative()) {
    // don't use xor because '>' will become '>='
    return std::lexicographical_compare(a._data.rbegin(), a._data.rend(), b._data.rbegin(), b._data.rend());
  } else {
    return std::lexicographical_compare(b._data.rbegin(), b._data.rend(), a._data.rbegin(), a._data.rend());
  }
}

bool operator<=(const big_integer& a, const big_integer& b) {
  return !operator>(a, b);
}

bool operator>=(const big_integer& a, const big_integer& b) {
  return operator<=(b, a);
}

std::string to_string(const big_integer& a) {
  if (a.is_zero()) {
    return "0";
  }

  big_integer copy = a;
  std::string res;

  while (!copy.is_zero()) {
    auto rem = copy.abs_divide_int(big_integer::DIGIT_MOD10);
    size_t len = 9;
    while (rem != 0) {
      res.push_back(rem % 10 + '0');
      rem /= 10;
      --len;
    }
    if (!copy.is_zero()) {
      res.insert(res.end(), len, '0');
    }
  }

  if (a.is_negative()) {
    res.push_back('-');
  }

  std::reverse(res.begin(), res.end());
  return res;
}

std::ostream& operator<<(std::ostream& out, const big_integer& a) {
  return out << to_string(a);
}

bool big_integer::is_negative() const noexcept {
  return _negative && !is_zero();
}

void big_integer::swap(big_integer& other) noexcept(std::is_nothrow_swappable_v<std::vector<digit_type>>) {
  std::swap(_data, other._data);
  std::swap(_negative, other._negative);
}

bool big_integer::is_zero() const noexcept {
  return _data.empty();
}

void big_integer::trim() noexcept {
  while (!_data.empty() && _data.back() == 0) {
    _data.pop_back();
  }
}

big_integer& big_integer::negate() noexcept {
  _negative = !_negative;
  return *this;
}

void big_integer::abs_add(const big_integer& rhs, big_integer& output) const {
  assert(this == &output || &rhs == &output);
  if (_data.size() < rhs._data.size()) {
    rhs.abs_add(*this, output);
    return;
  }
  double_digit_type carry = 0;
  size_t i = 0;
  for (; i < rhs._data.size(); i++) {
    carry += rhs._data[i];
    carry += _data[i];
    output._data[i] = static_cast<digit_type>(carry);
    carry >>= DIGIT_BITS;
  }

  output._data.resize(_data.size());
  for (; i < _data.size(); i++) {
    carry += _data[i];
    output._data[i] = static_cast<digit_type>(carry);
    carry >>= DIGIT_BITS;
  }
  if (carry) {
    output._data.push_back(static_cast<digit_type>(carry));
  }
}

void big_integer::abs_subtract(const big_integer& rhs, big_integer& output, size_t max_size) const {
  assert(this == &output || &rhs == &output);

  output._negative = is_negative();
  double_digit_type borrow = 0;

  size_t i = 0;
  size_t size = std::min(max_size, rhs._data.size());
  for (; i < size; i++) {
    double_digit_type diff = static_cast<double_digit_type>(_data[i]) - rhs._data[i] - borrow;
    borrow = diff > DIGIT_MAX_VALUE;
    output._data[i] = static_cast<digit_type>(diff);
  }

  size = std::min(max_size, _data.size());
  for (; i < size && borrow; i++) {
    double_digit_type diff = static_cast<double_digit_type>(_data[i]) - borrow;
    borrow = diff > DIGIT_MAX_VALUE;
    output._data[i] = static_cast<digit_type>(diff);
  }
  output.trim();
}

big_integer::digit_type big_integer::twos_complement(bool neg, big_integer::digit_type digit,
                                                     big_integer::digit_type& carry) {
  if (!neg) {
    return digit;
  }
  double_digit_type res = carry;
  res += ~digit;
  carry = static_cast<digit_type>(res >> DIGIT_BITS);
  return static_cast<digit_type>(res);
}

big_integer& big_integer::abs_mul(const big_integer& rhs) {
  size_t this_size = _data.size();
  size_t rhs_size = rhs._data.size();

  _data.insert(_data.begin(), rhs_size, 0);

  for (size_t i = 0; i < this_size; ++i) {
    digit_type carry = 0;
    double_digit_type data_i = _data[i + rhs_size];
    for (size_t j = 0; j < rhs_size; ++j) {
      double_digit_type res = data_i * rhs._data[j] + _data[i + j] + carry;
      carry = static_cast<digit_type>(res >> DIGIT_BITS);
      _data[i + j] = static_cast<digit_type>(res);
    }
    _data[i + rhs_size] = carry;
  }

  trim();
  return *this;
}

big_integer::digit_type big_integer::abs_divide_int(digit_type val) {
  double_digit_type remainder = 0;
  for (auto it = _data.rbegin(); it != _data.rend(); ++it) {
    remainder = (remainder << DIGIT_BITS) | *it;
    *it = remainder / val;
    remainder %= val;
  }
  trim();
  return remainder;
}

big_integer& big_integer::abs_add_int(digit_type rhs) {
  double_digit_type s = rhs;
  for (digit_type& d : _data) {
    s += d;
    d = static_cast<digit_type>(s);
    s >>= DIGIT_BITS;
  }
  if (s != 0) {
    _data.push_back(static_cast<digit_type>(s));
  }
  return *this;
}

big_integer& big_integer::abs_sub_int(digit_type rhs) {
  double_digit_type borrow = rhs;
  for (digit_type& d : _data) {
    double_digit_type diff = static_cast<double_digit_type>(d) - borrow;
    borrow = diff > DIGIT_MAX_VALUE;
    d = static_cast<digit_type>(diff);
  }
  trim();
  return *this;
}

big_integer& big_integer::abs_mul_int(digit_type rhs) {
  double_digit_type s = 0;
  for (digit_type& d : _data) {
    s = static_cast<double_digit_type>(d) * rhs + s;
    d = static_cast<digit_type>(s);
    s >>= DIGIT_BITS;
  }
  if (s != 0) {
    _data.push_back(static_cast<digit_type>(s));
  }
  trim();
  return *this;
}

int32_t big_integer::normalize() {
  if (_data.back() >= DIGIT_BASE / 2) {
    return 0;
  }
  auto k = static_cast<int32_t>(std::log2(DIGIT_BASE / 2) - std::log2(_data.back()));
  if ((_data.back() << k) < DIGIT_BASE / 2) {
    k++;
  }
  *this <<= k;
  return k;
}

big_integer big_integer::div_rem(big_integer rhs) {
  if (rhs.is_zero()) {
    throw std::invalid_argument("big_integer division by zero");
  }

  big_integer rem;

  if (is_zero() || _data.size() < rhs._data.size()) {
    rem.swap(*this);
    return rem;
  }

  bool this_sign = is_negative();
  bool result_sign = is_negative() ^ rhs.is_negative();

  _negative = false;
  rhs._negative = false;

  int32_t k = rhs.normalize();
  *this <<= k;

  size_t m = _data.size() - rhs._data.size();

  std::vector<digit_type> q(m + 1, 0);
  auto q_it = q.rbegin();
  rhs._data.insert(rhs._data.begin(), m, 0);

  if (*this >= rhs) {
    *q_it = 1;
    this->abs_subtract(rhs, *this);
  }
  ++q_it;

  for (; q_it != q.rend() && _data.size() >= 2; ++q_it) {
    rhs._data.erase(rhs._data.begin());
    double_digit_type t =
        ((static_cast<double_digit_type>(_data.back()) << DIGIT_BITS) | _data[_data.size() - 2]) / rhs._data.back();
    *q_it = std::min(t, DIGIT_BASE - 1);
    *this -= *q_it * rhs;
    while (*this < 0) {
      --*q_it;
      *this += rhs;
    }
  }

  std::swap(rem._data, _data);
  rem.trim();
  rem.abs_divide_int(static_cast<digit_type>(1) << k);
  rem._negative = this_sign;

  std::swap(_data, q);
  _negative = result_sign;
  trim();

  return rem;
}
