/** 
 * @file llformat.h
 * @date   January 2007
 * @brief string formatting utility
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFORMAT_H
#define LL_LLFORMAT_H

// Use as follows:
// llinfos << llformat("Test:%d (%.2f %.2f)", idx, x, y) << llendl;
//
// *NOTE: buffer limited to 1024, (but vsnprintf prevents overrun)
// should perhaps be replaced with boost::format.

std::string llformat(const char *fmt, ...);

#endif // LL_LLFORMAT_H
