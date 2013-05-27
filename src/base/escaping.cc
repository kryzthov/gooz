#include "base/escaping.h"

#include <cctype>

#include <boost/format.hpp>
using boost::format;

#include "glog/logging.h"

namespace base {

int HexDigitValue(char c) {
  if ((c >= '0') && (c <= '9'))
    return c - '0';
  if ((c >= 'a') && (c <= 'f'))
    return c - 'a';
  if ((c >= 'A') && (c <= 'F'))
    return c - 'A';
  LOG(FATAL) << "Expected an hexadecimal char, got: " << c;
}

void Escape(const StringPiece& raw, string* escaped,
            const char* const extra_escape) {
  CHECK_NOTNULL(escaped);
  const StringPiece extra(extra_escape);
  for (uint i = 0; i < raw.size(); ++i) {
    const char c = raw[i];
    switch(c) {
      case '\\': escaped->append("\\\\"); break;
      case '\r': escaped->append("\\r"); break;
      case '\n': escaped->append("\\n"); break;
      case '\t': escaped->append("\\t"); break;
      default: {
        if ((c >= 32) && (c <= 127)) {
          if (extra.find(c) != StringPiece::npos)
            escaped->push_back('\\');
          escaped->push_back(c);
        } else {
          escaped->append((format("\\x%02x") % c).str());
        }
        break;
      }
    }
  }
}

void Unescape(const StringPiece& escaped, string* raw,
              const char* const extra_unescape) {
  CHECK_NOTNULL(raw);
  const StringPiece extra(extra_unescape);
  uint i = 0;
  for (; i < escaped.size(); ++i) {
    const char c = escaped[i];
    if ((c == '\\') && (i + 1 < escaped.size())) {
      ++i;
      const char c2 = escaped[i];
      switch(c2) {
        case '\\': raw->push_back('\\'); continue;
        case 'r': raw->push_back('\r'); continue;
        case 'n': raw->push_back('\n'); continue;
        case 't': raw->push_back('\t'); continue;
        case 'x': {
          if ((i + 2 < escaped.size())
              && isxdigit(escaped[i + 1])
              && isxdigit(escaped[i + 2])) {
            i += 2;
            char ec = (HexDigitValue(escaped[i + 1]) << 4)
                | HexDigitValue(escaped[i + 2]);
            raw->push_back(ec);
            continue;
          }
          break;
        }
        default: {
          if (extra.find(c2) != StringPiece::npos) {
            raw->push_back(c2);
            continue;
          }
        }
      }
      --i;
    }
    raw->push_back(c);
  }
}

}  // namespace base
