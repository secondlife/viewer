/**
 * @file   llavatarrenderinfoaccountant.h
 * @author Dave Simmons
 * @date   2013-02-28
 * @brief  
 * 
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#if ! defined(LL_llavatarrenderinfoaccountant_H)
#define LL_llavatarrenderinfoaccountant_H

#include "httpcommon.h"
#include "llcoros.h"

class LLViewerRegion;

// Class to gather avatar rendering information 
// that is sent to or fetched from regions.
class LLAvatarRenderInfoAccountant
{
public:

	static void sendRenderInfoToRegion(LLViewerRegion * regionp);
	static void getRenderInfoFromRegion(LLViewerRegion * regionp);

	static void expireRenderInfoReportTimer(const LLUUID& region_id);

    static void idle();

	static bool logRenderInfo();

private:
	LLAvatarRenderInfoAccountant() {};
	~LLAvatarRenderInfoAccountant()	{};

	// Send data updates about once per minute, only need per-frame resolution
	static LLFrameTimer sRenderInfoReportTimer;

    static void avatarRenderInfoGetCoro(std::string url, U64 regionHandle);
    static void avatarRenderInfoReportCoro(std::string url, U64 regionHandle);


};

#endif /* ! defined(LL_llavatarrenderinfoaccountant_H) */
