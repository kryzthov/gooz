#ifndef STORE_THREAD_INL_H_
#define STORE_THREAD_INL_H_

namespace store {

// static
inline
Thread* Thread::New(Store* store,
                    Engine* engine,
                    Closure* closure,
                    Array* parameters,
                    Store* thread_store) {
  return new(CHECK_NOTNULL(store->Alloc<Thread>()))
      Thread(engine, closure, parameters, thread_store);
}

inline
Thread::Thread(Engine* engine,
               Closure* closure,
               Array* parameters,
               Store* store)
    : id_(GetNextThreadID()),
      engine_(engine),
      store_(CHECK_NOTNULL(store)),
      exception_(New::Free(store)) {
  CHECK_NOTNULL(closure);
  CHECK_NOTNULL(parameters);
  engine_->AddThread(this);
  call_stack_.push_back(CallStackEntry(store_, closure, parameters));
}

inline
Value Thread::RGet(const Register& reg) {
  switch (reg.type) {
    case Register::LOCAL:
      return call_stack_.back().locals_->Access(reg.index);
    case Register::PARAM:
      return call_stack_.back().parameters_->Access(reg.index);
    case Register::ENVMT:
      return call_stack_.back().proc_->environment()->Access(reg.index);
    case Register::ARRAY:
      return call_stack_.back().array_->Access(reg.index);
    case Register::LOCAL_ARRAY:
      return call_stack_.back().locals_;
    case Register::PARAM_ARRAY:
      return call_stack_.back().parameters_;
    case Register::ENVMT_ARRAY:
      return call_stack_.back().proc_->environment();
    case Register::ARRAY_ARRAY:
      return call_stack_.back().array_;
    case Register::EXN:
      return exception_;
    default:
      LOG(FATAL) << "Unknown register type " << reg.type;
  }
}

inline
void Thread::RSet(const Register& reg, Value value) {
  switch (reg.type) {
    case Register::LOCAL: {
      call_stack_.back().locals_->Assign(reg.index, value);
      break;
    }
    case Register::PARAM: {
      call_stack_.back().parameters_->Assign(reg.index, value);
      break;
    }
    case Register::ENVMT: {
      // TODO: Better error reporting.
      LOG(FATAL) << "Modifying and environment register.";
      // call_stack_.back().proc_ ->environment()->Assign(reg.index, value);
    }
    case Register::ARRAY: {
      call_stack_.back().array_->Assign(reg.index, value);

      break;
    }
    case Register::LOCAL_ARRAY: {
      call_stack_.back().locals_ = value.as<Array>();
      break;
    }
    case Register::PARAM_ARRAY: {
      call_stack_.back().parameters_ = value.as<Array>();
      break;
    }
    case Register::ENVMT_ARRAY: {
      // TODO: Better error reporting.
      LOG(FATAL) << "Modifying the environment array.";
      // call_stack_.back().proc_.environment_ = value.as<Array>();
      break;
    }
    case Register::ARRAY_ARRAY: {
      call_stack_.back().array_ = value.as<Array>();
      break;
    }
    case Register::EXN: {
      exception_ = value;
      break;
    }
    default:
      LOG(FATAL) << "Unknown register type " << reg.type;
  }
}

inline
void Thread::RSet(const Operand& op, Value value) {
  CHECK_EQ(op.type, Operand::REGISTER);
  RSet(op.reg, value);
}

inline
Value Thread::OpGet(const Operand& operand) {
  switch (operand.type) {
    case Operand::REGISTER: return RGet(operand.reg);
    case Operand::IMMEDIATE: return operand.value;
    case Operand::INVALID: LOG(FATAL) << "Invalid operand";
    default: LOG(FATAL) << "Unknown operand type " << operand.type;
  }
}

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_THREAD_INL_H_
