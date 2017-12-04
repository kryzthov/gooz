#ifndef STORE_SMALL_INTEGER_INL_H_
#define STORE_SMALL_INTEGER_INL_H_

#include <boost/format.hpp>
using boost::format;

namespace store {

inline
void SmallInteger::ToASCII(string* repr) const {
  if (value_ < 0)
    repr->append((format("~%i") % -value_).str());
  else
    repr->append((format("%i") % value_).str());
}

inline
bool SmallInteger::LiteralLessThan(Value other) const {
  switch (other.tag()) {
    case kHeapValueTag: return value_ < other.as<Integer>()->mpz();
    case kSmallIntTag: return value_ < SmallInteger(other).value();
  }
  throw NotImplemented();
}

}  // namespace store

#endif  // STORE_SMALL_INTEGER_INL_H_
