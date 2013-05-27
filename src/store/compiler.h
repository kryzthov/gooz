// Compiles Oz code description into bytecode.

#ifndef STORE_COMPILER_H_
#define STORE_COMPILER_H_

#include <memory>
#include <vector>
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

#include "store/environment.h"

#include "store/values.h"
#include "store/ozvalue.h"

namespace store {

// -----------------------------------------------------------------------------

// A placeholder for the operand representing the value computed by a
// compiled expression.
class ExpressionResult {
 public:
  // Use this constructor when the result value may be immediate or go
  // in a temporary register (lazily allocated).
  ExpressionResult(Environment* env)
      : temp_(new ScopedTemp(env)) {
  }

  // Use this constructor when the result value must be stored in the specified
  // symbol. No additional temporary register will be used.
  ExpressionResult(const Symbol& into)
      : value_(into.GetOperand()),
        into_(into.GetOperand()),
        temp_(nullptr) {
  }

  // @return The operand for the result value.
  const Operand& value() { return value_; }

  // Sets the explicit operand for the result value.
  void SetValue(Operand value) {
    value_ = value;
  }

  // Forces the result value to be stored into a value place-holder.
  void SetupValuePlaceholder(const string& description = "") {
    if (into_.invalid()) {
      CHECK_NOTNULL(temp_.get());
      if (!temp_->valid())
        temp_->Allocate(description);
      into_ = temp_->GetOperand();
    }
    value_ = into_;
  }

  const Operand& into() const { return into_; }

 private:
  // The operand representing the resulting value.
  Operand value_;

  // When specified, the resulting value must eventually be stored in there.
  Operand into_;

  // Optional temporary symbol used to store the resulting value.
  unique_ptr<ScopedTemp> temp_;
};

// -----------------------------------------------------------------------------

// Compiles Oz code descriptions (AST).
// The Compiler object compiles a single procedure.
class Compiler {
 public:
  // Pushes a nested environment on the environment stack.
  // Automatically pops the environment when going out of scope.
  class NestedEnvironment {
   public:
    NestedEnvironment(Compiler* compiler)
        : compiler_(CHECK_NOTNULL(compiler)),
          parent_env_(compiler_->environment_),
          env_(parent_env_) {
      compiler_->environment_ = &env_;
    }

    ~NestedEnvironment() {
      compiler_->environment_ = parent_env_;
    }

   private:
    Compiler* const compiler_;
    Environment* const parent_env_;
    Environment env_;
  };

  // ---------------------------------------------------------------------------

  // Initializes the compiler with the given environment.
  // @param store Store to build values into.
  // @param environment Root environment.
  //     Does not take ownership.
  Compiler(Store* store, Environment* environment);

  // ---------------------------------------------------------------------------

  // Compiles a procedure description into a closure.
  // This method is the compiler entry point.
  //
  // @param desc The procedure description
  // @param env Returns the names of the environment symbols.
  // @returns The closure value.
  Closure* CompileProcedure(OzValue desc, vector<string>* env);

  // Compiles an expression.
  // Dispatches to the appropriate CompileXXX method.
  //
  // @params desc The expression AST.
  // @param result Optional result value placeholder.
  //     NULL means no result value (statement).
  void CompileExpression(OzValue desc, ExpressionResult* result);
  void CompileExpressionEx(OzValue desc, ExpressionResult* result);

  // ---------------------------------------------------------------------------

  // Compiles a skip statement.
  void CompileSkip(OzValue desc);

  // Compiles a loop statement.
  void CompileLoop(OzValue desc);

  // Compiles a procedure call.
  // Operates both as a statement or as an expression.
  //
  // call(
  //   {
  //    | proc:<expr:closure>
  //    | native:<expr:atom>
  //   }
  //   params: tuple( { <expr> | 'returned' }* )
  // )
  //
  // @param desc The procedure call AST.
  // @param temp Specifies or returns an optional temporary register.
  //     Specify NULL to compile this as a statement.
  // @returns The operand containing the expression value.
  //     Invalid for a statement.
  void CompileCall(OzValue desc, ExpressionResult* result);

  // Compiles a conditional control-flow.
  // Operates both as a statement or as an expression.
  //
  // conditional(
  //   cases: tuple(
  //     {
  //      | if(cond:<expr:bool> then:<expr-or-stmt>)
  //      | match(
  //          value:<expr>
  //          cases({ with(pattern:<pattern>
  //                       { cond:<expr:bool> }?
  //                       then:<expr-or-stmt>) }*
  //          )
  //        )
  //     }*
  //   )
  //   else: <stmt-or-expr>
  // )
  //
  // @param desc The conditional AST.
  // @param temp Specifies or returns an optional temporary register.
  //     Specify NULL to compile this as a statement.
  // @returns The operand containing the expression value.
  //     Invalid for a statement.
  void CompileConditional(OzValue desc, ExpressionResult* result);

  // Compiles a local block (allows declaring new local variables).
  // Operates both as a statement or as an expression.
  //
  // local(
  //   locals:tuple(var1 var2(<expr>) ...)
  //   in: <stmt-or-expr>
  // )
  //
  // @param desc The local AST.
  // @param temp Specifies or returns an optional temporary register.
  //     Specify NULL to compile this as a statememt.
  // @returns The operand containing the expression value.
  //     Invalid for a statement.
  void CompileLocal(OzValue desc, ExpressionResult* result);

  // sequence( <stmt> <stmt> ... { expr:<expr> }? )
  void CompileSequence(OzValue desc, ExpressionResult* result);

  void CompileUnify(OzValue desc);

  // Compiles a try/finally flow-control.
  //     try(code: <stmt> finally: <stmt>)
  // which executes 'code' and then 'finally'.
  // When an exception occurs during 'code', the control-flow jumps to
  // 'finally' with the exception register set.
  void CompileTry(OzValue desc, ExpressionResult* result);
  void CompileRaise(OzValue desc);


  // Compiles a nested procedure, ie. a closure.
  //
  // @param desc The nested procedure AST.
  // @param temp Specifies or returns a temporary the value is put into.
  //     Must be non NULL.
  // @returns The operand containing the value of the closure.
  void CompileNewClosure(OzValue desc, ExpressionResult* result);
  void CompileNewVariable(OzValue desc, ExpressionResult* result);
  void CompileNewCell(OzValue desc, ExpressionResult* result);
  void CompileNewArray(OzValue desc, ExpressionResult* result);
  void CompileNewRecord(OzValue desc, ExpressionResult* result);
  void CompileValueConstructor(OzValue desc, ExpressionResult* result);

  void CompilePattern(const Operand& match_value,
                      OzValue pattern_desc,
                      Value next_pattern_ip);

 private:
  // The store to create values into.
  Store* const store_;

  // This is the current environment we are compiling against.
  // Before the initial call to CompileProcedure, it contains the global
  // environment.
  // As we go deeper in the AST, nested environments will be stacked and popped.
  Environment* environment_;

  // Bytecode generated by the many compiler methods.
  // Each method appends to this segment.
  shared_ptr<vector<Bytecode> > segment_;
};

}  // namespace store

#endif  // STORE_COMPILER_H_
