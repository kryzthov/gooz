#include "combinators/oznode_eval_visitor.h"

#include <gtest/gtest.h>

#include "combinators/ozparser.h"
#include "store/store.h"

namespace combinators { namespace oz {

const uint64 kStoreSize = 1024 * 1024;

class EvalVisitorTest : public testing::Test {
 public:
  EvalVisitorTest()
      : store_(kStoreSize),
        visitor_(&store_) {
  }

  store::Value Eval(const string& source) {
    OzParser parser;
    CHECK(parser.Parse(source)) << "Error parsing: " << source;
    shared_ptr<OzNodeGeneric> root =
        std::dynamic_pointer_cast<OzNodeGeneric>(parser.root());
    LOG(INFO) << "AST:\n" << *root << std::endl;
    CHECK_EQ(OzLexemType::TOP_LEVEL, root->type);
    store::Value result;
    for (shared_ptr<AbstractOzNode> node : root->nodes) {
      store::Value value = visitor_.Eval(node.get());
      LOG(INFO) << "Value: " << value.ToString() << std::endl;
      result = value;
    }
    return result;
  }

  store::StaticStore store_;
  EvalVisitor visitor_;
};

TEST_F(EvalVisitorTest, Atom) {
  const string source = "atom";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::ATOM));
  EXPECT_EQ("atom", value.ToString());
}

TEST_F(EvalVisitorTest, Integer_1) {
  const string source = "15";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::SMALL_INTEGER));
  EXPECT_EQ("15", value.ToString());
}

TEST_F(EvalVisitorTest, Integer_2) {
  const string source = "123456789123456789123456789";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::INTEGER));
  EXPECT_EQ("123456789123456789123456789", value.ToString());
}

TEST_F(EvalVisitorTest, Integer_3) {
  const string source = "~15";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::SMALL_INTEGER));
  EXPECT_EQ("~15", value.ToString());
}

TEST_F(EvalVisitorTest, Tuple_1) {
  const string source = "tuple(a b c)";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::TUPLE));
  EXPECT_EQ("tuple(a b c)", value.ToString());
}

TEST_F(EvalVisitorTest, Tuple_2) {
  const string source = "a # b";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::TUPLE));
  EXPECT_EQ("a#b", value.ToString());
}

TEST_F(EvalVisitorTest, Record_1) {
  const string source = "record(a:b c:d)";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::RECORD));
  EXPECT_EQ("record(a:b c:d)", value.ToString());
}

TEST_F(EvalVisitorTest, Record_2) {
  const string source = "record(e f a:b c:d)";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::RECORD));
  EXPECT_EQ("record(1:e 2:f a:b c:d)", value.ToString());  // FIXME
}

TEST_F(EvalVisitorTest, OpenRecord) {
  const string source = "record(a:b ...)";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::OPEN_RECORD));
  EXPECT_EQ("record(a:b ...)", value.ToString());
}

TEST_F(EvalVisitorTest, List_1) {
  const string source = "a | b";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::LIST));
  EXPECT_EQ("a|b", value.ToString());
}

TEST_F(EvalVisitorTest, List_2) {
  const string source = "a | b | c";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::LIST));
  EXPECT_EQ("a|b|c", value.ToString());
}

TEST_F(EvalVisitorTest, List_3) {
  const string source = "[a b c]";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::LIST));
  EXPECT_EQ("[a b c]", value.ToString());
}

TEST_F(EvalVisitorTest, Priority_ListTuple_1) {
  const string source = "[a b#c d]";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::LIST));
  EXPECT_EQ("[a b#c d]", value.ToString());
}

TEST_F(EvalVisitorTest, Priority_ListTuple_2) {
  const string source = "[a b]#[c d]";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::TUPLE));
  EXPECT_EQ("[a b]#[c d]", value.ToString());
}

TEST_F(EvalVisitorTest, Var_1) {
  const string source = "X = a";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value.Deref(), store::Value::ATOM));
  EXPECT_EQ("a", visitor_.vars()["X"].ToString());
}

TEST_F(EvalVisitorTest, Var_2) {
  const string source =
      "X = a \n"
      "Y = record(X) \n";
  Eval(source);
  EXPECT_EQ("record(a)", visitor_.vars()["Y"].ToString());
}

TEST_F(EvalVisitorTest, Name_1) {
  const string source = "{NewName}";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::NAME));
  EXPECT_EQ("{NewName}", value.ToString());
}

TEST_F(EvalVisitorTest, Cell_1) {
  const string source = "{NewCell a}";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::CELL));
  EXPECT_EQ("{NewCell a}", value.ToString());
}

TEST_F(EvalVisitorTest, Array_1) {
  const string source = "{NewArray 3 a}";
  store::Value value = Eval(source);
  EXPECT_TRUE(store::HasType(value, store::Value::ARRAY));
  EXPECT_EQ("{NewArray array(a a a)}", value.ToString());
}

}}  // namespace combinators::oz
