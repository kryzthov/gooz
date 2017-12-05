#ifndef STORE_ARITY_H_
#define STORE_ARITY_H_

#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

using std::string;
using std::unordered_multimap;
using std::vector;

#include "proto/store.pb.h"

#include "base/string_piece.h"
using base::StringPiece;

#include "base/stl-util.h"

namespace store {

// -----------------------------------------------------------------------------
// Arities.
//
// An arity is a ordered set of literals called features.
// Each feature is mapped to an integer in the range 0..(size-1).
//
// TODO: Unit-test the record/tuple interface

class Arity : public HeapValue {
 public:
  static const Value::ValueType kType = Value::ARITY;

  // ---------------------------------------------------------------------------
  // Factory methods

  // The following constructors assume the literals are unsorted.
  static Arity* Get(const vector<Value>& literals);
  static Arity* Get(uint64 size, Value const * literals);

  // Singleton, pair and triplet arities
  static Arity* Get(Value value);
  static Arity* Get(Value value1, Value value2);
  static Arity* Get(Value value1, Value value2, Value value3);

  static Arity* New(Store* store, uint64 size, Value const * literals);

  // @returns The arity corresponding to the given sorted literal set.
  // @param sorted In order set of literals.
  static Arity* GetFromSorted(const vector<Value>& sorted);

  // @returns The arity of a tuple of a given size.
  static Arity* GetTuple(uint64 size);

  // ---------------------------------------------------------------------------
  // Arity specific interface

  uint64 hash() const { return hash_; }
  vector<Value>& features() { return features_; }
  uint64 size() const { return features_.size(); }

  uint64 Map(Value feature); // throws FeatureNotFound
  bool Has(Value value) const noexcept;

  // @returns If this is a tuple arity.
  bool IsTuple() const;

  // @returns The arity without the specified feature.
  Arity* Subtract(Value feature);
  // @returns The arity with an additional feature.
  Arity* Extend(Value feature);

  // @returns The tuple arity contained in this arity.
  Arity* GetSubTuple();

  bool IsSubsetOf(Arity* arity) const;
  bool Equals(const Arity* arity) const { return this == arity; }

  // @returns Whether this < arity.
  bool LessThan(Arity* arity) const;

  // Finds the position a given literal should be in the arity.
  //
  // @param literal The literal to place in the arity.
  // @param has_feature Returns whether the feature exists or not in the arity.
  // @returns The index of the given literal.
  uint64 IndexOf(Value literal, bool* has_feature);

  // Computes a bit field.
  //
  // @param store Store to create the returned value into.
  // @param arity An arity to test the features of against this arity.
  // @returns A bit field, as an integer, that has bit #i set if
  //     this arity feature #i exists in the specified arity.
  Value ComputeSubsetMask(Store* store, Arity* arity) const;

  // Convenience
  uint64 Map(int64 integer);  // throws FeatureNotFound
  uint64 Map(const StringPiece& atom);  // throws FeatureNotFound
  bool Has(int64 integer) const noexcept;
  bool Has(const StringPiece& atom) const noexcept;

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const noexcept { return kType; }
  virtual void ExploreValue(ReferenceMap* ref_map);

  // Arities are interned.
  virtual Value Move(Store* store) { return this; }

  // ---------------------------------------------------------------------------
  // Implement serialization
  virtual void ToASCII(ToASCIIContext* context, string* repr);
  virtual void ToProtoBuf(oz_pb::Value* pb);

  // ---------------------------------------------------------------------------
  // Tuple interface
  virtual Value RecordLabel() { return Atom::Get("arity"); }
  virtual Arity* RecordArity() { return Arity::GetTuple(features_.size()); }
  virtual uint64 RecordWidth() { return features_.size(); }
  virtual bool RecordHas(Value feature) {
    // TODO: Handle small integers.
    if (feature.type() != Value::INTEGER) return false;
    const int64 val = feature.as<Integer>()->value();
    return (val >= 1) && (static_cast<uint64>(val) <= features_.size());
  }
  virtual Value RecordGet(Value* feature) { throw NotImplemented(); }
  virtual Value::ItemIterator* RecordIterItems() {
    return new ItemIterator(this);
  }
  virtual Value::ValueIterator* RecordIterValues() {
    return new ValueIterator(this);
  }

  // ---------------------------------------------------------------------------
  // Tuple interface
  virtual Value TupleGet(uint64 index) {
    CHECK_LT(index, features_.size());
    return features_[index];
  }

 private:  // ------------------------------------------------------------------

  // Copy constructor needed by STL containers.
 public:  // really, this is private!!!
  Arity(const Arity& arity)
      : hash_(arity.hash_),
        // fmap_(arity.fmap_),
        features_(arity.features_) {
  }
 private:

  // // For STL maps.
  // friend class std::pair<const uint64, Arity>;
  // friend class std::__is_convertible_helper<Arity&, Arity, false>;
  // friend class std::__is_convertible_helper<Arity&, Arity>;

  // Trivial uint64 hash function.
  struct UInt64Hash {
    inline size_t operator()(uint64 value) const { return value; }
  };

  // Global interned map of all known arities indexed by their hash.
  typedef unordered_multimap<uint64, Arity, UInt64Hash> ArityMap;
  static ArityMap arity_map_;

  // Initializes a new arity object from the given in-order literals set.
  // @param literals In-order vector of the literals. Copied.
  // @param hash Hash of the arity.
  Arity(const vector<Value>& literals, uint64 hash);

  // ---------------------------------------------------------------------------
  // Memory layout

  // Hash code of the arity itself.
  const uint64 hash_;

  // Map literals to their position in the arity, in [0 .. (size-1)]
  // How does binary searching compare to looking up in the hash map?
  // typedef UnorderedMap<Value*, uint32,
  //                      Literal::Hash, Literal::Equal> FeatureMap;
  // FeatureMap fmap_;

  // Ordered set of the arity literals.
  // TODO: Eventually, the features should be nested in this object:
  // Value[] features_;
  vector<Value> features_;

  // ---------------------------------------------------------------------------
  class ItemIterator : public Value::ItemIterator {
   public:
    ItemIterator(Arity* arity)
        : arity_(CHECK_NOTNULL(arity)),
          index_(0) {
    }

    virtual ~ItemIterator() {}

    virtual ValuePair operator*() {
      CHECK(!at_end());
      return std::make_pair(Value::Integer(index_ + 1),
                            arity_->features_[index_]);
    }

    virtual ItemIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= arity_->size();
    }

   private:
    Arity* const arity_;
    uint64 index_;
  };

  // ---------------------------------------------------------------------------
  class ValueIterator : public Value::ValueIterator {
   public:
    ValueIterator(Arity* arity)
        : arity_(CHECK_NOTNULL(arity)),
          index_(0) {
    }

    virtual ~ValueIterator() {}

    virtual Value operator*() {
      CHECK(!at_end());
      return arity_->features_[index_];
    }

    virtual ValueIterator& operator++() {
      ++index_;
      return *this;
    }

    virtual bool at_end() {
      return index_ >= arity_->size();
    }

   private:
    Arity* const arity_;
    uint64 index_;
  };

};

}  // namespace store

#endif  // STORE_ARITY_H_
