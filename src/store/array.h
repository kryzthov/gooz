#ifndef STORE_ARRAY_H_
#define STORE_ARRAY_H_

#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Array
//
// TODO: arrays may have lower/upper bounds.

class Array : public HeapValue {
 public:
  static const ValueType kType = Value::ARRAY;
  static Array* const EmptyArray;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline Array* New(Store* store, uint64 size, Value initial) {
    void* block = store->Alloc(sizeof(Array) + size * sizeof(Value*));
    return new(CHECK_NOTNULL(block)) Array(size, initial);
  }

  // ---------------------------------------------------------------------------
  // Array specific API

  inline
  Value Access(uint64 index) const {
    CHECK_LT(index, size_);
    return values_[index];
  }

  inline
  void Assign(uint64 index, Value value) {
    CHECK_LT(index, size_);
    // VLOG(1) << (format("array@%p[%lld/%lld] := %p")
    //             % this % index % size_ % value);
    values_[index] = value;
  }

  uint64 size() const { return size_; }
  const Value* values() const { return values_; }

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const throw() { return kType; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);
  virtual bool IsStateless(StatelessnessContext* context) {
    return false;
  }

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  Array(uint64 size, Value initial)
      : size_(size) {
    for (uint64 i = 0; i < size; ++i)
      values_[i] = initial;
  }

  virtual ~Array() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  const uint64 size_;
  Value values_[];
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_ARRAY_H_
