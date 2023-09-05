#pragma once

#include <cstddef>
#include <iterator>

template <typename T>
class list {
public:
  using value_type = T;

  using reference = T&;
  using const_reference = const T&;

  using pointer = T*;
  using const_pointer = const T*;

private:
  struct node {
    node() = default;

    node(node* prev, node* next) noexcept : prev(prev), next(next) {}

    void link_next(node* other) noexcept {
      next = other;
      other->prev = this;
    }

    node* prev;
    node* next;
  };

  struct value_node : node {
    value_node(const value_type& value) : value(value) {}

    value_node(const value_type& value, node* prev, node* next) : node(prev, next), value(value) {}

    value_type value;
  };

  template <class U>
  class basic_list_iterator {
    friend list;

  public:
    using value_type = T;
    using reference = U&;
    using pointer = U*;
    using difference_type = ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

  private:
    node* _node;

    basic_list_iterator(node* node) noexcept : _node(node) {}

  public:
    basic_list_iterator() noexcept = default;

    operator basic_list_iterator<const U>() const noexcept {
      return basic_list_iterator<const U>(_node);
    }

    basic_list_iterator& operator++() noexcept {
      _node = _node->next;
      return *this;
    }

    basic_list_iterator& operator--() noexcept {
      _node = _node->prev;
      return *this;
    }

    basic_list_iterator operator++(int) noexcept {
      basic_list_iterator copy(*this);
      ++*this;
      return copy;
    }

    basic_list_iterator operator--(int) noexcept {
      basic_list_iterator copy(*this);
      --*this;
      return copy;
    }

    reference operator*() const noexcept {
      return static_cast<value_node*>(_node)->value;
    }

    pointer operator->() const noexcept {
      return &static_cast<value_node*>(_node)->value;
    }

    friend bool operator==(const basic_list_iterator& left, const basic_list_iterator& right) noexcept = default;
  };

  size_t _size;
  node _root;

public:
  using iterator = basic_list_iterator<value_type>;
  using const_iterator = basic_list_iterator<const value_type>;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
  // O(1), nothrow
  list() noexcept : _size(0), _root(&_root, &_root) {}

  // O(n), strong
  list(const list& other) : list(other.begin(), other.end()) {}

  // O(n), strong
  template <std::input_iterator InputIt>
  list(InputIt first, InputIt last) : list() {
    std::copy(first, last, std::back_inserter(*this));
  }

  // O(n), strong
  list& operator=(const list& other) {
    if (&other != this) {
      list copy = other;
      swap(*this, copy);
    }
    return *this;
  }

  // O(n), nothrow
  ~list() noexcept {
    clear();
  }

  // O(1), nothrow
  bool empty() const noexcept {
    return _size == 0;
  }

  // O(1), nothrow
  size_t size() const noexcept {
    return _size;
  }

  // O(1), nothrow
  T& front() noexcept {
    return *begin();
  }

  // O(1), nothrow
  const T& front() const noexcept {
    return *begin();
  }

  // O(1), nothrow
  T& back() noexcept {
    return *std::prev(end());
  }

  // O(1), nothrow
  const T& back() const noexcept {
    return *std::prev(end());
  }

  // O(1), strong
  void push_front(const T& val) {
    insert(begin(), val);
  }

  // O(1), strong
  void push_back(const T& val) {
    insert(end(), val);
  }

  // O(1), nothrow
  void pop_front() noexcept {
    erase(begin());
  }

  // O(1), nothrow
  void pop_back() noexcept {
    erase(std::prev(end()));
  }

  // O(1), nothrow
  iterator begin() noexcept {
    return iterator(_root.next);
  }

  // O(1), nothrow
  const_iterator begin() const noexcept {
    return const_iterator(_root.next);
  }

  // O(1), nothrow
  iterator end() noexcept {
    return iterator(&_root);
  }

  // O(1), nothrow
  const_iterator end() const noexcept {
    return const_iterator(const_cast<node*>(&_root));
  }

  // O(1), nothrow
  reverse_iterator rbegin() noexcept {
    return std::reverse_iterator(end());
  }

  // O(1), nothrow
  const_reverse_iterator rbegin() const noexcept {
    return std::reverse_iterator(end());
  }

  // O(1), nothrow
  reverse_iterator rend() noexcept {
    return std::reverse_iterator(begin());
  }

  // O(1), nothrow
  const_reverse_iterator rend() const noexcept {
    return std::reverse_iterator(begin());
  }

  // O(n), nothrow
  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  // O(1), strong
  iterator insert(const_iterator pos, const T& val) {
    node* new_node = new value_node(val);
    pos._node->prev->link_next(new_node);
    new_node->link_next(pos._node);
    ++_size;
    return iterator(new_node);
  }

  // O(last - first), strong
  template <std::input_iterator InputIt>
  iterator insert(const_iterator pos, InputIt first, InputIt last) {
    if (first == last) {
      return iterator(pos._node);
    }
    list temp(first, last);
    auto b = temp.begin();
    splice(pos, temp, temp.begin(), temp.end());
    return b;
  }

  // O(1), nothrow
  iterator erase(const_iterator pos) noexcept {
    value_node* temp = static_cast<value_node*>(pos._node);
    node* next = temp->next;
    temp->prev->link_next(next);
    delete temp;
    _size--;
    return iterator(next);
  }

  // O(last - first), nothrow
  iterator erase(const_iterator first, const_iterator last) noexcept {
    list temp;
    auto it = iterator(last._node);
    temp.splice(temp.begin(), *this, first, last);
    return it;
  }

  // O(last - first) in general but O(1) when possible, nothrow
  void splice(const_iterator pos, list& other, const_iterator first, const_iterator last) noexcept {
    if (first == last) {
      return;
    }
    if (this != &other) {
      ptrdiff_t diff = other.size();
      if (first != other.begin() || last != other.end()) {
        diff = std::distance(first, last);
      }
      other._size -= diff;
      _size += diff;
    }

    node* cp = pos._node->prev;
    last._node->prev->link_next(pos._node);
    first._node->prev->link_next(last._node);
    cp->link_next(first._node);
  }

  // O(1), nothrow
  friend void swap(list& left, list& right) noexcept {
    node* lp = left._root.prev;
    node* ln = left._root.next;
    node* rp = right._root.prev;
    node* rn = right._root.next;
    ln->prev = lp->next = &right._root;
    rn->prev = rp->next = &left._root;
    std::swap(left._root, right._root);
    std::swap(left._size, right._size);
  }
};
