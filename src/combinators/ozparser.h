#ifndef COMBINATORS_OZPARSER_H_
#define COMBINATORS_OZPARSER_H_

#include <memory>
#include <string>

#include "base/stl-util.h"
#include "combinators/base.h"
#include "combinators/ozlexer.h"
#include "combinators/oznode.h"
#include "combinators/oznode_visitor.h"
#include "combinators/oznode_dump_visitor.h"
#include "combinators/stream.h"

using std::shared_ptr;
using std::string;
using std::unique_ptr;

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

inline OzLexemStream OzLexemSlice(const AbstractOzNode& first,
                                  const AbstractOzNode& last) {
  return OzLexemStream(first.tokens, last.tokens.SliceAfter());
}

// -----------------------------------------------------------------------------

class TopLevelScopeParser;
class MidLevelScopeParser;
class ExpressionParser;
class OzParser;

// Parser for construction scopes
class TopLevelScopeParser {
 public:
  TopLevelScopeParser();

  // For testing purpose only
  TopLevelScopeParser(MidLevelScopeParser* midlevel_parser);

  // Parses a stream of lexems into a tree of generic nodes.
  // Invokes the mid-level parser each time a scope has been identified.
  //
  // @param lexems Stream of lexems to parse into an AST.
  // @param root AST root to fill in.
  // @returns status in the form of a lexem stream parsing result.
  ParsingResult<OzLexemStream>
  Parse(const OzLexemStream& lexems, shared_ptr<AbstractOzNode>& root);

 private:
  ParsingResult<OzLexemStream>
  ParseInternal(const OzLexemStream& lexems, OzNodeGeneric* const root);

  // Each time a scope is identified, it is processed by this mid-level
  // scope parser.
  // nullptr means no further parsing, mainly for testing.
  unique_ptr<MidLevelScopeParser> midlevel_parser_;
};

// Parser for scoped constructions
class MidLevelScopeParser {
 public:
  MidLevelScopeParser();
  MidLevelScopeParser(ExpressionParser* expr_parser);

  // Parses an identified Oz scope.
  //
  // @param root OzNodeGeneric to be parsed.
  // @returns parsed node (possibly an OzNodeError).
  shared_ptr<AbstractOzNode> Parse(shared_ptr<OzNodeGeneric>& root);

 private:
  shared_ptr<AbstractOzNode> ParseLocal(shared_ptr<OzNodeGeneric>& root);
  shared_ptr<AbstractOzNode> ParseIfBranch(shared_ptr<OzNodeGeneric>& root,
                                           bool pattern);
  shared_ptr<AbstractOzNode> ParseCaseBranch(shared_ptr<OzNodeGeneric>& root);

  shared_ptr<AbstractOzNode> ParseTry(shared_ptr<OzNodeGeneric>& root);

  unique_ptr<ExpressionParser> expr_parser_;
};

// Parser for expression structures
class ExpressionParser {
 public:
  void Parse(OzNodeGeneric* const root);

  // Parses record constructors.
  void ParseRecordCons(OzNodeGeneric* const branch);

  // Parses a unary operator.
  void ParseUnaryOperator(OzNodeGeneric* const branch, OzLexemType op_type);

  // Parses a binary operator.
  void ParseBinaryOperatorLTR(OzNodeGeneric* const branch, OzLexemType op_type);
  void ParseBinaryOperatorRTL(OzNodeGeneric* const branch, OzLexemType op_type);

  // Parses a n-ary operator.
  void ParseNaryOperator(OzNodeGeneric* const branch, OzLexemType op_type);
};


// Full Oz parser
class OzParser {
 public:
  OzParser();

  bool Parse(const string& text);

  // Parses a stream of lexems.
  ParsingResult<OzLexemStream>
  Parse(const OzLexemStream& input);

  shared_ptr<AbstractOzNode>& root() { return root_; }

 private:
  OzLexer lexer_;
  TopLevelScopeParser parser_;
  shared_ptr<AbstractOzNode> root_;
};

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZPARSER_H_
