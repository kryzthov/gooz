#ifndef STORE_SMALL_INTEGER_H_
#define STORE_SMALL_INTEGER_H_

#include <string>
using std::string;

#include <gmpxx.h>

#include <glog/logging.h>

//#include "store/value.h"

namespace store {

// -----------------------------------------------------------------------------
// Small integers
//
// This class is not meant to be stored. It should only be instantiated as a
// local variable, where it can be optimized/inlined.
//
class SmallInteger {
 public:
  static const Value::ValueType kType = Value::SMALL_INTEGER;

  // Returns the small integer encoded in the given value.
  static inline
  int64 ValueToSmallInt(const Value& value) {
    CHECK(value.IsSmallInt());
    return ((int64) value.bits()) >> kTagBits;
  }

  static inline
  bool IsSmallInt(int64 value) {
    return (value > kSmallIntMin) && (value < kSmallIntMax);
  }

  static inline
  bool IsSmallInt(uint64 value) {
    return (value < static_cast<uint64>(kSmallIntMax));
  }

  static inline
  bool IsSmallInt(const mpz_class& value) {
    return value.fits_slong_p() && IsSmallInt(value.get_si());
  }

  SmallInteger(const Value& value) : value_(ValueToSmallInt(value)) {
  }

  SmallInteger(int64 value) : value_(value) {
    CHECK(IsSmallInt(value));
  }

  inline
  int64 value() const { return value_; }

  inline
  Value Encode() const {
    return Value((value_ << kTagBits) | kSmallIntTag);
  }

  inline
  uint64 caps() const { return Value::CAP_LITERAL; }

  inline uint64 LiteralHashCode() const {
    return static_cast<uint64>(value_);
  }

  void ToASCII(string* repr) const;

  bool LiteralLessThan(Value other) const;

 private:
  // The small integer value.
  const int64 value_;
};

}  // namespace store

#endif  // STORE_SMALL_INTEGER_H_
