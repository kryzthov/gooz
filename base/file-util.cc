#include "base/file-util.h"

#include <dirent.h>

#include <iostream>
#include <fstream>

#include <boost/regex.hpp>

using std::ifstream;

namespace util {

bool ReadFileToString(const string& file_path, string* content) {
  CHECK_NOTNULL(content);
  ifstream input(file_path, ifstream::in);
  if (!input) {
    LOG(INFO) << "Unable to open file: " << file_path;
    return false;
  }
  input.unsetf(std::ios::skipws);  // no white-space skipping
  std::copy(std::istream_iterator<char>(input),
            std::istream_iterator<char>(),
            std::back_inserter(*content));
  return true;
}

void ListDir(const string& path, vector<string>* entries)
    throw(Exception) {
  CHECK_NOTNULL(entries);
  DIR* dp = opendir(path.c_str());
  if (dp == NULL) throw Exception();

  while (true) {
    struct dirent* entry = readdir(dp);
    if (entry == NULL) break;
    entries->push_back(entry->d_name);
  }

  closedir(dp);
}

void ListDirPattern(const string& path,
                    const string& pattern,
                    vector<string>* entries)
    throw(Exception) {
  vector<string> lentries;
  ListDir(path, &lentries);

  boost::regex re(pattern);
  for (auto it = lentries.begin(); it != lentries.end(); ++it) {
    const StringPiece& entry = *it;
    boost::cmatch match;
    if (boost::regex_search(entry.begin(), entry.end(), match, re,
                            boost::regex_constants::match_continuous))
      entries->push_back(entry.as_string());
  }
}

}  // namespace util
