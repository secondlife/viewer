/**
 * @file llcommonutils.h
 * @brief Common utils
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLCOMMONUTILS_H
#define LL_LLCOMMONUTILS_H

#include "lluuid.h"

namespace LLCommonUtils
{
    /**
     * Computes difference between 'vnew' and 'vcur' vectors.
     * Items present in 'vnew' and missing in 'vcur' are treated as added and are copied into 'vadded'
     * Items missing in 'vnew' and present in 'vcur' are treated as removed and are copied into 'vremoved'
     *
     * @param vnew[in] - incoming IDs
     * @param vcur[in] - current IDs
     * @param vadded[out] - difference between incoming and current IDS - added IDs
     * @param vremoved[out] - difference between incoming and current IDS - removed IDs
     */
    LL_COMMON_API void computeDifference(
        const uuid_vec_t& vnew,
        const uuid_vec_t& vcur,
        uuid_vec_t& vadded,
        uuid_vec_t& vremoved);
};

#endif //LL_LLCOMMONUTILS_H

// EOF
