#include "store/values.h"

#include <gtest/gtest.h>

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class ArityTest : public testing::Test {
 protected:
  ArityTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(ArityTest, EmptyArity) {
  const vector<Value> features;
  const Arity* arity = Arity::Get(features);
  EXPECT_EQ(0UL, arity->size());
  EXPECT_EQ(arity, Arity::Get(features));
}

TEST_F(ArityTest, SingleFeature) {
  vector<Value> features;
  features.push_back(Atom::Get("coucou"));
  const Arity* arity = Arity::Get(features);
  EXPECT_EQ(1UL, arity->size());
  EXPECT_EQ(arity, Arity::Get(features));
}

TEST_F(ArityTest, AtomOrdering) {
  vector<Value> features;
  features.push_back(Atom::Get("atom1"));
  features.push_back(Atom::Get("atom2"));
  features.push_back(Atom::Get("atom3"));
  features.push_back(Atom::Get("atom4"));
  features.push_back(Atom::Get("atom5"));
  const Arity* arity1 = Arity::Get(features);
  EXPECT_EQ(5UL, arity1->size());

  features.clear();
  features.push_back(Atom::Get("atom1"));
  features.push_back(Atom::Get("atom5"));
  features.push_back(Atom::Get("atom3"));
  features.push_back(Atom::Get("atom2"));
  features.push_back(Atom::Get("atom4"));
  const Arity* arity2 = Arity::Get(features);
  EXPECT_EQ(5UL, arity2->size());

  EXPECT_EQ(arity1, arity2);
}

TEST_F(ArityTest, FeaturesOrdering) {
  Name* name1 = Name::New(&store_);
  Name* name2 = Name::New(&store_);
  ASSERT_TRUE(Literal::LessThan(name1, name2));

  // Build the arity:
  //     arity(1 15 51 atom1 atom2 atom3 atom4 atom5 name1 name2)
  vector<Value> features;
  features.push_back(Atom::Get("atom5"));
  features.push_back(Value::Integer(15));
  features.push_back(Atom::Get("atom2"));
  features.push_back(Value::Integer(51));
  features.push_back(Atom::Get("atom1"));
  features.push_back(Value::Integer(1));
  features.push_back(Atom::Get("atom4"));
  features.push_back(Atom::Get("atom3"));
  features.push_back(name1);
  features.push_back(name2);
  Arity* arity = Arity::Get(features);

  EXPECT_EQ(10UL, arity->size());

  EXPECT_EQ(1, IntValue(arity->features()[0]));
  EXPECT_EQ(15, IntValue(arity->features()[1]));
  EXPECT_EQ(51, IntValue(arity->features()[2]));

  EXPECT_EQ(Value::ATOM, arity->features()[3].type());
  EXPECT_EQ("atom1", arity->features()[3].as<Atom>()->value());

  EXPECT_EQ(0UL, arity->Map(1));
  EXPECT_EQ(1UL, arity->Map(15));
  EXPECT_EQ(2UL, arity->Map(51));
  EXPECT_EQ(3UL, arity->Map("atom1"));
  EXPECT_EQ(4UL, arity->Map("atom2"));
  EXPECT_EQ(5UL, arity->Map("atom3"));
  EXPECT_EQ(6UL, arity->Map("atom4"));
  EXPECT_EQ(7UL, arity->Map("atom5"));
  EXPECT_EQ(8UL, arity->Map(name1));
  EXPECT_EQ(9UL, arity->Map(name2));

  EXPECT_FALSE(arity->Has(-1));
  EXPECT_FALSE(arity->Has("atom0"));
  EXPECT_TRUE(arity->Has(15));
  EXPECT_TRUE(arity->Has("atom5"));
}

TEST_F(ArityTest, IsSubsetOf) {
  Name* name1 = Name::New(&store_);
  Name* name2 = Name::New(&store_);
  ASSERT_TRUE(Literal::LessThan(name1, name2));

  Value arity1[] = {
    Value::Integer(2),
    Atom::Get("atom1"),
  };

  Value arity2[] = {
    Value::Integer(1),
    Value::Integer(2),
    Value::Integer(3),
    Atom::Get("atom1"),
    Atom::Get("atom2"),
  };

  Value arity3[] = {
    Value::Integer(1),
    Value::Integer(2),
    Value::Integer(4),
    Atom::Get("atom1"),
    Atom::Get("atom2"),
  };

  Value arity4[] = {
    Value::Integer(1),
    Value::Integer(2),
    Value::Integer(3),
    Value::Integer(4),
    Value::Integer(5),
    Atom::Get("atom1"),
    Atom::Get("atom2"),
    name1,
    name2,
  };

  Arity* a0 = KArityEmpty();  // empty arity
  Arity* a1 = Arity::Get(ArraySize(arity1), arity1);
  Arity* a2 = Arity::Get(ArraySize(arity2), arity2);
  Arity* a3 = Arity::Get(ArraySize(arity3), arity3);
  Arity* a4 = Arity::Get(ArraySize(arity4), arity4);

  EXPECT_TRUE(a0->IsSubsetOf(a0));
  EXPECT_TRUE(a1->IsSubsetOf(a1));
  EXPECT_TRUE(a2->IsSubsetOf(a2));
  EXPECT_TRUE(a3->IsSubsetOf(a3));
  EXPECT_TRUE(a4->IsSubsetOf(a4));

  EXPECT_TRUE(a0->IsSubsetOf(a1));
  EXPECT_TRUE(a0->IsSubsetOf(a2));

  EXPECT_TRUE(a1->IsSubsetOf(a2));

  EXPECT_FALSE(a1->IsSubsetOf(a0));

  EXPECT_FALSE(a2->IsSubsetOf(a0));
  EXPECT_FALSE(a2->IsSubsetOf(a1));

  EXPECT_FALSE(a2->IsSubsetOf(a3));
  EXPECT_FALSE(a3->IsSubsetOf(a2));
}

TEST_F(ArityTest, Serialization) {
  {
    Arity* arity = KArityEmpty();
    EXPECT_EQ("{NewArity 0 features()}", Value(arity).ToString());
  }
  {
    Arity* arity = KAritySingleton();
    EXPECT_EQ("{NewArity 1 features(1)}", Value(arity).ToString());
  }
  {
    Value features[] = { Atom::Get("atom1") };
    Arity* arity = Arity::Get(ArraySize(features), features);
    EXPECT_EQ("{NewArity 1 features(atom1)}", Value(arity).ToString());
  }
  {
    Value features[] = {
      Atom::Get("atom1"),
      Value::Integer(-1),
      Name::New(&store_)
    };
    Arity* arity = Arity::Get(ArraySize(features), features);
    EXPECT_EQ("{NewArity 3 features(~1 atom1 {NewName})}",
              Value(arity).ToString());
  }
}

TEST_F(ArityTest, Extend) {
  Value i1 = Value::Integer(1);
  Value i2 = Value::Integer(2);
  Value i3 = Value::Integer(3);

  Arity* empty = KArityEmpty();
  EXPECT_EQ(0UL, empty->size());

  Arity* single_one = empty->Extend(i1);
  EXPECT_EQ(1UL, single_one->size());

  Arity* single_two = empty->Extend(i2);
  EXPECT_EQ(1UL, single_two->size());

  Arity* tuple2_1 = single_one->Extend(i2);
  Arity* tuple2_2 = single_two->Extend(i1);
  EXPECT_EQ(tuple2_1, tuple2_2);
  EXPECT_EQ(KArityPair(), tuple2_1);

  Arity* tuple3 = single_one->Extend(i3)->Extend(i2);
  EXPECT_EQ(Arity::GetTuple(3), tuple3);
}

TEST_F(ArityTest, Subtract) {
  Value i1 = Value::Integer(1);
  Value i2 = Value::Integer(2);
  Value i3 = Value::Integer(3);
  Value i4 = Value::Integer(4);

  EXPECT_EQ(Arity::GetTuple(3), Arity::GetTuple(4)->Subtract(i4));
  EXPECT_EQ(Arity::Get(i2), KArityPair()->Subtract(i1));
  EXPECT_EQ(Arity::Get(i3), Arity::GetTuple(3)->Subtract(i1)->Subtract(i2));
  EXPECT_EQ(Arity::Get(i3), Arity::GetTuple(3)->Subtract(i2)->Subtract(i1));
  EXPECT_EQ(KArityEmpty(), KArityPair()->Subtract(i2)->Subtract(i1));
}

TEST_F(ArityTest, LessThan) {
  EXPECT_FALSE(KArityEmpty()->LessThan(KArityEmpty()));
  EXPECT_TRUE(KArityEmpty()->LessThan(KAritySingleton()));
  EXPECT_FALSE(KAritySingleton()->LessThan(KArityEmpty()));
  EXPECT_FALSE(KAritySingleton()->LessThan(KAritySingleton()));

  EXPECT_TRUE(KArityPair()->LessThan(
      Arity::GetTuple(3)->Subtract(Value::Integer(2))));
}

TEST_F(ArityTest, SubsetMask) {
  Value i1 = Value::Integer(1);
  Value i2 = Value::Integer(2);
  Value i3 = Value::Integer(3);
  Value i4 = Value::Integer(4);

  EXPECT_TRUE(Equals(
      Value::Integer(0),
      Arity::GetTuple(10)->ComputeSubsetMask(&store_, KArityEmpty())));
  EXPECT_TRUE(Equals(
      Value::Integer(1),
      Arity::GetTuple(10)->ComputeSubsetMask(&store_, KAritySingleton())));
  EXPECT_TRUE(Equals(
      Value::Integer(1 | 2),
      Arity::GetTuple(10)->ComputeSubsetMask(&store_, KArityPair())));
  EXPECT_TRUE(Equals(
      Value::Integer(0),
      KArityEmpty()->ComputeSubsetMask(&store_, KArityPair())));
  EXPECT_TRUE(Equals(
      Value::Integer(1),
      KAritySingleton()->ComputeSubsetMask(&store_, KArityPair())));

  EXPECT_TRUE(Equals(
      Value::Integer(2),
      Arity::Get(i1, i3)->ComputeSubsetMask(&store_, Arity::Get(i3))));
  EXPECT_TRUE(Equals(
      Value::Integer(0),
      Arity::Get(i1, i3)->ComputeSubsetMask(&store_, Arity::Get(i4))));
  EXPECT_TRUE(Equals(
      Value::Integer(4),
      Arity::Get(i1, i2, i4)->ComputeSubsetMask(&store_, Arity::Get(i4))));

}


}  // namespace store
