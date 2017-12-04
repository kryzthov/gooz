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
    CompileStatement(def);
  }

  CHECK_EQ(0, environment_->nparams());
  CHECK_EQ(0, environment_->nclosures());

  top_level_ =
      New::Closure(store_, segment_, 0, environment_->nlocals(), 0)
      .as<Closure>();

  LOG(INFO) << "Top-level procedure:\n" << top_level_->ToString();
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
    CHECK(IsStatement());

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

    Compile(unify, result_);
    return;
  }

  CHECK(IsExpression()) << "Procedure value cannot be used as a statement";

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

    CompileExpression(proc, result_);
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
  CompileStatement(node->body);

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

  result_->SetValue(Operand(store::Optimize(closure).as<Closure>()));
}

// virtual
void CompileVisitor::Visit(OzNodeVar* node) {
  CHECK(IsExpression()) << "Invalid statement: " << node;

  if (declaring_
      && !node->no_declare
      && !environment_->ExistsLocally(node->var_name)) {
    const Symbol& symbol = environment_->AddLocal(node->var_name);
    VLOG(1)
        << "New local variable: " << node->var_name
        << " - " << DebugString(symbol);
  }

  const Symbol& symbol = environment_->Get(node->var_name);
  result_->SetValue(symbol.GetOperand());
}

Arity* CompileVisitor::GetStaticArity(OzNodeRecord* record) {
  uint64 auto_feature = 1;
  vector<Value> features;
  for (auto feature : record->features->nodes) {
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
          // Cannot evaluate arity statically:
          // We could push static analyzis further...
          return nullptr;
        }
        default:
          LOG(FATAL) << "Invalid record feature label: " << *def->lop;
      }

    } else {
      features.push_back(SmallInteger(auto_feature).Encode());
      auto_feature += 1;
    }
  }
  return Arity::Get(features);
}

// virtual
void CompileVisitor::Visit(OzNodeRecord* node) {
  CHECK(IsExpression()) << "Invalid use of record as a statement";
  CHECK(!node->open) << "open records not supported yet";

  shared_ptr<ExpressionResult> record_label_result =
      CompileExpression(node->label);

  // Is the arity known statically?
  Arity* arity = GetStaticArity(node);
  shared_ptr<ExpressionResult> arity_result(
      new ExpressionResult(environment_));
  if (arity == nullptr) {
    // TODO: Compile arity constructor
    LOG(FATAL) << "Dynamic arities not implemented yet";
  } else {
    arity_result->SetValue(Operand(arity));
  }

  result_->SetupValuePlaceholder("RecordPlaceHolder");
  segment_->push_back(
      Bytecode(Bytecode::NEW_RECORD,
               result_->value(),
               arity_result->value(),
               record_label_result->value()));


  // Assign features
  uint64 auto_feature = 1;
  for (auto feature : node->features->nodes) {
    shared_ptr<ExpressionResult> label_result;
    shared_ptr<ExpressionResult> value_result;

    if (feature->type == OzLexemType::RECORD_DEF_FEATURE) {
      shared_ptr<OzNodeBinaryOp> def =
          std::dynamic_pointer_cast<OzNodeBinaryOp>(feature);
      label_result = CompileExpression(def->lop);
      value_result = CompileExpression(def->rop);

    } else {
      // Automatically compute feature label:
      label_result.reset(new ExpressionResult(environment_));
      label_result->SetValue(Operand(SmallInteger(auto_feature).Encode()));
      auto_feature += 1;

      value_result = CompileExpression(feature);
    }

    segment_->push_back(
        Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                 result_->value(),         // record
                 label_result->value(),    // feature label
                 value_result->value()));  // feature value
  }
}


// virtual
void CompileVisitor::Visit(OzNodeBinaryOp* node) {
  shared_ptr<ExpressionResult> lop = CompileExpression(node->lop);
  shared_ptr<ExpressionResult> rop = CompileExpression(node->rop);

  switch (node->operation.type) {
    case OzLexemType::LIST_CONS: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("ListConstructorResult");
      segment_->push_back(
          Bytecode(Bytecode::NEW_LIST,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    case OzLexemType::EQUAL: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("EqualityTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_EQUALITY,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    case OzLexemType::LESS_THAN: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("LessThanTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_THAN,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    case OzLexemType::LESS_OR_EQUAL: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("LessOrEqualTestResult");
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_OR_EQUAL,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    case OzLexemType::GREATER_THAN: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("GreaterThanTestResult");
      // Switch left and right operands on purpose:
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_THAN,
                   result_->value(),
                   rop->value(),
                   lop->value()));
      break;
    }
    case OzLexemType::GREATER_OR_EQUAL: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("GreaterOrEqualTestResult");
      // Switch left and right operands on purpose:
      segment_->push_back(
          Bytecode(Bytecode::TEST_LESS_OR_EQUAL,
                   result_->value(),
                   rop->value(),
                   lop->value()));
      break;
    }
    case OzLexemType::CELL_ASSIGN: {
      // TODO: ASSIGN_CELL as an expression (atomic read+write)
      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_CELL,
                   lop->value(),  // cell
                   rop->value()));  // value
      break;
    }
    case OzLexemType::RECORD_ACCESS: {
      // TODO: optimize this when used as an operand in a unify operation
      //     using Bytecode::UNIFY_RECORD_FIELD.
      result_->SetupValuePlaceholder("RecordAccessResult");
      segment_->push_back(
          Bytecode(Bytecode::ACCESS_RECORD,
                   result_->value(),
                   lop->value(),    // record
                   rop->value()));  // feature
      break;
    }
    case OzLexemType::NUMERIC_MINUS: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("NumericMinusResult");
      segment_->push_back(
          Bytecode(Bytecode::NUMBER_INT_SUBTRACT,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    case OzLexemType::NUMERIC_DIV: {
      CHECK(IsExpression()) << "Invalid use of binary expression as statement.";
      result_->SetupValuePlaceholder("NumericDivResult");
      segment_->push_back(
          Bytecode(Bytecode::NUMBER_INT_DIVIDE,
                   result_->value(),
                   lop->value(),
                   rop->value()));
      break;
    }
    default:
      LOG(FATAL) << "Invalid or unsupported binary operator: " << node->type;
  }
}

// virtual
void CompileVisitor::Visit(OzNodeUnaryOp* node) {
  CHECK(IsExpression())
      << "Invalid use of unary expression as statement" << *node;

  switch (node->operation.type) {
    case OzLexemType::NUMERIC_NEG: {
      result_->SetupValuePlaceholder("NumericNegResult");
      Compile(node->operand, result_);
      segment_->push_back(
          Bytecode(
              Bytecode::NUMBER_INT_INVERSE,
              result_->into(),     // in
              result_->value()));  // number
      break;
    }
    case OzLexemType::VAR_NODEF: {
      if (node->operand->type != OzLexemType::VARIABLE) {
        LOG(FATAL)
            << "Non-declaring operator can only be applied to variables, got: "
            << *node;
      }
      shared_ptr<OzNodeVar> var =
          std::dynamic_pointer_cast<OzNodeVar>(node->operand);
      var->no_declare = true;
      Compile(var, result_);
      break;
    }
    default:
      LOG(FATAL) << "Invalid or unsupported unary operator: "
                 << node->operation.type;
  }
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
  // Compute and save the first operand.
  // Other operands will be unified against this one.
  // The first operand is the one being returned, if this is an expression.
  const bool saved_declaring = declaring_;
  declaring_ = true;
  shared_ptr<ExpressionResult> first =
      IsExpression()
      ? CompileExpression(node->operands[0], result_)
      : CompileExpression(node->operands[0]);
  declaring_ = saved_declaring;

  const uint64 size = node->operands.size();
  for (uint64 i = 1; i < size; ++i) {
    shared_ptr<ExpressionResult> result = CompileExpression(node->operands[i]);
    segment_->push_back(
        Bytecode(Bytecode::UNIFY,
                 first->value(),
                 result->value()));
  }
}

void CompileVisitor::CompileTupleCons(OzNodeNaryOp* node) {
  CHECK(IsExpression()) << "Invalid use of tuple as statement.";

  Operand size_op(SmallInteger(node->operands.size()).Encode());
  Operand label_op(KAtomTuple());
  Operand tuple_op = result_->SetupValuePlaceholder("TupleResult");
  segment_->push_back(
      Bytecode(Bytecode::NEW_TUPLE,
               tuple_op,    // into
               size_op,     // tuple size
               label_op));  // tuple label

  for (uint64 ival = 0; ival < node->operands.size(); ++ival) {
    Operand feat_op(SmallInteger(ival + 1).Encode());

    // Compute the value of this tuple feature:
    shared_ptr<ExpressionResult> feat = CompileExpression(node->operands[ival]);
    segment_->push_back(
        Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                 tuple_op,
                 feat_op,
                 feat->value()));
  }
}

void CompileVisitor::CompileMulOrAdd(OzNodeNaryOp* node) {
  CHECK(IsExpression()) << "Invalid use of numeric operation as statement.";

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

  // Evaluate the first left operand in-place:
  result_->SetupValuePlaceholder("NumericResult");
  CompileExpression(node->operands[0], result_);

  for (int iop = 1; iop < node->operands.size(); ++iop) {
    shared_ptr<ExpressionResult> rop_result =
        CompileExpression(node->operands[iop]);

    segment_->push_back(
        Bytecode(opcode,
                 result_->into(),        // in
                 result_->value(),       // number1 (left operand)
                 rop_result->value()));  // number2 (right operand)
    result_->SetValue(result_->into());
  }
}

// virtual
void CompileVisitor::Visit(OzNodeFunctor* node) {
  LOG(FATAL) << "Cannot evaluate functors";
}

// virtual
void CompileVisitor::Visit(OzNodeLocal* node) {
  unique_ptr<Environment::NestedLocalAllocator> local(
      environment_->NewNestedLocalAllocator());

  if (node->defs != nullptr) {
    CompileStatement(node->defs);
  }

  // Lock the local allocator:
  //  - symbols from this scope are still visible,
  //  - no new symbol may be defined in this scope.
  local->Lock();

  if (node->body != nullptr) {
    Compile(node->body, result_);  // handle both statement and expression
  }

  // Remove the local symbols:
  local.reset();
}

// virtual
void CompileVisitor::Visit(OzNodeCond* node) {
  Value saved_cond_next_branch_ip = cond_next_branch_ip_;
  Value saved_cond_end_ip = cond_end_ip_;

  cond_next_branch_ip_ = Variable::New(store_);
  cond_end_ip_ = Variable::New(store_);

  const bool is_statement = IsStatement();
  if (!is_statement) {
    // Forces the result to be stored in this place-holder:
    result_->SetupValuePlaceholder("ConditionalResultValue");
  }

  for (auto branch : node->branches) {
    Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
    cond_next_branch_ip_ = Variable::New(store_);

    // enforces result in place-holder
    Compile(branch, result_);
  }

  if (node->else_branch != nullptr) {
    Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
    cond_next_branch_ip_ = Variable::New(store_);

    Compile(node->else_branch, result_);

    if (IsExpression() && (result_->value() != result_->into())) {
      // enforces result in place-holder:
      segment_->push_back(
          Bytecode(Bytecode::UNIFY,  // UNIFY or LOAD?
                   result_->value(),
                   result_->into()));
    }
  }

  Unify(cond_next_branch_ip_, Value::Integer(segment_->size()));
  Unify(cond_end_ip_, Value::Integer(segment_->size()));

  cond_next_branch_ip_ = saved_cond_next_branch_ip;
  cond_end_ip_ = saved_cond_end_ip;
  if (!is_statement) {
    result_->SetValue(result_->into());  // acknowledge result in place-holder
  }
}

// virtual
void CompileVisitor::Visit(OzNodeCondBranch* node) {
  // Evaluate the boolean condition for the branch:
  shared_ptr<ExpressionResult> cond_result = CompileExpression(node->condition);

  // Jump to next branch if condition evaluates to false:
  segment_->push_back(
      Bytecode(Bytecode::BRANCH_UNLESS,
               cond_result->value(),
               Operand(cond_next_branch_ip_)));

  // If condition evaluates to true, execute the branch body:
  Compile(node->body, result_);

  if (IsExpression() && (result_->value() != result_->into())) {
    // enforces result in place-holder:
    segment_->push_back(
        Bytecode(Bytecode::UNIFY,  // UNIFY or LOAD?
                 result_->value(),
                 result_->into()));
  }

  // After the branch body, jump right after the end of the conditional:
  segment_->push_back(
      Bytecode(Bytecode::BRANCH,
               Operand(cond_end_ip_)));
}

// virtual
void CompileVisitor::Visit(OzNodePatternMatch* node) {
  shared_ptr<ExpressionResult> val_result = CompileExpression(node->value);

  // !!!!!
  // FIXME: pattern matching should not affect/mutate the value.
  // !!!!!

  for (auto branch : node->branches) {
    unique_ptr<Environment::NestedLocalAllocator> local(
        environment_->NewNestedLocalAllocator());

    shared_ptr<OzNodePatternBranch> pbranch =
        std::dynamic_pointer_cast<OzNodePatternBranch>(branch);

    // Create pattern:
    shared_ptr<ExpressionResult> pattern_result =
        CompileExpression(pbranch->pattern);

    ScopedTemp success_temp(environment_, "try_unify_success");

    // Attempt to match pattern with value:
    segment_->push_back(
        Bytecode(Bytecode::TRY_UNIFY,
                 result_->value(),
                 val_result->value(),
                 success_temp.GetOperand()));

    // Jump to next branch if condition evaluates to false:
    segment_->push_back(
        Bytecode(Bytecode::BRANCH_UNLESS,
                 success_temp.GetOperand(),
                 Operand(cond_next_branch_ip_)));

    if (pbranch->condition != nullptr) {
      shared_ptr<ExpressionResult> cond_result =
          CompileExpression(pbranch->condition);

      // Jump to next branch if condition evaluates to false:
      segment_->push_back(
          Bytecode(Bytecode::BRANCH_UNLESS,
                   cond_result->value(),
                   Operand(cond_next_branch_ip_)));
    }

    Compile(pbranch->body, result_);

    segment_->push_back(
        Bytecode(Bytecode::BRANCH,
                 Operand(cond_end_ip_)));
  }
}

// virtual
void CompileVisitor::Visit(OzNodePatternBranch* node) {
  LOG(FATAL) << "Internal error";
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
  // If this is an expression, this should probably default the value to an
  // undetermined variable.
  if (IsExpression()) {
    result_->SetupValuePlaceholder("RaiseUndeterminedResult");
    segment_->push_back(
        Bytecode(Bytecode::NEW_VARIABLE, result_->into()));
  }

  shared_ptr<ExpressionResult> exn = CompileExpression(node->exn);
  segment_->push_back(
      Bytecode(Bytecode::EXN_RAISE, result_->value()));
}

// virtual
void CompileVisitor::Visit(OzNodeClass* node) {
  LOG(FATAL) << "classes are not implemented";
}

// virtual
void CompileVisitor::Visit(OzNodeSequence* node) {
  CHECK_GE(node->nodes.size(), 1);

  const uint64 ilast = (node->nodes.size() - 1);
  for (uint64 i = 0; i <= ilast; ++i) {
    const bool is_last = (i == ilast);
    if (is_last) {
      Compile(node->nodes[i], result_);
    } else {
      CompileStatement(node->nodes[i]);
    }
  }
}

// virtual
void CompileVisitor::Visit(OzNodeCall* node) {
  // Expression {Proc Param1 ... ParamK} implies an implicit return parameter.
  // Expression {Proc Param1 ... $ ... ParamK} has an explicit return parameter.

  const bool saved_declaring = declaring_;
  declaring_ = false;

  // Determine if there is a return parameter '$':
  bool has_expr_val = false;
  for (uint64 iparam = 1; iparam < node->nodes.size(); ++iparam) {
    if (node->nodes[iparam]->type == OzLexemType::EXPR_VAL) {
      CHECK(!has_expr_val) << "Invalid call with multiple '$':\n" << *node;
      has_expr_val = true;
    }
  }

  // Return parameter requires this to be an expression:
  CHECK(!(has_expr_val && IsStatement()))
      << "Invalid statement call with '$':\n" << *node;

  if (IsExpression()) {
    // Setup return value place-holder:
    segment_->push_back(
        Bytecode(Bytecode::NEW_VARIABLE,
                 result_->SetupValuePlaceholder("CallReturnPlaceholder")));
  }

  // Determine the actual number of parameters for the call,
  // including the implicit return value, if needed:
  uint64 nparams = (node->nodes.size() - 1);
  if (IsExpression() && !has_expr_val) nparams += 1;

  ScopedTemp params_temp(environment_);
  Operand params_op;  // Invalid by default.

  if (nparams > 0) {
    params_op = params_temp.Allocate("CallParametersArray");

    // Create the parameters array:
    segment_->push_back(
        Bytecode(Bytecode::NEW_ARRAY,
                 params_op,
                 Operand(New::Integer(store_, nparams)),
                 Operand(KAtomEmpty())));  // dummy value

    // Compute each parameter and set its value in the array:
    for (uint64 iparam = 1; iparam < node->nodes.size(); ++iparam) {
      shared_ptr<AbstractOzNode> param_node = node->nodes[iparam];
      shared_ptr<ExpressionResult> param_result(
          new ExpressionResult(environment_));

      if (param_node->type == OzLexemType::EXPR_VAL) {
        // Explicit output parameter
        param_result->SetValue(result_->into());

      } else {
        // Expression parameter
        CompileExpression(param_node, param_result);
      }

      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_ARRAY,
                   params_op,
                   Operand(New::Integer(store_, iparam - 1)),
                   param_result->value()));
    }

    if (IsExpression() && !has_expr_val) {
      // Implicit output parameter
      segment_->push_back(
          Bytecode(Bytecode::ASSIGN_ARRAY,
                   params_op,
                   Operand(New::Integer(store_, nparams - 1)),
                   result_->into()));
    }

  } else {
    // No parameter for the call
  }

  // Evaluate the expression that determines which procedure to invoke:
  shared_ptr<ExpressionResult> proc_result =
      CompileExpression(node->nodes.front());
  Operand proc_op = proc_result->value();

  // This is a little bit inaccurate, but good enough to start with:
  const bool native =
      (proc_op.type == Operand::IMMEDIATE)
      && (proc_op.value.IsA<Atom>());

  segment_->push_back(
      Bytecode(native ? Bytecode::CALL_NATIVE : Bytecode::CALL,
               proc_op,
               params_op));

  // Restore state:
  declaring_ = saved_declaring;
}

// virtual
void CompileVisitor::Visit(OzNodeList* node) {
  CHECK(IsExpression()) << "Invalid use of list as statement.";
  result_->SetupValuePlaceholder("ListConstructor");

  vector<shared_ptr<ExpressionResult> > elements;
  for (auto element : node->nodes) {
    elements.push_back(CompileExpression(element));
  }

  segment_->push_back(
      Bytecode(Bytecode::LOAD,
               result_->into(),        // dest
               Operand(KAtomNil())));  // src

  for (int64 ielt = elements.size() - 1; ielt >= 0; --ielt) {
    segment_->push_back(
        Bytecode(Bytecode::NEW_LIST,
                 result_->into(),          // into
                 elements[ielt]->value(),  // head
                 result_->value()));       // tail
  }
}

}}  // namespace combinators::oz
