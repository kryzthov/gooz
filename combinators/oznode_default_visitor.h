#ifndef COMBINATORS_OZNODE_DEFAULT_VISITOR_H_
#define COMBINATORS_OZNODE_DEFAULT_VISITOR_H_

#include "combinators/oznode.h"

namespace combinators { namespace oz {

class DefaultVisitor : public AbstractOzNodeVisitor {
 public:
  DefaultVisitor() {}

  virtual void Visit(OzNode* node) {
    // Nothing to visit from here
  }

  virtual void Visit(OzNodeGeneric* node) {
    for (shared_ptr<AbstractOzNode> branch : node->nodes)
      branch->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeError* node) {
    // Need to go deeper?
  }

  virtual void Visit(OzNodeVar* node) {
    // Nothing to visit from here
  }

  virtual void Visit(OzNodeRecord* node) {
    node->label->AcceptVisitor(this);
    node->features->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeUnaryOp* node) {
    node->operand->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeBinaryOp* node) {
    node->lop->AcceptVisitor(this);
    node->rop->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeNaryOp* node) {
    for (shared_ptr<AbstractOzNode>& op : node->operands)
      op->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeFunctor* node) {
    if (node->functor != nullptr) node->functor->AcceptVisitor(this);
    if (node->exports != nullptr) node->exports->AcceptVisitor(this);
    if (node->require != nullptr) node->require->AcceptVisitor(this);
    if (node->prepare != nullptr) node->prepare->AcceptVisitor(this);
    if (node->import != nullptr) node->import->AcceptVisitor(this);
    if (node->define != nullptr) node->define->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeLocal* node) {
    if (node->defs != nullptr) node->defs->AcceptVisitor(this);
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeProc* node) {
    node->signature->AcceptVisitor(this);
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeCond* node) {
    for (auto branch : node->branches) {
      branch->AcceptVisitor(this);
    }
    if (node->else_branch != nullptr) {
      node->else_branch->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeCondBranch* node) {
    node->condition->AcceptVisitor(this);
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodePatternMatch* node) {
    if (node->value != nullptr) node->value->AcceptVisitor(this);
    for (auto branch : node->branches) {
      branch->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodePatternBranch* node) {
    node->pattern->AcceptVisitor(this);
    if (node->condition != nullptr) {
      node->condition->AcceptVisitor(this);
    }
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeThread* node) {
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeClass* node) {
    LOG(FATAL) << "Not implemented";
  }

  virtual void Visit(OzNodeLock* node) {
    node->lock->AcceptVisitor(this);
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeLoop* node) {
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeForLoop* node) {
    // TODO: Complete?
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeTry* node) {
    node->body->AcceptVisitor(this);
    if (node->catches != nullptr) node->catches->AcceptVisitor(this);
    if (node->finally != nullptr) node->finally->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeRaise* node) {
    node->exn->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeList* node) {
    for (auto step : node->nodes) {
      step->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeCall* node) {
    for (auto step : node->nodes) {
      step->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeSequence* node) {
    for (auto step : node->nodes) {
      step->AcceptVisitor(this);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultVisitor);
};

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_DEFAULT_VISITOR_H_
