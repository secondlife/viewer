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
#include "llviewertexturelist.h"
#include "llimage.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llvorbisencode.h"
#include "lluploaddialog.h"
#include "llpreviewscript.h"
#include "llnotificationsutil.h"
#include "llagent.h"
#include "llfloaterreg.h"
#include "llfloatersnapshot.h"
#include "llstatusbar.h"
#include "llinventorypanel.h"
#include "llsdutil.h"
#include "llviewerassetupload.h"
#include "llappviewer.h"
#include "llviewerstats.h"
#include "llfilesystem.h"
#include "llgesturemgr.h"
#include "llpreviewnotecard.h"
#include "llpreviewgesture.h"
#include "llcoproceduremanager.h"
#include "llthread.h"
#include "llkeyframemotion.h"
#include "lldatapacker.h"
#include "llvoavatarself.h"

void dialog_refresh_all();

static const U32 LL_ASSET_UPLOAD_TIMEOUT_SEC = 60;

LLResourceUploadInfo::LLResourceUploadInfo(LLTransactionID transactId,
        LLAssetType::EType assetType, std::string name, std::string description,
        S32 compressionInfo, LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType, U32 nextOWnerPerms,
        U32 groupPerms, U32 everyonePerms, S32 expectedCost, bool showInventory) :
    mTransactionId(transactId),
    mAssetType(assetType),
    mName(name),
    mDescription(description),
    mCompressionInfo(compressionInfo),
    mDestinationFolderType(destinationType),
    mInventoryType(inventoryType),
    mNextOwnerPerms(nextOWnerPerms),
    mGroupPerms(groupPerms),
    mEveryonePerms(everyonePerms),
    mExpectedUploadCost(expectedCost),
    mShowInventory(showInventory),
    mFolderId(LLUUID::null),
    mItemId(LLUUID::null),
    mAssetId(LLAssetID::null)
{ }


LLResourceUploadInfo::LLResourceUploadInfo(std::string name, 
        std::string description, S32 compressionInfo, 
        LLFolderType::EType destinationType, LLInventoryType::EType inventoryType, 
        U32 nextOWnerPerms, U32 groupPerms, U32 everyonePerms, S32 expectedCost, bool showInventory) :
    mName(name),
    mDescription(description),
    mCompressionInfo(compressionInfo),
    mDestinationFolderType(destinationType),
    mInventoryType(inventoryType),
    mNextOwnerPerms(nextOWnerPerms),
    mGroupPerms(groupPerms),
    mEveryonePerms(everyonePerms),
    mExpectedUploadCost(expectedCost),
    mShowInventory(showInventory),
    mTransactionId(),
    mAssetType(LLAssetType::AT_NONE),
    mFolderId(LLUUID::null),
    mItemId(LLUUID::null),
    mAssetId(LLAssetID::null)
{ 
    mTransactionId.generate();
}

LLResourceUploadInfo::LLResourceUploadInfo(LLAssetID assetId, LLAssetType::EType assetType, std::string name) :
    mAssetId(assetId),
    mAssetType(assetType),
    mName(name),
    mDescription(),
    mCompressionInfo(0),
    mDestinationFolderType(LLFolderType::FT_NONE),
    mInventoryType(LLInventoryType::IT_NONE),
    mNextOwnerPerms(0),
    mGroupPerms(0),
    mEveryonePerms(0),
    mExpectedUploadCost(0),
    mShowInventory(true),
    mTransactionId(),
    mFolderId(LLUUID::null),
    mItemId(LLUUID::null)
{
}

LLSD LLResourceUploadInfo::prepareUpload()
{
    if (mAssetId.isNull())
        generateNewAssetId();

    incrementUploadStats();
    assignDefaults();

    return LLSD().with("success", LLSD::Boolean(true));
}

std::string LLResourceUploadInfo::getAssetTypeString() const
{
    return LLAssetType::lookup(mAssetType);
}

std::string LLResourceUploadInfo::getInventoryTypeString() const
{
    return LLInventoryType::lookup(mInventoryType);
}

LLSD LLResourceUploadInfo::generatePostBody()
{
    LLSD body;

    body["folder_id"] = mFolderId;
    body["asset_type"] = getAssetTypeString();
    body["inventory_type"] = getInventoryTypeString();
    body["name"] = mName;
    body["description"] = mDescription;
    body["next_owner_mask"] = LLSD::Integer(mNextOwnerPerms);
    body["group_mask"] = LLSD::Integer(mGroupPerms);
    body["everyone_mask"] = LLSD::Integer(mEveryonePerms);

    return body;

}

void LLResourceUploadInfo::logPreparedUpload()
{
    LL_INFOS() << "*** Uploading: " << std::endl <<
        "Type: " << LLAssetType::lookup(mAssetType) << std::endl <<
        "UUID: " << mAssetId.asString() << std::endl <<
        "Name: " << mName << std::endl <<
        "Desc: " << mDescription << std::endl <<
        "Expected Upload Cost: " << mExpectedUploadCost << std::endl <<
        "Folder: " << mFolderId << std::endl <<
        "Asset Type: " << LLAssetType::lookup(mAssetType) << LL_ENDL;
}

LLUUID LLResourceUploadInfo::finishUpload(LLSD &result)
{
    if (getFolderId().isNull())
    {
        return LLUUID::null;
    }

    U32 permsEveryone = PERM_NONE;
    U32 permsGroup = PERM_NONE;
    U32 permsNextOwner = PERM_ALL;

    if (result.has("new_next_owner_mask"))
    {
        // The server provided creation perms so use them.
        // Do not assume we got the perms we asked for in
        // since the server may not have granted them all.
        permsEveryone = result["new_everyone_mask"].asInteger();
        permsGroup = result["new_group_mask"].asInteger();
        permsNextOwner = result["new_next_owner_mask"].asInteger();
    }
    else
    {
        // The server doesn't provide creation perms
        // so use old assumption-based perms.
        if (getAssetTypeString() != "snapshot")
        {
            permsNextOwner = PERM_MOVE | PERM_TRANSFER;
        }
    }

    LLPermissions new_perms;
    new_perms.init(
        gAgent.getID(),
        gAgent.getID(),
        LLUUID::null,
        LLUUID::null);

    new_perms.initMasks(
        PERM_ALL,
        PERM_ALL,
        permsEveryone,
        permsGroup,
        permsNextOwner);

    U32 flagsInventoryItem = 0;
    if (result.has("inventory_flags"))
    {
        flagsInventoryItem = static_cast<U32>(result["inventory_flags"].asInteger());
        if (flagsInventoryItem != 0)
        {
            LL_INFOS() << "inventory_item_flags " << flagsInventoryItem << LL_ENDL;
        }
    }
    S32 creationDate = time_corrected();

    LLUUID serverInventoryItem = result["new_inventory_item"].asUUID();
    LLUUID serverAssetId = result["new_asset"].asUUID();

    LLPointer<LLViewerInventoryItem> item = new LLViewerInventoryItem(
        serverInventoryItem,
        getFolderId(),
        new_perms,
        serverAssetId,
        getAssetType(),
        getInventoryType(),
        getName(),
        getDescription(),
        LLSaleInfo::DEFAULT,
        flagsInventoryItem,
        creationDate);

    gInventory.updateItem(item);
    gInventory.notifyObservers();

    return serverInventoryItem;
}


LLAssetID LLResourceUploadInfo::generateNewAssetId()
{
    if (gDisconnected)
    {
        LLAssetID rv;

        rv.setNull();
        return rv;
    }
    mAssetId = mTransactionId.makeAssetID(gAgent.getSecureSessionID());

    return mAssetId;
}

void LLResourceUploadInfo::incrementUploadStats() const
{
    if (LLAssetType::AT_SOUND == mAssetType)
    {
        add(LLStatViewer::UPLOAD_SOUND, 1);
    }
    else if (LLAssetType::AT_TEXTURE == mAssetType)
    {
        add(LLStatViewer::UPLOAD_TEXTURE, 1);
    }
    else if (LLAssetType::AT_ANIMATION == mAssetType)
    {
        add(LLStatViewer::ANIMATION_UPLOADS, 1);
    }
}

void LLResourceUploadInfo::assignDefaults()
{
    if (LLInventoryType::IT_NONE == mInventoryType)
    {
        mInventoryType = LLInventoryType::defaultForAssetType(mAssetType);
    }
    LLStringUtil::stripNonprintable(mName);
    LLStringUtil::stripNonprintable(mDescription);

    if (mName.empty())
    {
        mName = "(No Name)";
    }
    if (mDescription.empty())
    {
        mDescription = "(No Description)";
    }

    mFolderId = gInventory.findUserDefinedCategoryUUIDForType(
        (mDestinationFolderType == LLFolderType::FT_NONE) ?
        (LLFolderType::EType)mAssetType : mDestinationFolderType);
}

std::string LLResourceUploadInfo::getDisplayName() const
{
    return (mName.empty()) ? mAssetId.asString() : mName;
};

bool LLResourceUploadInfo::findAssetTypeOfExtension(const std::string& exten, LLAssetType::EType& asset_type)
{
	U32 codec;
	return findAssetTypeAndCodecOfExtension(exten, asset_type, codec, false);
}

// static
bool LLResourceUploadInfo::findAssetTypeAndCodecOfExtension(const std::string& exten, LLAssetType::EType& asset_type, U32& codec, bool bulk_upload)
{
	bool succ = false;
	std::string exten_lc(exten);
	LLStringUtil::toLower(exten_lc);
	codec = LLImageBase::getCodecFromExtension(exten_lc);
	if (codec != IMG_CODEC_INVALID)
	{
		asset_type = LLAssetType::AT_TEXTURE; 
		succ = true;
	}
	else if (exten_lc == "wav")
	{
		asset_type = LLAssetType::AT_SOUND; 
		succ = true;
	}
	else if (exten_lc == "anim")
	{
		asset_type = LLAssetType::AT_ANIMATION; 
		succ = true;
	}
	else if (!bulk_upload && (exten_lc == "bvh"))
	{
		asset_type = LLAssetType::AT_ANIMATION;
		succ = true;
	}

	return succ;
}

//=========================================================================
LLNewFileResourceUploadInfo::LLNewFileResourceUploadInfo(
    std::string fileName,
    std::string name,
    std::string description,
    S32 compressionInfo,
    LLFolderType::EType destinationType,
    LLInventoryType::EType inventoryType,
    U32 nextOWnerPerms,
    U32 groupPerms,
    U32 everyonePerms,
    S32 expectedCost,
    bool show_inventory) :
    LLResourceUploadInfo(name, description, compressionInfo,
    destinationType, inventoryType,
    nextOWnerPerms, groupPerms, everyonePerms, expectedCost, show_inventory),
    mFileName(fileName)
{
}

LLSD LLNewFileResourceUploadInfo::prepareUpload()
{
    if (getAssetId().isNull())
        generateNewAssetId();

    LLSD result = exportTempFile();
    if (result.has("error"))
        return result;

    return LLResourceUploadInfo::prepareUpload();
}

LLSD LLNewFileResourceUploadInfo::exportTempFile()
{
    std::string filename = gDirUtilp->getTempFilename();

    std::string exten = gDirUtilp->getExtension(getFileName());

    LLAssetType::EType assetType = LLAssetType::AT_NONE;
	U32 codec = IMG_CODEC_INVALID;
	bool found_type = findAssetTypeAndCodecOfExtension(exten, assetType, codec);

    std::string errorMessage;
    std::string errorLabel;

    bool error = false;

    if (exten.empty())
    {
        std::string shortName = gDirUtilp->getBaseFileName(filename);

        // No extension
        errorMessage = llformat(
            "No file extension for the file: '%s'\nPlease make sure the file has a correct file extension",
            shortName.c_str());
        errorLabel = "NoFileExtension";
        error = true;
    }
    else if (!found_type)
    {
        // Unknown extension
        errorMessage = llformat(LLTrans::getString("UnknownFileExtension").c_str(), exten.c_str());
        errorLabel = "ErrorMessage";
        error = TRUE;;
    }
    else if (assetType == LLAssetType::AT_TEXTURE)
    {
        // It's an image file, the upload procedure is the same for all
        if (!LLViewerTextureList::createUploadFile(getFileName(), filename, codec))
        {
            errorMessage = llformat("Problem with file %s:\n\n%s\n",
                getFileName().c_str(), LLImage::getLastError().c_str());
            errorLabel = "ProblemWithFile";
            error = true;
        }
    }
    else if (assetType == LLAssetType::AT_SOUND)
    {
        S32 encodeResult = 0;

        LL_INFOS() << "Attempting to encode wav as an ogg file" << LL_ENDL;

        encodeResult = encode_vorbis_file(getFileName(), filename);

        if (LLVORBISENC_NOERR != encodeResult)
        {
            switch (encodeResult)
            {
            case LLVORBISENC_DEST_OPEN_ERR:
                errorMessage = llformat("Couldn't open temporary compressed sound file for writing: %s\n", filename.c_str());
                errorLabel = "CannotOpenTemporarySoundFile";
                break;

            default:
                errorMessage = llformat("Unknown vorbis encode failure on: %s\n", getFileName().c_str());
                errorLabel = "UnknownVorbisEncodeFailure";
                break;
            }
            error = true;
        }
    }
    else if (exten == "bvh")
    {
        errorMessage = llformat("We do not currently support bulk upload of animation files\n");
        errorLabel = "DoNotSupportBulkAnimationUpload";
        error = true;
    }
    else if (exten == "anim")
    {
		// Default unless everything succeeds
		errorLabel = "ProblemWithFile";
		error = true;

        // read from getFileName()
		LLAPRFile infile;
		infile.open(getFileName(),LL_APR_RB);
		if (!infile.getFileHandle())
		{
			LL_WARNS() << "Couldn't open file for reading: " << getFileName() << LL_ENDL;
			errorMessage = llformat("Failed to open animation file %s\n", getFileName().c_str());
		}
		else
		{
			S32 size = LLAPRFile::size(getFileName());
			U8* buffer = new U8[size];
			S32 size_read = infile.read(buffer,size);
			if (size_read != size)
			{
				errorMessage = llformat("Failed to read animation file %s: wanted %d bytes, got %d\n", getFileName().c_str(), size, size_read);
			}
			else
			{
				LLDataPackerBinaryBuffer dp(buffer, size);
				LLKeyframeMotion *motionp = new LLKeyframeMotion(getAssetId());
				motionp->setCharacter(gAgentAvatarp);
				if (motionp->deserialize(dp, getAssetId(), false))
				{
					// write to temp file
					bool succ = motionp->dumpToFile(filename);
					if (succ)
					{
						assetType = LLAssetType::AT_ANIMATION;
						errorLabel = "";
						error = false;
					}
					else
					{
						errorMessage = "Failed saving temporary animation file";
					}
				}
				else
				{
					errorMessage = "Failed reading animation file";
				}
			}
		}
    }
    else
    {
        // Unknown extension
        errorMessage = llformat(LLTrans::getString("UnknownFileExtension").c_str(), exten.c_str());
        errorLabel = "ErrorMessage";
        error = TRUE;;
    }

    if (error)
    {
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        errorResult["label"] = errorLabel;
        return errorResult;
    }

    setAssetType(assetType);

    // copy this file into the cache for upload
    S32 file_size;
    LLAPRFile infile;
    infile.open(filename, LL_APR_RB, NULL, &file_size);
    if (infile.getFileHandle())
    {
        LLFileSystem file(getAssetId(), assetType, LLFileSystem::APPEND);

        const S32 buf_size = 65536;
        U8 copy_buf[buf_size];
        while ((file_size = infile.read(copy_buf, buf_size)))
        {
            file.write(copy_buf, file_size);
        }
    }
    else
    {
        errorMessage = llformat("Unable to access output file: %s", filename.c_str());
        LLSD errorResult(LLSD::emptyMap());

        errorResult["error"] = LLSD::Binary(true);
        errorResult["message"] = errorMessage;
        return errorResult;
    }

    return LLSD();

}

//=========================================================================
LLBufferedAssetUploadInfo::LLBufferedAssetUploadInfo(LLUUID itemId, LLAssetType::EType assetType, std::string buffer, invnUploadFinish_f finish) :
    LLResourceUploadInfo(std::string(), std::string(), 0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
        0, 0, 0, 0),
    mTaskUpload(false),
    mTaskId(LLUUID::null),
    mContents(buffer),
    mInvnFinishFn(finish),
    mTaskFinishFn(nullptr),
    mStoredToCache(false)
{
    setItemId(itemId);
    setAssetType(assetType);
}

LLBufferedAssetUploadInfo::LLBufferedAssetUploadInfo(LLUUID itemId, LLPointer<LLImageFormatted> image, invnUploadFinish_f finish) :
    LLResourceUploadInfo(std::string(), std::string(), 0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
        0, 0, 0, 0),
    mTaskUpload(false),
    mTaskId(LLUUID::null),
    mContents(),
    mInvnFinishFn(finish),
    mTaskFinishFn(nullptr),
    mStoredToCache(false)
{
    setItemId(itemId);

    EImageCodec codec = static_cast<EImageCodec>(image->getCodec());

    switch (codec)
    {
    case IMG_CODEC_JPEG:
        setAssetType(LLAssetType::AT_IMAGE_JPEG);
        LL_INFOS() << "Upload Asset type set to JPEG." << LL_ENDL;
        break;
    case IMG_CODEC_TGA:
        setAssetType(LLAssetType::AT_IMAGE_TGA);
        LL_INFOS() << "Upload Asset type set to TGA." << LL_ENDL;
        break;
    default:
        LL_WARNS() << "Unknown codec to asset type transition. Codec=" << (int)codec << "." << LL_ENDL;
        break;
    }

    size_t imageSize = image->getDataSize();
    mContents.reserve(imageSize);
    mContents.assign((char *)image->getData(), imageSize);
}

LLBufferedAssetUploadInfo::LLBufferedAssetUploadInfo(LLUUID taskId, LLUUID itemId, LLAssetType::EType assetType, std::string buffer, taskUploadFinish_f finish) :
    LLResourceUploadInfo(std::string(), std::string(), 0, LLFolderType::FT_NONE, LLInventoryType::IT_NONE,
        0, 0, 0, 0),
    mTaskUpload(true),
    mTaskId(taskId),
    mContents(buffer),
    mInvnFinishFn(nullptr),
    mTaskFinishFn(finish),
    mStoredToCache(false)
{
    setItemId(itemId);
    setAssetType(assetType);
}

LLSD LLBufferedAssetUploadInfo::prepareUpload()
{
    if (getAssetId().isNull())
        generateNewAssetId();

    LLFileSystem file(getAssetId(), getAssetType(), LLFileSystem::APPEND);

    S32 size = mContents.length() + 1;
    file.write((U8*)mContents.c_str(), size);

    mStoredToCache = true;

    return LLSD().with("success", LLSD::Boolean(true));
}

LLSD LLBufferedAssetUploadInfo::generatePostBody()
{
    LLSD body;

    if (!getTaskId().isNull())
    {
        body["task_id"] = getTaskId();
    }
    body["item_id"] = getItemId();

    return body;
}

LLUUID LLBufferedAssetUploadInfo::finishUpload(LLSD &result)
{
    LLUUID newAssetId = result["new_asset"].asUUID();
    LLUUID itemId = getItemId();

    if (mStoredToCache)
    {
        LLAssetType::EType assetType(getAssetType());
        LLFileSystem::renameFile(getAssetId(), assetType, newAssetId, assetType);
    }

    if (mTaskUpload)
    {
        LLUUID taskId = getTaskId();

        dialog_refresh_all();

        if (mTaskFinishFn)
        {
            mTaskFinishFn(itemId, taskId, newAssetId, result);
        }
    }
    else
    {
        LLUUID newItemId(LLUUID::null);

        if (itemId.notNull())
        {
            LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(itemId);
            if (!item)
            {
                LL_WARNS() << "Inventory item for " << getDisplayName() << " is no longer in agent inventory." << LL_ENDL;
                return newAssetId;
            }

            // Update viewer inventory item
            LLPointer<LLViewerInventoryItem> newItem = new LLViewerInventoryItem(item);
            newItem->setAssetUUID(newAssetId);

            gInventory.updateItem(newItem);

            newItemId = newItem->getUUID();
            LL_INFOS() << "Inventory item " << item->getName() << " saved into " << newAssetId.asString() << LL_ENDL;
        }

        if (mInvnFinishFn)
        {
            mInvnFinishFn(itemId, newAssetId, newItemId, result);
        }
        gInventory.notifyObservers();
    }

    return newAssetId;
}

//=========================================================================

LLScriptAssetUpload::LLScriptAssetUpload(LLUUID itemId, std::string buffer, invnUploadFinish_f finish):
    LLBufferedAssetUploadInfo(itemId, LLAssetType::AT_LSL_TEXT, buffer, finish),
    mExerienceId(),
    mTargetType(MONO),
    mIsRunning(false)
{
}

LLScriptAssetUpload::LLScriptAssetUpload(LLUUID taskId, LLUUID itemId, TargetType_t targetType,
        bool isRunning, LLUUID exerienceId, std::string buffer, taskUploadFinish_f finish):
    LLBufferedAssetUploadInfo(taskId, itemId, LLAssetType::AT_LSL_TEXT, buffer, finish),
    mExerienceId(exerienceId),
    mTargetType(targetType),
    mIsRunning(isRunning)
{
}

LLSD LLScriptAssetUpload::generatePostBody()
{
    LLSD body;

    if (getTaskId().isNull())
    {
        body["item_id"] = getItemId();
        body["target"] = "mono";
    }
    else
    {
        body["task_id"] = getTaskId();
        body["item_id"] = getItemId();
        body["is_script_running"] = getIsRunning();
        body["target"] = (getTargetType() == MONO) ? "mono" : "lsl2";
        body["experience"] = getExerienceId();
    }

    return body;
}

//=========================================================================
/*static*/
LLUUID LLViewerAssetUpload::EnqueueInventoryUpload(const std::string &url, const LLResourceUploadInfo::ptr_t &uploadInfo)
{
    std::string procName("LLViewerAssetUpload::AssetInventoryUploadCoproc(");
    
    LLUUID queueId = LLCoprocedureManager::instance().enqueueCoprocedure("Upload", 
        procName + LLAssetType::lookup(uploadInfo->getAssetType()) + ")",
        boost::bind(&LLViewerAssetUpload::AssetInventoryUploadCoproc, _1, _2, url, uploadInfo));

    return queueId;
}

//=========================================================================
/*static*/
void LLViewerAssetUpload::AssetInventoryUploadCoproc(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, 
    const LLUUID &id, std::string url, LLResourceUploadInfo::ptr_t uploadInfo)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);
    httpOptions->setTimeout(LL_ASSET_UPLOAD_TIMEOUT_SEC);

    LLSD result = uploadInfo->prepareUpload();
    uploadInfo->logPreparedUpload();

    if (result.has("error"))
    {
        HandleUploadError(LLCore::HttpStatus(499), result, uploadInfo);
        return;
    }

    llcoro::suspend();

    if (uploadInfo->showUploadDialog())
    {
        std::string uploadMessage = "Uploading...\n\n";
        uploadMessage.append(uploadInfo->getDisplayName());
        LLUploadDialog::modalUploadDialog(uploadMessage);
    }

    LLSD body = uploadInfo->generatePostBody();

    result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOptions);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if ((!status) || (result.has("error")))
    {
        HandleUploadError(status, result, uploadInfo);
        if (uploadInfo->showUploadDialog())
            LLUploadDialog::modalUploadFinished();
        return;
    }

    std::string uploader = result["uploader"].asString();

    bool success = false;
    if (!uploader.empty() && uploadInfo->getAssetId().notNull())
    {
        result = httpAdapter->postFileAndSuspend(httpRequest, uploader, uploadInfo->getAssetId(), uploadInfo->getAssetType(), httpOptions);
        httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        std::string ulstate = result["state"].asString();

        if ((!status) || (ulstate != "complete"))
        {
            HandleUploadError(status, result, uploadInfo);
            if (uploadInfo->showUploadDialog())
                LLUploadDialog::modalUploadFinished();
            return;
        }
        if (!result.has("success"))
        {
            result["success"] = LLSD::Boolean((ulstate == "complete") && status);
        }

        S32 uploadPrice = result["upload_price"].asInteger();

        if (uploadPrice > 0)
        {
            // this upload costed us L$, update our balance
            // and display something saying that it cost L$
            LLStatusBar::sendMoneyBalanceRequest();

            LLSD args;
            args["AMOUNT"] = llformat("%d", uploadPrice);
            LLNotificationsUtil::add("UploadPayment", args);
        }
    }
    else
    {
        LL_WARNS() << "No upload url provided.  Nothing uploaded, responding with previous result." << LL_ENDL;
    }
    LLUUID serverInventoryItem = uploadInfo->finishUpload(result);

    if (uploadInfo->showInventoryPanel())
    {
        if (serverInventoryItem.notNull())
        {
            success = true;

            LLFocusableElement* focus = gFocusMgr.getKeyboardFocus();

            // Show the preview panel for textures and sounds to let
            // user know that the image (or snapshot) arrived intact.
            LLInventoryPanel* panel = LLInventoryPanel::getActiveInventoryPanel(FALSE);
            LLInventoryPanel::openInventoryPanelAndSetSelection(TRUE, serverInventoryItem, FALSE, TAKE_FOCUS_NO, (panel == NULL));

            // restore keyboard focus
            gFocusMgr.setKeyboardFocus(focus);
        }
        else
        {
            LL_WARNS() << "Can't find a folder to put it in" << LL_ENDL;
        }
    }

    // remove the "Uploading..." message
    if (uploadInfo->showUploadDialog())
        LLUploadDialog::modalUploadFinished();

    // Let the Snapshot floater know we have finished uploading a snapshot to inventory
    LLFloater* floater_snapshot = LLFloaterReg::findInstance("snapshot");
    if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE && floater_snapshot && floater_snapshot->isShown())
    {
        floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", success).with("msg", "inventory")));
    }
}

//=========================================================================
/*static*/
void LLViewerAssetUpload::HandleUploadError(LLCore::HttpStatus status, LLSD &result, LLResourceUploadInfo::ptr_t &uploadInfo)
{
    std::string reason;
    std::string label("CannotUploadReason");

    LL_WARNS() << ll_pretty_print_sd(result) << LL_ENDL;

    if (result.has("label"))
    {
        label = result["label"].asString();
    }

    if (result.has("message"))
    {
        reason = result["message"].asString();
    }
    else
    {
        switch (status.getType())
        {
        case 404:
            reason = LLTrans::getString("AssetUploadServerUnreacheble");
            break;
        case 499:
            reason = LLTrans::getString("AssetUploadServerDifficulties");
            break;
        case 503:
            reason = LLTrans::getString("AssetUploadServerUnavaliable");
            break;
        default:
            reason = LLTrans::getString("AssetUploadRequestInvalid");
        }
    }

    LLSD args;
    if(label == "ErrorMessage")
    {
        args["ERROR_MESSAGE"] = reason;
    }
    else
    {
        args["FILE"] = uploadInfo->getDisplayName();
        args["REASON"] = reason;
        args["ERROR"] = reason;
    }

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

    // Let the Snapshot floater know we have failed uploading.
    LLFloaterSnapshot* floater_snapshot = LLFloaterSnapshot::findInstance();
    if (floater_snapshot && floater_snapshot->isWaitingState())
    {
        if (uploadInfo->getAssetType() == LLAssetType::AT_IMAGE_JPEG)
        {
            floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", false).with("msg", "postcard")));
        }
        if (uploadInfo->getAssetType() == LLAssetType::AT_TEXTURE)
        {
            floater_snapshot->notify(LLSD().with("set-finished", LLSD().with("ok", false).with("msg", "inventory")));
        }
    }
}

