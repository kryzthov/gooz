// Tests for values equality.
#include "store/values.h"

#include <gtest/gtest.h>

#include "base/stl-util.h"
#include "combinators/oznode_eval_visitor.h"

using combinators::oz::ParseEval;

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class EqualityTest : public testing::Test {
 protected:
  EqualityTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};


TEST_F(EqualityTest, Primitives) {
  EXPECT_TRUE(Equals(Boolean::True, Boolean::True));
  EXPECT_FALSE(Equals(Boolean::True, Boolean::False));

  EXPECT_TRUE(Equals(Value::Integer(12345), Value::Integer(12345)));
  EXPECT_FALSE(Equals(Value::Integer(23456), Value::Integer(12345)));
  EXPECT_FALSE(Equals(Value::Integer(12345), Boolean::True));
  EXPECT_FALSE(Equals(Value::Integer(12345), Atom::Get("x")));

  EXPECT_TRUE(Equals(Atom::Get("x"), Atom::Get("x")));
  EXPECT_FALSE(Equals(Atom::Get("y"), Atom::Get("x")));

  Name* name = Name::New(&store_);
  EXPECT_TRUE(Equals(name, name));
  EXPECT_FALSE(Equals(name, Name::New(&store_)));
  EXPECT_FALSE(Equals(name, Atom::Get("x")));
}

TEST_F(EqualityTest, Tuples) {
  Tuple* tuple1 = ParseEval("tuple(x y z)", &store_).as<Tuple>();
  Tuple* tuple2 = ParseEval("X = x Y = y Z = z T = tuple(X Y Z)", &store_)
      .Deref().as<Tuple>();
  Tuple* tuple3 = ParseEval("tuple(x y t)", &store_).as<Tuple>();
  Tuple* tuple4 = ParseEval("tuple(x y _)", &store_).as<Tuple>();
  Tuple* tuple5 = ParseEval("tuple(x y _)", &store_).as<Tuple>();
  EXPECT_TRUE(Equals(tuple1, tuple2));
  EXPECT_FALSE(Equals(tuple1, tuple3));
  EXPECT_FALSE(Equals(tuple1, tuple4));
  EXPECT_FALSE(Equals(tuple4, tuple5));

  ASSERT_TRUE(Unify(tuple4, tuple5));
  EXPECT_TRUE(Equals(tuple4, tuple5));
}

TEST_F(EqualityTest, Records) {
}

TEST_F(EqualityTest, Lists) {
}

}  // namespace store
