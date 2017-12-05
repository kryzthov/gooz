// Simple file utilities.

#ifndef BASE_FILE_UTIL_H__
#define BASE_FILE_UTIL_H__

#include <exception>
#include <string>
#include <vector>

#include "glog/logging.h"

#include "base/string_piece.h"

using base::StringPiece;
using std::string;
using std::vector;

namespace util {

class Exception: public std::exception {};

// Reads the content of a file and append if to a string.
// @param file_path The path of the file to read.
// @param buffer Appends the file content to this string.
// @returns True if successful, false if an error occured.
bool ReadFileToString(const string& file_path, string* buffer);

// Functional version.
inline
string ReadFileToString(const string& file_path) {
  string buffer;
  CHECK(ReadFileToString(file_path, &buffer));
  return buffer;
}

void ListDir(const string& path, vector<string>* entries);

void ListDirPattern(const string& path,
                    const string& pattern,
                    vector<string>* entries);

}  // namespace util

#endif  // BASE_FILE_UTIL_H__
