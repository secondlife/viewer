/**
 * @file llmediaimplgstreamertriviallogging.h
 * @brief minimal logging utilities.
 *
 * @cond
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 * @endcond
 */

#ifndef __LLMEDIAIMPLGSTREAMERTRIVIALLOGGING_H__
#define __LLMEDIAIMPLGSTREAMERTRIVIALLOGGING_H__

#include <cstdio>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
}

/////////////////////////////////////////////////////////////////////////
// Debug/Info/Warning macros.
#define MSGMODULEFOO "(media plugin)"
#define STDERRMSG(...) do{\
    fprintf(stderr, " pid:%d: ", (int)getpid());\
    fprintf(stderr, MSGMODULEFOO " %s:%d: ", __FUNCTION__, __LINE__);\
    fprintf(stderr, __VA_ARGS__);\
    fputc('\n',stderr);\
  }while(0)
#define NULLMSG(...) do{}while(0)

#define DEBUGMSG NULLMSG
#define INFOMSG  STDERRMSG
#define WARNMSG  STDERRMSG
/////////////////////////////////////////////////////////////////////////

#endif /* __LLMEDIAIMPLGSTREAMERTRIVIALLOGGING_H__ */
