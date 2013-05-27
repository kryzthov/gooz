#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

namespace store {

// -----------------------------------------------------------------------------

// virtual
Arity* HeapValue::OpenRecordArity(Store* store) {
  return RecordArity();
}

// virtual
uint64 HeapValue::OpenRecordWidth() {
  return RecordWidth();
}

// virtual
bool HeapValue::OpenRecordHas(Value feature) {
  return RecordHas(feature);
}

// virtual
Value HeapValue::OpenRecordGet(Value feature) {
  return RecordGet(feature);
}

// virtual
Value HeapValue::OpenRecordClose(Store* store) {
  return Value(this);
}

// -----------------------------------------------------------------------------

// virtual
Value HeapValue::RecordLabel() {
  throw NotImplemented();
}

// virtual
Arity* HeapValue::RecordArity() {
  throw NotImplemented();
}

// virtual
uint64 HeapValue::RecordWidth() {
  throw NotImplemented();
}

// virtual
bool HeapValue::RecordHas(Value feature) {
  throw NotImplemented();
}

// virtual
Value HeapValue::RecordGet(Value feature) {
  throw NotImplemented();
}

// virtual
Value::ItemIterator* HeapValue::RecordIterItems() {
  throw NotImplemented();
}

// virtual
Value::ValueIterator* HeapValue::RecordIterValues() {
  throw NotImplemented();
}

// virtual
Value HeapValue::TupleGet(uint64 index) {
  throw NotImplemented();
}

// virtual
uint64 HeapValue::LiteralHashCode() {
  throw NotImplemented();
}

// virtual
bool HeapValue::LiteralEquals(Value other) {
  throw NotImplemented();
}

// virtual
bool HeapValue::LiteralLessThan(Value other) {
  throw NotImplemented();
}

// virtual
Value::LiteralClass HeapValue::LiteralGetClass() {
  throw NotImplemented();
}

// virtual
Value HeapValue::Move(Store* store) {
  HeapValue* const new_location = MoveInternal(store);
  // Do not free this value memory block, it should belong to a Store.
  this->~HeapValue();
  CHECK_EQ(this, MovedValue::New(this, new_location));
  return new_location;
}

bool HeapValue::UnifyWith(UnificationContext* context, Value ovalue) {
  CHECK_NOTNULL(context);
  if (!ovalue.IsDetermined()) {
    LOG(WARNING) << __PRETTY_FUNCTION__ << " : unexpected unbound value.";
    return ovalue.UnifyWith(context, this);
  }
  return ovalue == this;
}

}  // namespace store
