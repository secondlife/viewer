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
			virtual void completed(bool success, LLImageRaw* raw, LLImageRaw* aux)
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

	// Test wrapper declaration : image worker
	// Note: this class is not meant to be instantiated outside an LLImageDecodeThread instance
	// but it's not a bad idea to get its public API a good shake as part of a thorough unit test set.
	// Some gotcha with the destructor though (see below).
	struct imagerequest_test
	{
		// Instance to be tested
		LLImageDecodeThread::ImageRequest* mRequest;
		bool done;

		// Constructor and destructor of the test wrapper
		imagerequest_test()
		{
			done = false;
			mRequest = new LLImageDecodeThread::ImageRequest(0, 0,
											 LLQueuedThread::PRIORITY_NORMAL, 0, FALSE,
											 new responder_test(&done));
		}
		~imagerequest_test()
		{
			// We should delete the object *but*, because its destructor is protected, that cannot be
			// done from outside an LLImageDecodeThread instance... So we leak memory here... It's fine...
			//delete mRequest;
		}
	};

	// Tut templating thingamagic: test group, object and test instance
	typedef test_group<imagedecodethread_test> imagedecodethread_t;
	typedef imagedecodethread_t::object imagedecodethread_object_t;
	tut::imagedecodethread_t tut_imagedecodethread("LLImageDecodeThread");

	typedef test_group<imagerequest_test> imagerequest_t;
	typedef imagerequest_t::object imagerequest_object_t;
	tut::imagerequest_t tut_imagerequest("LLImageRequest");

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
	//
	// Note on Unit Testing Queued Thread Classes
	//
	// Since methods on such a class are called on a separate loop and that we can't insert tut
	// ensure() calls in there, we exercise the class with 2 sets of tests:
	// - 1: Test as a single threaded instance: We declare the class but ask for no thread
	//   to be spawned (easy with LLThreads since there's a boolean argument on the constructor
	//   just for that). We can then unit test each public method like we do on a normal class.
	// - 2: Test as a threaded instance: We let the thread launch and check that its external 
	//   behavior is as expected (i.e. it runs, can accept a work order and processes
	//   it). Typically though there's no guarantee that this exercises all the methods of the
	//   class which is why we also need the previous "non threaded" set of unit tests for
	//   complete coverage.
	//
	// ---------------------------------------------------------------------------------------

	template<> template<>
	void imagedecodethread_object_t::test<1>()
	{
		// Test a *non threaded* instance of the class
		mThread = new LLImageDecodeThread(false);
		ensure("LLImageDecodeThread: non threaded constructor failed", mThread != NULL);
		// Test that we start with an empty list right at creation
		ensure("LLImageDecodeThread: non threaded init state incorrect", mThread->tut_size() == 0);
		// Insert something in the queue
		bool done = false;
		LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, LLQueuedThread::PRIORITY_NORMAL, 0, FALSE, new responder_test(&done));
		// Verifies we got a valid handle
		ensure("LLImageDecodeThread: non threaded decodeImage(), returned handle is null", decodeHandle != 0);
		// Verifies that we do now have something in the queued list
		ensure("LLImageDecodeThread: non threaded decodeImage() insertion in threaded list failed", mThread->tut_size() == 1);
		// Trigger queue handling "manually" (on a threaded instance, this is done on the thread loop)
		S32 res = mThread->update(0);
		// Verifies that we successfully handled the list
		ensure("LLImageDecodeThread: non threaded update() list handling test failed", res == 0);
		// Verifies that the list is now empty
		ensure("LLImageDecodeThread: non threaded update() list emptying test failed", mThread->tut_size() == 0);
	}

	template<> template<>
	void imagedecodethread_object_t::test<2>()
	{
		// Test a *threaded* instance of the class
		mThread = new LLImageDecodeThread(true);
		ensure("LLImageDecodeThread: threaded constructor failed", mThread != NULL);
		// Test that we start with an empty list right at creation
		ensure("LLImageDecodeThread: threaded init state incorrect", mThread->tut_size() == 0);
		// Insert something in the queue
		bool done = false;
		LLImageDecodeThread::handle_t decodeHandle = mThread->decodeImage(NULL, LLQueuedThread::PRIORITY_NORMAL, 0, FALSE, new responder_test(&done));
		// Verifies we get back a valid handle
		ensure("LLImageDecodeThread:  threaded decodeImage(), returned handle is null", decodeHandle != 0);
		// Wait a little so to simulate the main thread doing something on its main loop...
		ms_sleep(500);		// 500 milliseconds
		// Verifies that the responder has *not* been called yet in the meantime
		ensure("LLImageDecodeThread: responder creation failed", done == false);
		// Ask the thread to update: that means tells the queue to check itself and creates work requests
		mThread->update(1);
		// Wait till the thread has time to handle the work order (though it doesn't do much per work order...)
		const U32 INCREMENT_TIME = 500;				// 500 milliseconds
		const U32 MAX_TIME = 20 * INCREMENT_TIME;	// Do the loop 20 times max, i.e. wait 10 seconds but no more
		U32 total_time = 0;
		while ((done == false) && (total_time < MAX_TIME))
		{
			ms_sleep(INCREMENT_TIME);
			total_time += INCREMENT_TIME;
		}
		// Verifies that the responder has now been called
		ensure("LLImageDecodeThread: threaded work unit not processed", done == true);
	}

	// ---------------------------------------------------------------------------------------
	// Test the LLImageDecodeThread::ImageRequest interface
	// ---------------------------------------------------------------------------------------
	
	template<> template<>
	void imagerequest_object_t::test<1>()
	{
		// Test that we start with a correct request at creation
		ensure("LLImageDecodeThread::ImageRequest::ImageRequest() constructor test failed", mRequest->tut_isOK());
		bool res = mRequest->processRequest();
		// Verifies that we processed the request successfully
		ensure("LLImageDecodeThread::ImageRequest::processRequest() processing request test failed", res == true);
		// Check that we can call the finishing call safely
		try {
			mRequest->finishRequest(false);
		} catch (...) {
			fail("LLImageDecodeThread::ImageRequest::finishRequest() test failed");
		}
	}
}
