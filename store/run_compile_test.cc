#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "base/file-util.h"
#include "base/stl-util.h"
#include "base/string_piece.h"
#include "combinators/oznode_eval_visitor.h"
#include "store/bytecode.h"
#include "store/compiler.h"
#include "store/thread.inl.h"

DEFINE_string(
    oz_compile_path,
    "store/compile_tests",
    "Path to the .ozb file to run."
);

DEFINE_string(
    oz_compile_test,
    "",
    "Optional name of compile test to run."
);

namespace store {

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

const uint64 kStoreSize = 1024 * 1024;  // 1MB

TEST(CompileTest, RunTests) {
  vector<string> test_names;
  if (FLAGS_oz_compile_test.empty())
    util::ListDirPattern(FLAGS_oz_compile_path, ".*\\.ozc", &test_names);
  else
    test_names.push_back(FLAGS_oz_compile_test);

  for (auto it = test_names.begin(); it != test_names.end(); ++it) {
    const StringPiece test_name(*it);
    const string file_path = (format("%s/%s")
                              % FLAGS_oz_compile_path
                              % test_name).str();
    const string source = util::ReadFileToString(file_path);
    LOG(INFO) << "Running compile test: " << test_name;

    StaticStore store(kStoreSize);

    combinators::oz::OzParser parser;
    CHECK(parser.Parse(source)) << "Error parsing:\n" << source;
    shared_ptr<combinators::oz::OzNodeGeneric> root =
        std::dynamic_pointer_cast<combinators::oz::OzNodeGeneric>(parser.root());
    LOG(INFO) << "AST:\n" << *root << std::endl;
    CHECK_EQ(combinators::oz::OzLexemType::TOP_LEVEL, root->type);
    combinators::oz::EvalVisitor visitor(&store);
    for (shared_ptr<combinators::oz::AbstractOzNode> node : root->nodes)
      visitor.Eval(node.get());

    Value code_desc = visitor.vars()["Main"];
    const string& expected = visitor.vars()["Expected"]
        .Deref().as<Atom>()->value();

    Compiler compiler(&store, NULL);
    vector<string> env;
    Closure* const closure = compiler.CompileProcedure(code_desc, &env);
    CHECK(env.empty());
    LOG(INFO) << "Generated closure:\n" << Value(closure).ToString();

    // TODO: handle recursive closure!
    VLOG(1) << Value(closure).ToString();

    Engine engine;
    TestPrint test_print;
    engine.RegisterNative("print", &test_print);
    New::Thread(&store, &engine, closure, Array::EmptyArray, &store);
    engine.Run();

    EXPECT_EQ(expected, test_print.output());
  }
}

// -----------------------------------------------------------------------------

}  // namespace store
