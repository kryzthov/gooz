#include "store/values.h"

#include "base/escaping.h"

namespace store {

// -----------------------------------------------------------------------------
// String

const Value::ValueType String::kType;

const char* const kDoubleQuotes = "\"";

// virtual
bool String::UnifyWith(UnificationContext* context, Value value) {
  CHECK_NOTNULL(context);
  return (value.type() == Value::STRING)
      && (value_ == value.as<String>()->value_);
}

// virtual
bool String::Equals(EqualityContext* context, Value value) {
  return value_ == value.as<String>()->value_;
}

// virtual
void String::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->push_back('"');
  repr->append(base::Escape(value_, kDoubleQuotes));
  repr->push_back('"');
}

// virtual
void String::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  pb->mutable_primitive()->set_type(oz_pb::Primitive::STRING);
  pb->mutable_primitive()->set_text(value_);
}

// -----------------------------------------------------------------------------

}  // namespace store

