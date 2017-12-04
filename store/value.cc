#include "store/values.h"

#include <algorithm>

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------

void Value::Explore(ReferenceMap* ref_map) const {
  CHECK_NOTNULL(ref_map);
  ReferenceMap::iterator it = ref_map->find(*this);
  if (it == ref_map->end()) {
    (*ref_map)[*this] = false;
    if (IsHeapValue())
      heap_value_->ExploreValue(ref_map);
  } else {
    it->second = true;
  }
}

FeatureNotFound::FeatureNotFound(const Value& feature, Arity* arity)
    : message_("Feature " + feature.ToString()
               + " not found in arity " + CHECK_NOTNULL(arity)->ToString()) {
}

// -----------------------------------------------------------------------------
// Serialization

void ToASCIIContext::Encode(Value value, string* repr) {
  value = value.Deref();
  ReferenceMap::iterator it = ref_map.find(value);
  if ((it != ref_map.end()) && it->second
      // Write atoms and integers directly.
      && !(value.IsA<Atom>()
           || value.IsA<Integer>()
           || value.IsA<SmallInteger>()))
    repr->append((format("V%p") % value.heap_value()).str());
  else
    value.ToASCII(this, repr);
}

void Value::ToString(string* repr) const {
  CHECK_NOTNULL(repr);
  ToASCIIContext context;
  Explore(&context.ref_map);
  for (auto it = context.ref_map.begin(); it != context.ref_map.end(); ++it) {
    if (!it->second) continue;
    Value value = it->first;
    if (!value.IsHeapValue()
        || value.IsA<store::Atom>()
        || value.IsA<store::Integer>())
      continue;

    repr->append((format("V%p=") % value.heap_value()).str());
    value.ToASCII(&context, repr);
    repr->append("\n");
  }
  this->ToASCII(&context, repr);
}

void Value::ToASCII(ToASCIIContext* context, string* repr) const {
  switch (tag()) {
    case kHeapValueTag: heap_value_->ToASCII(context, repr); return;
    case kSmallIntTag: SmallInteger(*this).ToASCII(repr); return;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

void Value::ToProtoBuf(oz_pb::Value* value) const {
  throw NotImplemented();
}

// -----------------------------------------------------------------------------
// Value unification

void UnificationContext::AddMutation(Variable* var) {
  shared_ptr<SuspensionList>& initial_suspensions = mutations[var];
  if (initial_suspensions == NULL) {
    initial_suspensions.reset(
        new SuspensionList(*CHECK_NOTNULL(var->suspensions())));
  } else {
    // The variable initial state is already saved in the mutation pool.
  }
}

// static
bool Value::Unify(UnificationContext* context, Value value1, Value value2) {
  CHECK_NOTNULL(context);
  value1 = value1.Deref();
  value2 = value2.Deref();
  if (value1 == value2) return true;
  if (!context->Add(value1, value2))
    return true;
  // Favor UnboundValue->UnifyWith(BoundValue).
  if (value1.IsDetermined()) {
    return value2.UnifyWith(context, value1);
  } else {
    return value1.UnifyWith(context, value2);
  }
}

bool Value::UnifyWith(UnificationContext* context, Value ovalue) {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->UnifyWith(context, ovalue);
    case kSmallIntTag: return false;  // bits equality
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

bool Unify(Value value1, Value value2, SuspensionList* suspensions) {
  CHECK_NOTNULL(suspensions);

  value1 = value1.Deref();
  value2 = value2.Deref();
  if (value1 == value2) return true;
  // Do not run a transaction, if we can avoid it.
  if (value1.type() == Value::VARIABLE) {
    Variable* var1 = value1.as<Variable>();
    if (var1->BindTo(value2)) {
      suspensions->splice(suspensions->end(), *var1->suspensions());
    }
    return true;

  } else if (value2.type() == Value::VARIABLE) {
    Variable* var2 = value2.as<Variable>();
    if (var2->BindTo(value1)) {
      suspensions->splice(suspensions->end(), *var2->suspensions());
    }
    return true;

  } else {
    UnificationContext context;
    if (Value::Unify(&context, value1, value2)) {
      suspensions->splice(suspensions->end(), context.new_runnable);
      return true;

    } else {
      // Unification failed
      for (auto it = context.mutations.begin();
	   it != context.mutations.end();
	   ++it)
        it->first->RevertToFree(it->second.get());
      return false;
    }
  }
}

// -----------------------------------------------------------------------------
// Value equality

// static
bool EqualityContext::Equals(Value value1, Value value2) {
  value1 = value1.Deref();
  value2 = value2.Deref();
  if (!Add(value1, value2)) return true;
  if (value1 == value2) return true;
  if (value1.type() != value2.type()) return false;
  return value1.Equals(this, value2);
}

bool Value::Equals(EqualityContext* context, Value value) const {
  if (tag() != value.tag()) return false;
  switch (tag()) {
    case kHeapValueTag: return heap_value_->Equals(context, value);
    case kSmallIntTag: return false;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

bool Equals(Value value1, Value value2) {
  EqualityContext context;
  return context.Equals(value1, value2);
}

// -----------------------------------------------------------------------------
// Stateless-ness test

bool StatelessnessContext::IsStateless(Value value) {
  return !ref_map_.insert(value).second
      || value.IsStateless(this);
}

bool Value::IsStateless(StatelessnessContext* context) const {
  CHECK(IsHeapValue());
  return heap_value_->IsStateless(context);
}

bool IsStateless(Value value) {
  StatelessnessContext context;
  return context.IsStateless(value);
}

// -----------------------------------------------------------------------------
// Value graph optimization

Value OptimizeContext::Optimize(Value value) {
  if (ref_map_.insert(value).second)
    return value.Optimize(this);
  else
    return value.Deref();
}

Value Value::Optimize(OptimizeContext* context) {
  switch (tag()) {
    case kSmallIntTag: return *this;
    case kHeapValueTag: return heap_value_->Optimize(context);
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

Value Optimize(Value value) {
  OptimizeContext context;
  return context.Optimize(value);
}

// -----------------------------------------------------------------------------

Value Value::Move(Store* store) {
  CHECK(IsHeapValue());
  return heap_value_->Move(store);
}

int64 IntValue(Value value) {
  value = value.Deref();
  switch (value.tag()) {
    case kSmallIntTag: return SmallInteger(value).value();
    case kHeapValueTag: return value.as<Integer>()->value();
  }
  LOG(FATAL) << "Unexpected value type: tag=" << value.tag();
}

// -----------------------------------------------------------------------------

Atom* const kAtomEmpty = NULL;
Atom* const kAtomTrue = NULL;
Atom* const kAtomFalse = NULL;
Atom* const kAtomNil = NULL;
Atom* const kAtomList = NULL;
Atom* const kAtomTuple = NULL;

Arity* const kArityEmpty = NULL;
Arity* const kAritySingleton = NULL;
Arity* const kArityPair = NULL;


void Initialize() {
  VLOG(1) << "Entering " << __PRETTY_FUNCTION__;

  CHECK_EQ(kWordSize, 8 * sizeof(Value));

  const_cast<Atom*&>(kAtomEmpty) = Atom::Get("");
  const_cast<Atom*&>(kAtomTrue) = Atom::Get("true");
  const_cast<Atom*&>(kAtomFalse) = Atom::Get("false");
  const_cast<Atom*&>(kAtomNil) = Atom::Get("nil");
  const_cast<Atom*&>(kAtomList) = Atom::Get("|");
  const_cast<Atom*&>(kAtomTuple) = Atom::Get("#");

  const_cast<Arity*&>(kArityEmpty) = Arity::GetTuple(0);
  const_cast<Arity*&>(kAritySingleton) = Arity::GetTuple(1);
  const_cast<Arity*&>(kArityPair) = Arity::GetTuple(2);

  VLOG(1) << "Exiting " << __PRETTY_FUNCTION__;
}

}  // namespace store
