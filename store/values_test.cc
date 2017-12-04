#include "store/values.h"

#include <gtest/gtest.h>

#include "base/stl-util.h"
#include "combinators/oznode_eval_visitor.h"

using combinators::oz::ParseEval;

namespace store {

const uint64 kStoreSize = 1024 * 1024;

// -----------------------------------------------------------------------------
// Value graph exploring

class ExploreTest : public testing::Test {
 protected:
  ExploreTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(ExploreTest, PrimitiveValue) {
  {
    Atom* atom = Atom::Get("atom");
    ReferenceMap rmap;
    Value(atom).Explore(&rmap);
    EXPECT_EQ(1UL, rmap.size());
    EXPECT_TRUE(ContainsKey(rmap, atom));
    EXPECT_FALSE(GetExisting(rmap, atom));

    Value(atom).Explore(&rmap);
    EXPECT_EQ(1, rmap.size());
    EXPECT_TRUE(ContainsKey(rmap, atom));
    EXPECT_TRUE(GetExisting(rmap, atom));
  }
  {
    Value integer = Value::Integer(1);
    ReferenceMap rmap;
    integer.Explore(&rmap);
    EXPECT_EQ(1, rmap.size());
    EXPECT_TRUE(ContainsKey(rmap, integer));
    EXPECT_FALSE(GetExisting(rmap, integer));

    integer.Explore(&rmap);
    EXPECT_EQ(1, rmap.size());
    EXPECT_TRUE(ContainsKey(rmap, integer));
    EXPECT_TRUE(GetExisting(rmap, integer));
  }
}

// -----------------------------------------------------------------------------
// String representation

class ToStringTest : public testing::Test {
 protected:
  ToStringTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(ToStringTest, Primitives) {
  EXPECT_EQ("1", Value::Integer(1).ToString());
  EXPECT_EQ("1000", Value::Integer(1000).ToString());
  EXPECT_EQ("0", Value::Integer(0).ToString());
  EXPECT_EQ("~500", Value::Integer(-500).ToString());

  EXPECT_EQ("''", Atom::Get("")->ToString());
  EXPECT_EQ("atom1", Atom::Get("atom1")->ToString());
  EXPECT_EQ("'atom with \\'quote\\''",
            Atom::Get("atom with 'quote'")->ToString());

  EXPECT_EQ("{NewName}", Name::New(&store_)->ToString());
  EXPECT_EQ("{NewName}", ParseEval("{NewName}", &store_).ToString());
}

TEST_F(ToStringTest, Lists) {
  EXPECT_EQ("1|2|3|4",
            ParseEval("1 | 2 | 3 |4", &store_).ToString());
  EXPECT_EQ("[1 2 3]",
            ParseEval("[1 2 3]", &store_).ToString());
  EXPECT_EQ("1|2|3|_",
            ParseEval("1 | 2|3|_", &store_).ToString());

  EXPECT_EQ("[1 {NewName} 3]",
            ParseEval("[1 {NewName} 3]", &store_).ToString());
}

}  // namespace store
