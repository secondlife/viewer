/** 
 * @file llpreviewnotecard.cpp
 * @brief Implementation of the notecard editor
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

#include "llviewerprecompiledheaders.h"

#include "llpreviewnotecard.h"

#include "llinventory.h"

#include "llagent.h"
#include "lldraghandle.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llfloaterreg.h"
#include "llinventorydefines.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnotificationsutil.h"
#include "llresmgr.h"
#include "roles_constants.h"
#include "llscrollbar.h"
#include "llselectmgr.h"
#include "llviewertexteditor.h"
#include "llvfile.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "lldir.h"
#include "llviewerstats.h"
#include "llviewercontrol.h"		// gSavedSettings
#include "llappviewer.h"		// app_abort_quit()
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llviewerassetupload.h"

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLPreviewNotecard::LLPreviewNotecard(const LLSD& key) //const LLUUID& item_id, 
	: LLPreview( key )
{
	const LLInventoryItem *item = getItem();
	if (item)
	{
		mAssetID = item->getAssetUUID();
	}	
}

LLPreviewNotecard::~LLPreviewNotecard()
{
}

BOOL LLPreviewNotecard::postBuild()
{
	LLViewerTextEditor *ed = getChild<LLViewerTextEditor>("Notecard Editor");
	ed->setNotecardInfo(mItemUUID, mObjectID, getKey());
	ed->makePristine();

	childSetAction("Save", onClickSave, this);
	getChildView("lock")->setVisible( FALSE);	

	childSetAction("Delete", onClickDelete, this);
	getChildView("Delete")->setEnabled(false);

	const LLInventoryItem* item = getItem();

	childSetCommitCallback("desc", LLPreview::onText, this);
	if (item)
	{
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
		BOOL source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(item->getUUID(), gInventory.getLibraryRootFolderID());
		getChildView("Delete")->setEnabled(!source_library);
	}
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);

	return LLPreview::postBuild();
}

bool LLPreviewNotecard::saveItem()
{
	LLInventoryItem* item = gInventory.getItem(mItemUUID);
	return saveIfNeeded(item);
}

void LLPreviewNotecard::setEnabled( BOOL enabled )
{

	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	getChildView("Notecard Editor")->setEnabled(enabled);
	getChildView("lock")->setVisible( !enabled);
	getChildView("desc")->setEnabled(enabled);
	getChildView("Save")->setEnabled(enabled && editor && (!editor->isPristine()));
}


void LLPreviewNotecard::draw()
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");
	BOOL changed = !editor->isPristine();

	getChildView("Save")->setEnabled(changed && getEnabled());
	
	LLPreview::draw();
}

// virtual
BOOL LLPreviewNotecard::handleKeyHere(KEY key, MASK mask)
{
	if(('S' == key) && (MASK_CONTROL == (mask & MASK_CONTROL)))
	{
		saveIfNeeded();
		return TRUE;
	}

	return LLPreview::handleKeyHere(key, mask);
}

// virtual
BOOL LLPreviewNotecard::canClose()
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if(mForceClose || editor->isPristine())
	{
		return TRUE;
	}
	else
	{
		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotificationsUtil::add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLPreviewNotecard::handleSaveChangesDialog,this, _1, _2));
								  
		return FALSE;
	}
}

const LLInventoryItem* LLPreviewNotecard::getDragItem()
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if(editor)
	{
		return editor->getDragItem();
	}
	return NULL;
}

bool LLPreviewNotecard::hasEmbeddedInventory()
{
	LLViewerTextEditor* editor = NULL;
	editor = getChild<LLViewerTextEditor>("Notecard Editor");
	if (!editor) return false;
	return editor->hasEmbeddedInventory();
}

void LLPreviewNotecard::refreshFromInventory(const LLUUID& new_item_id)
{
	if (new_item_id.notNull())
	{
		mItemUUID = new_item_id;
		setKey(LLSD(new_item_id));
	}
	LL_DEBUGS() << "LLPreviewNotecard::refreshFromInventory()" << LL_ENDL;
	loadAsset();
}

void LLPreviewNotecard::updateTitleButtons()
{
	LLPreview::updateTitleButtons();

	LLUICtrl* lock_btn = getChild<LLUICtrl>("lock");
	if(lock_btn->getVisible() && !isMinimized()) // lock button stays visible if floater is minimized.
	{
		LLRect lock_rc = lock_btn->getRect();
		LLRect buttons_rect = getDragHandle()->getButtonsRect();
		buttons_rect.mLeft = lock_rc.mLeft;
		getDragHandle()->setButtonsRect(buttons_rect);
	}
}

void LLPreviewNotecard::loadAsset()
{
	// request the asset.
	const LLInventoryItem* item = getItem();
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if (!editor)
		return;


	if(item)
	{
		LLPermissions perm(item->getPermissions());
		BOOL is_owner = gAgent.allowOperation(PERM_OWNER, perm, GP_OBJECT_MANIPULATE);
		BOOL allow_copy = gAgent.allowOperation(PERM_COPY, perm, GP_OBJECT_MANIPULATE);
		BOOL allow_modify = canModify(mObjectUUID, item);
		BOOL source_library = mObjectUUID.isNull() && gInventory.isObjectDescendentOf(mItemUUID, gInventory.getLibraryRootFolderID());

		if (allow_copy || gAgent.isGodlike())
		{
			mAssetID = item->getAssetUUID();
			if(mAssetID.isNull())
			{
				editor->setText(LLStringUtil::null);
				editor->makePristine();
				editor->setEnabled(TRUE);
				mAssetStatus = PREVIEW_ASSET_LOADED;
			}
			else
			{
				LLHost source_sim = LLHost();
				LLSD* user_data = new LLSD();
				if (mObjectUUID.notNull())
				{
					LLViewerObject *objectp = gObjectList.findObject(mObjectUUID);
					if (objectp && objectp->getRegion())
					{
						source_sim = objectp->getRegion()->getHost();
					}
					else
					{
						// The object that we're trying to look at disappeared, bail.
						LL_WARNS() << "Can't find object " << mObjectUUID << " associated with notecard." << LL_ENDL;
						mAssetID.setNull();
						editor->setText(getString("no_object"));
						editor->makePristine();
						editor->setEnabled(FALSE);
						mAssetStatus = PREVIEW_ASSET_LOADED;
						return;
					}
					user_data->with("taskid", mObjectUUID).with("itemid", mItemUUID);
				}
				else
				{
				    user_data =  new LLSD(mItemUUID);
				}

				gAssetStorage->getInvItemAsset(source_sim,
												gAgent.getID(),
												gAgent.getSessionID(),
												item->getPermissions().getOwner(),
												mObjectUUID,
												item->getUUID(),
												item->getAssetUUID(),
												item->getType(),
												&onLoadComplete,
												(void*)user_data,
												TRUE);
				mAssetStatus = PREVIEW_ASSET_LOADING;
			}
		}
		else
		{
			mAssetID.setNull();
			editor->setText(getString("not_allowed"));
			editor->makePristine();
			editor->setEnabled(FALSE);
			mAssetStatus = PREVIEW_ASSET_LOADED;
		}

		if(!allow_modify)
		{
			editor->setEnabled(FALSE);
			getChildView("lock")->setVisible( TRUE);
		}

		if((allow_modify || is_owner) && !source_library)
		{
			getChildView("Delete")->setEnabled(TRUE);
		}
	}
	else
	{
		editor->setText(LLStringUtil::null);
		editor->makePristine();
		editor->setEnabled(TRUE);
		// Don't set asset status here; we may not have set the item id yet
		// (e.g. when this gets called initially)
		//mAssetStatus = PREVIEW_ASSET_LOADED;
	}
}

// static
void LLPreviewNotecard::onLoadComplete(LLVFS *vfs,
									   const LLUUID& asset_uuid,
									   LLAssetType::EType type,
									   void* user_data, S32 status, LLExtStat ext_status)
{
	LL_INFOS() << "LLPreviewNotecard::onLoadComplete()" << LL_ENDL;
	LLSD* floater_key = (LLSD*)user_data;
	LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", *floater_key);
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type, LLVFile::READ);

			S32 file_length = file.getSize();

			std::vector<char> buffer(file_length+1);
			file.read((U8*)&buffer[0], file_length);

			// put a EOS at the end
			buffer[file_length] = 0;

			
			LLViewerTextEditor* previewEditor = preview->getChild<LLViewerTextEditor>("Notecard Editor");

			if( (file_length > 19) && !strncmp( &buffer[0], "Linden text version", 19 ) )
			{
				if( !previewEditor->importBuffer( &buffer[0], file_length+1 ) )
				{
					LL_WARNS() << "Problem importing notecard" << LL_ENDL;
				}
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				previewEditor->setText(LLStringExplicit(&buffer[0]));
			}

			previewEditor->makePristine();
			BOOL modifiable = preview->canModify(preview->mObjectID, preview->getItem());
			preview->setEnabled(modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotificationsUtil::add("NotecardMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotificationsUtil::add("NotecardNoPermissions");
			}
			else
			{
				LLNotificationsUtil::add("UnableToLoadNotecard");
			}

			LL_WARNS() << "Problem loading notecard: " << status << LL_ENDL;
			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete floater_key;
}

// static
void LLPreviewNotecard::onClickSave(void* user_data)
{
	//LL_INFOS() << "LLPreviewNotecard::onBtnSave()" << LL_ENDL;
	LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
	if(preview)
	{
		preview->saveIfNeeded();
	}
}


// static
void LLPreviewNotecard::onClickDelete(void* user_data)
{
	LLPreviewNotecard* preview = (LLPreviewNotecard*)user_data;
	if(preview)
	{
		preview->deleteNotecard();
	}
}

struct LLSaveNotecardInfo
{
	LLPreviewNotecard* mSelf;
	LLUUID mItemUUID;
	LLUUID mObjectUUID;
	LLTransactionID mTransactionID;
	LLPointer<LLInventoryItem> mCopyItem;
	LLSaveNotecardInfo(LLPreviewNotecard* self, const LLUUID& item_id, const LLUUID& object_id,
					   const LLTransactionID& transaction_id, LLInventoryItem* copyitem) :
		mSelf(self), mItemUUID(item_id), mObjectUUID(object_id), mTransactionID(transaction_id), mCopyItem(copyitem)
	{
	}
};

void LLPreviewNotecard::finishInventoryUpload(LLUUID itemId, LLUUID newAssetId, LLUUID newItemId)
{
    // Update the UI with the new asset.
    LLPreviewNotecard* nc = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", LLSD(itemId));
    if (nc)
    {
        // *HACK: we have to delete the asset in the VFS so
        // that the viewer will redownload it. This is only
        // really necessary if the asset had to be modified by
        // the uploader, so this can be optimized away in some
        // cases. A better design is to have a new uuid if the
        // script actually changed the asset.
        if (nc->hasEmbeddedInventory())
        {
            gVFS->removeFile(newAssetId, LLAssetType::AT_NOTECARD);
        }
        if (newItemId.isNull())
        {
            nc->setAssetId(newAssetId);
            nc->refreshFromInventory();
        }
        else
        {
            nc->refreshFromInventory(newItemId);
        }
    }
}

void LLPreviewNotecard::finishTaskUpload(LLUUID itemId, LLUUID newAssetId, LLUUID taskId)
{

    LLSD floater_key;
    floater_key["taskid"] = taskId;
    floater_key["itemid"] = itemId;
    LLPreviewNotecard* nc = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", floater_key);
    if (nc)
    {
        if (nc->hasEmbeddedInventory())
        {
            gVFS->removeFile(newAssetId, LLAssetType::AT_NOTECARD);
        }
        nc->setAssetId(newAssetId);
        nc->refreshFromInventory();
    }
}

bool LLPreviewNotecard::saveIfNeeded(LLInventoryItem* copyitem)
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if(!editor)
	{
		LL_WARNS() << "Cannot get handle to the notecard editor." << LL_ENDL;
		return false;
	}

	if(!editor->isPristine())
	{
		std::string buffer;
		if (!editor->exportBuffer(buffer))
		{
			return false;
		}

		editor->makePristine();

		const LLInventoryItem* item = getItem();
		// save it out to database
        if (item)
        {
            const LLViewerRegion* region = gAgent.getRegion();
            if (!region)
            {
                LL_WARNS() << "Not connected to a region, cannot save notecard." << LL_ENDL;
                return false;
            }
            std::string agent_url = region->getCapability("UpdateNotecardAgentInventory");
            std::string task_url = region->getCapability("UpdateNotecardTaskInventory");

            if (!agent_url.empty() && !task_url.empty())
            {
                std::string url;
                LLResourceUploadInfo::ptr_t uploadInfo;

                if (mObjectUUID.isNull() && !agent_url.empty())
                {
                    uploadInfo = LLResourceUploadInfo::ptr_t(new LLBufferedAssetUploadInfo(mItemUUID, LLAssetType::AT_NOTECARD, buffer, 
                        boost::bind(&LLPreviewNotecard::finishInventoryUpload, _1, _2, _3)));
                    url = agent_url;
                }
                else if (!mObjectUUID.isNull() && !task_url.empty())
                {
                    uploadInfo = LLResourceUploadInfo::ptr_t(new LLBufferedAssetUploadInfo(mObjectUUID, mItemUUID, LLAssetType::AT_NOTECARD, buffer, 
                        boost::bind(&LLPreviewNotecard::finishTaskUpload, _1, _3, mObjectUUID)));
                    url = task_url;
                }

                if (!url.empty() && uploadInfo)
                {
                    mAssetStatus = PREVIEW_ASSET_LOADING;
                    setEnabled(false);

                    LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
                }

            }
			else if (gAssetStorage)
			{
                // We need to update the asset information
                LLTransactionID tid;
                LLAssetID asset_id;
                tid.generate();
                asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

                LLVFile file(gVFS, asset_id, LLAssetType::AT_NOTECARD, LLVFile::APPEND);


				LLSaveNotecardInfo* info = new LLSaveNotecardInfo(this, mItemUUID, mObjectUUID,
																tid, copyitem);

                S32 size = buffer.length() + 1;
                file.setMaxSize(size);
                file.write((U8*)buffer.c_str(), size);

				gAssetStorage->storeAssetData(tid, LLAssetType::AT_NOTECARD,
												&onSaveComplete,
												(void*)info,
												FALSE);
				return true;
			}
			else // !gAssetStorage
			{
				LL_WARNS() << "Not connected to an asset storage system." << LL_ENDL;
				return false;
			}
			if(mCloseAfterSave)
			{
				closeFloater();
			}
		}
	}
	return true;
}

void LLPreviewNotecard::deleteNotecard()
{
	LLNotificationsUtil::add("DeleteNotecard", LLSD(), LLSD(), boost::bind(&LLPreviewNotecard::handleConfirmDeleteDialog,this, _1, _2));
}

// static
void LLPreviewNotecard::onSaveComplete(const LLUUID& asset_uuid, void* user_data, S32 status, LLExtStat ext_status) // StoreAssetData callback (fixed)
{
	LLSaveNotecardInfo* info = (LLSaveNotecardInfo*)user_data;
	if(info && (0 == status))
	{
		if(info->mObjectUUID.isNull())
		{
			LLViewerInventoryItem* item;
			item = (LLViewerInventoryItem*)gInventory.getItem(info->mItemUUID);
			if(item)
			{
				LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
				new_item->setAssetUUID(asset_uuid);
				new_item->setTransactionID(info->mTransactionID);
				new_item->updateServer(FALSE);
				gInventory.updateItem(new_item);
				gInventory.notifyObservers();
			}
			else
			{
				LL_WARNS() << "Inventory item for script " << info->mItemUUID
						<< " is no longer in agent inventory." << LL_ENDL;
			}
		}
		else
		{
			LLViewerObject* object = gObjectList.findObject(info->mObjectUUID);
			LLViewerInventoryItem* item = NULL;
			if(object)
			{
				item = (LLViewerInventoryItem*)object->getInventoryObject(info->mItemUUID);
			}
			if(object && item)
			{
				item->setAssetUUID(asset_uuid);
				item->setTransactionID(info->mTransactionID);
				object->updateInventory(item, TASK_INVENTORY_ITEM_KEY, false);
				dialog_refresh_all();
			}
			else
			{
				LLNotificationsUtil::add("SaveNotecardFailObjectNotFound");
			}
		}
		// Perform item copy to inventory
		if (info->mCopyItem.notNull())
		{
			LLViewerTextEditor* editor = info->mSelf->getChild<LLViewerTextEditor>("Notecard Editor");
			if (editor)
			{
				editor->copyInventory(info->mCopyItem);
			}
		}
		
		// Find our window and close it if requested.

		LLPreviewNotecard* previewp = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", info->mItemUUID);
		if (previewp && previewp->mCloseAfterSave)
		{
			previewp->closeFloater();
		}
	}
	else
	{
		LL_WARNS() << "Problem saving notecard: " << status << LL_ENDL;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotificationsUtil::add("SaveNotecardFailReason", args);
	}

	std::string uuid_string;
	asset_uuid.toString(uuid_string);
	std::string filename;
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,uuid_string) + ".tmp";
	LLFile::remove(filename);
	delete info;
}

bool LLPreviewNotecard::handleSaveChangesDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:  // "Yes"
		mCloseAfterSave = TRUE;
		LLPreviewNotecard::onClickSave((void*)this);
		break;

	case 1:  // "No"
		mForceClose = TRUE;
		closeFloater();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}

bool LLPreviewNotecard::handleConfirmDeleteDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option != 0)
	{
		// canceled
		return false;
	}

	if (mObjectUUID.isNull())
	{
		// move item from agent's inventory into trash
		LLViewerInventoryItem* item = gInventory.getItem(mItemUUID);
		if (item != NULL)
		{
			const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
			gInventory.changeItemParent(item, trash_id, FALSE);
		}
	}
	else
	{
		// delete item from inventory of in-world object
		LLViewerObject* object = gObjectList.findObject(mObjectUUID);
		if(object)
		{
			LLViewerInventoryItem* item = dynamic_cast<LLViewerInventoryItem*>(object->getInventoryObject(mItemUUID));
			if (item != NULL)
			{
				object->removeInventory(mItemUUID);
			}
		}
	}

	// close floater, ignore unsaved changes
	mForceClose = TRUE;
	closeFloater();
	return false;
}

// EOF
