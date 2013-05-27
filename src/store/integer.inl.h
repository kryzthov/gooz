#ifndef STORE_INTEGER_INL_H_
#define STORE_INTEGER_INL_H_

namespace store {

// static
inline
Integer* Integer::New(Store* store, int64 value) {
  CHECK(!SmallInteger::IsSmallInt(value));
  return new(CHECK_NOTNULL(store->Alloc<Integer>())) Integer(value);
}

// static
inline
Integer* Integer::New(Store* store, const mpz_class& value) {
  CHECK(!SmallInteger::IsSmallInt(value));
  return new(CHECK_NOTNULL(store->Alloc<Integer>())) Integer(value);
}

}  // namespace store

#endif  // STORE_INTEGER_INL_H_
