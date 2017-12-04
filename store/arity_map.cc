#include "store/values.h"

#include <algorithm>
#include <utility>

#include <glog/logging.h>

namespace store {

bool ArityLessThan(Arity* arity1, Arity* arity2) {
  return arity1->LessThan(arity2);
}

// static
ArityMap* ArityMap::New(const vector<Arity*>& arities) {
  return new ArityMap(arities);
}

ArityMap::ArityMap(const vector<Arity*>& arities)
    : arities_(arities) {
  sort(arities_.begin(), arities_.end(), ArityLessThan);
}

uint64 ArityMap::Lookup(Arity* arity) const {
  auto bounds = std::equal_range(arities_.begin(), arities_.end(),
                                 arity, ArityLessThan);
  const uint64 nmatches = int(bounds.second - bounds.first);
  if (nmatches == 0) return 0;
  CHECK_EQ(1UL, nmatches);
  return 1 + uint64(bounds.first - arities_.begin());
}

}  // namespace store
