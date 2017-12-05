#ifndef STORE_INTEGER_H_
#define STORE_INTEGER_H_

#include <string>
using std::string;

#include <glog/logging.h>

#include <gmpxx.h>

namespace store {

// -----------------------------------------------------------------------------
// Arbitrary precision integers
//
class Integer : public HeapValue {
 public:
  static const Value::ValueType kType = Value::INTEGER;

  // ---------------------------------------------------------------------------
  // Factory methods
  static Integer* New(Store* store, int64 value);
  static Integer* New(Store* store, const mpz_class& value);

  // ---------------------------------------------------------------------------
  // Integer specific interface
  int64 value() const { return value_.get_si(); }
  const mpz_class& mpz() const { return value_; }

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const noexcept { return kType; }
  virtual uint64 caps() const { return Value::CAP_LITERAL; }
  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool Equals(EqualityContext* context, Value value);
  virtual HeapValue* MoveInternal(Store* store);

  // ---------------------------------------------------------------------------
  // Literal interface

  virtual uint64 LiteralHashCode()  { return value(); }
  virtual bool LiteralEquals(Value other) {
    return (other.type() == Value::INTEGER)
        && (value_ == other.as<Integer>()->value_);
  }
  virtual bool LiteralLessThan(Value other) {
    const Value::LiteralClass tclass = LiteralGetClass();
    const Value::LiteralClass oclass = other.LiteralGetClass();
    return (oclass == LiteralGetClass())
        ? (value_ < other.as<Integer>()->value_)
        : (tclass < oclass);
  }
  virtual Value::LiteralClass LiteralGetClass() {
    return Value::LITERAL_CLASS_INTEGER;
  }

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  explicit Integer(int64 value)
      : value_(value) {
  }
  explicit Integer(const mpz_class& value)
      : value_(value) {
  }
  virtual ~Integer() {
  }

  // ---------------------------------------------------------------------------
  // Memory layout
  mpz_class value_;
};

}  // namespace store

#endif  // STORE_INTEGER_H_
