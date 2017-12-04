#include <iostream>
#include <string>

#include <glog/logging.h>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "base/file-util.h"
#include "store/oz_lexer.h"

namespace lex = boost::spirit::lex;

int main(int argc, char* argv[]) {
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::InitGoogleLogging(argv[0]);

  CHECK_GE(argc, 2) << "No input file provided";
  const string file_path = argv[1];
  const string str = util::ReadFileToString(file_path);
  oz::RunLexerTest(str, file_path);

  return 0;
}
