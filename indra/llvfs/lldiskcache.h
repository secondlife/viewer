/**
 * @file lldiskcache.h
 * @brief Worker thread to read/write from/to disk
 *        in a thread safe manner. See the implementation
 *        file for a description of how it works
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

#ifndef _LLDISKCACHE
#define _LLDISKCACHE

#include "llthreadsafequeue.h"
#include "lleventtimer.h"
#include "llsingleton.h"

class llDiskCache :
    public LLSingleton<llDiskCache>,
    public LLEventTimer
{
        LLSINGLETON(llDiskCache);

    public:
        virtual ~llDiskCache() = default;

        void cleanupSingleton() override;

        typedef std::shared_ptr<std::vector<U8>> request_payload_t;
        typedef std::function<void(request_payload_t, bool)> request_callback_t;

        void addReadRequest(std::string filename,
                            request_callback_t cb);

        void addWriteRequest(std::string filename,
                             request_payload_t buffer,
                             request_callback_t cb);

    private:
        std::thread mWorkerThread;

        struct mResult
        {
            U32 id;
            request_payload_t payload;
            bool ok;
        };

        struct mRequest
        {
            request_callback_t cb;
        };

        BOOL tick() override;

        typedef std::function<mResult()> callable_t;
        LLThreadSafeQueue<callable_t> mITaskQueue;
        LLThreadSafeQueue<mResult> mResultQueue;

        typedef std::map<U32, mRequest> request_map_t;
        request_map_t mRequestMap;

        U32 mRequestId{ 0 };

    private:
        void requestThread();
};

#endif // _LLDISKCACHE
