#!/usr/bin/env python3
# -*- mode: python -*-
# -*- encoding: utf8 -*-

"""Build configuration."""

Library(
  name='base_lib',
  sources=[
    'base/escaping.cc',
    'base/file-util.cc',
    'base/real.cc',
    'base/string_piece.cc',
    'base/stringer.cc',
  ],
)

ProtoLibrary(
  name='proto_lib',
  sources=[
    'proto/store.proto'
  ],
)

Library(
  name='store_lib',
  sources=[
    'store/arity.cc',
    'store/array.cc',
    'store/atom.cc',
    'store/boolean.cc',
    'store/bytecode.cc',
    'store/cell.cc',
    'store/closure.cc',
    'store/compiler.cc',
    'store/engine.cc',
    'store/environment.cc',
    'store/float.cc',
    'store/heap_value.cc',
    'store/integer.cc',
    'store/list.cc',
    'store/literal.cc',
    'store/moved_value.cc',
    'store/name.cc',
    'store/open_record.cc',
    'store/ozvalue.cc',
    'store/record.cc',
    'store/store.cc',
    'store/string.cc',
    'store/thread.cc',
    'store/tuple.cc',
    'store/value.cc',
    'store/variable.cc',
  ],
  dependencies=[
    'base_lib',
    'proto_lib',
  ],
)

Library(
  name='combinators_lib',
  sources=[
    'combinators/base.cc',
    'combinators/ozlexer.cc',
    'combinators/oznode.cc',
    'combinators/oznode_dump_visitor.cc',
    'combinators/oznode_eval_visitor.cc',
    'combinators/oznode_visitor.cc',
    'combinators/ozparser.cc',
    'combinators/stream.cc',
  ],
  dependencies=[
    'store_lib',
  ],
)

# Binary(
#   name='run_bytecode',
#   sources=[
#     'store/run_bytecode.cc',
#   ],
#   dependencies=[
#     'base_lib',
#     'combinators_lib',
#     'proto_lib',
#     'store_lib',
#   ],
# )

Binary(
  name='compile_run_bytecode',
  sources=[
    'store/compile_run_bytecode.cc',
  ],
  dependencies=[
    'base_lib',
    'combinators_lib',
    'proto_lib',
    'store_lib',
  ],
)

# ------------------------------------------------------------------------------
#Tests

Library(
  name='gtest_main_lib',
  sources=[
    'base/gtest_main.cc',
  ],
)

Test(
  name='base/base_test',
  sources=[
    'base/real_test.cc',
  ],
  dependencies=[
    'gtest_main_lib',
  ],
)

Test(
  name='combinators/base_test',
  sources=[
    'combinators/base_test.cc',
  ],
  dependencies=[
    'combinators_lib',
    'gtest_main_lib',
  ],
)

Test(
  name='combinators/ozlexer_test',
  sources=[
    'combinators/ozlexer_test.cc',
  ],
  dependencies=[
    'combinators_lib',
    'gtest_main_lib',
  ],
)

Test(
  name='combinators/ozparser_test',
  sources=[
    'combinators/ozparser_test.cc',
    'combinators/oznode_eval_visitor_test.cc',
  ],
  dependencies=[
    'combinators_lib',
    'gtest_main_lib',
  ],
)

Test(
  name='store/values_test',
  sources=[
    "store/arity_test.cc",
    "store/atom_test.cc",
    "store/equality_test.cc",
    "store/integer_test.cc",
    "store/list_test.cc",
    "store/open_record_test.cc",
    "store/ozvalue_test.cc",
    "store/small_integer_test.cc",
    "store/unification_test.cc",
    "store/values_test.cc"
  ],
  dependencies=[
    'combinators_lib',
    'gtest_main_lib',
    'store_lib',
  ],
)

# Test(
#   name='store/bytecode_test',
#   sources=[
#     'store/run_bytecode_test.cc',
#   ],
#   dependencies=[
#     'base_lib',
#     'combinators_lib',
#     'gtest_main_lib',
#     'proto_lib',
#     'store_lib',
#   ],
# )

Test(
  name='store/compile_test',
  sources=[
    'store/run_compile_test.cc',
  ],
  dependencies=[
    'base_lib',
    'combinators_lib',
    'gtest_main_lib',
    'proto_lib',
    'store_lib',
  ],
)
