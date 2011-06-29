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
#include "llinventoryfunctions.h" // for change_item_parent()

#include "llagent.h"
#include "llassetuploadresponders.h"
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
		getChildView("Delete")->setEnabled(true);
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
	lldebugs << "LLPreviewNotecard::refreshFromInventory()" << llendl;
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
		if (gAgent.allowOperation(PERM_COPY, item->getPermissions(),
									GP_OBJECT_MANIPULATE)
			|| gAgent.isGodlike())
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
				LLHost source_sim = LLHost::invalid;
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
						llwarns << "Can't find object " << mObjectUUID << " associated with notecard." << llendl;
						mAssetID.setNull();
						editor->setText(getString("no_object"));
						editor->makePristine();
						editor->setEnabled(FALSE);
						mAssetStatus = PREVIEW_ASSET_LOADED;
						return;
					}
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
												(void*)new LLUUID(mItemUUID),
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
		if(!gAgent.allowOperation(PERM_MODIFY, item->getPermissions(),
								GP_OBJECT_MANIPULATE))
		{
			editor->setEnabled(FALSE);
			getChildView("lock")->setVisible( TRUE);
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
	llinfos << "LLPreviewNotecard::onLoadComplete()" << llendl;
	LLUUID* item_id = (LLUUID*)user_data;
	
	LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", LLSD(*item_id));
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
					llwarns << "Problem importing notecard" << llendl;
				}
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				previewEditor->setText(LLStringExplicit(&buffer[0]));
			}

			previewEditor->makePristine();

			const LLInventoryItem* item = preview->getItem();
			BOOL modifiable = item && gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
			preview->setEnabled(modifiable);
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

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

			llwarns << "Problem loading notecard: " << status << llendl;
			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete item_id;
}

// static
void LLPreviewNotecard::onClickSave(void* user_data)
{
	//llinfos << "LLPreviewNotecard::onBtnSave()" << llendl;
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

bool LLPreviewNotecard::saveIfNeeded(LLInventoryItem* copyitem)
{
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if(!editor)
	{
		llwarns << "Cannot get handle to the notecard editor." << llendl;
		return false;
	}

	if(!editor->isPristine())
	{
		// We need to update the asset information
		LLTransactionID tid;
		LLAssetID asset_id;
		tid.generate();
		asset_id = tid.makeAssetID(gAgent.getSecureSessionID());

		LLVFile file(gVFS, asset_id, LLAssetType::AT_NOTECARD, LLVFile::APPEND);

		std::string buffer;
		if (!editor->exportBuffer(buffer))
		{
			return false;
		}

		editor->makePristine();

		S32 size = buffer.length() + 1;
		file.setMaxSize(size);
		file.write((U8*)buffer.c_str(), size);

		const LLInventoryItem* item = getItem();
		// save it out to database
		if (item)
		{			
			const LLViewerRegion* region = gAgent.getRegion();
			if (!region)
			{
				llwarns << "Not connected to a region, cannot save notecard." << llendl;
				return false;
			}
			std::string agent_url = region->getCapability("UpdateNotecardAgentInventory");
			std::string task_url = region->getCapability("UpdateNotecardTaskInventory");

			if (mObjectUUID.isNull() && !agent_url.empty())
			{
				// Saving into agent inventory
				mAssetStatus = PREVIEW_ASSET_LOADING;
				setEnabled(FALSE);
				LLSD body;
				body["item_id"] = mItemUUID;
				llinfos << "Saving notecard " << mItemUUID
					<< " into agent inventory via " << agent_url << llendl;
				LLHTTPClient::post(agent_url, body,
					new LLUpdateAgentInventoryResponder(body, asset_id, LLAssetType::AT_NOTECARD));
			}
			else if (!mObjectUUID.isNull() && !task_url.empty())
			{
				// Saving into task inventory
				mAssetStatus = PREVIEW_ASSET_LOADING;
				setEnabled(FALSE);
				LLSD body;
				body["task_id"] = mObjectUUID;
				body["item_id"] = mItemUUID;
				llinfos << "Saving notecard " << mItemUUID << " into task "
					<< mObjectUUID << " via " << task_url << llendl;
				LLHTTPClient::post(task_url, body,
					new LLUpdateTaskInventoryResponder(body, asset_id, LLAssetType::AT_NOTECARD));
			}
			else if (gAssetStorage)
			{
				LLSaveNotecardInfo* info = new LLSaveNotecardInfo(this, mItemUUID, mObjectUUID,
																tid, copyitem);
				gAssetStorage->storeAssetData(tid, LLAssetType::AT_NOTECARD,
												&onSaveComplete,
												(void*)info,
												FALSE);
			}
			else // !gAssetStorage
			{
				llwarns << "Not connected to an asset storage system." << llendl;
				return false;
			}
		}
	}
	return true;
}

void LLPreviewNotecard::deleteNotecard()
{
	LLViewerInventoryItem* item = gInventory.getItem(mItemUUID);
	if (item != NULL)
	{
		const LLUUID trash_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TRASH);
		change_item_parent(&gInventory, item, trash_id, FALSE);
	}

	closeFloater();
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
				llwarns << "Inventory item for script " << info->mItemUUID
						<< " is no longer in agent inventory." << llendl;
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
		llwarns << "Problem saving notecard: " << status << llendl;
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

// EOF
