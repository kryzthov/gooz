#include "combinators/ozparser.h"

#include <gtest/gtest.h>

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

class TopLevelScopeParserTest : public testing::Test {
 protected:
  TopLevelScopeParserTest() : parser_(nullptr) {}

  void Init(const string& text) {
    text_ = text;
    ParsingResult<CharStream> lres = lexer_.Parse(text_);
    ASSERT_EQ(ParsingStatus::OK, lres.status);

    pres_ = parser_.Parse(lexer_.lexems(), root_);
  }

  string text_;
  OzLexer lexer_;
  TopLevelScopeParser parser_;
  shared_ptr<AbstractOzNode> root_;
  ParsingResult<OzLexemStream> pres_;
};

TEST_F(TopLevelScopeParserTest, Basic_0) {
  Init("X");
  EXPECT_EQ(ParsingStatus::OK, pres_.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeVar& var = dynamic_cast<OzNodeVar&>(*root.nodes[0]);
  EXPECT_EQ("X", var.var_name);
}

TEST_F(TopLevelScopeParserTest, Basic_1) {
  Init("(X)");
  EXPECT_EQ(ParsingStatus::OK, pres_.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeGeneric& node = dynamic_cast<OzNodeGeneric&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::BEGIN, node.type);
  EXPECT_EQ(1UL, node.nodes.size());
  const OzNodeVar& var = dynamic_cast<OzNodeVar&>(*node.nodes[0]);
  EXPECT_EQ("X", var.var_name);
}

TEST_F(TopLevelScopeParserTest, Basic_2) {
  Init("((X))");
  EXPECT_EQ(ParsingStatus::OK, pres_.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeGeneric& node =
      dynamic_cast<OzNodeGeneric&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::BEGIN, node.type);
  EXPECT_EQ(1UL, node.nodes.size());
  const OzNodeGeneric& node2 =
      dynamic_cast<OzNodeGeneric&>(*node.nodes[0]);
  EXPECT_EQ(OzLexemType::BEGIN, node2.type);
  EXPECT_EQ(1UL, node2.nodes.size());
  EXPECT_EQ(OzLexemType::VARIABLE, node2.nodes[0]->type);
}

TEST_F(TopLevelScopeParserTest, Basic_3) {
  Init("({X})");
  EXPECT_EQ(ParsingStatus::OK, pres_.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeGeneric& node =
      dynamic_cast<OzNodeGeneric&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::BEGIN, node.type);
  EXPECT_EQ(1UL, node.nodes.size());
  const OzNodeGeneric& node2 =
      dynamic_cast<OzNodeGeneric&>(*node.nodes[0]);
  EXPECT_EQ(OzLexemType::CALL_BEGIN, node2.type);
  EXPECT_EQ(1UL, node2.nodes.size());
  EXPECT_EQ(OzLexemType::VARIABLE, node2.nodes[0]->type);
}

TEST_F(TopLevelScopeParserTest, Basic_4) {
  Init("functor F end");
  EXPECT_EQ(ParsingStatus::OK, pres_.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeGeneric& node =
      dynamic_cast<OzNodeGeneric&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::FUNCTOR, node.type);
  EXPECT_EQ(1UL, node.nodes.size());
  EXPECT_EQ(OzLexemType::VARIABLE, node.nodes[0]->type);
}


TEST_F(TopLevelScopeParserTest, Error_1) {
  Init("(X}");
  EXPECT_EQ(ParsingStatus::FAILED, pres_.status);
  LOG(INFO) << "Expected error: " << pres_.errors[0];
}

TEST_F(TopLevelScopeParserTest, Error_2) {
  Init("functor X");
  EXPECT_EQ(ParsingStatus::FAILED, pres_.status);
  LOG(INFO) << "Expected error: " << pres_.errors[0];
}

// -----------------------------------------------------------------------------

class MidLevelScopeParserTest : public testing::Test {
 protected:
  void Init(const string& text) {
    text_ = text;
    ParsingResult<CharStream> lres = lexer_.Parse(text_);
    ASSERT_EQ(ParsingStatus::OK, lres.status);
  }

  string text_;
  OzLexer lexer_;
  TopLevelScopeParser parser_;
  shared_ptr<AbstractOzNode> root_;
};

TEST_F(MidLevelScopeParserTest, Local_1) {
  Init("(X)");
  ParsingResult<OzLexemStream> pres = parser_.Parse(lexer_.lexems(), root_);
  EXPECT_EQ(ParsingStatus::OK, pres.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeGeneric& node = dynamic_cast<OzNodeGeneric&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::BEGIN, node.type);
  EXPECT_EQ(1UL, node.nodes.size());
}

TEST_F(MidLevelScopeParserTest, Local_2) {
  Init("(X in Y)");
  ParsingResult<OzLexemStream> pres = parser_.Parse(lexer_.lexems(), root_);
  EXPECT_EQ(ParsingStatus::OK, pres.status);
  OzNodeGeneric& root = dynamic_cast<OzNodeGeneric&>(*root_);
  LOG(INFO) << "AST:\n" << root;

  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  EXPECT_EQ(1UL, root.nodes.size());
  const OzNodeLocal& node = dynamic_cast<OzNodeLocal&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::NODE_LOCAL, node.type);
  EXPECT_EQ(1UL, node.defs->nodes.size());
  EXPECT_EQ(1UL, node.body->nodes.size());
}

// -----------------------------------------------------------------------------

class ExpressionParserTest : public testing::Test {
 public:
  string text_;
  OzLexer lexer_;
  TopLevelScopeParser tlsparser_;
  ExpressionParser parser_;
  shared_ptr<AbstractOzNode> root_;
  OzNodeGeneric* generic_;

  ExpressionParserTest() : tlsparser_(nullptr) {}

  void Init(const string& text) {
    text_ = text;
    ParsingResult<CharStream> lres = lexer_.Parse(text_);
    ASSERT_EQ(ParsingStatus::OK, lres.status);

    ParsingResult<OzLexemStream> pres =
        tlsparser_.Parse(lexer_.lexems(), root_);
    ASSERT_EQ(ParsingStatus::OK, pres.status);
    generic_ = dynamic_cast<OzNodeGeneric*>(root_.get());
  }
};

TEST_F(ExpressionParserTest, ParseUnaryOp_1) {
  Init("@X");
  parser_.ParseUnaryOperator(generic_, OzLexemType::CELL_ACCESS);

  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeUnaryOp& node = dynamic_cast<OzNodeUnaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::CELL_ACCESS, node.type);
}

TEST_F(ExpressionParserTest, ParseUnaryOp_2) {
  Init("@!X @Y");
  parser_.ParseUnaryOperator(generic_, OzLexemType::VAR_NODEF);
  parser_.ParseUnaryOperator(generic_, OzLexemType::CELL_ACCESS);

  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(2UL, generic_->nodes.size());
  const OzNodeUnaryOp& node0 = dynamic_cast<OzNodeUnaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::CELL_ACCESS, node0.type);

  const OzNodeUnaryOp& subnode = dynamic_cast<OzNodeUnaryOp&>(*node0.operand);
  EXPECT_EQ(OzLexemType::VAR_NODEF, subnode.type);

  const OzNodeUnaryOp& node1 = dynamic_cast<OzNodeUnaryOp&>(*generic_->nodes[1]);
  EXPECT_EQ(OzLexemType::CELL_ACCESS, node1.type);
}

TEST_F(ExpressionParserTest, ParseUnaryOp_3) {
  Init("X");
  parser_.ParseUnaryOperator(generic_, OzLexemType::CELL_ACCESS);

  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeVar& node = dynamic_cast<OzNodeVar&>(*generic_->nodes[0]);
  EXPECT_EQ("X", node.var_name);
}

TEST_F(ExpressionParserTest, ParseBinaryOp_1) {
  Init("X : Y");
  parser_.ParseBinaryOperatorLTR(generic_, OzLexemType::RECORD_DEF_FEATURE);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeBinaryOp& node =
      dynamic_cast<OzNodeBinaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::RECORD_DEF_FEATURE, node.type);
}

TEST_F(ExpressionParserTest, ParseBinaryOp_2) {
  Init("X : Y Z : T");
  parser_.ParseBinaryOperatorLTR(generic_, OzLexemType::RECORD_DEF_FEATURE);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(2UL, generic_->nodes.size());
  const OzNodeBinaryOp& node1 =
      dynamic_cast<OzNodeBinaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::RECORD_DEF_FEATURE, node1.type);
  const OzNodeBinaryOp& node2 =
      dynamic_cast<OzNodeBinaryOp&>(*generic_->nodes[1]);
  EXPECT_EQ(OzLexemType::RECORD_DEF_FEATURE, node2.type);
}

TEST_F(ExpressionParserTest, ParseBinaryOp_3) {
  Init("X");
  parser_.ParseBinaryOperatorLTR(generic_, OzLexemType::RECORD_DEF_FEATURE);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeVar& node = dynamic_cast<OzNodeVar&>(*generic_->nodes[0]);
  EXPECT_EQ("X", node.var_name);
}

TEST_F(ExpressionParserTest, ParseNaryOp_1) {
  Init("A");
  parser_.ParseNaryOperator(generic_, OzLexemType::NUMERIC_ADD);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
}

TEST_F(ExpressionParserTest, ParseNaryOp_2) {
  Init("A + B C");
  parser_.ParseNaryOperator(generic_, OzLexemType::NUMERIC_ADD);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  ASSERT_EQ(2UL, generic_->nodes.size());
  const OzNodeNaryOp& node =
      dynamic_cast<OzNodeNaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::NUMERIC_ADD, node.type);
  EXPECT_EQ(2UL, node.operands.size());
  const OzNodeVar& node2 =
      dynamic_cast<OzNodeVar&>(*generic_->nodes[1]);
  EXPECT_EQ("C", node2.var_name);
}

TEST_F(ExpressionParserTest, ParseNaryOp_3) {
  Init("A + B + C");
  parser_.ParseNaryOperator(generic_, OzLexemType::NUMERIC_ADD);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeNaryOp& node =
      dynamic_cast<OzNodeNaryOp&>(*generic_->nodes[0]);
  EXPECT_EQ(OzLexemType::NUMERIC_ADD, node.type);
  EXPECT_EQ(3UL, node.operands.size());
}

TEST_F(ExpressionParserTest, ParseNaryOp_4) {
  Init("A");
  parser_.ParseNaryOperator(generic_, OzLexemType::NUMERIC_ADD);
  EXPECT_EQ(OzLexemType::TOP_LEVEL, generic_->type);
  EXPECT_EQ(1UL, generic_->nodes.size());
  const OzNodeVar& node = dynamic_cast<OzNodeVar&>(*generic_->nodes[0]);
  EXPECT_EQ("A", node.var_name);
}

// -----------------------------------------------------------------------------

class OzParserTest : public testing::Test {
 public:
  bool Init(const string& text) {
    text_ = text;
    const bool success = parser_.Parse(text_);
    LOG(INFO) << "AST:\n" << *parser_.root();
    return success;
  }

  string text_;
  OzParser parser_;
  AbstractOzNode* node_;
};

TEST_F(OzParserTest, Var) {
  CHECK(Init("X"));
  const OzNodeGeneric& root =
      dynamic_cast<const OzNodeGeneric&>(*parser_.root());
  const OzNodeVar& var = dynamic_cast<const OzNodeVar&>(*root.nodes[0]);
  EXPECT_EQ("X", var.var_name);
}

TEST_F(OzParserTest, BinaryOp) {
  CHECK(Init("X - Y"));
  const OzNodeGeneric& root =
      dynamic_cast<const OzNodeGeneric&>(*parser_.root());
  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  const OzNodeBinaryOp& bop =
      dynamic_cast<const OzNodeBinaryOp&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::NUMERIC_MINUS, bop.type);
}

TEST_F(OzParserTest, NaryOp) {
  CHECK(Init("X = Y = Z"));
  const OzNodeGeneric& root =
      dynamic_cast<const OzNodeGeneric&>(*parser_.root());
  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  const OzNodeNaryOp& nop =
      dynamic_cast<const OzNodeNaryOp&>(*root.nodes[0]);
  EXPECT_EQ(OzLexemType::UNIFY, nop.type);
}

TEST_F(OzParserTest, RecordEmpty) {
  CHECK(Init("record()"));
  const OzNodeGeneric& root =
      dynamic_cast<const OzNodeGeneric&>(*parser_.root());
  EXPECT_EQ(OzLexemType::TOP_LEVEL, root.type);
  const OzNodeRecord& record =
      dynamic_cast<const OzNodeRecord&>(*root.nodes[0]);
  EXPECT_EQ("record", Str(record.label->tokens.first().value));
}

TEST_F(OzParserTest, RecordSingleton) {
  CHECK(Init("record(a)"));
}

TEST_F(OzParserTest, Record) {
  CHECK(Init("record(a 2 k:v)"));
}

TEST_F(OzParserTest, OpenRecord) {
  CHECK(Init("record(a 2 k:v ...)"));
}

TEST_F(OzParserTest, List_1) {
  CHECK(Init("[a]"));
}

TEST_F(OzParserTest, List_2) {
  CHECK(Init("[a 2]"));
}

TEST_F(OzParserTest, List_3) {
  CHECK(Init("1 | 2 | nil"));
}

TEST_F(OzParserTest, Functor_1) {
  CHECK(Init("functor F end"));
}

TEST_F(OzParserTest, Fun_1) {
  CHECK(Init("fun {F} nil end"));
}

TEST_F(OzParserTest, Fun_2) {
  CHECK(Init("fun {F X} nil end"));
}

TEST_F(OzParserTest, Proc_1) {
  CHECK(Init("proc {P} skip end"));
}

TEST_F(OzParserTest, Proc_2) {
  CHECK(Init("proc {P X} skip end"));
}

TEST_F(OzParserTest, CondIf_1) {
  CHECK(Init("if X then Y end"));
}

TEST_F(OzParserTest, CondIf_2) {
  CHECK(Init("if X then Y else Z end"));
}

TEST_F(OzParserTest, CondIf_3) {
  CHECK(Init("if X then Y elseif Z then T end"));
}

TEST_F(OzParserTest, CondIf_4) {
  CHECK(Init("if X then Y elseif Z then T else U end"));
}

TEST_F(OzParserTest, CondCase_1) {
  CHECK(Init("case X of Y then Z end"));
}

TEST_F(OzParserTest, CondCase_2) {
  CHECK(Init("case X of Y then Z else T end"));
}

TEST_F(OzParserTest, CondCase_3) {
  CHECK(Init("case X of Y then Z elsecase A of B then T end"));
}

TEST_F(OzParserTest, CondCase_4) {
  CHECK(Init("case X of Y andthen Z then T end"));
}

TEST_F(OzParserTest, CondCase_5) {
  CHECK(Init("case X of Y then Z elseof A then B end"));
}

TEST_F(OzParserTest, CondCase_6) {
  CHECK(Init("case X of Y andthen T orelse U then Z elseof A then B end"));
}

TEST_F(OzParserTest, CondIfCase_1) {
  CHECK(Init("if X then Y elsecase A of B then C else D end"));
}

TEST_F(OzParserTest, CondIfCase_2) {
  CHECK(Init("case A of B then C elseif X then Y else D end"));
}

TEST_F(OzParserTest, Thread) {
  CHECK(Init("thread X end"));
}

TEST_F(OzParserTest, Raise) {
  CHECK(Init("raise X end"));
}

// TEST_F(OzParserTest, TryEnd) {
//   if (Init("try X end")) {
//     FAIL();
//   }
// }

TEST_F(OzParserTest, TryCatch) {
  CHECK(Init("try X catch Y then Z end"));
}

TEST_F(OzParserTest, TryFinally) {
  CHECK(Init("try X finally Z end"));
}

}}  // namespace combinators::oz
