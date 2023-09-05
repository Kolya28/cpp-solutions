#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <execution>
#include <type_traits>
#include <utility>

namespace socow_vector_impl {
template <typename T>
struct shared_buffer {
  size_t capacity;
  size_t ref_count;
  T data[0];

  static shared_buffer* allocate(size_t capacity) {
    auto new_buf = static_cast<shared_buffer*>(operator new(sizeof(shared_buffer) + sizeof(T) * capacity));
    new (new_buf) shared_buffer{capacity, 0, {}};
    return new_buf;
  }

  static void deallocate(shared_buffer* buffer) noexcept {
    buffer->~shared_buffer();
    operator delete(buffer);
  }

  static void release_ref(shared_buffer* buffer, size_t size) {
    if (--buffer->ref_count == 0) {
      std::destroy_n(buffer->data, size);
      deallocate(buffer);
    }
  }
};
} // namespace socow_vector_impl

template <typename T, size_t SMALL_SIZE>
class socow_vector {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

private:
  using buffer = socow_vector_impl::shared_buffer<T>;

  union {
    buffer* _dynamic_buffer;
    T _small_data[SMALL_SIZE];
  };

  size_t _size;
  bool _is_small;

  bool is_small() const noexcept {
    return _is_small;
  }

  bool is_shared() const noexcept {
    return !is_small() && _dynamic_buffer->ref_count > 1;
  }

  explicit socow_vector(size_t initial_capacity) : _size(0), _is_small(true) {
    if (initial_capacity > SMALL_SIZE) {
      assign_buffer(buffer::allocate(initial_capacity));
    }
  }

  void copy_and_swap(const socow_vector& other, size_t new_capacity) {
    socow_vector temp(new_capacity);
    std::uninitialized_copy_n(other.data(), other.size(), temp.unsafe_data());
    temp._size = other.size();
    swap(temp);
  }

  void assign_buffer(buffer* new_buffer) {
    size_t s = size();
    clear_and_make_small();
    _size = s;
    _dynamic_buffer = new_buffer;
    _is_small = false;
    ++_dynamic_buffer->ref_count;
  }

  void clear_and_make_small() noexcept {
    if (is_small()) {
      std::destroy_n(unsafe_data(), size());
      _size = 0;
      return;
    }
    buffer::release_ref(_dynamic_buffer, size());
    _size = 0;
    _is_small = true;
  }

  void unshare() {
    if (!is_shared()) {
      return;
    }

    set_capacity(_dynamic_buffer->capacity);
  }

  void big_to_small(const socow_vector& other) {
    auto temp = _dynamic_buffer;
    pointer small_data = _small_data;
    try {
      std::uninitialized_copy_n(other.data(), other.size(), small_data);
    } catch (...) {
      _dynamic_buffer = temp;
      throw;
    }
    buffer::release_ref(temp, size());
    _is_small = true;
  }

  pointer unsafe_data() {
    return is_small() ? _small_data : _dynamic_buffer->data;
  }

  const_pointer unsafe_data() const {
    return is_small() ? _small_data : _dynamic_buffer->data;
  }

public:
  socow_vector() noexcept : _size(0), _is_small(true) {}

  socow_vector(const socow_vector& other) : _size(0), _is_small(true) {
    if (!other.is_small()) {
      assign_buffer(other._dynamic_buffer);
      _size = other.size();
    } else {
      copy_and_swap(other, other.size());
    }
  }

  ~socow_vector() noexcept {
    clear_and_make_small();
  }

  socow_vector& operator=(const socow_vector& other) {
    if (&other == this) {
      return *this;
    }

    if (!other.is_small()) {
      assign_buffer(other._dynamic_buffer);

    } else if (!is_small() && other.is_small()) {
      big_to_small(other);

    } else { // both small
      size_t min_size = std::min(size(), other.size());

      socow_vector temp;
      std::uninitialized_copy_n(other._small_data, min_size, temp._small_data);
      temp._size = min_size;

      if (other.size() > size()) {
        std::uninitialized_copy_n(other._small_data + size(), other.size() - size(), _small_data + size());
      } else if (other.size() < size()) {
        std::destroy(_small_data + other.size(), _small_data + size());
      }
      _size = other.size();
      std::swap_ranges(temp._small_data, temp._small_data + temp.size(), _small_data);
    }

    _size = other.size();
    return *this;
  }

  reference operator[](size_t index) noexcept {
    assert(index < size() && "Out of range in subscript operator");
    return data()[index];
  }

  const_reference operator[](size_t index) const noexcept {
    assert(index < size() && "Out of range in subscript operator");
    return data()[index];
  }

  pointer data() {
    unshare();
    return unsafe_data();
  }

  const_pointer data() const noexcept {
    return unsafe_data();
  }

  size_t size() const noexcept {
    return _size;
  }

  reference front() {
    assert(!empty() && "Getting front element from empty vector");
    return *begin();
  }

  const_reference front() const noexcept {
    assert(!empty() && "Getting front element from empty vector");
    return *begin();
  }

  reference back() {
    assert(!empty() && "Getting back element from empty vector");
    return *(end() - 1);
  }

  const_reference back() const noexcept {
    assert(!empty() && "Getting back element from empty vector");
    return *(end() - 1);
  }

  void push_back(const T& value) {
    insert(cend(), value);
  }

  void pop_back() {
    assert(!empty() && "vector::pop_back() on empty vector");
    erase(cend() - 1);
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t capacity() const noexcept {
    return is_small() ? SMALL_SIZE : _dynamic_buffer->capacity;
  }

  void set_capacity(size_t new_capacity) {
    if (new_capacity < size()) {
      return;
    }

    if (!is_small() && new_capacity <= SMALL_SIZE) {
      big_to_small(*this);
    } else if (new_capacity > capacity() || is_shared()) {
      socow_vector temp(new_capacity);
      std::uninitialized_copy_n(unsafe_data(), size(), temp.unsafe_data());
      temp._size = size();
      *this = temp;
    }
  }

  void reserve(size_t new_capacity) {
    set_capacity(new_capacity);
  }

  void shrink_to_fit() {
    if (size() != capacity()) {
      if (size() > SMALL_SIZE) {
        copy_and_swap(*this, size());
      } else {
        set_capacity(size());
      }
    }
  }

  void clear() {
    if (is_shared()) {
      clear_and_make_small();
      return;
    }
    std::destroy_n(unsafe_data(), size());
    _size = 0;
  }

  void swap(socow_vector& other) {
    if (this == &other) {
      return;
    }
    if (is_small() && other.is_small()) {
      pointer copy_to = other._small_data, copy_from = _small_data;
      size_t min_size = std::min(size(), other.size());
      if (size() < other.size()) {
        std::swap(copy_to, copy_from);
      }

      size_t max_size = std::max(size(), other.size());

      std::uninitialized_copy_n(copy_from + min_size, max_size - min_size, copy_to + min_size);
      std::destroy_n(copy_from + min_size, max_size - min_size);

      std::swap(_size, other._size);
      std::swap_ranges(_small_data, _small_data + min_size, other._small_data);
    } else if (!is_small() && other.is_small()) {
      socow_vector temp = *this;
      *this = other;
      other = temp;
    } else if (is_small() && !other.is_small()) {
      other.swap(*this);
    } else {
      std::swap(_dynamic_buffer, other._dynamic_buffer);
      std::swap(_size, other._size);
    }
  }

  iterator begin() {
    return data();
  }

  iterator end() {
    return begin() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  const_iterator cbegin() const noexcept {
    return std::as_const(*this).begin();
  }

  const_iterator cend() const noexcept {
    return std::as_const(*this).end();
  }

  iterator insert(const_iterator pos, const T& value) {
    size_t index = pos - cbegin();
    assert(index <= size() && "Index out of range in vector::insert()");

    size_t cap = capacity();

    if (size() + 1 > cap || is_shared()) {
      socow_vector temp(size() + 1 > cap ? cap * 2 : cap);
      std::uninitialized_copy_n(unsafe_data(), index, temp.unsafe_data());
      temp._size = index;
      new (temp.unsafe_data() + index) T(value);
      ++temp._size;
      std::uninitialized_copy(unsafe_data() + index, unsafe_data() + size(), temp.unsafe_data() + temp.size());
      temp._size = size() + 1;
      *this = temp;
      return unsafe_data() + index;
    }

    new (end()) T(value);
    ++_size;

    iterator new_pos = unsafe_data() + index;
    for (iterator it = end() - 1; it != new_pos; --it) {
      std::iter_swap(it, it - 1);
    }
    return new_pos;
  }

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) {
    size_t index = first - cbegin();
    size_t last_index = last - cbegin();
    assert(index <= last_index && "last_index > first_index in vector::erase()");
    assert(last_index <= size() && "Index out of range in vector::erase()");

    if (index == last_index) {
      return unsafe_data() + index;
    }

    size_t new_size = size() - (last_index - index);

    if (is_shared()) {
      socow_vector temp(_dynamic_buffer->capacity);

      std::uninitialized_copy_n(_dynamic_buffer->data, index, temp.unsafe_data());
      temp._size = index;
      std::uninitialized_copy_n(_dynamic_buffer->data + last_index, new_size - index, temp.unsafe_data() + index);
      temp._size = new_size;

      *this = temp;
      return unsafe_data() + index;
    }

    std::swap_ranges(unsafe_data() + last_index, unsafe_data() + size(), unsafe_data() + index);
    std::destroy(unsafe_data() + new_size, unsafe_data() + size());
    _size = new_size;

    return unsafe_data() + index;
  }
};
