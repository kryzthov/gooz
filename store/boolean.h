#ifndef STORE_PRIMITIVES_H_
#define STORE_PRIMITIVES_H_

#include <string>

using std::string;

namespace store {

// -----------------------------------------------------------------------------
// Boolean
//
// Booleans are the specific atoms false and true.
//
// TODO: Implement the atom/record interface
// Should Boolean subclass Atom? Should it even exist in its own class?
// TODO: If we keep this class, we will need to normalize
// Atom::Get("false") and Atom::Get("true")
//
class Boolean : public HeapValue {
 public:
  static const ValueType kType = Value::BOOLEAN;

  // ---------------------------------------------------------------------------
  // Factory methods

  static inline
  Value Get(bool value) {
    return value ? KAtomTrue() : KAtomFalse();
  }

  static inline
  Boolean* GetBoolean(bool value) { return value ? True : False; }

  static Boolean* const True;
  static Boolean* const False;

  // ---------------------------------------------------------------------------
  // Boolean specific interface

  bool value() const { return value_; }
  Atom* atom() const { return atom_; }

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const throw() { return kType; }

  // ---------------------------------------------------------------------------
  // Implement serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  Boolean(bool value, const string& name);
  virtual ~Boolean() {}

  // ---------------------------------------------------------------------------
  const bool value_;
  const string name_;
  Atom* const atom_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_PRIMITIVES_H_
