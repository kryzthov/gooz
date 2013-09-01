// Representation of an Oz thread
#ifndef STORE_THREAD_H_
#define STORE_THREAD_H_

#include <list>
#include <string>
#include <vector>
using std::list;
using std::string;
using std::vector;

#include <boost/format.hpp>

#include "store/engine.h"

namespace store {

// -----------------------------------------------------------------------------

struct Register {
  enum RegisterType {
    INVALID = -1,
    LOCAL,        // Local registers
    PARAM,        // Parameters
    ENVMT,        // Closure registers
    ARRAY,        // Array content
    LOCAL_ARRAY,  // Array of the local registers
    PARAM_ARRAY,  // Array of the parameters
    ENVMT_ARRAY,  // Array of the environment values
    ARRAY_ARRAY,  // Array
    EXN,          // Exception register
    REGISTER_TYPE_COUNT
  };

  Register() : type(INVALID), index(-1) {}
  Register(RegisterType ptype, int pindex = 0) : type(ptype), index(pindex) {}

  bool operator==(const Register& that) const {
    return (type == that.type)
        && (index == that.index);
  }

  RegisterType type;
  int index;
};

inline
string DebugString(const Register& reg) {
  switch (reg.type) {
    case Register::INVALID: return "<invalid register>";
    case Register::LOCAL: return (boost::format("l%d") % reg.index).str();
    case Register::PARAM: return (boost::format("p%d") % reg.index).str();
    case Register::ENVMT: return (boost::format("e%d") % reg.index).str();
    case Register::ARRAY: return (boost::format("a%d") % reg.index).str();
    case Register::LOCAL_ARRAY: return "l*";
    case Register::PARAM_ARRAY: return "p*";
    case Register::ENVMT_ARRAY: return "e*";
    case Register::ARRAY_ARRAY: return "a*";
    case Register::EXN: return "exn";
    default:
      LOG(FATAL) << "Unknown register type: " << reg.type;
  }
}

// -----------------------------------------------------------------------------

struct Operand {
  enum OperandType {
    INVALID,
    REGISTER,
    IMMEDIATE,
  };

  Operand() : type(INVALID) {
  }

  explicit Operand(Register preg)
      : type(REGISTER),
        reg(preg) {
  }

  explicit Operand(Value pvalue)
      : type(IMMEDIATE),
        value(pvalue) {
  }

  bool invalid() const { return type == INVALID; }

  bool operator==(const Operand& that) const {
    return (type == that.type)
        && (reg == that.reg)
        && (value == that.value);
  }

  bool operator!=(const Operand& that) const { return !operator==(that); }

  OperandType type;
  Register reg;
  Value value;
};

inline
string DebugString(const Operand& op) {
  switch (op.type) {
    case Operand::INVALID: return "<invalid operand>";
    case Operand::REGISTER: return DebugString(op.reg);
    case Operand::IMMEDIATE: return op.value.ToString();
  }
  LOG(FATAL) << "Unknown operand type: " << op.type;
}

// -----------------------------------------------------------------------------

class Thread : public HeapValue {
 public:
  static
  Thread* New(Store* store,
              Engine* engine,
              Closure* closure,
              Array* parameters,
              Store* thread_store);

  enum ThreadState {
    INVALID = -1,
    RUNNABLE,
    WAITING,
    TERMINATED,
    THREAD_STATE_MAX,
  };

  // Executes instructions for this thread.
  // @param steps_count How many instructions to execute, at most.
  // @param new_runnable Returns new runnable threads in this list.
  //     Do not include this thread in this list: its runnable state is
  //     determined by the returned ThreadState.
  // @returns The state of the thread.
  ThreadState Run(uint64 steps_count, list<Thread*>* new_runnable);

  inline Value RGet(const Register& reg);
  inline void RSet(const Register& reg, Value value);
  inline void RSet(const Operand& op, Value value);
  inline Value OpGet(const Operand& operand);

  bool WaitOn(Value value);

  // ---------------------------------------------------------------------------
  // Exception handler
  class ExnStackEntry {
   public:
    enum ExnHandlerType {
      // Finally handlers are always executed.
      FINALLY = 0,

      // Catch handlers are executed only when the exception register is set.
      CATCH = 1,
    };

    ExnStackEntry(ExnHandlerType type, int32 code_pointer)
        : type_(type),
          code_pointer_(code_pointer) {
    }

    ExnHandlerType type_;
    uint64 code_pointer_;
  };

  // ---------------------------------------------------------------------------
  // Call stack
  class CallStackEntry {
   public:
    CallStackEntry(Store* store, Closure* closure, Array* parameters)
        : proc_(closure),
          parameters_(parameters),
          locals_(Array::New(store, closure->nlocals(), KAtomEmpty())),
          array_(NULL),
          code_pointer_(0) {
      CHECK_NOTNULL(closure);
    }

    // Bytecode segment and environment closure
    Closure* proc_;

    // Call parameters
    Array* parameters_;

    // Local registers
    Array* locals_;

    // Array manipulation registers
    Array* array_;

    // Index of the current bytecode instruction
    uint64 code_pointer_;

    // The exception handler stack
    vector<ExnStackEntry> exn_handlers_;
  };

  // ---------------------------------------------------------------------------

  uint64 id() const { return id_; }

 private:   // -----------------------------------------------------------------

  Thread(Engine* engine, Closure* closure, Array* parameters, Store* store);
  virtual ~Thread();

  // The next thread ID to allocate
  static uint64 next_id_;

  // Returns a new unique thread ID.
  static uint64 GetNextThreadID();

  // ---------------------------------------------------------------------------
  // Memory layout

  // The thread unique ID.
  uint64 id_;

  // The engine this thread belongs to.
  Engine* const engine_;

  // The store this thread creates values into.
  Store* const store_;

  // The call stack.
  vector<CallStackEntry> call_stack_;

  // Per-thread exception register.
  Value exception_;
};

// -----------------------------------------------------------------------------

}  // namespace store

#endif  // STORE_THREAD_H_
