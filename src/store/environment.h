#ifndef STORE_ENVIRONMENT_H_
#define STORE_ENVIRONMENT_H_

#include <memory>

#include "base/macros.h"
#include "base/stl-util.h"
#include "store/values.h"

using std::unique_ptr;

namespace store {

class Environment;
class NestedRegisterAllocator;
class RegisterAllocator;
class RegisterAllocatorInterface;
class ScopedSymbol;
class ScopedTemp;
class Symbol;

// -----------------------------------------------------------------------------

// A symbol is a way to reference a value, be it explicit, the value of a
// named or an unnamed variable (eg. implicit variables for the results of
// a function call), etc.
class Symbol {
 public:
  enum Type {
    INVALID,

    // Symbol value is stored in a parameter register.
    PARAMETER,

    // Symbol value is stored in a local register.
    LOCAL,

    // Symbol value is stored in a closure register.
    CLOSURE,

    // Symbol value is an immediate (global) value.
    GLOBAL,
  };

  // The default constructor creates an invalid symbol.
  Symbol()
      : type_(INVALID),
        name_(),
        index_(-1),
        immediate_() {
  }

  // Copy constructor.
  Symbol(const Symbol& symbol)
      : type_(symbol.type_),
        name_(symbol.name_),
        index_(symbol.index_),
        immediate_(symbol.immediate_) {
  }

  // Initializes a global immediate symbol.
  Symbol(const string& name, Value immediate)
      : type_(GLOBAL),
        name_(name),
        index_(-1),
        immediate_(immediate) {
    CHECK(immediate_.IsDefined());
  }

  // Initializes a symbol reachable through a register.
  Symbol(Type type, const string& name, int index)
      : type_(type),
        name_(name),
        index_(index),
        immediate_() {
    CHECK_NE(GLOBAL, type_);
    CHECK_GE(index_, 0);
  }

  Type type() const { return type_; }
  bool valid() const { return type_ != INVALID; }

  string name() const {
    CHECK_NE(INVALID, type_);
    return name_;
  }

  int index() const {
    CHECK_NE(INVALID, type_);
    CHECK_NE(GLOBAL, type_);
    return index_;
  }

  Value immediate() const {
    CHECK_EQ(GLOBAL, type_);
    return immediate_;
  }

  Operand GetOperand() const {
    switch (type_) {
      case PARAMETER: return Operand(Register(Register::PARAM, index_));
      case LOCAL: return Operand(Register(Register::LOCAL, index_));
      case CLOSURE: return Operand(Register(Register::ENVMT, index_));
      case GLOBAL: return Operand(immediate_);
      case INVALID: LOG(FATAL) << "Invalid symbol";
    }
    LOG(FATAL) << "Dead code";
  }

  bool operator==(const Symbol& o) const {
    return (type_ == o.type_)
        && (name_ == o.name_)
        && (index_ == o.index_)
        && (immediate_ == o.immediate_);
  }

  // Assignment operator
  Symbol& operator=(const Symbol& o) {
    type_ = o.type_;
    name_ = o.name_;
    index_ = o.index_;
    immediate_ = o.immediate_;
    return *this;
  }

  static
  string DebugString(Symbol::Type type) {
    switch (type) {
      case INVALID: return "INVALID";
      case PARAMETER: return "PARAMETER";
      case LOCAL: return "LOCAL";
      case CLOSURE: return "CLOSURE";
      case GLOBAL: return "GLOBAL";
      default: LOG(FATAL) << "Invalid symbol type: " << type;
    }
  }

  string DebugString() const {
    return (boost::format("Symbol(type:%s name:'%s' index:%s)")
            % DebugString(type_) % name_ % index_).str();
  }

 private:
  // Type of the symbol.
  Type type_;

  // Optional name for the symbol.
  string name_;

  // For symbol stored in registers, this is the register index.
  int index_;

  // For global, immediate symbols, this is the actual value.
  Value immediate_;
};

inline
string DebugString(const Symbol& symbol) {
  return symbol.DebugString();
}

// -----------------------------------------------------------------------------

// Interface for a register allocator.
class RegisterAllocatorInterface {
 public:
  virtual ~RegisterAllocatorInterface() {}

  virtual Symbol Allocate(const string& name) = 0;
  virtual void Free(const Symbol& symbol) = 0;
  virtual int nregisters() const = 0;
  virtual RegisterAllocatorInterface* parent() const = 0;

  virtual const Symbol& Get(int index) const = 0;
  virtual const Symbol& Get(const string& name) const = 0;

  virtual const UnorderedMap<int, Symbol>& symbols() const = 0;
  virtual const UnorderedMap<string, Symbol>& named_symbols() const = 0;
};

// -----------------------------------------------------------------------------

// An allocator for indexed registers.
class RegisterAllocator : public RegisterAllocatorInterface {
 public:
  // Initializes a new allocator.
  RegisterAllocator(Symbol::Type type)
      : type_(type),
        nregisters_(0) {
  }

  virtual ~RegisterAllocator() {
  }

  virtual Symbol Allocate(const string& name = "") {
    int reg_index;
    if (returned_.empty()) {
      reg_index = nregisters_;
      nregisters_ += 1;
    } else {
      auto it = returned_.begin();
      reg_index = *it;
      returned_.erase(it);
    }

    const Symbol symbol(type_, name, reg_index);
    symbols_.insert_new(reg_index, symbol);
    if (!name.empty())
      named_symbols_.insert_new(name, symbol);
    return symbol;
  }

  virtual void Free(const Symbol& symbol) {
    CHECK_GE(symbol.index(), 0);
    CHECK_LT(symbol.index(), nregisters_);

    CHECK(symbol == symbols_[symbol.index()]);
     CHECK_EQ(1UL, symbols_.erase(symbol.index()));

    const string& name = symbol.name();
    if (!name.empty()) {
      CHECK(symbol == named_symbols_[name]);
      CHECK_EQ(1UL, named_symbols_.erase(name));
    }

    returned_.insert_new(symbol.index());
  }

  virtual int nregisters() const {
    return nregisters_;
  }

  virtual RegisterAllocatorInterface* parent() const {
    return NULL;
  }

  virtual const Symbol& Get(int index) const {
    return symbols_[index];
  }

  virtual const Symbol& Get(const string& name) const {
    return named_symbols_[name];
  }

  virtual const UnorderedMap<int, Symbol>& symbols() const {
    return symbols_;
  }

  virtual const UnorderedMap<string, Symbol>& named_symbols() const {
    return named_symbols_;
  }

 private:
  // Type of symbols to create.
  const Symbol::Type type_;

  // Maximum number of registers. Never decreases.
  int nregisters_;

  // Map of all symbols, indexed by their register index.
  UnorderedMap<int, Symbol> symbols_;

  // Map of named symbols, indexed by their names.
  UnorderedMap<string, Symbol> named_symbols_;

  // Returned registers may be allocated again.
  Set<int> returned_;

  DISALLOW_COPY_AND_ASSIGN(RegisterAllocator);
};

// -----------------------------------------------------------------------------

// An allocator nested on top of an existing allocator.
class NestedRegisterAllocator : public RegisterAllocatorInterface {
 public:
  NestedRegisterAllocator(RegisterAllocatorInterface* parent)
      : parent_(CHECK_NOTNULL(parent)) {
  }

  virtual ~NestedRegisterAllocator() {
    for (auto it = symbols_.begin(); it != symbols_.end(); ++it)
      parent_->Free(it->second);
  }

  virtual Symbol Allocate(const string& name = "") {
    const Symbol symbol = parent_->Allocate(name);
    symbols_.insert_new(symbol.index(), symbol);
    return symbol;
  }

  virtual void Free(const Symbol& symbol) {
    CHECK_EQ(1UL, symbols_.erase(symbol.index()));
    parent_->Free(symbol);
  }

  virtual int nregisters() const {
    return parent_->nregisters();
  }

  virtual RegisterAllocatorInterface* parent() const {
    return parent_;
  }

  virtual const Symbol& Get(int index) const {
    return parent_->Get(index);
  }

  virtual const Symbol& Get(const string& string) const {
    return parent_->Get(string);
  }

  virtual const UnorderedMap<int, Symbol>& symbols() const {
    return parent_->symbols();
  }

  virtual const UnorderedMap<string, Symbol>& named_symbols() const {
    return parent_->named_symbols();
  }

 private:
  // The allocator on top of which this allocator is built.
  RegisterAllocatorInterface* const parent_;

  // Symbols generated.
  UnorderedMap<int, Symbol> symbols_;

  DISALLOW_COPY_AND_ASSIGN(NestedRegisterAllocator);
};

// -----------------------------------------------------------------------------

// A scoped symbol keeps a symbol in the environment until it is destroyed.
class ScopedSymbol {
 public:
  ScopedSymbol(Environment* environment, const Symbol& symbol);
  virtual ~ScopedSymbol();

  void Release();

  const Environment& environment() const { return *environment_; }
  const Symbol& symbol() const { return symbol_; }
  Operand operand() const { return symbol_.GetOperand(); }

 private:
  // The environment the symbol belongs to.
  Environment* environment_;

  // The symbol hold by this ScopedSymbol.
  Symbol symbol_;

  DISALLOW_COPY_AND_ASSIGN(ScopedSymbol);
};

// -----------------------------------------------------------------------------

// Environments are organized as a stack.
// The root environment may contain global symbols (immediate values).
// Each function has its own environment, inheriting all the symbols from the
// environment of its outer scope.
// Symbols from the outer scope may be reached through closure registers.
// TODO: Global symbols (immediate values) can be accessed directly.
// Importing an external value into the closure means the value must be
// imported all the way down from the environment where it was defined.
// Local register allocators (local symbols and temporaries) may be nested
// within one environment.
class Environment {
 public:
  Environment(Environment* parent);
  virtual ~Environment();

  bool IsRoot() const {
    return parent_ == NULL;
  }

  // @returns The symbol with the given name.
  //     Brings the symbol in the local closure if necessary.
  const Symbol& Get(const string& name);

  // @returns Whether a symbol exists in the environment chain.
  // @param name The symbol name.
  bool ExistsGlobally(const string& name);

  // @returns Whether a symbol exists locally in this environment.
  // @param name The symbol name.
  bool ExistsLocally(const string& name);

  // @returns A local symbol by its name.
  const Symbol& GetLocally(const string& name) const;

  // Adds a global symbol into a root environment.
  void AddGlobal(const string& name, Value value);

  // Adds a symbol for a named local variable.
  // @param name The name of the local symbol to add.
  // @returns Symbol for the local register.
  Symbol AddLocal(const string& name);

  // Removes a symbol for a named local variable.
  void RemoveLocal(const string& name);

  ScopedSymbol* NewScopedLocal(const string& name);

  // @returns A new temporary local register index.
  // @param description Optional description for the local register.
  Symbol AddTemporary(const string& description = "");
  void RemoveTemporary(const Symbol& symbol);

  // Adds a procedure parameter symbol.
  // Parameter registers cannot be freed.
  void AddParameter(const string& name);

  // Imports a symbol from outer/parent environments locally into this
  // environment.
  // Concretely, this allocate a closure register.
  // Closure registers cannot be freed.
  void ImportIntoClosure(const string& name);

  // Nested local register allocators.
  void BeginNestedLocalAllocator();
  shared_ptr<RegisterAllocatorInterface> EndNestedLocalAllocator();

  class NestedLocalAllocator {
   public:
    NestedLocalAllocator(Environment* env)
        : env_(CHECK_NOTNULL(env)),
          locked_(false) {
      env_->BeginNestedLocalAllocator();
    }

    virtual ~NestedLocalAllocator() {
      if (!locked_) Lock();
    }

    void Lock() {
      CHECK(!locked_);
      locked_ = true;
      allocator_ = env_->EndNestedLocalAllocator();
    }

   private:
    Environment* const env_;

    // Local allocator, initialized when the local allocator is locked.
    shared_ptr<RegisterAllocatorInterface> allocator_;

    // Becomes true when the local allocator is locked.
    bool locked_;
  };

  NestedLocalAllocator* NewNestedLocalAllocator() {
    return new NestedLocalAllocator(this);
  }



  const UnorderedMap<string, Symbol>& global_symbols() const {
    return global_symbols_;
  }
  Environment* parent() const { return parent_; }
  uint64 nlocals() const { return local_allocator_->nregisters(); }
  uint64 nparams() const { return param_allocator_.nregisters(); }
  uint64 nclosures() const { return closure_allocator_.nregisters(); }
  const vector<string>& closure_symbol_names() const {
    return closure_symbol_names_;
  }

 private:
  // The parent environment. NULL for a root environment.
  Environment* const parent_;

  // Global symbols are defined here.
  UnorderedMap<string, Symbol> global_symbols_;

  // Registers for the temporaries: local register index -> temp description.
  UnorderedMap<int, string> temps_;

  // local registers are reusable.
  unique_ptr<RegisterAllocatorInterface> local_allocator_;

  // param registers are not reusable.
  RegisterAllocator param_allocator_;

  // closure registers are not reusable.
  RegisterAllocator closure_allocator_;

  // Names of symbols in the closure.
  vector<string> closure_symbol_names_;

  DISALLOW_COPY_AND_ASSIGN(Environment);
};

// -----------------------------------------------------------------------------

// Automatically frees a temporary register when going out of scope.
class ScopedTemp {
 public:
  ScopedTemp(Environment* env)
      : env_(CHECK_NOTNULL(env)) {
  }

  ScopedTemp(Environment* env, const string& description)
      : env_(env) {
    Allocate(description);
  }

  ~ScopedTemp() {
    Free();
  }

  Operand Allocate(const string& description = "") {
    CHECK(!valid());
    symbol_ = env_->AddTemporary(description);
    return GetOperand();
  }

  void Free() {
    if (!valid()) return;
    env_->RemoveTemporary(symbol_);
    symbol_ = Symbol();
  }

  Operand GetOperand() const {
    return symbol_.GetOperand();
  }

  const Symbol& symbol() const { return symbol_; }
  bool valid() const { return symbol_.valid(); }

 private:
  // The environment the temporary belongs to, or NULL.
  Environment* const env_;

  // Symbol for the temporary. May be INVALID.
  Symbol symbol_;

  DISALLOW_COPY_AND_ASSIGN(ScopedTemp);
};

}  // namespace store

#endif  // STORE_ENVIRONMENT_H_
