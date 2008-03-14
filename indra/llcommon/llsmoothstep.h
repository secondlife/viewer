/** 
 * @file llsmoothstep.h
 * @brief Smoothstep - transition from 0 to 1 - function, first and second derivatives all continuous (smooth)
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
