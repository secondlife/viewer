/** 
 * @file llviewermenufile.h
 * @brief "File" menu in the main menu bar.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LLVIEWERMENUFILE_H
#define LLVIEWERMENUFILE_H

#include "llfoldertype.h"
#include "llassetstorage.h"
#include "llinventorytype.h"
#include "llfilepicker.h"
#include "llthread.h"
#include <queue>

class LLTransactionID;


void init_menu_file();

class NewResourceUploadInfo
{
public:
    typedef boost::shared_ptr<NewResourceUploadInfo> ptr_t;

    NewResourceUploadInfo(
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
            S32 expectedCost) :
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
        mFolderId(LLUUID::null),
        mItemId(LLUUID::null),
        mAssetId(LLAssetID::null)
    { }

    virtual ~NewResourceUploadInfo()
    { }

    virtual LLSD        prepareUpload();
    virtual LLSD        generatePostBody();
    virtual void        logPreparedUpload();
    virtual LLUUID      finishUpload(LLSD &result);

    //void                setAssetType(LLAssetType::EType assetType) { mAssetType = assetType; }
    //void                setTransactionId(LLTransactionID transactionId) { mTransactionId = transactionId; }

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

    virtual std::string getDisplayName() const;

    LLUUID              getFolderId() const { return mFolderId; }
    LLUUID              getItemId() const { return mItemId; }
    LLAssetID           getAssetId() const { return mAssetId; }

protected:
    NewResourceUploadInfo(
            std::string name,
            std::string description,
            S32 compressionInfo,
            LLFolderType::EType destinationType,
            LLInventoryType::EType inventoryType,
            U32 nextOWnerPerms,
            U32 groupPerms,
            U32 everyonePerms,
            S32 expectedCost) :
        mName(name),
        mDescription(description),
        mCompressionInfo(compressionInfo),
        mDestinationFolderType(destinationType),
        mInventoryType(inventoryType),
        mNextOwnerPerms(nextOWnerPerms),
        mGroupPerms(groupPerms),
        mEveryonePerms(everyonePerms),
        mExpectedUploadCost(expectedCost),
        mTransactionId(),
        mAssetType(LLAssetType::AT_NONE),
        mFolderId(LLUUID::null),
        mItemId(LLUUID::null),
        mAssetId(LLAssetID::null)
    { }

    void                setTransactionId(LLTransactionID tid) { mTransactionId = tid; }
    void                setAssetType(LLAssetType::EType assetType) { mAssetType = assetType; }

    LLAssetID           generateNewAssetId();
    void                incrementUploadStats() const;
    virtual void        assignDefaults();

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
};

class NewFileResourceUploadInfo : public NewResourceUploadInfo
{
public:
    NewFileResourceUploadInfo(
        std::string fileName,
        std::string name,
        std::string description,
        S32 compressionInfo,
        LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType,
        U32 nextOWnerPerms,
        U32 groupPerms,
        U32 everyonePerms,
        S32 expectedCost);

    virtual LLSD        prepareUpload();

    std::string         getFileName() const { return mFileName; };

protected:

    virtual LLSD        exportTempFile();

private:
    std::string         mFileName;

};


LLUUID upload_new_resource(
    const std::string& src_filename,
    std::string name,
    std::string desc,
    S32 compression_info,
    LLFolderType::EType destination_folder_type,
    LLInventoryType::EType inv_type,
    U32 next_owner_perms,
    U32 group_perms,
    U32 everyone_perms,
    const std::string& display_name,
    LLAssetStorage::LLStoreAssetCallback callback,
    S32 expected_upload_cost,
    void *userdata);

void upload_new_resource(
    NewResourceUploadInfo::ptr_t &uploadInfo,
    LLAssetStorage::LLStoreAssetCallback callback = NULL,
    void *userdata = NULL);


void assign_defaults_and_show_upload_message(
	LLAssetType::EType asset_type,
	LLInventoryType::EType& inventory_type,
	std::string& name,
	const std::string& display_name,
	std::string& description);

void on_new_single_inventory_upload_complete(
	LLAssetType::EType asset_type,
	LLInventoryType::EType inventory_type,
	const std::string inventory_type_string,
	const LLUUID& item_folder_id,
	const std::string& item_name,
	const std::string& item_description,
	const LLSD& server_response,
	S32 upload_price);

class LLFilePickerThread : public LLThread
{ //multi-threaded file picker (runs system specific file picker in background and calls "notify" from main thread)
public:

	static std::queue<LLFilePickerThread*> sDeadQ;
	static LLMutex* sMutex;

	static void initClass();
	static void cleanupClass();
	static void clearDead();

	std::string mFile; 

	LLFilePicker::ELoadFilter mFilter;

	LLFilePickerThread(LLFilePicker::ELoadFilter filter)
		: LLThread("file picker"), mFilter(filter)
	{

	}

	void getFile();

	virtual void run();

	virtual void notify(const std::string& filename) = 0;
};


#endif
