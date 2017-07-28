/** 
 * @file llviewerstats.cpp
 * @brief LLViewerStats class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "httpstats.h"
#include "llerror.h"

namespace LLCore
{
HTTPStats::HTTPStats()
{
    resetStats();
}


HTTPStats::~HTTPStats()
{
}

void HTTPStats::resetStats()
{
    mResutCodes.clear();
    mDataDown.reset();
    mDataUp.reset();
}


void HTTPStats::recordResultCode(S32 code)
{
    std::map<S32, S32>::iterator it;

    it = mResutCodes.find(code);

    if (it == mResutCodes.end())
        mResutCodes[code] = 1;
    else
        (*it).second = (*it).second + 1;

}

void HTTPStats::dumpStats()
{
    std::stringstream out;

    out << "HTTPCore Stats" << std::endl;
    out << "Bytes Sent: " << mDataUp.getSum() << std::endl;
    out << "Bytes Recv: " << mDataDown.getSum() << std::endl;
    out << "Result Codes:" << std::endl << "--- -----" << std::endl;


    for (std::map<S32, S32>::iterator it = mResutCodes.begin(); it != mResutCodes.end(); ++it)
    { 
        out << (*it).first << " " << (*it).second << std::endl;
    }

    LL_WARNS("HTTP Core") << out.str() << LL_ENDL;
}


}
