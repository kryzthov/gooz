#ifndef COMBINATORS_BASE_H_
#define COMBINATORS_BASE_H_

#include "combinators/stream.h"

#include <functional>
#include <iostream>
#include <tuple>
#include <vector>

#include <boost/regex.hpp>
#include <boost/variant.hpp>
#include <boost/variant/static_visitor.hpp>

#include "base/string_piece.h"

using base::StringPiece;
using std::tuple;
using std::vector;

namespace combinators {

typedef Stream CharStream;

// Empty parsing result
struct Empty {};

enum class ParsingStatus {
  OK,
  FAILED,
};

inline std::ostream& operator <<(std::ostream& os, ParsingStatus status) {
  switch (status) {
    case ParsingStatus::OK:
      return os << "ParsingStatus::OK";
    case ParsingStatus::FAILED:
      return os << "ParsingStatus::FAILED";
  }
  return os << "UNKNOWN ParsingStatus enum value";
}


template <typename Stream, typename Payload = Empty>
struct ParsingResult {
  ParsingResult() {}

  ParsingResult(Stream stream,
                ParsingStatus init_status = ParsingStatus::FAILED)
      : status(init_status),
        start(stream),
        next(stream) {
  }

  inline
  ParsingResult& Succeed() {
    status = ParsingStatus::OK;
    return *this;
  }

  inline
  ParsingResult& Succeed(Stream next_) {
    next = next_;
    return Succeed();
  }

  inline
  ParsingResult& Succeed(Stream next_, const Payload& payload_) {
    payload = payload_;
    return Succeed(next_);
  }

  inline
  ParsingResult& Fail() {
    status = ParsingStatus::FAILED;
    return *this;
  }

  inline ParsingResult& Fail(const string& error) {
    errors.push_back(error);
    return Fail();
  }

  inline
  ParsingResult& SetNext(Stream next_) {
    next = next_;
    return *this;
  }

  ParsingStatus status;
  Stream start;
  Stream next;
  Payload payload;
  vector<string> errors;
};

// -----------------------------------------------------------------------------

// A parser is a function: input stream → ParsingResult
template <typename _Payload, typename _Stream>
class Parser {
 public:
  typedef _Payload PayloadType;
  typedef _Stream StreamType;
  typedef ParsingResult<StreamType, PayloadType> ParsingResultType;

  virtual ParsingResultType Parse(StreamType input) = 0;

  inline
  ParsingResultType operator()(StreamType input) {
    return this->Parse(input);
  }
};

// -----------------------------------------------------------------------------

inline
CharStream SkipBlank(CharStream input) {
  while (!input.empty() && std::isspace(input.Get()))
    input.Walk();
  return input;
}

// -----------------------------------------------------------------------------

class StringParser : public Parser<StringPiece, CharStream> {
 public:
  StringParser(char ch) : str_(1, ch) {}
  StringParser(const string& str) : str_(str) {}

  virtual StringParser::ParsingResultType Parse(CharStream input) {
    StringParser::ParsingResultType result(input);
    if (input.starts_with(str_)) {
      result.Succeed(input.Next(str_.size()), str_);
    }
    return result;
  }

 private:
  const string str_;
};

typedef StringParser CharParser;

class RegexParser : public Parser<StringPiece, CharStream> {
 public:
  RegexParser(boost::regex regexp) : regexp_(regexp) {}
  RegexParser(const string& regexp) : regexp_(regexp) {}

  virtual RegexParser::ParsingResultType Parse(CharStream input) {
    RegexParser::ParsingResultType result(input);
    boost::cmatch match;
    if (boost::regex_search(input.begin(), input.end(), match, regexp_,
                            boost::regex_constants::match_continuous)) {
      const int length = match.length();
      result.Succeed(input.Next(length), input.substr(0, length));
    }
    return result;
  }

 private:
  const boost::regex regexp_;
};

// -----------------------------------------------------------------------------
// Repeat applies the same parser as many times as it succeeds.
// This parser always succeeds (status is OK).

template <
  typename EltParser,
  typename _Stream,
  typename EltPayload = typename EltParser::PayloadType,
  typename _Payload = vector<EltPayload>
>
class Repeat : public Parser<_Payload, _Stream> {
 public:
  typedef _Payload PayloadType;
  typedef _Stream StreamType;
  typedef ParsingResult<StreamType, PayloadType> ParsingResultType;

  Repeat(EltParser& parser): parser_(parser) {}

  virtual ParsingResultType Parse(StreamType input) {
    ParsingResultType result(input, ParsingStatus::OK);
    for (;;) {
      typename EltParser::ParsingResultType parse_result = parser_(result.next);
      switch (parse_result.status) {
        case ParsingStatus::OK:
          result.payload.push_back(parse_result.payload);
          result.Succeed(parse_result.next);
          break;
        case ParsingStatus::FAILED:
          return result;
      }
    }
  }

 private:
  EltParser& parser_;
};

// -----------------------------------------------------------------------------
// Parser switch/select: P1(input) | P2(input) | ... | Pk(input).
// Parsers are evaluated in order, stopping at the first match.
// Payload is variant<P1, P2, ..., Pk>.

// Logical OR of multiple parsers.
// Selects the first parser that matches.
template <
  typename _Payload,
  typename _Stream,
  typename... Parsers
>
class Or : public Parser<_Payload, _Stream> {
 public:
  typedef _Payload PayloadType;
  typedef _Stream StreamType;
  typedef ParsingResult<StreamType, PayloadType> ParsingResultType;

  Or(Parsers... parsers) : parsers_(parsers...) {
  }

  ParsingResultType Parse(StreamType input) {
    return ParseOr<0, Parsers...>(input);
  }

 private:
  // Failure case: no more parser to test
  template <int iparser>
  ParsingResultType ParseOr(StreamType input) {
    return ParsingResultType(input);  // status = failing
  }

  // General case
  template <
    int iparser,
    typename ParserHead,
    typename... ParserTail
  >
  ParsingResultType ParseOr(StreamType input) {
    ParsingResultType result(input);
    auto parse_result = std::get<iparser>(parsers_)(input);
    switch (parse_result.status) {
      case ParsingStatus::OK:
        result.Succeed(parse_result.next, parse_result.payload);
        return result;
      case ParsingStatus::FAILED:
        return ParseOr<iparser + 1, ParserTail...>(input);
    }
    LOG(FATAL) << "unreachable";
  }

  tuple<Parsers...> parsers_;
};

template <typename Payload, typename Stream, typename... Parsers>
Parser<Payload, Stream>* NewOr(Parsers... parsers) {
  return new Or<Payload, Stream, Parsers...>(parsers...);
}

// -----------------------------------------------------------------------------
// Parser sequence: applies Pk(Pk-1(...(P2(P1(input)))...)).
// Payload is tuple<P1, P2, ..., Pk>.

template <
  typename _Payload,
  typename _Stream,
  typename... Parsers
>
class And : public Parser<_Payload, _Stream> {
 public:
  typedef _Payload PayloadType;
  typedef _Stream StreamType;
  typedef ParsingResult<StreamType, PayloadType> ParsingResultType;

  And(Parsers... parsers)
      : parsers_(parsers...) {
  }

  ParsingResultType Parse(StreamType input) {
    ParsingResultType result(input);
    ParseAnd<0, Parsers...>(input, &result);
    return result;
  }

 private:
  template <int iparser>
  void ParseAnd(StreamType input, ParsingResultType* result) {
    // No more parser to apply, succeed.
    result->Succeed(input);
  }

  // General case
  template <
    int iparser,
    typename ParserHead,
    typename... ParserTail
  >
  void ParseAnd(StreamType input, ParsingResultType* result) {
    auto parse_result = std::get<iparser>(parsers_)(input);
    switch (parse_result.status) {
      case ParsingStatus::OK: {
        std::get<iparser>(result->payload) = parse_result.payload;
        ParseAnd<iparser + 1, ParserTail...>(parse_result.next, result);
        break;
      }
      case ParsingStatus::FAILED: {
        result->Fail();
        break;
      }
    }
  }

  tuple<Parsers...> parsers_;
};

template <typename Payload, typename Stream, typename... Parsers>
Parser<Payload, Stream>* NewAnd(Parsers... parsers) {
  return new And<Payload, Stream, Parsers...>(parsers...);
}

// -----------------------------------------------------------------------------
// Wrappers

template<
  typename _Payload,
  typename FromPayload,
  typename _Stream,
  typename _WrapFunType
>
class Wrapper : public Parser<_Payload, _Stream> {
 public:
  typedef _Payload PayloadType;
  typedef _Stream StreamType;
  typedef ParsingResult<StreamType, PayloadType> ParsingResultType;

  typedef Parser<FromPayload, StreamType> WrappedParserType;
  typedef _WrapFunType WrapFunType;

  Wrapper(WrappedParserType& parser, WrapFunType wrap_fun)
      : parser_(parser),
        wrap_fun_(wrap_fun) {
  }

  virtual ParsingResultType Parse(StreamType input) {
    ParsingResultType result(input);
    ParsingResult<StreamType, FromPayload> from_result = parser_.Parse(input);
    if (from_result.status == ParsingStatus::OK) {
      result.Succeed(from_result.next, wrap_fun_(from_result.payload));
    }
    return result;
  }

 private:
  WrappedParserType& parser_;
  WrapFunType wrap_fun_;
};

template<typename _Payload, typename FromPayload, typename _Stream, typename F>
Wrapper<_Payload, FromPayload, _Stream, F>*
Wrap(Parser<FromPayload, _Stream>& parser, F wrap_fun) {
  return new Wrapper<_Payload, FromPayload, _Stream, F>(parser, wrap_fun);
}

}  // namespace combinators

#endif  // COMBINATORS_BASE_H_
