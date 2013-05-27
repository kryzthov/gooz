#ifndef STORE_ARITY_MAP_H_
#define STORE_ARITY_MAP_H_

namespace store {

class ArityMap : public HeapValue {
 public:
  static const Value::ValueType kType = Value::ARITY_MAP;

  // ---------------------------------------------------------------------------
  // Factory methods

  ArityMap* New(const vector<Arity*>& arities);

  // Retrieves the position of an arity in the sorted map.
  // @param arity The arity to look up.
  // @returns The arity position, in the range [1..# of arities].
  //     0 when the arity is not found.
  uint64 Lookup(Arity* arity) const;

  // ---------------------------------------------------------------------------
  // Value API
  virtual Value::ValueType type() const throw() { return kType; }
  virtual void ExploreValue(ReferenceMap* ref_map);

 private: // -------------------------------------------------------------------

  ArityMap(const vector<Arity*>& arities);

  // ---------------------------------------------------------------------------
  // Memory layout
  vector<Arity*> arities_;

  DISALLOW_COPY_AND_ASSIGN(ArityMap);
};

}  // namespace store

#endif  // STORE_ARITY_MAP_H_
