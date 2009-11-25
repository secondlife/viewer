/** 
 * @file llmediadataclient_test.cpp
 * @brief LLMediaDatClient tests
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
 
#include <iostream>
#include "../test/lltut.h"

#include "llsdserialize.h"
#include "llsdutil.h"
#include "llerrorcontrol.h"
#include "llhttpstatuscodes.h"

#include "../llmediadataclient.h"
#include "../llvovolume.h"

#include "../../llprimitive/llmediaentry.cpp"
#include "../../llprimitive/lltextureentry.cpp"
#include "../../llmessage/tests/llcurl_stub.cpp"

#if LL_WINDOWS
#pragma warning (push)
#pragma warning (disable : 4702) // boost::lexical_cast generates this warning
#endif
#include <boost/lexical_cast.hpp>
#if LL_WINDOWS
#pragma warning (pop)
#endif

#define VALID_OBJECT_ID   "3607d5c4-644b-4a8a-871a-8b78471af2a2"
#define VALID_OBJECT_ID_1 "11111111-1111-1111-1111-111111111111"
#define VALID_OBJECT_ID_2 "22222222-2222-2222-2222-222222222222"
#define VALID_OBJECT_ID_3 "33333333-3333-3333-3333-333333333333"
#define VALID_OBJECT_ID_4 "44444444-4444-4444-4444-444444444444"

#define FAKE_OBJECT_MEDIA_CAP_URL "foo_ObjectMedia"
#define FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL "foo_ObjectMediaNavigate"
#define FAKE_OBJECT_MEDIA_CAP_URL_503 "foo_ObjectMedia_503"
#define FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR "foo_ObjectMediaNavigate_ERROR"

#define MEDIA_DATA "\
<array>														\
<string>foo</string>										\
<string>bar</string>										\
<string>baz</string>										\
</array>"

#define _DATA_URLS(ID,DIST,INT,URL1,URL2) "					\
<llsd>											\
  <map>											\
    <key>uuid</key>								\
    <string>" ID "</string>						\
    <key>distance</key>											\
    <real>" DIST "</real>										\
    <key>interest</key>											\
    <real>" INT "</real>											\
    <key>cap_urls</key>											\
    <map>														\
      <key>ObjectMedia</key>									\
      <string>" URL1 "</string>			\
      <key>ObjectMediaNavigate</key>							\
      <string>" URL2 "</string>	\
    </map>														\
    <key>media_data</key>                                       \
	" MEDIA_DATA "												\
  </map>														\
</llsd>"

#define _DATA(ID,DIST,INT) _DATA_URLS(ID,DIST,INT,FAKE_OBJECT_MEDIA_CAP_URL,FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL)

const char *DATA = _DATA(VALID_OBJECT_ID,"1.0","1.0");
	
#define STR(I) boost::lexical_cast<std::string>(I)

#define LOG_TEST(N) LL_DEBUGS("LLMediaDataClient") << "\n" <<			\
"================================================================================\n" << \
"===================================== TEST " #N " ===================================\n" << \
"================================================================================\n" << LL_ENDL;

LLSD *gPostRecords = NULL;

// stubs:
void LLHTTPClient::post(
		const std::string& url,
		const LLSD& body,
		LLHTTPClient::ResponderPtr responder,
		const LLSD& headers,
		const F32 timeout)
{
	LLSD record;
	record["url"] = url;
	record["body"] = body;
	record["headers"] = headers;
	record["timeout"] = timeout;
	gPostRecords->append(record);
	
	// Magic URL that triggers a 503:
	if ( url == FAKE_OBJECT_MEDIA_CAP_URL_503 )
	{
		responder->error(HTTP_SERVICE_UNAVAILABLE, "fake reason");
	}
	else if (url == FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR) 
	{
		LLSD result;
		LLSD error;
		error["code"] = LLObjectMediaNavigateClient::ERROR_PERMISSION_DENIED_CODE;
		result["error"] = error;
		responder->result(result);
	}
	else {
		responder->result(LLSD());
	}
}

const F32 HTTP_REQUEST_EXPIRY_SECS = 60.0f;

class LLMediaDataClientObjectTest : public LLMediaDataClientObject
{
public:
	LLMediaDataClientObjectTest(const char *data) 
		{
			std::istringstream d(data);
			LLSDSerialize::fromXML(mRep, d);
			mNumBounceBacks = 0;
			mDead = false;
            
           // std::cout << ll_pretty_print_sd(mRep) << std::endl;
           // std::cout << "ID: " << getID() << std::endl;
		}
	LLMediaDataClientObjectTest(const LLSD &rep) 
		: mRep(rep), mNumBounceBacks(0), mDead(false) {}
	~LLMediaDataClientObjectTest()
		{ LL_DEBUGS("LLMediaDataClient") << "~LLMediaDataClientObjectTest" << LL_ENDL; }
	
	virtual U8 getMediaDataCount() const 
		{ return mRep["media_data"].size(); }
	virtual LLSD getMediaDataLLSD(U8 index) const
		{ return mRep["media_data"][(LLSD::Integer)index]; }
	virtual LLUUID getID() const 
		{ return mRep["uuid"]; }
	virtual void mediaNavigateBounceBack(U8 index)
		{
			mNumBounceBacks++;
		}
	
	virtual bool hasMedia() const
		{ return mRep.has("media_data"); }
	
	virtual void updateObjectMediaData(LLSD const &media_data_array)
		{ mRep["media_data"] = media_data_array; }
	
	virtual F64 getDistanceFromAvatar() const
		{ return (LLSD::Real)mRep["distance"]; }
	
	virtual F64 getTotalMediaInterest() const
		{ return (LLSD::Real)mRep["interest"]; }

	virtual std::string getCapabilityUrl(const std::string &name) const 
		{ return mRep["cap_urls"][name]; }

	virtual bool isDead() const
		{ return mDead; }

	void setDistanceFromAvatar(F64 val)
		{ mRep["distance"] = val; }
	
	void setTotalMediaInterest(F64 val)
		{ mRep["interest"] = val; }

	int getNumBounceBacks() const
		{ return mNumBounceBacks; }
	
	void markDead()
		{ mDead = true; }
private:
	LLSD mRep;
	int mNumBounceBacks;
	bool mDead;
};

// This special timer delay should ensure that the timer will fire on the very
// next pump, no matter what (though this does make an assumption about the
// implementation of LLEventTimer::updateClass()):
const F32 NO_PERIOD = -1000.0f;

static void pump_timers()
{
	LLEventTimer::updateClass();
}

namespace tut
{
    struct mediadataclient
    {
		mediadataclient() {
			gPostRecords = &mLLSD;
			
 			//LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
 			//LLError::setClassLevel("LLMediaDataClient", LLError::LEVEL_DEBUG);
			//LLError::setTagLevel("MediaOnAPrim", LLError::LEVEL_DEBUG);
		}
		LLSD mLLSD;
    };
    
	typedef test_group<mediadataclient> mediadataclient_t;
	typedef mediadataclient_t::object mediadataclient_object_t;
	tut::mediadataclient_t tut_mediadataclient("mediadataclient");
    
    void ensure(const std::string &msg, int value, int expected)
    {
        std::string m = msg;
        m += " value: " + STR(value);
        m += ", expected: " + STR(expected);
        ensure(m, value == expected);
    }
    
    void ensure(const std::string &msg, const std::string & value, const std::string & expected)
    {
        std::string m = msg;
        m += " value: " + value;
        m += ", expected: " + expected;
        ensure(m, value == expected);
    }
    
    void ensure(const std::string &msg, const LLUUID & value, const LLUUID & expected)
    {
        std::string m = msg;
        m += " value: " + value.asString();
        m += ", expected: " + expected.asString();
        ensure(m, value == expected);
    }
    
    void ensure_llsd(const std::string &msg, const LLSD & value, const char *expected)
    {
        LLSD expected_llsd;
        std::istringstream e(expected);
        LLSDSerialize::fromXML(expected_llsd, e);
   
        std::string value_str = ll_pretty_print_sd(value);
        std::string expected_str = ll_pretty_print_sd(expected_llsd);
        std::string m = msg;
        m += " value: " + value_str;
        m += ", expected: " + expected_str;
        ensure(m, value_str == expected_str);
    }

	//////////////////////////////////////////////////////////////////////////////////////////
	
	template<> template<>
	void mediadataclient_object_t::test<1>()
	{
		//
		// Test fetchMedia()
		//
		LOG_TEST(1);
		
		LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(DATA);
		int num_refs_start = o->getNumRefs();
		{
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
			mdc->fetchMedia(o);

			// Make sure no posts happened yet...
			ensure("post records", gPostRecords->size(), 0);

			::pump_timers();
		
			ensure("post records", gPostRecords->size(), 1);
			ensure("post url", (*gPostRecords)[0]["url"], FAKE_OBJECT_MEDIA_CAP_URL);
			ensure("post GET", (*gPostRecords)[0]["body"]["verb"], "GET");
			ensure("post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
			ensure("queue empty", mdc->isEmpty());
		}
		
		// Make sure everyone's destroyed properly
		ensure("REF COUNT", o->getNumRefs(), num_refs_start);
    }

	//////////////////////////////////////////////////////////////////////////////////////////

	template<> template<>
	void mediadataclient_object_t::test<2>()
	{
		//
		// Test updateMedia()
		//
		LOG_TEST(2);

		LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(DATA);
		{
			// queue time w/ no delay ensures that ::pump_timers() will hit the tick()
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);  
			mdc->updateMedia(o);
			ensure("post records", gPostRecords->size(), 0);
			::pump_timers();
		
			ensure("post records", gPostRecords->size(), 1);
			ensure("post url", (*gPostRecords)[0]["url"], FAKE_OBJECT_MEDIA_CAP_URL);
			ensure("post UPDATE", (*gPostRecords)[0]["body"]["verb"], "UPDATE");
			ensure("post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
			ensure_llsd("post data llsd", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_MEDIA_DATA_KEY], 
						"<llsd>" MEDIA_DATA "</llsd>");
			ensure("queue empty", mdc->isEmpty());
		}

		ensure("REF COUNT", o->getNumRefs(), 1);
	}

	//////////////////////////////////////////////////////////////////////////////////////////

    template<> template<>
    void mediadataclient_object_t::test<3>()
    {
		//
		// Test navigate()
		//
		LOG_TEST(3);

		LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(DATA);
		{		
			LLPointer<LLObjectMediaNavigateClient> mdc = new LLObjectMediaNavigateClient(NO_PERIOD,NO_PERIOD);
			const char *TEST_URL = "http://example.com";
			mdc->navigate(o, 0, TEST_URL);
			ensure("post records", gPostRecords->size(), 0);
			::pump_timers();

			// ensure no bounce back
			ensure("bounce back", dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o))->getNumBounceBacks(), 0);
		
			ensure("post records", gPostRecords->size(), 1);
			ensure("post url", (*gPostRecords)[0]["url"], FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL);
			ensure("post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
			ensure("post data", (*gPostRecords)[0]["body"][LLTextureEntry::TEXTURE_INDEX_KEY], 0);
			ensure("post data", (*gPostRecords)[0]["body"][LLMediaEntry::CURRENT_URL_KEY], TEST_URL);
			ensure("queue empty", mdc->isEmpty());
		}
		ensure("REF COUNT", o->getNumRefs(), 1);
    }
	
	//////////////////////////////////////////////////////////////////////////////////////////

    template<> template<>
    void mediadataclient_object_t::test<4>()
    {
		//
		// Test queue ordering
		//
		LOG_TEST(4);

		LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(
			_DATA(VALID_OBJECT_ID_1,"3.0","1.0"));
		LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(
			_DATA(VALID_OBJECT_ID_2,"1.0","1.0"));
		LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(
			_DATA(VALID_OBJECT_ID_3,"2.0","1.0"));
		{
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);  
			const char *ORDERED_OBJECT_IDS[] = { VALID_OBJECT_ID_2, VALID_OBJECT_ID_3, VALID_OBJECT_ID_1 };
			mdc->fetchMedia(o1);
			mdc->fetchMedia(o2);
			mdc->fetchMedia(o3);

			// Make sure no posts happened yet...
			ensure("post records", gPostRecords->size(), 0);

			// tick 3 times...
			::pump_timers();
			ensure("post records", gPostRecords->size(), 1);
			::pump_timers();
			ensure("post records", gPostRecords->size(), 2);
			::pump_timers();
			ensure("post records", gPostRecords->size(), 3);
		
			for( int i=0; i < 3; i++ )
			{
				ensure("[" + STR(i) + "] post url", (*gPostRecords)[i]["url"], FAKE_OBJECT_MEDIA_CAP_URL);
				ensure("[" + STR(i) + "] post GET", (*gPostRecords)[i]["body"]["verb"], "GET");
				ensure("[" + STR(i) + "] post object id", (*gPostRecords)[i]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), 
					   LLUUID(ORDERED_OBJECT_IDS[i]));
			}

			ensure("queue empty", mdc->isEmpty());
		}
		ensure("refcount of o1", o1->getNumRefs(), 1);
		ensure("refcount of o2", o2->getNumRefs(), 1);
		ensure("refcount of o3", o3->getNumRefs(), 1);
    }

	//////////////////////////////////////////////////////////////////////////////////////////

	template<> template<>
	void mediadataclient_object_t::test<5>()
	{
		//
		// Test fetchMedia() getting a 503 error
		//
		LOG_TEST(5);
		
		LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(
			_DATA_URLS(VALID_OBJECT_ID,
					   "1.0",
					   "1.0",
					   FAKE_OBJECT_MEDIA_CAP_URL_503,
					   FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL));
		int num_refs_start = o->getNumRefs();
		{
			const int NUM_RETRIES = 5;
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD,NUM_RETRIES);

			// This should generate a retry
			mdc->fetchMedia(o);

			// Make sure no posts happened yet...
			ensure("post records before", gPostRecords->size(), 0);

			// Once, causes retry
			// Second, fires retry timer
			// Third, fires queue timer again
			for (int i=0; i<NUM_RETRIES; ++i)
			{
				::pump_timers();  // Should pump (fire) the queue timer, causing a retry timer to be scheduled
				// XXX This ensure is not guaranteed, because scheduling a timer might actually get it pumped in the same loop
				//ensure("post records " + STR(i), gPostRecords->size(), i+1);
				::pump_timers();  // Should pump (fire) the retry timer, scheduling the queue timer
			}

			// Do some extra pumps to make sure no other timer work occurs.
			::pump_timers();
			::pump_timers();
			::pump_timers();
			
			// Make sure there were 2 posts
			ensure("post records after", gPostRecords->size(), NUM_RETRIES);
			for (int i=0; i<NUM_RETRIES; ++i)
			{
				ensure("[" + STR(i) + "] post url", (*gPostRecords)[i]["url"], FAKE_OBJECT_MEDIA_CAP_URL_503);
				ensure("[" + STR(i) + "] post GET", (*gPostRecords)[i]["body"]["verb"], "GET");
				ensure("[" + STR(i) + "] post object id", (*gPostRecords)[i]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
			}
			ensure("queue empty", mdc->isEmpty());
		}
		
		// Make sure everyone's destroyed properly
		ensure("REF COUNT", o->getNumRefs(), num_refs_start);
    }

    template<> template<>
    void mediadataclient_object_t::test<6>()
    {
		//
		// Test navigate() with a bounce back
		//
		LOG_TEST(6);

		LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(
			_DATA_URLS(VALID_OBJECT_ID,
					   "1.0",
					   "1.0",
					   FAKE_OBJECT_MEDIA_CAP_URL,
					   FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR));
		{		
			LLPointer<LLObjectMediaNavigateClient> mdc = new LLObjectMediaNavigateClient(NO_PERIOD,NO_PERIOD);
			const char *TEST_URL = "http://example.com";
			mdc->navigate(o, 0, TEST_URL);
			ensure("post records", gPostRecords->size(), 0);
			::pump_timers();

			// ensure bounce back
			ensure("bounce back", 
				   dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o))->getNumBounceBacks(),
				   1);
			
			ensure("post records", gPostRecords->size(), 1);
			ensure("post url", (*gPostRecords)[0]["url"], FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR);
			ensure("post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
			ensure("post data", (*gPostRecords)[0]["body"][LLTextureEntry::TEXTURE_INDEX_KEY], 0);
			ensure("post data", (*gPostRecords)[0]["body"][LLMediaEntry::CURRENT_URL_KEY], TEST_URL);
			ensure("queue empty", mdc->isEmpty());
		}
		ensure("REF COUNT", o->getNumRefs(), 1);
    }

	template<> template<>
    void mediadataclient_object_t::test<7>()
    {
		// Test LLMediaDataClient::isInQueue()
		LOG_TEST(7);
		
		LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(
			_DATA(VALID_OBJECT_ID_1,"3.0","1.0"));
		LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(
			_DATA(VALID_OBJECT_ID_2,"1.0","1.0"));
		int num_refs_start = o1->getNumRefs();
		{
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
			
			ensure("not in queue yet 1", ! mdc->isInQueue(o1));
			ensure("not in queue yet 2", ! mdc->isInQueue(o2));
			
			mdc->fetchMedia(o1);
			
			ensure("is in queue", mdc->isInQueue(o1));
			ensure("is not in queue", ! mdc->isInQueue(o2));
			
			::pump_timers();
			
			ensure("not in queue anymore", ! mdc->isInQueue(o1));
			ensure("still is not in queue", ! mdc->isInQueue(o2));
			
			ensure("queue empty", mdc->isEmpty());
		}
		
		// Make sure everyone's destroyed properly
		ensure("REF COUNT", o1->getNumRefs(), num_refs_start);
		
	}

	template<> template<>
    void mediadataclient_object_t::test<8>()
    {
		// Test queue handling of objects that are marked dead.
		LOG_TEST(8);
		
		LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"1.0","1.0"));
		LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"2.0","1.0"));
		LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"3.0","1.0"));
		LLMediaDataClientObject::ptr_t o4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"4.0","1.0"));
		{
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
			
			// queue up all 4 objects
			mdc->fetchMedia(o1);
			mdc->fetchMedia(o2);
			mdc->fetchMedia(o3);
			mdc->fetchMedia(o4);
			
			// and mark the second and fourth ones dead.
			dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o2))->markDead();
			dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o4))->markDead();

			ensure("is in queue 1", mdc->isInQueue(o1));
			ensure("is in queue 2", mdc->isInQueue(o2));
			ensure("is in queue 3", mdc->isInQueue(o3));
			ensure("is in queue 4", mdc->isInQueue(o4));
			ensure("post records", gPostRecords->size(), 0);
			
			::pump_timers();
			
			// The first tick should remove the first one 
			ensure("is not in queue 1", !mdc->isInQueue(o1));
			ensure("is in queue 2", mdc->isInQueue(o2));
			ensure("is in queue 3", mdc->isInQueue(o3));
			ensure("is in queue 4", mdc->isInQueue(o4));
			ensure("post records", gPostRecords->size(), 1);
			
			::pump_timers();
			
			// The second tick should skip the second and remove the third
			ensure("is not in queue 2", !mdc->isInQueue(o2));
			ensure("is not in queue 3", !mdc->isInQueue(o3));
			ensure("is in queue 4", mdc->isInQueue(o4));
			ensure("post records", gPostRecords->size(), 2);

			::pump_timers();

			// The third tick should skip the fourth one and empty the queue.
			ensure("is not in queue 4", !mdc->isInQueue(o4));
			ensure("post records", gPostRecords->size(), 2);

			ensure("queue empty", mdc->isEmpty());
		}
		ensure("refcount of o1", o1->getNumRefs(), 1);
		ensure("refcount of o2", o2->getNumRefs(), 1);
		ensure("refcount of o3", o3->getNumRefs(), 1);
		ensure("refcount of o4", o4->getNumRefs(), 1);

	}
	
	//////////////////////////////////////////////////////////////////////////////////////////

    template<> template<>
    void mediadataclient_object_t::test<9>()
    {
		//
		// Test queue re-ordering
		//
		LOG_TEST(9);
		
		LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"10.0","1.0"));
		LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"20.0","1.0"));
		LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"30.0","1.0"));
		LLMediaDataClientObject::ptr_t o4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"40.0","1.0"));
		{
			LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
			
			// queue up all 4 objects.  They should now be in the queue in
			// order 1 through 4, with 4 being at the front of the queue
			mdc->fetchMedia(o1);
			mdc->fetchMedia(o2);
			mdc->fetchMedia(o3);
			mdc->fetchMedia(o4);
			
			int test_num = 0;
			
			ensure(STR(test_num) + ". is in queue 1", mdc->isInQueue(o1));
			ensure(STR(test_num) + ". is in queue 2", mdc->isInQueue(o2));
			ensure(STR(test_num) + ". is in queue 3", mdc->isInQueue(o3));
			ensure(STR(test_num) + ". is in queue 4", mdc->isInQueue(o4));
			ensure(STR(test_num) + ". post records", gPostRecords->size(), 0);
			
			::pump_timers();
			++test_num;
			
			// The first tick should remove the first one 
			ensure(STR(test_num) + ". is not in queue 1", !mdc->isInQueue(o1));
			ensure(STR(test_num) + ". is in queue 2", mdc->isInQueue(o2));
			ensure(STR(test_num) + ". is in queue 3", mdc->isInQueue(o3));
			ensure(STR(test_num) + ". is in queue 4", mdc->isInQueue(o4));
			ensure(STR(test_num) + ". post records", gPostRecords->size(), 1);
			
			// Now, pretend that object 4 moved relative to the avatar such
			// that it is now closest
			static_cast<LLMediaDataClientObjectTest*>(
				static_cast<LLMediaDataClientObject*>(o4))->setDistanceFromAvatar(5.0);
			
			::pump_timers();
			++test_num;
			
			// The second tick should still pick off item 2, but then re-sort
			// have picked off object 4
			ensure(STR(test_num) + ". is in queue 2", mdc->isInQueue(o2));
			ensure(STR(test_num) + ". is in queue 3", mdc->isInQueue(o3));
			ensure(STR(test_num) + ". is not in queue 4", !mdc->isInQueue(o4));
			ensure(STR(test_num) + ". post records", gPostRecords->size(), 2);

			::pump_timers();
			++test_num;
			
			// The third tick should pick off object 2
			ensure(STR(test_num) + ". is not in queue 2", !mdc->isInQueue(o2));
			ensure(STR(test_num) + ". is in queue 3", mdc->isInQueue(o3));
			ensure(STR(test_num) + ". post records", gPostRecords->size(), 3);

			// The fourth tick should pick off object 3
			::pump_timers();
			++test_num;

			ensure(STR(test_num) + ". is not in queue 3", !mdc->isInQueue(o3));
			ensure(STR(test_num) + ". post records", gPostRecords->size(), 4);

			ensure("queue empty", mdc->isEmpty());
		}
		ensure("refcount of o1", o1->getNumRefs(), 1);
		ensure("refcount of o2", o2->getNumRefs(), 1);
		ensure("refcount of o3", o3->getNumRefs(), 1);
		ensure("refcount of o4", o4->getNumRefs(), 1);
    }
	
}
