#include "store/values.h"

#include <string>
using std::string;

#include <boost/format.hpp>

using boost::format;


namespace store {

uint64 Thread::next_id_ = 0;

Thread::~Thread() {
}

uint64 Thread::GetNextThreadID() {
  uint64 id = next_id_;
  ++next_id_;
  return id;
}

// @returns True if the current thread suspends on the specified value.
//     Registers the waiting thread as a suspension of the free variable.
// @param value A value that has already been dereferenced.
bool Thread::WaitOn(Value value) {
  // TODO Find a correct way to report suspensions
  if (value.type() != Value::VARIABLE) return false;
  Variable* var = value.as<Variable>();
  var->AddSuspension(this);
  return true;
}

Thread::ThreadState Thread::Run(
    uint64 steps_count,
    list<Thread*>* new_runnable) {

  for (uint64 i = 0; i < steps_count; ++i) {

    // Warning: Do not use cse after call_stack_ has been modified!
    CallStackEntry* cse = &call_stack_.back();
    if (cse->code_pointer_ >= cse->proc_->bytecode().size())
      goto terminated;
    const Bytecode& inst =
        cse->proc_->bytecode()[cse->code_pointer_];
    VLOG(3) << "Executing: "
            << (format("closure@%p cp=%d ")
                % cse->proc_ % cse->code_pointer_).str()
            << inst.GetOpcodeName();

    int32 next_code_pointer = cse->code_pointer_ + 1;

    switch (inst.opcode) {
      case Bytecode::NO_OPERATION:
        break;

      case Bytecode::LOAD: {
        RSet(inst.operand1, OpGet(inst.operand2));
        break;
      }

      case Bytecode::UNIFY: {
        const bool success =
            store::Unify(
                OpGet(inst.operand1),
                OpGet(inst.operand2),
                new_runnable);
        if (!success) {
          // TODO: throw an exception instead
          goto bad_operand;
        }
        break;
      }

      case Bytecode::TRY_UNIFY: {
        const bool success =
            store::Unify(
                OpGet(inst.operand1),
                OpGet(inst.operand2),
                new_runnable);
        RSet(inst.operand3, success ? KAtomTrue() : KAtomFalse());
        break;
      }

      case Bytecode::UNIFY_RECORD_FIELD: {
        Value record = OpGet(inst.operand1).Deref();
        if (WaitOn(record)) goto suspended;
        if (!(record.caps() & Value::CAP_RECORD)) goto bad_operand;

        Value feature = OpGet(inst.operand2).Deref();
        if (WaitOn(feature)) goto suspended;
        if (!(feature.caps() & Value::CAP_LITERAL)) goto bad_operand;

        const bool success =
            store::Unify(
                record.RecordGet(feature),
                OpGet(inst.operand3),
                new_runnable);
        if (!success) {
          // TODO: throw an exception instead
          goto bad_operand;
        }
        break;
      }

      // -----------------------------------------------------------------------
      // Control-flow

      case Bytecode::BRANCH: {
        Value bc_pointer = OpGet(inst.operand1).Deref();
        if (!HasType(bc_pointer, Value::SMALL_INTEGER)) goto bad_operand;

        next_code_pointer = SmallInteger(bc_pointer).value();
        break;
      }

      case Bytecode::BRANCH_IF: {
        Value cond_val = OpGet(inst.operand1).Deref();
        if (WaitOn(cond_val)) goto suspended;
        // if (!HasType(cond_val, Value::BOOLEAN)) goto bad_operand;
        bool cond;
        if (cond_val == KAtomTrue()) cond = true;
        else if (cond_val == KAtomFalse()) cond = false;
        else goto bad_operand;

        // The following check could be statically verified.
        Value bc_pointer = OpGet(inst.operand2).Deref();
        if (!HasType(bc_pointer, Value::SMALL_INTEGER)) goto bad_operand;

        // const bool cond = cond_val.as<Boolean>()->value();
        if (cond) {
          next_code_pointer = SmallInteger(bc_pointer).value();
        }
        break;
      }

      case Bytecode::BRANCH_UNLESS: {
        Value cond_val = OpGet(inst.operand1).Deref();
        if (WaitOn(cond_val)) goto suspended;
        // if (!HasType(cond_val, Value::BOOLEAN)) goto bad_operand;
        bool cond;
        if (cond_val == KAtomTrue()) cond = true;
        else if (cond_val == KAtomFalse()) cond = false;
        else goto bad_operand;

        // The following check could be statically verified.
        Value bc_pointer = OpGet(inst.operand2).Deref();
        if (!HasType(bc_pointer, Value::SMALL_INTEGER)) goto bad_operand;

        // const bool cond = cond_val.as<Boolean>()->value();
        if (!cond) {
          next_code_pointer = SmallInteger(bc_pointer).value();
        }
        break;
      }

      case Bytecode::BRANCH_SWITCH_LITERAL: {
        Value branches = OpGet(inst.operand2).Deref();
        if (!(branches.caps() & Value::CAP_ARITY)) goto bad_operand;

        Value value = OpGet(inst.operand1).Deref();
        if (WaitOn(value)) goto suspended;
        if (!(value.caps() & Value::CAP_LITERAL)) goto bad_operand;

        try {
          Value bc_pointer = branches.RecordGet(value).Deref();
          if (!HasType(bc_pointer, Value::SMALL_INTEGER)) goto bad_operand;
          next_code_pointer = SmallInteger(bc_pointer).value();
        } catch (FeatureNotFound) {
          // Move to next instruction
        }
        break;
      }

      case Bytecode::CALL: {
        Value closure_val = OpGet(inst.operand1).Deref();
        if (WaitOn(closure_val)) goto suspended;
        if (!HasType(closure_val, Value::CLOSURE)) goto bad_operand;
        Closure* closure = closure_val.as<Closure>();

        Value params_val = OpGet(inst.operand2).Deref();
        if (!HasType(params_val, Value::ARRAY)) goto bad_operand;
        Array* params = params_val.as<Array>();

        cse->code_pointer_ = next_code_pointer;
        call_stack_.push_back(CallStackEntry(store_, closure, params));
        // Do not use cse after call_stack_ has been modified!
        continue;
        break;
      }

      case Bytecode::RETURN: {
        const ExnStackEntry* finally_handler = NULL;
        while (!cse->exn_handlers_.empty()) {
          const ExnStackEntry& ese = cse->exn_handlers_.back();
          if (ese.type_ == ExnStackEntry::FINALLY) {
            finally_handler = &ese;
            break;
          }
          cse->exn_handlers_.pop_back();
        }
        if (finally_handler != NULL) {
          // Finally handler found: branch to it.
          next_code_pointer = finally_handler->code_pointer_;
          cse->exn_handlers_.pop_back();
        } else {
          // No finally handler in the current call: back to caller.
          call_stack_.pop_back();
          if (call_stack_.empty()) goto terminated;
          // Do not use cse after call_stack_ is modified!
          continue;
        }
        break;
      }

      case Bytecode::CALL_TAIL: {
        Value closure_val = OpGet(inst.operand1).Deref();
        if (WaitOn(closure_val)) goto suspended;
        if (!HasType(closure_val, Value::CLOSURE)) goto bad_operand;
        Closure* closure = closure_val.as<Closure>();

        Value params_val = OpGet(inst.operand2).Deref();
        if (!HasType(params_val, Value::ARRAY)) goto bad_operand;
        Array* params = params_val.as<Array>();

        cse->proc_ = closure;
        cse->parameters_ = params;
        // keep the existing cse->locals_
        cse->array_ = NULL;
        cse->exn_handlers_.clear();
        next_code_pointer = 0;
        break;
      }

      case Bytecode::CALL_NATIVE: {
        Value native_val = OpGet(inst.operand1).Deref();
        if (WaitOn(native_val)) goto suspended;
        if (!HasType(native_val, Value::ATOM)) goto bad_operand;
        const string& native_name = native_val.as<Atom>()->value();

        Value params_val = OpGet(inst.operand2).Deref();
        if (!HasType(params_val, Value::ARRAY)) goto bad_operand;
        Array* params = params_val.as<Array>();

        // TODO: Better implementation for natives?
        engine_->native_map_[native_name]->Execute(params);
        break;
      }

      // -----------------------------------------------------------------------
      // Exception handling

      case Bytecode::EXN_PUSH_CATCH: {
        Value bc_pointer_val = OpGet(inst.operand1);
        if (!HasType(bc_pointer_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 bc_pointer = SmallInteger(bc_pointer_val).value();

        cse->exn_handlers_.push_back(
            ExnStackEntry(ExnStackEntry::CATCH, bc_pointer));
        break;
      }

      case Bytecode::EXN_PUSH_FINALLY: {
        Value bc_pointer_val = OpGet(inst.operand1);
        if (!HasType(bc_pointer_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 bc_pointer = SmallInteger(bc_pointer_val).value();

        cse->exn_handlers_.push_back(
            ExnStackEntry(ExnStackEntry::FINALLY, bc_pointer));
        break;
      }

      case Bytecode::EXN_POP: {
        if (cse->exn_handlers_.empty()) goto bad_operand;
        const ExnStackEntry& ese = cse->exn_handlers_.back();
        if (ese.type_ == ExnStackEntry::FINALLY)
          // Branch to the finally block
          next_code_pointer = ese.code_pointer_;
        cse->exn_handlers_.pop_back();
        break;
      }

      case Bytecode::EXN_RERAISE: {
        Value exn_val = OpGet(inst.operand1);
        if (!exn_val.IsDetermined())
          break;  // Do not raise!
        // Fall through EXN_RAISE
      }

      case Bytecode::EXN_RAISE: {
        Value exn_val = OpGet(inst.operand1);
        if (WaitOn(exn_val)) goto suspended;

        // RSet(Operand(Register(Register::EXN)), exn_val);
        exception_ = exn_val;

        // Jump to the first reachable exception/finally handler.
        while ((cse != NULL) && cse->exn_handlers_.empty()) {
          call_stack_.pop_back();
          cse = call_stack_.empty() ? NULL : &call_stack_.back();
        }
        if (cse == NULL) {
          LOG(INFO) << "Thread terminated by uncaught exception: "
                    << exn_val.ToString();
          goto terminated;
        }
        const ExnStackEntry& ese = cse->exn_handlers_.back();
        cse->code_pointer_ = ese.code_pointer_;
        cse->exn_handlers_.pop_back();
        continue;
        break;
      }

      case Bytecode::EXN_RESET: {
        RSet(inst.operand1, exception_);
        exception_ = New::Free(store_);
        break;
      }

      // -----------------------------------------------------------------------
      // Constructors

      case Bytecode::NEW_VARIABLE: {
        RSet(inst.operand1, New::Free(store_));
        break;
      }

      case Bytecode::NEW_NAME: {
        RSet(inst.operand1, New::Name(store_));
        break;
      }

      case Bytecode::NEW_CELL: {
        Value initial_val = OpGet(inst.operand2).Deref();

        RSet(inst.operand1, New::Cell(store_, initial_val));
        break;
      }

      case Bytecode::NEW_ARRAY: {
        Value size_val = OpGet(inst.operand2).Deref();
        if (WaitOn(size_val)) goto suspended;
        if (!HasType(size_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 array_size = SmallInteger(size_val).value();

        Value initial_val = OpGet(inst.operand3).Deref();

        RSet(inst.operand1, New::Array(store_, array_size, initial_val));
        break;
      }

      case Bytecode::NEW_ARITY: {
        Value array_val = OpGet(inst.operand2).Deref();
        if (WaitOn(array_val)) goto suspended;
        if (!HasType(array_val, Value::ARRAY)) goto bad_operand;
        Array* const array = array_val.as<Array>();

        RSet(inst.operand1, New::Arity(store_, array->size(), array->values()));
        break;
      }

      case Bytecode::NEW_LIST: {
        Value head_val = OpGet(inst.operand2).Deref();
        Value tail_val = OpGet(inst.operand3).Deref();

        RSet(inst.operand1, New::List(store_, head_val, tail_val));
        break;
      }

      case Bytecode::NEW_TUPLE: {
        Value size_val = OpGet(inst.operand2).Deref();
        if (WaitOn(size_val)) goto suspended;
        if (!HasType(size_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 size = SmallInteger(size_val).value();

        Value label_val = OpGet(inst.operand3).Deref();
        if (WaitOn(label_val)) goto suspended;
        if (!(label_val.caps() & Value::CAP_LITERAL)) goto bad_operand;

        RSet(inst.operand1, New::Tuple(store_, label_val, size));
        break;
      }

      case Bytecode::NEW_RECORD: {
        Value arity_val = OpGet(inst.operand2).Deref();
        if (WaitOn(arity_val)) goto suspended;
        if (!HasType(arity_val, Value::ARITY)) goto bad_operand;
        Arity* arity = arity_val.as<Arity>();

        Value label_val = OpGet(inst.operand3).Deref();
        if (WaitOn(label_val)) goto suspended;
        if (!(label_val.caps() & Value::CAP_LITERAL)) goto bad_operand;

        RSet(inst.operand1, New::Record(store_, label_val, arity));
        break;
      }

      case Bytecode::NEW_PROC: {
        Value closure_val = OpGet(inst.operand2).Deref();
        if (WaitOn(closure_val)) goto suspended;
        if (!HasType(closure_val, Value::CLOSURE)) goto bad_operand;
        Closure* closure = closure_val.as<Closure>();

        Value env_val = OpGet(inst.operand3).Deref();
        if (!HasType(env_val, Value::ARRAY)) goto bad_operand;
        Array* env = env_val.as<Array>();

        RSet(inst.operand1, New::Closure(store_, closure, env));
        break;
      }

      case Bytecode::NEW_THREAD: {
        Value closure_val = OpGet(inst.operand2).Deref();
        if (WaitOn(closure_val)) goto suspended;
        if (!HasType(closure_val, Value::CLOSURE)) goto bad_operand;
        Closure* closure = closure_val.as<Closure>();

        Value params_val = OpGet(inst.operand3).Deref();
        if (!HasType(params_val, Value::ARRAY)) goto bad_operand;
        Array* params = params_val.as<Array>();

        RSet(inst.operand1,
             New::Thread(store_, engine_, closure, params, store_));
        break;
      }

      // -----------------------------------------------------------------------
      // Accessors

      case Bytecode::GET_VALUE_TYPE: {
        Value value = OpGet(inst.operand2).Deref();
        RSet(inst.operand1, New::Integer(store_, value.type()));
        break;
      }

      case Bytecode::ACCESS_CELL: {
        Value cell_val = OpGet(inst.operand2).Deref();
        if (WaitOn(cell_val)) goto suspended;
        if (!HasType(cell_val, Value::CELL)) goto bad_operand;
        Cell* cell = cell_val.as<Cell>();

        RSet(inst.operand1, cell->Access());
        break;
      }

      case Bytecode::ACCESS_ARRAY: {
        Value array_val = OpGet(inst.operand2).Deref();
        if (WaitOn(array_val)) goto suspended;
        if (!HasType(array_val, Value::ARRAY)) goto bad_operand;
        Array* array = array_val.as<Array>();

        Value index_val = OpGet(inst.operand3).Deref();
        if (WaitOn(index_val)) goto suspended;
        if (!HasType(index_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 index = SmallInteger(index_val).value();

        RSet(inst.operand1, array->Access(index));
        break;
      }

      case Bytecode::ACCESS_RECORD: {
        Value record = OpGet(inst.operand2).Deref();
        if (WaitOn(record)) goto suspended;
        if (!(record.caps() & Value::CAP_RECORD)) goto bad_operand;

        Value feature = OpGet(inst.operand3).Deref();
        if (WaitOn(feature)) goto suspended;
        if (!(feature.caps() & Value::CAP_LITERAL)) goto bad_operand;

        RSet(inst.operand1, record.RecordGet(feature));
        break;
      }

      case Bytecode::ACCESS_RECORD_LABEL: {
        Value record = OpGet(inst.operand2).Deref();
        if (WaitOn(record)) goto suspended;
        if (!(record.caps() & Value::CAP_RECORD)) goto bad_operand;

        RSet(inst.operand1, record.RecordLabel());
        break;
      }

      case Bytecode::ACCESS_RECORD_ARITY: {
        Value record = OpGet(inst.operand2).Deref();
        if (WaitOn(record)) goto suspended;
        if (!(record.caps() & Value::CAP_RECORD)) goto bad_operand;

        RSet(inst.operand1, record.RecordArity());
        break;
      }

      case Bytecode::ACCESS_OPEN_RECORD_ARITY: {
        Value record = OpGet(inst.operand2).Deref();
        if (WaitOn(record)) goto suspended;
        if (!(record.caps() & Value::CAP_RECORD)) goto bad_operand;

        RSet(inst.operand1, record.OpenRecordArity(store_));
        break;
      }

      // -----------------------------------------------------------------------
      // Mutations

      case Bytecode::ASSIGN_CELL: {
        Value cell_val = OpGet(inst.operand1).Deref();
        if (WaitOn(cell_val)) goto suspended;
        if (!HasType(cell_val, Value::CELL)) goto bad_operand;
        Cell* cell = cell_val.as<Cell>();

        Value new_val = OpGet(inst.operand2).Deref();

        cell->Assign(new_val);
        break;
      }

      case Bytecode::ASSIGN_ARRAY: {
        Value array_val = OpGet(inst.operand1).Deref();
        if (WaitOn(array_val)) goto suspended;
        if (!HasType(array_val, Value::ARRAY)) goto bad_operand;
        Array* const array = array_val.as<Array>();

        Value index_val = OpGet(inst.operand2).Deref();
        if (WaitOn(index_val)) goto suspended;
        if (!HasType(index_val, Value::SMALL_INTEGER)) goto bad_operand;
        const uint64 index = SmallInteger(index_val).value();

        Value new_val = OpGet(inst.operand3).Deref();

        array->Assign(index, new_val);
        break;
      }

      // -----------------------------------------------------------------------
      // Predicates

      case Bytecode::TEST_IS_DET: {
        Value value = OpGet(inst.operand2).Deref();
        RSet(inst.operand1, Boolean::Get(store::IsDet(value)));
        break;
      }

      case Bytecode::TEST_IS_RECORD: {
        Value value = OpGet(inst.operand2).Deref();
        RSet(inst.operand1, Boolean::Get(value.caps() & Value::CAP_RECORD));
        break;
      }

      case Bytecode::TEST_ARITY_EXTENDS: {
        Value super_val = OpGet(inst.operand2).Deref();
        if (WaitOn(super_val)) goto suspended;
        if (!HasType(super_val, Value::ARITY)) goto bad_operand;
        Value sub_val = OpGet(inst.operand3).Deref();
        if (WaitOn(sub_val)) goto suspended;
        if (!HasType(sub_val, Value::ARITY)) goto bad_operand;
        Arity* const super = super_val.as<Arity>();
        Arity* const sub = sub_val.as<Arity>();
        RSet(inst.operand1, Boolean::Get(sub->LessThan(super)));
        break;
      }

      case Bytecode::TEST_EQUALITY: {
        Value value1 = OpGet(inst.operand2).Deref();
        Value value2 = OpGet(inst.operand3).Deref();
        RSet(inst.operand1, Boolean::Get(store::Equals(value1, value2)));
        break;
      }

      case Bytecode::TEST_LESS_THAN: {
        Value value1 = OpGet(inst.operand2).Deref();
        if (WaitOn(value1)) goto suspended;
        if (!(value1.caps() & Value::CAP_LITERAL)) goto bad_operand;
        Value value2 = OpGet(inst.operand3).Deref();
        if (WaitOn(value2)) goto suspended;
        if (!(value2.caps() & Value::CAP_LITERAL)) goto bad_operand;
        RSet(inst.operand1, Boolean::Get(value1.LiteralLessThan(value2)));
        break;
      }

      case Bytecode::TEST_LESS_OR_EQUAL: {
        Value value1 = OpGet(inst.operand2).Deref();
        if (WaitOn(value1)) goto suspended;
        if (!(value1.caps() & Value::CAP_LITERAL)) goto bad_operand;
        Value value2 = OpGet(inst.operand3).Deref();
        if (WaitOn(value2)) goto suspended;
        if (!(value2.caps() & Value::CAP_LITERAL)) goto bad_operand;
        const bool less_or_equal =
            value1.LiteralLessThan(value2) || value1.LiteralEquals(value2);
        RSet(inst.operand1, Boolean::Get(less_or_equal));
        break;
      }

      case Bytecode::NUMBER_INT_INVERSE: {
        Value number1 = OpGet(inst.operand2).Deref();
        if (WaitOn(number1)) goto suspended;

        // TODO: handle big integers
        RSet(inst.operand1, Value::Integer(-IntValue(number1)));
        break;
      }

      case Bytecode::NUMBER_INT_ADD: {
        Value number1 = OpGet(inst.operand2).Deref();
        if (WaitOn(number1)) goto suspended;

        Value number2 = OpGet(inst.operand3).Deref();
        if (WaitOn(number2)) goto suspended;

        // TODO: handle big integers
        RSet(inst.operand1,
             Value::Integer(IntValue(number1) + IntValue(number2)));
        break;
      }

      case Bytecode::NUMBER_INT_SUBTRACT: {
        Value number1 = OpGet(inst.operand2).Deref();
        if (WaitOn(number1)) goto suspended;

        Value number2 = OpGet(inst.operand3).Deref();
        if (WaitOn(number2)) goto suspended;

        // TODO: handle big integers
        RSet(inst.operand1,
             Value::Integer(IntValue(number1) - IntValue(number2)));
        break;
      }

      case Bytecode::NUMBER_INT_MULTIPLY: {
        Value number1 = OpGet(inst.operand2).Deref();
        if (WaitOn(number1)) goto suspended;

        Value number2 = OpGet(inst.operand3).Deref();
        if (WaitOn(number2)) goto suspended;

        // TODO: handle big integers
        RSet(inst.operand1,
             Value::Integer(IntValue(number1) * IntValue(number2)));
        break;
      }

      case Bytecode::NUMBER_INT_DIVIDE: {
        Value number1 = OpGet(inst.operand2).Deref();
        if (WaitOn(number1)) goto suspended;

        Value number2 = OpGet(inst.operand3).Deref();
        if (WaitOn(number2)) goto suspended;

        // TODO: handle big integers
        RSet(inst.operand1,
             Value::Integer(IntValue(number1) / IntValue(number2)));
        break;
      }

      case Bytecode::NUMBER_BOOL_NEGATE: {
        Value boolean = OpGet(inst.operand2).Deref();
        if (WaitOn(boolean)) goto suspended;

        Value negated;
        if (boolean == KAtomTrue()) {
          negated = KAtomFalse();
        } else if (boolean == KAtomFalse()) {
          negated = KAtomTrue();
        } else {
          goto bad_operand;
        }
        RSet(inst.operand1, negated);
        break;
      }

      case Bytecode::NUMBER_BOOL_AND_THEN: {
        Value bool1 = OpGet(inst.operand2).Deref();
        if (WaitOn(bool1)) goto suspended;

        if (bool1 == KAtomTrue()) {
          // Move on
        } else if (bool1 == KAtomFalse()) {
          RSet(inst.operand1, KAtomFalse());
        } else {
          goto bad_operand;
        }

        Value bool2 = OpGet(inst.operand3).Deref();
        if (WaitOn(bool2)) goto suspended;

        if ((bool2 != KAtomTrue()) && (bool2 != KAtomFalse())) {
          goto bad_operand;
        }
        RSet(inst.operand1, bool2);
        break;
      }

      case Bytecode::NUMBER_BOOL_OR_ELSE: {
        Value bool1 = OpGet(inst.operand2).Deref();
        if (WaitOn(bool1)) goto suspended;

        if (bool1 == KAtomTrue()) {
          RSet(inst.operand1, KAtomTrue());
        } else if (bool1 == KAtomFalse()) {
          // Move on
        } else {
          goto bad_operand;
        }

        Value bool2 = OpGet(inst.operand3).Deref();
        if (WaitOn(bool2)) goto suspended;

        if ((bool2 != KAtomTrue()) && (bool2 != KAtomFalse())) {
          goto bad_operand;
        }
        RSet(inst.operand1, bool2);
        break;
      }

      case Bytecode::NUMBER_BOOL_XOR: {
        Value bool1 = OpGet(inst.operand2).Deref();
        if (WaitOn(bool1)) goto suspended;

        if ((bool1 != KAtomTrue()) && (bool1 != KAtomFalse())) {
          goto bad_operand;
        }

        Value bool2 = OpGet(inst.operand3).Deref();
        if (WaitOn(bool2)) goto suspended;

        if ((bool2 != KAtomTrue()) && (bool2 != KAtomFalse())) {
          goto bad_operand;
        }

        Value xored = (bool1 == bool2) ? KAtomFalse() : KAtomTrue();
        RSet(inst.operand1, xored);
        break;
      }

      // -----------------------------------------------------------------------

      default:
        LOG(FATAL) << "Unknown opcode " << inst.opcode;

    }  // switch (inst.opcode)

    // This is executed by all instructions that do not modify the call_stack_
    // explicitly. After call_stack_ is modified (e.g. by push_back()), cse
    // may be invalid.
    cse->code_pointer_ = next_code_pointer;

  }  // for loop

  return RUNNABLE;

  //----------------------------------------------------------------------------

suspended:  // The thread is suspended on a variable.
  LOG(INFO) << "Thread " << id_ << " suspended";
  return WAITING;

bad_operand:  // An operation encountered a bad operand.
  LOG(INFO) << "Thread " << id_ << " terminated: bad operand at CP="
            << call_stack_.back().code_pointer_;
  return TERMINATED;

terminated:  // The thread is terminated.
  LOG(INFO) << "Thread " << id_ << " terminated";
  return TERMINATED;
}

}  // namespace store
