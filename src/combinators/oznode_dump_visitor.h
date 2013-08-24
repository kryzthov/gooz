#ifndef COMBINATORS_OZNODE_DUMP_VISITOR_H_
#define COMBINATORS_OZNODE_DUMP_VISITOR_H_

#include <iostream>

#include "base/stringer.h"
#include "combinators/oznode.h"

namespace combinators { namespace oz {

class DumpVisitor : public AbstractOzNodeVisitor {
 public:
  DumpVisitor(std::ostream& os):
      os_(os),
      compact_(false),
      level_(0),
      indented_(false) {
  }

  virtual void Visit(OzNode* node) {
    Indent();
    {
      Node n(this, "OzNode", true);  // compact
      os_ << node->type;
      switch (node->type) {
        case OzLexemType::ATOM:
        case OzLexemType::INTEGER:
        case OzLexemType::STRING:
          os_ << "=" << Str(node->tokens.first().value);
        default: break;
      }
    }
    NewLine();
  }

  virtual void Visit(OzNodeGeneric* node) {
    Node n(this, "OzNodeGeneric");
    Indent() << "type:" << node->type << " ";
    if (node->tokens.StreamSize() > 0) {
      os_ << "tokens:" << node->tokens.first();
      if (node->tokens.StreamSize() > 1)
        os_ << ".." << node->tokens.last();
    }
    NewLine();

    int i = 1;
    for (shared_ptr<AbstractOzNode> branch : node->nodes) {
      Indent() << "node" << i++ << ":"; branch->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeError* node) {
    Indent();
    {
      Node n(this, "OzNodeError", true);  // compact
      os_ << node->error;
    }
    NewLine();
  }

  virtual void Visit(OzNodeVar* node) {
    Indent();
    {
      Node n(this, "OzNodeVar", true);  // compact
      os_ << node->var_name;
    }
    NewLine();
  }

  virtual void Visit(OzNodeRecord* node) {
    Node n(this, "OzNodeRecord");
    Indent() << "label:"; node->label->AcceptVisitor(this);
    Indent() << "features:"; node->features->AcceptVisitor(this);
    Indent() << "open:" << (node->open ? "true" : "false"); NewLine();
  }

  virtual void Visit(OzNodeUnaryOp* node) {
    Node n(this, "OzNodeUnaryOp");
    Indent() << "operator:" << node->type; NewLine();
    Indent() << "operand:"; node->operand->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeBinaryOp* node) {
    Node n(this, "OzNodeBinaryOp");
    Indent() << "operator:" << node->type; NewLine();
    Indent() << "left:"; node->lop->AcceptVisitor(this);
    Indent() << "right:"; node->rop->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeNaryOp* node) {
    Node n(this, "OzNodeNaryOp");
    Indent() << "operator:" << node->type; NewLine();
    Indent() << "operands:"; NewLine();
    level_++;
    int i = 1;
    for (shared_ptr<AbstractOzNode>& op : node->operands) {
      Indent() << "operand" << i++ << ":"; op->AcceptVisitor(this);
    }
    level_--;
  }

  virtual void Visit(OzNodeFunctor* node) {
    CHECK_EQ(OzLexemType::NODE_FUNCTOR, node->type);
    Node n(this, "OzNodeFunctor");
    static auto RecursiveVisit =
        [&] (const string& section, shared_ptr<AbstractOzNode>& node) {
      level_++;
      Indent() << section << ":";
      node->AcceptVisitor(this);
      level_--;
    };

    if (node->functor != nullptr) RecursiveVisit("functor", node->functor);
    if (node->exports != nullptr) RecursiveVisit("export", node->exports);
    if (node->require != nullptr) RecursiveVisit("require", node->require);
    if (node->prepare != nullptr) RecursiveVisit("prepare", node->prepare);
    if (node->import != nullptr) RecursiveVisit("import", node->import);
    if (node->define != nullptr) RecursiveVisit("define", node->define);
  }

  virtual void Visit(OzNodeLocal* node) {
    Node n(this, "OzNodeLocal");
    if (node->defs != nullptr) {
      Indent() << "defs:"; node->defs->AcceptVisitor(this);
    }
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeProc* node) {
    Node n(this, "OzNodeProc");
    Indent() << "signature:"; node->signature->AcceptVisitor(this);
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeCond* node) {
    Node n(this, "OzNodeCond");
    Indent() << "branches:"; NewLine();
    level_++;
    int i = 1;
    for (auto branch : node->branches) {
      Indent() << "branch" << i++ << ":"; branch->AcceptVisitor(this);
    }
    level_--;
    if (node->else_branch != nullptr) {
      Indent() << "else:"; node->else_branch->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeCondBranch* node) {
    Node n(this, "OzNodeCondBranch");
    Indent() << "condition:"; node->condition->AcceptVisitor(this);
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodePatternMatch* node) {
    Node n(this, "OzNodePatternMatch");
    if (node->value != nullptr) {
      Indent() << "value:"; node->value->AcceptVisitor(this);
    } else {
      Indent() << "value: caught-exception";
    }
    Indent() << "branches:"; NewLine();
    level_++;
    int i = 1;
    for (auto branch : node->branches) {
      Indent() << "branch" << i++ << ":"; branch->AcceptVisitor(this);
    }
    level_--;
  }

  virtual void Visit(OzNodePatternBranch* node) {
    Node n(this, "OzNodePatternBranch");
    Indent() << "pattern:"; node->pattern->AcceptVisitor(this);
    if (node->condition != nullptr) {
      Indent() << "condition:"; node->condition->AcceptVisitor(this);
    }
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeThread* node) {
    Node n(this, "OzNodeThread");
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeClass* node) {
    Node n(this, "OzNodeClass");
    // TODO
  }

  virtual void Visit(OzNodeLoop* node) {
    Node n(this, "OzNodeLoop");
    Indent() << "body:"; node->body->AcceptVisitor(this);
    // TODO: more things to dump?
  }

  virtual void Visit(OzNodeForLoop* node) {
    Node n(this, "OzNodeForLoop");
    Indent() << "var:"; node->var->AcceptVisitor(this);
    Indent() << "spec:"; node->spec->AcceptVisitor(this);
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeRaise* node) {
    Node n(this, "OzNodeRaise");
    Indent() << "exn:"; node->exn->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeTry* node) {
    Node n(this, "OzNodeTry");
    Indent() << "body:"; node->body->AcceptVisitor(this);
    if (node->catches != nullptr) {
      Indent() << "catches:"; node->catches->AcceptVisitor(this);
    }
    if (node->finally != nullptr) {
      Indent() << "finally:"; node->finally->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeLock* node) {
    Node n(this, "OzNodeLock");
    Indent() << "lock:"; node->lock->AcceptVisitor(this);
    Indent() << "body:"; node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeList* node) {
    Node n(this, "OzNodeList");
    Indent() << "nodes:";
    int i = 1;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      Indent() << "node" << i++ << ":"; step->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeCall* node) {
    Node n(this, "OzNodeCall");
    Indent() << "nodes:";
    int i = 1;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      Indent() << "node" << i++ << ":"; step->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeSequence* node) {
    Node n(this, "OzNodeSequence");
    Indent() << "nodes:";
    int i = 1;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      Indent() << "node" << i++ << ":"; step->AcceptVisitor(this);
    }
  }

 private:
  inline std::ostream& NewLine() {
    indented_ = false;
    return compact_ ? os_ : os_ << std::endl;
  }

  inline std::ostream& Indent() {
    if (indented_) return os_;
    indented_ = true;
    return compact_ ? os_ : os_ << prefix();
  }

  inline string prefix() const { return string(level_ * 2, ' '); }

  class Node {
   public:
    Node(DumpVisitor* visitor, const string& name, bool compact = false)
        : visitor_(visitor),
          compact_(compact) {
      if (!compact_) visitor_->Indent();
      visitor_->os_ << name << "(";
      if (!compact_) visitor_->NewLine();
      visitor_->level_++;
    }

    ~Node() {
      visitor_->level_--;
      if (!compact_) visitor_->Indent();
      visitor_->os_ << ")";
      if (!compact_) visitor_->NewLine();
    }

    DumpVisitor* const visitor_;
    bool compact_;
  };

  std::ostream& os_;
  bool compact_;
  int level_;
  bool indented_;
};

inline
void DumpOzNode(const AbstractOzNode& node, std::ostream& os, int level = 0) {
  DumpVisitor v(os);
  const_cast<AbstractOzNode&>(node).AcceptVisitor(&v);
}

inline std::ostream& operator<<(std::ostream& os, const AbstractOzNode& node) {
  DumpOzNode(node, os);
  return os;
}

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_DUMP_VISITOR_H_
