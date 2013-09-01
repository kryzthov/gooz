#include "combinators/oznode_compile_visitor.h"

#include <gtest/gtest.h>

#include "combinators/ozparser.h"
#include "store/store.h"

namespace combinators { namespace oz {

const uint64 kStoreSize = 1024 * 1024;

class CompileVisitorTest : public testing::Test {
 public:
  CompileVisitorTest()
      : store_(kStoreSize),
        visitor_(&store_) {
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
};

TEST_F(CompileVisitorTest, Base) {
  Compile("proc {P X Y} X = {X X = Y} end");
}


}}  // namespace combinators::oz
