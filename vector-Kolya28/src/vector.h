#pragma once

#include <cstddef>
#include <type_traits>

template <typename T>
class vector {
private:
  T* data_;
  size_t size_;
  size_t capacity_;

  vector(const vector& other, size_t new_capacity) : size_(0), capacity_(new_capacity) {
    assert(other.size() <= new_capacity && "new_capacity < size in vector copy constructor");
    if (new_capacity == 0) {
      data_ = nullptr;
      return;
    }
    data_ = static_cast<T*>(operator new(sizeof(T) * new_capacity));
    try {
      for (; size() < other.size(); ++size_) {
        new (end()) T(other[size()]);
      }
    } catch (...) {
      clear();
      operator delete(data());
      throw;
    }
  }

public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

  using iterator = pointer;
  using const_iterator = const_pointer;

public:
  vector() noexcept : data_(nullptr), size_(0), capacity_(0) {}

  vector(const vector& other) : vector(other, other.size()) {}

  vector& operator=(const vector& other) {
    if (&other != this) {
      vector(other).swap(*this);
    }
    return *this;
  }

  ~vector() noexcept {
    if (data() != nullptr) {
      clear();
      operator delete(data());
    }
  }

  reference operator[](size_t index) noexcept {
    assert(index < size() && "Out of range in subscript operator");
    return data()[index];
  }

  const_reference operator[](size_t index) const noexcept {
    assert(index < size() && "Out of range in subscript operator");
    return data()[index];
  }

  pointer data() noexcept {
    return data_;
  }

  const_pointer data() const noexcept {
    return data_;
  }

  size_t size() const noexcept {
    return size_;
  }

  reference front() noexcept {
    assert(!empty() && "Getting front element from empty vector");
    return *begin();
  }

  const_reference front() const noexcept {
    assert(!empty() && "Getting front element from empty vector");
    return *begin();
  }

  reference back() noexcept {
    assert(!empty() && "Getting back element from empty vector");
    return *(end() - 1);
  }

  const_reference back() const noexcept {
    assert(!empty() && "Getting back element from empty vector");
    return *(end() - 1);
  }

  void push_back(const T& value) {
    if (size() + 1 <= capacity()) {
      new (end()) T(value);
      ++size_;
      return;
    }
    size_t new_capacity = (capacity() == 0) ? 1 : capacity() * 2;
    vector copy(*this, new_capacity);
    copy.push_back(value);
    swap(copy);
  }

  void pop_back() noexcept {
    assert(!empty() && "vector::pop_back() on empty vector");
    back().~T();
    --size_;
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  size_t capacity() const noexcept {
    return capacity_;
  }

  void reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
      vector(*this, new_capacity).swap(*this);
    }
  }

  void shrink_to_fit() {
    if (size() != capacity()) {
      vector(*this, size()).swap(*this);
    }
  }

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  void swap(vector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
  }

  iterator begin() noexcept {
    return data();
  }

  iterator end() noexcept {
    return begin() + size();
  }

  const_iterator begin() const noexcept {
    return data();
  }

  const_iterator end() const noexcept {
    return begin() + size();
  }

  iterator insert(const_iterator pos, const T& value) {
    size_t index = pos - begin();
    assert(index <= size() && "Index out of range in vector::insert()");

    push_back(value);
    iterator new_pos = begin() + index;
    for (iterator it = end() - 1; it != new_pos; --it) {
      std::iter_swap(it, it - 1);
    }
    return new_pos;
  }

  iterator erase(const_iterator pos) noexcept(noexcept(erase(pos, pos))) {
    return erase(pos, pos + 1);
  }

  iterator erase(const_iterator first, const_iterator last) noexcept(std::is_nothrow_swappable_v<T>) {
    size_t index = first - begin();
    size_t last_index = last - begin();

    assert(index <= last_index && "last_index > first_index in vector::erase()");
    assert(last_index <= size() && "Index out of range in vector::erase()");

    iterator new_end = std::swap_ranges(begin() + last_index, end(), begin() + index);
    size_t new_size = new_end - begin();
    while (size() != new_size) {
      pop_back();
    }

    return begin() + index;
  }
};
