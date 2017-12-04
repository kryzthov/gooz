// Tests for small integers.
#include "store/values.h"

#include <gtest/gtest.h>

namespace store {

TEST(SmallInteger, Limits) {
  const uint64 kStoreSize = 1024 * 1024;
  StaticStore store(kStoreSize);

  Value imax = Value::Integer(&store, mpz_class(kSmallIntMax) - 1);
  Value imin = Value::Integer(&store, mpz_class(kSmallIntMin) + 1);

  EXPECT_TRUE(imin.IsA<SmallInteger>());
  LOG(INFO) << "kSmallIntMin = " << imin.ToString();
  EXPECT_TRUE(imax.IsA<SmallInteger>());
  LOG(INFO) << "kSmallIntMax = " << imax.ToString();
}


TEST(SmallInteger, Basic) {
  Value i0 = SmallInteger(0).Encode();
  Value i1 = SmallInteger(1).Encode();
  Value i2 = SmallInteger(2).Encode();

  EXPECT_EQ("0", i0.ToString());
  EXPECT_EQ("1", i1.ToString());
  EXPECT_EQ("2", i2.ToString());
}

}  // namespace store
