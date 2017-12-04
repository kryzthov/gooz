#ifndef STORE_OPEN_RECORD_H_
#define STORE_OPEN_RECORD_H_

#include <list>
#include <string>
using std::list;
using std::string;

#include "base/stl-util.h"
#include "base/string_piece.h"

using base::StringPiece;

namespace store {

// -----------------------------------------------------------------------------
// Open record
//
// An open-record is a record to which new features may be added.
// The arity of an open-record may only grow: existing features may not be
// modified or removed.
//
// TODO: An OpenRecord may turn into a regular immutable record.
// -> An open-record would include a variable which will be set
// once the open-record gets finalized.

class OpenRecord : public HeapValue {
 public:
  static const ValueType kType = Value::OPEN_RECORD;

  // ---------------------------------------------------------------------------
  // Factory methods
  static inline OpenRecord* New(Store* store, Value label) {
    return new(CHECK_NOTNULL(store->Alloc<OpenRecord>()))
        OpenRecord(store, label);
  }

  // ---------------------------------------------------------------------------
  // OpenRecord specific interface

  Value label() const { return label_; }

  // @returns The current size of the open-record.
  int64 size() const { return features_.size(); }

  bool Has(Value feature) const {
    return features_.contains(feature);
  }
  inline bool Has(int64 feature) const {
    return Has(Value::Integer(feature));
  }
  inline bool Has(const StringPiece& feature) const {
    return Has(Atom::Get(feature));
  }

  Value Get(Value feature) const;
  inline Value Get(int64 feature) const {
    return Get(Value::Integer(feature));
  }
  inline Value Get(const StringPiece& feature) const {
    return Get(Atom::Get(feature));
  }

  // Inserts a new feature into this open-record.
  // @param feature The new feature to insert.
  // @param value The value to associate to the new feature.
  // @returns True if the feature was inserted,
  //     or false if not, because it already existed beforehand.
  bool Set(Value feature, Value value);
  inline bool Set(int64 feature, Value value) {
    return Set(Value::Integer(feature), value);
  }
  inline bool Set(const StringPiece& feature, Value value) {
    return Set(Atom::Get(feature), value);
  }

  // @returns True if this open-record currently has the arity of a tuple.
  bool IsTuple() const;

  // @returns The arity matching the current state of this open-record.
  //     This is an expensive operation.
  Arity* GetArity(Store* store) const;

  // @returns A record matching the current state of this open-record.
  Value GetRecord(Store* store) const;

  // ---------------------------------------------------------------------------
  // Value API

  virtual ValueType type() const throw() { return kType; }
  virtual uint64 caps() const { return Value::CAP_RECORD; }

  virtual void ExploreValue(ReferenceMap* ref_map);
  virtual Value Deref();
  virtual Value Optimize(OptimizeContext* context);
  virtual bool IsDetermined();
  virtual bool UnifyWith(UnificationContext* context, Value other);
  virtual HeapValue* MoveInternal(Store* store);
  virtual bool IsStateless(StatelessnessContext* context);

  // ---------------------------------------------------------------------------
  // OpenRecord interface
  virtual Arity* OpenRecordArity(Store* store);
  virtual uint64 OpenRecordWidth();
  virtual bool OpenRecordHas(Value feature);
  virtual Value OpenRecordGet(Value feature);
  virtual Value OpenRecordClose(Store* store);

  virtual Value::ItemIterator* OpenRecordIterItems();
  virtual Value::ValueIterator* OpenRecordIterValues();

  // ---------------------------------------------------------------------------
  // Record interface

  virtual Value RecordLabel() { return label_; }

  virtual Arity* RecordArity();
  virtual uint64 RecordWidth();
  virtual bool RecordHas(Value feature);
  virtual Value RecordGet(Value feature);

  virtual Value::ItemIterator* RecordIterItems();
  virtual Value::ValueIterator* RecordIterValues();

  // ---------------------------------------------------------------------------
  // Serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

 private:  // ------------------------------------------------------------------

  // Creates a new empty open-record.
  explicit OpenRecord(Store* store, Value label);
  virtual ~OpenRecord() {}

  // ---------------------------------------------------------------------------
  // Memory layout

  // This variable will be set once the record is closed or merged into
  // another open-record.
  // Threads will be pending on this free variable until the record gets closed.
  Variable* ref_;

  Value label_;
  typedef Map<Value, Value, Literal::Compare> FeatureMap;
  FeatureMap features_;

  // ---------------------------------------------------------------------------
  class ItemIterator : public Value::ItemIterator {
   public:
    ItemIterator(OpenRecord* record)
        : record_(CHECK_NOTNULL(record)),
          it_(record_->features_.begin()) {
    }

    virtual ~ItemIterator() {}

    virtual ValuePair operator*() {
      CHECK(!at_end());
      return std::make_pair(it_->first, it_->second);
    }

    virtual ItemIterator& operator++() {
      ++it_;
      return *this;
    }

    virtual bool at_end() {
      return it_ == record_->features_.end();
    }

   private:
    OpenRecord* const record_;
    FeatureMap::const_iterator it_;
  };

  // ---------------------------------------------------------------------------
  class ValueIterator : public Value::ValueIterator {
   public:
    ValueIterator(OpenRecord* record)
        : record_(CHECK_NOTNULL(record)),
          it_(record_->features_.begin()) {
    }

    virtual ~ValueIterator() {}

    virtual Value operator*() {
      CHECK(!at_end());
      return it_->second;
    }

    virtual ValueIterator& operator++() {
      ++it_;
      return *this;
    }

    virtual bool at_end() {
      return it_ == record_->features_.end();
    }

   private:
    OpenRecord* const record_;
    FeatureMap::const_iterator it_;
  };

};

}  // namespace store

#endif  // STORE_OPEN_RECORD_H_
