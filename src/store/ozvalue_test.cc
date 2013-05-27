
#include "store/ozvalue.h"

#include <gtest/gtest.h>

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class OzValueTest : public testing::Test {
 protected:
  OzValueTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(OzValueTest, Basic) {
  OzValue int1(1);
  OzValue int2 = 2;

  EXPECT_TRUE(int1 == 1);
  EXPECT_FALSE(int1 == 0);
  EXPECT_FALSE(int1 == "x");

  EXPECT_TRUE(int2 == 2);

  OzValue atom1("atom1");
  OzValue atom2 = "atom2";

  EXPECT_TRUE(atom1 == "atom1");
  EXPECT_FALSE(atom1 == "atom2");
  EXPECT_FALSE(atom1 == 1);

  EXPECT_TRUE(atom2 == "atom2");

  OzValue x(int1);
  OzValue y = x;
  OzValue z = OzValue::Parse("arecord(a b c)");
  y = x;
  CHECK_EQ(1, x.int_val());
  CHECK_EQ(1, y.int_val());
  CHECK_EQ("arecord", z.label().atom_val());
  CHECK_EQ("a", z[1].atom_val());
}

TEST_F(OzValueTest, RecordInterface) {
  OzValue record = Value(Tuple::New(&store_, Atom::Get("label"), 10));
  EXPECT_FALSE(record[1].IsDetermined());
  record[1] = 10;
  EXPECT_TRUE(record[1].IsDetermined());
}

}  // namespace store
