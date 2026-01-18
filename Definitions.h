#pragma once

#ifndef USE_FIXED_TYPES
#define INT_8 char
#define UINT_8 unsigned char
#define INT_16 short
#define UINT_16 unsigned short
#define INT_32 int
#define UINT_32 unsigned int
#define INT_64 long long
#define UINT_64 unsigned long long
#else
#define INT_8 int8_t
#define UINT_8 uint8_t
#define INT_16 int16_t
#define UINT_16 uint16_t
#define INT_32 int32_t
#define UINT_32 uint32_t
#define INT_64 int64_t
#define UINT_64 uint64_t
#endif

#ifndef DEV
#ifdef EXPORT
#define DF_API __declspec(dllexport)
#else
#define DF_API __declspec(dllimport)
#endif
#else
#define DF_API
#endif

constexpr size_t SPEC_FIXED = 0;
constexpr size_t SPEC_MIXED = 1;