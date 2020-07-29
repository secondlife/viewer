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
llDiskCache::llDiskCache()
{
    mWorkerThread = std::thread([this]()
    {
        requestThread();
    });

    // TODO: reduce this,p perhaps as far as 0.0 (every tick)
    const F32 period = 0.05f;
    ticker.reset(LLEventTimer::run_every(period, [this]()
    {
        perTick();
    }));
}

///////////////////////////////////////////////////////////////////////////////
//
llDiskCache::~llDiskCache()
{
}

///////////////////////////////////////////////////////////////////////////////
//
void llDiskCache::cleanupSingleton()
{
    // TODO: comment why cleanup is here and not in dtor
    // see comments in llsingleton.h


    mInQueue.close();

    // TODO: comment why we don't need joinable
//    if (mWorkerThread.joinable())
//    {
    mWorkerThread.join();
//    }

    // TODO: note the possibility that there are remaining items
    // in this queue for now
}

///////////////////////////////////////////////////////////////////////////////
//
void llDiskCache::requestThread()
{
    while (!mInQueue.isClosed())
    {
        // consider API call to test as well as pop to avoid a second lock
        auto item = mInQueue.popBack();

        // when we have N kinds of requests (initially READ/WRITE but based
        // on existing API, we might need APPEND too, add an enum with
        // request type and make sure the ID is unique

        // call the callable - this is what does the work
        auto res = item();

        // put the result out to the outbound results queue
        mOutQueue.pushFront(res);
    }

    // TODO: comment and be precise about what close() does
    mOutQueue.close();
}

///////////////////////////////////////////////////////////////////////////////
//
void llDiskCache::perTick(/*request_map_t& rm, LLThreadSafeQueue<result>& out*/)
{
    result res;
    while (mOutQueue.tryPopBack((res)))
    {
        // TODO: add comment - consider changing to inspect queue for (empty or counter too high or LLTimer expiration)
        // to avoid spending to long here

        std::cout << "Working: thread returned " << res.ok;
        std::cout << " with id = " << res.id;
        std::cout << " and a payload of " << res.payload << std::endl;

        // note: no need to lock the map because it's only accessed on main thread
        auto found = mRequestMap.find(res.id);
        if (found != mRequestMap.end())
        {
            found->second.cb(found->second.cbd, res.payload, res.ok);
            mRequestMap.erase(found);
        }
        else
        {
            // TODO: this should not be possible but we should consider it anyway
            std::cout << "Working: result came back with unknown id" << std::endl;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void llDiskCache::addReadRequest(std::string filename, vfs_callback_t cb, vfs_callback_data_t cbd)
{
    static U32 id = 0;

    ++id;

    mRequestMap.emplace(id, request{ cb, cbd });

    // TODO: add a comment about what to consider if the code that runs on the
    // request thread here can throw an exception - this needs to be accounted
    // and Nat has a sugestion using std::packaged_task(..)

    mInQueue.pushFront([filename]()
    {

        std::cout << "Running on thread - processing filename: " << filename << std::endl;

        bool success = true;

        U32 filesize = 12;

        // TODO: comment why this is a great idiom
        shared_payload_t file_contents = std::make_shared<std::vector<U8>>(filesize);
        memset(file_contents->data(), 'A', file_contents->size());

        return result{ id, file_contents, success };
    });
}
