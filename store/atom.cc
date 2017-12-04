#include "store/values.h"

#include <string>
using std::string;

#include "base/escaping.h"
#include "store/arity.h"

namespace store {

uint64 StringHashCode(const StringPiece& str) {
  uint64 hash = 7;
  for (uint i = 0; i < str.size(); ++i) {
    hash = 31 * hash + 7 * str[i];
  }
  return hash;
}

const Value::ValueType Atom::kType;
const boost::regex Atom::kSimpleAtomRegexp("[a-z][A-Za-z0-9_]*");

Atom::AtomMap Atom::atom_map_;

// static
string Atom::Escape(const StringPiece& raw_atom) {
  string escaped;
  escaped.push_back('\'');
  escaped.append(base::Escape(raw_atom, "\'"));
  escaped.push_back('\'');
  return escaped;
}

// static
string Atom::Unescape(const StringPiece& escaped_atom) {
  CHECK_GE(escaped_atom.size(), 2UL);
  CHECK_EQ('\'', escaped_atom[0]);
  CHECK_EQ('\'', escaped_atom[escaped_atom.size() - 1]);
  return base::Unescape(escaped_atom.substr(1, escaped_atom.size() - 2), "\'");
}

// static
Atom* Atom::Get(const StringPiece& atom) {
  const uint64 hash = StringHashCode(atom);
  pair<AtomMap::iterator, AtomMap::iterator> range =
      atom_map_.equal_range(hash);

  AtomMap::iterator it;
  for (it = range.first; it != range.second; ++it) {
    if (it->second.value() == atom)
      return &it->second;
  }
  Atom new_atom(atom, hash);
  it = atom_map_.insert(AtomMap::value_type(hash, new_atom));
  return &it->second;
}

// static
Atom* Atom::GetEscaped(const StringPiece& escaped_atom) {
  return Get(Unescape(escaped_atom));
}

// virtual
Arity* Atom::RecordArity() { return KArityEmpty(); }

// virtual
Value Atom::RecordGet(Value feature) {
  throw FeatureNotFound(feature, KArityEmpty());
}

void Atom::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);

  // Escape only when necessary
  if (boost::regex_match(value_, kSimpleAtomRegexp))
    repr->append(value_);
  else
    repr->append(Escape(value_));
}

void Atom::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  pb->mutable_primitive()->set_type(oz_pb::Primitive::ATOM);
  pb->mutable_primitive()->set_text(value_);
}

}  // namespace store
