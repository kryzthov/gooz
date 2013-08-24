#ifndef COMBINATORS_OZLEXER_H_
#define COMBINATORS_OZLEXER_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <gmpxx.h>

#include <boost/variant.hpp>

#include "base/real.h"
#include "base/stl-util.h"
#include "base/stringer.h"
#include "base/string_piece.h"
#include "combinators/base.h"
#include "combinators/stream.h"

using base::StringPiece;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;

namespace combinators { namespace oz {

namespace real = base::real;

enum class OzLexemType {
  INVALID = 0,

    // Normalized lexem types

    COMMENT,

    INTEGER,
    REAL,
    ATOM,
    STRING,
    VARIABLE,

    FUNCTOR, EXPORT, REQUIRE, PREPARE, IMPORT, DEFINE, AT,
    FUN, PROC,
    CLASS, FROM, PROP, FEAT, ATTR, METH,
    IF, THEN, ELSEIF, CASE, ELSECASE, OF, ELSEOF, ELSE,
    FOR, DO,
    THREAD,
    LOCK,
    IN,

    AND_THEN, OR_ELSE,
    OR_CHOICE,

    TRY, CATCH, FINALLY, RAISE,

    SKIP,

    BEGIN, END,  // local...end or (...)
    CALL_BEGIN, CALL_END,
    BEGIN_RECORD_FEATURES,

    RECORD_CONS,
    RECORD_ACCESS, RECORD_EXTEND,
    RECORD_DEF_FEATURE, RECORD_OPEN,

    LIST_CONS, LIST_BEGIN, LIST_END,
    TUPLE_CONS,
    UNIFY,

    EQUAL, DIFFERENT,
    GREATER_OR_EQUAL, LESS_OR_EQUAL,
    GREATER_THAN, LESS_THAN,

    NUMERIC_NEG,  // unary minus
    NUMERIC_ADD,
    NUMERIC_MINUS,  // binary minus
    NUMERIC_MUL,
    NUMERIC_DIV,

    VAR_ANON, VAR_NODEF,
    EXPR_VAL,

    CELL_ACCESS, CELL_ASSIGN,
    ATTR_ASSIGN,

    READ_ONLY,
    LOOP_INT_RANGE,

    // Exact lexem types

    INTEGER_B16,
    INTEGER_B8,
    INTEGER_B2,

    ATOM_ESCAPED,
    VARIABLE_ESCAPED,

    LOCAL,
    BEGIN_LPAREN, END_RPAREN,

    COMMENT_EOL,

    // Higher level AST node types
    TOP_LEVEL,
    NODE_UNARY_OP,
    NODE_BINARY_OP,
    NODE_NARY_OP,

    NODE_RECORD,

    NODE_LOCAL,

    NODE_FUNCTOR,
    NODE_THREAD,
    NODE_CLASS,
    NODE_PROC,

    NODE_TRY,
    NODE_RAISE,
    NODE_LOOP,
    NODE_LOCK,

    NODE_LIST,
    NODE_CALL,
    NODE_SEQUENCE,
};

string OzLexemTypeStr(OzLexemType type);
inline string Str(OzLexemType t) {return OzLexemTypeStr(t);}

inline std::ostream& operator <<(std::ostream& os, OzLexemType type) {
  return os << OzLexemTypeStr(type);
}


struct OzLexemTypeHash {
  size_t operator()(OzLexemType value) const {
    return size_t(value);
  }
};

// -----------------------------------------------------------------------------

// Oz lexical element
struct OzLexem {
  typedef boost::variant<
    Empty,
    string,
    mpz_class,
    real::Real
  > ValueType;

  // Normalized lexem type.
  OzLexemType type;

  // Exact lexem type, carrying the formatting style.
  OzLexemType exact_type;

  // Absolute begin/end position of the token.
  // Exact source text.
  CharStream begin, end;

  // Normalized value of the token.
  ValueType value;

  // TODO: position, relatively to the previous lexem.

  OzLexem()
      : type(OzLexemType::INVALID),
        exact_type(OzLexemType::INVALID) {}

  inline
  OzLexem& SetType(OzLexemType type_) {
    type = type_;
    return *this;
  }

  inline
  OzLexem& SetExactType(OzLexemType type_) {
    exact_type = type_;
    return *this;
  }

  inline
  OzLexem& SetBegin(CharStream begin_) {
    begin = begin_;
    return *this;
  }

  inline
  OzLexem& SetEnd(CharStream end_) {
    end = end_;
    return *this;
  }

  inline
  OzLexem& SetStreamFromResult(ParsingResult<CharStream, StringPiece> res) {
    CHECK_EQ(ParsingStatus::OK, res.status);
    return SetBegin(res.start).SetEnd(res.next);
  }

  inline
  OzLexem& SetValue(ValueType value_) {
    value = value_;
    return *this;
  }
};

// -----------------------------------------------------------------------------

class OzLexemValueVisitor : public boost::static_visitor<string> {
 public:
  string operator()(const mpz_class& integer) const {
    return integer.get_str();
  }

  string operator()(const string& str) const {
    return str;
  }

  string operator()(const base::real::Real& real) const {
    return real.String();
  }

  string operator()(Empty empty) const {
    return "";
  }
};

inline string Str(const OzLexem::ValueType& value) {
  return boost::apply_visitor(OzLexemValueVisitor(), value);
}

// -----------------------------------------------------------------------------

inline string Str(const OzLexem& lexem) {
  Stringer s = Stringer("OzLexem")("type", lexem.type);
  if (lexem.type != lexem.exact_type) s("exact_type", lexem.exact_type);
  const string value = Str(lexem.value);
  if (!value.empty()) s("value", value);
  s("begin", lexem.begin);
  return s;
}

inline std::ostream& operator <<(std::ostream& os, const OzLexem& lexem) {
  return os << Str(lexem);
}

typedef Parser<OzLexem, CharStream> OzLexemParser;
typedef ParsingResult<CharStream, OzLexem> OzLexResult;
typedef TVectorStream<OzLexem> OzLexemStream;

// -----------------------------------------------------------------------------

extern RegexParser eol_comment;
extern RegexParser simple_atom;
extern RegexParser escaped_atom;
extern RegexParser simple_variable;
extern RegexParser escaped_variable;
extern RegexParser string_parser;
extern RegexParser decimal_integer;
extern RegexParser signed_decimal_integer;
extern RegexParser hexadecimal_integer;
extern RegexParser c_octal_integer;
extern RegexParser octal_integer;
extern RegexParser binary_integer;
extern RegexParser signed_decimal_real;
extern RegexParser decimal_real;

// -----------------------------------------------------------------------------

struct OzSymbol {
  string str;
  OzLexemType type;
  OzLexemType exact_type;

  OzSymbol() {}
  OzSymbol(const string& str_, OzLexemType type_,
           OzLexemType exact_type_ = OzLexemType::INVALID)
      : str(str_),
        type(type_),
        exact_type(exact_type_ == OzLexemType::INVALID
                   ? type_ : exact_type_) {}
};

class SymbolTableParser : public OzLexemParser {
 public:
  SymbolTableParser(const OzSymbol table[]) {
    for(int i = 0; table[i].type != OzLexemType::INVALID; ++i) {
      const OzSymbol& symbol = table[i];
      table_.push_back(symbol);
      map_[symbol.str] = symbol;
    }
  }

  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    for (OzSymbol symbol : table_) {
      if(input.starts_with(symbol.str)) {
        const Stream next = input.Next(symbol.str.size());
        return result.Succeed(next,
                              OzLexem()
                              .SetBegin(input)
                              .SetEnd(next)
                              .SetType(symbol.type)
                              .SetExactType(symbol.exact_type));
      }
    }
    return result;
  }

  const UnorderedMap<string, OzSymbol>& map() const { return map_; }

 private:
  vector<OzSymbol> table_;
  UnorderedMap<string, OzSymbol> map_;
};

class AtomParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    {
      RegexParser::ParsingResultType res = simple_atom.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(res.next,
                              OzLexem()
                              .SetType(OzLexemType::ATOM)
                              .SetExactType(OzLexemType::ATOM)
                              .SetStreamFromResult(res)
                              .SetValue(res.payload.as_string()));
      }
    }
    {
      RegexParser::ParsingResultType res = escaped_atom.Parse(input);
      if (res.status == ParsingStatus::OK) {
        const StringPiece payload =
            res.payload.substr(1, res.payload.size() - 2);
        return result.Succeed(res.next,
                              OzLexem()
                              .SetType(OzLexemType::ATOM)
                              .SetExactType(OzLexemType::ATOM_ESCAPED)
                              .SetStreamFromResult(res)
                              .SetValue(base::Unescape(payload, "'")));
      }
    }
    return result;
  }
};

class VariableParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    {
      RegexParser::ParsingResultType res = simple_variable.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(res.next,
                              OzLexem()
                              .SetType(OzLexemType::VARIABLE)
                              .SetExactType(OzLexemType::VARIABLE)
                              .SetStreamFromResult(res)
                              .SetValue(res.payload.as_string()));
      }
    }
    {
      RegexParser::ParsingResultType res = escaped_variable.Parse(input);
      if (res.status == ParsingStatus::OK) {
        const StringPiece payload =
            res.payload.substr(1, res.payload.size() - 2);
        return result.Succeed(res.next,
                              OzLexem()
                              .SetType(OzLexemType::VARIABLE)
                              .SetExactType(OzLexemType::VARIABLE_ESCAPED)
                              .SetValue(base::Unescape(payload, "`")));
      }
    }
    return result;
  }
};

class CommentParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    {
      RegexParser::ParsingResultType res = eol_comment.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(res.next,
                              OzLexem()
                              .SetType(OzLexemType::COMMENT)
                              .SetExactType(OzLexemType::COMMENT_EOL)
                              .SetStreamFromResult(res)
                              .SetValue(res.payload.substr(1).as_string()));
      }
    }
    return result;
  }
};

class StringParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    RegexParser::ParsingResultType res = string_parser.Parse(input);
    if (res.status == ParsingStatus::OK) {
      const StringPiece payload =
          res.payload.substr(1, res.payload.size() - 2);
      return result.Succeed(res.next,
                            OzLexem()
                            .SetType(OzLexemType::STRING)
                            .SetExactType(OzLexemType::STRING)
                            .SetStreamFromResult(res)
                            .SetValue(base::Unescape(payload, "\"")));
    }
    return result;
  }
};

class IntegerParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    {
      RegexParser::ParsingResultType res = hexadecimal_integer.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(
            res.next,
            OzLexem()
            .SetType(OzLexemType::INTEGER)
            .SetExactType(OzLexemType::INTEGER_B16)
            .SetStreamFromResult(res)
            .SetValue(mpz_class(res.payload.substr(2).as_string(), 16)));
      }
    }
    {
      RegexParser::ParsingResultType res = octal_integer.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(
            res.next,
            OzLexem()
            .SetType(OzLexemType::INTEGER)
            .SetExactType(OzLexemType::INTEGER_B8)
            .SetStreamFromResult(res)
            .SetValue(mpz_class(res.payload.substr(2).as_string(), 8)));
      }
    }
    {
      RegexParser::ParsingResultType res = binary_integer.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(
            res.next,
            OzLexem()
            .SetType(OzLexemType::INTEGER)
            .SetExactType(OzLexemType::INTEGER_B2)
            .SetStreamFromResult(res)
            .SetValue(mpz_class(res.payload.substr(2).as_string(), 2)));
      }
    }
    {
      RegexParser::ParsingResultType res = decimal_integer.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(
            res.next,
            OzLexem()
            .SetType(OzLexemType::INTEGER)
            .SetExactType(OzLexemType::INTEGER)
            .SetStreamFromResult(res)
            .SetValue(mpz_class(res.payload.as_string(), 10)));
      }
    }
    return result;
  }
};

class RealParser : public OzLexemParser {
 public:
  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);
    {
      RegexParser::ParsingResultType res = decimal_real.Parse(input);
      if (res.status == ParsingStatus::OK) {
        return result.Succeed(
            res.next,
            OzLexem()
            .SetType(OzLexemType::REAL)
            .SetExactType(OzLexemType::REAL)
            .SetStreamFromResult(res)
            .SetValue(real::Real(res.payload.as_string(), 10)));
      }
    }
    return result;
  }
};

// -----------------------------------------------------------------------------

// Parses exactly one token from the input character stream.
class OneOzLexemParser : public OzLexemParser {
 public:
  OneOzLexemParser(bool parse_keywords = true);

  virtual
  OzLexemParser::ParsingResultType Parse(CharStream input) {
    OzLexemParser::ParsingResultType result(input);

    result = atom_parser_.Parse(input);
    if (result.status == ParsingStatus::OK) {
      // Handle Oz keywords as a special subset of unescaped atoms
      if (parse_keywords_
          && (result.payload.exact_type == OzLexemType::ATOM)) {
        const string& atom = boost::get<string>(result.payload.value);
        if (keywords_parser_.map().contains(atom)) {
          const OzSymbol& symbol = keywords_parser_.map()[atom];
          result.payload
              .SetType(symbol.type)
              .SetExactType(symbol.exact_type == OzLexemType::INVALID
                            ? symbol.type
                            : symbol.exact_type);
        }
      }
      return result;
    }

    result = variable_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    result = string_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    result = real_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    result = integer_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    result = comment_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    result = symbols_parser_.Parse(input);
    if (result.status == ParsingStatus::OK)
      return result;

    return result.Fail();
  }

 private:
  AtomParser atom_parser_;
  VariableParser variable_parser_;
  CommentParser comment_parser_;
  StringParser string_parser_;
  IntegerParser integer_parser_;
  RealParser real_parser_;

  SymbolTableParser keywords_parser_;
  SymbolTableParser symbols_parser_;

  const bool parse_keywords_;
};

// -----------------------------------------------------------------------------

// Parses a full character stream into a sequence of lexems.
class OzLexer {
 public:
  struct Options {
    Options()
        : skip_comments(true),
          parse_keywords(true) {
    }

    Options& set_skip_comments(bool skip_comments_) {
      skip_comments = skip_comments_;
      return *this;
    }

    Options& set_parse_keywords(bool parse_keywords_) {
      parse_keywords = parse_keywords_;
      return *this;
    }

    bool skip_comments;
    bool parse_keywords;
  };

  OzLexer();
  OzLexer(const Options& options);
  ~OzLexer();

  ParsingResult<CharStream> Parse(const CharStream& input);

  const vector<OzLexem>& lexems() const { return lexems_; }

 private:
  void Setup();

  const Options options_;
  unique_ptr<OneOzLexemParser> parser_;
  vector<OzLexem> lexems_;
};

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZLEXER_H_
