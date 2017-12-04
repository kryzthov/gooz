#include "store/values.h"

namespace store {

// -----------------------------------------------------------------------------
// Tuple

// static
const Value::ValueType Tuple::kType;

// virtual
void Tuple::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  label_.Explore(ref_map);
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    values_[i].Explore(ref_map);
}

// virtual
Value Tuple::Optimize(OptimizeContext* context) {
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    values_[i] = context->Optimize(values_[i]);
  return this;
}

// virtual
bool Tuple::UnifyWith(UnificationContext* context, Value ovalue) {
  if (ovalue.type() != Value::TUPLE) return false;
  Tuple* otuple = ovalue.as<Tuple>();
  if (size_ != otuple->size_) return false;
  if (!Value::Unify(context, label_, otuple->label_)) return false;
  for (uint64 i = 0; i < size_; ++i)
    if (!Value::Unify(context, values_[i], otuple->TupleGet(i))) return false;
  return true;
}

// virtual
bool Tuple::Equals(EqualityContext* context, Value value) {
  Tuple* tuple = value.as<Tuple>();
  if (size_ != tuple->size_) return false;
  if (!context->Equals(label_, tuple->label_)) return false;
  for (uint64 i = 0; i < size_; ++i)
    if (!context->Equals(values_[i], tuple->values_[i])) return false;
  return true;
}

// virtual
HeapValue* Tuple::MoveInternal(Store* store) {
  const uint64 nvalues = size();
  // TODO: We could avoid copying the array.
  Value moved_values[nvalues];
  for (uint64 i = 0; i < nvalues; ++i)
    moved_values[i] = values_[i].Move(store);
  return New(store, label_.Move(store), nvalues, moved_values);
}

// virtual
bool Tuple::IsStateless(StatelessnessContext* context) {
  const uint64 nvalues = size();
  for (uint64 i = 0; i < nvalues; ++i)
    if (!context->IsStateless(values_[i])) return false;
  return true;
}

// virtual
void Tuple::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);

  const uint64 nvalues = size();
  CHECK_GT(nvalues, 0UL);

  if (label_ == KAtomTuple()) {
    // Special case for # tuples
    // TODO: Handle nested tuples or list constructors
    context->Encode(values_[0], repr);
    for (uint64 i = 1; i < nvalues; ++i) {
      repr->push_back('#');
      context->Encode(values_[i], repr);
    }
  } else {
    context->Encode(label_, repr);
    repr->push_back('(');
    context->Encode(values_[0], repr);
    for (uint64 i = 1; i < nvalues; ++i) {
      repr->push_back(' ');
      context->Encode(values_[i], repr);
    }
    repr->push_back(')');
  }
}

// -----------------------------------------------------------------------------

}  // namespace store

