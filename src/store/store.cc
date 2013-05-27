#include "store/store.h"

#include <glog/logging.h>

#include "store/values.h"

namespace store {

HeapStore::HeapStore()
    : nallocs_(0),
      size_(0) {
}

HeapStore::~HeapStore() {
}

// virtual
void* HeapStore::Alloc(uint64 size) {
  nallocs_++;
  size_ += size;
  return new char[size];
}

HeapStore* const kHeapStore = new HeapStore();

// -----------------------------------------------------------------------------

StaticStore::StaticStore(uint64 size)
    : size_(size),
      free_(size),
      base_(new char[size]),
      next_(base_) {
  CHECK_NOTNULL(base_);
}

StaticStore::~StaticStore() {
}

// virtual
void* StaticStore::Alloc(uint64 size) {
  VLOG(3) << __PRETTY_FUNCTION__
          << " size=" << size
          << " free=" << free_;
  // TODO: Ensure 8 bytes alignment
  if (size > free_) return NULL;
  void* const new_alloc = next_;
  free_ -= size;
  next_ += size;
  return new_alloc;
}

void StaticStore::AddRoot(HeapValue* root) {
  roots_.insert(root);
}

void StaticStore::RemoveRoot(HeapValue* root) {
  roots_.erase(root);
}

// static
void StaticStore::Move(StaticStore* from, Store* to) {
  for (auto it = from->roots_.begin(); it != from->roots_.end(); ++it) {
    (*it)->Move(to);
  }
}

}  // namespace store
