/** 
 * @file stdtypes.h
 * @brief Basic type declarations for cross platform compatibility.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */
#ifndef LL_STDTYPES_H
#define LL_STDTYPES_H

#include <cfloat>
#include <climits>

typedef signed char			S8;
typedef unsigned char			U8;
typedef signed short			S16;
typedef unsigned short			U16;
typedef signed int			S32;
typedef unsigned int			U32;

#if LL_WINDOWS
// Windows wchar_t is 16-bit
typedef U32				llwchar;
#else
typedef wchar_t				llwchar;
#endif

#if LL_WINDOWS
typedef signed __int64			S64;
// probably should be 'hyper' or similiar
#define S64L(a)					(a)
typedef unsigned __int64		U64;
#define U64L(a)					(a)
#else
typedef long long int			S64;
typedef long long unsigned int		U64;
#if LL_DARWIN || LL_LINUX || LL_SOLARIS
#define S64L(a)				(a##LL)
#define U64L(a)				(a##ULL)
#endif
#endif

typedef float				F32;
typedef double				F64;

typedef S32				BOOL;
typedef U8				KEY;
typedef U32				MASK;
typedef U32             		TPACKETID;

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

#define	LL_ARRAY_SIZE( _kArray ) ( sizeof( (_kArray) ) / sizeof( _kArray[0] ) )

#if LL_LINUX && __GNUC__ <= 2
typedef int intptr_t;
#endif

#endif
