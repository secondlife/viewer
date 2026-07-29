/**
 * @file llimageworker_test.cpp
 * @author Merov Linden
 * @date 2009-04-28
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

// Precompiled header: almost always required for newview cpp files
#include "linden_common.h"
// Class to test
#include "../llimageworker.h"
// For timer class
#include "../llcommon/lltimer.h"
// for lltrace class
#include "../llcommon/lltrace.h"
// Tut header
#include "../test/lltut.h"

// -------------------------------------------------------------------------------------------
// Stubbing: Declarations required to link and run the class being tested
// Notes:
// * Add here stubbed implementation of the few classes and methods used in the class to be tested
// * Add as little as possible (let the link errors guide you)
// * Do not make any assumption as to how those classes or methods work (i.e. don't copy/paste code)
// * A simulator for a class can be implemented here. Please comment and document thoroughly.

LLImageBase::LLImageBase()
: mData(NULL),
mDataSize(0),
mWidth(0),
mHeight(0),
mComponents(0),
mBadBufferAllocation(false),
mAllowOverSize(false)
{
}
LLImageBase::~LLImageBase() {}
void LLImageBase::dump() { }
void LLImageBase::sanityCheck() { }
void LLImageBase::deleteData() { }
U8* LLImageBase::allocateData(S32 size) { return NULL; }
U8* LLImageBase::reallocateData(S32 size) { return NULL; }

LLImageRaw::LLImageRaw(U16 width, U16 height, S8 components) { }
LLImageRaw::~LLImageRaw() { }
void LLImageRaw::deleteData() { }
U8* LLImageRaw::allocateData(S32 size) { return NULL; }
U8* LLImageRaw::reallocateData(S32 size) { return NULL; }
const U8* LLImageBase::getData() const { return NULL; }
U8* LLImageBase::getData() { return NULL; }
const std::string& LLImage::getLastThreadError() { static std::string msg; return msg; }

// End Stubbing
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------

namespace tut
{
    // Test wrapper declarations

    // Note: We derive the responder class for 2 reasons:
    // 1. It's a pure virtual class and we can't compile without completed() being implemented
    // 2. We actually need a responder to test that the thread work test completed
    // We implement this making no assumption on what's done in the thread or worker
    // though, just that the responder's completed() method is called in the end.
    // Note on responders: responders are ref counted and *will* be deleted by the request they are
    // attached to when the queued request is deleted. The recommended way of using them is to
    // create them when creating a request, put a callback method in completed() and not rely on
    // anything to survive in the responder object once completed() has been called. Let the request
    // do the deletion and clean up itself.
    class responder_test : public LLImageDecodeThread::Responder
    {
        public:
            responder_test(bool* res)
            {
                done = res;
                *done = false;
            }
            virtual void completed(bool success, const std::string& error_message, LLImageRaw* raw, LLImageRaw* aux, U32 request_id)
            {
                *done = true;
            }
        private:
            // This is what can be thought of as the minimal implementation of a responder
            // Done will be switched to true when completed() is called and can be tested
            // outside the responder. A better way of doing this is to store a callback here.
            bool* done;
    };

    // Test wrapper declaration : decode thread
    struct imagedecodethread_test
    {
        // Instance to be tested
        LLImageDecodeThread* mThread;
        // Constructor and destructor of the test wrapper
        imagedecodethread_test()
        {
            mThread = NULL;
        }
        ~imagedecodethread_test()
        {
            delete mThread;
        }
    };

    // Tut templating thingamagic: test group, object and test instance
    typedef test_group<imagedecodethread_test> imagedecodethread_t;
    typedef imagedecodethread_t::object imagedecodethread_object_t;
    tut::imagedecodethread_t tut_imagedecodethread("LLImageDecodeThread");

    // ---------------------------------------------------------------------------------------
    // Test functions
    // Notes:
    // * Test as many as you possibly can without requiring a full blown simulation of everything
    // * The tests are executed in sequence so the test instance state may change between calls
    // * Remember that you cannot test private methods with tut
    // ---------------------------------------------------------------------------------------

    // ---------------------------------------------------------------------------------------
    // Test the LLImageDecodeThread interface
    // ---------------------------------------------------------------------------------------

    template<> template<>
    void imagedecodethread_object_t::test<1>()
    {
        // Test a *threaded* instance of the class
        mThread = new LLImageDecodeThread(true);
        ensure("LLImageDecodeThread: threaded constructor failed", mThread != NULL);
        // Insert something in the queue
        bool done = false;
        LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, 0, false, new responder_test(&done));
        // Verifies we get back a valid handle
        ensure("LLImageDecodeThread:  threaded decodeImage(), returned handle is null", decodeHandle != 0);
        // Wait till the thread has time to handle the work order (though it doesn't do much per work order...)
        const U32 INCREMENT_TIME = 500;             // 500 milliseconds
        const U32 MAX_TIME = 20 * INCREMENT_TIME;   // Do the loop 20 times max, i.e. wait 10 seconds but no more
        U32 total_time = 0;
        while ((done == false) && (total_time < MAX_TIME))
        {
            ms_sleep(INCREMENT_TIME);
            total_time += INCREMENT_TIME;
        }
        // Verifies that the responder has now been called
        ensure("LLImageDecodeThread: threaded work unit not processed", done == true);
    }
}
