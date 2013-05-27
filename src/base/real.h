#ifndef BASE_REAL_H_
#define BASE_REAL_H_

#include <iostream>
#include <string>
using std::string;

#include <mpfr.h>
#include <boost/lexical_cast.hpp>

namespace base { namespace real {

enum class Rounding {
  NEAREST = MPFR_RNDN,
  ZERO = MPFR_RNDZ,
  PLUS_INFINITY = MPFR_RNDU,
  MINUS_INFINITY = MPFR_RNDD,
  AWAY_ZERO = MPFR_RNDA,
};

// Wrapper for mpfr_t
class Real {
 public:
  Real() {
    mpfr_init(real_);
  }

  Real(const Real& r) {
    mpfr_init2(real_, r.precision());
    mpfr_set(real_, r.real_, MPFR_RNDN);
  }

  Real(const string& str, int base=10, Rounding rounding = Rounding::NEAREST) {
    mpfr_init_set_str(real_, str.c_str(), base, mpfr_rnd_t(rounding));
  }

  virtual ~Real() {
    mpfr_clear(real_);
  }

  void String(string* s, int base = 10,
              Rounding rounding = Rounding::NEAREST) const {
    // TODO: Fix size
    int size = 256;
    char buffer[size];
    mpfr_exp_t exp;
    mpfr_get_str(buffer, &exp, base, size, real_, mpfr_rnd_t(rounding));
    s->append("0.").append(buffer)
        .append("e").append(boost::lexical_cast<string>(exp));
  }

  inline
  string String(int base = 10,
                Rounding rounding = Rounding::NEAREST) const {
    string s;
    String(&s, base, rounding);
    return s;
  }

  // Note: resets the float to NaN.
  void SetPrecision(mpfr_prec_t precision) {
    mpfr_set_prec(real_, precision);
  }

  mpfr_prec_t precision() const {
    return mpfr_get_prec(real_);
  }

 private:
  mpfr_t real_;
};

inline std::ostream& operator <<(std::ostream& os,
                                 const base::real::Real& real) {
  return os << real.String();
}

}}  // namespace base::real

#endif  // BASE_REAL_H_
