#ifndef BASE_ESCAPING_H_
#define BASE_ESCAPING_H_

#include <string>
using std::string;

#include "base/string_piece.h"
using base::StringPiece;

namespace base {

// Escapes characters in a raw string.
//
// @param raw The string to escape.
// @param escaped Appends the escaped string here.
// @param extra_escape Extra characters to escape (e.g. "\"").
void Escape(const StringPiece& raw, string* escaped,
            const char* const extra_escape = NULL);
inline string Escape(const StringPiece& raw,
                     const char* const extra_escape = NULL) {
  string escaped;
  Escape(raw, &escaped, extra_escape);
  return escaped;
}

// Unescapes characters in an escaped string.
//
// @param raw The string to escape.
// @param escaped Appends the escaped string here.
// @param extra_unescape Extra characters to unescape (e.g. "\"").
void Unescape(const StringPiece& escaped, string* raw,
              const char* const extra_unescape = NULL);
inline string Unescape(const StringPiece& escaped,
                       const char* const extra_unescape = NULL) {
  string raw;
  Unescape(escaped, &raw, extra_unescape);
  return raw;
}

}  // namespace base

#endif  // BASE_ESCAPING_H_
