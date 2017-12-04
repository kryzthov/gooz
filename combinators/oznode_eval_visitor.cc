#include "combinators/oznode_eval_visitor.h"

#include "combinators/ozparser.h"

namespace combinators { namespace oz {

store::Value ParseEval(const string& code, store::Store* store) {
  OzParser parser;
  CHECK(parser.Parse(code)) << "Error parsing: " << code;
  shared_ptr<OzNodeGeneric> root =
      std::dynamic_pointer_cast<OzNodeGeneric>(parser.root());
  LOG(INFO) << "AST:\n" << *root << std::endl;
  CHECK_EQ(OzLexemType::TOP_LEVEL, root->type);
  EvalVisitor visitor(store);
  store::Value result;
  for (shared_ptr<AbstractOzNode> node : root->nodes)
    result = visitor.Eval(node.get());
  return result;
}

}}  // namespace combinators::oz
