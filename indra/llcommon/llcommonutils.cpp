/**
 * @file llcommonutils.h
 * @brief Commin utils
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

#include "linden_common.h"
#include "llcommonutils.h"

void LLCommonUtils::computeDifference(
    const uuid_vec_t& vnew,
    const uuid_vec_t& vcur,
    uuid_vec_t& vadded,
    uuid_vec_t& vremoved)
{
    uuid_vec_t vnew_copy(vnew);
    uuid_vec_t vcur_copy(vcur);

    std::sort(vnew_copy.begin(), vnew_copy.end());
    std::sort(vcur_copy.begin(), vcur_copy.end());

    size_t maxsize = llmax(vnew_copy.size(), vcur_copy.size());
    vadded.resize(maxsize);
    vremoved.resize(maxsize);

    uuid_vec_t::iterator it;
    // what was removed
    it = set_difference(vcur_copy.begin(), vcur_copy.end(), vnew_copy.begin(), vnew_copy.end(), vremoved.begin());
    vremoved.erase(it, vremoved.end());

    // what was added
    it = set_difference(vnew_copy.begin(), vnew_copy.end(), vcur_copy.begin(), vcur_copy.end(), vadded.begin());
    vadded.erase(it, vadded.end());
}

// EOF
