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

#if 1
class NewResourceUploadInfo
{
public:
    typedef boost::shared_ptr<NewResourceUploadInfo> ptr_t;

    NewResourceUploadInfo(std::string name, 
        std::string description, 
        S32 compressionInfo,
        LLFolderType::EType destinationType,
        LLInventoryType::EType inventoryType,
        U32 nextOWnerPerms, 
        U32 groupPerms, 
        U32 everyonePerms, 
        std::string displayName,
        S32 expectedCost) :
            mName(name),
            mDescription(description),
            mDisplayName(displayName),
            mCompressionInfo(compressionInfo),
            mNextOwnerPerms(nextOWnerPerms),
            mGroupPerms(groupPerms),
            mEveryonePerms(everyonePerms),
            mExpectedUploadCost(expectedCost),
            mInventoryType(inventoryType),
            mDestinationFolderType(destinationType)
    { }

    virtual ~NewResourceUploadInfo()
    { }

    virtual LLAssetID   prepareUpload();
    virtual LLSD        generatePostBody();
    virtual void        logPreparedUpload();

    void                setAssetType(LLAssetType::EType assetType) { mAssetType = assetType; }
    LLAssetType::EType  getAssetType() const { return mAssetType; }
    std::string         getAssetTypeString() const;
    void                setTransactionId(LLTransactionID transactionId) { mTransactionId = transactionId; }
    LLTransactionID     getTransactionId() const { return mTransactionId; }
    LLUUID              getFolderId() const { return mFolderId; }

    LLAssetID           getAssetId() const { return mAssetId; }

    std::string         getName() const { return mName; };
    std::string         getDescription() const { return mDescription; };
    std::string         getDisplayName() const { return mDisplayName; };
    S32                 getCompressionInfo() const { return mCompressionInfo; };
    U32                 getNextOwnerPerms() const { return mNextOwnerPerms; };
    U32                 getGroupPerms() const { return mGroupPerms; };
    U32                 getEveryonePerms() const { return mEveryonePerms; };
    S32                 getExpectedUploadCost() const { return mExpectedUploadCost; };
    LLInventoryType::EType  getInventoryType() const { return mInventoryType; };
    std::string         getInventoryTypeString() const;
        
    LLFolderType::EType     getDestinationFolderType() const { return mDestinationFolderType; };

protected:
    LLAssetID           generateNewAssetId();
    void                incrementUploadStats() const;
    virtual void        assignDefaults();

private:
    LLAssetType::EType  mAssetType;
    std::string         mName;
    std::string         mDescription;
    std::string         mDisplayName;
    S32                 mCompressionInfo;
    U32                 mNextOwnerPerms;
    U32                 mGroupPerms;
    U32                 mEveryonePerms;
    S32                 mExpectedUploadCost;
    LLUUID              mFolderId;

    LLInventoryType::EType mInventoryType;
    LLFolderType::EType    mDestinationFolderType;

    LLAssetID           mAssetId;
    LLTransactionID     mTransactionId;
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
    const LLTransactionID &tid, 
    LLAssetType::EType type,
    NewResourceUploadInfo::ptr_t &uploadInfo,
    LLAssetStorage::LLStoreAssetCallback callback = NULL,
    void *userdata = NULL);


#else
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
	const LLTransactionID &tid, 
	LLAssetType::EType type,
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

LLAssetID generate_asset_id_for_new_upload(const LLTransactionID& tid);
void increase_new_upload_stats(LLAssetType::EType asset_type);
#endif
void assign_defaults_and_show_upload_message(
	LLAssetType::EType asset_type,
	LLInventoryType::EType& inventory_type,
	std::string& name,
	const std::string& display_name,
	std::string& description);

#if 0
LLSD generate_new_resource_upload_capability_body(
	LLAssetType::EType asset_type,
	const std::string& name,
	const std::string& desc,
	LLFolderType::EType destination_folder_type,
	LLInventoryType::EType inv_type,
	U32 next_owner_perms,
	U32 group_perms,
	U32 everyone_perms);
#endif

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
