#ifndef STORE_HEAP_VALUE_H_
#define STORE_HEAP_VALUE_H_

#include <list>
#include <memory>
#include <string>
using std::list;
using std::shared_ptr;
using std::string;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Abstract base class for all values represented with objects in the heap.

class HeapValue {
 protected:
  HeapValue() {}

 public:
  virtual ~HeapValue() {}

  // @returns the type of the value.
  virtual ValueType type() const throw() { return Value::INVALID; }

  template <class C>
  bool IsA() const throw() { return type() == C::kType; }

  // Dereferences this value.
  // @returns The dereferenced value.
  virtual Value Deref() { return this; }

  // @returns Whether this value is determined or not.
  virtual bool IsDetermined() { return true; }

  // ---------------------------------------------------------------------------
  // Value graph exploration

  inline void Explore(ReferenceMap* ref_map) {
    Value(this).Explore(ref_map);
  }

  // Explores the references from this value.
  // This value has already been registered in the reference map.
  // The default implementation is "do nothing".
  // @param ref_map The reference map to populate.
  virtual void ExploreValue(ReferenceMap* ref_map) {}

  // ---------------------------------------------------------------------------
  // Serialization

  // Shortcut
  inline string ToString() {
    return Value(this).ToString();
  }

  // Serializes the value to its ASCII representation with the given context.
  //
  // The values associated to true in the context reference map have already
  // been serialized under the variable name "V<value-address>".
  //
  // @param context The context for the serialization.
  // @param repr Returns the ASCII representation of this value.
  //     The representation is appended to the given string.
  virtual void ToASCII(ToASCIIContext* context, string* repr) {
    throw NotImplemented();
  }

  // Serializes this value to its protocol buffer representation.
  //
  // @param The protocol buffer to initialize with this value.
  virtual void ToProtoBuf(oz_pb::Value* value) {
    throw NotImplemented();
  }

  // ---------------------------------------------------------------------------
  // Unification

  // Unifies this value with another value.
  //
  // The default implementation only accepts unifying a value with itself.
  // This method should only be invoked through:
  //   static bool Value::Unify(context, v1, v2);
  //
  // @param context The unification context.
  // @param value The value to unify with this with. Must be dereferenced.
  // @returns True if the unification is successful, false otherwise.
  //     False means the unification transaction aborts.
  virtual bool UnifyWith(UnificationContext* context, Value value);

  // ---------------------------------------------------------------------------
  // Deep value equality

  // Tests whether this value is equal to another value.
  //
  // @param context Equality context. Keeps track of already tested value pairs.
  // @param value The value to test this value against.
  //     It is guaranteed that this != value.
  //     It is guaranteed that this and value have the same type.
  // @returns True if this and value are equals.
  virtual bool Equals(EqualityContext* context, Value value) {
    // The default behavior is that two values are equals if they are the same
    // physical entity (pointer equality).
    return false;
  }

  // ---------------------------------------------------------------------------
  // Statelessness

  virtual bool IsStateless(StatelessnessContext* context) {
    return true;
  }

  // ---------------------------------------------------------------------------
  // Value graph optimization

  // Optimizes the references contained in this value.
  // @returns An optimize reference for this value.
  virtual Value Optimize(OptimizeContext* context) { return this; }

  // ---------------------------------------------------------------------------
  // Support for Stop&Copy collection

  // Moves this value into another store, as well as all referenced values.
  // Prefer the term move over copy: for stateful values, there should be only
  // one instance, the previous instance will be destroyed!
  //
  // The default behavior is to overwrite this value with a MovedValue
  // after creating a copy of this value in the new store and "finalizing"
  // this value.
  //
  // @param store The store to move this value into.
  // @return The new value location.
  virtual Value Move(Store* store);

  // Copies this value into the given store, and moves its referenced values.
  // Meant to be invoked through Move().
  virtual HeapValue* MoveInternal(Store* store) { throw NotImplemented(); }

  // ---------------------------------------------------------------------------
  // Capacities

  // @returns The capabilities of the value as a set of enabled/disabled bits.
  virtual uint64 caps() const { return Value::CAP_NONE; }

  // ---------------------------------------------------------------------------
  // OpenRecord interface
  virtual Arity* OpenRecordArity(Store* store);
  virtual uint64 OpenRecordWidth();
  virtual bool OpenRecordHas(Value feature);
  virtual Value OpenRecordGet(Value feature);
  virtual Value OpenRecordClose(Store* store);

  // ---------------------------------------------------------------------------
  // Record interface
  virtual Value RecordLabel();
  virtual Arity* RecordArity();
  virtual uint64 RecordWidth();
  virtual bool RecordHas(Value feature);
  virtual Value RecordGet(Value feature);

  // @returns A new iterator on the record items, in order.
  // Caller must take ownership.
  virtual Value::ItemIterator* RecordIterItems();

  // @returns A new iterator on the record values, in order.
  // Caller must take ownership.
  virtual Value::ValueIterator* RecordIterValues();

  // ---------------------------------------------------------------------------
  // Tuple interface

  // @returns The request value from the tuple.
  // @param index The value index, ranging from 0 to size - 1.
  virtual Value TupleGet(uint64 index);

  // ---------------------------------------------------------------------------
  // Literal interface

  virtual uint64 LiteralHashCode();
  virtual bool LiteralEquals(Value other);
  virtual bool LiteralLessThan(Value other);
  virtual Value::LiteralClass LiteralGetClass();

 private:
  DISALLOW_COPY_AND_ASSIGN(HeapValue);
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_VALUE_H_
