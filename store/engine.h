#ifndef STORE_ENGINE_H_
#define STORE_ENGINE_H_

#include <list>
#include <map>
#include <string>

using std::list;
using std::map;
using std::string;

#include "base/basictypes.h"

namespace store {

class Array;
class Thread;

class NativeInterface {
 public:
  virtual ~NativeInterface() {}

  virtual void Execute(Array* parameters) = 0;
};

// The engine runs a collection of threads.
class Engine {
 public:
  Engine();

  // Runs as long as there are live threads.
  void Run();

  // Registers a native procedure.
  // Override any pre-existing native with the specified name.
  void RegisterNative(string name, NativeInterface* native);

 private:
  void AddThread(Thread* thread);

  map<uint64, Thread*> thread_map_;
  list<Thread*> runnable_;

  map<string, NativeInterface*> native_map_;

  friend class Thread;
};

}  // namespace store

#endif  // STORE_ENGINE_H_
