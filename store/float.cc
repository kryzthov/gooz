#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------
// Float

const Value::ValueType Float::kType;

// virtual
bool Float::UnifyWith(UnificationContext* context, Value value) {
  CHECK_NOTNULL(context);
  return (value.type() == Value::FLOAT)
      && (value_ == value.as<Float>()->value_);
}

// virtual
bool Float::Equals(EqualityContext* context, Value value) {
  return value_ == value.as<Float>()->value_;
}

// virtual
void Float::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->append((format("%f") % value_).str());
}

// virtual
void Float::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  pb->mutable_primitive()->set_type(oz_pb::Primitive::FLOAT);
  LOG(FATAL) << "Not implemented";
}

// -----------------------------------------------------------------------------

}  // namespace store
