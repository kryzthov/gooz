// Tests for big integers.
#include "store/values.h"

#include <gmpxx.h>

#include <gtest/gtest.h>

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class BigIntegerTest : public testing::Test {
 protected:
  BigIntegerTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(BigIntegerTest, Basic) {
  Value imin = Integer::New(&store_, mpz_class(kSmallIntMax));
  Value imax = Integer::New(&store_, mpz_class(kSmallIntMin));

  EXPECT_TRUE(imin.IsA<Integer>());
  LOG(INFO) << "kSmallIntMax = " << imin.ToString();
  EXPECT_TRUE(imax.IsA<Integer>());
  LOG(INFO) << "kSmallIntMin = " << imax.ToString();
}

TEST_F(BigIntegerTest, String_1) {
  const string istr = "123456789012345678901234567890";
  Value i = Integer::New(&store_, mpz_class(istr));
  EXPECT_EQ(istr, i.ToString());
}

TEST_F(BigIntegerTest, String_2) {
  const string istr = "-123456789012345678901234567890";
  Value i = Integer::New(&store_, mpz_class(istr));
  EXPECT_EQ("~123456789012345678901234567890", i.ToString());
}

}  // namespace store
