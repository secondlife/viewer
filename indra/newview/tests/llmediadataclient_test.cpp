/**
 * @file llmediadataclient_test.cpp
 * @brief LLMediaDatClient tests
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

#include "linden_common.h"
#include "../llviewerprecompiledheaders.h"

#include <iostream>
#include "../test/lldoctest.h"

#include "llsdserialize.h"
#include "llsdutil.h"
#include "llerrorcontrol.h"
#include "llhttpconstants.h"

#include "../llmediadataclient.h"
#include "../llvovolume.h"

#include "../../llprimitive/llmediaentry.cpp"
#include "../../llprimitive/llmaterialid.cpp"
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
<array>                                                     \
<string>http://foo.example.com</string>                                     \
<string>http://bar.example.com</string>                                     \
<string>baz</string>                                        \
</array>"

#define _DATA_URLS(ID,INTEREST,NEW,URL1,URL2) "                 \
<llsd>                                          \
  <map>                                         \
    <key>uuid</key>                             \
    <string>" ID "</string>                     \
    <key>interest</key>                                         \
    <real>" INTEREST "</real>                                           \
    <key>cap_urls</key>                                         \
    <map>                                                       \
      <key>ObjectMedia</key>                                    \
      <string>" URL1 "</string>         \
      <key>ObjectMediaNavigate</key>                            \
      <string>" URL2 "</string> \
    </map>                                                      \
    <key>media_data</key>                                       \
    " MEDIA_DATA "                                              \
    <key>is_dead</key>                                          \
    <boolean>false</boolean>                                    \
    <key>is_new</key>                                           \
    <boolean>" NEW "</boolean>                                  \
  </map>                                                        \
</llsd>"

#define _DATA(ID,INTEREST,NEW) _DATA_URLS(ID,INTEREST,NEW,FAKE_OBJECT_MEDIA_CAP_URL,FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL)

const char *DATA = _DATA(VALID_OBJECT_ID,"1.0","true");

#define STR(I) boost::lexical_cast<std::string>(I)

#define LOG_TEST(N) LL_DEBUGS("LLMediaDataClient") << "\n" <<           \
"================================================================================\n" << \
"==================================== TEST " #N " ===================================\n" << \
"================================================================================\n" << LL_ENDL;

LLSD *gPostRecords = NULL;
F64   gMinimumInterestLevel = (F64)0.0;
#if 0
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
    LLSD result;
    result[LLTextureEntry::OBJECT_ID_KEY] = body[LLTextureEntry::OBJECT_ID_KEY];
    if ( url == FAKE_OBJECT_MEDIA_CAP_URL_503 )
    {
        LLSD content;
        content["reason"] = "fake reason";
        responder->failureResult(HTTP_SERVICE_UNAVAILABLE, "fake reason", content);
        return;
    }
    else if (url == FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR)
    {
        LLSD error;
        error["code"] = LLObjectMediaNavigateClient::ERROR_PERMISSION_DENIED_CODE;
        result["error"] = error;
    }
    responder->successResult(result);
}
#endif

const F32 HTTP_REQUEST_EXPIRY_SECS = 60.0f;

class LLMediaDataClientObjectTest : public LLMediaDataClientObject
{
public:
    LLMediaDataClientObjectTest(const char *data)
        {
            std::istringstream d(data);
            LLSDSerialize::fromXML(mRep, d);
            mNumBounceBacks = 0;

           // std::cout << ll_pretty_print_sd(mRep) << std::endl;
           // std::cout << "ID: " << getID() << std::endl;
        }
    LLMediaDataClientObjectTest(const LLSD &rep)
        : mRep(rep), mNumBounceBacks(0) {}
    ~LLMediaDataClientObjectTest()
        { LL_DEBUGS("LLMediaDataClient") << "~LLMediaDataClientObjectTest" << LL_ENDL; }

    virtual U8 getMediaDataCount() const
        { return mRep["media_data"].size(); }
    virtual LLSD getMediaDataLLSD(U8 index) const
        { return mRep["media_data"][(LLSD::Integer)index]; }
    virtual bool isCurrentMediaUrl(U8 index, const std::string &url) const
        { return (mRep["media_data"][(LLSD::Integer)index].asString() == url); }
    virtual LLUUID getID() const
        { return mRep["uuid"]; }
    virtual void mediaNavigateBounceBack(U8 index)
        { mNumBounceBacks++; }

    virtual bool hasMedia() const
        { return mRep.has("media_data"); }

    virtual void updateObjectMediaData(LLSD const &media_data_array, const std::string &media_version)
        { mRep["media_data"] = media_data_array; mRep["media_version"] = media_version; }

    virtual F64 getMediaInterest() const
        { return (LLSD::Real)mRep["interest"]; }

    virtual bool isInterestingEnough() const
        { return getMediaInterest() > gMinimumInterestLevel; }

    virtual std::string getCapabilityUrl(const std::string &name) const
        { return mRep["cap_urls"][name]; }

    virtual bool isDead() const
        { return mRep["is_dead"]; }

    virtual U32 getMediaVersion() const
        { return (LLSD::Integer)mRep["media_version"]; }

    virtual bool isNew() const
        { return mRep["is_new"]; }

    void setMediaInterest(F64 val)
        { mRep["interest"] = val; }

    int getNumBounceBacks() const
        { return mNumBounceBacks; }

    void markDead()
        { mRep["is_dead"] = true; }

    void markOld()
        { mRep["is_new"] = false; }

private:
    LLSD mRep;
    int mNumBounceBacks;
};

// This special timer delay should ensure that the timer will fire on the very
// next pump, no matter what (though this does make an assumption about the
// implementation of LLEventTimer::updateClass()):
const F32 NO_PERIOD = -1000.0f;

static void pump_timers()
{
    LLEventTimer::updateClass();
}

TEST_SUITE("LLMediaDataClient") {

struct mediadataclient
{

        mediadataclient() {
            gPostRecords = &mLLSD;
            gMinimumInterestLevel = (F64)0.0;

//          LLError::setDefaultLevel(LLError::LEVEL_DEBUG);
//          LLError::setClassLevel("LLMediaDataClient", LLError::LEVEL_DEBUG);
//          LLError::setTagLevel("MediaOnAPrim", LLError::LEVEL_DEBUG);
        
};

TEST_CASE_FIXTURE(mediadataclient, "test_1")
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
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);

            ::pump_timers();

            CHECK_MESSAGE(gPostRecords->size(, "post records"), 1);
            CHECK_MESSAGE((*gPostRecords, "post url")[0]["url"], FAKE_OBJECT_MEDIA_CAP_URL);
            CHECK_MESSAGE((*gPostRecords, "post GET")[0]["body"]["verb"], "GET");
            CHECK_MESSAGE((*gPostRecords, "post object id")[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_2")
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
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);
            ::pump_timers();

            CHECK_MESSAGE(gPostRecords->size(, "post records"), 1);
            CHECK_MESSAGE((*gPostRecords, "post url")[0]["url"], FAKE_OBJECT_MEDIA_CAP_URL);
            CHECK_MESSAGE((*gPostRecords, "post UPDATE")[0]["body"]["verb"], "UPDATE");
            CHECK_MESSAGE((*gPostRecords, "post object id")[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
            ensure_llsd("post data llsd", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_MEDIA_DATA_KEY],
                        "<llsd>" MEDIA_DATA "</llsd>");
            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_3")
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
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);
            ::pump_timers();

            // ensure no bounce back
            CHECK_MESSAGE(dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o, "bounce back"))->getNumBounceBacks(), 0);

            CHECK_MESSAGE(gPostRecords->size(, "post records"), 1);
            CHECK_MESSAGE((*gPostRecords, "post url")[0]["url"], FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL);
            CHECK_MESSAGE((*gPostRecords, "post object id")[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
            CHECK_MESSAGE((*gPostRecords, "post data")[0]["body"][LLTextureEntry::TEXTURE_INDEX_KEY], 0);
            CHECK_MESSAGE((*gPostRecords, "post data")[0]["body"][LLMediaEntry::CURRENT_URL_KEY], TEST_URL);
            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_4")
{

        //
        // Test queue ordering
        //
        LOG_TEST(4);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(
            _DATA(VALID_OBJECT_ID_1,"1.0","true"));
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(
            _DATA(VALID_OBJECT_ID_2,"3.0","true"));
        LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(
            _DATA(VALID_OBJECT_ID_3,"2.0","true"));
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
            const char *ORDERED_OBJECT_IDS[] = { VALID_OBJECT_ID_2, VALID_OBJECT_ID_3, VALID_OBJECT_ID_1 
}

TEST_CASE_FIXTURE(mediadataclient, "test_5")
{

        //
        // Test fetchMedia() getting a 503 error
        //
        LOG_TEST(5);

        LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(
            _DATA_URLS(VALID_OBJECT_ID,
                       "1.0","true",
                       FAKE_OBJECT_MEDIA_CAP_URL_503,
                       FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL));
        int num_refs_start = o->getNumRefs();
        {
            const int NUM_RETRIES = 5;
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD,NUM_RETRIES);

            // This should generate a retry
            mdc->fetchMedia(o);

            // Make sure no posts happened yet...
            CHECK_MESSAGE(gPostRecords->size(, "post records before"), 0);

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

TEST_CASE_FIXTURE(mediadataclient, "test_6")
{

        //
        // Test navigate() with a bounce back
        //
        LOG_TEST(6);

        LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(
            _DATA_URLS(VALID_OBJECT_ID,
                       "1.0","true",
                       FAKE_OBJECT_MEDIA_CAP_URL,
                       FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR));
        {
            LLPointer<LLObjectMediaNavigateClient> mdc = new LLObjectMediaNavigateClient(NO_PERIOD,NO_PERIOD);
            const char *TEST_URL = "http://example.com";
            mdc->navigate(o, 0, TEST_URL);
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);
            ::pump_timers();

            // ensure bounce back
            CHECK_MESSAGE(dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o, "bounce back"))->getNumBounceBacks(),
                   1);

            CHECK_MESSAGE(gPostRecords->size(, "post records"), 1);
            CHECK_MESSAGE((*gPostRecords, "post url")[0]["url"], FAKE_OBJECT_MEDIA_NAVIGATE_CAP_URL_ERROR);
            CHECK_MESSAGE((*gPostRecords, "post object id")[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID));
            CHECK_MESSAGE((*gPostRecords, "post data")[0]["body"][LLTextureEntry::TEXTURE_INDEX_KEY], 0);
            CHECK_MESSAGE((*gPostRecords, "post data")[0]["body"][LLMediaEntry::CURRENT_URL_KEY], TEST_URL);
            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_7")
{

        // Test LLMediaDataClient::isInQueue()
        LOG_TEST(7);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(
            _DATA(VALID_OBJECT_ID_1,"3.0","true"));
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(
            _DATA(VALID_OBJECT_ID_2,"1.0","true"));
        int num_refs_start = o1->getNumRefs();
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);

            CHECK_MESSAGE(! mdc->isInQueue(o1, "not in queue yet 1"));
            CHECK_MESSAGE(! mdc->isInQueue(o2, "not in queue yet 2"));

            mdc->fetchMedia(o1);

            CHECK_MESSAGE(mdc->isInQueue(o1, "is in queue"));
            CHECK_MESSAGE(! mdc->isInQueue(o2, "is not in queue"));

            ::pump_timers();

            CHECK_MESSAGE(! mdc->isInQueue(o1, "not in queue anymore"));
            CHECK_MESSAGE(! mdc->isInQueue(o2, "still is not in queue"));

            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_8")
{

        // Test queue handling of objects that are marked dead.
        LOG_TEST(8);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"4.0","true"));
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"3.0","true"));
        LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"2.0","true"));
        LLMediaDataClientObject::ptr_t o4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"1.0","true"));
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);

            // queue up all 4 objects
            mdc->fetchMedia(o1);
            mdc->fetchMedia(o2);
            mdc->fetchMedia(o3);
            mdc->fetchMedia(o4);

            CHECK_MESSAGE(mdc->isInQueue(o1, "is in queue 1"));
            CHECK_MESSAGE(mdc->isInQueue(o2, "is in queue 2"));
            CHECK_MESSAGE(mdc->isInQueue(o3, "is in queue 3"));
            CHECK_MESSAGE(mdc->isInQueue(o4, "is in queue 4"));
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);

            // and mark the second and fourth ones dead.  Call removeFromQueue when marking dead, since this is what LLVOVolume will do.
            dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o2))->markDead();
            mdc->removeFromQueue(o2);
            dynamic_cast<LLMediaDataClientObjectTest*>(static_cast<LLMediaDataClientObject*>(o4))->markDead();
            mdc->removeFromQueue(o4);

            // The removeFromQueue calls should remove the second and fourth ones
            CHECK_MESSAGE(mdc->isInQueue(o1, "is in queue 1"));
            CHECK_MESSAGE(!mdc->isInQueue(o2, "is not in queue 2"));
            CHECK_MESSAGE(mdc->isInQueue(o3, "is in queue 3"));
            CHECK_MESSAGE(!mdc->isInQueue(o4, "is not in queue 4"));
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 0);

            ::pump_timers();

            // The first tick should process the first item
            CHECK_MESSAGE(!mdc->isInQueue(o1, "is not in queue 1"));
            CHECK_MESSAGE(!mdc->isInQueue(o2, "is not in queue 2"));
            CHECK_MESSAGE(mdc->isInQueue(o3, "is in queue 3"));
            CHECK_MESSAGE(!mdc->isInQueue(o4, "is not in queue 4"));
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 1);

            ::pump_timers();

            // The second tick should process the third, emptying the queue
            CHECK_MESSAGE(!mdc->isInQueue(o3, "is not in queue 3"));
            CHECK_MESSAGE(gPostRecords->size(, "post records"), 2);

            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_9")
{

        //
        // Test queue re-ordering
        //
        LOG_TEST(9);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"40.0","true"));
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"30.0","true"));
        LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"20.0","true"));
        LLMediaDataClientObjectTest *object4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"10.0","true"));
        LLMediaDataClientObject::ptr_t o4 = object4;
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);

            // queue up all 4 objects.  They should now be in the queue in
            // order 1 through 4, with 4 being at the front of the queue
            mdc->fetchMedia(o1);
            mdc->fetchMedia(o2);
            mdc->fetchMedia(o3);
            mdc->fetchMedia(o4);

            int tick_num = 0;

            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 0);

            ::pump_timers();
            ++tick_num;

            // The first tick should remove the first one
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 1);

            // Now, pretend that object 4 moved relative to the avatar such
            // that it is now closest
            object4->setMediaInterest(50.0);

            ::pump_timers();
            ++tick_num;

            // The second tick should still pick off item 2, but then re-sort
            // have picked off object 4
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 2);

            ::pump_timers();
            ++tick_num;

            // The third tick should pick off object 2
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 3);

            // The fourth tick should pick off object 3
            ::pump_timers();
            ++tick_num;

            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 4);

            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_10")
{

        //
        // Test using the "round-robin" queue
        //
        LOG_TEST(10);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"1.0","true"));
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"2.0","true"));
        LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"3.0","false"));
        LLMediaDataClientObject::ptr_t o4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"4.0","false"));
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);

            // queue up all 4 objects.  The first two should be in the sorted
            // queue [2 1], the second in the round-robin queue.  The queues
            // are serviced interleaved, so we should expect:
            // 2, 3, 1, 4
            mdc->fetchMedia(o1);
            mdc->fetchMedia(o2);
            mdc->fetchMedia(o3);
            mdc->fetchMedia(o4);

            int tick_num = 0;

            // 0
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 0);

            ::pump_timers();
            ++tick_num;

            // 1 The first tick should remove object 2
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 1);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_2));

            ::pump_timers();
            ++tick_num;

            // 2 The second tick should send object 3
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 2);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[1]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_3));

            ::pump_timers();
            ++tick_num;

            // 3 The third tick should remove object 1
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 3);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[2]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_1));

            ::pump_timers();
            ++tick_num;

            // 4 The fourth tick should send object 4
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 4);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[3]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_4));

            ::pump_timers();
            ++tick_num;

            // 5 The fifth tick should not change the state of anything.
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is not in queue 2", !mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 4);

            ::pump_timers();

            // Whew....better be empty
            CHECK_MESSAGE(mdc->isEmpty(, "queue empty"));
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_11")
{

        //
        // Test LLMediaDataClient's destructor
        //
        LOG_TEST(11);

        LLMediaDataClientObject::ptr_t o = new LLMediaDataClientObjectTest(DATA);
        int num_refs_start = o->getNumRefs();
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);
            mdc->fetchMedia(o);
            // must tick enough times to clear refcount of mdc
            ::pump_timers();
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_12")
{

        //
        // Test the "not interesting enough" call
        //
        LOG_TEST(12);

        LLMediaDataClientObjectTest *object1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"1.0","true"));
        LLMediaDataClientObject::ptr_t o1 = object1;
        LLMediaDataClientObject::ptr_t o2 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_2,"2.0","true"));
        LLMediaDataClientObject::ptr_t o3 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_3,"3.0","true"));
        LLMediaDataClientObject::ptr_t o4 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_4,"4.0","true"));
        {
            LLPointer<LLObjectMediaDataClient> mdc = new LLObjectMediaDataClient(NO_PERIOD,NO_PERIOD);

            // queue up all 4 objects.  The first two are "interesting enough".
            // Firing the timer 4 times should therefore leave them.
            // Note that they should be sorted 4,3,2,1
            // Then, we'll make one "interesting enough", fire the timer a few
            // times, and make sure only it gets pulled off the queue
            gMinimumInterestLevel = 2.5;
            mdc->fetchMedia(o1);
            mdc->fetchMedia(o2);
            mdc->fetchMedia(o3);
            mdc->fetchMedia(o4);

            int tick_num = 0;

            // 0
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is in queue 4", mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 0);

            ::pump_timers();
            ++tick_num;

            // 1 The first tick should remove object 4
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is in queue 3", mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 1);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[0]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_4));

            ::pump_timers();
            ++tick_num;

            // 2 The second tick should send object 3
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 2);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[1]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_3));

            ::pump_timers();
            ++tick_num;

            // 3 The third tick should not pull off anything
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 2);

            ::pump_timers();
            ++tick_num;

            // 4 The fourth tick (for good measure) should not pull off anything
            ensure(STR(tick_num) + ". is in queue 1", mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 2);

            // Okay, now futz with object 1's interest, such that it is now
            // "interesting enough"
            object1->setMediaInterest((F64)5.0);

            // This should sort so that the queue is now [1 2]
            ::pump_timers();
            ++tick_num;

            // 5 The fifth tick should now identify objects 3 and 4 as no longer
            // needing "updating", and remove them from the queue
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 3);
            ensure(STR(tick_num) + ". post object id", (*gPostRecords)[2]["body"][LLTextureEntry::OBJECT_ID_KEY].asUUID(), LLUUID(VALID_OBJECT_ID_1));

            ::pump_timers();
            ++tick_num;

            // 6 The sixth tick should not pull off anything
            ensure(STR(tick_num) + ". is not in queue 1", !mdc->isInQueue(o1));
            ensure(STR(tick_num) + ". is in queue 2", mdc->isInQueue(o2));
            ensure(STR(tick_num) + ". is not in queue 3", !mdc->isInQueue(o3));
            ensure(STR(tick_num) + ". is not in queue 4", !mdc->isInQueue(o4));
            ensure(STR(tick_num) + ". post records", gPostRecords->size(), 3);

            ::pump_timers();
            ++tick_num;

            // Whew....better NOT be empty ... o2 should still be there
            CHECK_MESSAGE(!mdc->isEmpty(, "queue not empty"));

            // But, we need to clear the queue, or else we won't destroy MDC...
            // this is a strange interplay between the queue timer and the MDC
            mdc->removeFromQueue(o2);
            // tick
            ::pump_timers();
        
}

TEST_CASE_FIXTURE(mediadataclient, "test_13")
{

        //
        // Test supression of redundant navigates.
        //
        LOG_TEST(13);

        LLMediaDataClientObject::ptr_t o1 = new LLMediaDataClientObjectTest(_DATA(VALID_OBJECT_ID_1,"1.0","true"));
        {
            LLPointer<LLObjectMediaNavigateClient> mdc = new LLObjectMediaNavigateClient(NO_PERIOD,NO_PERIOD);
            const char *TEST_URL = "http://foo.example.com";
            const char *TEST_URL_2 = "http://example.com";
            mdc->navigate(o1, 0, TEST_URL);
            mdc->navigate(o1, 1, TEST_URL);
            mdc->navigate(o1, 0, TEST_URL_2);
            mdc->navigate(o1, 1, TEST_URL_2);

            // This should add two requests to the queue, one for face 0 of the object and one for face 1.

            CHECK_MESSAGE(mdc->isInQueue(o1, "before pump: 1 is in queue"));

            ::pump_timers();

            CHECK_MESSAGE(mdc->isInQueue(o1, "after first pump: 1 is in queue"));

            ::pump_timers();

            CHECK_MESSAGE(!mdc->isInQueue(o1, "after second pump: 1 is not in queue"));

            CHECK_MESSAGE((*gPostRecords, "first post has correct url")[0]["body"][LLMediaEntry::CURRENT_URL_KEY].asString(), std::string(TEST_URL_2));
            CHECK_MESSAGE((*gPostRecords, "second post has correct url")[1]["body"][LLMediaEntry::CURRENT_URL_KEY].asString(), std::string(TEST_URL_2));

        
}

} // TEST_SUITE

