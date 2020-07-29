/**
 * @file llthreadsafedisk.cpp
 * @brief Worker thread to read/write from/to disk
 *        in a thread safe manner
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

#include "llthreadsafediskcache.h"

///////////////////////////////////////////////////////////////////////////////
//
llThreadSafeDiskCache::llThreadSafeDiskCache()
{
}

llThreadSafeDiskCache::~llThreadSafeDiskCache()
{
}

///////////////////////////////////////////////////////////////////////////////
// static
void llThreadSafeDiskCache::initClass()
{
    LL_INFOS() << "llThreadSafeDiskCache::initClass()" << LL_ENDL;
    //mWorkerThread([&out]() {
    //    requestThread(in, out);
    //});
}

///////////////////////////////////////////////////////////////////////////////
// static
void llThreadSafeDiskCache::cleanupClass()
{
    LL_INFOS() << "llThreadSafeDiskCache::cleanupClass()" << LL_ENDL;

    //if (mWorkerThread.joinable())
    //{
    //    mWorkerThread.join();
    //}
}

///////////////////////////////////////////////////////////////////////////////
//
void llThreadSafeDiskCache::requestThread(LLThreadSafeQueue<callable_t>& in, 
                                          LLThreadSafeQueue<result>& out)
{
    while (!in.isClosed())
    {
        // consider API call to test as well as pop to avoid a second lock
        auto item = in.popBack();

        // when we have N kinds of requests (initially READ/WRITE but based
        // on existing API, we might need APPEND too, add an enum with
        // request type and make sure the ID is unique

        // call the callable - this is what does the work
        auto res = item();

        // put the result out to the outbound results queue
        out.pushFront(res);
    }

    // TODO: comment and be precise about what close() does
    out.close();
}

///////////////////////////////////////////////////////////////////////////////
//
void llThreadSafeDiskCache::perTick(request_map_t& rm, LLThreadSafeQueue<result>& out)
{
    result res;
    while (out.tryPopBack((res)))
    {
        // TODO: add comment - consider changing to inspect queue for (empty or counter too high or LLTimer expiration)
        // to avoid spending to long here

        std::cout << "Working: thread returned " << res.ok;
        std::cout << " with id = " << res.id;
        std::cout << " and a payload of " << res.payload << std::endl;

        // note: no need to lock the map because it's only accessed on main thread
        auto found = rm.find(res.id);
        if (found != rm.end())
        {
            found->second.cb(found->second.cbd, res.ok);
            rm.erase(found);
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
void llThreadSafeDiskCache::addReadRequest(std::string filename, vfs_callback_t cb, vfs_callback_data_t cbd)
{
    static U32 id = 0;

    ++id;

    mRequestMap.emplace(id, request{ cb, cbd });

    // TODO: add a comment about what to consider if the code that runs on the 
    // request thread here can throw an exception - this needs to be accounted 
    // and Nat has a sugestion using std::packaged_task(..)

    mInQueue.pushFront([filename]() {

        std::cout << "Running on thread - processing filename: " << filename << std::endl;

        // simulate doing some work
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        bool success = true;
        std::string payload("This will eventually be the contents of the file we read");

        return result{ id, payload, success };
    });
}
