#ifndef STORE_VALUES_H_
#define STORE_VALUES_H_

#include "store/value.h"
#include "store/heap_value.h"
#include "store/moved_value.h"
#include "store/store.h"

#include "store/literal.h"

#include "store/variable.h"

// Primitive values
#include "store/atom.h"
#include "store/boolean.h"
#include "store/float.h"
#include "store/integer.h"
#include "store/name.h"
#include "store/small_integer.h"
#include "store/string.h"

// Compound values
#include "store/arity.h"
#include "store/arity_map.h"

#include "store/array.h"
#include "store/cell.h"
#include "store/closure.h"
#include "store/list.h"
#include "store/open_record.h"
#include "store/record.h"
#include "store/tuple.h"

#include "store/thread.h"
#include "store/bytecode.h"

// Inlined declarations

#include "store/value.inl.h"
#include "store/arity.inl.h"
#include "store/small_integer.inl.h"
#include "store/integer.inl.h"
#include "store/list.inl.h"
#include "store/open_record.inl.h"
#include "store/tuple.inl.h"
#include "store/record.inl.h"

#include "store/thread.inl.h"

#endif  // STORE_VALUES_H_
