// STL utilities.

#ifndef BASE_STL_UTIL_H__
#define BASE_STL_UTIL_H__

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <glog/logging.h>

// -----------------------------------------------------------------------------
// Generic map functions

// ContainsKey for maps
template <class Container>
bool ContainsKey(const Container& container,
                 const typename Container::value_type::first_type& key) {
  return container.find(key) != container.end();
}

// ContainsKey for sets
template <class Container>
bool ContainsKey(const Container& container,
                 const typename Container::value_type& key) {
  return container.find(key) != container.end();
}

template <class Container>
const typename Container::value_type::second_type& GetExisting(
    const Container& container,
    const typename Container::value_type::first_type& key) {
  auto it = container.find(key);
  CHECK(it != container.end());
  return it->second;
}

template <class Container>
const typename Container::value_type::second_type* FindOrNull(
    const Container& container,
    const typename Container::value_type::first_type& key) {
  auto it = container.find(key);
  if (it == container.end())
    return NULL;
  else
    return &it->second;
}

template <class Container>
void InsertNew(Container& container,
	       const typename Container::value_type::first_type& key,
	       const typename Container::value_type::second_type& value) {
  CHECK(container.insert(std::make_pair(key, value)).second);
}

// -----------------------------------------------------------------------------
// Adds convenience methods to standard containers.

template <typename Key,
	  typename Value,
	  typename Compare = std::less<Key>,
	  typename Alloc = std::allocator<std::pair<const Key, Value> > >
class Map : public std::map<Key, Value, Compare, Alloc> {
 public:
  typedef typename std::map<Key, Value, Compare, Alloc>::iterator
      iterator;

  bool contains(const Key& key) const {
    return this->find(key) != this->end();
  }

  iterator insert_new(const Key& key, const Value& value) noexcept {
    auto insert_result = this->insert(std::make_pair(key, value));
    CHECK(insert_result.second);
    return insert_result.first;
  }

  const Value& operator[](const Key& key) const
      noexcept {
    auto it = this->find(key);
    CHECK(it != this->end());
    return it->second;
  }

  Value& operator[](const Key& key) {
    return std::map<Key, Value, Compare, Alloc>::operator[](key);
  }
};

// -----------------------------------------------------------------------------

template<typename Key,
	 typename Compare = std::less<Key>,
	 typename Alloc = std::allocator<Key> >
class Set : public std::set<Key, Compare, Alloc> {
 public:
  typedef typename std::set<Key, Compare, Alloc>::iterator
      iterator;

  bool contains(const Key& key) const {
    return this->find(key) != this->end();
  }

  iterator insert_new(const Key& key) noexcept {
    auto it = this->insert(key);
    CHECK(it.second);
    return it.first;
  }
};

// -----------------------------------------------------------------------------

template<class Key,
	 class Value,
	 class Hash = std::hash<Key>,
	 class Pred = std::equal_to<Key>,
	 class Alloc = std::allocator<std::pair<const Key, Value> > >
class UnorderedMap : public std::unordered_map<Key, Value, Hash, Pred, Alloc> {
 public:
  typedef typename std::unordered_map<Key, Value, Hash, Pred, Alloc>::iterator
      iterator;

  bool contains(const Key& key) const {
    return this->find(key) != this->end();
  }

  iterator insert_new(const Key& key, const Value& value) noexcept {
    auto it = this->insert(std::make_pair(key, value));
    CHECK(it.second);
    return it.first;
  }

  const Value& operator[](const Key& key) const noexcept {
    auto it = this->find(key);
    CHECK(it != this->end());
    return it->second;
  }

  Value& operator[](const Key& key) {
    return std::unordered_map<Key, Value, Hash, Pred, Alloc>::operator[](key);
  }
};

// -----------------------------------------------------------------------------

template<class Key,
         class Hash = std::hash<Key>,
         class Pred = std::equal_to<Key>,
         class Alloc = std::allocator<Key> >
class UnorderedSet : public std::unordered_set<Key, Hash, Pred, Alloc> {
 public:
  typedef typename std::unordered_set<Key, Hash, Pred, Alloc>::iterator
      iterator;

  bool contains(const Key& key) const {
    return this->find(key) != this->end();
  }

  iterator insert_new(const Key& key) noexcept {
    auto it = this->insert(key);
    CHECK(it.second);
    return it.first;
  }
};

// -----------------------------------------------------------------------------
// Symmetric pairs: (x, y) == (y, x)

template <typename T1, typename T2>
struct SymmetricPair : public std::pair<T1, T2> {
  SymmetricPair() {}
  SymmetricPair(const T1& a, const T2& b)
      : std::pair<T1, T2>(a, b) {
  }
};

template<typename T1, typename T2>
inline bool operator==(const SymmetricPair<T1, T2>& x,
                       const SymmetricPair<T1, T2>& y) {
  return ((x.first == y.first) && (x.second == y.second))
      || ((x.first == y.second) && (x.second == y.first));
}

template<typename T1, typename T2 = T1,
         typename Hash1 = std::hash<T1>,
         typename Hash2 = Hash1>
struct SymmetricPairHash {
  inline
  size_t operator()(const SymmetricPair<T1, T2>& spair) const {
    // return std::hash<T1>()(spair.first) ^ std::hash<T2>()(spair.second);
    return Hash1()(spair.first) ^ Hash2()(spair.second);
  }
};

#endif  // BASE_STL_UTIL_H__
