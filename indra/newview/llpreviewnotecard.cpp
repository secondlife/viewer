/** 
 * @file llpreviewnotecard.cpp
 * @brief Implementation of the notecard editor
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewnotecard.h"

#include "llinventory.h"

#include "llagent.h"
#include "llassetuploadresponders.h"
#include "llviewerwindow.h"
#include "llbutton.h"
#include "llinventorymodel.h"
#include "lllineeditor.h"
#include "llnotify.h"
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
//#include "llfloaterchat.h"
#include "llviewerstats.h"
#include "llviewercontrol.h"		// gSavedSettings
#include "llappviewer.h"		// app_abort_quit()
#include "lllineeditor.h"
#include "lluictrlfactory.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------

const S32 PREVIEW_MIN_WIDTH =
	2 * PREVIEW_BORDER +
	2 * PREVIEW_BUTTON_WIDTH + 
	PREVIEW_PAD + RESIZE_HANDLE_WIDTH +
	PREVIEW_PAD;
const S32 PREVIEW_MIN_HEIGHT = 
	2 * PREVIEW_BORDER +
	3*(20 + PREVIEW_PAD) +
	2 * SCROLLBAR_SIZE + 128;

///----------------------------------------------------------------------------
/// Class LLPreviewNotecard
///----------------------------------------------------------------------------

// Default constructor
LLPreviewNotecard::LLPreviewNotecard(const std::string& name,
									 const LLRect& rect,
									 const std::string& title,
									 const LLUUID& item_id, 
									 const LLUUID& object_id,
									 const LLUUID& asset_id,
									 BOOL show_keep_discard,
									 LLPointer<LLViewerInventoryItem> inv_item) :
	LLPreview(name, rect, title, item_id, object_id, TRUE,
			  PREVIEW_MIN_WIDTH,
			  PREVIEW_MIN_HEIGHT,
			  inv_item),
	mAssetID( asset_id ),
	mNotecardItemID(item_id),
	mObjectID(object_id)
{
	LLRect curRect = rect;

	if (show_keep_discard)
	{
		LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_notecard_keep_discard.xml");
		childSetAction("Keep",onKeepBtn,this);
		childSetAction("Discard",onDiscardBtn,this);
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_notecard.xml");
		childSetAction("Save",onClickSave,this);
		
		if( mAssetID.isNull() )
		{
			const LLInventoryItem* item = getItem();
			if( item )
			{
				mAssetID = item->getAssetUUID();
			}
		}
	}	

	// only assert shape if not hosted in a multifloater
	if (!getHost())
	{
		reshape(curRect.getWidth(), curRect.getHeight(), TRUE);
		setRect(curRect);
	}
			
	childSetVisible("lock", FALSE);	
	
	const LLInventoryItem* item = getItem();
	
	childSetCommitCallback("desc", LLPreview::onText, this);
	if (item)
		childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);

	setTitle(title);
	
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	if (editor)
	{
		editor->setWordWrap(TRUE);
		editor->setSourceID(item_id);
		editor->setHandleEditKeysDirectly(TRUE);
	}

	gAgent.changeCameraToDefault();
}

LLPreviewNotecard::~LLPreviewNotecard()
{
}

BOOL LLPreviewNotecard::postBuild()
{
	LLViewerTextEditor *ed = getChild<LLViewerTextEditor>("Notecard Editor");
	if (ed)
	{
		ed->setNotecardInfo(mNotecardItemID, mObjectID);
		ed->makePristine();
	}
	return TRUE;
}

bool LLPreviewNotecard::saveItem(LLPointer<LLInventoryItem>* itemptr)
{
	LLInventoryItem* item = NULL;
	if (itemptr && itemptr->notNull())
	{
		item = (LLInventoryItem*)(*itemptr);
	}
	bool res = saveIfNeeded(item);
	if (res)
	{
		delete itemptr;
	}
	return res;
}

void LLPreviewNotecard::setEnabled( BOOL enabled )
{

	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

	childSetEnabled("Notecard Editor", enabled);
	childSetVisible("lock", !enabled);
	childSetEnabled("desc", enabled);
	childSetEnabled("Save", enabled && editor && (!editor->isPristine()));

}


void LLPreviewNotecard::draw()
{
	

	//childSetFocus("Save", FALSE);

	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");
	BOOL script_changed = !editor->isPristine();
	
	childSetEnabled("Save", script_changed && getEnabled());
	
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
		LLNotifications::instance().add("SaveChanges", LLSD(), LLSD(), boost::bind(&LLPreviewNotecard::handleSaveChangesDialog,this, _1, _2));
								  
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

void LLPreviewNotecard::refreshFromInventory()
{
	lldebugs << "LLPreviewNotecard::refreshFromInventory()" << llendl;
	loadAsset();
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
				LLUUID* new_uuid = new LLUUID(mItemUUID);
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
						delete new_uuid;
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
												(void*)new_uuid,
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
			childSetVisible("lock", TRUE);
		}
	}
	else
	{
		editor->setText(LLStringUtil::null);
		editor->makePristine();
		editor->setEnabled(TRUE);
		mAssetStatus = PREVIEW_ASSET_LOADED;
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
	LLPreviewNotecard* preview = LLPreviewNotecard::getInstance(*item_id);
	if( preview )
	{
		if(0 == status)
		{
			LLVFile file(vfs, asset_uuid, type, LLVFile::READ);

			S32 file_length = file.getSize();

			char* buffer = new char[file_length+1];
			file.read((U8*)buffer, file_length);		/*Flawfinder: ignore*/

			// put a EOS at the end
			buffer[file_length] = 0;

			
			LLViewerTextEditor* previewEditor = preview->getChild<LLViewerTextEditor>("Notecard Editor");

			if( (file_length > 19) && !strncmp( buffer, "Linden text version", 19 ) )
			{
				if( !previewEditor->importBuffer( buffer, file_length+1 ) )
				{
					llwarns << "Problem importing notecard" << llendl;
				}
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				previewEditor->setText(LLStringExplicit(buffer));
			}

			previewEditor->makePristine();

			const LLInventoryItem* item = preview->getItem();
			BOOL modifiable = item && gAgent.allowOperation(PERM_MODIFY,
								item->getPermissions(), GP_OBJECT_MANIPULATE);
			preview->setEnabled(modifiable);
			delete[] buffer;
			preview->mAssetStatus = PREVIEW_ASSET_LOADED;
		}
		else
		{
			LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );

			if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
				LL_ERR_FILE_EMPTY == status)
			{
				LLNotifications::instance().add("NotecardMissing");
			}
			else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
			{
				LLNotifications::instance().add("NotecardNoPermissions");
			}
			else
			{
				LLNotifications::instance().add("UnableToLoadNotecard");
			}

			llwarns << "Problem loading notecard: " << status << llendl;
			preview->mAssetStatus = PREVIEW_ASSET_ERROR;
		}
	}
	delete item_id;
}

// static
LLPreviewNotecard* LLPreviewNotecard::getInstance(const LLUUID& item_id)
{
	LLPreview* instance = NULL;
	preview_map_t::iterator found_it = LLPreview::sInstances.find(item_id);
	if(found_it != LLPreview::sInstances.end())
	{
		instance = found_it->second;
	}
	return (LLPreviewNotecard*)instance;
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
	if(!gAssetStorage)
	{
		llwarns << "Not connected to an asset storage system." << llendl;
		return false;
	}

	
	LLViewerTextEditor* editor = getChild<LLViewerTextEditor>("Notecard Editor");

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
			std::string agent_url = gAgent.getRegion()->getCapability("UpdateNotecardAgentInventory");
			std::string task_url = gAgent.getRegion()->getCapability("UpdateNotecardTaskInventory");
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
		}
	}
	return true;
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
				LLNotifications::instance().add("SaveNotecardFailObjectNotFound");
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
		LLPreviewNotecard* previewp = (LLPreviewNotecard*)LLPreview::find(info->mItemUUID);
		if (previewp && previewp->mCloseAfterSave)
		{
			previewp->close();
		}
	}
	else
	{
		llwarns << "Problem saving notecard: " << status << llendl;
		LLSD args;
		args["REASON"] = std::string(LLAssetStorage::getErrorString(status));
		LLNotifications::instance().add("SaveNotecardFailReason", args);
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
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:  // "Yes"
		mCloseAfterSave = TRUE;
		LLPreviewNotecard::onClickSave((void*)this);
		break;

	case 1:  // "No"
		mForceClose = TRUE;
		close();
		break;

	case 2: // "Cancel"
	default:
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}

void LLPreviewNotecard::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLPreview::reshape( width, height, called_from_parent );

	if( !isMinimized() )
	{
		// So that next time you open a script it will have the same height and width 
		// (although not the same position).
		gSavedSettings.setRect("NotecardEditorRect", getRect());
	}
}

// EOF
