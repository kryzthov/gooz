#ifndef STORE_TYPE_H_
#define STORE_TYPE_H_

#include <list>
#include <string>
using std::list;
using std::string;

#include "store/value.h"
#include "store/values.h"


namespace store {

// -----------------------------------------------------------------------------
// Type values

// A type value.
class Type : public HeapValue {
 public:
  virtual ValueType type() const { return TYPE; }

  Value* desc() const { return desc_; }

 private:
  // The type descriptor.
  Value* desc_;
};

// A free type variable.
// A type variable has sub-typing and super-typing constraints.
class TypeVariable : public HeapValue {
 public:
  virtual ValueType type() const { return TYPE_VARIABLE; }

 private:
  // Values being sub-types of this type variable.
  list<Value*> sub_types_;

  // Values being super-types of this type variable.
  list<Value*> super_types_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_TYPE_H_
