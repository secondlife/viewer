/** 
 * @file llviewerprecompiledheaders.h
 * @brief precompiled headers for newview project
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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


#ifndef LL_LLVIEWERPRECOMPILEDHEADERS_H
#define LL_LLVIEWERPRECOMPILEDHEADERS_H

// This file MUST be the first one included by each .cpp file
// in viewer.
// It is used to precompile headers for improved build speed.

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
#include "llinterp.h"
#include "llperlin.h"
#include "llplane.h"
#include "llquantize.h"
#include "llrand.h"
#include "llrect.h"
#include "lluuid.h"
#include "m3math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "raytrace.h"
#include "v2math.h"
#include "v3color.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v4math.h"
////#include "vmath.h"
#include "xform.h"

// Library includes from llvfs
#include "lldir.h"

// Library includes from llmessage project
#include "llcachename.h"

// llxuixml
#include "llinitparam.h"

#endif
