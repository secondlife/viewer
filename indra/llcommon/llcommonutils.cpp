/**
 * @file llcommonutils.h
 * @brief Commin utils
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
