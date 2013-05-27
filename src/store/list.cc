#include "store/values.h"

namespace store {

// -----------------------------------------------------------------------------
// List

/* static */ const Value::ValueType List::kType;

int64 List::GetValuesCount(Value* last) {
  CHECK_NOTNULL(last);
  int64 count = 0;
  ReferenceSet ref_set;
  CHECK(ref_set.insert(this).second);
  GetValuesCountInternal(&ref_set, &count, last);
  return count;
}

void List::GetValuesCountInternal(
    ReferenceSet* ref_set, int64* count, Value* last) {
  Value tail = tail_.Deref();
  if (tail.type() == Value::LIST) {
    *count += 1;
    if (ref_set->insert(tail).second) {
      tail.as<List>()->GetValuesCountInternal(ref_set, count, last);
    } else {
      *last = tail;
    }
  } else {
    *last = tail;
    *count += 1;
  }
}

// virtual
void List::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  head_.Explore(ref_map);
  tail_.Explore(ref_map);
}

// virtual
Value List::Optimize(OptimizeContext* context) {
  head_ = context->Optimize(head_);
  tail_ = context->Optimize(tail_);
  return this;
}

// virtual
bool List::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  if (ovalue.type() != Value::LIST) return false;  // Not a list
  List* olist = ovalue.as<List>();
  return Value::Unify(context, head_, olist->head_)
      && Value::Unify(context, tail_, olist->tail_);
}

// virtual
bool List::Equals(EqualityContext* context, Value value) {
  List* list = value.as<List>();
  return context->Equals(head_, list->head_)
      && context->Equals(tail_, list->tail_);
}

// virtual
HeapValue* List::MoveInternal(Store* store) {
  return New(store, head_.Move(store), tail_.Move(store));
}

// virtual
bool List::IsStateless(StatelessnessContext* context) {
  return context->IsStateless(head_)
      && context->IsStateless(tail_);
}

// virtual
void List::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);

  Value last = NULL;
  const int64 nvalues = GetValuesCount(&last);
  if (last == KAtomNil()) {
    repr->push_back('[');
    context->Encode(head_, repr);

    List* current = this;
    for (int i = 1; i < nvalues; ++i) {
      current = current->Next();
      repr->push_back(' ');
      context->Encode(current->head_, repr);
    }
    repr->push_back(']');
  } else {
    // TODO: add parenthesis around tuple values (a#b#c).
    List* current = this;
    context->Encode(current->head_, repr);

    for (int i = 1; i < nvalues; ++i) {
      current = current->Next();
      repr->push_back('|');
      context->Encode(current->head_, repr);
    }
    repr->push_back('|');
    context->Encode(current->tail_, repr);
  }
}

// virtual
void List::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// virtual
bool List::RecordHas(Value feature) {
  if (!feature.IsA<SmallInteger>()) return false;
  const uint64 value = SmallInteger(feature).value() - 1;
  return (value < 2);
}

// virtual
Value List::RecordGet(Value feature) {
  if (!feature.IsA<SmallInteger>())
    throw FeatureNotFound(feature, RecordArity());

  const uint64 index = SmallInteger(feature).value() - 1;
  if (index >= 2)
    throw FeatureNotFound(feature, RecordArity());
  return values()[index];
}

// -----------------------------------------------------------------------------

}  // namespace store
