#include "store/values.h"

namespace store {

// -----------------------------------------------------------------------------
// Array

const Value::ValueType Array::kType;
Array* const Array::EmptyArray = new Array(0, Value::Integer(0));

// virtual
void Array::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  for (uint64 i = 0; i < size_; ++i) {
    values_[i].Explore(ref_map);
  }
}

// virtual
Value Array::Optimize(OptimizeContext* context) {
  for (uint64 i = 0; i < size_; ++i)
    values_[i] = context->Optimize(values_[i]);
  return this;
}

// virtual
void Array::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  repr->append("{NewArray array(");
  if (size_ > 0)
    context->Encode(values_[0], repr);
  for (uint64 i = 1; i < size_; ++i) {
    repr->append(" ");
    context->Encode(values_[i], repr);
  }
  repr->append(")}");
}

// virtual
void Array::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// -----------------------------------------------------------------------------

}  // namespace store
