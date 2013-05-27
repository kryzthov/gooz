
#ifndef STORE_CODE_H_
#define STORE_CODE_H_

#include <map>
#include <string>
#include <vector>

namespace store {

// A variable is a value placeholder.
// Since the value is not known at generation time, the variable is always
// associated with the type of the values suitable for this variable.
class Variable {
  // The unescaped name of the variable.
  const string name;

  // The valid value types for this variable.
  Value* type;
};

// This is the code representation of a value.
// The value is either immediate or a reference to a variable.
struct CodeValue {
  // An immediate value. NULL means the value is a variable.
  Value* value;

  // A variable.
  string variable;
};

// The environment contains all the variables available to a piece of code.
class Environment {
 public:
  Environment()
      : parent_(NULL) {
  }

  explicit Environment(const Environment* parent)
      : parent_(parent) {
  }

 private:
  const Environment* const parent_;
  map<string, Variable*> variables_;
};

// -----------------------------------------------------------------------------

// Ancestor class of any code representation.
class AbstractCode {
 public:
};

// A sequence of code operations.
class Local : public AbstractCode {
 public:
  vector<AbstractCode*> code_;
};

class Apply : public AbstractCode {
 public:
  CodeValue* procedure;
  vector<CodeValue*> parameters;
};

// A conditional control-flow.
class Conditional : public AbstractCode {
 public:

  // Representation of a simple condition statement.
  class Condition : public ConditionBase {
   public:
    // The condition value placeholder.
    Variable* cond_value;

    // The code evaluating the condition.
    AbstractCode* cond_code;

    // The code to execute when the condition evaluates to true.
    AbstractCode* code;
  };

  // Representation of a pattern matching statement.
  // A pattern may be associated with a condition.
  class PatternMatch : public Condition {
   public:
    Pattern* pattern;
  };

  // Representation of a pattern matching block.
  class PatternMatching {
   public:
    // The value to match.
    Variable* value;

    // The code to execute to retrieve the value to match.
    AbstractCode* value_code;

    // The patterns to match against, in order.
    vector<PatternMatch*> patterns;
  };

  // The conditional statements, in the order they have to be tested.
  // The first one to be satisfied will be executed, and this one only.
  vector<AbstractCondition*> conditions_;
};


// Looping control-flow.
class Loop : public AbstractCode {
 public:
  // The variable holding the values we are iterating over.
  Variable* iterator;

  // The code to execute for all iteration.
  AbstractCode* code;
};

}  // namespace store

#endif  // STORE_CODE_H_
