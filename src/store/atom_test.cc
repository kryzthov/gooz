#include "store/values.h"

#include <memory>
using std::unique_ptr;

#include <gtest/gtest.h>

namespace store {

TEST(Atom, Basics) {
  Atom* const hello = Atom::Get("hello");
  EXPECT_EQ(hello->value(), "hello");
  EXPECT_EQ(0, hello->RecordWidth());
  EXPECT_EQ(KArityEmpty(), hello->RecordArity());
  {
    unique_ptr<Value::ValueIterator> it_deleter(hello->RecordIterValues());
    Value::ValueIterator& it = *it_deleter;
    EXPECT_TRUE(it.at_end());
  }
  {
    unique_ptr<Value::ItemIterator> it_deleter(hello->RecordIterItems());
    Value::ItemIterator& it = *it_deleter;
    EXPECT_TRUE(it.at_end());
  }
  Atom* const coucou1 = Atom::Get("coucou");
  Atom* const coucou2 = Atom::Get("coucou");
  EXPECT_EQ(coucou1, coucou2);
}

}  // namespace store
