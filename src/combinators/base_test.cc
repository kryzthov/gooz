// Tests for parser combinators.

#include "combinators/base.h"

#include <gtest/gtest.h>

namespace combinators {

TEST(BaseCombinators, CharParser) {
  auto parser = CharParser('[');
  {
    auto result = parser(StringPiece("["));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ("[", result.payload);
  }
  {
    auto result = parser(StringPiece("[xxx"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("x[xxx"));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
  {
    auto result = parser(StringPiece(""));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
}

TEST(BaseCombinators, StringParser) {
  auto parser = StringParser("word");
  {
    auto result = parser(StringPiece("word"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("wordxxx"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("xxword"));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
  {
    auto result = parser(StringPiece(""));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
}

TEST(BaseCombinators, RegexParser) {
  auto parser = RegexParser("a*b");
  {
    auto result = parser(StringPiece(""));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
  {
    auto result = parser(StringPiece("b"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("bx"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("ab"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("aaaab"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("aaabx"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
  }
  {
    auto result = parser(StringPiece("eaaab"));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
}

TEST(BaseCombinators, RepeatParser) {
  CharParser char_parser('a');
  auto parser = Repeat<CharParser, CharStream>(char_parser);
  {
    auto result = parser(StringPiece(""));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ(0UL, result.payload.size());
  }
  {
    auto result = parser(StringPiece("b"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ(0UL, result.payload.size());
  }
  {
    auto result = parser(StringPiece("a"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ(1UL, result.payload.size());
  }
  {
    auto result = parser(StringPiece("aaaa"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ(4UL, result.payload.size());
  }
  {
    auto result = parser(StringPiece("aaaab"));
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ(4UL, result.payload.size());
  }
}

TEST(BaseCombinators, AndParser) {
  CharParser a_parser('a'), b_parser('b'), c_parser('c');
  auto parser = And<std::tuple<StringPiece, StringPiece, StringPiece>,
                    CharStream,
                    CharParser, CharParser, CharParser>
      (a_parser, b_parser, c_parser);

  auto parser2 = NewAnd<std::tuple<StringPiece, StringPiece, StringPiece>,
                        CharStream>(a_parser, b_parser, c_parser);

  {
    auto result = parser(StringPiece(""));
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
  {
    const string text = "abc";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ("a", std::get<0>(result.payload));
    EXPECT_EQ("b", std::get<1>(result.payload));
    EXPECT_EQ("c", std::get<2>(result.payload));
  }
  {
    const string text = "abcx";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ("a", std::get<0>(result.payload));
    EXPECT_EQ("b", std::get<1>(result.payload));
    EXPECT_EQ("c", std::get<2>(result.payload));
  }
  {
    const string text = "abx";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
    EXPECT_EQ("a", std::get<0>(result.payload));
    EXPECT_EQ("b", std::get<1>(result.payload));
  }
}

TEST(BaseCombinators, OrParser) {
  CharParser a_parser('a'), b_parser('b'), c_parser('c');
  auto parser = Or<boost::variant<StringPiece>,
                   CharStream,
                   CharParser, CharParser, CharParser>
      (a_parser, b_parser, c_parser);

  auto parser2 = NewOr<boost::variant<StringPiece>, CharStream>
      (a_parser, b_parser, c_parser);

  {
    const string text = "";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
  {
    const string text = "abc";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ("a", boost::get<StringPiece>(result.payload));
  }
  {
    const string text = "bcx";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::OK, result.status);
    EXPECT_EQ("b", boost::get<StringPiece>(result.payload));
  }
  {
    const string text = "xab";
    auto result = parser(text);
    EXPECT_EQ(ParsingStatus::FAILED, result.status);
  }
}


}  // namespace combinators
