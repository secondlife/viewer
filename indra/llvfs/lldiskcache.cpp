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
    mRequestId(0)
{
    // Create the worker thread and worker function
    mWorkerThread = std::thread([this]()
    {
        requestThread();
    });

    // Run the update function periodically. If the period is 0 (seconds),
    // then the update function is called every frame. We don't want
    // to saturate the main loop so keep above 0 for now but consider
    // reducing once the code is more complete.
    const F32 period = 0.05f;
    ticker.reset(LLEventTimer::run_every(period, [this]()
    {
        perTick();
    }));
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
    mInQueue.close();

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
    // We used to use while (!mInQueue.isClosed()) test for this loop
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
            // We might consider using LLThreadSafeQueue::tryPopBack() to
            // inspect the queue first before we try to call popBack();
            // This might be slightly more efficient but probably not significant
            auto item = mInQueue.popBack();

            // This is where the real work happens. The callable we pulled off
            // of the input queue is executed here and the result captured.
            auto res = item();

            // Push the result out to the outbound results queue
            mOutQueue.pushFront(res);
        }
    }
    catch (const LLThreadSafeQueueInterrupt&)
    {
    }

    // Calling close() here indicates that we are finished with the output queue
    // the it can be closed. See the note about potentially losing the last few
    // items in the queue under some circumstances elsewhere in this file
    mOutQueue.close();
}

///////////////////////////////////////////////////////////////////////////////
//
void llDiskCache::perTick()
{
    result res;
    while (mOutQueue.tryPopBack((res)))
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
            found->second.cb(found->second.cbd, res.payload, res.ok);

            // we have processed this entry in the queue so delete it
            mRequestMap.erase(found);
        }
        else
        {
            // TODO: This should not be possible but we should consider it anyway
            // and decide whether to do nothing, assert or something else
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// TODO: This is test code that (kind of) adds a read request. Next is we must
// decide whether to have a single add_request function and pass in the type
// such as READ, WRITE, APPEND etc. or have separate functions for each..
void llDiskCache::addReadRequest(std::string filename,
                                 vfs_callback_t cb,
                                 vfs_callback_data_t cbd)
{
    // This is an ID we pass in to our worker function so we can match up
    // requests and results by comparing the id.
    mRequestId++;

    // record the ID in a map - used to compare against results queue in update
    // (per tick) function
    mRequestMap.emplace(mRequestId, request{ cb, cbd });

    // In the future, we should consider if the code that runs on the
    // request thread here can throw an exception - this needs to be accounted
    // and @Nat has a sugestion around using std::packaged_task(..). We do not
    // need it for this use case - we assert this code will not throw but
    // other, more complex code in the future might
    mInQueue.pushFront([this, filename]()
    {
        // TODO: This is a place holder for doing some work on the worker
        // thread - eventually, for this use case, it will be used to
        // read and write cache files and update their meta data
        std::cout << "READ running on worker thread and reading from " << filename << std::endl;
        bool success = true;
        U32 filesize = 12;

        // This is an interesting idiom. We will be passing back the contents of files
        // we read and an std::vector<U8> is suitable for that. However, that means
        // we will typically be moving or copying it around and that is inefficient.
        // Adding a smart (shared) pointer into the mix means we can pass that around
        // and the consumer can either hang onto it (its ref count is incremented) or
        // just look and ignore it - when no one has a reference to it anymore, the
        // C++ mechanics will automatically delete it.
        shared_payload_t file_contents = std::make_shared<std::vector<U8>>(filesize);

        // TODO: more place holder code
        memset(file_contents->data(), 'A', file_contents->size());

        // We pass back the ID (for lookup), the contents of the file we read
        // (in our use case) and a flag indicating success/failure. We might
        // also consider using file_contents and comparing with ! vs a
        // separate bool. This is okay for now though.
        return result{ mRequestId, file_contents, success };
    });
}

///////////////////////////////////////////////////////////////////////////////
// TODO: This is test code that (kind of) adds a write request.
void llDiskCache::addWriteRequest(std::string filename,
                                  shared_payload_t buffer,
                                  vfs_callback_t cb,
                                  vfs_callback_data_t cbd)
{
    // This is an ID we pass in to our worker function so we can match up
    // requests and results by comparing the id.
    mRequestId++;

    // record the ID in a map - used to compare against results queue in update
    // (per tick) function
    mRequestMap.emplace(mRequestId, request{ cb, cbd });

    // In the future, we should consider if the code that runs on the
    // request thread here can throw an exception - this needs to be accounted
    // and @Nat has a sugestion around using std::packaged_task(..). We do not
    // need it for this use case - we assert this code will not throw but
    // other, more complex code in the future might
    mInQueue.pushFront([this, filename, buffer]()
    {
        std::cout << "WRITE running on worker thread and writing buffer to " << filename << std::endl;

        LL_INFOS() << "Buffer to write to disk is of size " << buffer->size() << " and contains " << LL_ENDL;
        for (auto p = 0; p < buffer->size(); ++p)
        {
            std::cout << buffer->data()[p];
        }
        LL_INFOS() << "" << LL_ENDL;


        // we don't send anything back when we are writing - result goes back as a bool
        shared_payload_t file_contents = nullptr;

        bool success = true;

        // We pass back the ID (for lookup), the contents of the file we read
        // (in our use case) and a flag indicating success/failure. We might
        // also consider using file_contents and comparing with ! vs a
        // separate bool. This is okay for now though.
        return result{ mRequestId, file_contents, success };
    });
}
