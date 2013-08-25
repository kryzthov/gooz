#include "combinators/oznode_format_visitor.h"

namespace combinators { namespace oz {

string Format(OzLexemType type) {
  switch (type) {
    case OzLexemType::AND_THEN: return "andthen";
    case OzLexemType::AT: return "@";
      // case OzLexemType::ATOM:
      // case OzLexemType::ATOM_ESCAPED:
      // case OzLexemType::ATTR: "ATTR";
    case OzLexemType::ATTR_ASSIGN: return "<-";
      // case OzLexemType::BEGIN:
      // case OzLexemType::BEGIN_LPAREN:
      // case OzLexemType::BEGIN_RECORD_FEATURES:
      // case OzLexemType::CALL_BEGIN:
      // case OzLexemType::CALL_END:
      // case OzLexemType::CASE:
      // case OzLexemType::CATCH:
    case OzLexemType::CELL_ACCESS: return "@";
    case OzLexemType::CELL_ASSIGN: return "<-";
      // case OzLexemType::CLASS:
      // case OzLexemType::COMMENT:
      // case OzLexemType::COMMENT_EOL:
      // case OzLexemType::DEFINE:
    case OzLexemType::DIFFERENT: return "≠";
      // case OzLexemType::DO:
      // case OzLexemType::ELSE:
      // case OzLexemType::ELSECASE:
      // case OzLexemType::ELSEIF:
      // case OzLexemType::ELSEOF:
      // case OzLexemType::END:
      // case OzLexemType::END_RPAREN:
    case OzLexemType::EQUAL: return "=";
      // case OzLexemType::EXPORT:
    case OzLexemType::EXPR_VAL: return "EXPR_VAL";
      // case OzLexemType::FEAT:
      // case OzLexemType::FINALLY:
      // case OzLexemType::FOR:
      // case OzLexemType::FROM:
      // case OzLexemType::FUN:
      // case OzLexemType::FUNCTOR:
    case OzLexemType::GREATER_OR_EQUAL: return "≥";
    case OzLexemType::GREATER_THAN: return ">";
      // case OzLexemType::IF:
      // case OzLexemType::IMPORT:
      // case OzLexemType::IN:
      // case OzLexemType::INTEGER:
      // case OzLexemType::INTEGER_B16:
      // case OzLexemType::INTEGER_B2:
      // case OzLexemType::INTEGER_B8:
    case OzLexemType::INVALID: LOG(FATAL) << "Invalid lexem";
    case OzLexemType::LESS_OR_EQUAL: return "≤";
    case OzLexemType::LESS_THAN: return "<";
      // case OzLexemType::LIST_BEGIN:
    case OzLexemType::LIST_CONS: return "|";
      // case OzLexemType::LIST_END:
      // case OzLexemType::LOCAL:
      // case OzLexemType::LOCK:
    case OzLexemType::LOOP_INT_RANGE: return "LOOP_INT_RANGE";
      // case OzLexemType::METH:
      // case OzLexemType::NODE_BINARY_OP:
      // case OzLexemType::NODE_CALL:
      // case OzLexemType::NODE_FUNCTOR:
      // case OzLexemType::NODE_LIST:
      // case OzLexemType::NODE_NARY_OP:
      // case OzLexemType::NODE_RECORD:
      // case OzLexemType::NODE_SEQUENCE:
      // case OzLexemType::NODE_UNARY_OP:
    case OzLexemType::NUMERIC_ADD: return "+";
    case OzLexemType::NUMERIC_DIV: return "/";
    case OzLexemType::NUMERIC_MINUS: return "−";
    case OzLexemType::NUMERIC_MUL: return "×";
    case OzLexemType::NUMERIC_NEG: return "-";
      // case OzLexemType::OF:
    case OzLexemType::OR_CHOICE: return "OR_CHOICE";
    case OzLexemType::OR_ELSE: return "orelse";
      // case OzLexemType::PREPARE:
      // case OzLexemType::PROC:
      // case OzLexemType::PROP:
      // case OzLexemType::RAISE:
    case OzLexemType::READ_ONLY: return "!!";
      // case OzLexemType::REAL:
    case OzLexemType::RECORD_ACCESS: return ".";
      // case OzLexemType::RECORD_CONS:
      // case OzLexemType::RECORD_DEF_FEATURE:
      // case OzLexemType::RECORD_EXTEND:
      // case OzLexemType::RECORD_OPEN:
      // case OzLexemType::REQUIRE:
      // case OzLexemType::SKIP:
      // case OzLexemType::STRING:
      // case OzLexemType::THEN:
      // case OzLexemType::THREAD:
      // case OzLexemType::TOP_LEVEL:
      // case OzLexemType::TRY:
    case OzLexemType::TUPLE_CONS: return "#";
    case OzLexemType::UNIFY: return "=";
      // case OzLexemType::VARIABLE:
      // case OzLexemType::VARIABLE_ESCAPED:
    case OzLexemType::VAR_ANON: return "_";
    case OzLexemType::VAR_NODEF: return "!";
    default: return "<invalid lexem>";
  }
}

}}  // namespace combinators::oz
