#ifndef STORE_MOVED_VALUE_H_
#define STORE_MOVED_VALUE_H_

namespace store {

// This is not really a value, but rather a tool supporting the Stop&Copy
// collection mechanism.
// Once a value is moved into another store, the former value memory block
// is reinitialized to a MovedValue, whose only purpose is to link to the new
// value location, until all reachable values have been moved.
// After the collection, there should be no MovedValue anymore in the live
// value graph.
class MovedValue : public HeapValue {
 public:
  static const Value::ValueType kType = Value::MOVED_VALUE;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline MovedValue* New(HeapValue* former_location,
                                HeapValue* new_location) {
    return new(CHECK_NOTNULL(former_location)) MovedValue(new_location);
  }

  // ---------------------------------------------------------------------------
  // Variable interface

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const throw() { return kType; }

  // This is not really a value.
  virtual Value Deref() { throw NotImplemented(); }
  virtual void ExploreValue(ReferenceMap* ref_map) { throw NotImplemented(); }
  virtual bool UnifyWith(UnificationContext* context, Value value) {
    throw NotImplemented();
  }

  // This value is just a placeholder for an already moved value.
  // We simply return the new value location.
  virtual Value Move(Store* store) {
    CHECK_NOTNULL(store);
    return CHECK_NOTNULL(new_location_);
  }

 private:  // ------------------------------------------------------------------

  // Initializes a new free variable.
  MovedValue(HeapValue* new_location)
      : new_location_(CHECK_NOTNULL(new_location)) {
  }

  virtual ~MovedValue() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  // New location of this value. Cannot be NULL.
  HeapValue* const new_location_;
};

}  // namespace store

#endif  // STORE_MOVED_VALUE_H_
