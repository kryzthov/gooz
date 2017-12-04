#ifndef STORE_CELL_H_
#define STORE_CELL_H_

#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Cell

class Cell : public HeapValue {
 public:
  static const ValueType kType = Value::CELL;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline Cell* New(Store* store, Value initial) {
    return new(CHECK_NOTNULL(store->Alloc<Cell>())) Cell(initial);
  }

  // ---------------------------------------------------------------------------
  // Cell specific API

  inline
  Value Access() const {
    return ref_;
  }

  inline
  void Assign(Value value) {
    ref_ = value;
  }

  // ---------------------------------------------------------------------------
  // Value API

  virtual ValueType type() const throw() { return kType; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);
  virtual bool IsStateless(StatelessnessContext* context) {
    return false;
  }

  // ---------------------------------------------------------------------------
  // Serialization

  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  explicit Cell(Value initial) : ref_(initial) {}
  virtual ~Cell() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  Value ref_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_CELL_H_
