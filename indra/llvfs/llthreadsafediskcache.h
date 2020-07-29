/**
 * @file llthreadsafedisk.h
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

#ifndef _LLTHREADSAFEDISKCACHE
#define _LLTHREADSAFEDISKCACHE

#include "llthreadsafequeue.h"

// todo: where to put this? shouldn't be in global scope
typedef std::function<void(void*, bool)> vfs_callback_t;
typedef void* vfs_callback_data_t;

// todo: better name
struct result
{
    U32 id;
    std::string payload;
    bool ok;
};

// todo: better name
struct request
{
    vfs_callback_t cb;
    vfs_callback_data_t cbd;
};

class llThreadSafeDiskCache
{
    public:
        llThreadSafeDiskCache();
        virtual ~llThreadSafeDiskCache();

        static void initClass();
	    static void cleanupClass();

    private:
        std::thread mWorkerThread;

        typedef std::function<result()> callable_t;
        LLThreadSafeQueue<callable_t> mInQueue;
        LLThreadSafeQueue<result> mOutQueue;

        typedef std::map<U32, request> request_map_t;
        request_map_t mRequestMap;

    private:
        void requestThread(LLThreadSafeQueue<callable_t>& in, LLThreadSafeQueue<result>& out);
        void perTick(request_map_t& rm, LLThreadSafeQueue<result>& out);
        void addReadRequest(std::string filename, vfs_callback_t cb, vfs_callback_data_t cbd);
};

#endif // _LLTHREADSAFEDISKCACHE
