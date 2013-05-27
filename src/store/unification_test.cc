// Tests for unification.

#include "store/value.h"
#include "store/values.h"

#include <gtest/gtest.h>

#include "base/stl-util.h"
#include "combinators/oznode_eval_visitor.h"

using combinators::oz::ParseEval;

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class UnifyTest : public testing::Test {
 protected:
  UnifyTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

// -----------------------------------------------------------------------------

TEST_F(UnifyTest, Primitives) {
  EXPECT_TRUE(Unify(Boolean::True, Boolean::True));
  EXPECT_TRUE(Unify(Boolean::False, Boolean::False));
  EXPECT_FALSE(Unify(Boolean::False, Boolean::True));
  EXPECT_FALSE(Unify(Boolean::True, Boolean::False));

  Value i1 = Value::Integer(1);
  Value i1bis = Value::Integer(1);
  Value i2 = Value::Integer(2);

  Value name1 = Name::New(&store_);
  Value name2 = Name::New(&store_);

  EXPECT_TRUE(Unify(i1, i1));
  EXPECT_FALSE(Unify(i1, i2));
  EXPECT_TRUE(Unify(i1, i1bis));

  EXPECT_TRUE(Unify(name1, name1));
  EXPECT_FALSE(Unify(name1, name2));
  EXPECT_FALSE(Unify(name1, i1));

  EXPECT_FALSE(Unify(Boolean::True, i1));
  EXPECT_FALSE(Unify(i2, Boolean::False));
}

TEST_F(UnifyTest, FreeVariable) {
  Variable* v1 = Variable::New(&store_);
  Variable* v2 = Variable::New(&store_);

  EXPECT_TRUE(v1->IsFree());
  EXPECT_TRUE(v2->IsFree());

  EXPECT_TRUE(Unify(v1, v1));  // No-op
  EXPECT_TRUE(v1->IsFree());
  EXPECT_TRUE(v2->IsFree());
  EXPECT_TRUE(Deref(v1) != Deref(v2));

  EXPECT_TRUE(Unify(v1, v2));
  EXPECT_TRUE(Deref(v1) == Deref(v2));

  EXPECT_TRUE(Unify(v2, v1));
  EXPECT_TRUE(Deref(v1) == Deref(v2));
}

TEST_F(UnifyTest, SimpleRecord) {
  {
    Value record1 = ParseEval("record(a:b c d:e)", &store_);
    Value record2 = ParseEval("record(a:b c d:e)", &store_);
    EXPECT_TRUE(Unify(record1, record2));
  }
  {
    Value record1 = ParseEval("record(a:b c d:e)", &store_);
    Value record2 = ParseEval("record(a:b 1 d:e)", &store_);
    EXPECT_FALSE(Unify(record1, record2));
  }
}

TEST_F(UnifyTest, Tuple) {
  Tuple* tuple1 = Tuple::New(&store_, Atom::Get("label"), 1);
  Tuple* tuple2 = Tuple::New(&store_, Atom::Get("label"), 2);
  Tuple* tuple3 = Tuple::New(&store_, Atom::Get("another"), 1);
  Tuple* tuple4 = Tuple::New(&store_, Atom::Get("label"), 1);

  EXPECT_FALSE(Unify(tuple1, tuple2));
  EXPECT_FALSE(Unify(tuple1, tuple3));
  EXPECT_TRUE(Unify(tuple1, tuple4));
  EXPECT_TRUE(Deref(tuple1->TupleGet(0)) == Deref(tuple4->TupleGet(0)));
}

TEST_F(UnifyTest, RecordWithFreeVar) {
  Record* record1 = ParseEval("record(a:b c d:e)", &store_).as<Record>();
  Record* record2 = ParseEval("record(a:b _ d:e)", &store_).as<Record>();

  Value unbound = record2->Get(Value::Integer(1));
  EXPECT_FALSE(IsDet(unbound));

  EXPECT_TRUE(Unify(record1, record2));

  EXPECT_TRUE(unbound.Deref() == Atom::Get("c"));
}

TEST_F(UnifyTest, RecordAtomicFailure) {
  Record* record1 = ParseEval("record(a:_ b:1)", &store_).as<Record>();
  Record* record2 = ParseEval("record(a:1 b:2)", &store_).as<Record>();

  Value unbound = record1->Get(Atom::Get("a"));
  EXPECT_FALSE(IsDet(unbound));

  EXPECT_FALSE(Unify(record1, record2));

  EXPECT_FALSE(IsDet(unbound));
}

TEST_F(UnifyTest, RecursiveRecord) {
  Record* record1 =
      ParseEval("record(a:1 b:_ c:nested(_))", &store_).as<Record>();
  Value nested = record1->Get("c");
  ASSERT_TRUE(Unify(nested.RecordGet(Value::Integer(1)), record1));

  Record* record2 = ParseEval("record(a:_ b:_ c:_)", &store_).as<Record>();

  EXPECT_TRUE(Unify(record1, record2));

  EXPECT_FALSE(IsDet(record1->Get(Atom::Get("b"))));
  EXPECT_EQ(IntValue(record1->Get("a")),
            IntValue(record2->Get("a").Deref()));

  Record* record3 = ParseEval("record(a:_ b:x c:y)", &store_).as<Record>();
  EXPECT_FALSE(Unify(record1, record3));
  EXPECT_FALSE(IsDet(record3->Get("a")));
  EXPECT_FALSE(IsDet(record1->Get("b")));
}

TEST_F(UnifyTest, ListWithFreeVar) {
  List* list1 = ParseEval("1 | 2 | c | _", &store_).as<List>();
  LOG(INFO) << "Definition for list1:\n" << list1->ToString();

  List* list2 = ParseEval("[1 _ _]", &store_).as<List>();
  LOG(INFO) << "Definition for list2:\n" << list2->ToString();

  Value eol1 = NULL;
  EXPECT_EQ(3, list1->GetValuesCount(&eol1));
  EXPECT_FALSE(IsDet(eol1));

  Value eol2 = NULL;
  EXPECT_EQ(3, list2->GetValuesCount(&eol2));
  EXPECT_TRUE(eol2 == KAtomNil());

  LOG(INFO) << "Unifying list1 = list2";
  EXPECT_TRUE(Unify(list1, list2));
  LOG(INFO) << "list1:\n" << list1->ToString();
  LOG(INFO) << "list2:\n" << list2->ToString();

  EXPECT_EQ(3, list2->GetValuesCount(&eol1));
  EXPECT_TRUE(Deref(eol1) == KAtomNil());
}

TEST_F(UnifyTest, RecursiveList) {
  List* list1 = ParseEval("1 | 2 | _", &store_).as<List>();
  ASSERT_TRUE(Unify(list1->Next()->tail(), list1));
  Value eol1 = NULL;
  EXPECT_EQ(2, list1->GetValuesCount(&eol1));
  EXPECT_TRUE(eol1.Deref() == list1);

  List* list2 = ParseEval("[1 _]", &store_).as<List>();
  Value eol2 = NULL;
  EXPECT_EQ(2, list2->GetValuesCount(&eol2));
  EXPECT_TRUE(eol2 == KAtomNil());
  EXPECT_FALSE(IsDet(list2->Next()->head()));
  EXPECT_FALSE(Unify(list1, list2));
  EXPECT_FALSE(IsDet(list2->Next()->head()));
  EXPECT_EQ(2, list2->GetValuesCount(&eol2));

  List* list3 = ParseEval("1 | _", &store_).as<List>();
  Value eol3;
  EXPECT_EQ(1, list3->GetValuesCount(&eol3));
  EXPECT_FALSE(IsDet(eol3));

  EXPECT_TRUE(Unify(list1, list3));

  EXPECT_EQ(list1->Next(), list3->Next());
  EXPECT_EQ(3, list3->GetValuesCount(&eol3));
  EXPECT_TRUE(eol3 == list3->Next());
}

TEST_F(UnifyTest, OpenRecord) {
  {
    OpenRecord* or1 = ParseEval("r(a:b ...)", &store_).as<OpenRecord>();
    OpenRecord* or2 = ParseEval("r(c:d ...)", &store_).as<OpenRecord>();

    EXPECT_TRUE(Unify(or1, or2));
    EXPECT_TRUE(Deref(or1) == Deref(or2));

    OpenRecord* or3 = Deref(or1).as<OpenRecord>();
    EXPECT_EQ(2, or3->size());
    EXPECT_TRUE(Equals(ParseEval("r(a:b c:d)", &store_),
                       or3->GetRecord(&store_)));
  }
  {
    OpenRecord* or1 = ParseEval("r(a:b ...)", &store_).as<OpenRecord>();
    OpenRecord* or2 = ParseEval("r2(c:d ...)", &store_).as<OpenRecord>();

    EXPECT_FALSE(Unify(or1, or2));
    EXPECT_EQ(1, or1->size());
    EXPECT_TRUE(or1->Has("a"));
    EXPECT_EQ(1, or2->size());
    EXPECT_TRUE(or2->Has("c"));
  }
  {
    OpenRecord* or1 = ParseEval("r(a:b ...)", &store_).as<OpenRecord>();
    Record* or2 = ParseEval("r(a:b)", &store_).as<Record>();

    EXPECT_TRUE(Unify(or1, or2));
    EXPECT_TRUE(Deref(or1) == Deref(or2));
  }
  {
    OpenRecord* orec = OpenRecord::New(&store_, Atom::Get("r"));
    Tuple* tuple = Tuple::New(&store_, Atom::Get("r"), 10);

    EXPECT_TRUE(Unify(orec, tuple));
    EXPECT_TRUE(Deref(orec) == tuple);
  }
  {
    OpenRecord* orec = OpenRecord::New(&store_, KAtomList());
    List* list = List::New(&store_, Atom::Get("a"), Atom::Get("b"));

    EXPECT_TRUE(Unify(orec, list));
    EXPECT_TRUE(Deref(orec) == list);
  }
}

}  // namespace store
