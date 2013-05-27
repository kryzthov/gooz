#ifndef STORE_TUPLE_INL_H_
#define STORE_TUPLE_INL_H_

namespace store {

// static
inline
Tuple* Tuple::New(Store* store, Value label, uint64 size) {
  CHECK(size > 0);
  CHECK((size != 2) || (label != KAtomList()));
  void* block = store->AllocWithNestedArray<Tuple, Value>(size);
  Tuple* tuple = new(CHECK_NOTNULL(block)) Tuple(label, size);
  for (uint64 i = 0; i < size; ++i)
    tuple->values_[i] = Variable::New(store);
  return tuple;
}

// static
inline
Tuple* Tuple::New(Store* store, Value label, uint64 size, Value* values) {
  CHECK(size > 0);
  CHECK((size != 2) || (label != KAtomList()));
  void* block = store->AllocWithNestedArray<Tuple, Value>(size);
  return new(CHECK_NOTNULL(block)) Tuple(label, size, values);
}

inline
Tuple::Tuple(Value label, uint64 size)
    : label_(label),
      size_(size) {
  CHECK_GT(size, 0UL);
  CHECK(!((size == 2) && (label == KAtomList())));
}

inline
Tuple::Tuple(Value label, uint64 size, Value* values)
    : label_(label),
      size_(size) {
  CHECK_GT(size, 0UL);
  CHECK(!((size == 2) && (label == KAtomList())));
  for (uint64 i = 0; i < size; ++i)
    values_[i] = values[i];
}

inline
Value Tuple::Get(Value int_value) const {
  return Get(int_value.as<Integer>()->value());
}

inline
Value Tuple::Get(uint64 index) const {
  --index;
  CHECK_LT(index, size_);
  return values_[index];
}

inline
Arity* Tuple::arity() const {
  return Arity::GetTuple(size_);
}

// virtual
inline
Arity* Tuple::RecordArity() {
  return Arity::GetTuple(size_);
}

// virtual
inline
bool Tuple::RecordHas(Value feature) {
  if (feature.type() != Value::SMALL_INTEGER) return false;
  const uint64 ifeat = IntValue(feature) - 1;
  return ifeat < size_;
}

// virtual
inline
Value Tuple::RecordGet(Value feature) {
  if (feature.type() != Value::SMALL_INTEGER)
    throw FeatureNotFound(feature, Arity::GetTuple(size_));
  const int64 ifeat = IntValue(feature);
  return Get(ifeat);
}

// virtual
inline
Value Tuple::TupleGet(uint64 index) {
  if (index > size_)
    throw FeatureNotFound(Value::Integer(index), Arity::GetTuple(size_));
  return values_[index];
}

}  // namespace store

#endif  // STORE_TUPLE_INL_H_
