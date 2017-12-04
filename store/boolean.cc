#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------
// Boolean

const Value::ValueType Boolean::kType;
Boolean* const Boolean::True = new Boolean(true, "true");
Boolean* const Boolean::False = new Boolean(false, "false");

Boolean::Boolean(bool value, const string& name)
    : value_(value), name_(name), atom_(NULL /*Atom::Get(name)*/) {
  // TODO: Fix static initialization!
}

void Boolean::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->append(name_);
}

void Boolean::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  pb->mutable_primitive()->set_type(oz_pb::Primitive::BOOLEAN);
  pb->mutable_primitive()->set_integer(static_cast<int>(value_));
}

// -----------------------------------------------------------------------------

}  // namespace store
