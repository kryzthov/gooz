// Utilities for literals.

#ifndef STORE_LITERAL_H_
#define STORE_LITERAL_H_

#include "base/macros.h"

namespace store {

// -----------------------------------------------------------------------------
// Literal operations and STL utility functions.

class Literal {
 public:
  inline static bool LessThan(Value l1, Value l2) {
      return l1.LiteralLessThan(l2);
  }

  inline static bool Equals(Value l1, Value l2) {
    return l1.LiteralEquals(l2);
  }

  struct Hash {
    size_t operator()(Value value) const {
      return value.LiteralHashCode();
    }
  };

  struct Compare {
    bool operator()(Value v1, Value v2) const {
      return LessThan(v1, v2);
    }
  };

  struct Equal {
    bool operator()(Value v1, Value v2) const {
      return Equals(v1, v2);
    }
  };

 private:
  Literal();
  DISALLOW_COPY_AND_ASSIGN(Literal);
};

}  // namespace store

#endif  // STORE_LITERAL_H_
