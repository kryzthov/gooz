// Atom strings.

#ifndef STORE_ATOM_H_
#define STORE_ATOM_H_

#include <string>
#include <unordered_map>
using std::pair;
using std::string;
using std::unordered_multimap;

#include <boost/regex.hpp>

#include "base/macros.h"
#include "base/string_piece.h"
using base::StringPiece;

namespace store {

// -----------------------------------------------------------------------------
// Atom
//
// Atoms are interned in a table.
//
class Atom : public HeapValue {
 public:
  static const Value::ValueType kType = Value::ATOM;
  static const boost::regex kSimpleAtomRegexp;

  // Escapes a raw atom.
  static string Escape(const StringPiece& raw_atom);
  // Unescapes an escaped atom.
  static string Unescape(const StringPiece& escaped_atom);

  // @returns The atom with the specified non escaped text.
  // @param atom The atom text.
  static Atom* Get(const StringPiece& atom);

  // @returns The atom with the specified escaped text.
  // @param atom The escaped atom text (including the single quotes).
  static Atom* GetEscaped(const StringPiece& atom);

  const string& value() const { return value_; }
  const uint64 hash() const { return hash_; }

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const throw() { return kType; }
  virtual uint64 caps() const {
    return Value::CAP_RECORD | Value::CAP_TUPLE | Value::CAP_LITERAL;
  }

  // Atoms are interned.
  virtual Value Move(Store* store) { return this; }

  // ---------------------------------------------------------------------------
  // Record interface
  virtual Value RecordLabel() { return this; }
  virtual Arity* RecordArity();
  virtual uint64 RecordWidth() { return 0; }
  virtual bool RecordHas(Value feature) { return false; }
  virtual Value RecordGet(Value feature);

  virtual Iterator<ValuePair>* RecordIterItems() {
    return new EmptyItemIterator();
  }
  virtual Iterator<Value>* RecordIterValues() {
    return new EmptyValueIterator();
  }

  // ---------------------------------------------------------------------------
  // Literal interface

  virtual uint64 LiteralHashCode() { return hash_; }
  virtual bool LiteralEquals(Value other) {
    return Value(this) == other;  // atoms are interned.
  }
  virtual bool LiteralLessThan(Value other) {
    const Value::LiteralClass tclass = LiteralGetClass();
    const Value::LiteralClass oclass = other.LiteralGetClass();
    return (oclass == LiteralGetClass())
        ? (value_ < other.as<Atom>()->value_)
        : (tclass < oclass);
  }
  virtual Value::LiteralClass LiteralGetClass() {
    return Value::LITERAL_CLASS_ATOM;
  }

  // ---------------------------------------------------------------------------
  // Tuple interface
  virtual Value TupleGet(uint64 index) {
    throw FeatureNotFound("Atoms are empty tuples.");
  }

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  struct UInt64Hash {
    size_t operator()(uint64 value) const {
      return value;
    }
  };

  // Allow copy and assign internally, for STL containers.
  Atom() : hash_(0) {}
 public:  // public for STL only, private otherwise!
  Atom(const Atom& atom)
      : value_(atom.value_), hash_(atom.hash_) {
  }
 private:

  Atom(const StringPiece& value, uint64 hash)
      : value_(value.as_string()), hash_(hash) {
  }

  typedef unordered_multimap<uint64, Atom, UInt64Hash> AtomMap;
  static AtomMap atom_map_;

  // ---------------------------------------------------------------------------
  // Memory layout

  // The unescaped atom string.
  const string value_;

  const uint64 hash_;

  // // for STL containers
  // friend class pair<const uint64, Atom>;
  // friend class std::__is_convertible_helper<Atom&, Atom, false>;
};

}  // namespace store

#endif  // STORE_ATOM_H_
