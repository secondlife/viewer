/** 
 * @file llviewerprecompiledheaders.h
 * @brief precompiled headers for newview project
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
 */


#ifndef LL_LLVIEWERPRECOMPILEDHEADERS_H
#define LL_LLVIEWERPRECOMPILEDHEADERS_H

// This file MUST be the first one included by each .cpp file
// in viewer.
// It is used to precompile headers for improved build speed.

#include <boost/coroutine/coroutine.hpp>

#include "linden_common.h"

// Work around stupid Microsoft STL warning
#ifdef LL_WINDOWS
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif

#include <algorithm>
#include <deque>
#include <functional>
#include <map>
#include <set>

#ifdef LL_WINDOWS
#pragma warning (3 : 4702) // we like level 3, not 4
#endif

// Library headers from llcommon project:
#include "bitpack.h"
#include "lldeleteutils.h"
#include "imageids.h"
#include "indra_constants.h"
#include "llinitparam.h"

//#include "linden_common.h"
//#include "llpreprocessor.h"
#include "llallocator.h"
#include "llapp.h"
#include "llcriticaldamp.h"
#include "lldefs.h"
#include "lldepthstack.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llframetimer.h"
#include "llhash.h"
#include "lllocalidhashmap.h"
#include "llnametable.h"
#include "llpointer.h"
#include "llpriqueuemap.h"
#include "llprocessor.h"
#include "llrefcount.h"
#include "llsafehandle.h"
//#include "llsecondlifeurls.h"
#include "llsd.h"
#include "llsingleton.h"
#include "llstat.h"
#include "llstl.h"
#include "llstrider.h"
#include "llstring.h"
#include "llsys.h"
#include "llthread.h"
#include "lltimer.h"
#include "lluuidhashmap.h"
//#include "processor.h"
#include "stdenums.h"
#include "stdtypes.h"
//#include "string_table.h"
//#include "timer.h"
#include "timing.h"
#include "u64.h"

// Library includes from llmath project
#include "llmath.h"
#include "llbboxlocal.h"
#include "llcamera.h"
#include "llcoord.h"
#include "llcoordframe.h"
#include "llcrc.h"
#include "llplane.h"
#include "llquantize.h"
#include "llrand.h"
#include "llrect.h"
#include "lluuid.h"
#include "m3math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "v2math.h"
#include "v3color.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v4math.h"
#include "xform.h"

// Library includes from llvfs
#include "lldir.h"

// Library includes from llmessage project
#include "llcachename.h"


#endif
