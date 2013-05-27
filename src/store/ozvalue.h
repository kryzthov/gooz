// Convenience C++ interface to manipulate Oz values.

#ifndef STORE_OZVALUE_H_
#define STORE_OZVALUE_H_

#include "combinators/oznode_eval_visitor.h"
#include "combinators/ozparser.h"
#include "store/values.h"

namespace store {

// Value unification: x = y
// Access to record feature
//     x.i, i integer : x[i]
//     x.a, a atom : x["a"]
// Access to record label: x.label()
// Conversion to native C++ types:
//     integer: static_cast<int>(x)
//     atom: static_cast<string>(x)
//     boolean: static_cast<bool>(x)
class OzValue {
 public:
  // FIXME: Get rid of kHeapStore!
  static OzValue Parse(const string& code, Store* store = kHeapStore) {
    return OzValue(combinators::oz::ParseEval(code, store));
  }

  OzValue(Store* store)
      : value_(Variable::New(store)) {
  }

  OzValue(const OzValue& ozvalue)
      : value_(ozvalue.value_.Deref()) {
  }

  OzValue(Value value)
      : value_(value.Deref()) {
  }

  OzValue(int64 intval)
      : value_(Value::Integer(intval)) {
  }

  OzValue(const StringPiece& atom)
      : value_(Atom::Get(atom)) {
  }

  OzValue(const char* const atom)
      : value_(Atom::Get(atom)) {
  }

  OzValue& operator=(OzValue& new_value) {
    CHECK(Unify(value(), new_value.value()));
    return *this;
  }

  OzValue& operator=(Value new_value) {
    CHECK(Unify(value(), new_value));
    return *this;
  }

  OzValue& operator=(int64 new_value) {
    CHECK(Unify(value(), Value::Integer(new_value)));
    return *this;
  }

  OzValue& operator=(const StringPiece& atom) {
    CHECK(Unify(value(), Atom::Get(atom)));
    return *this;
  }

  // ---------------------------------------------------------------------------

  Value value() {
    value_ = value_.Deref();
    return value_;
  }

  ValueType type() {
    return value().type();
  }

  template <typename T>
  T* value() { return value().as<T>(); }

  // ---------------------------------------------------------------------------
  // Record interface

  bool IsOpenRecord() {
    return type() == Value::OPEN_RECORD;
  }

  bool IsRecord() {
    return value().caps() & Value::CAP_RECORD;
  }

  bool IsTuple() {
    return value().caps() & Value::CAP_TUPLE;
  }

  bool IsLiteral() {
    return value().caps() & Value::CAP_LITERAL;
  }

  OzValue label() {
    return OzValue(value().RecordLabel());
  }

  Arity* arity() {
    return value().RecordArity();
  }

  uint64 size() {
    return value().RecordWidth();
  }

  bool HasFeature(OzValue feature) {
    return arity()->Has(feature.value());
  }

  OzValue operator[](int64 index) {
    return OzValue(value().RecordGet(Value::Integer(index)));
  }

  OzValue operator[](const char* atom) {
    return OzValue(value().RecordGet(Atom::Get(atom)));
  }

  OzValue operator[](const string& atom) {
    return OzValue(value().RecordGet(Atom::Get(atom)));
  }

  OzValue operator[](OzValue feat) {
    return OzValue(value().RecordGet(feat.value()));
  }

  int int_val() { return IntValue(value()); }
  bool bool_val() { return value().as<Boolean>()->value(); }
  string atom_val() { return value().as<Atom>()->value(); }

  bool operator==(OzValue& other) {
    return store::Equals(value(), other.value());
  }

  bool operator==(int64 intval) {
    return (type() == Value::SMALL_INTEGER)
        && (IntValue(value()) == intval);
  }

  bool operator==(const string& atom) {
    return (type() == Value::ATOM)
        && (value().as<Atom>()->value() == atom);
  }

  bool IsDetermined() {
    return IsDet(value());
  }

 private:
  Value value_;
};

inline
std::ostream& operator<<(std::ostream& os, OzValue ozvalue) {
  return os << ozvalue.value().ToString();
}

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_OZVALUE_H_
