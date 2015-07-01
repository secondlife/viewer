/**
* @file llviewerassetupload.cpp
* @author optional
* @brief brief description of the file
*
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2011, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "linden_common.h"
#include "llviewerassetupload.h"
#include "llviewertexturelist.h"
#include "llimage.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llvorbisencode.h"
#include "lluploaddialog.h"
#include "lleconomy.h"

//=========================================================================
/*static*/
void LLViewerAssetUpload::AssetInventoryUploadCoproc(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, const LLUUID &id,
    std::string url, NewResourceUploadInfo::ptr_t uploadInfo)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    uploadInfo->prepareUpload();
    uploadInfo->logPreparedUpload();

    std::string uploadMessage = "Uploading...\n\n";
    uploadMessage.append(uploadInfo->getDisplayName());
    LLUploadDialog::modalUploadDialog(uploadMessage);

    LLSD body = uploadInfo->generatePostBody();

    LLSD result = httpAdapter->postAndYield(httpRequest, url, body);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {

    }

    std::string uploader = result["uploader"].asString();

    result = httpAdapter->postFileAndYield(httpRequest, uploader, uploadInfo->getAssetId(), uploadInfo->getAssetType());

    if (!status)
    {

    }

    S32 expected_upload_cost = 0;

    // Update L$ and ownership credit information
    // since it probably changed on the server
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE ||
        uploadInfo->getAssetType() == LLAssetType::AT_SOUND ||
        uploadInfo->getAssetType() == LLAssetType::AT_ANIMATION ||
        uploadInfo->getAssetType() == LLAssetType::AT_MESH)
    {
        expected_upload_cost = 
            LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
    }

    on_new_single_inventory_upload_complete(
        uploadInfo->getAssetType(),
        uploadInfo->getInventoryType(),
        uploadInfo->getAssetTypeString(), // note the paramert calls for inv_type string...
        uploadInfo->getFolderId(),
        uploadInfo->getName(),
        uploadInfo->getDescription(),
        result,
        expected_upload_cost);

#if 0

    LLSD initalBody = generate_new_resource_upload_capability_body();



    LLSD result = httpAdapter->postAndYield(httpRequest, url, initalBody);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {

    }

    std::string state = result["state"].asString();

    if (state == "upload")
    {
//        Upload the file...
        result = httpAdapter->postFileAndYield(httpRequest, url, initalBody);

        httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        state = result["state"].asString();
    }

    if (state == "complete")
    {
        // done with the upload.
    }
    else
    {
        // an error occurred
    }
#endif
}

//=========================================================================
