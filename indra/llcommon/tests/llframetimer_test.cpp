/** 
 * @file lltiming_test.cpp
 * @date 2006-07-23
 * @brief Tests the timers.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "../llframetimer.h"
#include "../llsd.h"

#include "../test/lltut.h"

namespace tut
{
	struct frametimer_test
	{
		frametimer_test()
		{
			LLFrameTimer::updateFrameTime();			
		}
	};
	typedef test_group<frametimer_test> frametimer_group_t;
	typedef frametimer_group_t::object frametimer_object_t;
	tut::frametimer_group_t frametimer_instance("frametimer");

	template<> template<>
	void frametimer_object_t::test<1>()
	{
		F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
		LLFrameTimer timer;
		timer.setExpiryAt(seconds_since_epoch);
		F64 expires_at = timer.expiresAt();
		ensure_distance(
			"set expiry matches get expiry",
			expires_at,
			seconds_since_epoch,
			0.001);
	}

	template<> template<>
	void frametimer_object_t::test<2>()
	{
		F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
		seconds_since_epoch += 10.0;
		LLFrameTimer timer;
		timer.setExpiryAt(seconds_since_epoch);
		F64 expires_at = timer.expiresAt();
		ensure_distance(
			"set expiry matches get expiry 1",
			expires_at,
			seconds_since_epoch,
			0.001);
		seconds_since_epoch += 10.0;
		timer.setExpiryAt(seconds_since_epoch);
		expires_at = timer.expiresAt();
		ensure_distance(
			"set expiry matches get expiry 2",
			expires_at,
			seconds_since_epoch,
			0.001);
	}
	template<> template<>
	void frametimer_object_t::test<3>()
	{
		F64 seconds_since_epoch = LLFrameTimer::getTotalSeconds();
		seconds_since_epoch += 2.0;
		LLFrameTimer timer;
		timer.setExpiryAt(seconds_since_epoch);
		ensure("timer not expired on create", !timer.hasExpired());
		int ii;
		for(ii = 0; ii < 10; ++ii)
		{
			ms_sleep(150);
			LLFrameTimer::updateFrameTime();			
		}
		ensure("timer not expired after a bit", !timer.hasExpired());
		for(ii = 0; ii < 10; ++ii)
		{
			ms_sleep(100);
			LLFrameTimer::updateFrameTime();			
		}
		ensure("timer expired", timer.hasExpired());
	}
/*
	template<> template<>
	void frametimer_object_t::test<4>()
	{
	}
*/
}
