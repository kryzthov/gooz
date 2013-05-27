#ifndef STORE_OPEN_RECORD_INL_H_
#define STORE_OPEN_RECORD_INL_H_

namespace store {

// virtual
inline
Arity* OpenRecord::OpenRecordArity(Store* store) {
  return GetArity(store);
}

// virtual
inline
uint64 OpenRecord::OpenRecordWidth() {
  return size();
}

// virtual
inline
bool OpenRecord::OpenRecordHas(Value feature) {
  return Has(feature);
}

// virtual
inline
Value OpenRecord::OpenRecordGet(Value feature) {
  return Get(feature);
}

// virtual
inline
Value OpenRecord::OpenRecordClose(Store* store) {
  Value record = GetRecord(store);
  // TODO: Should act like a unification → wakes suspensions up
  ref_->BindTo(record);
  return record;
}

// virtual
inline
Value::ItemIterator* OpenRecord::OpenRecordIterItems() {
  return new ItemIterator(this);
}

// virtual
inline
Value::ValueIterator* OpenRecord::OpenRecordIterValues() {
  return new ValueIterator(this);
}

// -----------------------------------------------------------------------------

// virtual
inline
Arity* OpenRecord::RecordArity() {
  throw SuspendThread(ref_->suspensions());
}

// virtual
inline
uint64 OpenRecord::RecordWidth() {
  throw SuspendThread(ref_->suspensions());
}

// virtual
inline
bool OpenRecord::RecordHas(Value feature) {
  throw SuspendThread(ref_->suspensions());
}

// virtual
inline
Value OpenRecord::RecordGet(Value feature) {
  throw SuspendThread(ref_->suspensions());
}

// virtual
inline
Value::ItemIterator* OpenRecord::RecordIterItems() {
  throw SuspendThread(ref_->suspensions());
}

// virtual
inline
Value::ValueIterator* OpenRecord::RecordIterValues() {
  throw SuspendThread(ref_->suspensions());
}

}  // namespace store

#endif  // STORE_OPEN_RECORD_INL_H_
