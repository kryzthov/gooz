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
class OzNodeGeneric;

class OzNodeError;

class OzNodeVar;
class OzNodeRecord;
class OzNodeUnaryOp;
class OzNodeBinaryOp;
class OzNodeNaryOp;

class OzNodeFunctor;
class OzNodeLocal;
class OzNodeProc;
class OzNodeClass;
class OzNodeThread;

class OzNodeCond;
class OzNodeCondBranch;
class OzNodePatternMatch;
class OzNodePatternBranch;

class OzNodeRaise;
class OzNodeTry;

class OzNodeLoop;
class OzNodeForLoop;

class OzNodeLock;

class OzNodeCall;
class OzNodeSequence;

// -----------------------------------------------------------------------------

class AbstractOzNodeVisitor {
 public:
  AbstractOzNodeVisitor() {}
  virtual ~AbstractOzNodeVisitor() {}

  virtual void Visit(OzNode* node) = 0;
  virtual void Visit(OzNodeGeneric* node) = 0;

  virtual void Visit(OzNodeError* node) = 0;

  virtual void Visit(OzNodeVar* node) = 0;
  virtual void Visit(OzNodeRecord* node) = 0;
  virtual void Visit(OzNodeBinaryOp* node) = 0;
  virtual void Visit(OzNodeUnaryOp* node) = 0;
  virtual void Visit(OzNodeNaryOp* node) = 0;

  virtual void Visit(OzNodeFunctor* node) = 0;
  virtual void Visit(OzNodeLocal* node) = 0;
  virtual void Visit(OzNodeProc* node) = 0;
  virtual void Visit(OzNodeClass* node) = 0;
  virtual void Visit(OzNodeThread* node) = 0;

  virtual void Visit(OzNodeCond* node) = 0;
  virtual void Visit(OzNodeCondBranch* node) = 0;
  virtual void Visit(OzNodePatternMatch* node) = 0;
  virtual void Visit(OzNodePatternBranch* node) = 0;

  virtual void Visit(OzNodeRaise* node) = 0;
  virtual void Visit(OzNodeTry* node) = 0;

  virtual void Visit(OzNodeLoop* node) = 0;
  virtual void Visit(OzNodeForLoop* node) = 0;

  virtual void Visit(OzNodeLock* node) = 0;

  virtual void Visit(OzNodeCall* node) = 0;
  virtual void Visit(OzNodeSequence* node) = 0;

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }
};

// -----------------------------------------------------------------------------

// AST node representing a sequence of nodes.
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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  OzLexem token;
  string var_name;
  bool no_declare;
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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> label;
  shared_ptr<OzNodeGeneric> features;
  bool open;
};

// -----------------------------------------------------------------------------

// AST node representing a unary expression.
class OzNodeUnaryOp : public AbstractOzNode {
 public:
  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  OzLexem operation;
  shared_ptr<AbstractOzNode> operand;
};

// -----------------------------------------------------------------------------

// AST node representing a binary expression.
class OzNodeBinaryOp : public AbstractOzNode {
 public:
  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  OzLexem operation;
  shared_ptr<AbstractOzNode> lop, rop;
};

// -----------------------------------------------------------------------------

// AST node representing a n-ary expression.
class OzNodeNaryOp : public AbstractOzNode {
 public:
  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> defs;
  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a procedure or function definition.
class OzNodeProc : public AbstractOzNode {
 public:
  OzNodeProc() {
    type = OzLexemType::NODE_PROC;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  // TODO: Define
};

// -----------------------------------------------------------------------------

// AST node for a thread definition.
class OzNodeThread : public AbstractOzNode {
 public:
  OzNodeThread() {
    type = OzLexemType::NODE_THREAD;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

class OzNodeCondBranch : public AbstractOzNode {
 public:
  OzNodeCondBranch() {}

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> condition;
  shared_ptr<AbstractOzNode> body;
};

class OzNodePatternBranch : public AbstractOzNode {
 public:
  OzNodePatternBranch() {}

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> pattern;
  shared_ptr<AbstractOzNode> condition;  // may be null
  shared_ptr<AbstractOzNode> body;
};

class OzNodePatternMatch : public AbstractOzNode {
 public:
  OzNodePatternMatch() {}

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> value;  // null in exn catch statements.
  vector<shared_ptr<AbstractOzNode> > branches;
};

// AST node for branching (if/case).
class OzNodeCond : public AbstractOzNode {
 public:
  OzNodeCond() {}

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> exn;
};

// -----------------------------------------------------------------------------

// AST node for an infinite loop.
class OzNodeLoop : public AbstractOzNode {
 public:
  OzNodeLoop() {
    type = OzLexemType::NODE_LOOP;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a 'for' loop definition.
class OzNodeForLoop : public AbstractOzNode {
 public:
  OzNodeForLoop() {
    type = OzLexemType::FOR;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

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

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  shared_ptr<AbstractOzNode> lock;
  shared_ptr<AbstractOzNode> body;
};

// -----------------------------------------------------------------------------

// AST node for a function call
class OzNodeCall : public AbstractOzNode {
 public:
  OzNodeCall() {
    type = OzLexemType::NODE_CALL;
  }

  OzNodeCall(const OzLexemStream& stream)
      : AbstractOzNode(stream) {
    type = OzLexemType::NODE_CALL;
  }

  OzNodeCall(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
    type = OzLexemType::NODE_CALL;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

// AST node for a sequence of instructions.
class OzNodeSequence : public AbstractOzNode {
 public:
  OzNodeSequence() {
    type = OzLexemType::NODE_SEQUENCE;
  }

  OzNodeSequence(const OzLexemStream& stream)
      : AbstractOzNode(stream) {
    type = OzLexemType::NODE_SEQUENCE;
  }

  OzNodeSequence(const OzNodeGeneric& node)
      : AbstractOzNode(node),
        nodes(node.nodes) {
    type = OzLexemType::NODE_SEQUENCE;
  }

  virtual void AcceptVisitor(AbstractOzNodeVisitor* visitor) {
    visitor->Visit(this);
  }

  vector<shared_ptr<AbstractOzNode> > nodes;
};

// -----------------------------------------------------------------------------

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_H_
