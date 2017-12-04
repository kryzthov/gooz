#ifndef STORE_TUPLE_H_
#define STORE_TUPLE_H_

#include <string>
using std::string;

#include <glog/logging.h>

#include "store/values.h"

namespace store {

// -----------------------------------------------------------------------------
// Tuple
//
// A tuple is a record whose arity is the sequence:
//     arity(1 2 3 ... size)
//
class Tuple : public HeapValue {
 public:
  static const ValueType kType = Value::TUPLE;

  // ---------------------------------------------------------------------------
  // Factory methods
  static Tuple* New(Store* store, Value label, uint64 size);

  // Copies the values
  static Tuple* New(Store* store, Value label, uint64 size, Value* values);

  // ---------------------------------------------------------------------------
  // Tuple specific interface

  Value label() const { return label_; }
  uint64 size() const { return size_; }
  const Value* values() const { return values_; }
  Arity* arity() const;

  Value Get(Value int_value) const;
  Value Get(uint64 index) const;

  // ---------------------------------------------------------------------------
  // Record interface
  virtual Value RecordLabel() { return label_; }
  virtual Arity* RecordArity();
  virtual uint64 RecordWidth() { return size_; }
  virtual bool RecordHas(Value feature);
  virtual Value RecordGet(Value feature);

  virtual Value::ItemIterator* RecordIterItems() {
    return new ItemIterator(this);
  }
  virtual Value::ValueIterator* RecordIterValues() {
    return new ValueIterator(this);
  }

  virtual Value TupleGet(uint64 index);

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const throw() { return kType; }
  virtual uint64 caps() const { return Value::CAP_RECORD | Value::CAP_TUPLE; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);
  virtual bool UnifyWith(UnificationContext* context, Value ovalue);
  virtual bool Equals(EqualityContext* context, Value value);
  virtual HeapValue* MoveInternal(Store* store);
  virtual bool IsStateless(StatelessnessContext* context);

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  // virtual void ToProtoBuf(oz_pb::Value* pb);

 private: // -------------------------------------------------------------------

  Tuple(Value label, uint64 size);
  Tuple(Value label, uint64 size, Value* values);

  // ---------------------------------------------------------------------------
  // Memory layout

  Value label_;
  const uint64 size_;
  Value values_[];

  // ---------------------------------------------------------------------------
  class ItemIterator : public Value::ItemIterator {
   public:
    ItemIterator(Tuple* tuple)
        : tuple_(CHECK_NOTNULL(tuple)),
          index_(0) {
    }

    virtual ~ItemIterator() {}

    virtual ValuePair operator*() {
      CHECK(!at_end());
      return std::make_pair(Value::Integer(index_ + 1),
                            tuple_->values_[index_]);
    }

    virtual ItemIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= tuple_->size();
    }

   private:
    Tuple* const tuple_;
    uint64 index_;
  };

  // ---------------------------------------------------------------------------
  class ValueIterator : public Value::ValueIterator {
   public:
    ValueIterator(Tuple* tuple)
        : tuple_(CHECK_NOTNULL(tuple)),
          index_(0) {
    }

    virtual ~ValueIterator() {}

    virtual Value operator*() {
      CHECK(!at_end());
      return tuple_->values_[index_];
    }

    virtual ValueIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= tuple_->size();
    }

   private:
    Tuple* const tuple_;
    uint64 index_;
  };

};

}  // namespace store

#endif  // STORE_TUPLE_H_
