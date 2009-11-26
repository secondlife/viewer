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

// We may want to take the windows.h include out, but it used to be in 
// linden_common.h, and hence in all the libraries.  This is better. JC
#if LL_WINDOWS
	// Limit Windows API to small and manageable set.
	// If you get undefined symbols, find the appropriate
	// Windows header file and include that in your .cpp file.
	#define WIN32_LEAN_AND_MEAN
	#include <winsock2.h>
	#include <windows.h>
#endif

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
#include "llapr.h"
#include "llcriticaldamp.h"
//#include "lldarray.h"
//#include "lldarrayptr.h"
#include "lldefs.h"
#include "lldepthstack.h"
//#include "lldqueueptr.h"
#include "llendianswizzle.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llframetimer.h"
#include "llhash.h"
#include "lllocalidhashmap.h"
#include "llmap.h"
//#include "llmemory.h"
#include "llnametable.h"
#include "llpointer.h"
#include "llpriqueuemap.h"
#include "llprocessor.h"
#include "llrefcount.h"
#include "llsafehandle.h"
//#include "llsecondlifeurls.h"
#include "llsd.h"
#include "llsingleton.h"
#include "llstack.h"
#include "llstat.h"
#include "llstl.h"
#include "llstrider.h"
#include "llstring.h"
#include "llsys.h"
#include "llthread.h"
#include "lltimer.h"
#include "lluuidhashmap.h"
//#include "llversionviewer.h"
//#include "processor.h"
#include "stdenums.h"
#include "stdtypes.h"
//#include "string_table.h"
//#include "timer.h"
#include "timing.h"
#include "u64.h"

// Library includes from llimage
//#include "llblockdata.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagepng.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagetga.h"
#include "llmapimagetype.h"

// Library includes from llmath project
//#include "camera.h"
//#include "coordframe.h"
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

// Library includes from llmessage project
//#include "llassetstorage.h"
#include "llcachename.h"
#include "llcircuit.h"
#include "lldatapacker.h"
#include "lldbstrings.h"
#include "lldispatcher.h"
#include "lleventflags.h"
#include "llhost.h"
#include "llinstantmessage.h"
#include "llinvite.h"
//#include "llloginflags.h"
#include "llmail.h"
#include "llmessagethrottle.h"
#include "llnamevalue.h"
#include "llpacketack.h"
#include "llpacketbuffer.h"
#include "llpacketring.h"
#include "llpartdata.h"
//#include "llqueryflags.h"
//#include "llregionflags.h"
#include "llregionhandle.h"
#include "lltaskname.h"
#include "llteleportflags.h"
#include "llthrottle.h"
#include "lltransfermanager.h"
#include "lltransfersourceasset.h"
#include "lltransfersourcefile.h"
#include "lltransfertargetfile.h"
#include "lltransfertargetvfile.h"
#include "lluseroperation.h"
#include "llvehicleparams.h"
#include "llxfer.h"
#include "llxfer_file.h"
#include "llxfer_mem.h"
#include "llxfer_vfile.h"
#include "llxfermanager.h"
#include "machine.h"
#include "mean_collision_data.h"
#include "message.h"
#include "message_prehash.h"
#include "net.h"
//#include "network.h"
#include "partsyspacket.h"
#include "patch_code.h"
#include "patch_dct.h"
#include "sound_ids.h"

// Builds work with all headers below commented out as of 2009-09-10 JC

// Library includes from llprimitive
#include "imageids.h"
#include "legacy_object_types.h"
#include "llmaterialtable.h"
//#include "llprimitive.h"
#include "lltextureanim.h"
//#include "lltextureentry.h"
#include "lltreeparams.h"
//#include "llvolume.h"
#include "llvolumemgr.h"
#include "material_codes.h"

// Library includes from llvfs
#include "llassettype.h"
#include "lldir.h"
//#include "lldir_linux.h"
//#include "lldir_mac.h"
//#include "lldir_win32.h"
#include "llvfile.h"
#include "llvfs.h"

// Library includes from llui
// In skinning-7, llui.h dependencies are changing too often.
//#include "llui.h"

// llxuixml
#include "llinitparam.h"

#endif
