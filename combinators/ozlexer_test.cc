#include "combinators/ozlexer.h"

#include <gtest/gtest.h>

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

class OneOzLexemParserTest : public testing::Test {
 public:
  OneOzLexemParser lexer_;
};

TEST_F(OneOzLexemParserTest, Integer_1) {
  const string text = "314  junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::INTEGER, res.payload.type);
  EXPECT_EQ(314, boost::get<mpz_class>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Integer_2) {
  const string text = "0x314f  junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::INTEGER, res.payload.type);
  EXPECT_EQ(0x314f, boost::get<mpz_class>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Integer_3) {
  const string text = "0b11101  junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::INTEGER, res.payload.type);
  EXPECT_EQ(0x1d, boost::get<mpz_class>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Integer_4) {
  const string text = "0o774  junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::INTEGER, res.payload.type);
  EXPECT_EQ(0x1fc, boost::get<mpz_class>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Real_1) {
  const string text = "3.14159  junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::REAL, res.payload.type);
  EXPECT_EQ("0.31415",
            boost::get<real::Real>(res.payload.value).String().substr(0, 7));
}

TEST_F(OneOzLexemParserTest, Atom_1) {
  const string text = "atom junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::ATOM, res.payload.type);
  EXPECT_EQ("atom", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, EscapedAtom_1) {
  const string text = "'Atom' junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::ATOM, res.payload.type);
  EXPECT_EQ("Atom", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Var_1) {
  const string text = "Var junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::VARIABLE, res.payload.type);
  EXPECT_EQ("Var", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, EscapedVar_1) {
  const string text = "`escaped\\`var` junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::VARIABLE, res.payload.type);
  EXPECT_EQ("escaped`var", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, String_1) {
  const string text = "\"string\" junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::STRING, res.payload.type);
  EXPECT_EQ("string", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, EOLComment_1) {
  const string text = "% eol comment\n junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::COMMENT, res.payload.type);
  EXPECT_EQ(" eol comment\n", boost::get<string>(res.payload.value));
}

TEST_F(OneOzLexemParserTest, Keyword_1) {
  const string text = "functor junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::FUNCTOR, res.payload.type);
}

TEST_F(OneOzLexemParserTest, Symbol_1) {
  const string text = "(junk";
  OzLexResult res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::BEGIN, res.payload.type);
  EXPECT_EQ(OzLexemType::BEGIN_LPAREN, res.payload.exact_type);
}

// -----------------------------------------------------------------------------

TEST(OneOzLexemParserTest_NoKeyword, Keyword_1) {
  OneOzLexemParser lexer(false);
  const string text = "functor end";
  OzLexResult res = lexer.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(OzLexemType::ATOM, res.payload.type);
}

// -----------------------------------------------------------------------------

class OzLexerTest : public testing::Test {
 public:
  OzLexer lexer_;
};

TEST_F(OzLexerTest, Simple_1) {
  const string text =
      "314\n"
      "atom\n"
      "Var = 3\n"
      "local record(1) end\n";
  ParsingResult<CharStream> res = lexer_.Parse(text);
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(12UL, lexer_.lexems().size());

  for (OzLexem lexem : lexer_.lexems())
    LOG(INFO) << lexem;
}

TEST_F(OzLexerTest, NoComment_1) {
  const string text =
      "% comment\n"
      "atom";
  ParsingResult<CharStream> res = lexer_.Parse(text);
  for (OzLexem lexem : lexer_.lexems())
    LOG(INFO) << lexem;
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(1UL, lexer_.lexems().size());
}

TEST_F(OzLexerTest, WithComment_1) {
  OzLexer lexer(OzLexer::Options().set_skip_comments(false));
  const string text =
      "% comment\n"
      "atom";
  ParsingResult<CharStream> res = lexer.Parse(text);
  for (OzLexem lexem : lexer.lexems())
    LOG(INFO) << lexem;
  EXPECT_EQ(ParsingStatus::OK, res.status);
  EXPECT_EQ(2UL, lexer.lexems().size());
}

}}  // namespace combinators::oz
