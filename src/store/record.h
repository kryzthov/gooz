#ifndef STORE_RECORD_H_
#define STORE_RECORD_H_

#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------
// Record
//
// A record is an immutable value. Its arity and features cannot be modified.
// Record values must have at least one feature and cannot be tuples.
//
class Record : public HeapValue {
 public:
  static const ValueType kType = Value::RECORD;

  // ---------------------------------------------------------------------------
  // Factory methods
  //
  // The Value array, when specified, is copied. When not specified, the record
  // is initialized with unbound variables.
  static inline Record* New(Store* store, Value label, Arity* arity);

  static inline Record* New(Store* store,
                            Value label, Arity* arity, Value* values);

  // ---------------------------------------------------------------------------
  // Record specific interface

  Value label() const { return label_; }
  Arity* arity() const { return arity_; }
  uint64 size() const;
  const Value* values() const { return values_; }
  Value Get(Value feature) const;
  bool Has(Value feature) const;

  // Projects this record on a given subset of its arity.
  //
  // @param store Store to create new values into.
  // @param arity The features to select.
  // @returns A new record with the selected features from this record.
  Value Project(Store* store, Arity* arity);

  // Removes one feature from this record.
  //
  // @param store Store to create new values into.
  // @param feature The feature to remove.
  // @returns The value resulting from the feature removal.
  Value Subtract(Store* store, Value feature);

  // Convenience
  inline Value Get(int64 index) const {
    return Get(Value::Integer(index));
  }

  inline Value Get(const string& atom) const {
    return Get(Atom::Get(atom));
  }

  // ---------------------------------------------------------------------------
  // Value API
  virtual ValueType type() const throw() { return kType; }
  virtual uint64 caps() const { return Value::CAP_RECORD; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Optimize(OptimizeContext* context);
  virtual bool UnifyWith(UnificationContext* context, Value value);
  virtual bool Equals(EqualityContext* context, Value value);
  virtual HeapValue* MoveInternal(Store* store);
  virtual bool IsStateless(StatelessnessContext* context);

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

  // ---------------------------------------------------------------------------
  // Implement the record interface
  virtual Value RecordLabel() { return label_; }
  virtual Arity* RecordArity() { return arity_; }
  virtual uint64 RecordWidth();
  virtual bool RecordHas(Value feature);
  virtual Value RecordGet(Value feature);

  virtual Value::ItemIterator* RecordIterItems() {
    return new ItemIterator(this);
  }
  virtual Value::ValueIterator* RecordIterValues() {
    return new ValueIterator(this);
  }

 private:  // ------------------------------------------------------------------

  // Initializes the record with the specified label, arity.
  // Values are left uninitialized.
  Record(Value label, Arity* arity);

  // Initializes the record with the specified label, arity and values.
  // Copy the values.
  Record(Value label, Arity* arity, Value* values);

  virtual ~Record() {}

  // ---------------------------------------------------------------------------
  // Memory layout
  Value label_;
  Arity* const arity_;
  Value values_[];

  // ---------------------------------------------------------------------------
  class ItemIterator : public Value::ItemIterator {
   public:
    ItemIterator(Record* record)
        : record_(CHECK_NOTNULL(record)),
          index_(0) {
    }

    virtual ~ItemIterator() {}
    virtual ValuePair operator*();
    virtual ItemIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= record_->size();
    }

   private:
    Record* const record_;
    uint64 index_;
  };

  // ---------------------------------------------------------------------------
  class ValueIterator : public Value::ValueIterator {
   public:
    ValueIterator(Record* record)
        : record_(CHECK_NOTNULL(record)),
          index_(0) {
    }

    virtual ~ValueIterator() {}

    virtual Value operator*() {
      CHECK(!at_end());
      return record_->values_[index_];
    }

    virtual ValueIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= record_->size();
    }

   private:
    Record* const record_;
    uint64 index_;
  };

};

}  // namespace store

#endif  // STORE_RECORD_H_
