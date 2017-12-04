#include "combinators/oznode_compile_visitor.h"

#include <gtest/gtest.h>

#include "combinators/ozparser.h"
#include "store/store.h"

namespace combinators { namespace oz {

// -----------------------------------------------------------------------------

namespace {

// A mock 'print' native procedure that records everything.
class TestPrint: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    for (uint64 i = 0; i < parameters->size(); ++i) {
      Value value = parameters->values()[i];
      output_.append(value.ToString());
    }
  }

  const string& output() const { return output_; }

 private:
  string output_;
};

}  // namespace

// -----------------------------------------------------------------------------

const uint64 kStoreSize = 1024 * 1024;

class CompileVisitorTest : public testing::Test {
 public:
  CompileVisitorTest()
      : store_(kStoreSize),
        visitor_(&store_) {
    engine_.RegisterNative("print", &test_print_);
    engine_.RegisterNative("println", &test_print_);
  }

  store::Value Compile(const string& source) {
    OzParser parser;
    CHECK(parser.Parse(source)) << "Error parsing: " << source;
    shared_ptr<OzNodeGeneric> root =
        std::dynamic_pointer_cast<OzNodeGeneric>(parser.root());
    LOG(INFO) << "AST:\n" << *root << std::endl;
    CHECK_EQ(OzLexemType::TOP_LEVEL, root->type);
    return visitor_.Compile(root.get());
  }

  store::StaticStore store_;
  CompileVisitor visitor_;
  Engine engine_;
  TestPrint test_print_;
};

// -----------------------------------------------------------------------------

TEST_F(CompileVisitorTest, ProcedureCall1) {
  Compile("proc {P X Y} {X X Y} end");
}

TEST_F(CompileVisitorTest, ProcedureCallNative) {
  Compile("proc {Show X} {println X} end");
}

TEST_F(CompileVisitorTest, ProcedureCall2) {
  Compile("proc {P X Y Z} X = {Y Z} end");
}

TEST_F(CompileVisitorTest, ProcedureCall3) {
  Compile("proc {P X Y Z} X = {Y Z $} end");
}

TEST_F(CompileVisitorTest, ProcedureCall4) {
  Compile("proc {P X Y Z} X = {Y $ Z} end");
}

TEST_F(CompileVisitorTest, UnifyParams) {
  Compile("proc {P X Y} X = Y end");
}

TEST_F(CompileVisitorTest, UnifyConstantInt) {
  Compile("proc {P X Y} X = 1 end");
}

TEST_F(CompileVisitorTest, UnifyConstantNewVar) {
  Compile("proc {P X Y} X = _ end");
}

TEST_F(CompileVisitorTest, UnifyAsExpression) {
  Compile(
      "proc {P X Y Z}\n"
      "  X = (Y = Z)\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, Raise) {
  Compile("proc {P X} raise X end end");
}

TEST_F(CompileVisitorTest, ProcWithLocal) {
  Compile(
      "proc {P X}\n"
      "  Y = 1\n"
      "in\n"
      "  {show Y}\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, Function) {
  Compile(
      "fun {F X}\n"
      "  X\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, TupleSimple) {
  Compile("fun {F} 1#2#3 end");
}

TEST_F(CompileVisitorTest, Tuple2) {
  Compile("fun {F X Y} X#2#(X+Y) end");
}

TEST_F(CompileVisitorTest, CondStatement) {
  Compile(
      "proc {Proc X}\n"
      "  if false then\n"
      "    X = 1\n"
      "  else\n"
      "    X = 2\n"
      "  end\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, CondExpression) {
  Compile(
      "proc {Proc X}\n"
      "  X = if false then 1 else 2 end\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, RecordStatic) {
  Compile(
      "fun {F X Y Z T}\n"
      "  record(X 2:Y a:Z b:T)\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, ListSimple) {
  Compile(
      "fun {F X Y Z T}\n"
      "  [1 X 2 Y 3]\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, ListConstructor) {
  Compile(
      "fun {F X Y Z T}\n"
      "  1 | X | 2 | Y | 3\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, RecordAccess) {
  Compile(
      "proc {F X Y}\n"
      "  X.Y = 1\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, TuplePattern) {
  Compile(
      "proc {F Z T}\n"
      "  X # Y = Z\n"
      "in\n"
      "  T = X\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, NumberAdd) {
  Compile(
      "fun {Add X Y}\n"
      "  X + Y\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, NumberAddPlenty) {
  Compile(
      "fun {Add X Y Z T}\n"
      "  X + Y + 1 + Z + T\n"
      "end\n"
  );
}

TEST_F(CompileVisitorTest, NestedLocals) {
  Value top_level = Compile(
      "local\n"
      "  local\n"
      "    Y = 1\n"
      "  in\n"
      "    X = Y + 1\n"
      "  end\n"
      "in\n"
      "  {println X}"
      "end\n"
  );
  Engine engine;
  New::Thread(&store_, &engine, top_level.Deref(), Array::EmptyArray, &store_);
  engine.Run();
}

TEST_F(CompileVisitorTest, TopLevelProc) {
  Value top_level = Compile(
      "X = 1\n"
      "Y = X + 1\n"
      "{println Y}\n"
  );
  Engine engine;
  New::Thread(&store_, &engine, top_level.Deref(), Array::EmptyArray, &store_);
  engine.Run();
}

TEST_F(CompileVisitorTest, Factorial) {
  Compile(
      "fun {Factorial N}\n"
      "  if N == 0 then 1 else N * {Factorial (N - 1)} end\n"
      "end\n"
  );
}

}}  // namespace combinators::oz
