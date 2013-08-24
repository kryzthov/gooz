#ifndef COMBINATORS_OZNODE_EVAL_VISITOR_H_
#define COMBINATORS_OZNODE_EVAL_VISITOR_H_

#include <string>

#include "base/stl-util.h"
#include "combinators/oznode.h"
#include "combinators/oznode_dump_visitor.h"

#include "store/store.h"
#include "store/values.h"

using std::string;

namespace combinators { namespace oz {

class EvalVisitor : public AbstractOzNodeVisitor {
 public:
  EvalVisitor(store::Store* store)
      : store_(CHECK_NOTNULL(store)) {
  }

  store::Value Eval(AbstractOzNode* node) {
    node->AcceptVisitor(this);
    return value_;
  }

  virtual void Visit(OzNode* node) {
    const OzLexem& lexem = node->tokens.first();
    switch (node->type) {
      case OzLexemType::INTEGER: {
        value_ =
            store::New::Integer(store_, boost::get<mpz_class>(lexem.value));
        break;
      }
      case OzLexemType::ATOM: {
        value_ = store::New::Atom(store_, boost::get<string>(lexem.value));
        break;
      }
      case OzLexemType::STRING: {
        LOG(FATAL) << "Not implemented";
        // value_ =
        //     store::Value::String(store_, boost::get<string>(lexem.value));
        break;
      }
      case OzLexemType::REAL: {
        LOG(FATAL) << "Not implemented";
        // value_ =
        //     store::New::Real(store_, boost::get<real::Real>(lexem.value));
        break;
      }
      case OzLexemType::VAR_ANON: {
        value_ = store::New::Free(store_);
        break;
      }
      default:
        LOG(FATAL) << "Unexpected node: " << *node;
    }
  }

  virtual void Visit(OzNodeGeneric* node) {
    switch (node->type) {
      case OzLexemType::LIST_BEGIN: {
        const uint64 nvalues = node->nodes.size();
        store::Value values[nvalues];
        for (uint64 i = 0; i < nvalues; ++i)
          values[i] = Eval(node->nodes[i].get());
        value_ = store::New::List(store_, nvalues, values);
        break;
      }
      default:
        LOG(FATAL) << "Unexpected/unsupported node: " << *node;
    }
  }

  virtual void Visit(OzNodeError* node) {
    LOG(FATAL) << "AST error: " << node->error;
  }

  virtual void Visit(OzNodeVar* node) {
    const string& var_name = node->var_name;
    if (!vars_.contains(var_name))
      vars_[var_name] = store::New::Free(store_);
    value_ = vars_[var_name];
  }

  virtual void Visit(OzNodeRecord* node) {
    store::Value label = Eval(node->label.get());
    store::OpenRecord* record =
        store::New::OpenRecord(store_, label).as<store::OpenRecord>();

    int64 auto_counter = 1;
    for (auto feature : node->features->nodes) {
      store::Value feat_label;
      store::Value feat_value;

      if (feature->type == OzLexemType::RECORD_DEF_FEATURE) {
        OzNodeBinaryOp* def = dynamic_cast<OzNodeBinaryOp*>(feature.get());
        feat_label = Eval(def->lop.get());
        feat_value = Eval(def->rop.get());
      } else {
        feat_label = store::New::Integer(store_, auto_counter);
        feat_value = Eval(feature.get());
      }
      CHECK(record->Set(feat_label, feat_value));
      if (store::HasType(feat_label, store::Value::SMALL_INTEGER)
          && (store::IntValue(feat_label) == auto_counter))
          auto_counter++;
    }
    if (node->open)
      value_ = record;
    else
      value_ = record->GetRecord(store_);
  }

  virtual void Visit(OzNodeBinaryOp* node) {
    store::Value lop = Eval(node->lop.get());
    store::Value rop = Eval(node->rop.get());

    switch (node->type) {
      case OzLexemType::LIST_CONS: {
        value_ = store::New::List(store_, lop, rop);
        break;
      }
      default:
        LOG(FATAL) << "Binary operator not supported: " << node->type;
    }
  }

  virtual void Visit(OzNodeUnaryOp* node) {
    store::Value value = Eval(node->operand.get());
    switch (node->type) {
      case OzLexemType::NUMERIC_NEG: {
        switch (value.type()) {
          case store::Value::SMALL_INTEGER: {
            value_ = store::New::Integer(store_, -store::IntValue(value));
            break;
          }
          default:
            LOG(FATAL) << "Unsupported operand: " << value.ToString();
        }
        break;
      }
      default:
        LOG(FATAL) << "Unary operator not supported: " << node->type;
    }
  }

  virtual void Visit(OzNodeNaryOp* node) {
    const uint64 size = node->operands.size();
    store::Value operands[size];
    for (uint i = 0; i < size; ++i)
      operands[i] = Eval(node->operands[i].get());

    switch (node->type) {
      case OzLexemType::TUPLE_CONS: {
        value_ = store::New::Tuple(store_, size, operands);
        break;
      }
      case OzLexemType::UNIFY: {
        value_ = operands[0];
        for (uint i = 1; i < size; ++i)
          CHECK(store::Unify(value_, operands[i]));
        break;
      }
      default:
        LOG(FATAL) << "Binary operator not supported: " << node->type;
    }
  }

  virtual void Visit(OzNodeFunctor* node) {
    LOG(FATAL) << "Cannot evaluate functors";
  }

  virtual void Visit(OzNodeLocal* node) {
    LOG(FATAL) << "Cannot evaluate locals";
  }

  virtual void Visit(OzNodeProc* node) {
    LOG(FATAL) << "Cannot evaluate procedures";
  }

  virtual void Visit(OzNodeCond* node) {
    LOG(FATAL) << "Cannot evaluate conditionnals";
  }

  virtual void Visit(OzNodeCondBranch* node) {
    LOG(FATAL) << "Cannot evaluate branches";
  }

  virtual void Visit(OzNodePatternMatch* node) {
    LOG(FATAL) << "Cannot evaluate branches";
  }

  virtual void Visit(OzNodePatternBranch* node) {
    LOG(FATAL) << "Cannot evaluate branches";
  }

  virtual void Visit(OzNodeThread* node) {
    LOG(FATAL) << "Cannot evaluate threads";
  }

  virtual void Visit(OzNodeLoop* node) {
    LOG(FATAL) << "Cannot evaluate loops";
  }

  virtual void Visit(OzNodeForLoop* node) {
    LOG(FATAL) << "Cannot evaluate loops";
  }

  virtual void Visit(OzNodeLock* node) {
    LOG(FATAL) << "Cannot evaluate locks";
  }

  virtual void Visit(OzNodeTry* node) {
    LOG(FATAL) << "Cannot evaluate try blocks";
  }

  virtual void Visit(OzNodeRaise* node) {
    LOG(FATAL) << "Cannot evaluate raise";
  }

  virtual void Visit(OzNodeClass* node) {
    LOG(FATAL) << "Cannot evaluate class";
  }

  virtual void Visit(OzNodeCall* node) {
    CHECK_GE(node->nodes.size(), 1UL);
    OzNodeVar* const proc = dynamic_cast<OzNodeVar*>(node->nodes[0].get());
    if (proc->var_name == "NewName") {
      value_ = store::New::Name(store_);
    } else if (proc->var_name == "NewCell") {
      CHECK_EQ(2UL, node->nodes.size());
      store::Value value = Eval(node->nodes[1].get());
      value_ = store::New::Cell(store_, value);
    } else if (proc->var_name == "NewArray") {
      CHECK_EQ(3UL, node->nodes.size());
      store::Value size = Eval(node->nodes[1].get());
      store::Value value = Eval(node->nodes[2].get());
      value_ = store::New::Array(store_, store::IntValue(size), value);
    } else {
      LOG(FATAL) << "Unknown procedure name: " << proc->var_name;
    }
  }

  virtual void Visit(OzNodeSequence* node) {
    LOG(FATAL) << "Cannot evaluate sequence";
  }

  store::Value value() const { return value_; }

  const UnorderedMap<string, store::Value>& vars() const { return vars_; }

 private:
  store::Store* const store_;
  store::Value value_;
  UnorderedMap<string, store::Value> vars_;

  DISALLOW_COPY_AND_ASSIGN(EvalVisitor);
};

store::Value ParseEval(const string& code, store::Store* store);

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_EVAL_VISITOR_H_
