#ifndef BASE_STRINGER_H_
#define BASE_STRINGER_H_

#include <iostream>
#include <string>
using std::string;

#include "base/basictypes.h"

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

// String representation of arbitrary objects, for logging/debugging.
template <typename T>
inline string Str(T t) {return "?";}

inline string Str(const string& str) {return str;}
inline string Str(int64 i) {return boost::lexical_cast<string>(i);}
inline string Str(uint64 i) {return boost::lexical_cast<string>(i);}
inline string Str(uint i) {return boost::lexical_cast<string>(i);}
inline string Str(int i) {return boost::lexical_cast<string>(i);}
inline string Str(float f) {return boost::lexical_cast<string>(f);}
inline string Str(double f) {return boost::lexical_cast<string>(f);}
inline string Str(bool b) {return b ? "true" : "false";}

// Object stringer
class Stringer {
 public:
  Stringer() {}
  Stringer(const string& name) : name_(name) {}
  Stringer(const Stringer& stringer)
      : name_(stringer.name_),
        str_(stringer.str_) {}

  template <typename T>
  Stringer& operator()(T value) {
    if (!str_.empty()) str_ += ' ';
    str_ += Str(value);
    return *this;
  }

  Stringer& operator()(const string& str) {
    if (!str_.empty()) str_ += ' ';
    str_ += str;
    return *this;
  }

  template <typename T>
  Stringer& operator()(const string& attr, T value) {
    if (!str_.empty()) str_ += ' ';
    str_ += attr + '=' + Str(value);
    return *this;
  }

  inline string str() const {
    return name_ + '(' + str_ + ')';
  }

  inline operator string() {return str();}

 private:
  string name_;
  string str_;
};

inline
std::ostream& operator <<(std::ostream& os, const Stringer& stringer) {
  return os << stringer;
}

#endif  // BASE_STRINGER_H_
