/**
 * @file   fix_macros.h
 * @author Nat Goodspeed
 * @date   2012-11-16
 * @brief  The Mac system headers seem to #define macros with obnoxiously
 *         generic names, preventing any library from using those names. We've
 *         had to fix these in so many places that it's worth making a header
 *         file to handle it.
 * 
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Copyright (c) 2012, Linden Research, Inc.
 * $/LicenseInfo$
 */

// DON'T use an #include guard: every time we encounter this header, #undef
// these macros all over again.

// who injects MACROS with such generic names?! Grr.
#ifdef equivalent
#undef equivalent
#endif 

#ifdef check
#undef check
#endif
