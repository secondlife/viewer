/**
 * @file lldiskcache.cpp
 * @brief Cache items by reading/writing them to / from
 *        disk using a worker thread.
 *
 * There are 2 interesting components to this class:
 * 1/ The work (reading/writing) from disk happens in its
 *    own thread to avoid stalling the main loop. To do
 *    some work on this thread, you construct a request with
 *    the appropriate parameters and add it to the input queue
 *    which is implemented using LLThreadSafeQuue. At some point
 *    later, the result (id, payload, result code) appears on a
 *    second queue (also implemented as an LLThreadSafeQueue).
 *    The advantage of this approach is that so long as the
 *    LLThreadSafeQueue works correctly, there is no locking/mutex
 *    control needed as the queues behave like thread boundaries.
 *    Similarly, since all the file access is done sequentially
 *    in a single thread, there is no file level locking required.
 *    There may be a small performance increase to be gained
 *    by implementing N queues and the LLThreadSafe code would take
 *    care of managing the callable functions. However, since you
 *    would have to take account of the possibility of reading and/or
 *    writing the same file (it's a cache so that's a possibility)
 *    from multiple threads, the code complexity would rise
 *    dramatically. The assertion is that this code will be plenty
 *    fast enough and is very straightforward.
 * 2/ The caching mechanism... TODO: describe cache here...
 *
 *
 *
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#include "lldiskcache.h"

///////////////////////////////////////////////////////////////////////////////
//
llDiskCache::llDiskCache() :
    LLEventTimer(0.05)
{
    // Create the worker thread and worker function
    mWorkerThread = std::thread([this]()
    {
        requestThread();
    });

    // start the timer that will drive the request queue
    // (LLEventTimer initialized above defines the time in
    // seconds between each tick or set to 0.0 for every frame)
    mEventTimer.start();
}

///////////////////////////////////////////////////////////////////////////////
// Overrides the LLSingleton class - used to clean up after ourselves
// here vs making use of the regular C++ destructor mechanism
void llDiskCache::cleanupSingleton()
{
    // We do cleanup here vs the destructor based on recommendations found
    // in the llsingleton.h comments: Any cleanup logic that might take
    // significant real time -- or throw an exception -- must not be placed
    // in your LLSingleton's destructor, but rather in its cleanupSingleton()
    // method, which is called implicitly by deleteSingleton().

    // we won't be putting anything else onto the outbound request queue
    // so we close it to indicate to the worker function that we are finished
    mITaskQueue.close();

    // Some idioms have a test here for std::thread::joinable() before the
    // call to join() is made but we don't need it here. There are a very
    // narrow set of circumstances where it won't be joinable and none of
    // them apply here so we can safely just call join() and have the thread
    // come back to the main loop
    mWorkerThread.join();

    // Note: there is a possibility that there may be remaining items in
    // this queue that will not be acted up - thrown away in fact. Given
    // the nature of this caching code and the fact we are shutting down
    // here, this is okay - it would introduce more complexity than we want
    // to take on to ensure than this wasn't the case
}

///////////////////////////////////////////////////////////////////////////////
// Process requests on the worker thread from the input queue and push
// the result out to the output queue
void llDiskCache::requestThread()
{
    // We used to use while (!mITaskQueue.isClosed()) test for this loop
    // but there is an issue with LLThreadSafeQueue whereby the queue
    // has not yet been closed so isClosed() returns false. After we
    // make the call to popBack() the queue does get closed and the
    // next time through it throws an LLThreadSafeQueueInterrupt
    // exception. The solution for the moment is to catch the exception
    // and do nothing with it.
    try
    {
        while (true)
        {

            // grab a request off the queue to process
            auto request = mITaskQueue.popBack();

            // This is where the real work happens. The callable we pulled off
            // of the input queue is executed here and the result captured.
            auto result = request();

            // Push the result out to the outbound results queue
            mResultQueue.pushFront(result);
        }
    }
    catch (const LLThreadSafeQueueInterrupt&)
    {
    }

    // Calling close() here indicates that we are finished with the output queue
    // the it can be closed. See the note about potentially losing the last few
    // items in the queue under some circumstances elsewhere in this file
    mResultQueue.close();
}

///////////////////////////////////////////////////////////////////////////////
//
BOOL llDiskCache::tick()
{
    mResult res;
    while (mResultQueue.tryPopBack((res)))
    {
        // Here will pull all outstanding results off of the output
        // queue and call their callbacks to that the consumer can
        // retrieve the result of the operation they requested.
        // Depending on how this code evolves, we might consider
        // adding a throttle here so that the full contents of the
        // queue is not read each time. Instead, we might only
        // taken N items off, or taken items for M milliseconds
        // before aborting for this update pass and picking up
        // where we left off next time through

        // TODO: some test code to display the result of executing
        // the callable before we pass it back to the consumer via
        // the callback mechanism.
        std::cout << "Working: thread returned " << res.ok;
        std::cout << " with id = " << res.id << std::endl;

        // Note: no need to lock the map because it's only accessed
        // on the main thread - one of the benefits of this idiom
        auto found = mRequestMap.find(res.id);
        if (found != mRequestMap.end())
        {
            // execute the callback and pass the payload/result status
            // back to the consumer
            found->second.cb(res.payload, res.ok);

            // we have processed this entry in the queue so delete it
            mRequestMap.erase(found);
        }
        else
        {
            // TODO: This should not be possible but we should consider it anyway
            // and decide whether to do nothing, assert or something else
        }
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// TODO: This is test code that (kind of) adds a read request. Next is we must
// decide whether to have a single add_request function and pass in the type
// such as READ, WRITE, APPEND etc. or have separate functions for each..
void llDiskCache::addReadRequest(std::string filename,
                                 request_callback_t cb)
{
    // This is an ID we pass in to our worker function so we can match up
    // requests and results by comparing the id.
    auto request_id = mRequestId++;

    // record the ID in a map - used to compare against results queue in update
    // (per tick) function
    mRequestMap.emplace(request_id, mRequest{ cb });

    // In the future, we should consider if the code that runs on the
    // request thread here can throw an exception - this needs to be accounted
    // and @Nat has a sugestion around using std::packaged_task(..). We do not
    // need it for this use case - we assert this code will not throw but
    // other, more complex code in the future might
    mITaskQueue.pushFront([request_id, filename]()
    {
        // TODO: This is a place holder for doing some work on the worker
        // thread - eventually, for this use case, it will be used to
        // read and write cache files and update their meta data
        std::cout << "READ running on worker thread and reading from " << filename << std::endl;
        bool success = true;

        // This is an interesting idiom. We will be passing back the contents of files
        // we read and an std::vector<U8> is suitable for that. However, that means
        // we will typically be moving or copying it around and that is inefficient.
        // Adding a smart (shared) pointer into the mix means we can pass that around
        // and the consumer can either hang onto it (its ref count is incremented) or
        // just look and ignore it - when no one has a reference to it anymore, the
        // C++ mechanics will automatically delete it.
        request_payload_t file_contents = nullptr;

        std::ifstream ifs(filename);
        if (!ifs.is_open())
        {
            success = false;
        }
        else
        {
            std::string contents;
            if (std::getline(ifs, contents))
            {
                ifs.close();

                const U32 filesize = contents.length();
                file_contents = std::make_shared<std::vector<U8>>(filesize);
                memcpy(file_contents->data(), contents.c_str(), file_contents->size());
                success = true;
            }
        }

        // We pass back the ID (for lookup), the contents of the file we read
        // (in our use case) and a flag indicating success/failure. We might
        // also consider using file_contents and comparing with ! vs a
        // separate bool. This is okay for now though.
        return mResult{ request_id, file_contents, success };
    });
}

///////////////////////////////////////////////////////////////////////////////
// TODO: This is test code that (kind of) adds a write request.
void llDiskCache::addWriteRequest(std::string filename,
                                  request_payload_t buffer,
                                  request_callback_t cb)
{
    // This is an ID we pass in to our worker function so we can match up
    // requests and results by comparing the id.
    auto request_id = mRequestId++;

    // record the ID in a map - used to compare against results queue in update
    // (per tick) function
    mRequestMap.emplace(request_id, mRequest{ cb });

    // In the future, we should consider if the code that runs on the
    // request thread here can throw an exception - this needs to be accounted
    // and @Nat has a sugestion around using std::packaged_task(..). We do not
    // need it for this use case - we assert this code will not throw but
    // other, more complex code in the future might
    mITaskQueue.pushFront([request_id, filename, buffer]()
    {
        std::cout << "WRITE running on worker thread and writing buffer to " << filename << std::endl;

        bool success;
        std::ofstream ofs(filename);
        if (ofs)
        {
            ofs << buffer->data();
            ofs.close();
            success = true;
        }
        else
        {
            success = false;
        }

        // we don't send anything back when we are writing - task result goes back as a bool
        request_payload_t file_contents = nullptr;

        // We pass back the ID (for lookup), the contents of the file we read
        // (in our use case) and a flag indicating success/failure. We might
        // also consider using file_contents and comparing with ! vs a
        // separate bool. This is okay for now though.
        return mResult{ request_id, file_contents, success };
    });
}
