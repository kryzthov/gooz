#include "combinators/ozlexer.h"

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------
RegexParser eol_comment("%[^\\n]*\\n");

// -----------------------------------------------------------------------------
// Parser for atoms, variables, strings
RegexParser simple_atom("[a-z][A-Za-z0-9_]*");
RegexParser escaped_atom("'([^\\\\']|\\\\.)*'");
RegexParser simple_variable("[A-Z][A-Za-z0-9_]*");
RegexParser escaped_variable("`([^\\\\`]|\\\\.)*`");
RegexParser string_parser("\"([^\\\\\"]|\\\\.)*\"");

// -----------------------------------------------------------------------------
// Parser for numbers
RegexParser decimal_integer("([1-9][0-9]*)|0");
RegexParser signed_decimal_integer("[+-][0-9]+");
RegexParser hexadecimal_integer("0[xX][0-9a-fA-F]+");
RegexParser c_octal_integer("0[0-7]*");
RegexParser octal_integer("0[oO][0-7]+");
RegexParser binary_integer("0[bB][01]+");

RegexParser signed_decimal_real("[+-]?[0-9]+\\.[0-9]+([eE][+-]?[0-9]+)?");
RegexParser decimal_real("[0-9]+\\.[0-9]+([eE][+-]?[0-9]+)?");
// TODO:
//  • explicit precision (single/double/custom)
//  • exact binary representation (eg. using hexa)

// -----------------------------------------------------------------------------

string OzLexemTypeStr(OzLexemType type) {
  // Introspection would be too easy…
  switch (type) {
    case OzLexemType::AND_THEN: return "AND_THEN";
    case OzLexemType::AT: return "AT";
    case OzLexemType::ATOM: return "ATOM";
    case OzLexemType::ATOM_ESCAPED: return "ATOM_ESCAPED";
    case OzLexemType::ATTR: return "ATTR";
    case OzLexemType::ATTR_ASSIGN: return "ATTR_ASSIGN";
    case OzLexemType::BEGIN: return "BEGIN";
    case OzLexemType::BEGIN_LPAREN: return "BEGIN_LPAREN";
    case OzLexemType::BEGIN_RECORD_FEATURES: return "BEGIN_RECORD_FEATURES";
    case OzLexemType::CALL_BEGIN: return "CALL_BEGIN";
    case OzLexemType::CALL_END: return "CALL_END";
    case OzLexemType::CASE: return "CASE";
    case OzLexemType::CATCH: return "CATCH";
    case OzLexemType::CELL_ACCESS: return "CELL_ACCESS";
    case OzLexemType::CELL_ASSIGN: return "CELL_ASSIGN";
    case OzLexemType::CLASS: return "CLASS";
    case OzLexemType::COMMENT: return "COMMENT";
    case OzLexemType::COMMENT_EOL: return "COMMENT_EOL";
    case OzLexemType::DEFINE: return "DEFINE";
    case OzLexemType::DIFFERENT: return "DIFFERENT";
    case OzLexemType::DO: return "DO";
    case OzLexemType::ELSE: return "ELSE";
    case OzLexemType::ELSECASE: return "ELSECASE";
    case OzLexemType::ELSEIF: return "ELSEIF";
    case OzLexemType::ELSEOF: return "ELSEOF";
    case OzLexemType::END: return "END";
    case OzLexemType::END_RPAREN: return "END_RPAREN";
    case OzLexemType::EQUAL: return "EQUAL";
    case OzLexemType::EXPORT: return "EXPORT";
    case OzLexemType::EXPR_VAL: return "EXPR_VAL";
    case OzLexemType::FEAT: return "FEAT";
    case OzLexemType::FINALLY: return "FINALLY";
    case OzLexemType::FOR: return "FOR";
    case OzLexemType::FROM: return "FROM";
    case OzLexemType::FUN: return "FUN";
    case OzLexemType::FUNCTOR: return "FUNCTOR";
    case OzLexemType::GREATER_OR_EQUAL: return "GREATER_OR_EQUAL";
    case OzLexemType::GREATER_THAN: return "GREATER_THAN";
    case OzLexemType::IF: return "IF";
    case OzLexemType::IMPORT: return "IMPORT";
    case OzLexemType::IN: return "IN";
    case OzLexemType::INTEGER: return "INTEGER";
    case OzLexemType::INTEGER_B16: return "INTEGER_B16";
    case OzLexemType::INTEGER_B2: return "INTEGER_B2";
    case OzLexemType::INTEGER_B8: return "INTEGER_B8";
    case OzLexemType::INVALID: return "INVALID";
    case OzLexemType::LESS_OR_EQUAL: return "LESS_OR_EQUAL";
    case OzLexemType::LESS_THAN: return "LESS_THAN";
    case OzLexemType::LIST_BEGIN: return "LIST_BEGIN";
    case OzLexemType::LIST_CONS: return "LIST_CONS";
    case OzLexemType::LIST_END: return "LIST_END";
    case OzLexemType::LOCAL: return "LOCAL";
    case OzLexemType::LOCK: return "LOCK";
    case OzLexemType::LOOP_INT_RANGE: return "LOOP_INT_RANGE";
    case OzLexemType::METH: return "METH";
    case OzLexemType::NODE_BINARY_OP: return "NODE_BINARY_OP";
    case OzLexemType::NODE_FUNCTOR: return "NODE_FUNCTOR";
    case OzLexemType::NODE_NARY_OP: return "NODE_NARY_OP";
    case OzLexemType::NODE_RECORD: return "NODE_RECORD";
    case OzLexemType::NODE_UNARY_OP: return "NODE_UNARY_OP";
    case OzLexemType::NUMERIC_ADD: return "NUMERIC_ADD";
    case OzLexemType::NUMERIC_DIV: return "NUMERIC_DIV";
    case OzLexemType::NUMERIC_MINUS: return "NUMERIC_MINUS";
    case OzLexemType::NUMERIC_MUL: return "NUMERIC_MUL";
    case OzLexemType::NUMERIC_NEG: return "NUMERIC_NEG";
    case OzLexemType::OF: return "OF";
    case OzLexemType::OR_CHOICE: return "OR_CHOICE";
    case OzLexemType::OR_ELSE: return "OR_ELSE";
    case OzLexemType::PREPARE: return "PREPARE";
    case OzLexemType::PROC: return "PROC";
    case OzLexemType::PROP: return "PROP";
    case OzLexemType::RAISE: return "RAISE";
    case OzLexemType::READ_ONLY: return "READ_ONLY";
    case OzLexemType::REAL: return "REAL";
    case OzLexemType::RECORD_ACCESS: return "RECORD_ACCESS";
    case OzLexemType::RECORD_CONS: return "RECORD_CONS";
    case OzLexemType::RECORD_DEF_FEATURE: return "RECORD_DEF_FEATURE";
    case OzLexemType::RECORD_EXTEND: return "RECORD_EXTEND";
    case OzLexemType::RECORD_OPEN: return "RECORD_OPEN";
    case OzLexemType::REQUIRE: return "REQUIRE";
    case OzLexemType::SKIP: return "SKIP";
    case OzLexemType::STRING: return "STRING";
    case OzLexemType::THEN: return "THEN";
    case OzLexemType::THREAD: return "THREAD";
    case OzLexemType::TOP_LEVEL: return "TOP_LEVEL";
    case OzLexemType::TRY: return "TRY";
    case OzLexemType::TUPLE_CONS: return "TUPLE_CONS";
    case OzLexemType::UNIFY: return "UNIFY";
    case OzLexemType::VARIABLE: return "VARIABLE";
    case OzLexemType::VARIABLE_ESCAPED: return "VARIABLE_ESCAPED";
    case OzLexemType::VAR_ANON: return "VAR_ANON";
    case OzLexemType::VAR_NODEF: return "VAR_NODEF";
  }
  return "<Invalid OzLexemType>";
}

// -----------------------------------------------------------------------------

// Keywords
const OzSymbol kOzKeywords[] = {
  // "declare",

  OzSymbol("functor", OzLexemType::FUNCTOR),
  OzSymbol("export", OzLexemType::EXPORT),
  OzSymbol("require", OzLexemType::REQUIRE),
  OzSymbol("prepare", OzLexemType::PREPARE),
  OzSymbol("import", OzLexemType::IMPORT),
  OzSymbol("define", OzLexemType::DEFINE),
  OzSymbol("at", OzLexemType::AT),

  OzSymbol("fun", OzLexemType::FUN),
  OzSymbol("proc", OzLexemType::PROC),

  OzSymbol("class", OzLexemType::CLASS),
  OzSymbol("from", OzLexemType::FROM),
  OzSymbol("prop", OzLexemType::PROP),
  OzSymbol("feat", OzLexemType::FEAT),
  OzSymbol("attr", OzLexemType::ATTR),
  OzSymbol("meth", OzLexemType::METH),

  OzSymbol("if", OzLexemType::IF),
  OzSymbol("then", OzLexemType::THEN),
  OzSymbol("elseif", OzLexemType::ELSEIF),
  OzSymbol("case", OzLexemType::CASE),
  OzSymbol("elsecase", OzLexemType::ELSECASE),
  OzSymbol("of", OzLexemType::OF),
  OzSymbol("elseof", OzLexemType::ELSEOF),
  OzSymbol("else", OzLexemType::ELSE),

  OzSymbol("andthen", OzLexemType::AND_THEN),
  OzSymbol("orelse", OzLexemType::OR_ELSE),
  OzSymbol("or", OzLexemType::OR_CHOICE),

  OzSymbol("for", OzLexemType::FOR),
  OzSymbol("do", OzLexemType::DO),

  OzSymbol("thread", OzLexemType::THREAD),
  OzSymbol("local", OzLexemType::BEGIN, OzLexemType::LOCAL),
  OzSymbol("lock", OzLexemType::LOCK),
  OzSymbol("in", OzLexemType::IN),

  OzSymbol("try", OzLexemType::TRY),
  OzSymbol("catch", OzLexemType::CATCH),
  OzSymbol("finally", OzLexemType::FINALLY),
  OzSymbol("raise", OzLexemType::RAISE),

  OzSymbol("end", OzLexemType::END),

  OzSymbol("skip", OzLexemType::SKIP),

  OzSymbol("", OzLexemType::INVALID),
};

const OzSymbol kOzSymbols[] = {
  OzSymbol("...", OzLexemType::RECORD_OPEN),
  OzSymbol("…" /*\u2026*/, OzLexemType::RECORD_OPEN),

  OzSymbol("..", OzLexemType::LOOP_INT_RANGE),

  // OzSymbol("!="), OzSymbol("<>"),
  OzSymbol("==", OzLexemType::EQUAL),
  OzSymbol("⩵" /*\u2a75*/, OzLexemType::EQUAL),
  OzSymbol("⩶" /*\u2a76*/, OzLexemType::EQUAL),
  OzSymbol("≡" /*\u2261*/, OzLexemType::EQUAL),
  OzSymbol("≣" /*\u2263*/, OzLexemType::EQUAL),

  OzSymbol("\\=", OzLexemType::DIFFERENT),
  OzSymbol("≠" /*\u2260*/, OzLexemType::DIFFERENT),

  OzSymbol(">=", OzLexemType::GREATER_OR_EQUAL),
  OzSymbol("≥" /*\u2265*/, OzLexemType::GREATER_OR_EQUAL),

  OzSymbol("=<", OzLexemType::LESS_OR_EQUAL),
  OzSymbol("≤" /*\u2264*/, OzLexemType::LESS_OR_EQUAL),

  OzSymbol("<-", OzLexemType::ATTR_ASSIGN),

  OzSymbol(":=", OzLexemType::CELL_ASSIGN),
  OzSymbol("≔" /*\u2254*/, OzLexemType::CELL_ASSIGN),

  // OzSymbol("<=", OzLexemType::???),
  // OzSymbol("=>", OzLexemType::DEFAULT_VALUE),

  OzSymbol("!!", OzLexemType::READ_ONLY),
  OzSymbol("‼" /*\u203c*/, OzLexemType::READ_ONLY),

  OzSymbol("[]", OzLexemType::ELSEOF),

  OzSymbol("<", OzLexemType::LESS_THAN),
  OzSymbol(">", OzLexemType::GREATER_THAN),
  OzSymbol("=", OzLexemType::UNIFY),
  OzSymbol("#", OzLexemType::TUPLE_CONS),
  OzSymbol("|", OzLexemType::LIST_CONS),
  OzSymbol("~", OzLexemType::NUMERIC_NEG),
  OzSymbol("+", OzLexemType::NUMERIC_ADD),
  OzSymbol("-", OzLexemType::NUMERIC_MINUS),
  OzSymbol("−" /*\u2212*/, OzLexemType::NUMERIC_MINUS),

  OzSymbol("*", OzLexemType::NUMERIC_MUL),
  OzSymbol("×" /*\u00d7*/, OzLexemType::NUMERIC_MUL),

  OzSymbol("/", OzLexemType::NUMERIC_DIV),
  OzSymbol("÷" /*\u00f7*/, OzLexemType::NUMERIC_DIV),

  OzSymbol("_", OzLexemType::VAR_ANON),
  OzSymbol("$", OzLexemType::EXPR_VAL),

  OzSymbol("^", OzLexemType::RECORD_EXTEND),
  OzSymbol(".", OzLexemType::RECORD_ACCESS),

  OzSymbol("@", OzLexemType::CELL_ACCESS),

  OzSymbol("!", OzLexemType::VAR_NODEF),

  // OzSymbol("&"),
  OzSymbol(":", OzLexemType::RECORD_DEF_FEATURE),

  OzSymbol("{", OzLexemType::CALL_BEGIN),
  OzSymbol("}", OzLexemType::CALL_END),

  OzSymbol("[", OzLexemType::LIST_BEGIN),
  OzSymbol("]", OzLexemType::LIST_END),

  OzSymbol("(", OzLexemType::BEGIN, OzLexemType::BEGIN_LPAREN),
  OzSymbol(")", OzLexemType::END, OzLexemType::END_RPAREN),

  // OzSymbol("«", OzLexemType::), OzSymbol("»", OzLexemType::),

  OzSymbol("", OzLexemType::INVALID),
};

// -----------------------------------------------------------------------------

OneOzLexemParser::OneOzLexemParser(bool parse_keywords)
    : keywords_parser_(kOzKeywords),
      symbols_parser_(kOzSymbols),
      parse_keywords_(parse_keywords) {
}

// -----------------------------------------------------------------------------

OzLexer::OzLexer() {
  Setup();
}

OzLexer::OzLexer(const Options& options)
    : options_(options) {
  Setup();
}

OzLexer::~OzLexer() {
}

void OzLexer::Setup() {
  parser_.reset(new OneOzLexemParser(options_.parse_keywords));
}

ParsingResult<CharStream> OzLexer::Parse(const CharStream& input) {
  ParsingResult<CharStream> result(input);
  CharStream current = SkipBlank(input);

  while (!current.empty()) {
    OzLexResult res = parser_->Parse(current);
    switch (res.status) {
      case ParsingStatus::OK:
        if (options_.skip_comments
            && (res.payload.type == OzLexemType::COMMENT))
          break;;

        if ((res.payload.type == OzLexemType::ATOM)
            || (res.payload.type == OzLexemType::VARIABLE)) {
          // Look-ahead for a record constructor
          if (res.next.starts_with("(")) {
            lexems_.push_back(OzLexem()
                              .SetType(OzLexemType::RECORD_CONS)
                              .SetExactType(OzLexemType::RECORD_CONS)
                              .SetBegin(current)
                              .SetEnd(current));
            lexems_.push_back(res.payload);
            lexems_.push_back(OzLexem()
                              .SetType(OzLexemType::BEGIN_RECORD_FEATURES)
                              .SetExactType(OzLexemType::BEGIN_RECORD_FEATURES)
                              .SetBegin(res.next)
                              .SetEnd(res.next.Next()));
            res.next.Walk();
            break;
          }
        }

        lexems_.push_back(res.payload);
        break;

      case ParsingStatus::FAILED:
        return result.SetNext(current).Fail();
    }
    current = SkipBlank(res.next);
  }

  return result.Succeed(current);
}

}}  // namespace combinators::oz
