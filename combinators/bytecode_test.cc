// Tests for the bytecode parser combinators.
#include "combinators/bytecode.h"

#include <gtest/gtest.h>

#include "base/stl-util.h"
#include "store/store.h"


using store::Bytecode;
using store::Operand;
using store::Register;
using store::StaticStore;


namespace combinators {

const int64 kStoreSize = 1024 * 1024;

TEST(Bytecode, Register) {
  {
    const string text = "l15 ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::LOCAL, parser.reg().type);
    EXPECT_EQ(15, parser.reg().index);
  }
  {
    const string text = "p0 ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::PARAM, parser.reg().type);
    EXPECT_EQ(0, parser.reg().index);
  }
  {
    const string text = "e5 ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::ENVMT, parser.reg().type);
    EXPECT_EQ(5, parser.reg().index);
  }
  {
    const string text = "e ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::FAILURE, parser.status());
  }
  {
    const string text = "x5 ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::FAILURE, parser.status());
  }
  {
    const string text = "l* ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::LOCAL_ARRAY, parser.reg().type);
  }
  {
    const string text = "p* ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::PARAM_ARRAY, parser.reg().type);
  }
  {
    const string text = "e* ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::ENVMT_ARRAY, parser.reg().type);
  }
  {
    const string text = "a* ";
    RegisterParser parser(text);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Register::ARRAY_ARRAY, parser.reg().type);
  }
}

TEST(Bytecode, Operand) {
  {
    const string text = "15";
    OperandParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Operand::IMMEDIATE, parser.operand().type);
    EXPECT_EQ(15, store::IntValue(parser.operand().value));
  }
  {
    const string text = "atom";
    OperandParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Operand::IMMEDIATE, parser.operand().type);
    EXPECT_TRUE(parser.operand().value == store::Atom::Get("atom"));
  }
  {
    const string text = "e5";
    OperandParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Operand::REGISTER, parser.operand().type);
    EXPECT_EQ(Register::ENVMT, parser.operand().reg.type);
    EXPECT_EQ(5, parser.operand().reg.index);
  }
  {
    const string text = "'e5'";
    OperandParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    EXPECT_EQ(Operand::IMMEDIATE, parser.operand().type);
    EXPECT_TRUE(parser.operand().value == store::Atom::Get("e5"));
  }
}

TEST(Bytecode, Mnemonic) {
  {
    const string text = "return()";
    MnemonicParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    Bytecode code;
    EXPECT_TRUE(parser.GetBytecode(&code));
    EXPECT_EQ(Bytecode::RETURN, code.opcode);
  }
  {
    const string text = "branch(to:5)";
    MnemonicParser parser(text, NULL);
    EXPECT_EQ(Parser::OK, parser.status());
    Bytecode code;
    EXPECT_TRUE(parser.GetBytecode(&code));
    EXPECT_EQ(Bytecode::BRANCH, code.opcode);
    EXPECT_EQ(Operand::IMMEDIATE, code.operand1.type);
    EXPECT_EQ(5, store::IntValue(code.operand1.value));
  }
}

TEST(Bytecode, Instruction) {
  {
    const string text = "return()";
    InstructionParser inst(text, NULL);
    EXPECT_EQ(Parser::OK, inst.status());
    EXPECT_EQ(Bytecode::RETURN, inst.code().opcode);
  }
  {
    const string text = "Label:return()";
    StaticStore store(kStoreSize);
    ParsingContext context(&store);
    InstructionParser inst(text, &context);
    EXPECT_EQ(Parser::OK, inst.status());
    EXPECT_EQ(Bytecode::RETURN, inst.code().opcode);
    EXPECT_TRUE(ContainsKey(context.variable, "Label"));
    EXPECT_FALSE(store::IsDet(GetExisting(context.variable, "Label")));
  }
  {
    const string text = "l0 := 1";
    InstructionParser inst(text, NULL);
    EXPECT_EQ(Parser::OK, inst.status());
    EXPECT_EQ(Bytecode::LOAD, inst.code().opcode);
  }
  {
    const string text = "l0 = 1";
    InstructionParser inst(text, NULL);
    EXPECT_EQ(Parser::OK, inst.status());
    EXPECT_EQ(Bytecode::UNIFY, inst.code().opcode);
  }
  {
    const string text = "l0 := array(size:3 init:_)";
    InstructionParser inst(text, NULL);
    EXPECT_EQ(Parser::OK, inst.status());
    EXPECT_EQ(Bytecode::NEW_ARRAY, inst.code().opcode);
  }
}

TEST(Bytecode, Segment) {
  {
    const string text = "segment()";
    CodeSegmentParser segment_parser(text, NULL);
    EXPECT_EQ(Parser::OK, segment_parser.status());
    EXPECT_TRUE(segment_parser.segment()->empty());
  }
  {
    const string text = "segment(return())";
    CodeSegmentParser segment_parser(text, NULL);
    EXPECT_EQ(Parser::OK, segment_parser.status());
    EXPECT_EQ(1, segment_parser.segment()->size());
  }
  {
    const string text = "seg(return())";
    CodeSegmentParser segment_parser(text, NULL);
    EXPECT_EQ(Parser::FAILURE, segment_parser.status());
  }
  {
    StaticStore store(kStoreSize);
    ParsingContext context(&store);
    const string text =
        "segment("
        "  Label:return()"
        "  Label2: nop()"
        ")";
    CodeSegmentParser segment_parser(text, &context);
    EXPECT_EQ(Parser::OK, segment_parser.status());
    EXPECT_EQ(2, segment_parser.segment()->size());
    EXPECT_TRUE(ContainsKey(context.variable, "Label"));
    EXPECT_EQ(0, store::IntValue(GetExisting(context.variable, "Label")));
    EXPECT_TRUE(ContainsKey(context.variable, "Label2"));
    EXPECT_EQ(1, store::IntValue(GetExisting(context.variable, "Label2")));
  }
}

TEST(Bytecode, Proc) {
  const string text =
      "proc("
      "  nlocals: 5"
      "  bytecode: segment("
      "    nop()"
      "  )"
      ")";
  ProcParser proc_parser(text, NULL);
  EXPECT_EQ(Parser::OK, proc_parser.status());
  store::Closure* proc = proc_parser.proc();
  EXPECT_EQ(5, proc->nlocals());
}

TEST(Bytecode, BytecodeSource) {
  const string text =
      "X = 1 "
      "Main = proc("
      "  nlocals: X"
      "  bytecode: segment("
      "    nop()"
      "  )"
      ")";
  StaticStore store(kStoreSize);
  BytecodeSourceParser parser(text, &store);
  EXPECT_EQ(Parser::OK, parser.status());
  EXPECT_TRUE(ContainsKey(parser.context().variable, "X"));
  EXPECT_TRUE(ContainsKey(parser.context().variable, "Main"));
}

}  // namespace combinators
