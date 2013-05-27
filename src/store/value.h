#ifndef STORE_VALUE_H_
#define STORE_VALUE_H_

#include <exception>
#include <list>
#include <memory>
#include <string>
#include <vector>
using std::list;
using std::shared_ptr;
using std::string;
using std::vector;

#include <glog/logging.h>

#include <gmpxx.h>

#include "base/macros.h"
#include "base/stl-util.h"
#include "proto/store.pb.h"


namespace store {

// -----------------------------------------------------------------------------
// Forward class declarations

// Abstract value classes and interfaces
class Value;
class Store;
class HeapValue;
class MovedValue;

// Concrete value classes
class Name;
class Boolean;
class Integer;
class Atom;
class Float;
class String;

class Arity;
class ArityMap;

class OpenRecord;
class Record;
class Tuple;
class List;

class Variable;
class Cell;
class Array;
class Port;
class Closure;
class Thread;

class Type;
class TypeVariable;

// -----------------------------------------------------------------------------
// Class declarations

// Raised when an abstract method is invoked.
class NotImplemented : public std::exception {
};

// Raised when using an iterator past end.
class IteratorAtEnd : public std::exception {
};

typedef list<Thread*> SuspensionList;

// Raised when an operation results in suspensing the current thread.
class SuspendThread : public std::exception {
 public:
  SuspendThread(SuspensionList* slist)
      : suspensions(slist) {
  }

  // If suspending a thread, append it to this suspensions list.
  SuspensionList* suspensions;
};

// Raised when resolving a feature that does not exist.
class FeatureNotFound : public std::exception {
 public:
  FeatureNotFound(const string& message) : message_(message) {}
  FeatureNotFound(const Value& feature, Arity* arity);
  virtual ~FeatureNotFound() throw() {}
  virtual const char* what() const throw() {
    return message_.c_str();
  }

 private:
  const string message_;
};

// Iterator interface
// Typical usage:
//   unique_ptr<Iterator<T>> it_deleter(value->NewIterator());
//   for (Iterator<T>& it = *it_deleter; !it.at_end(); ++it) {
//     T& t = *it;
//   }
template <typename T>
class Iterator {
 public:
  Iterator() {}
  virtual ~Iterator() {}
  virtual T operator*() = 0;  // throws(IteratorAtEnd)
  virtual Iterator& operator++() = 0;
  virtual bool at_end() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Iterator);
};

// -----------------------------------------------------------------------------

// Targeting 64 bits machines
const uint64 kWordSize = 64;

// Assuming heap memory blocs are always aligned on 64 bits words.
const int kTagBits = 3;
const uint64 kTagBitMask = (1 << kTagBits) - 1;

enum ValueTag {
  kHeapValueTag = 0x00,
  kSmallIntTag = 0x01,
};

const int kSignedIntBits = kWordSize - 1;
const int64 kSmallIntMax = (1L << (kSignedIntBits - kTagBits)) - 1;
const int64 kSmallIntMin = - (1L << (kSignedIntBits - kTagBits));

// -----------------------------------------------------------------------------

#define DEPRECATED __attribute__((deprecated))

class ReferenceMap;
class ToASCIIContext;
class UnificationContext;
class EqualityContext;
class StatelessnessContext;
class OptimizeContext;

class Value {
 public:
  // ValueType indicates the nature of the underlying C++ object.
  enum ValueType {
    MOVED_VALUE = -2,
    INVALID     = -1,
    // Primitive values
    INTEGER     = 1,
    NAME        = 2,
    ATOM        = 3,
    STRING      = 4,
    FLOAT       = 5,
    BOOLEAN     = 6,

    // Composite/Compound values
    ARITY       = 7,
    ARITY_MAP   = 20,

    LIST        = 8,
    TUPLE       = 9,
    RECORD      = 10,
    OPEN_RECORD = 11,
    CELL        = 12,
    ARRAY       = 13,
    // Other
    VARIABLE    = 14,
    PORT        = 15,
    CLOSURE     = 16,

    TYPE        = 17,
    TYPE_VARIABLE = 18,

    SMALL_INTEGER = 19,  // Not a heap value
  };

  struct ValueHash {
    inline size_t operator()(Value value) const { return value.bits_; }
  };
  typedef Iterator<Value> ValueIterator;
  typedef std::pair<Value, Value> ValuePair;
  typedef Iterator<ValuePair> ItemIterator;

  // ---------------------------------------------------------------------------
  // Normalizing factory methods

  // ----------------------------------------
  // Deprecated factory functions, use store::New::XXX instead.

  // This factory is intended to generate small integers only!
  // TODO: Make it so, through a CHECK.
  static Value Integer(int64 integer);

  // TODO: Figure out a way to handle Atoms properly.
  static Value Atom(const string& atom_str);

  static Value Integer(Store* store, int64 integer);
  static Value Integer(Store* store, const mpz_class& integer);
  static Value Integer(Store* store, const string& integer, int base = 10);

  static Value Record(Store* store,
                      Value label, store::Arity* arity, Value* values);
  static Value Record(Store* store, Value label, store::Arity* arity);
  static Value Tuple(Store* store, Value label, uint64 size, Value* values);
  static Value Tuple(Store* store, Value label, uint64 size);
  static Value List(Store* store, Value head, Value tail);

  // ---------------------------------------------------------------------------
  // Constructors and copy/assignment.

  Value() : bits_(0) {}
  explicit Value(uint64 bits) : bits_(bits) {}
  Value(const Value& other) : bits_(other.bits_) {}
  Value(HeapValue* heap_value) : heap_value_(heap_value) {
    CHECK_EQ(kHeapValueTag, tag());
  }

  inline
  Value& operator=(const Value& other) {
    bits_ = other.bits_;
    return *this;
  }

  inline
  Value& operator=(HeapValue* heap_value) {
    heap_value_ = heap_value;
    CHECK_EQ(kHeapValueTag, tag());
    return *this;
  }

  // ---------------------------------------------------------------------------

  inline
  bool operator==(Value other) const {
    return bits_ == other.bits_;
  }

  inline
  bool operator!=(Value other) const {
    return !operator==(other);
  }

  // ---------------------------------------------------------------------------

  template <class T>
  bool IsA() const;

  template <class T>
  T* as() const;

  inline ValueType type() const;
  inline ValueTag tag() const {
    return static_cast<ValueTag>(bits_ & kTagBitMask);
  }

  // @returns Whether the value is defined or not.
  inline bool IsDefined() const {
    return bits_ != 0;
  }

  inline bool IsHeapValue() const { return tag() == kHeapValueTag; }
  inline bool IsSmallInt() const { return tag() == kSmallIntTag; }

  // Dereferences this value.
  // @returns The dereferenced value.
  Value Deref() const;

  // @returns Whether this value is determined or not.
  bool IsDetermined();

  // @returns This value as a raw uint64.
  inline uint64 bits() const { return bits_; }

  // May return NULL.
  inline
  HeapValue* heap_value() const {
    CHECK(IsHeapValue());
    return heap_value_;
  }

  // ---------------------------------------------------------------------------
  // Value graph exploration

  // Explores the value graph and builds the references map.
  // Does not modify/optimize the value graph.
  // This method is generic accross almost all values (except variables).
  // It relies on each value type overriding ExploreValue() to walk through
  // their references.
  // @param ref_map The reference map to populate.
  void Explore(ReferenceMap* ref_map) const;

  // ---------------------------------------------------------------------------
  // Serialization

  // @returns An ASCII representation of this value.
  //     The representation may have the form of a sequence of definitions,
  //     for example, when describing a recursive value.
  inline string ToString() const {
    string repr;
    ToString(&repr);
    return repr;
  }
  void ToString(string* repr) const;

  // Serializes the value to its ASCII representation with the given context.
  //
  // The values associated to true in the context reference map have already
  // been serialized under the variable name "V<value-address>".
  //
  // @param context The context for the serialization.
  // @param repr Returns the ASCII representation of this value.
  //     The representation is appended to the given string.
  void ToASCII(ToASCIIContext* context, string* repr) const;

  // Serializes this value to its protocol buffer representation.
  //
  // @param The protocol buffer to initialize with this value.
  void ToProtoBuf(oz_pb::Value* value) const;

  // ---------------------------------------------------------------------------
  // Unification

  // Internal unification entry-point.
  //
  // Adds a unification in a given unification transaction.
  // Keeps track of the value graph exploration.
  // Delegates the real unification operation to Value::UnifyWith().
  // Favors Variable::UnifyWith(<determined value>).
  //
  // @param context The unification transaction context.
  // @param value1 An arbitrary Oz value.
  // @param value2 Another arbitrary Oz value.
  static bool Unify(UnificationContext* context, Value value1, Value value2);

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
  bool UnifyWith(UnificationContext* context, Value value);

  // ---------------------------------------------------------------------------
  // Deep value equality

  // Tests whether this value is equal to another value.
  //
  // @param context Equality context. Keeps track of already tested value pairs.
  // @param value The value to test this value against.
  //     It is guaranteed that this != value.
  //     It is guaranteed that this and value have the same type.
  // @returns True if this and value are equals.
  bool Equals(EqualityContext* context, Value value) const;

  // ---------------------------------------------------------------------------
  // Statelessness

  bool IsStateless(StatelessnessContext* context) const;

  // ---------------------------------------------------------------------------
  // Value graph optimization

  // Optimizes the references contained in this value.
  // @returns An optimize reference for this value.
  Value Optimize(OptimizeContext* context);

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
  Value Move(Store* store);

  // ---------------------------------------------------------------------------
  // Capacities

  static const uint64 CAP_NONE = 0;
  enum Capabilities {
    // Literals: hash-code, equals and less-than.
    ICAP_LITERAL,

    // Value compatible with records: labels, arity, etc,
    // with some restrictions (see below).
    ICAP_RECORD,

    // Open-records have no arity yet.
    ICAP_ARITY,

    // Chunks do not reveal the arity.
    ICAP_ARITY_VISIBLE,

    // Tuples have fast indexed access.
    ICAP_TUPLE,
  };
  enum CapabilityBits {
    CAP_LITERAL = 1 << ICAP_LITERAL,
    CAP_RECORD = 1 << ICAP_RECORD,
    CAP_ARITY = 1 << ICAP_ARITY,
    CAP_ARITY_VISIBLE = 1 << ICAP_ARITY_VISIBLE,
    CAP_TUPLE = 1 << ICAP_TUPLE,
  };

  // @returns The capabilities of the value as a set of enabled/disabled bits.
  uint64 caps() const;

  // ---------------------------------------------------------------------------
  // OpenRecord interface

  // @returns The current arity of the open-record. Non blocking.
  Arity* OpenRecordArity(Store* store);

  // @returns The current width of the open-record. Non blocking.
  uint64 OpenRecordWidth();

  // Tests a feature existence in the open-record. Non blocking.
  bool OpenRecordHas(Value feature);

  // Looks a feature from the open-record up. Non-blocking.
  Value OpenRecordGet(Value feature);

  // Closes this open-record.
  Value OpenRecordClose(Store* store);

  // ---------------------------------------------------------------------------
  // Record interface

  // @returns The record label. Non-blocking.
  Value RecordLabel();

  // @returns The record arity.
  //     Blocks for an open-record until it is closed.
  Arity* RecordArity();

  // @returns The record width.
  //     Blocks for an open-record until it is closed.
  uint64 RecordWidth();

  // Tests a feature existence in the record.
  //     Blocks for an open-record until it is closed.
  bool RecordHas(Value feature);

  // Looks a feature from the record up.
  //     Blocks for an open-record until it is closed.
  Value RecordGet(Value feature);

  // @returns A new iterator on the record items, in order.
  // Caller must take ownership.
  ItemIterator* RecordIterItems();
  // @returns A new iterator on the record values, in order.
  // Caller must take ownership.
  ValueIterator* RecordIterValues();

  // ---------------------------------------------------------------------------
  // Tuple interface

  // @returns The request value from the tuple.
  // @param index The value index, ranging from 0 to size - 1.
  Value TupleGet(uint64 index);

  // ---------------------------------------------------------------------------
  // Literal interface

  // Ordered literal classes
  enum LiteralClass {
    LITERAL_CLASS_INTEGER = 1,
    LITERAL_CLASS_ATOM = 2,
    LITERAL_CLASS_NAME = 3,
  };

  uint64 LiteralHashCode();
  bool LiteralEquals(Value other);
  bool LiteralLessThan(Value other);
  LiteralClass LiteralGetClass();

 private:
  // Memory layout
  // This object must be lightweight and fit in a native word of the target
  // execution environment.
  // For now, we target 64 bits machine.
  union {
    uint64 bits_;
    HeapValue* heap_value_;
  };

  friend class std::hash<Value>;
};

}  // namespace store

// -----------------------------------------------------------------------------

namespace std {
template<>
struct hash<store::Value> : public __hash_base<size_t, store::Value> {
  inline size_t operator()(store::Value value) const noexcept {
    return value.bits_;
  }
};
}  // namespace std

// -----------------------------------------------------------------------------

namespace store {

typedef Value::ValueType ValueType;

// typedef Value::ValueIterator ValueIterator;
typedef Value::ValuePair ValuePair;
// typedef Value::ItemIterator ItemIterator;

// A reference map contains all the values in the transitive closure of
// a given set of values.
// A value in the map is associated to true when it appears multiple times in
// the transitive closure; it is associated to false when it appears exactly
// once.
class ReferenceMap : public UnorderedMap<Value, bool> {};

typedef SymmetricPair<Value, Value> SymmetricValuePair;
typedef UnorderedSet<SymmetricValuePair, SymmetricPairHash<Value> >
    SymmetricValuePairSet;

class ToASCIIContext {
 public:
  ToASCIIContext() {}

  // Appends an ASCII representation of a value to the given string.
  // Use references whenever the context indicates the value is known already.
  void Encode(Value value, string* repr);

  ReferenceMap ref_map;

 private:
  DISALLOW_COPY_AND_ASSIGN(ToASCIIContext);
};

class UnificationContext {
 public:
  UnificationContext() {}

  // Adds a value pair in the unification context.
  // All value pairs already registered in the context are assumed
  // done already and will not be processed again.
  //
  // @param value1, value2 The value pair.
  // @returns True if the pair (value1, value2) was unknown before
  //     and has to be processed.
  bool Add(Value value1, Value value2) {
    return done.insert(SymmetricValuePair(value1, value2)).second;
  }

  // Adds the given variable to the mutation pool.
  // No-op if the variable already exists in the pool.
  void AddMutation(Variable* var);

  // All the pair of values that have been examined already.
  SymmetricValuePairSet done;

  // Set of variables modified as part of the unification transaction.
  // Modified variable are mapped to their initial suspensions list.
  typedef UnorderedMap<Variable*, shared_ptr<SuspensionList> > MutationMap;
  MutationMap mutations;

  // Threads to wake up if the unification succeeds.
  SuspensionList new_runnable;

 private:
  DISALLOW_COPY_AND_ASSIGN(UnificationContext);
};

class EqualityContext {
 public:
  EqualityContext() {}

  // @returns True if value1 and value2 are equals.
  // Does not re-test if the pair (value1, value2) has already been processed.
  bool Equals(Value value1, Value value2);

  bool Add(Value value1, Value value2) {
    return done.insert(SymmetricValuePair(value1, value2)).second;
  }

  // All the pair of values that have been examined already.
  SymmetricValuePairSet done;

 private:
  DISALLOW_COPY_AND_ASSIGN(EqualityContext);
};

class StatelessnessContext {
 public:
  StatelessnessContext() {}

  // @return True if the given value is stateless.
  bool IsStateless(Value value);

 private:
  UnorderedSet<Value> ref_map_;

  DISALLOW_COPY_AND_ASSIGN(StatelessnessContext);
};

class OptimizeContext {
 public:
  OptimizeContext() {}

  // Optimizes the references graph of a value, if not done already.
  // @returns A reference to the optimized value.
  Value Optimize(Value value);

 private:
  UnorderedSet<Value> ref_map_;

  DISALLOW_COPY_AND_ASSIGN(OptimizeContext);
};

// -----------------------------------------------------------------------------

// Iterator for an empty set of items.
class EmptyItemIterator : public Value::ItemIterator {
 public:
  EmptyItemIterator() {
  }

  virtual ~EmptyItemIterator() {}

  virtual ValuePair operator*() {
    throw IteratorAtEnd();
  }

  virtual EmptyItemIterator& operator++() {
    return *this;
  }

  virtual bool at_end() {
    return true;
  }
};

// Iterator for an empty set of values.
class EmptyValueIterator : public Value::ValueIterator {
 public:
  EmptyValueIterator() {
  }

  virtual ~EmptyValueIterator() {}

  virtual Value operator*() {
    throw IteratorAtEnd();
  }

  virtual EmptyValueIterator& operator++() {
    return *this;
  }

  virtual bool at_end() {
    return true;
  }
};

// -----------------------------------------------------------------------------

// Initializes the library.
void Initialize();

// Unifies two arbitrary Oz values.
//
// The unification operation is transactional (all or nothing).
//
// @param value1 A value (maybe unbound).
// @param value2 Another value (maybe unbound).
// @param suspensions Fills this list with the suspensions to wake up.
// @returns True if successful.
bool Unify(Value value1, Value value2, list<Thread*>* suspensions);

// Simplified Unify() when threads are not involved.
inline
bool Unify(Value value1, Value value2) {
  list<Thread*> suspensions;
  const bool unified = Unify(value1, value2, &suspensions);
  CHECK(suspensions.empty());
  return unified;
}

// Dereferences an arbitrary value.
//
// The returned dereferenced value is:
//  - either an unbound free variable,
//  - or a determined value.
//
// @param An arbitrary value reference.
// @returns The leaf linked value.
inline
Value Deref(Value value) { return value.Deref(); }

// @returns True if the given value has the specified type.
inline
bool HasType(Value value, Value::ValueType type) {
  return value.type() == type;
}


// @returns Whether a value is determined or not.
// @param value The value to test.
inline
bool IsDet(Value value) {
  return Deref(value).IsDetermined();
}

// Tests whether the given value graph is stateless.
// A value is stateless if it is determined and immutable.
bool IsStateless(Value value);

// Optimizes the given value graph.
Value Optimize(Value value);

// Tests whether two values are equals.
// Two values are said equal if it is guaranteed they will always represent
// the same information.
// The semantic is equivalent to Unify(value1, value2) except it does
// not alter values to make them equals.
bool Equals(Value value1, Value value2);

// @returns The value of an Oz integer.
int64 IntValue(Value value);

// -----------------------------------------------------------------------------

Atom* KAtomEmpty();
Atom* KAtomTrue();
Atom* KAtomFalse();
Atom* KAtomNil();
Atom* KAtomList();
Atom* KAtomTuple();

Arity* KArityEmpty();
Arity* KAritySingleton();
Arity* KArityPair();

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_VALUE_H_
