// Basic stuff

#ifndef BASE_MACROS_H__
#define BASE_MACROS_H__

#include <cstdint>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#define ArraySize(array) \
  (sizeof(array) / sizeof(array[0]))

#endif  // BASE_MACROS_H__
