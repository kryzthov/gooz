// Parser combinators for Oz bytecode.

#ifndef COMBINATORS_BYTECODE_H_
#define COMBINATORS_BYTECODE_H_

#include <memory>
#include <string>
#include <vector>
using std::shared_ptr;
using std::string;
using std::vector;

#include "base/stl-util.h"

// #include "combinators/matchers.h"
#include "combinators/base.h"
#include "combinators/stream.h"
// #include "combinators/primitives.h"
// #include "combinators/composite.h"
#include "store/bytecode.h"
#include "store/values.h"

namespace combinators {

// -----------------------------------------------------------------------------

// Matches a register name:
//  - [lpea][0-9]+
//  - [lpea][*]
//  - exn
class RegisterParser : public Parser {
 public:
  explicit RegisterParser(Stream stream);

  const store::Register& reg() const { return reg_; }

 private:
  store::Register reg_;
};

// Parses an operand:
//   | <immediate value>
//   | <register>
class OperandParser : public Parser {
 public:
  OperandParser(Stream stream, ParsingContext* context);

  const store::Operand& operand() const {
    CHECK_EQ(OK, status());
    return operand_;
  };

 private:
  store::Operand operand_;
};

// Parses cell access: @cell
class CellAccessParser : public Parser {
 public:
  CellAccessParser(Stream stream, ParsingContext* context);

  const store::Operand& cell() const { return cell_; }

 private:
  store::Operand cell_;
};

// Parses array access: array[index]
class ArrayAccessParser : public Parser {
 public:
  ArrayAccessParser(Stream stream, ParsingContext* context);

  const store::Operand& array() const { return array_; }
  const store::Operand& index() const { return index_; }

 private:
  store::Operand array_;
  store::Operand index_;
};

// Parses record access: record.feature
class RecordAccessParser : public Parser {
 public:
  RecordAccessParser(Stream stream, ParsingContext* context);

  const store::Operand& record() const { return record_; }
  const store::Operand& feature() const { return feature_; }

 private:
  store::Operand record_;
  store::Operand feature_;
};

// Parses an extended operand:
//   | operand
//   | @cell
//   | array[index]
//   | record.feature
class ExtOperandParser : public Parser {
 public:
  ExtOperandParser(Stream stream, ParsingContext* context);

  enum Type {
    SIMPLE,
    CELL,
    ARRAY,
    RECORD
  };

  Type type() const {
    CHECK_EQ(OK, status());
    return type_;
  }

  const store::Operand& base() const {
    CHECK_EQ(OK, status());
    return base_;
  };

  const store::Operand& access() const {
    CHECK_EQ(OK, status());
    return access_;
  };

 private:
  Type type_;
  store::Operand base_;
  store::Operand access_;
};

// -----------------------------------------------------------------------------

// Parses an assignment ::=
//     <extended op> := <extended op> | <constructor>
// Combinations of lop and rop are limited.
class AssignParser : public Parser {
 public:
  AssignParser(Stream stream, ParsingContext* context);

  const store::Bytecode& code() const {
    CHECK_EQ(OK, status());
    return code_;
  }

 private:
  store::Bytecode code_;
};

// Parses a unification: <extended op> = <extended op>
// Combinations of lop and rop are limited.
class UnifyParser : public Parser {
 public:
  UnifyParser(Stream stream, ParsingContext* context);

  const store::Bytecode& code() const {
    CHECK_EQ(OK, status());
    return code_;
  }

 private:
  store::Bytecode code_;
};

// -----------------------------------------------------------------------------

// Parses a bytecode instruction ::=
// (<Variable> ':')? (<mnemonic> | <assign> | <unify>)
class InstructionParser: public Parser {
 public:
  InstructionParser(Stream stream, ParsingContext* context);

  store::Variable* label() const {
    CHECK_EQ(OK, status());
    return label_;
  }

  const store::Bytecode& code() const {
    CHECK_EQ(OK, status());
    return code_;
  }

 private:
  store::Variable* label_;
  store::Bytecode code_;
};

// Parses a bytecode segment:
// segment((Label:)? instruction ...)
class CodeSegmentParser : public Parser {
 public:
  CodeSegmentParser(Stream stream, ParsingContext* context);

  const shared_ptr<vector<store::Bytecode> >& segment() const {
    CHECK_EQ(OK, status());
    CHECK_NOTNULL(segment_.get());
    return segment_;
  }

 private:
  shared_ptr<vector<store::Bytecode> > segment_;
};

// Parses a mnemonic:
// name(key:value ...)
class MnemonicParser : public Parser {
 public:
  MnemonicParser(Stream stream, ParsingContext* context);

  const string& name() const {
    CHECK_EQ(OK, status());
    return name_;
  }

  struct Value {
    enum Type {
      OPERAND,
      BYTECODE,
    };
    Type type;

    store::Operand operand;
    shared_ptr<vector<store::Bytecode> > bytecode;

    string ToString() const {
      switch (type) {
        case OPERAND: return DebugString(operand);
        case BYTECODE: return "<bytecode segment>";
      }
      LOG(FATAL) << "Unknown mnemonic value type: " << type;
    }
  };

  const UnorderedMap<string, Value>& params() const {
    CHECK_EQ(OK, status());
    return params_;
  }

  // Sets or overrides a parameter.
  void SetParam(const string& name, Value value);

  // Builds the Bytecode for this mnemonic description.
  // @param bytecode Fills this bytecode struct.
  // @returns True on success.
  bool GetBytecode(store::Bytecode* bytecode) const;

 private:
  string name_;
  UnorderedMap<string, Value> params_;
};

// -----------------------------------------------------------------------------

// Parses a procedure declaration
class ProcParser : public Parser {
 public:
  ProcParser(Stream stream, ParsingContext* context);

  store::Closure* proc() const {
    CHECK_EQ(OK, status());
    return proc_;
  }

 private:
  store::Closure* proc_;
};

// Parses a sequence of declarations
class BytecodeSourceParser : public Parser {
 public:
  BytecodeSourceParser(Stream stream, store::Store* store);

  const ParsingContext& context() const { return context_; }

 private:
  ParsingContext context_;
};

// -----------------------------------------------------------------------------

}  // namespace combinators

#endif  // COMBINATORS_BYTECODE_H_
