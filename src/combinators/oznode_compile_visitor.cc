#include "combinators/oznode_compile_visitor.h"

#include "combinators/ozparser.h"

using namespace store;

namespace combinators { namespace oz {

store::Value Compile(const string& code, store::Store* store) {
  OzParser parser;
  CHECK(parser.Parse(code)) << "Error parsing: " << code;
  shared_ptr<OzNodeGeneric> root =
      std::dynamic_pointer_cast<OzNodeGeneric>(parser.root());
  LOG(INFO) << "AST:\n" << *root << std::endl;
  CHECK_EQ(OzLexemType::TOP_LEVEL, root->type);
  CompileVisitor visitor(store);
  store::Value result;
  for (shared_ptr<AbstractOzNode> node : root->nodes)
    result = visitor.Compile(node.get());
  return result;
}

// -----------------------------------------------------------------------------

// virtual
void CompileVisitor::Visit(OzNodeGeneric* node) {
  // Compiles the top-level procedure:

  if (node->type != OzLexemType::TOP_LEVEL) {
    LOG(FATAL) << "Cannot compile generic node: " << node;
  }

  // Top-level has no result:
  result_.reset(new ExpressionResult);  // statement

  // Top-level allows declaring new variables:
  declaring_ = true;

  // Top-level acts as a procedure (accumulates statements):
  segment_.reset(new vector<Bytecode>);

  for (auto def : node->nodes) {
    def->AcceptVisitor(this);
  }
}

// virtual
void CompileVisitor::Visit(OzNode* node) {
  CHECK(!result_->statement());

  const OzLexem& lexem = node->tokens.first();
  switch (node->type) {
    case OzLexemType::INTEGER: {
      result_->SetValue(Operand(
          store::New::Integer(store_, boost::get<mpz_class>(lexem.value))));
      break;
    }
    case OzLexemType::ATOM: {
      result_->SetValue(Operand(
          store::New::Atom(store_, boost::get<string>(lexem.value))));
      break;
    }
    case OzLexemType::STRING: {
      result_->SetValue(Operand(
          store::New::String(store_, boost::get<string>(lexem.value))));
      break;
    }
    case OzLexemType::REAL: {
      result_->SetValue(Operand(
          store::New::Real(store_, boost::get<real::Real>(lexem.value))));
      break;
    }
    case OzLexemType::VAR_ANON: {
      result_->SetupValuePlaceholder("NewVariable");
      segment_->push_back(
          Bytecode(Bytecode::NEW_VARIABLE, result_->into()));
      break;
    }
    default:
      LOG(FATAL) << "Unexpected node: " << *node;
  }
}

// virtual
void CompileVisitor::Visit(OzNodeProc* node) {
  shared_ptr<OzNodeCall> signature =
      std::dynamic_pointer_cast<OzNodeCall>(node->signature);
  if (signature->nodes[0]->type != OzLexemType::EXPR_VAL) {
    CHECK(result_->statement());

    // Convert statement:
    //     proc {Proc ...} ... end
    // into:
    //     Proc = proc {$ ...} ... end
    shared_ptr<OzNodeNaryOp> unify(new OzNodeNaryOp);
    unify->operation = OzLexem().SetType(OzLexemType::UNIFY);
    unify->operands.push_back(signature->nodes[0]);

    shared_ptr<OzNodeCall> expr_sig(new OzNodeCall(*signature));
    expr_sig->nodes[0] = shared_ptr<AbstractOzNode>(new OzNode);
    expr_sig->nodes[0]->type = OzLexemType::EXPR_VAL;

    shared_ptr<OzNodeProc> expr_proc(new OzNodeProc);
    expr_proc->signature = expr_sig;
    expr_proc->body = node->body;
    expr_proc->fun = node->fun;

    unify->operands.push_back(expr_proc);

    unify->AcceptVisitor(this);
    return;
  }

  CHECK(!result_->statement())
      << "Procedure value cannot be used as a statement";
  shared_ptr<ExpressionResult> result = result_;

  if (node->fun) {
    // TODO: rewrite:
    //     fun {Fun Params...} (body) end
    // into:
    //     proc {Fun Params... Result} Result = (body) end

    shared_ptr<OzNodeVar> return_var(new OzNodeVar);
    return_var->var_name = "$return_var$";  // TODO: fix this hack :(

    shared_ptr<OzNodeCall> proc_sig(new OzNodeCall(*signature));
    proc_sig->nodes.push_back(return_var);

    shared_ptr<OzNodeNaryOp> proc_body(new OzNodeNaryOp);
    proc_body->operation = OzLexem().SetType(OzLexemType::UNIFY);
    proc_body->operands.push_back(return_var);
    proc_body->operands.push_back(node->body);

    shared_ptr<OzNodeProc> proc(new OzNodeProc);
    proc->fun = false;
    proc->signature = proc_sig;
    proc->body = proc_body;

    proc->AcceptVisitor(this);
    return;
  }

  // Compile procedure values (eg. proc {$ ...} ... end) into closures:

  // Create a new environment for this procedure,
  // using the current environment as root environment.
  NestedEnvironment nested_env(this);

  // Save the current state:
  shared_ptr<vector<Bytecode> > saved_segment = segment_;
  segment_.reset(new vector<Bytecode>());

  for (uint64 iparam = 1; iparam < signature->nodes.size(); ++iparam) {
    shared_ptr<OzNodeVar> param_var =
        std::dynamic_pointer_cast<OzNodeVar>(signature->nodes[iparam]);
    environment_->AddParameter(param_var->var_name);
  }

  // After normalization, procedure body is necessarily a statement:
  result_.reset(new ExpressionResult);
  node->body->AcceptVisitor(this);

  // Generate the Closure with the number of registers from the environment.
  const uint64 nparams = environment_->nparams();
  const uint64 nlocals = environment_->nlocals();
  const uint64 nclosures = environment_->nclosures();
  Closure* const closure =
      New::Closure(store_, segment_, nparams, nlocals, nclosures)
      .as<Closure>();

  LOG(INFO) << "Compiled procedure:\n" << closure->ToString();

  // Fills in the environment linking table.
  // const vector<string>&  names = environment_->closure_symbol_names();
  // env->insert(env->begin(), names.begin(), names.end());

  // Restore saved state:
  std::swap(segment_, saved_segment);

  result_ = result;
  result_->SetValue(Operand(store::Optimize(closure).as<Closure>()));
}

// virtual
void CompileVisitor::Visit(OzNodeVar* node) {
  if (result_->statement()) {
    LOG(FATAL) << "Invalid statement: " << node;
  }

  if (declaring_ && !environment_->ExistsLocally(node->var_name)) {
    const Symbol& symbol = environment_->AddLocal(node->var_name);
    VLOG(1)
        << "New local variable: " << node->var_name
        << " - " << DebugString(symbol);
  }

  const Symbol& symbol = environment_->Get(node->var_name);
  result_->SetValue(symbol.GetOperand());
}

// virtual
void CompileVisitor::Visit(OzNodeRecord* node) {
  CHECK(!result_->statement())
      << "Invalid use of record as a statement";

  CHECK(!node->open) << "open records not supported yet";

  shared_ptr<ExpressionResult> result = result_;
  result->SetupValuePlaceholder("RecordPlaceHolder");

  result_.reset(new ExpressionResult(environment_));
  node->label->AcceptVisitor(this);

  shared_ptr<ExpressionResult> label_result = result_;

  // TODO: Can we calculate the arity statically?
  uint64 auto_feature = 1;
  vector<Value> features;
  bool static_arity = true;
  for (auto feature : node->features->nodes) {
    if (feature->type == OzLexemType::RECORD_DEF_FEATURE) {
      shared_ptr<OzNodeBinaryOp> def =
          std::dynamic_pointer_cast<OzNodeBinaryOp>(feature);
      switch (def->lop->type) {
        case OzLexemType::ATOM: {
          shared_ptr<OzNode> feat_node =
              std::dynamic_pointer_cast<OzNode>(def->lop);
          const OzLexem& lexem = feat_node->tokens.first();
          features.push_back(
              store::New::Atom(store_, boost::get<string>(lexem.value)));
          break;
        }
        case OzLexemType::INTEGER: {
          shared_ptr<OzNode> feat_node =
              std::dynamic_pointer_cast<OzNode>(def->lop);
          const OzLexem& lexem = feat_node->tokens.first();
          features.push_back(
              store::New::Integer(store_, boost::get<mpz_class>(lexem.value)));
          break;
        }
        case OzLexemType::VARIABLE: {
          static_arity = false;
          goto exit_loop;  // we are done checking
        }
        default:
          LOG(FATAL) << "Invalid record feature label: " << *def->lop;
      }

    } else {
      features.push_back(SmallInteger(auto_feature).Encode());
      auto_feature += 1;
    }
  }
exit_loop:

  result_.reset(new ExpressionResult(environment_));
  if (static_arity) {
    Arity* arity = Arity::Get(features);
    result_->SetValue(Operand(arity));
  } else {
    LOG(FATAL) << "Dynamic arities not implemented yet";
  }

  shared_ptr<ExpressionResult> arity_result = result_;

  segment_->push_back(
      Bytecode(Bytecode::NEW_RECORD,
               result->value(),
               arity_result->value(),
               label_result->value()));

  // Assign features
  auto_feature = 1;
  for (auto feature : node->features->nodes) {
    shared_ptr<ExpressionResult> label_result(
        new ExpressionResult(environment_));
    shared_ptr<ExpressionResult> value_result(
        new ExpressionResult(environment_));

    if (feature->type == OzLexemType::RECORD_DEF_FEATURE) {
      shared_ptr<OzNodeBinaryOp> def =
          std::dynamic_pointer_cast<OzNodeBinaryOp>(feature);
      result_ = label_result;
      def->lop->AcceptVisitor(this);

      result_ = value_result;
      def->rop->AcceptVisitor(this);

    } else {
      label_result->SetValue(Operand(SmallInteger(auto_feature).Encode()));
      auto_feature += 1;

      result_ = value_result;
      feature->AcceptVisitor(this);
    }

    segment_->push_back(
        Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                 result->value(),  // record
                 label_result->value(),  // feature label
                 value_result->value()));  // feature value
  }

  // Return the record:
  result_ = result;
}


// virtual
void CompileVisitor::Visit(OzNodeBinaryOp* node) {
  shared_ptr<ExpressionResult> result = result_;

  result_.reset(new ExpressionResult(environment_));
  node->lop->AcceptVisitor(this);
  shared_ptr<ExpressionResult> lop_result = result_;

  result_.reset(new ExpressionResult(environment_));
  node->rop->AcceptVisitor(this);
  shared_ptr<ExpressionResult> rop_result = result_;

  result_ = result;

  switch (node->operation.type) {
    case OzLexemType::LIST_CONS: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("ListConstructorResult");
      segment_->push_back(
          Bytecode(Bytecode::NEW_LIST,
                   result_->value(),
                   lop_result->value(),
                   rop_result->value()));
      break;
    }
    case OzLexemType::EQUAL: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("EqualityTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_EQUALITY,
                   result_->value(),
                   lop_result->value(),
                   rop_result->value()));
      break;
    }
    case OzLexemType::LESS_THAN: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("LessThanTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_THAN,
                   result_->value(),
                   lop_result->value(),
                   rop_result->value()));
      break;
    }
    case OzLexemType::LESS_OR_EQUAL: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("LessOrEqualTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_OR_EQUAL,
                   result_->value(),
                   lop_result->value(),
                   rop_result->value()));
      break;
    }
    case OzLexemType::GREATER_THAN: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("GreaterThanTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_THAN,
                   result_->value(),
                   rop_result->value(),
                   lop_result->value()));
      break;
    }
    case OzLexemType::GREATER_OR_EQUAL: {
      CHECK(!result_->statement())
          << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("GreaterOrEqualTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_OR_EQUAL,
                   result_->value(),
                   rop_result->value(),
                   lop_result->value()));
      break;
    }
    case OzLexemType::CELL_ASSIGN: {
      // TODO: ASSIGN_CELL as an expression (atomic read+write)
      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_CELL,
                   lop_result->value(),  // cell
                   rop_result->value()));  // value
      break;
    }
    case OzLexemType::RECORD_ACCESS: {
      // TODO: optimize this when used as an operand in a unify operation
      //     using Bytecode::UNIFY_RECORD_FIELD.
      result_->SetupValuePlaceholder("RecordAccessResult");
      segment_->push_back(
          Bytecode(Bytecode::ACCESS_RECORD,
                   result_->value(),
                   lop_result->value(),  // record
                   rop_result->value()));  // feature
      break;
    }
    default:
      LOG(FATAL) << "Invalid or unsupported binary operator: " << node->type;
  }
}

// virtual
void CompileVisitor::Visit(OzNodeUnaryOp* node) {
  // store::Value value = Eval(node->operand.get());
  // switch (node->type) {
  //   case OzLexemType::NUMERIC_NEG: {
  //     switch (value.type()) {
  //       case store::Value::SMALL_INTEGER: {
  //         value_ = store::New::Integer(store_, -store::IntValue(value));
  //         break;
  //       }
  //       default:
  //         LOG(FATAL) << "Unsupported operand: " << value.ToString();
  //     }
  //     break;
  //   }
  //   default:
  //     LOG(FATAL) << "Unary operator not supported: " << node->type;
  // }
}

// virtual
void CompileVisitor::Visit(OzNodeNaryOp* node) {
  // Operator may be:
  //  - unify
  //  - tuple
  //  - numeric multiply
  //  - numeric add
  const uint64 size = node->operands.size();
  CHECK_GE(size, 1UL);

  switch (node->operation.type) {
    case OzLexemType::UNIFY:
      CompileUnify(node);
      break;
    case OzLexemType::TUPLE_CONS:
      CompileTupleCons(node);
      break;
    case OzLexemType::NUMERIC_MUL:
    case OzLexemType::NUMERIC_ADD:
      CompileMulOrAdd(node);
      break;
    default:
      LOG(FATAL) << "invalid n-ary operator: " << node->operation;
  }
}

void CompileVisitor::CompileUnify(OzNodeNaryOp* node) {
  // The result for the entire unification expression/statement:
  shared_ptr<ExpressionResult> result = result_;

  const bool is_statement = result->statement();
  if (is_statement) {
    // Create an expression result place-holder for the first expression.
    result_.reset(new ExpressionResult(environment_));
  } else {
    // First expression is the result for the entire unification expression.
  }

  // Compute and save the first operand.
  // Other operands will be unified against this one.
  // The first operand is the one being returned, if this is an expression.
  const bool saved_declaring = declaring_;
  declaring_ = true;
  node->operands[0]->AcceptVisitor(this);
  shared_ptr<ExpressionResult> first = result_;
  declaring_ = saved_declaring;

  const uint64 size = node->operands.size();
  for (uint64 i = 1; i < size; ++i) {
    // Compute the next operand:
    result_.reset(new ExpressionResult(environment_));
    node->operands[i]->AcceptVisitor(this);

    segment_->push_back(
        Bytecode(Bytecode::UNIFY,
                 first->value(),
                 result_->value()));
  }

  // Returns the result for this unify expression, if not a statement
  result_ = is_statement ? result : first;
}

void CompileVisitor::CompileTupleCons(OzNodeNaryOp* node) {
  CHECK(!result_->statement()) << "Invalid use of tuple as statement.";
  result_->SetupValuePlaceholder("TupleResult");
  shared_ptr<ExpressionResult> result = result_;

  // TODO: merge with Visit(OzNodeRecord*) ?
  Operand size_op(SmallInteger(node->operands.size()).Encode());
  Operand label_op(KAtomTuple());
  Operand tuple_op = result_->into();
  segment_->push_back(
      Bytecode(Bytecode::NEW_TUPLE, tuple_op, size_op, label_op));

  for (uint64 ival = 0; ival < node->operands.size(); ++ival) {
    Operand feat_op(SmallInteger(ival + 1).Encode());

    // Compute the value of this tuple feature:
    ScopedTemp feat_temp(environment_, "TupleFeatureTemp");
    result_.reset(new ExpressionResult(feat_temp.symbol()));
    node->operands[ival]->AcceptVisitor(this);

    segment_->push_back(
        Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                 tuple_op,
                 feat_op,
                 result_->value()));
  }

  result_ = result;
}

void CompileVisitor::CompileMulOrAdd(OzNodeNaryOp* node) {
  Bytecode::OpcodeType opcode;
  switch (node->operation.type) {
    case OzLexemType::NUMERIC_MUL: {
      opcode = Bytecode::NUMBER_INT_MULTIPLY;
      break;
    }
    case OzLexemType::NUMERIC_ADD: {
      opcode = Bytecode::NUMBER_INT_ADD;
      break;
    }
    default: LOG(FATAL) << "Unsupported operation: " << node->operation.type;
  }

  CHECK(!result_->statement())
      << "Invalid use of numeric operation as statement.";
  result_->SetupValuePlaceholder("NumericResult");
  shared_ptr<ExpressionResult> result = result_;
  node->operands[0]->AcceptVisitor(this);

  for (int iop = 1; iop < node->operands.size(); ++iop) {
    result_.reset(new ExpressionResult(environment_));  // = rop
    node->operands[iop]->AcceptVisitor(this);

    segment_->push_back(
        Bytecode(opcode,
                 result->into(),      // in
                 result->value(),     // number1 (left operand)
                 result_->value()));  // number2 (right operand)
    result->SetValue(result->into());
  }

  result_ = result;
}

// virtual
void CompileVisitor::Visit(OzNodeFunctor* node) {
  LOG(FATAL) << "Cannot evaluate functors";
}

// virtual
void CompileVisitor::Visit(OzNodeLocal* node) {
  shared_ptr<ExpressionResult> result = result_;

  unique_ptr<Environment::NestedLocalAllocator> local(
      environment_->NewNestedLocalAllocator());

  if (node->defs != nullptr) {
    result_.reset(new ExpressionResult);  // statement
    node->defs->AcceptVisitor(this);
  }

  // Lock the local allocator:
  //  - symbols from this scope are still visible,
  //  - no new symbol may be defined in this scope.
  local->Lock();

  result_ = result;
  if (node->body != nullptr) {
    node->body->AcceptVisitor(this);
  }

  // Remove the local symbols:
  local.reset();
}

// virtual
void CompileVisitor::Visit(OzNodeCond* node) {
  Value saved_cond_next_branch_ip = cond_next_branch_ip_;
  Value saved_cond_end_ip = cond_end_ip_;
  shared_ptr<ExpressionResult> result = result_;

  cond_next_branch_ip_ = Variable::New(store_);
  cond_end_ip_ = Variable::New(store_);

  const bool is_statement = result_->statement();
  if (!is_statement) {
    // Forces the result to be stored in this place-holder:
    result_->SetupValuePlaceholder("ConditionalResultValue");
  }

  for (auto branch : node->branches) {
    Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
    cond_next_branch_ip_ = Variable::New(store_);

    branch->AcceptVisitor(this);  // enforces result in place-holder
  }

  if (node->else_branch != nullptr) {
    Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
    cond_next_branch_ip_ = Variable::New(store_);

    node->else_branch->AcceptVisitor(this);

    if (!is_statement && (result_->value() != result_->into())) {
      // enforces result in place-holder:
      segment_->push_back(
          Bytecode(Bytecode::UNIFY,
                   result_->value(),
                   result_->into()));
    }
  }

  Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
  Unify(cond_end_ip_, Value::Integer(segment_->size()));

  cond_next_branch_ip_ = saved_cond_next_branch_ip;
  cond_end_ip_ = saved_cond_end_ip;
  result_ = result;
  if (!is_statement) {
    result_->SetValue(result_->into());  // acknowledge result in place-holder
  }
}

// virtual
void CompileVisitor::Visit(OzNodeCondBranch* node) {
  shared_ptr<ExpressionResult> result = result_;

  // Evaluate the boolean condition for the branch:
  result_.reset(new ExpressionResult(environment_));
  node->condition->AcceptVisitor(this);

  // Jump to next branch if condition evaluates to false:
  segment_->push_back(
      Bytecode(Bytecode::BRANCH_UNLESS,
               result_->value(),
               Operand(cond_next_branch_ip_)));

  // Otherwise, execute branch:
  result_ = result;
  node->body->AcceptVisitor(this);

  if (!result_->statement() && (result_->value() != result_->into())) {
    // enforces result in place-holder:
    segment_->push_back(
        Bytecode(Bytecode::UNIFY,
                 result_->value(),
                 result_->into()));
  }

  segment_->push_back(
      Bytecode(Bytecode::BRANCH,
               Operand(cond_end_ip_)));
}

// virtual
void CompileVisitor::Visit(OzNodePatternMatch* node) {
  shared_ptr<ExpressionResult> result = result_;

  shared_ptr<ExpressionResult> matched_val_result(
      new ExpressionResult(environment_));

  result_ = matched_val_result;
  node->value->AcceptVisitor(this);
  result_ = result;

  Operand matched_val = matched_val_result->value();

  for (auto branch : node->branches) {
    shared_ptr<OzNodePatternBranch> pbranch =
        std::dynamic_pointer_cast<OzNodePatternBranch>(branch);

    // TODO: Evaluate pattern
    // pbranch->pattern->AcceptVisitor(this);
    LOG(FATAL) << "not implemented";

    // Jump to next branch if condition evaluates to false:
    segment_->push_back(
        Bytecode(Bytecode::BRANCH_UNLESS,
                 result_->value(),  // FIXME
                 Operand(cond_next_branch_ip_)));

    if (pbranch->condition != nullptr) {
      result_.reset(new ExpressionResult(environment_));
      pbranch->condition->AcceptVisitor(this);

      // Jump to next branch if condition evaluates to false:
      segment_->push_back(
          Bytecode(Bytecode::BRANCH_UNLESS,
                   result_->value(),
                   Operand(cond_next_branch_ip_)));

      result_ = result;
    }

    pbranch->body->AcceptVisitor(this);

    segment_->push_back(
        Bytecode(Bytecode::BRANCH,
                 Operand(cond_end_ip_)));
  }
}

// virtual
void CompileVisitor::Visit(OzNodePatternBranch* node) {
  LOG(FATAL) << "Pattern branching not implemented";
}

// virtual
void CompileVisitor::Visit(OzNodeThread* node) {
  LOG(FATAL) << "Cannot evaluate threads";
}

// virtual
void CompileVisitor::Visit(OzNodeLoop* node) {
  LOG(FATAL) << "Cannot evaluate loops";
}

// virtual
void CompileVisitor::Visit(OzNodeForLoop* node) {
  LOG(FATAL) << "Cannot evaluate loops";
}

// virtual
void CompileVisitor::Visit(OzNodeLock* node) {
  LOG(FATAL) << "Cannot evaluate locks";
}

// virtual
void CompileVisitor::Visit(OzNodeTry* node) {
  LOG(FATAL) << "Cannot evaluate try blocks";
}

// virtual
void CompileVisitor::Visit(OzNodeRaise* node) {
  shared_ptr<ExpressionResult> result = result_;
  result_.reset(new ExpressionResult(environment_));
  node->exn->AcceptVisitor(this);
  segment_->push_back(
      Bytecode(Bytecode::EXN_RAISE, result_->value()));
  result_ = result;
  // No need to set the result value here.
}

// virtual
void CompileVisitor::Visit(OzNodeClass* node) {
  LOG(FATAL) << "classes are not implemented";
}

// virtual
void CompileVisitor::Visit(OzNodeSequence* node) {
  CHECK_GE(node->nodes.size(), 1);
  shared_ptr<ExpressionResult> result = result_;

  const uint64 ilast = (node->nodes.size() - 1);
  for (uint64 i = 0; i <= ilast; ++i) {
    const bool is_last = (i == ilast);
    if (is_last) {
      result_ = result;
    } else {
      result_.reset(new ExpressionResult());  // statement
    }
    node->nodes[i]->AcceptVisitor(this);
  }
}

// virtual
void CompileVisitor::Visit(OzNodeCall* node) {
  // Expression {Proc Param1 ... ParamK} implies an implicit return parameter.
  // Expression {Proc Param1 ... $ ... ParamK} has an explicit return parameter.

  shared_ptr<ExpressionResult> result = result_;
  const bool is_statement = result->statement();

  if (!is_statement) {
    result->SetupValuePlaceholder("CallReturnPlaceholder");
  }

  // Determine if there is a return parameter '$':
  bool has_expr_val = false;
  for (uint64 iparam = 1; iparam < node->nodes.size(); ++iparam) {
    if (node->nodes[iparam]->type == OzLexemType::EXPR_VAL) {
      CHECK(!has_expr_val) << "Invalid call with multiple '$':\n" << *node;
      has_expr_val = true;
    }
  }

  // Return parameter requires this to be an expression:
  CHECK(!(has_expr_val && is_statement))
      << "Invalid statement call with '$':\n" << *node;

  // Determine the actual number of parameters for the call,
  // including the implicit return value, if needed:
  uint64 nparams = (node->nodes.size() - 1);
  if (!is_statement && !has_expr_val) nparams += 1;

  ScopedTemp params_temp(environment_);
  Operand params_op;  // Invalid by default.

  if (nparams > 0) {
    params_op = params_temp.Allocate("CallParametersArray");

    // Create the parameters array:
    segment_->push_back(
        Bytecode(Bytecode::NEW_ARRAY,
                 params_op,
                 Operand(Value::Integer(nparams)),
                 Operand(KAtomEmpty())));  // dummy value

    // Compute each parameter and set its value in the array:
    for (uint64 iparam = 1; iparam < node->nodes.size(); ++iparam) {
      shared_ptr<AbstractOzNode> param = node->nodes[iparam];
      Operand param_op;
      result_.reset(new ExpressionResult(environment_));

      if (param->type == OzLexemType::EXPR_VAL) {  // Explicit output parameter
        param_op = result->value();
        segment_->push_back(Bytecode(Bytecode::NEW_VARIABLE, param_op));

      } else {  // Normal parameter
        param->AcceptVisitor(this);
        param_op = result_->value();
      }

      CHECK(!param_op.invalid());
      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_ARRAY,
                   params_op,
                   Operand(Value::Integer(iparam - 1)),
                   param_op));
    }

    // Initialize the implicit return-value parameter, if needed:
    if (!is_statement && !has_expr_val) {
      Operand param_op = result->value();
      segment_->push_back(Bytecode(Bytecode::NEW_VARIABLE, param_op));
      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_ARRAY,
                   params_op,
                   Operand(Value::Integer(nparams - 1)),
                   param_op));
    }

  } else {
    // No parameter
  }

  // Evaluate the expression that determines which procedure to invoke:
  shared_ptr<AbstractOzNode> proc = node->nodes.front();
  result_.reset(new ExpressionResult(environment_));
  proc->AcceptVisitor(this);
  Operand proc_op = result_->value();

  // This is a little bit inaccurate, but good enough to start with:
  const bool native = (proc_op.type == Operand::IMMEDIATE)
      && (proc_op.value.IsA<Atom>());

  segment_->push_back(
      Bytecode(native ? Bytecode::CALL_NATIVE : Bytecode::CALL,
               proc_op,
               params_op));

  // Result for this call expression/statement:
  result_ = result;
}

// virtual
void CompileVisitor::Visit(OzNodeList* node) {
  CHECK(!result_->statement())
      << "Invalid use of list as statement.";
  shared_ptr<ExpressionResult> result = result_;

  vector<shared_ptr<ExpressionResult> > elements;
  for (auto element : node->nodes) {
    result_.reset(new ExpressionResult(environment_));
    element->AcceptVisitor(this);
    elements.push_back(result_);
  }

  ScopedTemp temp(environment_, "ListBuilder");
  segment_->push_back(
      Bytecode(Bytecode::LOAD,
               temp.GetOperand(),  // dest
               Operand(KAtomNil())));  // src

  for (int64 ielt = elements.size() - 1; ielt >= 0; --ielt) {
    segment_->push_back(
        Bytecode(Bytecode::NEW_LIST,
                 temp.GetOperand(),  // into
                 elements[ielt]->value(),  // head
                 temp.GetOperand()));  // tail
  }

  result_ = result;
  result_->SetValue(temp.GetOperand());
}


}}  // namespace combinators::oz
