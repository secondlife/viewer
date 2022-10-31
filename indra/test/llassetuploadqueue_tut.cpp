/** 
 * @file asset_upload_queue_tut.cpp
 * @brief Tests for newview/llassetuploadqueue.cpp
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"
#include "lltut.h"

#include "mock_http_client.h"
#include "../newview/llassetuploadqueue.cpp"

// Mock implementation.
LLAssetUploadResponder::LLAssetUploadResponder(const LLSD& post_data,
                                               const LLUUID& vfile_id,
                                               LLAssetType::EType asset_type)
{
}

LLAssetUploadResponder::LLAssetUploadResponder(const LLSD& post_data, const std::string& file_name)
{
}

LLAssetUploadResponder::~LLAssetUploadResponder()
{
}

void LLAssetUploadResponder::httpFailure()
{
}

void LLAssetUploadResponder::httpSuccess()
{
}

void LLAssetUploadResponder::uploadUpload(const LLSD& content)
{
}

void LLAssetUploadResponder::uploadComplete(const LLSD& content)
{
}

void LLAssetUploadResponder::uploadFailure(const LLSD& content)
{
}

LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
                                                               const LLUUID& vfile_id,
                                                               LLAssetType::EType asset_type) :
    LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}

LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
                                                               const std::string& file_name) :
    LLAssetUploadResponder(post_data, file_name)
{
}

LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
                                                               const std::string& file_name,
                                                               const LLUUID& queue_id) :
    LLAssetUploadResponder(post_data, file_name)
{
}

void LLUpdateTaskInventoryResponder::uploadComplete(const LLSD& content)
{
}

namespace tut
{
    class asset_upload_queue_test_data : public MockHttpClient {};
    typedef test_group<asset_upload_queue_test_data> asset_upload_queue_test;
    typedef asset_upload_queue_test::object asset_upload_queue_object;
    tut::asset_upload_queue_test asset_upload_queue("asset_upload_queue");

    void queue(LLAssetUploadQueue& q, const std::string& filename)
    {
        LLUUID task_id;
        LLUUID item_id;
        BOOL is_running = FALSE; 
        BOOL is_target_mono = TRUE; 
        LLUUID queue_id;
        q.queue(filename, task_id, item_id, is_running, is_target_mono, queue_id);
    }

    class LLTestSupplier : public LLAssetUploadQueueSupplier
    {
    public:

        void set(LLAssetUploadQueue* queue) {mQueue = queue;}
        
        virtual LLAssetUploadQueue* get() const
            {
                return mQueue;
            }

    private:
        LLAssetUploadQueue* mQueue;  
    };

    template<> template<>
    void asset_upload_queue_object::test<1>()
    {
        setupTheServer();
        reset();
        LLTestSupplier* supplier = new LLTestSupplier();
        LLAssetUploadQueue q("http://localhost:8888/test/success", supplier);
        supplier->set(&q);
        queue(q, "foo.bar");
        ensure("upload queue not empty before request", q.isEmpty());
        runThePump(10);
        ensure("upload queue not empty after request", q.isEmpty());
    }

    template<> template<>
    void asset_upload_queue_object::test<2>()
    {
        reset();
        LLTestSupplier* supplier = new LLTestSupplier();
        LLAssetUploadQueue q("http://localhost:8888/test/error", supplier);
        supplier->set(&q);
        queue(q, "foo.bar");
        ensure("upload queue not empty before request", q.isEmpty());
        runThePump(10);
        ensure("upload queue not empty after request", q.isEmpty());
    }

    template<> template<>
    void asset_upload_queue_object::test<3>()
    {
        reset();
        LLTestSupplier* supplier = new LLTestSupplier();
        LLAssetUploadQueue q("http://localhost:8888/test/timeout", supplier);
        supplier->set(&q);
        queue(q, "foo.bar");
        ensure("upload queue not empty before request", q.isEmpty());
        runThePump(10);
        ensure("upload queue not empty after request", q.isEmpty());
    }

    template<> template<>
    void asset_upload_queue_object::test<4>()
    {
        reset();
        LLTestSupplier* supplier = new LLTestSupplier();
        LLAssetUploadQueue q("http://localhost:8888/test/success", supplier);
        supplier->set(&q);
        queue(q, "foo.bar");
        queue(q, "baz.bar");
        ensure("upload queue empty before request", !q.isEmpty());
        runThePump(10);
        ensure("upload queue not empty before request", q.isEmpty());
        runThePump(10);
        ensure("upload queue not empty after request", q.isEmpty());
    }

    template<> template<>
    void asset_upload_queue_object::test<5>()
    {
        reset();
        LLTestSupplier* supplier = new LLTestSupplier();
        LLAssetUploadQueue q("http://localhost:8888/test/success", supplier);
        supplier->set(&q);
        queue(q, "foo.bar");
        runThePump(10);
        ensure("upload queue not empty before request", q.isEmpty());
        queue(q, "baz.bar");
        ensure("upload queue not empty after request", q.isEmpty());
        runThePump(10);
        killServer();
    }
}
