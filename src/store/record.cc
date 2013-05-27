#include "store/values.h"

#include <memory>
using std::unique_ptr;

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------
// Record

const Value::ValueType Record::kType;

Record::Record(Value label, Arity* arity)
    : label_(label),
      arity_(CHECK_NOTNULL(arity)) {
  CHECK(label.caps() & Value::CAP_LITERAL);
  CHECK(!arity->IsTuple());
}

Record::Record(Value label, Arity* arity, Value* values)
    : label_(label),
      arity_(CHECK_NOTNULL(arity)) {
  CHECK(label.caps() & Value::CAP_LITERAL);
  CHECK_NOTNULL(values);
  const uint64 nvalues = arity->size();
  for (uint64 i = 0; i < nvalues; ++i)
    values_[i] = values[i];
}

Value Record::Get(Value feature) const {
  return values_[arity_->Map(feature)];
}

Value Record::Project(Store* store, Arity* arity) {
  const uint64 size = arity->size();
  Value values[size];
  for (uint64 i = 0, j = 0; i < size; ++i, ++j) {
    Value feature = arity->features()[i];
    while (!arity_->features()[j].LiteralEquals(feature)) {
      j += 1;
      CHECK_LT(j, arity_->size());
    }
    values[i] = values_[j];
  }
  return Value::Record(store, label_, arity, values);
}

Value Record::Subtract(Store* store, Value feature) {
  const uint64 size = arity_->size();

  bool has_feature = false;
  // TODO: Arity::Subtract() could return ifeature as well.
  const uint64 ifeature = arity_->IndexOf(feature, &has_feature);
  CHECK(has_feature);

  Arity* arity = arity_->Subtract(feature);
  CHECK_EQ(size - 1, arity_->size());
  Value values[size - 1];

  for (uint64 i = 0; i < ifeature; ++i)
    values[i] = values_[i];
  for (uint64 i = ifeature + 1; i < size; ++i)
    values[i - 1] = values_[i];

  return Value::Record(store, label_, arity, values);
}

// virtual
void Record::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  label_.Explore(ref_map);
  Value(arity_).Explore(ref_map);
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    values_[i].Explore(ref_map);
}


// virtual
Value Record::Optimize(OptimizeContext* context) {
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    values_[i] = context->Optimize(values_[i]);
  return this;
}

// virtual
bool Record::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  if (!(ovalue.caps() & Value::CAP_RECORD)) return false;
  if (!Value::Unify(context, label_, ovalue.RecordLabel())) return false;
  if (arity_ != ovalue.RecordArity()) return false;  // arities are interned.
  // Unify all values
  const uint64 nvalues = size();
  unique_ptr<Value::ValueIterator> it_deleter(ovalue.RecordIterValues());
  Value::ValueIterator& it(*it_deleter);
  for (uint64 i = 0; i < nvalues; ++i, ++it) {
    Value value1 = values_[i];
    Value value2 = *it;
    if (!Value::Unify(context, value1, value2)) return false;
  }
  return true;
}

// virtual
bool Record::Equals(EqualityContext* context, Value value) {
  Record* record = value.as<Record>();
  if (!context->Equals(label_, record->label_)) return false;
  if (!context->Equals(arity_, record->arity_)) return false;
  const uint64 nvalues = arity_->size();
  for (uint64 i = 0; i < nvalues; ++i)
    if (!context->Equals(values_[i], record->values_[i])) return false;
  return true;
}

// virtual
HeapValue* Record::MoveInternal(Store* store) {
  // TODO: We could save the array copy here...
  const uint64 nvalues = size();
  Value moved_values[nvalues];
  for (uint64 i = 0; i < nvalues; ++i)
    moved_values[i] = values_[i].Move(store);
  return New(store, label_.Move(store), arity_, moved_values);
}

// virtual
bool Record::IsStateless(StatelessnessContext* context) {
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    if (!context->IsStateless(values_[i])) return false;
  return true;
}

// virtual
void Record::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  context->Encode(label_, repr);
  const uint64 nvalues = size();
  if (nvalues == 0) return;
  const vector<Value>& features = arity_->features();
  repr->push_back('(');
  // First feature
  context->Encode(features[0], repr);
  repr->push_back(':');
  context->Encode(values_[0], repr);
  // Next features
  for (uint64 i = 1; i < nvalues; ++i) {
    repr->push_back(' ');
    context->Encode(features[i], repr);
    repr->push_back(':');
    context->Encode(values_[i], repr);
  }
  repr->push_back(')');
}

// virtual
void Record::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// -----------------------------------------------------------------------------

}  // namespace store
