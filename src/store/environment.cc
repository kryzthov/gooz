#include "store/environment.h"

#include "base/stl-util.h"

namespace store {

ScopedSymbol::ScopedSymbol(Environment* environment, const Symbol& symbol)
    : environment_(CHECK_NOTNULL(environment)),
      symbol_(symbol) {
}

ScopedSymbol::~ScopedSymbol() {
  if (environment_ != NULL) {
    environment_->RemoveLocal(symbol_.name());
    symbol_ = Symbol();
  }
}

void ScopedSymbol::Release() {
  environment_ = NULL;
  symbol_ = Symbol();
}

// -----------------------------------------------------------------------------

Environment::Environment(Environment* parent)
    : parent_(parent),
      local_allocator_(new RegisterAllocator(Symbol::LOCAL)),
      param_allocator_(Symbol::PARAMETER),
      closure_allocator_(Symbol::CLOSURE) {
}

Environment::~Environment() {
}

const Symbol& Environment::Get(const string& name) {
  ImportIntoClosure(name);
  return GetLocally(name);
}

bool Environment::ExistsGlobally(const string& name) {
  return ExistsLocally(name)
      || ((parent_ != NULL) && parent_->ExistsGlobally(name));
}

bool Environment::ExistsLocally(const string& name) {
  CHECK(!name.empty());
  return local_allocator_->named_symbols().contains(name)
      || param_allocator_.named_symbols().contains(name)
      || closure_allocator_.named_symbols().contains(name)
      || global_symbols_.contains(name);
}

const Symbol& Environment::GetLocally(const string& name) const {
  const UnorderedMap<string, Symbol>* symbols[] = {
    &local_allocator_->named_symbols(),
    &param_allocator_.named_symbols(),
    &closure_allocator_.named_symbols(),
    &global_symbols_,
  };
  for (uint i = 0; i < ArraySize(symbols); ++i) {
    auto it = symbols[i]->find(name);
    if (it != symbols[i]->end()) return it->second;
  }
  LOG(FATAL) << "Cannot find symbol locally: " << name;
}

void Environment::AddGlobal(const string& name, Value value) {
  CHECK(IsRoot());
  CHECK(!ExistsLocally(name));
  global_symbols_.insert_new(name, Symbol(name, value));
}

Symbol Environment::AddLocal(const string& name) {
  CHECK(!name.empty());
  CHECK(!ExistsGlobally(name)) << "Redefining existing name: '" << name << "'";
  return local_allocator_->Allocate(name);
}

void Environment::RemoveLocal(const string& name) {
  CHECK(!name.empty());
  const Symbol& symbol = GetLocally(name);
  CHECK_EQ(Symbol::LOCAL, symbol.type());
  local_allocator_->Free(symbol);
}

ScopedSymbol* Environment::NewScopedLocal(const string& name) {
  return new ScopedSymbol(this, AddLocal(name));
}

Symbol Environment::AddTemporary(const string& description) {
  const Symbol& symbol = local_allocator_->Allocate("");
  temps_.insert_new(symbol.index(), description);
  return symbol;
}

void Environment::RemoveTemporary(const Symbol& symbol) {
  CHECK_EQ(1UL, temps_.erase(symbol.index()));
  local_allocator_->Free(symbol);
}

void Environment::AddParameter(const string& name) {
  CHECK(!ExistsLocally(name));
  param_allocator_.Allocate(name);
}

void Environment::ImportIntoClosure(const string& name) {
  if (ExistsLocally(name)) return;
  CHECK(parent_ != NULL) << "Cannot find symbol \"" << name << "\"";
  parent_->ImportIntoClosure(name);
  closure_allocator_.Allocate(name);
  closure_symbol_names_.push_back(name);
}

void Environment::BeginNestedLocalAllocator() {
  CHECK_NOTNULL(local_allocator_.get());
  local_allocator_.reset(
      new NestedRegisterAllocator(local_allocator_.release()));
}

void Environment::EndNestedLocalAllocator() {
  local_allocator_.reset(local_allocator_->parent());
  CHECK_NOTNULL(local_allocator_.get());
}

}  // namespace store
