#ifndef COMBINATORS_OZNODE_H_
#define COMBINATORS_OZNODE_H_

#include <memory>
#include <string>

#include "combinators/stream.h"
#include "combinators/stream.h"
#include "combinators/ozlexer.h"

using std::shared_ptr;
using std::string;
using std::vector;

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

class AbstractOzNode;
class OzNode;
class OzNodeBinaryOp;
class OzNodeCall;
class OzNodeClass;
class OzNodeCond;
class OzNodeCondBranch;
class OzNodeError;
class OzNodeForLoop;
class OzNodeFunctor;
class OzNodeGeneric;
class OzNodeList;
class OzNodeLocal;
class OzNodeLock;
class OzNodeLoop;
class OzNodeNaryOp;
class OzNodePatternBranch;
class OzNodePatternMatch;
class OzNodeProc;
class OzNodeRaise;
class OzNodeRecord;
class OzNodeSequence;
class OzNodeThread;
class OzNodeTry;
class OzNodeUnaryOp;
class OzNodeVar;

// -----------------------------------------------------------------------------

// Interface for AST node visitors:
class AbstractOzNodeVisitor {
 public:
  AbstractOzNodeVisitor() {}
  virtual ~AbstractOzNodeVisitor() {}

  virtual void Visit(OzNode* node) = 0;
  virtual void Visit(OzNodeBinaryOp* node) = 0;
  virtual void Visit(OzNodeCall* node) = 0;
  virtual void Visit(OzNodeClass* node) = 0;
  virtual void Visit(OzNodeCond* node) = 0;
  virtual void Visit(OzNodeCondBranch* node) = 0;
  virtual void Visit(OzNodeError* node) = 0;
  virtual void Visit(OzNodeForLoop* node) = 0;
  virtual void Visit(OzNodeFunctor* node) = 0;
  virtual void Visit(OzNodeGeneric* node) = 0;
  virtual void Visit(OzNodeList* node) = 0;
  virtual void Visit(OzNodeLocal* node) = 0;
  virtual void Visit(OzNodeLock* node) = 0;
  virtual void Visit(OzNodeLoop* node) = 0;
  virtual void Visit(OzNodeNaryOp* node) = 0;
  virtual void Visit(OzNodePatternBranch* node) = 0;
  virtual void Visit(OzNodePatternMatch* node) = 0;
  virtual void Visit(OzNodeProc* node) = 0;
  virtual void Visit(OzNodeRaise* node) = 0;
  virtual void Visit(OzNodeRecord* node) = 0;
  virtual void Visit(OzNodeSequence* node) = 0;
  virtual void Visit(OzNodeThread* node) = 0;
  virtual void Visit(OzNodeTry* node) = 0;
  virtual void Visit(OzNodeUnaryOp* node) = 0;
  virtual void Visit(OzNodeVar* node) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AbstractOzNodeVisitor);
};

// -----------------------------------------------------------------------------

// Abstract base class for all AST nodes.
class AbstractOzNode {
 public:
  AbstractOzNode()
      : type(OzLexemType::INVALID) {
  }

  AbstractOzNode(const OzLexemStream& tokens_)
      : type(tokens_.StreamEmpty()
             ? OzLexemType::INVALID
             : tokens_.first().type),
        tokens(tokens_) {
  }

  virtual ~AbstractOzNode() {}

  OzLexemType type;
  OzLexemStream tokens;

  inline
  AbstractOzNode& SetType(OzLexemType type_) {
    type = type_;
    return *this;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) = 0;

  // Sub-classes must be declared VISITABLE():
  #define VISITABLE()                                          \
  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) { \
    visitor->Visit(this);                                      \
  }

};

// -----------------------------------------------------------------------------

// AST node wrapping a single lexical element.
class OzNode : public AbstractOzNode {
 public:
  OzNode() : AbstractOzNode() {
  }

  OzNode(const OzLexemStream& tokens)
      : AbstractOzNode(tokens) {
  }

  VISITABLE();
};

// -----------------------------------------------------------------------------

// AST node representing an arbitrary sequence of nodes.
//
// After the mid-level parser is run, there should be no generic nodes left,
// except for the following cases:
//  - top-level node
//  - record features
class OzNodeGeneric : public AbstractOzNode {
 public:
  OzNodeGeneric() {}

  OzNodeGeneric(const OzLexemStream& stream)
      : AbstractOzNode(stream) {
  }

  OzNodeGeneric(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
  }

  VISITABLE();

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

// Node representing a part of the AST / a sequence of tokens that could not
// be parsed properly.
class OzNodeError : public AbstractOzNode {
 public:
  static inline OzNodeError& New() { return *new OzNodeError; }

  OzNodeError() {}

  OzNodeError& SetTokens(OzLexemStream tokens_) {
    tokens = tokens_;
    return *this;
  }

  OzNodeError& SetError(const string& error_) {
    error = error_;
    return *this;
  }

  template <class Node>
  OzNodeError& SetNode(shared_ptr<Node>& node_) {
    node = node_;
    return *this;
  }

  VISITABLE();

  // Error message
  string error;

  // Node being parsed
  shared_ptr<AbstractOzNode> node;
};

// -----------------------------------------------------------------------------

// AST node representing a symbolic variable.
class OzNodeVar : public AbstractOzNode {
 public:
  OzNodeVar() {}
  OzNodeVar(const OzLexemStream& tokens_)
      : AbstractOzNode(tokens_),
        no_declare(false),
        is_output(false) {
    CHECK_EQ(OzLexemType::VARIABLE, type);
    token = tokens_.first();
    var_name = boost::get<string>(token.value);
  }

  VISITABLE();

  OzLexem token;
  string var_name;

  // !X ensures that X is a pre-existing variable
  bool no_declare;

  // ?X indicates an output parameter
  bool is_output;
};

// -----------------------------------------------------------------------------

// AST node describing a record.
class OzNodeRecord : public AbstractOzNode {
 public:
  OzNodeRecord()
      : AbstractOzNode(),
        open(false) {
    type = OzLexemType::NODE_RECORD;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> label;
  shared_ptr<OzNodeGeneric> features;
  bool open;
};

// -----------------------------------------------------------------------------

// AST node representing a unary expression.
class OzNodeUnaryOp : public AbstractOzNode {
 public:
  VISITABLE();

  OzLexem operation;
  shared_ptr<AbstractOzNode> operand;
};

// -----------------------------------------------------------------------------

// AST node representing a binary expression.
class OzNodeBinaryOp : public AbstractOzNode {
 public:
  VISITABLE();

  OzLexem operation;
  shared_ptr<AbstractOzNode> lop, rop;
};

// -----------------------------------------------------------------------------

// AST node representing a n-ary expression.
class OzNodeNaryOp : public AbstractOzNode {
 public:
  VISITABLE();

  OzLexem operation;
  vector<shared_ptr<AbstractOzNode> > operands;
};

// -----------------------------------------------------------------------------

// AST node for a functor declaration.
class OzNodeFunctor : public AbstractOzNode {
 public:
  OzNodeFunctor() {
    type = OzLexemType::NODE_FUNCTOR;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> functor;
  shared_ptr<AbstractOzNode> exports;
  shared_ptr<AbstractOzNode> require;
  shared_ptr<AbstractOzNode> prepare;
  shared_ptr<AbstractOzNode> import;
  shared_ptr<AbstractOzNode> define;
};

// -----------------------------------------------------------------------------

// AST node for a local scope.
class OzNodeLocal : public AbstractOzNode {
 public:
  OzNodeLocal() {
    type = OzLexemType::NODE_LOCAL;
  }

  OzNodeLocal(const OzNodeGeneric& node) : AbstractOzNode(node) {
    type = OzLexemType::NODE_LOCAL;
  }

  VISITABLE();

  // Defining section (variables may be declared here), may be null:
  shared_ptr<AbstractOzNode> defs;

  // Non defining section (variables may not be declared here), may be null:
  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a procedure or function definition.
class OzNodeProc : public AbstractOzNode {
 public:
  OzNodeProc() {
    type = OzLexemType::NODE_PROC;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> signature;
  shared_ptr<AbstractOzNode> body;
  bool fun;
};

// -----------------------------------------------------------------------------

// AST node for a class definition.
class OzNodeClass : public AbstractOzNode {
 public:
  OzNodeClass() {
    type = OzLexemType::NODE_CLASS;
  }

  VISITABLE();

  // TODO: Define
};

// -----------------------------------------------------------------------------

// AST node for a thread definition.
class OzNodeThread : public AbstractOzNode {
 public:
  OzNodeThread() {
    type = OzLexemType::NODE_THREAD;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

class OzNodeCondBranch : public AbstractOzNode {
 public:
  OzNodeCondBranch() {}

  VISITABLE();

  shared_ptr<AbstractOzNode> condition;
  shared_ptr<AbstractOzNode> body;
};

class OzNodePatternBranch : public AbstractOzNode {
 public:
  OzNodePatternBranch() {}

  VISITABLE();

  shared_ptr<AbstractOzNode> pattern;
  shared_ptr<AbstractOzNode> condition;  // may be null
  shared_ptr<AbstractOzNode> body;
};

class OzNodePatternMatch : public AbstractOzNode {
 public:
  OzNodePatternMatch() {}

  VISITABLE();

  shared_ptr<AbstractOzNode> value;  // null in exn catch statements.
  vector<shared_ptr<AbstractOzNode> > branches;
};

// AST node for branching (if/case).
class OzNodeCond : public AbstractOzNode {
 public:
  OzNodeCond(const OzNodeGeneric& node)
      : AbstractOzNode(node) {
  }

  VISITABLE();

  // Branches are either OzNodeCondBranch or OzNodePatternMatch.
  vector<shared_ptr<AbstractOzNode> > branches;

  shared_ptr<AbstractOzNode> else_branch;  // may be null
};

// -----------------------------------------------------------------------------

// AST node for exception handling statements try/catch/finally.
class OzNodeTry : public AbstractOzNode {
 public:
  OzNodeTry() {
    type = OzLexemType::NODE_TRY;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> body;
  shared_ptr<AbstractOzNode> catches;  // may be null or OzNodePatternMatch
  shared_ptr<AbstractOzNode> finally;  // may be null
};

// -----------------------------------------------------------------------------

// AST node for a raise statement.
class OzNodeRaise : public AbstractOzNode {
 public:
  OzNodeRaise() {
    type = OzLexemType::NODE_RAISE;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> exn;
};

// -----------------------------------------------------------------------------

// AST node for an infinite loop.
class OzNodeLoop : public AbstractOzNode {
 public:
  OzNodeLoop() {
    type = OzLexemType::NODE_LOOP;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a 'for' loop definition.
class OzNodeForLoop : public AbstractOzNode {
 public:
  OzNodeForLoop() {
    type = OzLexemType::FOR;
  }

  VISITABLE();

  // TODO: loop declaration???
  shared_ptr<AbstractOzNode> var;
  shared_ptr<AbstractOzNode> spec;
  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a lock statement.
class OzNodeLock : public AbstractOzNode {
 public:
  OzNodeLock() {
    type = OzLexemType::NODE_LOCK;
  }

  VISITABLE();

  shared_ptr<AbstractOzNode> lock;
  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a list definition
class OzNodeList : public AbstractOzNode {
 public:
  OzNodeList(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
    type = OzLexemType::NODE_LIST;
  }

  VISITABLE();

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

// AST node for a function call
class OzNodeCall : public AbstractOzNode {
 public:
  OzNodeCall(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
    type = OzLexemType::NODE_CALL;
  }

  VISITABLE();

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

// AST node for a sequence of instructions.
class OzNodeSequence : public AbstractOzNode {
 public:
  OzNodeSequence(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
    type = OzLexemType::NODE_SEQUENCE;
  }

  VISITABLE();

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_H_
