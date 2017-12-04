#ifndef STORE_STRING_H_
#define STORE_STRING_H_

#include <string>
using std::string;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// String
//
// Strings are immutable.
//
class String : public HeapValue {
 public:
  static const ValueType kType = Value::STRING;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline String* Get(Store* store, const string& value) {
    return new(CHECK_NOTNULL(store->Alloc<String>())) String(value);
  }

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const throw() { return kType; }
  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool Equals(EqualityContext* context, Value value);

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  explicit String(const string& value) : value_(value) {}
  virtual ~String() {}

  // ---------------------------------------------------------------------------
  // Memory layout
  const string value_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_STRING_H_
