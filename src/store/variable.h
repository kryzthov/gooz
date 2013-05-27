#ifndef STORE_VARIABLE_H_
#define STORE_VARIABLE_H_

#include <list>
#include <string>

using std::list;
using std::string;

namespace store {

// Free variable with suspensions.
class Variable : public HeapValue {
 public:
  static const Value::ValueType kType = Value::VARIABLE;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline Variable* New() DEPRECATED {
    return new Variable();
  }

  static inline Variable* New(Store* store) {
    return new(CHECK_NOTNULL(store->Alloc<Variable>())) Variable();
  }

  // ---------------------------------------------------------------------------
  // Variable interface

  // Binds this free variable to another value.
  //
  // The variable must be free.
  // This is a light implementation of the unification in the special case
  // where one value is a free variable: no transactional/unification context.
  //
  // It is the responsibility of the caller to wake up suspensed threads if this
  // returns true.
  //
  // If a free variable is bound to another free variable, the suspensions are
  // merged into the suspension list of the variable left free.
  //
  // @param value The dereferenced value to bind this variable to.
  //     May be unbound too.
  // @returns True if this variable becomes determined,
  //     false if the variable is still undetermined after the operation.
 bool BindTo(Value value);

  // Reverts the changes from an aborted unification.
  // The variable becomes free again if it was bound during the unification.
  // The suspensions list is reverted.
  // @param suspensions The initial suspensions list to revert to.
  void RevertToFree(list<Thread*>* suspensions);

  bool IsFree() const { return ref_ == NULL; }
  Value ref() const { return ref_; }

  SuspensionList* suspensions() { return &suspensions_; }
  void AddSuspension(Thread* thread) {
    suspensions_.push_back(thread);
  }

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const throw() { return kType; }

  virtual Value Deref();
  virtual Value Optimize(OptimizeContext* context);
  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool IsDetermined() { return !IsFree(); }
  virtual bool IsStateless(StatelessnessContext* context);

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  // Initializes a new free variable.
  Variable() : ref_((HeapValue*) NULL) {}
  virtual ~Variable() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  // NULL as long as the value is unbound.
  Value ref_;

  // List of the threads suspended on this value.
  SuspensionList suspensions_;
};

}  // namespace store

#endif  // STORE_VARIABLE_H_
