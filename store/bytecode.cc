#include "base/macros.h"
#include "store/values.h"

namespace store {

static const OpcodeSpec kOpcodeSpecTable[] = {
  OpcodeSpec("nop", Bytecode::NO_OPERATION),

  OpcodeSpec("load", Bytecode::LOAD, "dest", "src"),
  OpcodeSpec("unify", Bytecode::UNIFY, "value1", "value2"),
  OpcodeSpec("try_unify", Bytecode::TRY_UNIFY, "value1", "value2", "success"),
  OpcodeSpec("unify_record_field", Bytecode::UNIFY_RECORD_FIELD,
             "record", "feature", "value"),

  OpcodeSpec("branch", Bytecode::BRANCH, "to"),
  OpcodeSpec("branch_if", Bytecode::BRANCH_IF, "cond", "to"),
  OpcodeSpec("branch_unless", Bytecode::BRANCH_UNLESS, "cond", "to"),
  OpcodeSpec("branch_switch_literal", Bytecode::BRANCH_SWITCH_LITERAL,
             "value", "branches"),

  OpcodeSpec("call", Bytecode::CALL, "proc", "params"),
  OpcodeSpec("call_tail", Bytecode::CALL_TAIL, "proc", "params"),
  OpcodeSpec("call_native", Bytecode::CALL_NATIVE, "name", "params"),
  OpcodeSpec("return", Bytecode::RETURN),

  OpcodeSpec("exn_push_catch", Bytecode::EXN_PUSH_CATCH, "to"),
  OpcodeSpec("exn_push_finally", Bytecode::EXN_PUSH_FINALLY, "to"),
  OpcodeSpec("exn_pop", Bytecode::EXN_POP),
  OpcodeSpec("exn_raise", Bytecode::EXN_RAISE, "exn"),
  OpcodeSpec("exn_reset", Bytecode::EXN_RESET, "to"),
  OpcodeSpec("exn_reraise", Bytecode::EXN_RERAISE, "exn"),

  // TODO: Implement NEW_VARIABLE(in, type)
  OpcodeSpec("var", Bytecode::NEW_VARIABLE, "in"),
  OpcodeSpec("name", Bytecode::NEW_NAME, "in"),
  OpcodeSpec("cell", Bytecode::NEW_CELL, "in", "ref"),
  OpcodeSpec("array", Bytecode::NEW_ARRAY, "in", "size", "init"),
  OpcodeSpec("arity", Bytecode::NEW_ARITY, "in", "features"),
  OpcodeSpec("list", Bytecode::NEW_LIST, "in", "head", "tail"),
  OpcodeSpec("tuple", Bytecode::NEW_TUPLE, "in", "size", "label"),
  OpcodeSpec("record", Bytecode::NEW_RECORD, "in", "arity", "label"),

  OpcodeSpec("closure", Bytecode::NEW_PROC, "in", "proc", "env"),
  OpcodeSpec("thread", Bytecode::NEW_THREAD, "in", "proc", "params"),

  OpcodeSpec("get_value_type", Bytecode::GET_VALUE_TYPE, "in", "value"),

  OpcodeSpec("access_cell", Bytecode::ACCESS_CELL, "in", "cell"),
  OpcodeSpec("access_array", Bytecode::ACCESS_ARRAY, "in", "array", "index"),
  OpcodeSpec("access_record", Bytecode::ACCESS_RECORD,
             "in", "record", "feature"),
  OpcodeSpec("access_record_label", Bytecode::ACCESS_RECORD_LABEL,
             "in", "record"),
  OpcodeSpec("access_record_arity", Bytecode::ACCESS_RECORD_ARITY,
             "in", "record"),
  OpcodeSpec("access_open_record_arity", Bytecode::ACCESS_OPEN_RECORD_ARITY,
             "in", "record"),

  // TODO: assign_cell should return the former cell value
  OpcodeSpec("assign_cell", Bytecode::ASSIGN_CELL, "cell", "value"),
  // TODO: assign_array should return the former value
  OpcodeSpec("assign_array", Bytecode::ASSIGN_ARRAY, "array", "index", "value"),

  OpcodeSpec("test_is_det", Bytecode::TEST_IS_DET,
             "in", "value"),
  OpcodeSpec("test_is_record", Bytecode::TEST_IS_RECORD,
             "in", "value"),

  // Deep value equality:
  OpcodeSpec("test_equality", Bytecode::TEST_EQUALITY,
             "in", "value1", "value2"),

  // Literal comparison:
  OpcodeSpec("test_less_than", Bytecode::TEST_LESS_THAN,
             "in", "value1", "value2"),
  OpcodeSpec("test_less_or_equal", Bytecode::TEST_LESS_OR_EQUAL,
             "in", "value1", "value2"),

  OpcodeSpec("test_arity_extends", Bytecode::TEST_ARITY_EXTENDS,
             "in", "super", "sub"),

  // Integer operations:
  OpcodeSpec("number_int_inverse",
             Bytecode::NUMBER_INT_INVERSE,
             "in", "int"),
  OpcodeSpec("number_int_add",
             Bytecode::NUMBER_INT_ADD,
             "in", "int1", "int2"),
  OpcodeSpec("number_int_subtract",
             Bytecode::NUMBER_INT_SUBTRACT,
             "in", "int1", "int2"),
  OpcodeSpec("number_int_multiply",
             Bytecode::NUMBER_INT_MULTIPLY,
             "in", "int1", "int2"),
  OpcodeSpec("number_int_divide",
             Bytecode::NUMBER_INT_DIVIDE,
             "in", "int1", "int2"),

  OpcodeSpec("number_bool_negate",
             Bytecode::NUMBER_BOOL_NEGATE,
             "in", "bool"),
  OpcodeSpec("number_and_then",
             Bytecode::NUMBER_BOOL_AND_THEN,
             "in", "bool1", "bool2"),
  OpcodeSpec("number_bool_or_else",
             Bytecode::NUMBER_BOOL_OR_ELSE,
             "in", "bool1", "bool2"),
  OpcodeSpec("number_bool_xor",
             Bytecode::NUMBER_BOOL_XOR,
             "in", "bool1", "bool2"),
};

OpcodeSpecMap::OpcodeSpecMap() {
  CHECK_EQ(static_cast<uint64>(Bytecode::OPCODE_TYPE_COUNT),
           ArraySize(kOpcodeSpecTable));
  for (uint i = 0; i < ArraySize(kOpcodeSpecTable); ++i) {
    const OpcodeSpec& spec = kOpcodeSpecTable[i];
    CHECK_EQ(Bytecode::OpcodeType(i), spec.opcode);
    (*this)[spec.name] = spec;
  }
}

const OpcodeSpecMap kOpcodeSpecs;

void Bytecode::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  CHECK_GE(opcode, 0);
  CHECK_LT(opcode, Bytecode::OPCODE_TYPE_COUNT);
  const OpcodeSpec& spec = kOpcodeSpecTable[opcode];
  repr->append(spec.name);
  repr->append("(");
  const Operand* operands[] = { &operand1, &operand2, &operand3 };
  for (uint i = 0; i < spec.params.size(); ++i) {
    const Operand& operand = *operands[i];
    if (i > 0) repr->append(" ");
    repr->append(spec.params[i]);
    repr->append(":");
    if (operand.type == Operand::IMMEDIATE)
      context->Encode(operand.value, repr);
    else
      repr->append(DebugString(operand));
  }
  repr->append(")");
}

string Bytecode::GetOpcodeName() const {
  CHECK_GE(opcode, 0);
  CHECK_LT(opcode, Bytecode::OPCODE_TYPE_COUNT);
  const OpcodeSpec& spec = kOpcodeSpecTable[opcode];
  return spec.name;
}

string Bytecode::ToString() const {
  CHECK_GE(opcode, 0);
  CHECK_LT(opcode, Bytecode::OPCODE_TYPE_COUNT);
  const OpcodeSpec& spec = kOpcodeSpecTable[opcode];
  string str = spec.name;
  str.append("(");
  const Operand* operands[] = { &operand1, &operand2, &operand3 };
  for (uint i = 0; i < spec.params.size(); ++i) {
    if (i > 0) str.append(" ");
    str.append(spec.params[i]);
    str.append(":");
    str.append(DebugString(*operands[i]));
  }
  str.append(")");
  return str;
}

}  // namespace store
