#ifndef STORE_BYTECODE_H_
#define STORE_BYTECODE_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "base/stl-util.h"

namespace store {

// Bytecode representation
struct Bytecode {
 public:
  enum OpcodeType {
    NO_OPERATION = 0,

    LOAD,
    UNIFY,
    TRY_UNIFY,
    UNIFY_RECORD_FIELD,

    // Control-flow
    BRANCH,
    BRANCH_IF,
    BRANCH_UNLESS,
    BRANCH_SWITCH_LITERAL,

    CALL,
    CALL_TAIL,
    CALL_NATIVE,
    RETURN,

    // Exception handling
    EXN_PUSH_CATCH,
    EXN_PUSH_FINALLY,
    EXN_POP,
    EXN_RAISE,
    EXN_RESET,
    EXN_RERAISE,

    // Constructors
    NEW_VARIABLE,
    NEW_NAME,
    NEW_CELL,
    NEW_ARRAY,
    NEW_ARITY,
    NEW_LIST,
    NEW_TUPLE,
    NEW_RECORD,

    NEW_PROC,
    NEW_THREAD,

    // Accessors
    GET_VALUE_TYPE,

    ACCESS_CELL,
    ACCESS_ARRAY,
    ACCESS_RECORD,

    ACCESS_RECORD_LABEL,
    ACCESS_RECORD_ARITY,
    ACCESS_OPEN_RECORD_ARITY,

    // Mutations
    ASSIGN_CELL,
    ASSIGN_ARRAY,

    // Predicates
    TEST_IS_DET,
    TEST_IS_RECORD,

    TEST_EQUALITY,  // deep value equality

    // Literals only:
    TEST_LESS_THAN,
    TEST_LESS_OR_EQUAL,

    TEST_ARITY_EXTENDS,

    NUMBER_INT_INVERSE,
    NUMBER_INT_ADD,
    NUMBER_INT_SUBTRACT,
    NUMBER_INT_MULTIPLY,
    NUMBER_INT_DIVIDE,

    NUMBER_BOOL_NEGATE,
    NUMBER_BOOL_AND_THEN,  // lazy
    NUMBER_BOOL_OR_ELSE,  // lazy
    NUMBER_BOOL_XOR,

    OPCODE_TYPE_COUNT,
  };

  Bytecode(OpcodeType type, Operand op1, Operand op2, Operand op3)
      : opcode(type), operand1(op1), operand2(op2), operand3(op3) {
    CHECK_GE(opcode, 0);
    CHECK_LT(opcode, OPCODE_TYPE_COUNT);
  }

  Bytecode(OpcodeType type, Operand op1, Operand op2)
      : opcode(type), operand1(op1), operand2(op2) {
    CHECK_GE(opcode, 0);
    CHECK_LT(opcode, OPCODE_TYPE_COUNT);
  }

  Bytecode(OpcodeType type, Operand op1)
      : opcode(type), operand1(op1) {
    CHECK_GE(opcode, 0);
    CHECK_LT(opcode, OPCODE_TYPE_COUNT);
  }

  explicit Bytecode(OpcodeType type)
      : opcode(type) {
    CHECK_GE(opcode, 0);
    CHECK_LT(opcode, OPCODE_TYPE_COUNT);
  }

  Bytecode()
      : opcode(NO_OPERATION) {
  }

  void ToASCII(ToASCIIContext* context, string* repr);

  // For debugging.
  string GetOpcodeName() const;
  string ToString() const;

  OpcodeType opcode;
  Operand operand1;
  Operand operand2;
  Operand operand3;
};

// Specification of the opcode mnemonics.
struct OpcodeSpec {
  OpcodeSpec() {}
  OpcodeSpec(const char* name_,
             Bytecode::OpcodeType opcode_,
             const char* operand1_ = NULL,
             const char* operand2_ = NULL,
             const char* operand3_ = NULL)
      : name(name_),
        opcode(opcode_) {
    if (operand1_ == NULL) return; else
      params.push_back(operand1_);
    if (operand2_ == NULL) return; else
      params.push_back(operand2_);
    if (operand3_ == NULL) return; else
      params.push_back(operand3_);
  }

  // Name of the mnemonic
  const char* name;

  // Opcode the mnemonic maps to
  Bytecode::OpcodeType opcode;

  // Name of the parameters in the mnemonic
  vector<string> params;
};

class OpcodeSpecMap : public UnorderedMap<string, OpcodeSpec> {
 public:
  OpcodeSpecMap();
};
extern const OpcodeSpecMap kOpcodeSpecs;

}  // namespace store

#endif  // STORE_BYTECODE_H_
