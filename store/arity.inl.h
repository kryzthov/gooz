#ifndef STORE_ARITY_INL_H_
#define STORE_ARITY_INL_H_

namespace store {

// static
inline
Arity* Arity::Get(Value value) {
  vector<Value> features(1, value);
  return GetFromSorted(features);
}

// static
inline
Arity* Arity::Get(Value value1, Value value2) {
  vector<Value> features;
  features.push_back(value1);
  features.push_back(value2);
  return Get(features);
}

// static
inline
Arity* Arity::Get(Value value1, Value value2, Value value3) {
  vector<Value> features;
  features.push_back(value1);
  features.push_back(value2);
  features.push_back(value3);
  return Get(features);
}

// static
inline
Arity* Arity::New(Store* store, uint64 size, Value const * literals) {
  return Get(size, literals);
}

inline
uint64 Arity::Map(int64 integer) throw(FeatureNotFound) {
  return Map(Value::Integer(integer));
}

inline
uint64 Arity::Map(const StringPiece& atom) throw(FeatureNotFound) {
  return Map(Atom::Get(atom));
}

inline
bool Arity::Has(int64 integer) const throw() {
  return Has(Value::Integer(integer));
}

inline
bool Arity::Has(const StringPiece& atom) const throw() {
  return Has(Atom::Get(atom));
}

}  // namespace store

#endif  // STORE_ARITY_INL_H_
