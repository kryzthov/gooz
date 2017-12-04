#include "store/engine.h"

#include <list>
using std::list;

#include "store/values.h"

namespace store {

namespace native {

class Print: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    for (uint64 i = 0; i < parameters->size(); ++i) {
      Value value = parameters->values()[i];
      printf("%s", value.ToString().c_str());
    }
  }
};

class PrintLine: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    for (uint64 i = 0; i < parameters->size(); ++i) {
      Value value = parameters->values()[i];
      printf("%s\n", value.ToString().c_str());
    }
  }
};

class Decrement: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    parameters->Assign(0, Value::Integer(IntValue(parameters->Access(0)) - 1));
  }
};

class IsZero: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    parameters->Assign(0, Boolean::Get(IntValue(parameters->Access(0)) == 0));
  }
};

class Int64Multiply: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    const int64 mul =
        IntValue(parameters->Access(0)) * IntValue(parameters->Access(1));
    parameters->Assign(0, Value::Integer(mul));
  }
};

class GetLabel: public NativeInterface {
 public:
  virtual void Execute(Array* parameters) {
    // TODO: How to forward woken-up threads?
    Unify(parameters->Access(1), parameters->Access(0).RecordLabel());
  }
};

}  // namespace native

Engine::Engine() {
  RegisterNative("println", new native::PrintLine);
  RegisterNative("print", new native::Print);
  RegisterNative("decrement", new native::Decrement);
  RegisterNative("is_zero", new native::IsZero);
  RegisterNative("multiply", new native::Int64Multiply);
  RegisterNative("get_label", new native::GetLabel);
}

void Engine::Run() {
  const int kStepsCount = 1000;  // Execute at most 1k instructions at a time.

  while (!runnable_.empty()) {
    Thread* thread = runnable_.front();
    runnable_.pop_front();
    // The thread scheduling is determined by how woken up suspensions are added
    // to the runnable_ list.
    const Thread::ThreadState thread_state =
        thread->Run(kStepsCount, &runnable_);
    switch (thread_state) {
      case Thread::RUNNABLE:
        runnable_.push_back(thread);
        break;
      case Thread::WAITING:
        break;
      case Thread::TERMINATED:
        // Nothing to do.
        break;
      default:
        LOG(FATAL) << "Unexpected thread state: " << thread_state;
    }
  }
}

void Engine::AddThread(Thread* thread) {
  runnable_.push_back(thread);
  thread_map_[thread->id()] = thread;
}

void Engine::RegisterNative(string name, NativeInterface* native) {
  native_map_[name] = native;
}

}  // namespace store
