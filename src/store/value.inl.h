#ifndef STORE_VALUE_INL_H_
#define STORE_VALUE_INL_H_

#include <memory>
#include <vector>
using std::shared_ptr;
using std::vector;

namespace store {

// Deprecated factory methods

// static
inline
Value Value::Integer(int64 integer) {
  // Do not create values in store!
  CHECK(SmallInteger::IsSmallInt(integer));
  return SmallInteger(integer).Encode();
}

// static
inline
Value Value::Atom(const string& atom_str) {
  return Atom::Get(atom_str);
}

// -----------------------------------------------------------------------------

extern Atom* const kAtomEmpty;
extern Atom* const kAtomTrue;
extern Atom* const kAtomFalse;
extern Atom* const kAtomNil;
extern Atom* const kAtomList;
extern Atom* const kAtomTuple;

extern Arity* const kArityEmpty;
extern Arity* const kAritySingleton;
extern Arity* const kArityPair;

#if 1

inline Atom* KAtomEmpty() { return Atom::Get(""); }
inline Atom* KAtomTrue() { return Atom::Get("true"); }
inline Atom* KAtomFalse() { return Atom::Get("false"); }
inline Atom* KAtomNil() { return Atom::Get("nil"); }
inline Atom* KAtomList() { return Atom::Get("|"); }
inline Atom* KAtomTuple() { return Atom::Get("#"); }

inline Arity* KArityEmpty() { return Arity::GetTuple(0); }
inline Arity* KAritySingleton() { return Arity::GetTuple(1); }
inline Arity* KArityPair() { return Arity::GetTuple(2); }

#else

inline Atom* KAtomEmpty() { return kAtomEmpty; }
inline Atom* KAtomTrue() { return kAtomTrue; }
inline Atom* KAtomFalse() { return kAtomFalse; }
inline Atom* KAtomNil() { return kAtomNil; }
inline Atom* KAtomList() { return kAtomList; }
inline Atom* KAtomTuple() { return kAtomTuple; }

inline Arity* KArityEmpty() { return kArityEmpty; }
inline Arity* KAritySingleton() { return kAritySingleton; }
inline Arity* KArityPair() { return kArityPair; }

#endif

// -----------------------------------------------------------------------------

// static
inline
Value Value::Integer(Store* store, int64 integer) {
  if (SmallInteger::IsSmallInt(integer))
    return SmallInteger(integer).Encode();
  return Integer::New(store, integer);
}

// static
inline
Value Value::Integer(Store* store, const mpz_class& integer) {
  if (integer.fits_slong_p())
    return Integer(integer.get_si());
  return Integer::New(store, integer);
}

// static
inline
Value Value::Integer(Store* store, const string& integer, int base) {
  return Integer(store, mpz_class(integer, base));
}

// static
inline
Value Value::Record(Store* store, Value label, Arity* arity, Value* values) {
  if (arity->IsTuple()) return Tuple(store, label, arity->size(), values);
  return Record::New(store, label, arity, values);
}

// static
inline
Value Value::Record(Store* store, Value label, Arity* arity) {
  if (arity->IsTuple()) return Tuple(store, label, arity->size());
  return Record::New(store, label, arity);
}

// static
inline
Value Value::Tuple(Store* store, Value label, uint64 size, Value* values) {
  if (size == 0) return label;
  if ((size == 2) && (label == KAtomList()))
    return List(store, values[0], values[1]);
  return Tuple::New(store, label, size, values);
}

// static
inline
Value Value::Tuple(Store* store, Value label, uint64 size) {
  if (size == 0) return label;
  if ((size == 2) && (label == KAtomList()))
    return List(store, Variable::New(store), Variable::New(store));
  return Tuple::New(store, label, size);
}

// static
inline
Value Value::List(Store* store, Value head, Value tail) {
  return List::New(store, head, tail);
}

// -----------------------------------------------------------------------------

template <class T>
inline
bool Value::IsA() const {
  return type() == T::kType;
}

template <class T>
inline
T* Value::as() const {
  HeapValue* const value = heap_value();
  CHECK_EQ(T::kType, value->type());
  return static_cast<T*>(value);
}

inline
Value::ValueType Value::type() const {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->type();
    case kSmallIntTag: return SmallInteger::kType;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

inline
Value Value::Deref() const {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->Deref();
    case kSmallIntTag: return *this;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

inline
bool Value::IsDetermined() {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->IsDetermined();
    case kSmallIntTag: return true;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

inline
uint64 Value::caps() const {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->caps();
    case kSmallIntTag: return SmallInteger(*this).caps();
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

// -----------------------------------------------------------------------------
// OpenRecord interface

inline
Arity* Value::OpenRecordArity(Store* store) {
  CHECK(IsHeapValue());
  return heap_value_->OpenRecordArity(store);
}

inline
uint64 Value::OpenRecordWidth() {
  CHECK(IsHeapValue());
  return heap_value_->OpenRecordWidth();
}

inline
bool Value::OpenRecordHas(Value feature) {
  CHECK(IsHeapValue());
  return heap_value_->OpenRecordHas(feature);
}

inline
Value Value::OpenRecordGet(Value feature) {
  CHECK(IsHeapValue());
  return heap_value_->OpenRecordGet(feature);
}

inline
Value Value::OpenRecordClose(Store* store) {
  CHECK(IsHeapValue());
  return heap_value_->OpenRecordClose(store);
}

// -----------------------------------------------------------------------------
// Record interface

inline
Value Value::RecordLabel() {
  CHECK(IsHeapValue());
  return heap_value_->RecordLabel();
}

inline
Arity* Value::RecordArity() {
  CHECK(IsHeapValue());
  return heap_value_->RecordArity();
}

inline
uint64 Value::RecordWidth() {
  CHECK(IsHeapValue());
  return heap_value_->RecordWidth();
}

inline
bool Value::RecordHas(Value feature) {
  CHECK(IsHeapValue());
  return heap_value_->RecordHas(feature);
}

inline
Value Value::RecordGet(Value feature) {
  CHECK(IsHeapValue());
  return heap_value_->RecordGet(feature);
}

inline
Value::ItemIterator* Value::RecordIterItems() {
  CHECK(IsHeapValue());
  return heap_value_->RecordIterItems();
}

inline
Value::ValueIterator* Value::RecordIterValues() {
  CHECK(IsHeapValue());
  return heap_value_->RecordIterValues();
}

// -----------------------------------------------------------------------------
// Tuple interface

inline
Value Value::TupleGet(uint64 index) {
  CHECK(IsHeapValue());
  return heap_value_->TupleGet(index);
}

// -----------------------------------------------------------------------------
// Literal interface

inline
uint64 Value::LiteralHashCode() {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->LiteralHashCode();
    case kSmallIntTag: return SmallInteger(*this).LiteralHashCode();
  }
  throw NotImplemented();
}

inline
bool Value::LiteralEquals(Value other) {
  if (bits_ == other.bits_) return true;
  if (tag() != other.tag()) return false;
  switch (tag()) {
    case kHeapValueTag: return heap_value_->LiteralEquals(other.heap_value_);
    case kSmallIntTag: return false;
  }
  LOG(FATAL) << "Unexpected value type: tag=" << tag();
}

inline
bool Value::LiteralLessThan(Value other) {
  const Value::LiteralClass class1 = LiteralGetClass();
  const Value::LiteralClass class2 = other.LiteralGetClass();
  if (class1 == class2) {
    switch (tag()) {
      case kHeapValueTag: return heap_value_->LiteralLessThan(other);
      case kSmallIntTag: return SmallInteger(*this).LiteralLessThan(other);
    }
    throw NotImplemented();
  } else {
    return (class1 < class2);
  }
}

inline
Value::LiteralClass Value::LiteralGetClass() {
  switch (tag()) {
    case kHeapValueTag: return heap_value_->LiteralGetClass();
    case kSmallIntTag: return LITERAL_CLASS_INTEGER;
  }
  throw NotImplemented();
}

// -----------------------------------------------------------------------------

class New {
 public:

  static inline
  Value Free(Store* store) {
    return store::Variable::New(store);
  }

  static inline
  Value Integer(Store* store, int64 integer) {
    if (SmallInteger::IsSmallInt(integer))
      return SmallInteger(integer).Encode();
    return store::Integer::New(store, integer);
  }

  static inline
  Value Integer(Store* store, const mpz_class& integer) {
    if (integer.fits_slong_p())
      return Integer(store, integer.get_si());
    return store::Integer::New(store, integer);
  }

  static inline
  Value Integer(Store* store, const string& integer, int base) {
    return Integer(store, mpz_class(integer, base));
  }

  static inline
  Value Atom(Store* store, const string& atom) {
    return store::Atom::Get(atom);
  }

  static inline
  Value Name(Store* store) {
    return store::Name::New(store);
  }

  static inline
  Value String(Store* store, const string& str) {
    return store::String::Get(store, str);
  }

  static inline
  Value Real(Store* store, const base::real::Real& real) {
    LOG(FATAL) << "not implemented";
    // return store::String::Real(store, real);
  }

  static inline
  Value TupleArity(Store* store, uint64 size) {
    return store::Arity::GetTuple(size);
  }

  static inline
  Value Arity(Store* store, const vector<Value>& literals) {
    return store::Arity::Get(literals);
  }

  static inline
  Value Arity(Store* store, uint64 size, Value const * literals) {
    return store::Arity::New(store, size, literals);
  }

  static inline
  Value Arity(Store* store, Value literal1) {
    return store::Arity::Get(literal1);
  }

  static inline
  Value Arity(Store* store, Value literal1, Value literal2) {
    return store::Arity::Get(literal1, literal2);
  }

  static inline
  Value Arity(Store* store, Value literal1, Value literal2, Value literal3) {
    return store::Arity::Get(literal1, literal2, literal3);
  }

  static inline
  Value OpenRecord(Store* store, Value label) {
    return store::OpenRecord::New(store, label);
  }

  static inline
  Value Record(Store* store, Value label, store::Arity* arity, Value* values) {
    if (arity->IsTuple())
      return Tuple(store, label, arity->size(), values);
    return store::Record::New(store, label, arity, values);
  }

  static inline
  Value Record(Store* store, Value label, store::Arity* arity) {
    if (arity->IsTuple())
      return Tuple(store, label, arity->size());
    return store::Record::New(store, label, arity);
  }

  static inline
  Value Tuple(Store* store, uint64 size, Value* values) {
    Value label = KAtomTuple();
    if (size == 0) return label;
    return store::Tuple::New(store, label, size, values);
  }

  static inline
  Value Tuple(Store* store, Value label, uint64 size, Value* values) {
    if (size == 0) return label;
    if ((size == 2) && (label == KAtomList()))
      return List(store, values[0], values[1]);
    return store::Tuple::New(store, label, size, values);
  }

  static inline
  Value Tuple(Store* store, Value label, uint64 size) {
    if (size == 0) return label;
    if ((size == 2) && (label == KAtomList()))
      return List(store, Free(store), Free(store));
    return store::Tuple::New(store, label, size);
  }

  static inline
  Value List(Store* store, Value head, Value tail) {
    return store::List::New(store, head, tail);
  }

  static inline
  Value List(Store* store, uint64 nvalues, Value* values) {
    Value value = KAtomNil();
    for (int64 i = nvalues - 1; i >= 0; --i)
      value = List(store, values[i], value);
    return value;
  }

  static inline
  Value Cell(Store* store, Value initial) {
    return store::Cell::New(store, initial);
  }

  static inline
  Value Array(Store* store, uint64 size, Value initial) {
    return store::Array::New(store, size, initial);
  }

  static inline
  Value Closure(Store* store,
                const shared_ptr<vector<Bytecode> >& bytecode,
                int nparams, int nlocals, int nclosures) {
    return store::Closure::New(store, bytecode, nparams, nlocals, nclosures);
  }

  static inline
  Value Closure(Store* store,
                const store::Closure* closure,
                store::Array* environment) {
    return store::Closure::New(store, closure, environment);
  }

  static inline
  Value Thread(Store* store, Engine* engine, store::Closure* closure,
               store::Array* parameters, Store* thread_store) {
    return store::Thread::New(store, engine, closure, parameters, thread_store);
  }

 private:
  // This class has no instance. It is a collection of factory methods.
  New();
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_VALUE_INL_H_
