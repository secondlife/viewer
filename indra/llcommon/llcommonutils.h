/**
 * @file llcommonutils.h
 * @brief Common utils
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 *
 * Copyright (c) 2010, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLCOMMONUTILS_H
#define LL_LLCOMMONUTILS_H

namespace LLCommonUtils
{
	/**
	 * Computes difference between 'vnew' and 'vcur' vectors.
	 * Items present in 'vnew' and missing in 'vcur' are treated as added and are copied into 'vadded'
	 * Items missing in 'vnew' and present in 'vcur' are treated as removed and are copied into 'vremoved'
	 */
	LL_COMMON_API void computeDifference(
		const uuid_vec_t& vnew,
		const uuid_vec_t& vcur,
		uuid_vec_t& vadded,
		uuid_vec_t& vremoved);
};

#endif //LL_LLCOMMONUTILS_H

// EOF
