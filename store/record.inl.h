#ifndef STORE_RECORD_INL_H_
#define STORE_RECORD_INL_H_

namespace store {

// static
inline
Record* Record::New(Store* store, Value label, Arity* arity) {
  const uint64 nvalues = arity->size();
  CHECK(nvalues > 0);
  CHECK(!arity->IsTuple());
  void* block = store->AllocWithNestedArray<Record, Value>(nvalues);
  Record* record = new(block) Record(label, arity);
  for (uint64 i = 0; i < nvalues; ++i)
    record->values_[i] = Variable::New(store);
  return record;
}

// static
inline
Record* Record::New(Store* store, Value label, Arity* arity, Value* values) {
  const uint64 nvalues = arity->size();
  CHECK(nvalues > 0);
  CHECK(!arity->IsTuple());
  void* block = store->AllocWithNestedArray<Record, Value>(nvalues);
  return new(CHECK_NOTNULL(block)) Record(label, arity, values);
}

inline
uint64 Record::size() const {
  return arity_->size();
}

inline
bool Record::Has(Value feature) const {
  return arity_->Has(feature);
}

// virtual
inline
uint64 Record::RecordWidth() {
  return arity_->size();
}

// virtual
inline
bool Record::RecordHas(Value feature) {
  return arity_->Has(feature);
}

// virtual
inline
Value Record::RecordGet(Value feature) {
  return values_[arity_->Map(feature)];
}

// -----------------------------------------------------------------------------

// virtual
inline
ValuePair Record::ItemIterator::operator*() {
  CHECK(!at_end());
  return std::make_pair(record_->arity_->features()[index_],
                        record_->values_[index_]);
}

}  // namespace store

#endif  // STORE_RECORD_INL_H_
