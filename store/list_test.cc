// Tests for lists.
#include "store/values.h"

#include <memory>
using std::unique_ptr;

#include <gtest/gtest.h>

#include "base/stl-util.h"

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class ListTest : public testing::Test {
 protected:
  ListTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(ListTest, Basic) {
  Value v1 = Value::Integer(1);
  Value v2 = Atom::Get("atom");
  List* l = List::New(&store_, v1, v2);
  EXPECT_TRUE(l->RecordLabel() == KAtomList());
  EXPECT_TRUE(l->RecordArity() == KArityPair());
  EXPECT_EQ(2, l->RecordWidth());
  EXPECT_TRUE(v1 == l->TupleGet(0));
  EXPECT_TRUE(v2 == l->TupleGet(1));

  {
    unique_ptr<Value::ValueIterator> it_deleter(l->RecordIterValues());
    Value::ValueIterator& it = *it_deleter;
    EXPECT_FALSE(it.at_end());
    EXPECT_TRUE(v1 == *it);
    ++it;
    EXPECT_FALSE(it.at_end());
    EXPECT_TRUE(v2 == *it);
    ++it;
    EXPECT_TRUE(it.at_end());
  }

  {
    unique_ptr<Value::ItemIterator> it_deleter(l->RecordIterItems());
    Value::ItemIterator& it = *it_deleter;
    EXPECT_FALSE(it.at_end());
    EXPECT_EQ(1, IntValue((*it).first));
    EXPECT_TRUE(v1 == (*it).second);
    ++it;
    EXPECT_FALSE(it.at_end());
    EXPECT_EQ(2, IntValue((*it).first));
    EXPECT_TRUE(v2 == (*it).second);
    ++it;
    EXPECT_TRUE(it.at_end());
  }
}

TEST_F(ListTest, Finite) {
  Atom* atom1 = Atom::Get("atom1");
  Atom* atom2 = Atom::Get("atom2");
  Atom* nil = KAtomNil();
  List* l = List::New(&store_, atom1, List::New(&store_, atom2, nil));

  ReferenceMap rmap;
  Value(l).Explore(&rmap);

  EXPECT_EQ(5, rmap.size());
  EXPECT_FALSE(GetExisting(rmap, atom1));
  EXPECT_FALSE(GetExisting(rmap, atom2));
  EXPECT_FALSE(GetExisting(rmap, nil));
  EXPECT_FALSE(GetExisting(rmap, l));
  EXPECT_FALSE(GetExisting(rmap, l->tail()));

  Value tail = NULL;
  const int nvalues = l->GetValuesCount(&tail);
  EXPECT_EQ(2, nvalues);
  EXPECT_TRUE(tail == nil);
}

TEST_F(ListTest, Stream) {
  Atom* atom1 = Atom::Get("atom1");
  Atom* atom2 = Atom::Get("atom2");
  Variable* end = Variable::New(&store_);
  List* l = List::New(&store_, atom1, List::New(&store_, atom2, end));

  ReferenceMap rmap;
  Value(l).Explore(&rmap);

  EXPECT_EQ(5, rmap.size());
  EXPECT_FALSE(GetExisting(rmap, atom1));
  EXPECT_FALSE(GetExisting(rmap, atom2));
  EXPECT_FALSE(GetExisting(rmap, end));
  EXPECT_FALSE(GetExisting(rmap, l));
  EXPECT_FALSE(GetExisting(rmap, l->tail()));

  Value tail = NULL;
  const int nvalues = l->GetValuesCount(&tail);
  EXPECT_EQ(2, nvalues);
  EXPECT_TRUE(tail == end);
}

TEST_F(ListTest, Recursive) {
  Atom* atom1 = Atom::Get("atom1");
  Atom* atom2 = Atom::Get("atom2");
  Variable* end = Variable::New(&store_);
  List* l = List::New(&store_, atom1, List::New(&store_, atom2, end));
  CHECK(end->BindTo(l));

  ReferenceMap rmap;
  Value(l).Explore(&rmap);

  EXPECT_EQ(5, rmap.size());
  EXPECT_FALSE(GetExisting(rmap, atom1));
  EXPECT_FALSE(GetExisting(rmap, atom2));
  EXPECT_TRUE(GetExisting(rmap, l));
  EXPECT_FALSE(GetExisting(rmap, l->tail()));

  Value tail = NULL;
  const int nvalues = l->GetValuesCount(&tail);
  EXPECT_EQ(2, nvalues);
  EXPECT_TRUE(tail.Deref() == l);
}

}  // namespace store
