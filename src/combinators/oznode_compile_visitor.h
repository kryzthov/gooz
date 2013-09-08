#ifndef COMBINATORS_OZNODE_COMPILE_VISITOR_H_
#define COMBINATORS_OZNODE_COMPILE_VISITOR_H_

#include <memory>
#include <string>
#include <vector>
using std::shared_ptr;
using std::string;
using std::vector;

#include "base/stl-util.h"
#include "combinators/oznode.h"
#include "combinators/oznode_dump_visitor.h"

#include "store/environment.h"
#include "store/store.h"
#include "store/values.h"

using namespace store;

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

// Representation of the outcome of a compiled statement or expression.
// In case of a compiled expression, this lets the caller determine whether
// the result should be placed into a given pre-determined register.
// If no pre-determined destination location is defined, the compiler
// is free to chose where to put the expression value.
// In particular, if the expression value is immediate, this may directly
// return the Oz value.
class ExpressionResult {
 public:
  // Constructor for a statement (ie. no result).
  ExpressionResult()
      : statement_(true),
        temp_(nullptr) {
  }

  // Use this constructor when the result value is either immediate or will
  // be placed into a temporary register.
  ExpressionResult(Environment* env)
      : statement_(false),
        temp_(new ScopedTemp(env)) {
  }

  // Use this constructor when the result value must be stored in the specified
  // symbol. No additional temporary register will be used.
  ExpressionResult(const Symbol& into)
      : statement_(false),
        value_(into.GetOperand()),
        into_(into.GetOperand()),
        temp_(nullptr) {
    CHECK_EQ(into_.type, Operand::REGISTER);
  }

  // @return The operand for the result value.
  const Operand& value() { return value_; }

  // Sets the explicit operand for the result value.
  //
  // This may differ from the target location specified in into().
  // The user of the result must check the actual result in value()
  // and potentially add an extra LOAD from value() to into().
  //
  // @param value Operand for the result value.
  void SetValue(Operand value) {
    CHECK(!statement_);
    value_ = value;
  }

  // Validates or creates a place-holder for the result value.
  //
  // If no result place-holder symbol has already set, this allocates a
  // temporary register.
  //
  // @returns the place-holder operand.
  const Operand& SetupValuePlaceholder(const string& description = "") {
    CHECK(!statement_);
    if (into_.invalid()) {
      CHECK_NOTNULL(temp_.get());
      if (!temp_->valid())
        temp_->Allocate(description);
      into_ = temp_->GetOperand();
    }
    value_ = into_;
    return value_;
  }

  // @returns the operand to write the result into, if any.
  //     If specified (ie. not invalid), the result of the expression must
  //     be placed into this location (register).
  const Operand& into() const { return into_; }

  // @returns true if no result is expected.
  bool statement() const { return statement_; }

 private:
  // True means no result.
  bool statement_;

  // The operand representing the resulting value.
  Operand value_;

  // When specified, the resulting value should ideally be placed in there.
  Operand into_;

  // Optional temporary symbol used to store the resulting value.
  unique_ptr<ScopedTemp> temp_;

  DISALLOW_COPY_AND_ASSIGN(ExpressionResult);
};

// -----------------------------------------------------------------------------

// Visitor that compiles an AST into Oz values, and in particular,
// compiles functors, functions and procedures into closures (bytecode).
//
// Any compilation produces a procedure, ie. occurs in the context of a
// procedure.
// The top-level defines an implicit procedure with no parameters.
class CompileVisitor : public AbstractOzNodeVisitor {
 public:

  // Initializes a new visitor.
  //
  // @param store Creates values in this Oz store.
  CompileVisitor(store::Store* store)
      : store_(CHECK_NOTNULL(store)),
        environment_(new Environment(nullptr)),
        declaring_(false) {
  }

  // Destructor.
  virtual ~CompileVisitor() {}

  // Main entry point of the visitor.
  //
  // @param node AST to compile to Oz values.
  // @returns the last compiled value.
  store::Value Compile(AbstractOzNode* node) {
    LOG(INFO) << "Compiling:\n" << *node;
    node->AcceptVisitor(this);
    return top_level_;
  }

  virtual void Visit(OzNodeError* node) {
    LOG(FATAL) << "AST error: " << node->error;
  }

  virtual void Visit(OzNode* node);
  virtual void Visit(OzNodeBinaryOp* node);
  virtual void Visit(OzNodeCall* node);
  virtual void Visit(OzNodeClass* node);
  virtual void Visit(OzNodeCond* node);
  virtual void Visit(OzNodeCondBranch* node);
  virtual void Visit(OzNodeForLoop* node);
  virtual void Visit(OzNodeFunctor* node);
  virtual void Visit(OzNodeGeneric* node);
  virtual void Visit(OzNodeList* node);
  virtual void Visit(OzNodeLocal* node);
  virtual void Visit(OzNodeLock* node);
  virtual void Visit(OzNodeLoop* node);
  virtual void Visit(OzNodeNaryOp* node);
  virtual void Visit(OzNodePatternBranch* node);
  virtual void Visit(OzNodePatternMatch* node);
  virtual void Visit(OzNodeProc* node);
  virtual void Visit(OzNodeRaise* node);
  virtual void Visit(OzNodeRecord* node);
  virtual void Visit(OzNodeSequence* node);
  virtual void Visit(OzNodeThread* node);
  virtual void Visit(OzNodeTry* node);
  virtual void Visit(OzNodeUnaryOp* node);
  virtual void Visit(OzNodeVar* node);

  // Returns the compiled top-level procedure.
  Closure* top_level() const { return CHECK_NOTNULL(top_level_); }

  // ---------------------------------------------------------------------------

 private:
  void CompileUnify(OzNodeNaryOp* node);
  void CompileTupleCons(OzNodeNaryOp* node);
  void CompileMulOrAdd(OzNodeNaryOp* node);

  // Pushes a nested environment on the environment stack.
  // Automatically pops the environment when going out of scope.
  class NestedEnvironment {
   public:
    NestedEnvironment(CompileVisitor* compiler)
        : compiler_(CHECK_NOTNULL(compiler)),
          parent_env_(compiler_->environment_),
          env_(parent_env_) {
      compiler_->environment_ = &env_;
    }

    ~NestedEnvironment() {
      compiler_->environment_ = parent_env_;
    }

   private:
    CompileVisitor* const compiler_;
    store::Environment* const parent_env_;
    store::Environment env_;
  };

  // ---------------------------------------------------------------------------

 private:
  // Oz store where to create values.
  store::Store* const store_;

  // This is the current environment we are compiling against.
  // Before the initial call to CompileProcedure, it contains the global
  // environment.
  // As we go deeper in the AST, nested environments will be stacked and popped.
  store::Environment* environment_;

  // Bytecode generated by the many compiler methods.
  // Each method appends to this segment.
  shared_ptr<vector<store::Bytecode> > segment_;

  // Specification of the expected result of an expression being compiled.
  shared_ptr<ExpressionResult> result_;

  // When true, new variables may be declared.
  // Places where variables can be declared are:
  //  - left-operand in a unify
  //  - patterns in case branches?
  //  - other?
  bool declaring_;

  // When compiling conditionals, this is the next branch instruction pointer,
  // as an initially undetermined value.
  Value cond_next_branch_ip_;

  // Future for the instruction pointer at the end of a conditional.
  Value cond_end_ip_;

  // Compiled top-level closure.
  Closure* top_level_;

  DISALLOW_COPY_AND_ASSIGN(CompileVisitor);
};

store::Value Compile(const string& code, store::Store* store);

}}  // namespace combinators::oz

#endif  // COMBINATORS_OZNODE_COMPILE_VISITOR_H_
