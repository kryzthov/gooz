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
#include "combinators/bytecode.h"
#include "store/bytecode.h"
#include "store/thread.inl.h"

DEFINE_string(
    oz_bytecode_path,
    "store/bytecode_tests",
    "Path to the .ozb file to run."
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

TEST(BytecodeTest, RunTests) {
  vector<string> test_names;
  util::ListDirPattern(FLAGS_oz_bytecode_path, ".*\\.ozb", &test_names);

  for (auto it = test_names.begin(); it != test_names.end(); ++it) {
    const StringPiece test_name(*it);
    const string file_path = (format("%s/%s")
                              % FLAGS_oz_bytecode_path
                              % test_name).str();
    const string source = util::ReadFileToString(file_path);
    LOG(INFO) << "Running bytecode test: " << test_name;

    StaticStore store(kStoreSize);
    combinators::BytecodeSourceParser parser(source, &store);
    Closure* closure =
        GetExisting(parser.context().variable, "Main").as<Closure>();
    const string& expected =
        GetExisting(parser.context().variable, "Expected").as<Atom>()->value();

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
