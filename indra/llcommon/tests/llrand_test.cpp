/** 
 * @file llrandom_tut.cpp
 * @author Phoenix
 * @date 2007-01-25
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
#include "../test/lltut.h"

#include "../llrand.h"


namespace tut
{
	struct random
	{
	};

	typedef test_group<random> random_t;
	typedef random_t::object random_object_t;
	tut::random_t tut_random("random");

	template<> template<>
	void random_object_t::test<1>()
	{
		F32 number = 0.0f;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_frand();
			ensure("frand >= 0", (number >= 0.0f));
			ensure("frand < 1", (number < 1.0f));
		}
	}

	template<> template<>
	void random_object_t::test<2>()
	{
		F64 number = 0.0f;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_drand();
			ensure("drand >= 0", (number >= 0.0));
			ensure("drand < 1", (number < 1.0));
		}
	}

	template<> template<>
	void random_object_t::test<3>()
	{
		F32 number = 0.0f;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_frand(2.0f) - 1.0f;
			ensure("frand >= 0", (number >= -1.0f));
			ensure("frand < 1", (number <= 1.0f));
		}
	}

	template<> template<>
	void random_object_t::test<4>()
	{
		F32 number = 0.0f;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_frand(-7.0);
			ensure("drand <= 0", (number <= 0.0));
			ensure("drand > -7", (number > -7.0));
		}
	}

	template<> template<>
	void random_object_t::test<5>()
	{
		F64 number = 0.0f;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_drand(-2.0);
			ensure("drand <= 0", (number <= 0.0));
			ensure("drand > -2", (number > -2.0));
		}
	}

	template<> template<>
	void random_object_t::test<6>()
	{
		S32 number = 0;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_rand(100);
			ensure("rand >= 0", (number >= 0));
			ensure("rand < 100", (number < 100));
		}
	}

	template<> template<>
	void random_object_t::test<7>()
	{
		S32 number = 0;
		for(S32 ii = 0; ii < 100000; ++ii)
		{
			number = ll_rand(-127);
			ensure("rand <= 0", (number <= 0));
			ensure("rand > -127", (number > -127));
		}
	}
}
