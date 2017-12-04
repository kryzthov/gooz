#ifndef COMBINATORS_STREAM_H_
#define COMBINATORS_STREAM_H_

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
using std::max;
using std::min;
using std::string;
using std::vector;

#include <glog/logging.h>

#include "base/escaping.h"
#include "base/stringer.h"
#include "base/string_piece.h"
using base::StringPiece;

namespace combinators {

template <typename Elt>
class TStream {
 public:
  typedef Elt ElementType;
  typedef TStream<Elt> StreamType;

  inline
  const ElementType& first() const {
    return Get(0);
  }

  inline
  const ElementType& last() const {
    return Get(StreamSize() - 1);
  }

  inline
  StreamType Slice(int64 ifirst) const {
    return Slice(ifirst, StreamSize());
  }

  inline
  bool StreamEmpty() const {
    return StreamSize() == 0;
  }

  const ElementType& Get(int64 i) const;
  int64 StreamSize() const;
  StreamType Slice(int64 ifirst, int64 ilast) const;
};

// -----------------------------------------------------------------------------

template <typename Elt>
class TVectorStream : public TStream<Elt> {
 public:
  typedef Elt ElementType;
  typedef TVectorStream<Elt> StreamType;

  TVectorStream()
      : elements_(nullptr),
        first_(-1),
        last_(-1) {}

  TVectorStream(const vector<ElementType>& elements)
      : elements_(&elements),
        first_(0),
        last_(elements_->size()) {}

  TVectorStream(const vector<ElementType>& elements,
                int64 first, int64 last)
      : elements_(&elements),
        first_(first),
        last_(last) {
    CHECK_LE(0, first_);
    CHECK_LE(first_, last_);
    CHECK_LE(last_, (int64) elements_->size());
  }

  TVectorStream(const TVectorStream& stream)
      : elements_(stream.elements_),
        first_(stream.first_),
        last_(stream.last_) {
  }

  TVectorStream(const TVectorStream& from, const TVectorStream& to)
      : elements_(from.elements_),
        first_(from.first_),
        last_(to.first_) {
    CHECK_EQ(from.elements_, to.elements_);
    CHECK_LE(0, first_);
    CHECK_LE(first_, last_);
    CHECK_LE(last_, (int64) elements_->size());
  }

  TVectorStream& operator=(const TVectorStream& stream) {
    elements_ = stream.elements_;
    first_ = stream.first_;
    last_ = stream.last_;
    return *this;
  }

  inline
  const ElementType& Get(int64 i) const {
    CHECK_GE(i, 0);
    CHECK_LT(i, last_ - first_);
    return elements_->at(first_ + i);
  }

  inline
  int64 StreamSize() const {
    return last_ - first_;
  }

  TVectorStream Slice(int64 ifirst, int64 ilast) const {
    CHECK_LE(0, ifirst);
    CHECK_LE(ifirst, ilast);
    CHECK_LE(ilast, StreamSize());
    return TVectorStream(*elements_, first_ + ifirst, first_ + ilast);
  }

  inline
  const ElementType& first() const {
    return Get(0);
  }

  inline
  const ElementType& last() const {
    return Get(StreamSize() - 1);
  }

  inline
  StreamType Slice(int64 ifirst) const {
    return Slice(ifirst, StreamSize());
  }

  inline
  bool StreamEmpty() const {
    return StreamSize() == 0;
  }

  inline
  TVectorStream SliceBefore() const {
    return TVectorStream(*elements_, 0, first_);
  }

  inline
  TVectorStream SliceAfter() const {
    return TVectorStream(*elements_, last_, elements_->size());
  }

 private:
  const vector<ElementType>* elements_;
  int64 first_;
  int64 last_;
};

// -----------------------------------------------------------------------------

// Extends StringPiece with offset/line/column counters.
// Adds a "streaming" interface.
class Stream : public base::StringPiece, TStream<char> {
 public:
  static const char kEndOfLine = '\n';

  // Constructors for STL containers.
  Stream()
      : StringPiece(),
        offset_(0),
        line_(1),
        line_pos_(1) {
  }

  Stream(const Stream& other)
      : StringPiece(other.data(), other.size()),
        offset_(other.offset_),
        line_(other.line_),
        line_pos_(other.line_pos_) {
  }

  Stream& operator=(const Stream& other) {
    set(other.data(), other.size());
    offset_ = other.offset_;
    line_ = other.line_;
    line_pos_ = other.line_pos_;
    return *this;
  }

  // Initializes a stream from the given string.
  Stream(const StringPiece& source,
         uint offset = 0, int line = 1, int line_pos = 1)
      : StringPiece(source.data(), source.size()),
        offset_(offset),
        line_(line),
        line_pos_(line_pos) {
  }

  // Initializes a stream from the given string.
  Stream(const string& source,
         uint offset = 0, int line = 1, int line_pos = 1)
      : StringPiece(source),
        offset_(offset),
        line_(line),
        line_pos_(line_pos) {
  }

  // @returns the current character in the stream.
  char Get() const {
    CHECK_GT(size(), 0UL);
    return data()[0];
  }

  // @returns the slice of N characters in the stream.
  Stream Get(uint nchars) const {
    CHECK_GE(size(), nchars);
    return Stream(substr(nchars), offset_, line_, line_pos_);
  }

  // Moves this stream to the next character.
  Stream& Walk() {
    if (!empty()) {
      if (data()[0] == kEndOfLine) {
        ++line_;
        line_pos_ = 1;
      } else {
        ++line_pos_;
      }
      ++offset_;
      set(data() + 1, size() - 1);
    }
    return *this;
  }

  // @returns The stream advanced by one element.
  Stream Next() const {
    return Stream(*this).Walk();
  }

  // @returns The stream advanced by steps elements.
  // @param steps A positive number of elements to skip.
  Stream Next(uint steps) const {
    Stream next(*this);
    for (uint i = 0; i < steps; ++i)
      next.Walk();
    return next;
  }

  // @returns The index in the full stream, starting from 0.
  uint offset() const { return offset_; }

  // @returns The line number, starting from 1.
  int line() const { return line_; }

  // @returns The position in the current line, starting from 1.
  int line_pos() const { return line_pos_; }

 private:
  // Offset in the full stream, starting from 0.
  uint offset_;

  // The line number, starting from 1.
  int line_;

  // The character position in the line, starting from 1.
  int line_pos_;
};

inline
string DebugSlice(const StringPiece& s, uint limit = 20) {
  string slice;
  slice.push_back('"');
  slice.append(base::Escape(s.substr(0, limit), "\""));
  if (s.size() > limit)
    slice.append("...");
  slice.push_back('"');
  return slice;
}

inline string Str(const Stream& stream) {
  return Stringer("CharStream")
      ("@", stream.offset())
      ("l", stream.line())
      ("c", stream.line_pos())
      (DebugSlice(stream));
}

inline
std::ostream& operator<<(std::ostream& os, const Stream& stream) {
  return os << Str(stream);
}

}  // namespace combinators

#endif  // COMBINATORS_STREAM_H_
