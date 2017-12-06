#include "store/values.h"
#include "combinators/bytecode.h"

using store::Bytecode;
using store::Operand;
using store::Register;

namespace combinators {

int64 StrToInt(StringPiece s, int base = 10) {
  char* end = NULL;
  int64 value = strtol(s.data(), &end, base);
  CHECK_EQ(s.data() + s.size(), end);
  return value;
}

// -----------------------------------------------------------------------------

char kExceptionRegister[] = "exn";

RegisterParser::RegisterParser(Stream stream)
    : Parser(stream) {
  RegexParser indexed(stream, "[lpea][0-9]+");
  if (indexed.status() == OK) {
    switch (stream.Get()) {
      case 'l': reg_.type = Register::LOCAL; break;
      case 'p': reg_.type = Register::PARAM; break;
      case 'e': reg_.type = Register::ENVMT; break;
      case 'a': reg_.type = Register::ARRAY; break;
    }
    reg_.index = StrToInt(indexed.GetMatch().substr(1));
    SetOK(indexed);
    return;
  }
  RegexParser array(stream, "[lpae][*]");
  if (array.status() == OK) {
    switch (stream.Get()) {
      case 'l': reg_.type = Register::LOCAL_ARRAY; break;
      case 'p': reg_.type = Register::PARAM_ARRAY; break;
      case 'e': reg_.type = Register::ENVMT_ARRAY; break;
      case 'a': reg_.type = Register::ARRAY_ARRAY; break;
    }
    SetOK(array);
    return;
  }
  StringMatcher<kExceptionRegister> exn_reg(stream);
  if (exn_reg.status() == OK) {
    reg_.type = Register::EXN;
    SetOK(exn_reg);
    return;
  }
  SetFailure();
}

OperandParser::OperandParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  RegisterParser reg(stream);
  if (reg.status() == OK) {
    operand_ = Operand(reg.reg());
    SetOK(reg);
    return;
  }
  ValueParser value(stream, context);
  if (value.status() == OK) {
    operand_ = Operand(value.OzValue());
    SetOK(value);
    return;
  }
  SetFailure();
}

CellAccessParser::CellAccessParser(Stream stream,
                                   ParsingContext* context)
    : Parser(stream) {
  CharMatcher<'@'> cell_access(stream);
  if (cell_access.status() != OK) {
    SetFailure();
    return;
  }
  OperandParser cell_operand(cell_access.Next(), context);
  if (cell_operand.status() != OK) {
    SetFailure();
    return;
  }

  cell_ = cell_operand.operand();
  SetOK(cell_operand);
}

ArrayAccessParser::ArrayAccessParser(Stream stream,
                                     ParsingContext* context)
    : Parser(stream) {
  OperandParser array(stream, context);
  if (array.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<'['> lbracket(array.Next());
  if (lbracket.status() != OK) {
    SetFailure();
    return;
  }
  OperandParser index(SpaceConsumer(lbracket.Next()).Next(), context);
  if (index.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<']'> rbracket(SpaceConsumer(index.Next()).Next());
  if (rbracket.status() != OK) {
    SetFailure();
    return;
  }

  array_ = array.operand();
  index_ = index.operand();
  SetOK(rbracket);
}

RecordAccessParser::RecordAccessParser(Stream stream,
                                       ParsingContext* context)
    : Parser(stream) {
  OperandParser record(stream, context);
  if (record.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<'.'> dot(record.Next());
  if (dot.status() != OK) {
    SetFailure();
    return;
  }
  OperandParser feature(dot.Next(), context);
  if (feature.status() != OK) {
    SetFailure();
    return;
  }

  record_ = record.operand();
  feature_ = feature.operand();
  SetOK(feature);
}

ExtOperandParser::ExtOperandParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  CellAccessParser cell_access(stream, context);
  if (cell_access.status() == OK) {
    type_ = CELL;
    base_ = cell_access.cell();
    SetOK(cell_access);
    return;
  }
  ArrayAccessParser array_access(stream, context);
  if (array_access.status() == OK) {
    type_ = ARRAY;
    base_ = array_access.array();
    access_ = array_access.index();
    SetOK(array_access);
    return;
  }
  RecordAccessParser record_access(stream, context);
  if (record_access.status() == OK) {
    type_ = RECORD;
    base_ = record_access.record();
    access_ = record_access.feature();
    SetOK(record_access);
    return;
  }
  OperandParser operand(stream, context);
  if (operand.status() == OK) {
    type_ = SIMPLE;
    base_ = operand.operand();
    SetOK(operand);
    return;
  }
  SetFailure();
}

// -----------------------------------------------------------------------------

extern char kAssign[];
char kAssign[] = ":=";

AssignParser::AssignParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  ExtOperandParser lop(stream, context);
  if (lop.status() != OK) {
    SetFailure();
    return;
  }
  StringMatcher<kAssign> assign(SpaceConsumer(lop.Next()).Next());
  if (assign.status() != OK) {
    SetFailure();
    return;
  }

  Stream next = SpaceConsumer(assign.Next()).Next();

  MnemonicParser constructor(next, context);
  if (constructor.status() == OK) {
    if (lop.type() != ExtOperandParser::SIMPLE) {
      LOG(INFO) << "Rejecting constructor assignment with extended operator.";
      SetFailure();
      return;
    }
    // Sets the implicit "in" parameter of the constructor mnemonic.
    MnemonicParser::Value dest;
    dest.type = MnemonicParser::Value::OPERAND;
    dest.operand = lop.base();
    constructor.SetParam("in", dest);

    if (!constructor.GetBytecode(&code_))
      SetFailure();
    else
      SetOK(constructor);
    return;
  }

  ExtOperandParser rop(next, context);
  if (rop.status() == OK) {
    if ((lop.type() != ExtOperandParser::SIMPLE)
        && (rop.type() != ExtOperandParser::SIMPLE)) {
      LOG(INFO) << "Rejecting assignment with extended operator.";
      SetFailure();
      return;
    }
    switch (lop.type()) {
      case ExtOperandParser::SIMPLE: {
        if (lop.base().type != Operand::REGISTER) {
          LOG(INFO) << "Rejecting assignment with immediate left-value.";
          SetFailure();
          return;
        }
        switch (rop.type()) {
          case ExtOperandParser::SIMPLE: {
            code_ = Bytecode(Bytecode::LOAD, lop.base(), rop.base());
            break;
          }
          case ExtOperandParser::CELL: {
            code_ = Bytecode(
                Bytecode::ACCESS_CELL, lop.base(), rop.base());
            break;
          }
          case ExtOperandParser::ARRAY: {
            code_ = Bytecode(
                Bytecode::ACCESS_ARRAY, lop.base(), rop.base(), rop.access());
            break;
          }
          case ExtOperandParser::RECORD: {
            code_ = Bytecode(
                Bytecode::ACCESS_RECORD, lop.base(), rop.base(), rop.access());
            break;
          }
        }
        break;
      }
      case ExtOperandParser::CELL: {
        CHECK_EQ(ExtOperandParser::SIMPLE, rop.type());
        code_ = Bytecode(Bytecode::ASSIGN_CELL, lop.base(), rop.base());
        break;
      }
      case ExtOperandParser::ARRAY: {
        CHECK_EQ(ExtOperandParser::SIMPLE, rop.type());
        code_ = Bytecode(
            Bytecode::ASSIGN_ARRAY, lop.base(), lop.access(), rop.base());
        break;
      }
      case ExtOperandParser::RECORD: {
        LOG(INFO) << "Rejecting assignment with extended operator.";
        SetFailure();
        return;
      }
    }
    SetOK(rop);
    return;
  }

  SetFailure();
}

UnifyParser::UnifyParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  ExtOperandParser lop(stream, context);
  if (lop.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<'='> unify(SpaceConsumer(lop.Next()).Next());
  if (unify.status() != OK) {
    SetFailure();
    return;
  }
  ExtOperandParser rop(SpaceConsumer(unify.Next()).Next(), context);
  if (rop.status() != OK) {
    SetFailure();
    return;
  }

  // TODO: Add bytecode instruction for this.
  if ((lop.type() != ExtOperandParser::SIMPLE)
      || (rop.type() != ExtOperandParser::SIMPLE)) {
    LOG(INFO) << "Rejecting unification with extended operator.";
    SetFailure();
    return;
  }
  code_ = Bytecode(Bytecode::UNIFY, rop.base(), lop.base());
  SetOK(rop);
}

// -----------------------------------------------------------------------------

InstructionParser::InstructionParser(Stream stream, ParsingContext* context)
    : Parser(stream),
      label_(NULL) {
  VLOG(3) << __PRETTY_FUNCTION__ << ": " << stream;
  Stream next = SpaceAndCommentConsumer(stream).Next();
  SimpleVariableNameParser label(next);
  if (label.status() == OK) {
    CharMatcher<':'> separator(SpaceConsumer(label.Next()).Next());
    if (separator.status() != OK) {
      SetFailure();
      return;
    }
    next = SpaceAndCommentConsumer(separator.Next()).Next();

    const string& var = label.GetVariableName();
    store::Value* value = &context->variable[var];
    if (*value == NULL)
      *value = store::Variable::New(context->store);
    label_ = value->as<store::Variable>();
    CHECK(!store::IsDet(label_))
        << "Automatic labels cannot be determined already.";
  }
  MnemonicParser mnemonic(next, context);
  if (mnemonic.status() == OK) {
    if (mnemonic.GetBytecode(&code_)) {
      SetOK(mnemonic);
      return;
    }
  }
  AssignParser assign(next, context);
  if (assign.status() == OK) {
    code_ = assign.code();
    SetOK(assign);
    return;
  }
  UnifyParser unify(next, context);
  if (unify.status() == OK) {
    code_ = unify.code();
    SetOK(unify);
    return;
  }
  SetFailure();
}

extern char kSegment[];
char kSegment[] = "segment";

CodeSegmentParser::CodeSegmentParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  StringMatcher<kSegment> name(stream);
  if (name.status() != OK) {
    SetFailure();
    return;
  }
  // TODO: Optional parameters.
  CharMatcher<'('> lparen(name.Next());
  if (lparen.status() != OK) {
    SetFailure();
    return;
  }
  ListParser<InstructionParser> insts(lparen.Next(), context);
  if (insts.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<')'> rparen(SpaceConsumer(insts.Next()).Next());
  if (rparen.status() != OK) {
    SetFailure();
    return;
  }

  vector<store::Bytecode>* segment =
      new vector<store::Bytecode>(insts.elements().size());
  for (uint i = 0; i < insts.elements().size(); ++i) {
    const InstructionParser& inst_parser = *insts.elements()[i];
    store::Variable* label = inst_parser.label();
    if (label != NULL)
      label->BindTo(store::Value::Integer(context->store, i));
    segment->at(i) = inst_parser.code();
  }
  segment_.reset(segment);

  SetOK(rparen);
}

// -----------------------------------------------------------------------------

// Parses a mnemonic parameter ::= <key> ':' <value>
class ParamParser: public Parser {
 public:
  ParamParser(Stream stream, ParsingContext* context);

  // @returns The param name.
  const string name() const {
    CHECK_EQ(OK, status());
    return name_;
  }

  // @returns The operand value.
  const MnemonicParser::Value& value() const {
    CHECK_EQ(OK, status());
    return value_;
  }

 private:
  string name_;
  MnemonicParser::Value value_;
};

ParamParser::ParamParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  VLOG(3) << __PRETTY_FUNCTION__ << ": " << stream;
  SimpleAtomParser name(SpaceConsumer(stream).Next());
  if (name.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<':'> separator(SpaceConsumer(name.Next()).Next());
  if (separator.status() != OK) {
    SetFailure();
    return;
  }

  Stream next = SpaceConsumer(separator.Next()).Next();
  name_ = name.atom();

  CodeSegmentParser seg_parser(next, context);
  if (seg_parser.status() == OK) {
    value_.type = MnemonicParser::Value::BYTECODE;
    value_.bytecode = seg_parser.segment();
    SetOK(seg_parser);
    return;
  }

  OperandParser operand(next, context);
  if (operand.status() == OK) {
    value_.type = MnemonicParser::Value::OPERAND;
    value_.operand = operand.operand();
    SetOK(operand);
    return;
  }

  SetFailure();
}

MnemonicParser::MnemonicParser(Stream stream, ParsingContext* context)
    : Parser(stream) {
  SimpleAtomParser name(stream);
  // TODO: Check the mnemonic name.
  if (name.status() != OK) {
    SetFailure();
    return;
  }
  // TODO: Optional parameters.
  CharMatcher<'('> lparen(name.Next());
  if (lparen.status() != OK) {
    SetFailure();
    return;
  }
  ListParser<ParamParser> params_parser(lparen.Next(), context);
  if (params_parser.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<')'> rparen(SpaceConsumer(params_parser.Next()).Next());
  if (rparen.status() != OK) {
    SetFailure();
    return;
  }

  name_ = name.atom();
  for (uint i = 0; i < params_parser.elements().size(); ++i) {
    const ParamParser& param_parser = *params_parser.elements()[i];
    if (ContainsKey(params_, param_parser.name())) {
      LOG(INFO) << "Duplicate mnemonic parameter: " << param_parser.name();
      SetFailure();
      return;
    }
    VLOG(1)
        << "Mnemonic param " << name_ << "." << param_parser.name()
        << " = " << param_parser.value().ToString();
    params_[param_parser.name()] = param_parser.value();
  }
  SetOK(rparen);
}

void MnemonicParser::SetParam(const string& name, Value value) {
  params_[name] = value;
}

bool MnemonicParser::GetBytecode(Bytecode* bytecode) const {
  CHECK_EQ(OK, status());
  CHECK_NOTNULL(bytecode);
  if (!store::kOpcodeSpecs.contains(name_)) {
    LOG(INFO) << "Unknown mnemonic name: " << name_;
    return false;
  }
  const store::OpcodeSpec& cons_spec = store::kOpcodeSpecs[name_];
  const uint64 kMaxOperands = 3;
  Operand operand[kMaxOperands];
  CHECK_LE(cons_spec.params.size(), kMaxOperands);
  for (uint i = 0; i < cons_spec.params.size(); ++i) {
    if (!params_.contains(cons_spec.params[i])) {
      VLOG(1) << "Mnemonic: " << name_
              << " is missing parameter: " << cons_spec.params[i];
      return false;
    }
    const Value& value = params_[cons_spec.params[i]];
    if (value.type != MnemonicParser::Value::OPERAND) {
      VLOG(1) << "Mnemonic: " << name_
              << " has bad parameter: " << cons_spec.params[i];
      return false;
    }
    operand[i] = value.operand;
  }

  *bytecode = Bytecode(cons_spec.opcode, operand[0], operand[1], operand[2]);
  return true;
}

// -----------------------------------------------------------------------------

ProcParser::ProcParser(Stream stream, ParsingContext* context)
    : Parser(stream), proc_(NULL) {
  MnemonicParser mnemonic(stream, context);
  if ((mnemonic.status() != OK) || (mnemonic.name() != "proc")) {
    SetFailure();
    return;
  }

  const string kBytecodeParam = "bytecode";
  const string kNParams = "nparams";
  const string kNLocals = "nlocals";
  const string kNClosures = "nclosures";

  const MnemonicParser::Value* bc =
      FindOrNull(mnemonic.params(), kBytecodeParam);
  if ((bc == NULL) || (bc->type != MnemonicParser::Value::BYTECODE)) {
    LOG(INFO) << "Discarding proc mnemonic missing or bad bytecode.";
    SetFailure();
    return;
  }

  int nparams = 0;
  if (ContainsKey(mnemonic.params(), kNParams)) {
    Operand op = GetExisting(mnemonic.params(), kNParams).operand;
    if (op.type != Operand::IMMEDIATE) {
      LOG(INFO) << "Discarding proc mnemonic with bad nparams.";
      SetFailure();
      return;
    }
    nparams = store::IntValue(op.value);
  }

  int nlocals = 0;
  if (ContainsKey(mnemonic.params(), kNLocals)) {
    Operand op = GetExisting(mnemonic.params(), kNLocals).operand;
    if (op.type != Operand::IMMEDIATE) {
      LOG(INFO) << "Discarding proc mnemonic with bad nlocals.";
      SetFailure();
      return;
    }
    nlocals = store::IntValue(op.value);
  }

  int nclosures = 0;
  if (ContainsKey(mnemonic.params(), kNClosures)) {
    Operand op = GetExisting(mnemonic.params(), kNClosures).operand;
    if (op.type != Operand::IMMEDIATE) {
      LOG(INFO) << "Discarding proc mnemonic with bad nclosures.";
      SetFailure();
      return;
    }
    nclosures = store::IntValue(op.value);
  }

  proc_ = store::New::Closure(
      context->store, bc->bytecode, nparams, nlocals, nclosures)
      .as<store::Closure>();

  SetOK(mnemonic);
}

// -----------------------------------------------------------------------------

// Parses a bytecode statement ::= Variable '=' <value>
class BytecodeStatementParser : public Parser {
 public:
  BytecodeStatementParser(Stream stream, ParsingContext* context);

  const string& variable_name() const { return variable_name_; }

 private:
  string variable_name_;
};

BytecodeStatementParser::BytecodeStatementParser(Stream stream,
                                                 ParsingContext* context)
    : Parser(stream) {
  VariableNameParser var_name(SpaceAndCommentConsumer(stream).Next());
  if (var_name.status() != OK) {
    SetFailure();
    return;
  }
  CharMatcher<'='> equals(SpaceConsumer(var_name.Next()).Next());
  if (equals.status() != OK) {
    SetFailure();
    return;
  }

  Stream next(SpaceConsumer(equals.Next()).Next());
  store::Value value;

  ProcParser proc_parser(next, context);
  if (proc_parser.status() == OK) {
    value = proc_parser.proc();
    SetOK(proc_parser);
  } else {
    ValueParser value_parser(next, context);
    if (value_parser.status() == OK) {
      value = value_parser.OzValue();
      SetOK(value_parser);
    } else {
      SetFailure();
      return;
    }
  }
  CHECK(value.IsDefined());

  variable_name_ = var_name.GetVariableName();

  NamedValueMap* value_map = &context->variable;
  NamedValueMap::const_iterator it =
      value_map->find(variable_name_);
  if (it == value_map->end()) {
    value_map->insert_new(variable_name_, value);
  } else {
    store::Value named_value = it->second;
    CHECK(store::Unify(named_value, value));
  }
}

BytecodeSourceParser::BytecodeSourceParser(Stream stream, store::Store* store)
    : Parser(stream),
      context_(store) {
  ListParser<BytecodeStatementParser> stmts(stream, &context_);
  if (stmts.status() != OK) {
    SetFailure();
    return;
  }
  SetOK(stmts);
}

// -----------------------------------------------------------------------------

}  // namespace combinators
