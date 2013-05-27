// Tests for open-records.

#include "store/values.h"

#include <memory>
using std::unique_ptr;

#include <gtest/gtest.h>

#include "base/stl-util.h"

namespace store {

const uint64 kStoreSize = 1024 * 1024;

class OpenRecordTest : public testing::Test {
 protected:
  OpenRecordTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

TEST_F(OpenRecordTest, Basic) {
  Atom* label = Atom::Get("label");
  OpenRecord* orecord = OpenRecord::New(&store_, label);
  EXPECT_TRUE(orecord->label() == label);

  Arity* arity0 = orecord->GetArity(&store_);
  EXPECT_TRUE(arity0->IsTuple());
  EXPECT_EQ(0UL, arity0->size());

  EXPECT_TRUE(orecord->GetRecord(&store_) == label);

  // Add one feature
  EXPECT_TRUE(orecord->Set(1, Atom::Get("x")));
  EXPECT_TRUE(orecord->Get(1) == Atom::Get("x"));

  Arity* arity1 = orecord->GetArity(&store_);
  EXPECT_TRUE(arity1->IsTuple());
  EXPECT_EQ(1UL, arity1->size());

  Tuple* tuple1 = orecord->GetRecord(&store_).as<Tuple>();
  EXPECT_EQ(1UL, tuple1->size());

  // Cannot add an existing feature
  EXPECT_TRUE(!orecord->Set(1, Atom::Get("y")));
  EXPECT_TRUE(orecord->Get(1) == Atom::Get("x"));

  // Add a second feature
  EXPECT_TRUE(orecord->Set(3, Atom::Get("y")));
  EXPECT_TRUE(orecord->Get(3) == Atom::Get("y"));

  unique_ptr<Value::ValueIterator> it_deleter(orecord->OpenRecordIterValues());
  Value::ValueIterator& it = *it_deleter;
  EXPECT_FALSE(it.at_end());
  EXPECT_TRUE(*it == Atom::Get("x"));
  ++it;
  EXPECT_FALSE(it.at_end());
  EXPECT_TRUE(*it == Atom::Get("y"));
  ++it;
  EXPECT_TRUE(it.at_end());

  Arity* arity2 = orecord->GetArity(&store_);
  CHECK(!arity2->IsTuple());
  CHECK_EQ(2UL, arity2->size());

  Record* record = orecord->GetRecord(&store_).as<Record>();
  EXPECT_EQ(2UL, record->size());
}

TEST_F(OpenRecordTest, API) {
  Value label = New::Atom(&store_, "label");
  Value x = New::Atom(&store_, "x");
  Value y = New::Atom(&store_, "y");

  // Empty open-record:
  OpenRecord* orecord = OpenRecord::New(&store_, label);

  EXPECT_TRUE(orecord->RecordLabel() == label);
  EXPECT_EQ(0UL, orecord->OpenRecordWidth());
  EXPECT_TRUE(Equals(orecord->OpenRecordArity(&store_),
                     New::TupleArity(&store_, 0)));

  try {
    orecord->RecordGet(x);
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  try {
    orecord->RecordHas(x);
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  try {
    orecord->RecordArity();
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  try {
    orecord->RecordWidth();
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  // Add feature 'x':
  orecord->Set(x, Value::Integer(12));

  EXPECT_EQ(1UL, orecord->OpenRecordWidth());
  EXPECT_TRUE(Equals(orecord->OpenRecordArity(&store_),
                     New::Arity(&store_, x)));

  try {
    orecord->RecordGet(x);
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  try {
    orecord->RecordHas(x);
    FAIL();
  } catch (SuspendThread st) {
    SUCCEED();
  }

  // Add feature 'y':
  orecord->Set(y, Value::Integer(23));

  EXPECT_EQ(2UL, orecord->OpenRecordWidth());
  EXPECT_TRUE(Equals(orecord->OpenRecordArity(&store_),
                     New::Arity(&store_, x, y)));
}

}  // namespace store
