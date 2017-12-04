
#include <unistd.h>

#include <string>
using std::string;

#include <glog/logging.h>

#include <gtest/gtest.h>

#include "base/scoped_ptr.h"
#include "combinators/matchers.h"
#include "combinators/primitives.h"
#include "combinators/composite.h"
#include "combinators/stream.h"

namespace combinators {

TEST(Parser, Mixed) {
  EXPECT_EQ(Parser::OK, DecimalIntegerParser(string("123456789")).status());
  EXPECT_EQ(Parser::OK, DecimalIntegerParser(string("123456789+45")).status());
  EXPECT_EQ(Parser::FAILURE, DecimalIntegerParser(string("x123456789")).status());
  // This matches the leading 0 only!
  EXPECT_EQ(Parser::OK, DecimalIntegerParser(string("0123456789")).status());

  const string str = "aVery_NiceAtom45.hello";
  Stream stream(str);
  SimpleAtomParser sap(stream);
  EXPECT_EQ(Parser::OK, sap.status());
  EXPECT_EQ("aVery_NiceAtom45", sap.GetMatch());
  EXPECT_EQ(Parser::OK, SimpleAtomParser(string("atom")).status());

  {
    const string source = "record(a b 2:c cocou:hello)";
    RecordParser parser(Stream(source));
  }
  {
    const string source = "record(a b#c 2:c cocou:hello)";
    Stream stream(source);
    ValueParser value(stream, NULL);
    EXPECT_EQ(Parser::OK, value.status());
    LOG(INFO) << "Value: " << value.GetMatch();
  }
  {
    const string source = "record(a b#c 2:c cocou:hello) # hehehe # 'ou pas'";
    Stream stream(source);
    ValueParser value(stream, NULL);
    EXPECT_EQ(Parser::OK, value.status());
    LOG(INFO) << "Value: " << value.GetMatch();
  }
}

}  // namespace combinators
