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
#include "../test/lldoctest.h"

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

TEST_SUITE("LLImageDecodeThread") {

struct imagedecodethread_test
{

        // Instance to be tested
        LLImageDecodeThread* mThread;
        // Constructor and destructor of the test wrapper
        imagedecodethread_test()
        {
            mThread = NULL;
        
};

TEST_CASE_FIXTURE(imagedecodethread_test, "test_1")
{

        // Test a *threaded* instance of the class
        mThread = new LLImageDecodeThread(true);
        CHECK_MESSAGE(mThread != NULL, "LLImageDecodeThread: threaded constructor failed");
        // Insert something in the queue
        bool done = false;
        LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, 0, false, new responder_test(&done));
        // Verifies we get back a valid handle
        CHECK_MESSAGE(decodeHandle != 0, "LLImageDecodeThread:  threaded decodeImage(), returned handle is null");
        // Wait till the thread has time to handle the work order (though it doesn't do much per work order...)
        const U32 INCREMENT_TIME = 500;             // 500 milliseconds
        const U32 MAX_TIME = 20 * INCREMENT_TIME;   // Do the loop 20 times max, i.e. wait 10 seconds but no more
        U32 total_time = 0;
        while ((done == false) && (total_time < MAX_TIME))
        {
            ms_sleep(INCREMENT_TIME);
            total_time += INCREMENT_TIME;
        
}

} // TEST_SUITE

