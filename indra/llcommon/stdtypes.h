/** 
 * @file stdtypes.h
 * @brief Basic type declarations for cross platform compatibility.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LL_STDTYPES_H
#define LL_STDTYPES_H

#include <limits.h>
#include <float.h>

typedef signed char				S8;
typedef unsigned char			U8;
typedef signed short			S16;
typedef unsigned short			U16;
typedef signed int				S32;
typedef unsigned int			U32;

#if LL_WINDOWS
// Windows wchar_t is 16-bit
typedef U32						llwchar;
#else
typedef wchar_t					llwchar;
#endif

#if LL_WINDOWS
typedef signed __int64			S64;
// probably should be 'hyper' or similiar
#define S64L(a)					(a)
typedef unsigned __int64		U64;
#define U64L(a)					(a)
#else
typedef long long int			S64;
typedef long long unsigned int	U64;
#if LL_DARWIN || LL_LINUX
#define S64L(a)					(a##LL)
#define U64L(a)					(a##ULL)
#endif
#endif

typedef float			F32;
typedef double			F64;

typedef S32				BOOL;
typedef U8				KEY;
typedef U32				MASK;
typedef U32             TPACKETID;

// Use #define instead of consts to avoid conversion headaches
#define S8_MAX		(SCHAR_MAX)
#define U8_MAX		(UCHAR_MAX)
#define S16_MAX		(SHRT_MAX)
#define U16_MAX		(USHRT_MAX)
#define S32_MAX		(INT_MAX)
#define U32_MAX		(UINT_MAX)
#define F32_MAX		(FLT_MAX)
#define F64_MAX		(DBL_MAX)

#define S8_MIN		(SCHAR_MIN)
#define U8_MIN		(0)
#define S16_MIN		(SHRT_MIN)
#define U16_MIN		(0)
#define S32_MIN		(INT_MIN)
#define U32_MIN		(0)
#define F32_MIN		(FLT_MIN)
#define F64_MIN		(DBL_MIN)


#ifndef TRUE
#define TRUE			(1)
#endif

#ifndef FALSE
#define FALSE			(0)
#endif

#ifndef NULL
#define NULL			(0)
#endif

typedef U8 LLPCode;

#if LL_LINUX && __GNUC__ <= 2
typedef int intptr_t;
#endif

#endif
