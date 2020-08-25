/**
 * @file lldiskcache.h
 * @brief Use a worker thread read/write from/to disk
 *        via a thread safe request queue and avoid the
 *        need for complex code or file locking.
 *        See the implementation file for a description of
 *        how it works and how to use it.
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
#include "llassettype.h"
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
        typedef std::function<void(request_payload_t, std::string, bool)> request_callback_t;

        void addReadRequest(std::string id,
                            request_callback_t cb);

        void addWriteRequest(std::string id,
                             LLAssetType::EType at,
                             request_payload_t buffer,
                             request_callback_t cb);

        request_payload_t waitForReadComplete(std::string id);

        struct ReadError : public LLContinueError
        {
            ReadError(const std::string& what) : LLContinueError(what) {}
        };

    private:
        std::thread mWorkerThread;

        struct mResult
        {
            U32 id;
            request_payload_t payload;
            std::string filename;
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

        /**
         * This is the time period in seconds to run for.
         * It can set this to 0 which means run every time
         * through the main event loop.
         * All this extra decoration is required because of
         * the interesting C++ class initialization rules
         * in action when we initialize the LLEventTimer base
         * within the constructor for this class.
         */
        static constexpr F32 mTimePeriod = 0.05;

        U32 mRequestId{ 0 };

    private:
        void requestThread();
        const std::string assetTypeToString(LLAssetType::EType at);
        const std::string idToFilepath(const std::string id, LLAssetType::EType at);
};

#endif // _LLDISKCACHE
