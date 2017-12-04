#include "store/compiler.h"

#include <memory>
using std::shared_ptr;
using std::unique_ptr;

#include <glog/logging.h>

#include "store/values.h"
#include "store/ozvalue.h"

namespace store {

Compiler::Compiler(Store* store, Environment* environment)
    : store_(CHECK_NOTNULL(store)),
      environment_(environment),
      segment_(new vector<Bytecode>) {
}

Closure* Compiler::CompileProcedure(OzValue desc, vector<string>* env) {
  CHECK(desc.label() == "proc");
  CHECK_NOTNULL(env);

  // Creates a nested environment with the procedure parameters.
  NestedEnvironment nested_env(this);

  if (desc.HasFeature("params")) {
    OzValue params = desc["params"];
    CHECK(params.IsTuple()) << "procedure params must be a tuple.";
    const uint64 nparams = params.size();
    for (uint i = 1; i <= nparams; ++i) {
      const string& var_name = params[i].atom_val();
      environment_->AddParameter(var_name);
    }
  }

  CompileExpression(desc["code"], NULL);

  // Generate the Closure with the number of registers from the environment.
  const uint64 nparams = environment_->nparams();
  const uint64 nlocals = environment_->nlocals();
  const uint64 nclosures = environment_->nclosures();
  Closure* const closure =
      New::Closure(store_, segment_, nparams, nlocals, nclosures)
      .as<Closure>();

  // Fills in the environment linking table.
  const vector<string>&  names = environment_->closure_symbol_names();
  env->insert(env->begin(), names.begin(), names.end());

  return store::Optimize(closure).as<Closure>();
}

void Compiler::CompileSkip(OzValue skip) {
  segment_->push_back(Bytecode(Bytecode::NO_OPERATION));
}

void Compiler::CompileCall(OzValue desc, ExpressionResult* result) {
  const bool is_expr = (result != NULL);

  CHECK(desc.label() == "call");
  const bool native = desc.HasFeature("native");
  CHECK(native != desc.HasFeature("proc"));

  bool has_return_value = false;

  ScopedTemp params_temp(environment_);
  Operand params_op;  // Invalid by default.
  // Evaluate the parameters into an array of values
  if (desc.HasFeature("params")) {
    OzValue params_desc = desc["params"];
    CHECK(params_desc.IsTuple()) << "call params must be a tuple.";
    const uint64 nparams = params_desc.size();
    if (nparams > 0) {
      params_temp.Allocate("CallParametersArray");
      params_op = params_temp.GetOperand();

      segment_->push_back(Bytecode(Bytecode::NEW_ARRAY,
                                   params_op,
                                   Operand(Value::Integer(nparams)),
                                   Operand(KAtomEmpty())));  // dummy value
      for (uint64 i = 1; i <= nparams; ++i) {
        OzValue param_desc = params_desc[i];
        Operand param_op;
        ExpressionResult param_er(environment_);

        if (param_desc == "returned") {  // Explicit output parameter
          CHECK(!has_return_value)
              << "Procedure call may have only one returned value.";
          has_return_value = true;

          result->SetupValuePlaceholder("CallReturnedValue");
          param_op = result->value();
          segment_->push_back(Bytecode(Bytecode::NEW_VARIABLE, param_op));

        } else {  // Normal parameter
          CompileExpression(param_desc, &param_er);
          param_op = param_er.value();
        }
        CHECK(!param_op.invalid());
        segment_->push_back(Bytecode(Bytecode::ASSIGN_ARRAY,
                                     params_op,
                                     Operand(Value::Integer(i - 1)),
                                     param_op));
      }
    }
  }

  OzValue proc_desc = desc[native ? "native" : "proc"];
  ExpressionResult proc_er(environment_);
  CompileExpression(proc_desc, &proc_er);

  segment_->push_back(Bytecode(native ? Bytecode::CALL_NATIVE : Bytecode::CALL,
                               proc_er.value(),
                               params_op));

  CHECK(is_expr == has_return_value);
}

void Compiler::CompilePattern(const Operand& match_value,
                              OzValue pattern_desc,
                              Value next_pattern_ip) {
  if (pattern_desc.IsOpenRecord()) {
    LOG(FATAL) << "Not implemented";

  } else if (!pattern_desc.IsDetermined()) {
    // Pattern is a catch-all: nothing to check
    // TODO: IsDetermined() returns false for in case of an open-record
    // Is this the right behavior for IsDetermined?

  } else if (pattern_desc.IsRecord()) {
    if (pattern_desc.label() == "decl_var") {
      // De-structure into a new variable
      // TODO: Allow to recursively match a pattern here, eg. with
      //     decl_var(x(<pattern>)) or decl_var(x pattern:<pattern>)
      const string& var_name = pattern_desc[1].atom_val();

      // TODO: Allow to create a symbol for the existing match_value operand.
      const Symbol& var_symbol = environment_->AddLocal(var_name);
      segment_->push_back(Bytecode(Bytecode::LOAD,
                                   var_symbol.GetOperand(),
                                   match_value));

    } else if (pattern_desc.label() == "var") {
      // Variable content to match
      const string& var_name = pattern_desc[1].atom_val();

      ScopedTemp cond_temp(environment_, "PatternEqualityCond");
      segment_->push_back(Bytecode(Bytecode::TEST_EQUALITY,
                                   cond_temp.GetOperand(),
                                   match_value,
                                   environment_->Get(var_name).GetOperand()));
      segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                   cond_temp.GetOperand(),
                                   Operand(next_pattern_ip)));

    } else if (pattern_desc.label() == "record") {
      // Explicit record value to match
      OzValue label = pattern_desc["label"];
      OzValue features = pattern_desc["features"];
      // TODO: Implement
      LOG(FATAL) << "Not implemented";

    } else if (pattern_desc.label() == "val") {
      // Pattern is an immediate value to match
      ScopedTemp cond_temp(environment_, "PatternEqualityCond");
      segment_->push_back(Bytecode(Bytecode::TEST_EQUALITY,
                                   cond_temp.GetOperand(),
                                   match_value,
                                   Operand(pattern_desc[1].value())));
      segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                   cond_temp.GetOperand(),
                                   Operand(next_pattern_ip)));

    } else {
      // Match against a record pattern
      {
        // Tests the value kind
        ScopedTemp cond_temp(environment_, "PatternIsRecordCond");
        segment_->push_back(Bytecode(Bytecode::TEST_IS_RECORD,
                                     cond_temp.GetOperand(),
                                     match_value));
        segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                     cond_temp.GetOperand(),
                                     Operand(next_pattern_ip)));
      }

      {
        // Tests the record label
        OzValue label = pattern_desc.label();
        ScopedTemp label_temp(environment_, "MatchValueLabel");
        segment_->push_back(Bytecode(Bytecode::ACCESS_RECORD_LABEL,
                                     label_temp.GetOperand(),
                                     match_value));
        ScopedTemp cond_temp(environment_, "PatternLabelCond");
        segment_->push_back(Bytecode(Bytecode::TEST_EQUALITY,
                                     cond_temp.GetOperand(),
                                     label_temp.GetOperand(),
                                     Operand(label.value())));
        segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                     cond_temp.GetOperand(),
                                     Operand(next_pattern_ip)));
      }

      {
        // Tests the record arity and features
        ScopedTemp arity_temp(environment_, "MatchValueArity");
        segment_->push_back(Bytecode(Bytecode::ACCESS_OPEN_RECORD_ARITY,
                                     arity_temp.GetOperand(),
                                     match_value));
        if (HasType(pattern_desc.value(), Value::OPEN_RECORD)) {
          // TODO: Implement through extends
          LOG(FATAL) << "Not implemented";
        } else {
          ScopedTemp cond_temp(environment_, "ArityEqualsCond");
          segment_->push_back(Bytecode(Bytecode::TEST_EQUALITY,
                                       cond_temp.GetOperand(),
                                       arity_temp.GetOperand(),
                                       Operand(pattern_desc.value()
                                               .RecordArity())));
          segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                       cond_temp.GetOperand(),
                                       Operand(next_pattern_ip)));
        }

      }
      {
        // Recursively tests the record feature value patterns
        unique_ptr<Value::ItemIterator> it_deleter(
            pattern_desc.value().RecordIterItems());
        for (Value::ItemIterator& it = *it_deleter; !it.at_end(); ++it) {
          OzValue label = (*it).first;
          OzValue sub_pattern = (*it).second;
          ScopedTemp feature_temp(environment_, "FeatureTemp");
          segment_->push_back(Bytecode(Bytecode::ACCESS_RECORD,
                                       feature_temp.GetOperand(),
                                       match_value,
                                       Operand(label.value())));
          CompilePattern(feature_temp.GetOperand(),
                         sub_pattern,
                         next_pattern_ip);
        }
      }
    }

  } else {
    // Pattern is an immediate value to match
    ScopedTemp cond_temp(environment_, "PatternEqualityCond");
    segment_->push_back(Bytecode(Bytecode::TEST_EQUALITY,
                                 cond_temp.GetOperand(),
                                 match_value,
                                 Operand(pattern_desc.value())));
    segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                 cond_temp.GetOperand(),
                                 Operand(next_pattern_ip)));
  }
}

void Compiler::CompileConditional(OzValue conditional,
                                  ExpressionResult* result) {
  CHECK(conditional.label() == "conditional");
  OzValue cases = conditional["cases"];

  const bool is_expr = (result != NULL);
  if (is_expr)
    result->SetupValuePlaceholder("ConditionValue");

  // Bytecode IP of the end of the conditional statement
  Value conditional_end_ip = Variable::New(store_);

  // Bytecode IP of the branch following the one we are compiling
  Value next_case_ip = NULL;

  for (uint i = 1; i <= cases.size(); ++i) {
    if (next_case_ip != NULL)
      // Set the IP of this case in the previous branch.
      Unify(next_case_ip, Value::Integer(segment_->size()));
    next_case_ip = Variable::New(store_);

    OzValue branch = cases[i];
    if (branch.label() == "if") {
      OzValue cond_desc = branch["cond"];
      OzValue then_desc = branch["then"];
      {
        ExpressionResult cond_er(environment_);
        CompileExpression(cond_desc, &cond_er);
        // Jump to the next case if the condition is not satisfied.
        segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                     cond_er.value(),
                                     Operand(next_case_ip)));
      }

      CompileExpression(then_desc, result);

      // Jump to the end of the conditional
      segment_->push_back(Bytecode(Bytecode::BRANCH,
                                   Operand(conditional_end_ip)));

    } else if (branch.label() == "match") {
      OzValue value_desc = branch["value"];
      OzValue match_cases = branch["cases"];
      CHECK(match_cases.IsTuple())
          << "conditional match cases must be a tuple.";

      ExpressionResult match_value_er(environment_);
      CompileExpression(value_desc, &match_value_er);

      // Bytecode IP of the pattern to match after the one being compiled.
      Value next_pattern_ip = NULL;

      // TODO(taton) Many optimizations can be implemented here.
      // Eg. have a single branching/dispatching
      for (uint64 i = 1; i <= match_cases.size(); ++i) {
        if (next_pattern_ip != NULL)
          // Set the IP of this case in the previous branch.
          Unify(next_pattern_ip, Value::Integer(segment_->size()));
        next_pattern_ip = Variable::New(store_);

        OzValue case_desc = match_cases[i];
        CHECK(case_desc.label() == "with");
        OzValue pattern_desc = case_desc["pattern"];
        OzValue then_desc = case_desc["then"];

        Environment::NestedLocalAllocator nested_env(environment_);
        CompilePattern(match_value_er.value(), pattern_desc, next_pattern_ip);

        if (case_desc.HasFeature("cond")) {
          OzValue cond_desc = case_desc["cond"];
          ExpressionResult cond_er(environment_);
          CompileExpression(cond_desc, &cond_er);
          // Jump to the next pattern if the condition is not satisfied.
          segment_->push_back(Bytecode(Bytecode::BRANCH_UNLESS,
                                       cond_er.value(),
                                       Operand(next_pattern_ip)));
        }

        CompileExpression(then_desc, result);

        // Jump to the end of the conditional
        segment_->push_back(Bytecode(Bytecode::BRANCH,
                                     Operand(conditional_end_ip)));
      }

      if (next_pattern_ip != NULL)
        Unify(next_pattern_ip, Value::Integer(segment_->size()));

    } else {
      LOG(FATAL) << "Unknown conditional case kind:\n"
                 << branch.value().ToString();
    }
  }
  if (conditional.HasFeature("else")) {
    if (next_case_ip != NULL) {
      Unify(next_case_ip, Value::Integer(segment_->size()));
      next_case_ip = NULL;
    }
    // TODO: Maybe check an expression always has an else?
    // Or the result value should be initialized to a free variable?
    CompileExpression(conditional["else"], result);
  }

  if (next_case_ip != NULL)
    Unify(next_case_ip, Value::Integer(segment_->size()));
  Unify(conditional_end_ip, Value::Integer(segment_->size()));
}

void Compiler::CompileLocal(OzValue desc, ExpressionResult* result) {
  CHECK(desc.label() == "local");
  CHECK(desc.HasFeature("locals"));
  CHECK(desc.HasFeature("in"));

  Environment::NestedLocalAllocator nested_env(environment_);

  OzValue locals = desc["locals"];
  CHECK(locals.IsTuple())
      << "local locals must be a tuple.";
  for (uint64 i = 1; i <= locals.size(); ++i) {
    OzValue var_desc = locals[i];
    CHECK(var_desc.IsTuple())
        << "local variable descriptor must be a tuple.";
    CHECK_LE(var_desc.size(), 1UL);

    const Symbol local_symbol = (
        environment_->AddLocal(var_desc.label().atom_val()));

    if (var_desc.size() == 1) {
      // Initialize the variable
      ExpressionResult local_er(local_symbol);
      CompileExpression(var_desc[1], &local_er);
    } else {
      segment_->push_back(Bytecode(Bytecode::NEW_VARIABLE,
                                   local_symbol.GetOperand()));
    }
  }

  CompileExpression(desc["in"], result);
}

void Compiler::CompileLoop(OzValue loop) {
  CHECK(loop.label() == "loop");
  CHECK(loop.HasFeature("body"));

  const bool has_range = loop.HasFeature("range");

  unique_ptr<ScopedSymbol> iterator_sym;
  Operand iterator;

  ExpressionResult it_end_er(environment_);

  // Initialize the loop
  if (has_range) {
    OzValue range = loop["range"];

    const string var_name = range["var"].atom_val();
    const Symbol var_symbol = environment_->AddLocal(var_name);
    iterator_sym.reset(new ScopedSymbol(environment_, var_symbol));
    iterator = iterator_sym->operand();

    // Initialize the iterator to 'from'
    ExpressionResult from_er(iterator_sym->symbol());
    CompileExpression(range["from"], &from_er);

    CompileExpression(range["to"], &it_end_er);
  }

  Value loop_start_ip = Value::Integer(segment_->size());
  Value loop_exit_ip = Variable::New(store_);

  // End condition
  if (has_range) {
    ScopedTemp end_cond_temp(environment_, "end loop condition");
    Operand end_cond = end_cond_temp.GetOperand();
    segment_->push_back(Bytecode(Bytecode::TEST_LESS_THAN,
                                 end_cond,
                                 it_end_er.value(),
                                 iterator));
    segment_->push_back(Bytecode(Bytecode::BRANCH_IF,
                                 end_cond,
                                 Operand(loop_exit_ip)));
  }

  CompileExpression(loop["body"], NULL);

  // Update iterator
  if (has_range) {
    segment_->push_back(Bytecode(Bytecode::NUMBER_INT_ADD,
                                 iterator,
                                 iterator,
                                 Operand(Value::Integer(1))));
  }

  // Loop over
  segment_->push_back(Bytecode(Bytecode::BRANCH,
                               Operand(loop_start_ip)));

  Unify(loop_exit_ip, Value::Integer(segment_->size()));
}

void Compiler::CompileSequence(OzValue sequence, ExpressionResult* result) {
  const bool is_expr = (result != NULL);

  CHECK(sequence.label() == "sequence");
  CHECK_EQ(is_expr, sequence.HasFeature("expr"));

  const uint64 nstmts = is_expr ? sequence.size() - 1 : sequence.size();
  for (uint64 i = 1; i <= nstmts; ++i)
    CompileExpression(sequence[i], NULL);

  if (is_expr)
    CompileExpression(sequence["expr"], result);
}

void Compiler::CompileUnify(OzValue desc) {
  CHECK(desc.label() == "unify");
  CHECK_EQ(2UL, desc.size());
  CHECK(desc.IsTuple()) << "unify must be a tuple.";

  ExpressionResult value1_er(environment_);
  CompileExpression(desc[1], &value1_er);

  ExpressionResult value2_er(environment_);
  CompileExpression(desc[2], &value2_er);

  segment_->push_back(Bytecode(Bytecode::UNIFY,
                               value1_er.value(),
                               value2_er.value()));
}

void Compiler::CompileTry(OzValue desc, ExpressionResult* result) {
  const bool is_expr = (result != NULL);

  CHECK(desc.label() == "try");
  CHECK_EQ(2UL, desc.size());
  CHECK(desc.HasFeature("code"));
  OzValue code_desc = desc["code"];

  const bool has_finally = desc.HasFeature("finally");
  const bool has_catch = desc.HasFeature("catch");
  CHECK_NE(has_finally, has_catch)
      << "try() must have exactly one of 'finally' or 'catch'.";

  if (is_expr)
    result->SetupValuePlaceholder("TryResultValue");

  Value handler_ip = Variable::New(store_);
  segment_->push_back(Bytecode(has_finally
                               ? Bytecode::EXN_PUSH_FINALLY
                               : Bytecode::EXN_PUSH_CATCH,
                               Operand(handler_ip)));
  CompileExpression(code_desc, result);
  segment_->push_back(Bytecode(Bytecode::EXN_POP));

  if (has_finally) {
    Unify(handler_ip, Value::Integer(segment_->size()));

    // Save the exception register
    ScopedTemp saved_exn_temp(environment_, "saved exception register");
    Operand saved_exn = saved_exn_temp.GetOperand();
    segment_->push_back(Bytecode(Bytecode::EXN_RESET,
                                 saved_exn));

    CompileExpression(desc["finally"], result);

    // Re-raise the exception, if any
    segment_->push_back(Bytecode(Bytecode::EXN_RERAISE,
                                 saved_exn));

  } else if (has_catch) {
    Value end_ip = Variable::New(store_);
    // TODO: catch handler should map the exception into a variable
    segment_->push_back(Bytecode(Bytecode::BRANCH, Operand(end_ip)));

    Unify(handler_ip, Value::Integer(segment_->size()));
    // A catch handler should look like:
    // match(exn) {
    //   with patt -> do ...
    //   with patt -> do ...
    //   else raise(exn)
    // }

    CompileExpression(desc["catch"], result);

    Unify(end_ip, Value::Integer(segment_->size()));

  } else {
    LOG(FATAL) << "Dead code";
  }
}

void Compiler::CompileRaise(OzValue desc) {
  CHECK(desc.label() == "raise");
  CHECK(desc.IsTuple());
  CHECK_EQ(1UL, desc.size());
  ExpressionResult exn_er(environment_);
  CompileExpression(desc[1], &exn_er);
  segment_->push_back(Bytecode(Bytecode::EXN_RAISE, exn_er.value()));
}

void Compiler::CompileNewClosure(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  CHECK(desc.label() == "proc");
  Compiler nested_compiler(store_, environment_);

  vector<string> env;
  Closure* const closure = nested_compiler.CompileProcedure(desc, &env);
  if (env.empty()) {
    // Already a concrete procedure
    result->SetValue(Operand(closure));
    return;
  }

  // Setup the environment of the closure
  ScopedTemp env_temp(environment_, "Closure environment");
  Operand env_op = env_temp.GetOperand();
  segment_->push_back(Bytecode(Bytecode::NEW_ARRAY,
                               env_op,
                               Operand(Value::Integer(env.size())),
                               Operand(KAtomEmpty())));  // dummy value
  for (uint i = 0; i < env.size(); ++i) {
    const Symbol& symbol = environment_->Get(env[i]);
    segment_->push_back(Bytecode(Bytecode::ASSIGN_ARRAY,
                                 env_op,
                                 Operand(Value::Integer(i)),
                                 symbol.GetOperand()));
  }

  result->SetupValuePlaceholder("NewProc");
  segment_->push_back(Bytecode(Bytecode::NEW_PROC,
                               result->value(),
                               Operand(closure),
                               env_op));
}

void Compiler::CompileNewVariable(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  CHECK(desc.label() == "free");
  // TODO: Eventually, new variables could be tagged with a type.
  result->SetupValuePlaceholder("NewVariable");
  segment_->push_back(Bytecode(Bytecode::NEW_VARIABLE, result->value()));
}

void Compiler::CompileNewCell(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  CHECK(desc.label() == "cell");
  CHECK_EQ(1UL, desc.size());

  ExpressionResult init_value_er(environment_);
  CompileExpression(desc[1], &init_value_er);

  result->SetupValuePlaceholder("NewCell");
  segment_->push_back(Bytecode(Bytecode::NEW_CELL,
                               result->value(),
                               init_value_er.value()));
}

void Compiler::CompileNewArray(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  CHECK(desc.label() == "array");

  ExpressionResult size_er(environment_);
  CompileExpression(desc["size"], &size_er);

  ExpressionResult init_value_er(environment_);
  CompileExpression(desc["init"], &init_value_er);

  result->SetupValuePlaceholder("NewArray");
  segment_->push_back(Bytecode(Bytecode::NEW_ARRAY,
                               result->value(),
                               size_er.value(),
                               init_value_er.value()));
}

void Compiler::CompileNewRecord(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  CHECK(desc.label() == "record");
  CHECK_EQ(2UL, desc.size());

  ExpressionResult label_er(environment_);
  CompileExpression(desc["label"], &label_er);

  ExpressionResult arity_er(environment_);
  if (desc.HasFeature("arity"))
    CompileExpression(desc["arity"], &arity_er);
  else if (desc.HasFeature("features"))
    arity_er.SetValue(Operand(desc["features"].arity()));
  else
    LOG(FATAL) << "record() requires either 'arity' or 'features'.";

  result->SetupValuePlaceholder("NewRecord");
  segment_->push_back(Bytecode(Bytecode::NEW_RECORD,
                               result->value(),
                               arity_er.value(),
                               label_er.value()));

  if (desc.HasFeature("features")) {
    // Set record values
    OzValue features = desc["features"];
    unique_ptr<Value::ItemIterator> it_deleter(
        features.value().RecordIterItems());
    for (Value::ItemIterator& it = *it_deleter; !it.at_end(); ++it) {
      Operand feature_label((*it).first);
      ExpressionResult value_er(environment_);
      CompileExpression((*it).second, &value_er);
      segment_->push_back(Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                                   result->value(),
                                   feature_label,
                                   value_er.value()));
    }
  }
}

void Compiler::CompileExpression(OzValue desc, ExpressionResult* result) {
  CompileExpressionEx(desc, result);

  if ((result != NULL) && !result->into().invalid()) {
    // Ensure the "into" semantic of ExpressionResult
    const Operand& expected = result->into();
    CHECK_EQ(Operand::REGISTER, expected.type);
    const Operand& actual = result->value();
    if ((actual.type != Operand::REGISTER)
        || (expected.reg.type != actual.reg.type)
        || (expected.reg.index != actual.reg.index)) {
      segment_->push_back(Bytecode(Bytecode::LOAD,
                                   expected,
                                   actual));
      result->SetValue(expected);
    }
  }
}

void Compiler::CompileExpressionEx(OzValue desc, ExpressionResult* result) {
  // Returns immediately if the descriptor is an immediate value
  // such as a free variable, etc...
  if (!desc.IsRecord()) {
    CHECK_NOTNULL(result)->SetValue(Operand(desc.value()));
    return;
  }

  OzValue label = desc.label();
  if (label == "skip") {  // Statements without parameters
    CHECK(result == NULL);
    CompileSkip(desc);

  } else if (desc.size() == 0) {
    CHECK_NOTNULL(result)->SetValue(Operand(desc.value()));

  } else if (label == "loop") {  // Statements only
    CHECK(result == NULL);
    CompileLoop(desc);

  } else if (label == "raise") {
    CompileRaise(desc);

  } else if (label == "call") {  // Statements or expressions
    CompileCall(desc, result);

  } else if (label == "conditional") {
    CompileConditional(desc, result);

  } else if (label == "local") {
    CompileLocal(desc, result);

  } else if (label == "sequence") {
    CompileSequence(desc, result);

  } else if (label == "try") {
    CompileTry(desc, result);

  } else if (label == "proc") {  // Expressions only
    CompileNewClosure(desc, CHECK_NOTNULL(result));

  } else if (label == "free") {
    CompileNewVariable(desc, CHECK_NOTNULL(result));

  } else if (label == "cell") {
    CompileNewCell(desc, CHECK_NOTNULL(result));

  } else if (label == "array") {
    CompileNewArray(desc, CHECK_NOTNULL(result));

  } else if (label == "record") {
    CompileNewRecord(desc, CHECK_NOTNULL(result));

  } else if (label == "thread") {
    LOG(FATAL) << "Not implemented";

  } else if (desc.label() == "var") {
    // Reference to a variable
    CHECK(desc.IsTuple()) << "variable reference must be a tuple.";
    CHECK_EQ(1UL, desc.size());
    const string& var_name = desc[1].atom_val();
    const Symbol& symbol = environment_->Get(var_name);
    CHECK_NOTNULL(result)->SetValue(symbol.GetOperand());

  } else if (desc.label() == "val") {
    // Wrapped immediate value
    CHECK(desc.IsTuple()) << "Immediate ";
    CHECK_EQ(1UL, desc.size());
    CHECK_NOTNULL(result)->SetValue(Operand(desc[1].value()));

  } else {  // Record/List constructur
    CompileValueConstructor(desc, CHECK_NOTNULL(result));
  }
}

void Compiler::CompileValueConstructor(OzValue desc, ExpressionResult* result) {
  CHECK_NOTNULL(result);
  result->SetupValuePlaceholder("ValueConstructor");

  if (desc.IsRecord()) {
    segment_->push_back(Bytecode(Bytecode::NEW_RECORD,
                                 result->value(),
                                 Operand(desc.arity()),
                                 Operand(desc.label().value())));

    unique_ptr<Value::ItemIterator> it_deleter(desc.value().RecordIterItems());
    for (Value::ItemIterator& it = *it_deleter; !it.at_end(); ++it) {
      Operand feature_label((*it).first);
      ExpressionResult value_er(environment_);
      CompileExpression((*it).second, &value_er);
      segment_->push_back(Bytecode(Bytecode::UNIFY_RECORD_FIELD,
                                   result->value(),
                                   feature_label,
                                   value_er.value()));
    }

  } else {
    LOG(FATAL) << "Don't know how to compile value constructor: " << desc;
  }
}

}  // namespace store
