#ifndef COMBINATORS_OZNODE_FORMAT_VISITOR_H_
#define COMBINATORS_OZNODE_FORMAT_VISITOR_H_

#include <iostream>

#include "base/stringer.h"
#include "combinators/oznode.h"

namespace combinators { namespace oz {

string Format(OzLexemType type);

class FormatVisitor : public AbstractOzNodeVisitor {
 public:
  FormatVisitor(std::ostream& os):
      os_(os),
      compact_(false),
      level_(0),
      indented_(false) {
  }

  virtual void Visit(OzNode* node) {
    os_ << Str(node->tokens.first().value);
  }

  virtual void Visit(OzNodeGeneric* node) {
    bool first = true;
    for (shared_ptr<AbstractOzNode> branch : node->nodes) {
      if (!first) os_ << " ";
      branch->AcceptVisitor(this);
      first = false;
    }
  }

  virtual void Visit(OzNodeError* node) {
    os_ << "<" << node->error << ">";
  }

  virtual void Visit(OzNodeVar* node) {
    os_ << node->var_name;
  }

  virtual void Visit(OzNodeRecord* node) {
    node->label->AcceptVisitor(this);
    os_ << "(";
    node->features->AcceptVisitor(this);
    if (node->open) os_ << "...";
    os_ << ")";
  }

  virtual void Visit(OzNodeUnaryOp* node) {
    os_ << "(";
    os_ << Format(node->type);
    node->operand->AcceptVisitor(this);
    os_ << ")";
  }

  virtual void Visit(OzNodeBinaryOp* node) {
    os_ << "(";
    node->lop->AcceptVisitor(this);
    os_ << Format(node->type);
    node->rop->AcceptVisitor(this);
    os_ << ")";
  }

  virtual void Visit(OzNodeNaryOp* node) {
    os_ << "(";
    bool first = true;
    for (shared_ptr<AbstractOzNode>& op : node->operands) {
      if (!first) os_ << Format(node->type);
      op->AcceptVisitor(this);
      first = false;
    }
    os_ << ")";
  }

  virtual void Visit(OzNodeFunctor* node) {
    Indent() << "functor";
    static auto RecursiveVisit =
        [&] (const string& section, shared_ptr<AbstractOzNode>& node) {
      level_++;
      Indent() << section;
      node->AcceptVisitor(this);
      level_--;
    };

    if (node->functor != nullptr) RecursiveVisit("functor", node->functor);
    if (node->exports != nullptr) RecursiveVisit("export", node->exports);
    if (node->require != nullptr) RecursiveVisit("require", node->require);
    if (node->prepare != nullptr) RecursiveVisit("prepare", node->prepare);
    if (node->import != nullptr) RecursiveVisit("import", node->import);
    if (node->define != nullptr) RecursiveVisit("define", node->define);
    Indent() << "end";
  }

  virtual void Visit(OzNodeLocal* node) {
    Indent() << " local ";
    if (node->defs != nullptr) {
      node->defs->AcceptVisitor(this);
      Indent() << " in ";
    }
    node->body->AcceptVisitor(this);
    Indent() << " end ";
  }

  virtual void Visit(OzNodeProc* node) {
    Indent() << " " << (node->fun ? "fun" : "proc") << " ";
    node->signature->AcceptVisitor(this);
    node->body->AcceptVisitor(this);
    Indent() << " end ";
  }

  virtual void Visit(OzNodeCond* node) {
    level_++;
    bool first = true;
    for (auto branch : node->branches) {
      branch->AcceptVisitor(this);
      first = false;
    }
    level_--;

    if (node->else_branch != nullptr) {
      os_ << " else ";
      node->else_branch->AcceptVisitor(this);
    }
  }

  virtual void Visit(OzNodeCondBranch* node) {
    Indent() << " [else]if ";
    node->condition->AcceptVisitor(this);
    os_ << " then ";
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodePatternMatch* node) {
    if (node->value != nullptr) {
      Indent() << " [else]case ";
      node->value->AcceptVisitor(this);
    } else {
      // Exception catching section
    }
    level_++;
    int i = 1;
    for (auto branch : node->branches) {
      Indent() << "of ";
      branch->AcceptVisitor(this);
    }
    level_--;
  }

  virtual void Visit(OzNodePatternBranch* node) {
    node->pattern->AcceptVisitor(this);
    if (node->condition != nullptr) {
      os_ << " andthen "; node->condition->AcceptVisitor(this);
    }
    os_ << " then ";
    node->body->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeThread* node) {
    Indent() << " thread ";
    node->body->AcceptVisitor(this);
    Indent() << " end ";
  }

  virtual void Visit(OzNodeClass* node) {
    Node n(this, "OzNodeClass");
    // TODO
    LOG(FATAL) << "Not implemented";
  }

  virtual void Visit(OzNodeLoop* node) {
    Node n(this, "OzNodeLoop");
    Indent() << "body:"; node->body->AcceptVisitor(this);
    // TODO: more things to dump?
    LOG(FATAL) << "Not implemented";
  }

  virtual void Visit(OzNodeForLoop* node) {
    Indent() << " for ";
    node->var->AcceptVisitor(this);
    os_ << " in ";
    node->spec->AcceptVisitor(this);
    os_ << " do ";
    node->body->AcceptVisitor(this);
    Indent() << " end ";
  }

  virtual void Visit(OzNodeRaise* node) {
    Node n(this, "OzNodeRaise");
    Indent() << "exn:"; node->exn->AcceptVisitor(this);
  }

  virtual void Visit(OzNodeTry* node) {
    Indent() << " try ";
    node->body->AcceptVisitor(this);
    if (node->catches != nullptr) {
      Indent() << " catch ";
      node->catches->AcceptVisitor(this);
    }
    if (node->finally != nullptr) {
      Indent() << " finally ";
      node->finally->AcceptVisitor(this);
    }
    Indent() << " end ";
  }

  virtual void Visit(OzNodeLock* node) {
    Indent() << " lock ";
    node->lock->AcceptVisitor(this);
    Indent() << " then ";
    node->body->AcceptVisitor(this);
    Indent() << " end ";
  }

  virtual void Visit(OzNodeList* node) {
    os_ << "[";
    bool first = true;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      if (!first) os_ << " ";
      step->AcceptVisitor(this);
      first = false;
    }
    os_ << "]";
  }

  virtual void Visit(OzNodeCall* node) {
    os_ << "{";
    bool first = true;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      if (!first) os_ << " ";
      step->AcceptVisitor(this);
      first = false;
    }
    os_ << "}";
  }

  virtual void Visit(OzNodeSequence* node) {
    bool first = true;
    for (shared_ptr<AbstractOzNode> step : node->nodes) {
      if (!first) os_ << " ";
      step->AcceptVisitor(this);
      first = false;
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
    Node(FormatVisitor* visitor, const string& name, bool compact = false)
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

    FormatVisitor* const visitor_;
    bool compact_;
  };

  std::ostream& os_;
  bool compact_;
  int level_;
  bool indented_;
};

inline
void FormatOzNode(const AbstractOzNode& node, std::ostream& os, int level = 0) {
  FormatVisitor v(os);
  const_cast<AbstractOzNode&>(node).AcceptVisitor(&v);
}

// Private helper class
class FormattingOzNode_ {
 public:
  FormattingOzNode_(const AbstractOzNode& node): node_(node) {}

  const AbstractOzNode& node() const { return node_; }

 private:
  const AbstractOzNode& node_;
};

inline FormattingOzNode_ Format(const AbstractOzNode& node) {
  return FormattingOzNode_(node);
}

// Allows logging formatted nodes as LOG(LEVEL) << Format(node);
inline std::ostream& operator<<(std::ostream& os,
                                FormattingOzNode_ formatting_node) {
  FormatOzNode(formatting_node.node(), os);
  return os;
}

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_FORMAT_VISITOR_H_
