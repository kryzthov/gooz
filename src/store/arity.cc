#include "store/values.h"

#include <algorithm>
#include <utility>

#include <boost/format.hpp>
using boost::format;

#include <glog/logging.h>

namespace store {

// -----------------------------------------------------------------------------

const Value::ValueType Arity::kType;
Arity::ArityMap Arity::arity_map_;

uint64 ArityHashCode(const vector<Value>& literals) {
  // TODO: Clean this const mess.
  vector<Value>& ncliterals = const_cast<vector<Value>&>(literals);
  uint64 hash = 11;
  for (uint i = 0; i < literals.size(); ++i)
    hash = 31 * hash + 7 * ncliterals[i].LiteralHashCode();
  return hash;
}

Arity* Arity::Get(const vector<Value>& literals) {
  vector<Value> sorted = literals;
  sort(sorted.begin(), sorted.end(), Literal::LessThan);
  return GetFromSorted(sorted);
}

Arity* Arity::Get(uint64 size, Value const * literals) {
  vector<Value> sorted(size);
  for (uint64 i = 0; i < size; ++i)
    sorted[i] = literals[i];
  sort(sorted.begin(), sorted.end(), Literal::LessThan);
  return GetFromSorted(sorted);
}

Arity* Arity::GetTuple(uint64 size) {
  // TODO: Maybe specializes a little bit for tuple arities.
  vector<Value> sorted(size);
  for (uint64 i = 0; i < size; ++i)
    sorted[i] = Value::Integer(i + 1);
  return GetFromSorted(sorted);
}

Arity* Arity::GetFromSorted(const vector<Value>& sorted) {
  uint64 hash = ArityHashCode(sorted);
  pair<ArityMap::iterator, ArityMap::iterator> range =
      arity_map_.equal_range(hash);
  ArityMap::iterator it;
  for (it = range.first; it != range.second; ++it) {
    Arity* const arity = &it->second;
    // Checks that this is the right arity and not a collision.
    if (arity->size() != sorted.size()) continue;
    if (!equal(sorted.begin(), sorted.end(),
               arity->features().begin(),
               Literal::Equals))
      continue;
    return arity;
  }
  Arity new_arity(sorted, hash);
  it = arity_map_.insert(ArityMap::value_type(hash, new_arity));
  return &it->second;
}

Arity::Arity(const vector<Value>& literals, uint64 hash)
    : hash_(hash),
      features_(literals) {
  // for (uint i = 0; i < features_.size(); ++i)
  //   fmap_[features_[i]] = i;
}

uint64 Arity::Map(Value feature) throw(FeatureNotFound) {
  auto bounds = std::equal_range(features_.begin(), features_.end(),
                                 feature, Literal::LessThan);
  const uint64 nmatches = int(bounds.second - bounds.first);
  if (nmatches == 0)
    throw FeatureNotFound(feature, this);
  CHECK_EQ(1UL, nmatches);
  return uint64(bounds.first - features_.begin());
}

// int64 Arity::Map(Value* feature) const {
//   return fmap_.find(feature)->second;
// }

bool Arity::Has(Value feature) const throw() {
  return std::binary_search(features_.begin(), features_.end(),
                            feature, Literal::LessThan);
}

// bool Arity::Has(Value* feature) const {
//   return fmap_.find(feature) != fmap_.end();
// }

bool Arity::IsTuple() const {
  const int64 nfeatures = features_.size();
  if (nfeatures == 0) return true;
  Value last = features_[nfeatures - 1];
  // There should be no neew to handle big integers :)
  return (last.type() == Value::SMALL_INTEGER)
      && (SmallInteger(last).value() == nfeatures);
}

Arity* Arity::Subtract(Value feature) {
  const uint64 size = features_.size();
  vector<Value> features;
  features.reserve(size - 1);

  bool has_feature = false;
  const uint64 ifeature = IndexOf(feature, &has_feature);
  CHECK(has_feature);

  for (uint64 i = 0; i < ifeature; ++i)
    features.push_back(features_[i]);
  for (uint64 i = ifeature + 1; i < size; ++i)
    features.push_back(features_[i]);
  CHECK_EQ(size - 1, features.size());
  return GetFromSorted(features);
}

Arity* Arity::Extend(Value feature) {
  const uint64 size = features_.size();
  vector<Value> features;
  features.reserve(size + 1);

  bool has_feature = false;
  const uint64 ifeature = IndexOf(feature, &has_feature);
  CHECK(!has_feature);

  for (uint64 i = 0; i < ifeature; ++i)
    features.push_back(features_[i]);
  features.push_back(feature);
  for (uint64 i = ifeature; i < size; ++i)
    features.push_back(features_[i]);
  CHECK_EQ(size + 1, features.size());
  return GetFromSorted(features);
}

Arity* Arity::GetSubTuple() {
  const uint64 size = features_.size();
  for (int64 i = size - 1; i >= 0; --i) {
    Value feature = features_[i];
    if ((feature.type() == Value::SMALL_INTEGER)
        && (SmallInteger(feature).value() == i))
      return GetTuple(i + 1);
  }
  return GetTuple(0);
}

bool Arity::IsSubsetOf(Arity* arity) const {
  CHECK_NOTNULL(arity);
  if (features_.size() > arity->features_.size()) return false;
  uint i = 0;
  uint j = 0;
  while ((i < features_.size()) && (j < arity->features_.size())) {
    Value feature = features_[i];
    while (j < arity->features_.size()) {
      if (Literal::Equals(feature, arity->features_[j])) break;
      if (!Literal::LessThan(arity->features_[j], feature)) return false;
      ++j;
    }
    ++i;
    ++j;
  }
  return i == features_.size();
}

bool Arity::LessThan(Arity* arity) const {
  CHECK_NOTNULL(arity);
  const uint64 size1 = size();
  const uint64 size2 = arity->size();
  if (size1 != size2)
    return size1 < size2;

  // Here, both arities have the same size.
  for (uint64 i = 0; i < size1; ++i) {
    Value f1 = features_[i];
    Value f2 = arity->features_[i];
    if (f1.LiteralEquals(f2)) continue;
    else if (f1.LiteralLessThan(f2)) return true;
    else return false;
  }

  // Same arities
  return false;
}

uint64 Arity::IndexOf(Value literal, bool* has_feature) {
  CHECK_NOTNULL(has_feature);
  *has_feature = false;
  if (features_.empty()) return 0;

  uint64 lower = 0;
  uint64 upper = features_.size() - 1;

  if (literal.LiteralLessThan(features_[lower])) return 0;
  if (literal.LiteralEquals(features_[lower])) {
    *has_feature = true;
    return 0;
  }
  // if (upper == 0) return 1;
  if (features_[upper].LiteralLessThan(literal)) return upper + 1;
  if (features_[upper].LiteralEquals(literal)) {
    *has_feature = true;
    return upper;
  }

  while (upper - lower > 1) {
    // lower < x < upper
    uint64 middle = (lower + upper) / 2;
    Value mvalue = features_[middle];
    if (literal.LiteralEquals(mvalue)) {
      *has_feature = true;
      return middle;
    }
    if (literal.LiteralLessThan(mvalue))
      upper = middle;
    else  // mvalue.LiteralLessThan(literal)
      lower = middle;
  }
  CHECK_EQ(1UL, upper - lower);
  return upper;
}

Value Arity::ComputeSubsetMask(Store* store, Arity* arity) const {
  CHECK_NOTNULL(arity);
  const uint64 this_size = features_.size();
  const uint64 arity_size = arity->features_.size();

  // Maybe we would be better of testing each feature of arity through hashing
  // but this requires having the feature map.
  // Since IndexOf() is log(N), so the total worst case becomes N×log(N).

  mpz_class bit_field(0);
  uint64 i = 0, j = 0;
  while ((i < this_size) && (j < arity_size)) {
    Value fi = features_[i];
    Value fj = arity->features_[j];
    if (fi.LiteralEquals(fj)) {
      mpz_class bit(1);
      bit <<= i;
      bit_field |= bit;
      ++i;
      ++j;
    } else if (fi.LiteralLessThan(fj)) {
      ++i;
    } else {  // fj.LiteralLessThan(fi)
      ++j;
    }
  }
  return Value::Integer(store, bit_field);
}

// Value API

void Arity::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  for (uint i = 0; i < features_.size(); ++i)
    features_[i].Explore(ref_map);
}

// Serialization

void Arity::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(repr);
  repr->append("{NewArity ");
  repr->append((format("%d") % features_.size()).str());
  repr->append(" features(");
  if (features_.size() > 0)
    context->Encode(features_[0], repr);
  for (uint i = 1; i < features_.size(); ++i) {
    repr->append(" ");
    context->Encode(features_[i], repr);
  }
  repr->append(")}");
}

void Arity::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  oz_pb::Arity* arity_pb = pb->mutable_arity();
  for (uint i = 0; i < features_.size(); ++i) {
    oz_pb::Value feat_value;
    features_[i].ToProtoBuf(&feat_value);
    feat_value.mutable_primitive()->Swap(arity_pb->add_feature());
  }
}

// -----------------------------------------------------------------------------

}  // namespace store
