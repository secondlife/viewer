/** 
 * @file llscriptresource_tut.cpp
 * @brief Test LLScriptResource
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008-2009, Linden Research, Inc.
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

//#include <tut/tut.h>
#include "linden_common.h"

#include "lltut.h"

#include "llscriptresource.h"
#include "llscriptresourceconsumer.h"
#include "llscriptresourcepool.h"

class TestConsumer : public LLScriptResourceConsumer
{
public:
	TestConsumer()
		: mUsedURLs(0)
	{ }

	// LLScriptResourceConsumer interface:
	S32 getUsedPublicURLs() const
	{
		return mUsedURLs;
	}

	// Test details:
	S32 mUsedURLs;
};


namespace tut
{
	class LLScriptResourceTestData
	{
	};
	
	typedef test_group<LLScriptResourceTestData> LLScriptResourceTestGroup;
	typedef LLScriptResourceTestGroup::object		LLScriptResourceTestObject;
	LLScriptResourceTestGroup scriptResourceTestGroup("scriptResource");

	template<> template<>
	void LLScriptResourceTestObject::test<1>()
	{
		LLScriptResource resource;
		U32 total = 42;
		
		resource.setTotal(total);
		ensure_equals("Verify set/get total", resource.getTotal(), total);
		ensure_equals("Verify all resources are initially available",resource.getAvailable(),total);

		// Requesting too many, releasing non-allocated
		ensure("Request total + 1 resources should fail",!resource.request(total + 1));
		ensure_equals("Verify all resources available after failed request",resource.getAvailable(),total);

		ensure("Releasing resources when none allocated should fail",!resource.release());
		ensure_equals("All resources should be available after failed release",resource.getAvailable(),total);

		ensure("Request one resource", resource.request());
		ensure_equals("Verify available resources after successful request",resource.getAvailable(),total - 1);

		// Is this right?  Or should we release all used resources if we try to release more than are currently used?
		ensure("Release more resources than allocated",!resource.release(2));
		ensure_equals("Verify resource availability after failed release",resource.getAvailable(),total - 1);

		ensure("Release a resource",resource.release());
		ensure_equals("Verify all resources available after successful release",resource.getAvailable(),total);
	}


	template<> template<>
	void LLScriptResourceTestObject::test<2>()
	{
		LLScriptResource resource;
		U32 total = 42;

		resource.setTotal(total);

		S32 resources_to_request = 30;
		ensure("Get multiple resources resources",resource.request(resources_to_request));
		ensure_equals("Verify available resources is correct after request of multiple resources",resource.getAvailable(), total - resources_to_request);

		S32 resources_to_release = (resources_to_request / 2);
		ensure("Release some resources",resource.release(resources_to_release));

		S32 expected_available = (total - resources_to_request + resources_to_release);
		ensure_equals("Verify available resources after release of some resources",resource.getAvailable(), expected_available);

		resources_to_release = (resources_to_request - resources_to_release);
		ensure("Release remaining resources",resource.release(resources_to_release));

		ensure_equals("Verify available resources after release of remaining resources",resource.getAvailable(), total);
	}

	template<> template<>
	void LLScriptResourceTestObject::test<3>()
	{
		LLScriptResource resource;

		U32 total = 42;
		resource.setTotal(total);

		ensure("Request all resources",resource.request(total));

		U32 low_total = 10;
		ensure("Release all resources",resource.release(total));
		ensure_equals("Verify all resources available after releasing",resource.getAvailable(),total);

		resource.setTotal(low_total);
		ensure_equals("Verify low total resources are available after set",resource.getAvailable(),low_total);
	}
	

	template<> template<>
	void LLScriptResourceTestObject::test<4>()
	{
		S32 big_resource_total = 100;
		S32 small_resource_total = 10;
		LLScriptResourcePool big_pool;
		big_pool.getPublicURLResource().setTotal(big_resource_total);
		LLScriptResourcePool small_pool;
		small_pool.getPublicURLResource().setTotal(small_resource_total);

		TestConsumer consumer;
		LLScriptResourcePool& initial_pool = consumer.getScriptResourcePool();
		ensure("Initial resource pool is 'null'.", (&initial_pool == &LLScriptResourcePool::null));

		consumer.switchScriptResourcePools(big_pool);
		LLScriptResourcePool& get_pool = consumer.getScriptResourcePool();
		ensure("Get resource that was set.", (&big_pool == &get_pool));

		ensure_equals("No public urls in use yet.", consumer.getUsedPublicURLs(),0);

		S32 request_urls = 5;
		consumer.mUsedURLs = request_urls;
		consumer.getScriptResourcePool().getPublicURLResource().request(request_urls);

		ensure_equals("Available urls on big_pool is 5 less than total.",
			big_pool.getPublicURLResource().getAvailable(), big_resource_total - request_urls);

		ensure("Switching from big pool to small pool",
			   consumer.switchScriptResourcePools(small_pool));

		ensure_equals("All resources available to big pool again",
			big_pool.getPublicURLResource().getAvailable(), big_resource_total);

		ensure_equals("Available urls on small pool is 5 less than total.",
			small_pool.getPublicURLResource().getAvailable(), small_resource_total - request_urls);

		ensure("Switching from small pool to big pool",
			   consumer.switchScriptResourcePools(big_pool));

		consumer.getScriptResourcePool().getPublicURLResource().release(request_urls);

		request_urls = 50; // Too many for the small_pool

		consumer.mUsedURLs = request_urls;
		consumer.getScriptResourcePool().getPublicURLResource().request(request_urls);

		// Verify big pool has them
		ensure_equals("Available urls on big pool is 50 less than total.",
			big_pool.getPublicURLResource().getAvailable(), big_resource_total - request_urls);

		// Verify can't switch to small_pool
		ensure("Switching to small pool with too many resources",
			   !consumer.switchScriptResourcePools(small_pool));

		// Verify big pool still accounting for used resources
		ensure_equals("Available urls on big_pool is still 50 less than total.",
			big_pool.getPublicURLResource().getAvailable(), big_resource_total - request_urls);
		
		// Verify small pool still has all resources available.
		ensure_equals("All resources in small pool are still available.",
			small_pool.getPublicURLResource().getAvailable(), small_resource_total);
	}
}
