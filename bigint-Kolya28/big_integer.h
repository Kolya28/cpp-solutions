#pragma once

#include <cassert>
#include <cmath>
#include <compare>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

struct big_integer {
private:
  using digit_type = uint32_t;
  using double_digit_type = uint64_t;
  static constexpr digit_type DIGIT_MAX_VALUE = std::numeric_limits<digit_type>::max();
  static constexpr digit_type DIGIT_BITS = std::numeric_limits<digit_type>::digits;
  static constexpr double_digit_type DIGIT_BASE = static_cast<double_digit_type>(DIGIT_MAX_VALUE) + 1;
  static constexpr size_t DIGITS_10 = static_cast<size_t>(std::numeric_limits<big_integer::digit_type>::digits10);
  static constexpr digit_type DIGIT_MOD10 = 1'000'000'000;

private:
  std::vector<digit_type> _data;
  bool _negative;

private:
  void trim() noexcept;

  void abs_add(const big_integer& rhs, big_integer& output) const;

  void abs_subtract(const big_integer& rhs, big_integer& output,
                    size_t max_size = std::numeric_limits<size_t>::max()) const;

  static digit_type twos_complement(bool neg, digit_type digit, digit_type& carry);

  template <class BitwiseOperation>
  big_integer& bitwise_operation(const big_integer& rhs, BitwiseOperation op) {
    bool result_negative = op(is_negative(), rhs.is_negative());
    _data.resize(std::max(_data.size(), rhs._data.size()), 0);
    digit_type a_carry = 1, b_carry = 1, res_carry = 1;

    for (size_t i = 0; i < _data.size(); ++i) {
      digit_type a_digit = twos_complement(is_negative(), _data[i], a_carry);
      digit_type b_digit = i < rhs._data.size() ? rhs._data[i] : 0;
      b_digit = twos_complement(rhs.is_negative(), b_digit, b_carry);
      digit_type res_digit = op(a_digit, b_digit);
      _data[i] = twos_complement(result_negative, res_digit, res_carry);
    }

    _negative = result_negative;
    trim();
    return *this;
  }

  big_integer& abs_mul(const big_integer& rhs);

  int32_t normalize();

  big_integer div_rem(big_integer rhs);

  digit_type abs_divide_int(digit_type val);

  big_integer& abs_add_int(digit_type rhs);

  big_integer& abs_sub_int(digit_type rhs);

  big_integer& abs_mul_int(digit_type rhs);

public:
  big_integer();
  big_integer(const big_integer& other);

  big_integer(unsigned long long val, bool negative = false);
  big_integer(long long val);
  big_integer(long val);
  big_integer(unsigned long val);
  big_integer(int val);
  big_integer(unsigned int val);
  big_integer(short val);
  big_integer(unsigned short val);

  explicit big_integer(const std::string& str);
  ~big_integer();

  bool is_negative() const noexcept;

  void swap(big_integer& other) noexcept(std::is_nothrow_swappable_v<std::vector<digit_type>>);

  bool is_zero() const noexcept;

  big_integer& negate() noexcept;

  big_integer& operator=(const big_integer& other);

  big_integer& operator+=(const big_integer& rhs);
  big_integer& operator-=(const big_integer& rhs);
  big_integer& operator*=(const big_integer& rhs);
  big_integer& operator/=(const big_integer& rhs);
  big_integer& operator%=(const big_integer& rhs);

  big_integer& operator&=(const big_integer& rhs);
  big_integer& operator|=(const big_integer& rhs);
  big_integer& operator^=(const big_integer& rhs);

  big_integer& operator<<=(int rhs);
  big_integer& operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer& operator++();
  big_integer operator++(int);

  big_integer& operator--();
  big_integer operator--(int);

  friend bool operator==(const big_integer& a, const big_integer& b);
  friend bool operator!=(const big_integer& a, const big_integer& b);
  friend bool operator<(const big_integer& a, const big_integer& b);
  friend bool operator>(const big_integer& a, const big_integer& b);
  friend bool operator<=(const big_integer& a, const big_integer& b);
  friend bool operator>=(const big_integer& a, const big_integer& b);

  friend std::string to_string(const big_integer& a);
};

big_integer operator+(const big_integer& a, const big_integer& b);
big_integer operator-(const big_integer& a, const big_integer& b);
big_integer operator*(const big_integer& a, const big_integer& b);
big_integer operator/(const big_integer& a, const big_integer& b);
big_integer operator%(const big_integer& a, const big_integer& b);

big_integer operator&(const big_integer& a, const big_integer& b);
big_integer operator|(const big_integer& a, const big_integer& b);
big_integer operator^(const big_integer& a, const big_integer& b);

big_integer operator<<(const big_integer& a, int b);
big_integer operator>>(const big_integer& a, int b);

bool operator==(const big_integer& a, const big_integer& b);
bool operator!=(const big_integer& a, const big_integer& b);
bool operator<(const big_integer& a, const big_integer& b);
bool operator>(const big_integer& a, const big_integer& b);
bool operator<=(const big_integer& a, const big_integer& b);
bool operator>=(const big_integer& a, const big_integer& b);

std::string to_string(const big_integer& a);
std::ostream& operator<<(std::ostream& out, const big_integer& a);
