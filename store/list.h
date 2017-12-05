#ifndef STORE_LIST_H_
#define STORE_LIST_H_

#include <string>
using std::string;

#include "base/stl-util.h"

namespace store {

// -----------------------------------------------------------------------------
// List

class List : public HeapValue {
 public:
  static const Value::ValueType kType = Value::LIST;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline
  List* New(Store* store, Value head, Value tail) {
    return new(CHECK_NOTNULL(store->Alloc<List>())) List(head, tail);
  }

  // ---------------------------------------------------------------------------
  // List specific interface

  Value head() const { return head_; }
  Value tail() const { return tail_; }

  List* Next() const { return tail_.Deref().as<List>(); }

  // Counts the number of values in the list.
  //
  // Counts the list payload size, i.e. how many head values can be processed
  // when iterating on the list.
  // Does not include the list values nor the final tail value.
  //
  // Does not optimizer the value graph.
  //
  // @returns the number of values in the list.
  // @param last Returns the last value of the list:
  //     - nil for a finite list,
  //     - a free variable for a stream or unterminated list,
  //     - any other value for a non-conventional list,
  //     - the list value where the loop occurs, for infinite lists.
  int64 GetValuesCount(Value* last);
  typedef UnorderedSet<Value> ReferenceSet;
  void GetValuesCountInternal(ReferenceSet* ref_set,
                              int64* count, Value* last);

  // ---------------------------------------------------------------------------
  // Value API

  virtual Value::ValueType type() const noexcept { return kType; }
  virtual uint64 caps() const { return Value::CAP_RECORD | Value::CAP_TUPLE; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);

  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool Equals(EqualityContext* context, Value value);
  virtual HeapValue* MoveInternal(Store* store);
  virtual bool IsStateless(StatelessnessContext* context);

  // ---------------------------------------------------------------------------
  // Record interface
  virtual Value RecordLabel();
  virtual Arity* RecordArity();
  virtual uint64 RecordWidth() { return 2; }
  virtual bool RecordHas(Value feature);
  virtual Value RecordGet(Value feature);

  virtual Value::ItemIterator* RecordIterItems() {
    return new ItemIterator(this);
  }
  virtual Value::ValueIterator* RecordIterValues() {
    return new ValueIterator(this);
  }

  // ---------------------------------------------------------------------------
  // Tuple interface
  virtual Value TupleGet(uint64 index) {
    if (index >= 2)
      throw FeatureNotFound("List tuple has no feature " + index);
    return values()[index];
  }

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  List(Value head, Value tail) : head_(head), tail_(tail) {
  }

  virtual ~List() {
  }

  // ---------------------------------------------------------------------------
  // Memory layout
  Value head_;
  Value tail_;

  // This should normally be:
  // union {
  //   Value values_[2];
  //   struct {
  //     Value head_;  // == values_[0]
  //     Value tail_;  // == values_[1]
  //   };
  // };

  inline const Value* values() const { return &head_; }

  // ---------------------------------------------------------------------------
  class ItemIterator : public Value::ItemIterator {
   public:
    ItemIterator(List* list)
        : list_(CHECK_NOTNULL(list)),
          index_(0) {
    }

    virtual ~ItemIterator() {}

    virtual ValuePair operator*() {
      CHECK(!at_end());
      return std::make_pair(Value::Integer(index_ + 1),
                            list_->values()[index_]);
    }

    virtual ItemIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= 2;
    }

   private:
    List* const list_;
    uint64 index_;
  };

  // ---------------------------------------------------------------------------
  class ValueIterator : public Value::ValueIterator {
   public:
    ValueIterator(List* list)
        : list_(CHECK_NOTNULL(list)),
          index_(0) {
    }

    virtual ~ValueIterator() {}

    virtual Value operator*() {
      CHECK(!at_end());
      return list_->values()[index_];
    }

    virtual ValueIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= 2;
    }

   private:
    List* const list_;
    uint64 index_;
  };

};

}  // namespace store

#endif  // STORE_LIST_H_
