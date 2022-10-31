/**
* @file llviewerassetupload.h
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

#ifndef LL_VIEWER_ASSET_UPLOAD_H
#define LL_VIEWER_ASSET_UPLOAD_H

#include "llfoldertype.h"
#include "llassettype.h"
#include "llinventorytype.h"
#include "lleventcoro.h"
#include "llcoros.h"
#include "llcorehttputil.h"
#include "llimage.h"

//=========================================================================
class LLResourceUploadInfo
{
public:
    typedef std::shared_ptr<LLResourceUploadInfo> ptr_t;

    LLResourceUploadInfo(
        LLTransactionID transactId,
        LLAssetType::EType assetType,
        std::string name,
        std::string description,
        S32 compressionInfo,
        LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType,
        U32 nextOWnerPerms,
        U32 groupPerms,
        U32 everyonePerms,
        S32 expectedCost,
        bool showInventory = true);

    virtual ~LLResourceUploadInfo()
    { }

    virtual LLSD        prepareUpload();
    virtual LLSD        generatePostBody();
    virtual void        logPreparedUpload();
    virtual LLUUID      finishUpload(LLSD &result);

    LLTransactionID     getTransactionId() const { return mTransactionId; }
    LLAssetType::EType  getAssetType() const { return mAssetType; }
    std::string         getAssetTypeString() const;
    std::string         getName() const { return mName; };
    std::string         getDescription() const { return mDescription; };
    S32                 getCompressionInfo() const { return mCompressionInfo; };
    LLFolderType::EType getDestinationFolderType() const { return mDestinationFolderType; };
    LLInventoryType::EType  getInventoryType() const { return mInventoryType; };
    std::string         getInventoryTypeString() const;
    U32                 getNextOwnerPerms() const { return mNextOwnerPerms; };
    U32                 getGroupPerms() const { return mGroupPerms; };
    U32                 getEveryonePerms() const { return mEveryonePerms; };
    S32                 getExpectedUploadCost() const { return mExpectedUploadCost; };

    virtual bool        showUploadDialog() const { return true; }
    virtual bool        showInventoryPanel() const { return mShowInventory; }

    virtual std::string getDisplayName() const;

    LLUUID              getFolderId() const { return mFolderId; }
    LLUUID              getItemId() const { return mItemId; }
    LLAssetID           getAssetId() const { return mAssetId; }

    static bool         findAssetTypeOfExtension(const std::string& exten, LLAssetType::EType& asset_type);
    static bool         findAssetTypeAndCodecOfExtension(const std::string& exten, LLAssetType::EType& asset_type, U32& codec, bool bulk_upload = true);

protected:
    LLResourceUploadInfo(
        std::string name,
        std::string description,
        S32 compressionInfo,
        LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType,
        U32 nextOWnerPerms,
        U32 groupPerms,
        U32 everyonePerms,
        S32 expectedCost,
        bool showInventory = true);

    LLResourceUploadInfo(
        LLAssetID assetId,
        LLAssetType::EType assetType,
        std::string name );

    void                setTransactionId(LLTransactionID tid) { mTransactionId = tid; }
    void                setAssetType(LLAssetType::EType assetType) { mAssetType = assetType; }
    void                setItemId(LLUUID itemId) { mItemId = itemId; }

    LLAssetID           generateNewAssetId();
    void                incrementUploadStats() const;
    virtual void        assignDefaults();

    void                setAssetId(LLUUID assetId) { mAssetId = assetId; }

private:
    LLTransactionID     mTransactionId;
    LLAssetType::EType  mAssetType;
    std::string         mName;
    std::string         mDescription;
    S32                 mCompressionInfo;
    LLFolderType::EType mDestinationFolderType;
    LLInventoryType::EType mInventoryType;
    U32                 mNextOwnerPerms;
    U32                 mGroupPerms;
    U32                 mEveryonePerms;
    S32                 mExpectedUploadCost;

    LLUUID              mFolderId;
    LLUUID              mItemId;
    LLAssetID           mAssetId;
    bool                mShowInventory;
};

//-------------------------------------------------------------------------
class LLNewFileResourceUploadInfo : public LLResourceUploadInfo
{
public:
    LLNewFileResourceUploadInfo(
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
        bool show_inventory = true);

    virtual LLSD        prepareUpload();

    std::string         getFileName() const { return mFileName; };

protected:

    virtual LLSD        exportTempFile();

private:
    std::string         mFileName;

};

//-------------------------------------------------------------------------
class LLBufferedAssetUploadInfo : public LLResourceUploadInfo
{
public:
    typedef std::function<void(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD response)> invnUploadFinish_f;
    typedef std::function<void(LLUUID itemId, LLUUID taskId, LLUUID newAssetId, LLSD response)> taskUploadFinish_f;

    LLBufferedAssetUploadInfo(LLUUID itemId, LLAssetType::EType assetType, std::string buffer, invnUploadFinish_f finish);
    LLBufferedAssetUploadInfo(LLUUID itemId, LLPointer<LLImageFormatted> image, invnUploadFinish_f finish);
    LLBufferedAssetUploadInfo(LLUUID taskId, LLUUID itemId, LLAssetType::EType assetType, std::string buffer, taskUploadFinish_f finish);

    virtual LLSD        prepareUpload();
    virtual LLSD        generatePostBody();
    virtual LLUUID      finishUpload(LLSD &result);

    LLUUID              getTaskId() const { return mTaskId; }
    const std::string & getContents() const { return mContents; }

    virtual bool        showUploadDialog() const { return false; }
    virtual bool        showInventoryPanel() const { return false; }

protected:


private:
    bool                mTaskUpload;
    LLUUID              mTaskId;
    std::string         mContents;
    invnUploadFinish_f  mInvnFinishFn;
    taskUploadFinish_f  mTaskFinishFn;
    bool                mStoredToCache;
};

//-------------------------------------------------------------------------
class LLScriptAssetUpload : public LLBufferedAssetUploadInfo
{
public:
    enum TargetType_t
    {
        LSL2,
        MONO
    };

    LLScriptAssetUpload(LLUUID itemId, std::string buffer, invnUploadFinish_f finish);
    LLScriptAssetUpload(LLUUID taskId, LLUUID itemId, TargetType_t targetType, 
            bool isRunning, LLUUID exerienceId, std::string buffer, taskUploadFinish_f finish);

    virtual LLSD        generatePostBody();

    LLUUID              getExerienceId() const { return mExerienceId; }
    TargetType_t        getTargetType() const { return mTargetType; }
    bool                getIsRunning() const { return mIsRunning; }

private:
    LLUUID              mExerienceId;
    TargetType_t        mTargetType;
    bool                mIsRunning;

};

//=========================================================================
class LLViewerAssetUpload
{
public:
    static LLUUID EnqueueInventoryUpload(const std::string &url, const LLResourceUploadInfo::ptr_t &uploadInfo);

    static void AssetInventoryUploadCoproc(LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t &httpAdapter, const LLUUID &id, std::string url, LLResourceUploadInfo::ptr_t uploadInfo);

private:
    static void HandleUploadError(LLCore::HttpStatus status, LLSD &result, LLResourceUploadInfo::ptr_t &uploadInfo);
};

#endif // !VIEWER_ASSET_UPLOAD_H
