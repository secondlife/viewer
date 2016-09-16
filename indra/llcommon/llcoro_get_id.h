/**
 * @file   llcoro_get_id.h
 * @author Nat Goodspeed
 * @date   2016-09-03
 * @brief  Supplement the functionality in llcoro.h.
 *
 *         This is broken out as a separate header file to resolve
 *         circularity: LLCoros isa LLSingleton, yet LLSingleton machinery
 *         requires llcoro::get_id().
 *
 *         Be very suspicious of anyone else #including this header.
 * 
 * $LicenseInfo:firstyear=2016&license=viewerlgpl$
 * Copyright (c) 2016, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLCORO_GET_ID_H)
#define LL_LLCORO_GET_ID_H

namespace llcoro
{

/// Get an opaque, distinct token for the running coroutine (or main).
typedef void* id;
id get_id();

} // llcoro

#endif /* ! defined(LL_LLCORO_GET_ID_H) */
