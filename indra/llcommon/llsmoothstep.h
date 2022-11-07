/** 
 * @file llsmoothstep.h
 * @brief Smoothstep - transition from 0 to 1 - function, first and second derivatives all continuous (smooth)
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLSMOOTHSTEP_H
#define LL_LLSMOOTHSTEP_H

template <class LLDATATYPE>
inline LLDATATYPE llsmoothstep(const LLDATATYPE& edge0, const LLDATATYPE& edge1, const LLDATATYPE& value)
{
    if (value < edge0)
        return (LLDATATYPE)0;

    if (value >= edge1)
        return (LLDATATYPE)1;

    // Scale/bias into [0..1] range
    LLDATATYPE scaled_value = (value - edge0) / (edge1 - edge0);

    return scaled_value * scaled_value * (3 - 2 * scaled_value);
}

#endif // LL_LLSMOOTHSTEP_H
