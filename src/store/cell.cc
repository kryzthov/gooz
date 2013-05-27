#include "store/values.h"

namespace store {

// -----------------------------------------------------------------------------
// Cell

const Value::ValueType Cell::kType;

// virtual
void Cell::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  ref_.Explore(ref_map);
}

// virtual
Value Cell::Optimize(OptimizeContext* context) {
  ref_ = context->Optimize(ref_);
  return this;
}

// virtual
void Cell::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->append("{NewCell ");
  context->Encode(ref_, repr);
  repr->append("}");
}

// virtual
void Cell::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// -----------------------------------------------------------------------------

}  // namespace store
