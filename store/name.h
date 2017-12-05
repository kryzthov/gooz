#ifndef STORE_NAME_H_
#define STORE_NAME_H_

#include <string>
using std::string;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Names
//
// An name is an ID that cannot be forged and guaranteed unique.
// For now, it is unique within the process.
//
class Name : public HeapValue {
 public:
  static const Value::ValueType kType = Value::NAME;

  // --------------------------------------------------------------------------
  // Factory methods
  static inline Name* New(Store* store) {
    return new(CHECK_NOTNULL(store->Alloc<Name>())) Name();
  }

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const noexcept { return kType; }
  virtual uint64 caps() const { return Value::CAP_RECORD; }

  virtual HeapValue* MoveInternal(Store* store);

  // --------------------------------------------------------------------------
  // Record interface
  virtual Value RecordLabel() { return this; }
  virtual Arity* RecordArity();
  virtual uint64 RecordWidth() { return 0; }
  virtual bool RecordHas(Value feature) { return false; }
  virtual Value RecordGet(Value feature);

  // @returns A new iterator. The caller must take ownership.
  virtual Value::ItemIterator* RecordIterItems() {
    return new EmptyItemIterator();
  }
  // @returns A new iterator. The caller must take ownership.
  virtual Value::ValueIterator* RecordIterValues() {
    return new EmptyValueIterator();
  }

  // --------------------------------------------------------------------------
  // Literal interface

  virtual uint64 LiteralHashCode()  { return id_; }
  virtual bool LiteralEquals(Value other) {
    return (other.type() == Value::NAME)
        && (id_ == other.as<Name>()->id_);
  }
  virtual bool LiteralLessThan(Value other)  {
    const Value::LiteralClass tclass = LiteralGetClass();
    const Value::LiteralClass oclass = other.LiteralGetClass();
    return (oclass == LiteralGetClass())
        ? (id_ < other.as<Name>()->id_)
        : (tclass < oclass);
  }
  virtual Value::LiteralClass LiteralGetClass() {
    return Value::LITERAL_CLASS_NAME;
  }

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------
  static uint64 next_id_;
  static uint64 GetNextId();

  Name() : id_(GetNextId()) {
  }

  // To copy an existing name into another store.
  Name(uint64 id) : id_(id) {
  }

  virtual ~Name() {
  }

  // ---------------------------------------------------------------------------
  // Memory layout
  const uint64 id_;
};

}  // namespace store

#endif  // STORE_NAME_H_
