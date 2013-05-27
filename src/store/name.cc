#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------
// Name

const Value::ValueType Name::kType;

// static
uint64 Name::next_id_ = 0;

// static
uint64 Name::GetNextId() {
  return next_id_++;
}

// virtual
HeapValue* Name::MoveInternal(Store* store) {
  return new(CHECK_NOTNULL(store->Alloc<Name>())) Name(id_);
}

// virtual
Arity* Name::RecordArity() { return KArityEmpty(); }

// virtual
Value Name::RecordGet(Value feature) {
  throw FeatureNotFound(feature, KArityEmpty());
}

void Name::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->append("{NewName}");
}

void Name::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  pb->mutable_primitive()->set_type(oz_pb::Primitive::NAME);
  pb->mutable_primitive()->set_integer(id_);
}

// -----------------------------------------------------------------------------

}  // namespace store
