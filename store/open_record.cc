#include "store/values.h"

#include <memory>
using std::unique_ptr;

namespace store {

// -----------------------------------------------------------------------------
// Open record

const Value::ValueType OpenRecord::kType;

OpenRecord::OpenRecord(Store* store, Value label)
    : ref_(Variable::New(store)),
      label_(label) {
  CHECK(label.caps() & Value::CAP_LITERAL);
}

Value OpenRecord::Get(Value feature) const {
  FeatureMap::const_iterator it = features_.find(feature);
  if (it == features_.end())
    return NULL;
  else
    return it->second;
}

bool OpenRecord::Set(Value label, Value value) {
  FeatureMap::iterator it =
      features_.insert(features_.begin(), std::make_pair(label, value));
  return it->second == value;
}

bool OpenRecord::IsTuple() const {
  const int64 nfeatures = features_.size();
  if (nfeatures == 0) return true;
  Value last = features_.rbegin()->first;
  return (last.tag() == kSmallIntTag)
      && (SmallInteger(last).value() == nfeatures);
}

// TODO: account for references to the arity in the specified store
Arity* OpenRecord::GetArity(Store* store) const {
  if (IsTuple()) return Arity::GetTuple(size());
  vector<Value> features(size());
  int i = 0;
  for (auto it = features_.begin(); it != features_.end(); ++it, ++i)
    features[i] = it->first;
  return Arity::GetFromSorted(features);
}

Value OpenRecord::GetRecord(Store* store) const {
  const uint64 nvalues = size();
  if (nvalues == 0)
    return label_;
  Value values[nvalues];
  int i = 0;
  for (auto it = features_.begin(); it != features_.end(); ++it, ++i)
    values[i] = it->second;
  if (IsTuple()) {
    if ((nvalues == 2) && (label_ == KAtomList())) {
      return List::New(store, values[0], values[1]);
    } else {
      return Tuple::New(store, label_, nvalues, values);
    }
  } else {
    return Record::New(store, label_, GetArity(store), values);
  }
}

// virtual
void OpenRecord::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  label_.Explore(ref_map);
  for (auto it = features_.begin(); it != features_.end(); ++it) {
    const_cast<Value&>(it->first).Explore(ref_map);
    it->second.Explore(ref_map);
  }
}

// virtual
Value OpenRecord::Deref() {
  return ref_->IsFree() ? this : ref_->Deref();
}

// virtual
Value OpenRecord::Optimize(OptimizeContext* context) {
  if (!ref_->IsFree()) return context->Optimize(ref_);
  for (auto it = features_.begin(); it != features_.end(); ++it) {
    // We shouldn't need to optimize the feature value.
    it->second = context->Optimize(it->second);
  }
  return this;
}

// virtual
bool OpenRecord::IsDetermined() {
  return ref_->IsDetermined();
}

// virtual
HeapValue* OpenRecord::MoveInternal(Store* store) {
  OpenRecord* const moved = New(store, label_.Move(store));
  for (auto it = features_.begin(), insert_after = moved->features_.begin();
       it != features_.end(); ++it) {
    insert_after = moved->features_.insert(
        insert_after,
        std::make_pair(const_cast<Value&>(it->first).Move(store),
                       it->second.Move(store)));
  }
  return moved;
}

// virtual
bool OpenRecord::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  if (ovalue.type() == Value::OPEN_RECORD) {
    OpenRecord* orecord = ovalue.as<OpenRecord>();
    if (!Value::Unify(context, label_, orecord->label_)) return false;

    FeatureMap merged;
    FeatureMap::iterator insert_after = merged.begin();

    // First check if we can merge the common values
    FeatureMap::iterator it1 = features_.begin();
    FeatureMap::iterator it2 = orecord->features_.begin();
    while ((it1 != features_.end()) && (it2 != orecord->features_.end())) {
      if (const_cast<Value&>(it1->first).LiteralLessThan(it2->first)) {
        insert_after = merged.insert(insert_after, *it1);
        ++it1;
      } else if (const_cast<Value&>(it2->first).LiteralLessThan(it1->first)) {
        insert_after = merged.insert(insert_after, *it2);
        ++it2;
      } else {  // Common feature
        if (!Value::Unify(context, it1->second, it2->second)) return false;
        insert_after = merged.insert(insert_after, *it1);
        ++it1;
        ++it2;
      }
    }
    merged.insert(it1, features_.end());
    merged.insert(it2, orecord->features_.end());

    // TODO: This is bogus: won't be reversed if unification aborts.
    features_.swap(merged);
    orecord->ref_->UnifyWith(context, this);
    return true;

  } else if (ovalue.caps() & Value::CAP_RECORD) {
    if (!Value::Unify(context, label_, ovalue.RecordLabel())) return false;

    FeatureMap::iterator it1 = features_.begin();
    unique_ptr<Value::ItemIterator> it2_deleter(ovalue.RecordIterItems());
    Value::ItemIterator& it2 = *it2_deleter;

    while (it1 != features_.end()) {
      while (!it2.at_end() && (*it2).first.LiteralLessThan(it1->first))
        ++it2;
      if (it2.at_end()) return false;
      if (!(*it2).first.LiteralEquals(it1->first)) return false;
      if (!Value::Unify(context, it1->first, (*it2).first)) return false;
      ++it1;
      ++it2;
    }
    ref_->UnifyWith(context, ovalue);
    return true;

  } else {
    return false;
  }
}

// virtual
bool OpenRecord::IsStateless(StatelessnessContext* context) {
  // FIXME: ref_ should be a Value, and might reference another open-record.
  if (ref_ == NULL)
    return false;
  else
    return context->IsStateless(ref_);
}

// virtual
void OpenRecord::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  if (ref_->IsFree()) {
    context->Encode(label_, repr);
    repr->push_back('(');
    for (auto it = features_.begin(); it != features_.end(); ++it) {
      context->Encode(it->first, repr);
      repr->push_back(':');
      context->Encode(it->second, repr);
      repr->push_back(' ');
    }
    repr->append("...)");
  } else {
    context->Encode(ref_, repr);
  }
}

// virtual
void OpenRecord::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

}  // namespace store
