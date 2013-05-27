#ifndef STORE_LIST_INL_H_
#define STORE_LIST_INL_H_

namespace store {

// virtual
inline
Value List::RecordLabel() {
  return KAtomList();
}

// virtual
inline
Arity* List::RecordArity() {
  return KArityPair();
}

}  // namespace store

#endif  // STORE_LIST_INL_H_
