#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------
// Integer

const Value::ValueType Integer::kType;

// virtual
bool Integer::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  if (ovalue.type() != Value::INTEGER) return false;
  return value_ == ovalue.as<Integer>()->value_;
}

// virtual
bool Integer::Equals(EqualityContext* context, Value value) {
  return value_ == value.as<Integer>()->value_;
}

// virtual
HeapValue* Integer::MoveInternal(Store* store) {
  return Integer::New(store, value_);
}

// virtual
void Integer::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  string istr = value_.get_str(10);
  if (sgn(value_) < 0)
    istr[0] = '~';
  repr->append(istr);
}

// virtual
void Integer::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  // TODO: Big integers serialization
  pb->mutable_primitive()->set_integer(value());
}

}  // namespace store
