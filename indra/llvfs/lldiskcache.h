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

        // TODO: not shared_payload_t -> something else
        typedef std::shared_ptr<std::vector<U8>> shared_payload_t;
        typedef std::unique_ptr<U8> write_payload_t;
        typedef std::function<void(void*, shared_payload_t, bool)> vfs_callback_t;  // TODO: no VFS in name
        typedef void* vfs_callback_data_t;

        void addReadRequest(std::string filename,
                            vfs_callback_t cb,
                            vfs_callback_data_t cbd);

        void addWriteRequest(std::string filename,
                             shared_payload_t buffer,
                             vfs_callback_t cb,
                             vfs_callback_data_t cbd);

    private:
        std::thread mWorkerThread;

        // todo: better name
        struct result
        {
            U32 id;
            shared_payload_t payload;
            bool ok;
        };

        // todo: better name
        struct request
        {
            vfs_callback_t cb;
            vfs_callback_data_t cbd;
        };

        BOOL tick() override;
        const F32 mTimerPeriod;

        typedef std::function<result()> callable_t;
        LLThreadSafeQueue<callable_t> mITaskQueue;
        LLThreadSafeQueue<result> mResultQueue;

        typedef std::map<U32, request> request_map_t;
        request_map_t mRequestMap;

        U32 mRequestId{ 0 };

    private:
        void requestThread();
};

#endif // _LLDISKCACHE
