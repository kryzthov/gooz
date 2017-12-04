#ifndef STORE_CLOSURE_H_
#define STORE_CLOSURE_H_

#include <memory>
#include <string>
#include <vector>
using std::shared_ptr;
using std::string;
using std::vector;

#include <boost/format.hpp>
using boost::format;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Closures - High-level bytecode

class Bytecode;
struct Operand;

class Closure : public HeapValue {
 public:
  static const ValueType kType = Value::CLOSURE;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline Closure* New(Store* store,
                             const shared_ptr<vector<Bytecode> >& bytecode,
                             int nparams, int nlocals, int nclosures) {
    return new(CHECK_NOTNULL(store->Alloc<Closure>()))
        Closure(bytecode, nparams, nlocals, nclosures);
  }

  static inline Closure* New(Store* store,
                             const Closure* closure, Array* environment) {
    return new(CHECK_NOTNULL(store->Alloc<Closure>()))
        Closure(closure, environment);
  }

  // ---------------------------------------------------------------------------
  // Closure specific interface

  const vector<Bytecode>& bytecode() const { return *bytecode_; }
  Array* environment() const { return environment_; }
  uint64 nlocals() const { return nlocals_; }
  uint64 nclosures() const { return nclosures_; }

  // ---------------------------------------------------------------------------
  // Value API

  virtual ValueType type() const throw() { return kType; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);

  // ---------------------------------------------------------------------------
  // Serialization

  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  // Builds an abstract procedure or a procedure without closure.
  // @param bytecode The bytecode for the procedure.
  // @param nparams How many parameters this procedure takes.
  // @param nlocals How many local registers this procedure requires.
  Closure(const shared_ptr<vector<Bytecode> >& bytecode,
          int nparams, int nlocals, int nclosures);

  // Builds a closure from an abstract procedure and the given environment.
  // @param closure The abstract procedure to build the closure from.
  // @param environment The closure environment.
  Closure(const Closure* closure, Array* environment);

  virtual ~Closure();

  // Used by ExploreValue() to explore values referenced by bytecode operands.
  void ExploreOperand(const Operand& op, ReferenceMap* ref_map);

  // Used by Optimize() to optimize bytecode operands.
  void OptimizeOperand(Operand* op, OptimizeContext* context);

  // ---------------------------------------------------------------------------
  // Memory layout

  const shared_ptr<vector<Bytecode> > bytecode_;

  // Number of parameters.
  const int nparams_;

  // Number of local registers to allocate.
  const int nlocals_;

  // Number of values in the closure.
  const int nclosures_;

  // The closure. NULL for an abstract procedure, or a procedure which does
  // not have closure.
  Array* const environment_;
};

}  // namespace store

#endif  // STORE_CLOSURE_H_
