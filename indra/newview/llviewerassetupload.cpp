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
#include "llpreviewscript.h"
#include "llnotificationsutil.h"
#include "lleconomy.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llstatusbar.h"
#include "llinventorypanel.h"
#include "llsdutil.h"

//=========================================================================
/*static*/
void LLViewerAssetUpload::AssetInventoryUploadCoproc(LLCoros::self &self, LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, const LLUUID &id,
    std::string url, NewResourceUploadInfo::ptr_t uploadInfo)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = uploadInfo->prepareUpload();
    uploadInfo->logPreparedUpload();

    if (result.has("error"))
    {
        HandleUploadError(LLCore::HttpStatus(499), result, uploadInfo);
        return;
    }

    //self.yield();

    std::string uploadMessage = "Uploading...\n\n";
    uploadMessage.append(uploadInfo->getDisplayName());
    LLUploadDialog::modalUploadDialog(uploadMessage);

    LLSD body = uploadInfo->generatePostBody();

    result = httpAdapter->postAndYield(self, httpRequest, url, body);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if ((!status) || (result.has("error")))
    {
        HandleUploadError(status, result, uploadInfo);
        LLUploadDialog::modalUploadFinished();
        return;
    }

    std::string uploader = result["uploader"].asString();

    result = httpAdapter->postFileAndYield(self, httpRequest, uploader, uploadInfo->getAssetId(), uploadInfo->getAssetType());
    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        HandleUploadError(status, result, uploadInfo);
        LLUploadDialog::modalUploadFinished();
        return;
    }

    S32 uploadPrice = 0;

    // Update L$ and ownership credit information
    // since it probably changed on the server
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE ||
        uploadInfo->getAssetType() == LLAssetType::AT_SOUND ||
        uploadInfo->getAssetType() == LLAssetType::AT_ANIMATION ||
        uploadInfo->getAssetType() == LLAssetType::AT_MESH)
    {
        uploadPrice = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
    }

    bool success = false;

    if (uploadPrice > 0)
    {
        // this upload costed us L$, update our balance
        // and display something saying that it cost L$
        LLStatusBar::sendMoneyBalanceRequest();

        LLSD args;
        args["AMOUNT"] = llformat("%d", uploadPrice);
        LLNotificationsUtil::add("UploadPayment", args);
    }

    LLUUID serverInventoryItem = uploadInfo->finishUpload(result);

    if (serverInventoryItem.notNull())
    {
        success = true;

        // Show the preview panel for textures and sounds to let
        // user know that the image (or snapshot) arrived intact.
        LLInventoryPanel* panel = LLInventoryPanel::getActiveInventoryPanel();
        if (panel)
        {
            LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();
            panel->setSelection(serverInventoryItem, TAKE_FOCUS_NO);

            // restore keyboard focus
            gFocusMgr.setKeyboardFocus(focus);
        }
    }
    else
    {
        LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
    }

    // remove the "Uploading..." message
    LLUploadDialog::modalUploadFinished();

    // Let the Snapshot floater know we have finished uploading a snapshot to inventory.
    LLFloater* floater_snapshot = LLFloaterReg::findInstance("snapshot");
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE && floater_snapshot)
    {
        floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", success).with("msg", "inventory")));
    }
}

//=========================================================================
/*static*/
void LLViewerAssetUpload::HandleUploadError(LLCore::HttpStatus status, LLSD &result, NewResourceUploadInfo::ptr_t &uploadInfo)
{
    std::string reason;
    std::string label("CannotUploadReason");

    LL_WARNS() << ll_pretty_print_sd(result) << LL_ENDL;

    if (result.has("label"))
    {
        label = result["label"];
    }

    if (result.has("message"))
    {
        reason = result["message"];
    }
    else
    {
        if (status.getType() == 499)
        {
            reason = "The server is experiencing unexpected difficulties.";
        }
        else
        {
            reason = "Error in upload request.  Please visit "
                "http://secondlife.com/support for help fixing this problem.";
        }
    }

    LLSD args;
    args["FILE"] = uploadInfo->getDisplayName();
    args["REASON"] = reason;

    LLNotificationsUtil::add(label, args);

    // unfreeze script preview
    if (uploadInfo->getAssetType() == LLAssetType::AT_LSL_TEXT)
    {
        LLPreviewLSL* preview = LLFloaterReg::findTypedInstance<LLPreviewLSL>("preview_script", 
            uploadInfo->getItemId());
        if (preview)
        {
            LLSD errors;
            errors.append(LLTrans::getString("UploadFailed") + reason);
            preview->callbackLSLCompileFailed(errors);
        }
    }

}
