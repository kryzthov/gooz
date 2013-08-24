#ifndef COMBINATORS_OZNODE_CHECK_VISITOR_H_
#define COMBINATORS_OZNODE_CHECK_VISITOR_H_

#include "combinators/oznode_default_visitor.h"

namespace combinators { namespace oz {

// Walk through the AST and report errors.
class CheckErrorVisitor : public DefaultVisitor {
 public:
  CheckErrorVisitor()
      : valid_(true) {
  }

  virtual void Visit(OzNodeError* err) {
    valid_ = false;
    const OzLexem& scope_begin = err->node->tokens.first();
    // const OzLexem& scope_end = err->node->tokens.last();
    LOG(ERROR) << "Parse error: "
               << "l" << scope_begin.begin.line() << ","
               << "c" << scope_begin.begin.line_pos()
               << " : " << err->error;
  }

  bool valid() const { return valid_; }

 private:
  bool valid_;

  DISALLOW_COPY_AND_ASSIGN(CheckErrorVisitor);
};

// -----------------------------------------------------------------------------

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_CHECK_VISITOR_H_
