// Representation of a store.
// A store is a self-contained set of values with roots
// and external references.

#ifndef STORE_STORE_H_
#define STORE_STORE_H_

#include <vector>

#include "base/macros.h"
#include "base/stl-util.h"

namespace store {

class HeapValue;

// Computes the size of object T with a nested array A[size];
template <typename T, typename A>
uint64 SizeOfWithNestedArray(uint64 size) {
  return sizeof(T) + size * sizeof(A);
}

// -----------------------------------------------------------------------------

// Abstract base class for value stores.
class Store {
 public:
  Store() {}
  virtual ~Store() {}

  // Allocates a new block of memory in the store.
  // @returns Pointer to the allocated memory block,
  //     NULL if not enough space left.
  virtual void* Alloc(uint64 size) = 0;

  // Allocates a block of memory for the given object.
  template <typename T>
  T* Alloc() { return static_cast<T*>(Alloc(sizeof(T))); }

  // Allocates a block of memory for an array T[size];
  template <typename T>
  T** AllocArray(uint64 size) {
    return static_cast<T**>(Alloc(size * sizeof(T)));
  }

  // Allocates a memory block for an object T, with a nested array A[size].
  template <typename T, typename A>
  T* AllocWithNestedArray(uint64 size) {
    return static_cast<T*>(Alloc(SizeOfWithNestedArray<T, A>(size)));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Store);
};

// -----------------------------------------------------------------------------

// A store that allocates memory in the heap.
// This is a temporary solution: there is no way to clean values up.
class HeapStore : public Store {
 public:
  HeapStore();
  virtual ~HeapStore();

  virtual void* Alloc(uint64 size);

 private:
  // Number of allocs.
  uint64 nallocs_;

  // Number of bytes allocated.
  uint64 size_;

  DISALLOW_COPY_AND_ASSIGN(HeapStore);
};

extern HeapStore* const kHeapStore;

// -----------------------------------------------------------------------------

// A fixed size store.
class StaticStore : public Store {
 public:
  // Initializes a store with the specified size, in bytes.
  explicit StaticStore(uint64 size);
  virtual ~StaticStore();

  virtual void* Alloc(uint64 size);

  // @returns The size of the store, in bytes.
  uint64 size() const { return size_; }

  // @returns The space left, in bytes.
  uint64 free() const { return free_; }

  // @returns Whether the pointer belongs to this store or not.
  // @param ptr The pointer to test.
  bool Contains(const void* const ptr) const {
    return (base_ <= ptr) && (ptr < base_ + size_);
  }

  // Manages the store roots.
  void AddRoot(HeapValue* root);
  void RemoveRoot(HeapValue* root);

  // Moves the reachable content of store from into store to.
  // @param from Store to copy from.
  // @param to Store to copy into.
  static void Move(StaticStore* from, Store* to);

 private:  // ------------------------------------------------------------------
  // Size of the store, in bytes.
  const uint64 size_;

  // Space left, in bytes.
  uint64 free_;

  // Bottom of the store memory area.
  char* const base_;

  // Position of the next area to allocate.
  char* next_;

  // Set of roots determining the reachable content of the store.
  UnorderedSet<HeapValue*> roots_;

  DISALLOW_COPY_AND_ASSIGN(StaticStore);
};

}  // namespace store

#endif  // STORE_STORE_H_
