// Compiles and runs a code description (AST).
#include "store/compiler.h"

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "base/file-util.h"
#include "combinators/oznode_eval_visitor.h"
// #include "combinators/bytecode_parser.h"
#include "store/engine.h"
#include "store/environment.h"


DEFINE_string(
    oz_code_path,
    "",
    "Path to the .ozc file to compile."
);

namespace store {

const uint64 kStoreSize = 1024 * 1024;

void CompileRun() {
  CHECK(!FLAGS_oz_code_path.empty()) << "Specify --oz_code_path.";

  StaticStore store(kStoreSize);
  const string ascii_desc =
      util::ReadFileToString(FLAGS_oz_code_path);
  Value code_desc = combinators::oz::ParseEval(ascii_desc, &store);
  LOG(INFO) << code_desc.ToString();
  Compiler compiler(&store, NULL);
  vector<string> env;
  Closure* const closure = compiler.CompileProcedure(code_desc, &env);
  CHECK(env.empty());
  LOG(INFO) << "Generated closure:\n" << Value(closure).ToString();

  Engine engine;
  // Value thread1 =
  New::Thread(&store, &engine, closure, Array::EmptyArray, &store);

  engine.Run();
}

}  // namespace store

int main(int argc, char** argv) {
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  store::CompileRun();
  return EXIT_SUCCESS;
}
