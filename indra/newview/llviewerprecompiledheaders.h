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

#include "linden_common.h"

#include "llwin32headers.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// Library headers from llcommon project:
#include "apply.h"
#include "function_types.h"
#include "indra_constants.h"
#include "llinitparam.h"
#include "llapp.h"
#include "llcriticaldamp.h"
#include "lldefs.h"
#include "lldepthstack.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llframetimer.h"
#include "llinstancetracker.h"
#include "llpointer.h"
#include "llprocessor.h"
#include "llrand.h"
#include "llrefcount.h"
#include "llsafehandle.h"
#include "llsd.h"
#include "llsingleton.h"
#include "llstl.h"
#include "llstrider.h"
#include "llstring.h"
#include "llsys.h"
#include "lltimer.h"
#include "lluuid.h"
#include "stdtypes.h"
#include "u64.h"

// Library includes from llmath project
#include "llmath.h"
#include "llbbox.h"
#include "llbboxlocal.h"
#include "llcamera.h"
#include "llcoord.h"
#include "llcoordframe.h"
#include "llcrc.h"
#include "llplane.h"
#include "llquantize.h"
#include "llrect.h"
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
#include "llvector4a.h"
#include "llmatrix4a.h"
#include "lloctree.h"
#include "llvolume.h"

// Library includes from llfilesystem project
#include "lldir.h"

// Library includes from llmessage project
#include "llassetstorage.h"
#include "llavatarnamecache.h"
#include "llcachename.h"
#include "llcorehttputil.h"

// Library includes from llrender project
#include "llgl.h"
#include "llrender.h"

// Library includes from llrender project
#include "llcharacter.h"

// Library includes from llui project
#include "llnotifications.h"
#include "llpanel.h"
#include "llfloater.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/json.hpp>

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/ext/quaternion_float.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/matrix_decompose.hpp"

#endif
