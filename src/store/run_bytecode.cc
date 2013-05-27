// Runs various bytecode snippets.

#include <string>
using std::string;

#include <boost/format.hpp>
using boost::format;

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "base/file-util.h"
#include "base/stl-util.h"
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

const uint64 kStoreSize = 1024 * 1024;

// -----------------------------------------------------------------------------

void RunBytecode() {
  StaticStore store(kStoreSize);
  const string& source = GetTestBytecodeSource("recursive_factorial");
  combinators::BytecodeSourceParser parser(source, &store);
  Closure* closure =
      GetExisting(parser.context().variable, "Main").as<Closure>();
  // LOG(INFO) << Value(closure).ToString();

  Engine engine;
  New::Thread(&store, &engine, closure, Array::EmptyArray, &store);
  engine.Run();
}

// -----------------------------------------------------------------------------

}  // namespace store

int main(int argc, char** argv) {
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);
  store::RunBytecode();
}
