/** 
 * @file llviewerprecompiledheaders.h
 * @brief precompiled headers for newview project
 * @author James Cook
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
#include "doublelinkedlist.h"
#include "imageids.h"
#include "indra_constants.h"
//#include "linden_common.h"
//#include "llpreprocessor.h"
#include "linked_lists.h"
#include "llapp.h"
#include "llapr.h"
#include "llassoclist.h"
#include "llcriticaldamp.h"
#include "lldarray.h"
#include "lldarrayptr.h"
#include "lldefs.h"
#include "lldepthstack.h"
#include "lldlinked.h"
#include "lldqueueptr.h"
#include "llendianswizzle.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llfixedbuffer.h"
#include "llframetimer.h"
#include "llhash.h"
#include "lllinkedqueue.h"
#include "lllocalidhashmap.h"
#include "llmap.h"
#include "llmemory.h"
#include "llnametable.h"
#include "llpagemem.h"
#include "llpriqueuemap.h"
#include "llprocessor.h"
#include "llptrskiplist.h"
#include "llptrskipmap.h"
//#include "llsecondlifeurls.h"
#include "llskiplist.h"
#include "llskipmap.h"
#include "llstack.h"
#include "llstat.h"
#include "llstl.h"
#include "llstrider.h"
#include "llstring.h"
#include "llstringtable.h"
#include "llsys.h"
#include "llthread.h"
#include "lltimer.h"
#include "lluuidhashmap.h"
//#include "llversion.h"
//#include "processor.h"
#include "stdenums.h"
#include "stdtypes.h"
//#include "string_table.h"
//#include "timer.h"
#include "timing.h"
#include "u64.h"

// Library includes from llimage
//#include "kdc_flow_control.h"
//#include "kde_flow_control.h"
//#include "kdu_image.h"
//#include "kdu_image_local.h"
//#include "llblockdata.h"
//#include "llblockdecoder.h"
//#include "llblockencoder.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagejpeg.h"
#include "llimagetga.h"
//#include "llkdumem.h"
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
#include "llmd5.h"
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
#include "llcallbacklisth.h"
#include "llcircuit.h"
#include "lldatapacker.h"
#include "lldbstrings.h"
#include "lldispatcher.h"
#include "lleventflags.h"
#include "llhost.h"
#include "llinstantmessage.h"
#include "llinvite.h"
//#include "llloginflags.h"
#include "lllogtextmessage.h"
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

// Library includes from llxml
#include "llxmlnode.h"

// Library includes from llvfs
#include "llassettype.h"
#include "lldir.h"
//#include "lldir_linux.h"
//#include "lldir_mac.h"
//#include "lldir_win32.h"
#include "llvfile.h"
#include "llvfs.h"

#endif
