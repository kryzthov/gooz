#include "store/values.h"

#include <boost/format.hpp>
using boost::format;

#include "store/bytecode.h"
#include "store/thread.h"

namespace store {

// -----------------------------------------------------------------------------
// Closure

const Value::ValueType Closure::kType;

Closure::Closure(const shared_ptr<vector<Bytecode> >& bytecode,
                 int nparams, int nlocals, int nclosures)
    : bytecode_(bytecode),
      nparams_(nparams),
      nlocals_(nlocals),
      nclosures_(nclosures),
      environment_(NULL) {
  CHECK_NOTNULL(bytecode.get());
}

Closure::Closure(const Closure* closure, Array* environment)
    : bytecode_(CHECK_NOTNULL(closure)->bytecode_),
      nparams_(closure->nparams_),
      nlocals_(closure->nlocals_),
      nclosures_(CHECK_NOTNULL(environment)->size()),
      environment_(environment) {
  CHECK(closure->environment_ == NULL);
  CHECK_NOTNULL(bytecode_.get());
}

Closure::~Closure() {
}

// virtual
void Closure::ExploreValue(ReferenceMap* ref_map) {
  CHECK_NOTNULL(ref_map);
  if (environment_ != NULL)
    Value(environment_).Explore(ref_map);
  for (uint64 i = 0; i < bytecode_->size(); ++i) {
    ExploreOperand(bytecode_->at(i).operand1, ref_map);
    ExploreOperand(bytecode_->at(i).operand2, ref_map);
    ExploreOperand(bytecode_->at(i).operand3, ref_map);
  }
}

void Closure::ExploreOperand(const Operand& op, ReferenceMap* ref_map) {
  if (op.type != Operand::IMMEDIATE) return;
  op.value.Explore(ref_map);
}

// virtual
Value Closure::Optimize(OptimizeContext* context) {
  for (uint64 i = 0; i < bytecode_->size(); ++i) {
    OptimizeOperand(&bytecode_->at(i).operand1, context);
    OptimizeOperand(&bytecode_->at(i).operand2, context);
    OptimizeOperand(&bytecode_->at(i).operand3, context);
  }
  // TODO: Clarify environment_ being NULL-able or not
  if (environment_ != NULL)
    CHECK(context->Optimize(environment_) == environment_);
  return this;
}

void Closure::OptimizeOperand(Operand* op, OptimizeContext* context) {
  if (op->type != Operand::IMMEDIATE) return;
  op->value = context->Optimize(op->value);
}

void Closure::ToASCII(ToASCIIContext* context, string* repr) {
  CHECK_NOTNULL(context);
  CHECK_NOTNULL(repr);
  repr->append("proc(");
  repr->append((format("nlocals:%d") % nlocals_).str());
  repr->append((format(" nparams:%d") % nparams_).str());
  repr->append((format(" nclosures:%d") % nclosures_).str());
  if (environment_ != NULL) {
    repr->append(" environment:values(");
    for (uint64 i = 0; i < environment_->size(); ++i) {
      if (i > 0) repr->append(" ");
      context->Encode(environment_->Access(i), repr);
    }
    repr->append(")");
  }
  repr->append(" bytecode:segment(\n");
  for (uint64 i = 0; i < bytecode_->size(); ++i) {
    repr->append((format("%d:") % i).str());
    bytecode_->at(i).ToASCII(context, repr);
    repr->append("\n");
  }
  repr->append(")");
  repr->append(")");
}

void Closure::ToProtoBuf(oz_pb::Value* pb) {
  CHECK_NOTNULL(pb);
  LOG(FATAL) << "Not implemented";
}

// -----------------------------------------------------------------------------

}  // namespace store
