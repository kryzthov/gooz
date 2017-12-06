// Runs various bytecode snippets.

#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <gtest/gtest.h>

#include "base/file-util.h"
#include "base/stl-util.h"
#include "store/values.h"
// #include "store/thread.h"
#include "combinators/bytecode.h"
#include "store/bytecode.h"
#include "store/thread.inl.h"

namespace store {

namespace {

string GetTestBytecodeSourcePath(const string& name) {
  const string test_ozb_dir = "store/bytecode_tests";
  return (format("%s/%s.ozb") % test_ozb_dir % name).str();
}

string GetTestBytecodeSource(const string& name) {
  return util::ReadFileToString(GetTestBytecodeSourcePath(name));
}

}  // namespace

// -----------------------------------------------------------------------------
// Helpers to define a bytecode segment easily not using the bytecode parser.

#define I3(InstType, Op1, Op2, Op3) \
  Bytecode(Bytecode::InstType, (Op1), (Op2), (Op3))

#define I2(InstType, Op1, Op2) \
  Bytecode(Bytecode::InstType, (Op1))

#define I1(InstType, Op1) \
  Bytecode(Bytecode::InstType, (Op1))

#define I0(InstType) \
  Bytecode(Bytecode::InstType)

#define L(i) \
  Operand(Register(Register::LOCAL, (i)))

#define P(i) \
  Operand(Register(Register::PARAM, (i)))

#define E(i) \
  Operand(Register(Register::ENVMT, (i)))

#define REG_ARRAY \
  Operand(Register(Register::RARRAY))

#define A(i) \
  Operand(Register(Register::ARRAY, (i)))

#define REG_EXN \
  Operand(Register(Register::EXN))

#define FREE() \
  Operand(Variable::New())

#define INT(int_val) \
  Operand(Value::Integer(int_val))

#define BOOL(bool_val) \
  Operand(Boolean::Get(bool_val))

#define ATOM(atom_val) \
  Operand(Atom::Get(atom_val))

// -----------------------------------------------------------------------------

const uint64 kStoreSize = 1024 * 1024;

class BytecodeTest : public testing::Test {
 protected:
  BytecodeTest()
      : store_(kStoreSize) {
  }

  StaticStore store_;
};

// -----------------------------------------------------------------------------

TEST_F(BytecodeTest, Branching) {
  const string source =
      "X = _ "
      "Y = _ "
      "Main = proc("
      "  bytecode: segment("
      "    branch(to:End)"
      "    unify(value1:X value2: 1)"
      "  End:"
      "    unify(value1:Y value2: 1)"
      "  )"
      ")";
  combinators::BytecodeSourceParser parser(source, &store_);
  Closure* closure =
      GetExisting(parser.context().variable, "Main").as<Closure>();
  LOG(INFO) << Value(closure).ToString();

  Engine engine;
  /*Thread* thread =*/ Thread::New(&engine, closure, Array::EmptyArray);
  engine.Run();

  EXPECT_FALSE(IsDet(GetExisting(parser.context().variable, "X")));
  EXPECT_EQ(1, IntValue(GetExisting(parser.context().variable, "Y")));
}

TEST_F(BytecodeTest, TopLevelReturn) {
  const string source =
      "Main = proc("
      "  bytecode: segment("
      "    nop()"
      "    return()"
      "  )"
      ")";
  combinators::BytecodeSourceParser parser(source, &store_);
  Closure* closure =
      GetExisting(parser.context().variable, "Main").as<Closure>();
  LOG(INFO) << Value(closure).ToString();

  Engine engine;
  /*Thread* thread =*/ Thread::New(&engine, closure, Array::EmptyArray);
  engine.Run();
}

TEST_F(BytecodeTest, Loop) {
  const string source =
      "X = {NewCell 5}"
      "Main = proc("
      "  nlocals: 3"
      "  bytecode: segment("
      "    l0 := array(size:1 init:_)"
      "  Loop:"
      "    l1 := @X"
      "    l0[0] := l1"
      "    call_native(name:print params:l0)"
      "    call_native(name:is_zero params:l0)"
      "    l2 := l0[0]"
      "    cond_branch(to:End cond:l2)"
      "    l0[0] := l1"
      "    call_native(name:decrement params:l0)"
      "    l1 := l0[0]"
      "    @X := l1"
      "    branch(to:Loop)"
      "  End:"
      "    return()"
      "  )"
      ")";
  combinators::BytecodeSourceParser parser(source, &store_);
  Closure* closure =
      GetExisting(parser.context().variable, "Main").as<Closure>();
  LOG(INFO) << Value(closure).ToString();

  Engine engine;
  /*Thread* thread =*/ Thread::New(&engine, closure, Array::EmptyArray);
  engine.Run();
}

// TEST_F(BytecodeTest, InfiniteLoop) {
//   const string source =
//       "Main = proc("
//       "  nlocals: 1"
//       "  bytecode: segment("
//       "    l0 := array(size:1 init:_)"
//       "    l0[0] := 12345"
//       "  Loop:"
//       "    call_native(name:print params:l0)"
//       "    branch(to:Loop)"
//       "  )"
//       ")";
//   combinators::BytecodeSourceParser parser(source);
//   Closure* closure =
//       GetExisting(parser.context().variable, "Main").as<Closure>();
//   LOG(INFO) << Value(closure).ToString();

//   Engine engine;
//   /*Thread* thread =*/ Thread::New(&engine, closure, Array::EmptyArray);
//   engine.Run();
// }

TEST_F(BytecodeTest, RecursiveFactorial) {
  const string& source = GetTestBytecodeSource("recursive_factorial");
  combinators::BytecodeSourceParser parser(source, &store_);
  Closure* closure =
      GetExisting(parser.context().variable, "Main").as<Closure>();
  LOG(INFO) << Value(closure).ToString();

  Engine engine;
  /*Thread* thread =*/ Thread::New(&engine, closure, Array::EmptyArray);
  engine.Run();
}

TEST_F(BytecodeTest, MultiThread) {
  const string& source = GetTestBytecodeSource("multi_thread");
  combinators::BytecodeSourceParser parser(source, &store_);
  Engine engine;

  Unify(GetExisting(parser.context().variable, "True"), Boolean::True);

  Closure* thread1_proc =
      GetExisting(parser.context().variable, "Thread1").as<Closure>();
  LOG(INFO) << Value(thread1_proc).ToString();
  /*Thread* thread1 =*/ Thread::New(&engine, thread1_proc, Array::EmptyArray);

  Closure* thread2_proc =
      GetExisting(parser.context().variable, "Thread2").as<Closure>();
  LOG(INFO) << Value(thread2_proc).ToString();
  /*Thread* thread2 =*/ Thread::New(&engine, thread2_proc, Array::EmptyArray);

  engine.Run();
}

// -----------------------------------------------------------------------------
