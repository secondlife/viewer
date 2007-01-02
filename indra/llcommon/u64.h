/** 
 * @file u64.h
 * @brief Utilities to deal with U64s.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_U64_H
#define LL_U64_H

U64 str_to_U64(const char* str);
char* U64_to_str(U64 value, char* result, S32 result_size);

F64 U64_to_F64(const U64 value);

U64 llstrtou64(const char* str, char** end, S32 base);

#endif
