#ifndef STORE_FLOAT_H_
#define STORE_FLOAT_H_

#include <string>
using std::string;

#include <glog/logging.h>

#include "store/store.h"
#include "store/value.h"

namespace store {

// -----------------------------------------------------------------------------
// Float
//
// A floating-point number. Current precision: 64 bits.
//
class Float : public HeapValue {
 public:
  static const ValueType kType = Value::FLOAT;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline Float* New(Store* store, double value) {
    return new(CHECK_NOTNULL(store->Alloc<Float>())) Float(value);
  }

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const noexcept { return kType; }
  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool Equals(EqualityContext* context, Value value);

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  explicit Float(double value) : value_(value) {}
  virtual ~Float() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  const double value_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_FLOAT_H_
