#include "store/values.h"

#include <list>
#include <string>
using std::list;
using std::string;

#include "base/stl-util.h"

namespace store {

// -----------------------------------------------------------------------------
// Free variable

const Value::ValueType Variable::kType;

// virtual
Value Variable::Deref() {
  return ref_.IsDefined() ? ref_ : this;
}

// virtual
Value Variable::Optimize(OptimizeContext* context) {
  return ref_.IsDefined()
      ? context->Optimize(ref_)
      : this;
}

// virtual
void Variable::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  if (ref_.IsDefined())
    ref_.Explore(ref_map);
}

// virtual
bool Variable::IsStateless(StatelessnessContext* context) {
  return ref_.IsDefined() && context->IsStateless(ref_);
}

// virtual
void Variable::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  if (!ref_.IsDefined())
    repr->append("_");
  else
    context->Encode(ref_, repr);
}

// virtual
void Variable::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// virtual
bool Variable::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  CHECK(!ref_.IsDefined());
  CHECK(ovalue != this);

  // Save this variable state.
  context->AddMutation(this);

  ref_ = ovalue;
  if (ovalue.type() == Value::VARIABLE) {
    Variable* ovar = ovalue.as<Variable>();
    CHECK(!ovar->ref_.IsDefined());
    // Save this other variable state too.
    context->AddMutation(ovar);

    // Transfer suspensions to the other free variable.
    ovar->suspensions()->splice(ovar->suspensions()->end(), suspensions_);

  } else {
    // Wake up suspensions if the unification succeeds.
    context->new_runnable.splice(context->new_runnable.end(), suspensions_);
  }
  return true;
}

bool Variable::BindTo(Value value) {
  CHECK(!ref_.IsDefined());
  CHECK(value != this);
  ref_ = value;
  if (value.type() == Value::VARIABLE) {
    Variable* ovar = value.as<Variable>();
    CHECK(!ovar->ref_.IsDefined());
    // Merge this free variable into the other free variable:
    // transfer its suspensions into the other variable.
    ovar->suspensions_.splice(ovar->suspensions_.end(), suspensions_);
    return false;

  } else {
    // This free variable is bound to a determined value.
    return true;  // Notify the caller to wake up the suspensions.
  }
}

void Variable::RevertToFree(SuspensionList* suspensions) {
  ref_ = NULL;
  suspensions_.swap(*suspensions);
}

}  // namespace store
