/**
 * @file llassetuploadresponders.cpp
 * @brief Processes responses received for asset upload requests.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */ 

#include "llviewerprecompiledheaders.h"

#include "llassetuploadresponders.h"

#include "llagent.h"
#include "llfloaterbuycurrency.h"
#include "lleconomy.h"
#include "llfilepicker.h"
#include "llfocusmgr.h"
#include "llnotify.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llpermissionsflags.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llscrolllistctrl.h"
#include "lluploaddialog.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h"
#include "viewer.h"

void dialog_refresh_all();

LLAssetUploadResponder::LLAssetUploadResponder(const LLSD &post_data,
											   const LLUUID& vfile_id,
											   LLAssetType::EType asset_type)
	: LLHTTPClient::Responder(),
	  mPostData(post_data),
	  mVFileID(vfile_id),
	  mAssetType(asset_type)
{
	if (!gVFS->getExists(vfile_id, asset_type))
	{
		llwarns << "LLAssetUploadResponder called with nonexistant vfile_id" << llendl;
		mVFileID.setNull();
		mAssetType = LLAssetType::AT_NONE;
		return;
	}
}

LLAssetUploadResponder::LLAssetUploadResponder(const LLSD &post_data,
											   const std::string& file_name)
	: LLHTTPClient::Responder(),
	  mPostData(post_data),
	  mFileName(file_name)
{
}

LLAssetUploadResponder::~LLAssetUploadResponder()
{
	if (!mFileName.empty())
	{
		// Delete temp file
		LLFile::remove(mFileName.c_str());
	}
}

// virtual
void LLAssetUploadResponder::error(U32 statusNum, const std::string& reason)
{
	llinfos << "LLAssetUploadResponder::error " << statusNum 
			<< " reason: " << reason << llendl;
	LLStringBase<char>::format_map_t args;
	switch(statusNum)
	{
		case 400:
			args["[FILE]"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["[REASON]"] = "Error in upload request.  Please visit "
				"http://secondlife.com/support for help fixing this problem.";
			gViewerWindow->alertXml("CannotUploadReason", args);
			break;
		case 500:
		default:
			args["[FILE]"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["[REASON]"] = "The server is experiencing unexpected "
				"difficulties.";
			gViewerWindow->alertXml("CannotUploadReason", args);
			break;
	}
	LLUploadDialog::modalUploadFinished();
}

//virtual 
void LLAssetUploadResponder::result(const LLSD& content)
{
	lldebugs << "LLAssetUploadResponder::result from capabilities" << llendl;

	std::string state = content["state"];
	if (state == "upload")
	{
		uploadUpload(content);
	}
	else if (state == "complete")
	{
		// rename file in VFS with new asset id
		if (mFileName.empty())
		{
			// rename the file in the VFS to the actual asset id
			gVFS->renameFile(mVFileID, mAssetType, content["new_asset"].asUUID(), mAssetType);
		}
		uploadComplete(content);
	}
	else
	{
		uploadFailure(content);
	}
}

void LLAssetUploadResponder::uploadUpload(const LLSD& content)
{
	std::string uploader = content["uploader"];
	if (mFileName.empty())
	{
		LLHTTPClient::postFile(uploader, mVFileID, mAssetType, this);
	}
	else
	{
		LLHTTPClient::postFile(uploader, mFileName, this);
	}
}

void LLAssetUploadResponder::uploadFailure(const LLSD& content)
{
	std::string reason = content["state"];
	// deal with L$ errors
	if (reason == "insufficient funds")
	{
		LLFloaterBuyCurrency::buyCurrency("Uploading costs", gGlobalEconomy->getPriceUpload());
	}
	else
	{
		LLStringBase<char>::format_map_t args;
		args["[FILE]"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
		args["[REASON]"] = content["message"].asString();
		gViewerWindow->alertXml("CannotUploadReason", args);
	}
}

void LLAssetUploadResponder::uploadComplete(const LLSD& content)
{
}

LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(const LLSD& post_data,
														   const LLUUID& vfile_id,
														   LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}

LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(const LLSD& post_data, const std::string& file_name)
: LLAssetUploadResponder(post_data, file_name)
{
}

//virtual 
void LLNewAgentInventoryResponder::uploadComplete(const LLSD& content)
{
	lldebugs << "LLNewAgentInventoryResponder::result from capabilities" << llendl;

	LLAssetType::EType asset_type = LLAssetType::lookup(mPostData["asset_type"].asString().c_str());
	LLInventoryType::EType inventory_type = LLInventoryType::lookup(mPostData["inventory_type"].asString().c_str());

	// Update L$ and ownership credit information
	// since it probably changed on the server
	if (asset_type == LLAssetType::AT_TEXTURE ||
		asset_type == LLAssetType::AT_SOUND ||
		asset_type == LLAssetType::AT_ANIMATION)
	{
		gMessageSystem->newMessageFast(_PREHASH_MoneyBalanceRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_MoneyData);
		gMessageSystem->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
		gAgent.sendReliableMessage();

		LLString::format_map_t args;
		args["[AMOUNT]"] = llformat("%d",gGlobalEconomy->getPriceUpload());
		LLNotifyBox::showXml("UploadPayment", args);
	}

	// Actually add the upload to viewer inventory
	llinfos << "Adding " << content["new_inventory_item"].asUUID() << " "
			<< content["new_asset"].asUUID() << " to inventory." << llendl;
	if(mPostData["folder_id"].asUUID().notNull())
	{
		LLPermissions perm;
		U32 next_owner_perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
		if (mPostData["inventory_type"].asString() == "snapshot")
		{
			next_owner_perm = PERM_ALL;
		}
		else
		{
			next_owner_perm = PERM_MOVE | PERM_TRANSFER;
		}
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, next_owner_perm);
		S32 creation_date_now = time_corrected();
		LLPointer<LLViewerInventoryItem> item
			= new LLViewerInventoryItem(content["new_inventory_item"].asUUID(),
										mPostData["folder_id"].asUUID(),
										perm,
										content["new_asset"].asUUID(),
										asset_type,
										inventory_type,
										mPostData["name"].asString(),
										mPostData["description"].asString(),
										LLSaleInfo::DEFAULT,
										LLInventoryItem::II_FLAGS_NONE,
										creation_date_now);
		gInventory.updateItem(item);
		gInventory.notifyObservers();

		// Show the preview panel for textures and sounds to let
		// user know that the image (or snapshot) arrived intact.
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if(view)
		{
			LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
			LLFocusMgr::FocusLostCallback callback = gFocusMgr.getFocusCallback();

			view->getPanel()->setSelection(content["new_inventory_item"].asUUID(), TAKE_FOCUS_NO);
			if((LLAssetType::AT_TEXTURE == asset_type)
				|| (LLAssetType::AT_SOUND == asset_type))
			{
				view->getPanel()->openSelected();
			}
			//LLInventoryView::dumpSelectionInformation((void*)view);
			// restore keyboard focus
			gFocusMgr.setKeyboardFocus(focus_ctrl, callback);
		}
	}
	else
	{
		llwarns << "Can't find a folder to put it in" << llendl;
	}

	// remove the "Uploading..." message
	LLUploadDialog::modalUploadFinished();
	
	// *FIX: This is a pretty big hack. What this does is check the
	// file picker if there are any more pending uploads. If so,
	// upload that file.
	const char* next_file = LLFilePicker::instance().getNextFile();
	if(next_file)
	{
		const char* name = LLFilePicker::instance().getDirname();

		LLString asset_name = name;
		LLString::replaceNonstandardASCII( asset_name, '?' );
		LLString::replaceChar(asset_name, '|', '?');
		LLString::stripNonprintable(asset_name);
		LLString::trim(asset_name);

		char* asset_name_str = (char*)asset_name.c_str();
		char* end_p = strrchr(asset_name_str, '.');		 // strip extension if exists
		if( !end_p )
		{
			 end_p = asset_name_str + strlen( asset_name_str );			/*Flawfinder: ignore*/
		}
			
		S32 len = llmin( (S32) (DB_INV_ITEM_NAME_STR_LEN), (S32) (end_p - asset_name_str) );

		asset_name = asset_name.substr( 0, len );

		upload_new_resource(next_file, asset_name, asset_name,
							0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE);
	}
}


LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(const LLSD& post_data,
																 const LLUUID& vfile_id,
																 LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}

LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(const LLSD& post_data,
																 const std::string& file_name)
: LLAssetUploadResponder(post_data, file_name)
{
}

//virtual 
void LLUpdateAgentInventoryResponder::uploadComplete(const LLSD& content)
{
	llinfos << "LLUpdateAgentInventoryResponder::result from capabilities" << llendl;
	LLUUID item_id = mPostData["item_id"];

	LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(item_id);
	if(!item)
	{
		llwarns << "Inventory item for " << mVFileID
			<< " is no longer in agent inventory." << llendl;
		return;
	}

	// Update viewer inventory item
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	new_item->setAssetUUID(content["new_asset"].asUUID());
	gInventory.updateItem(new_item);
	gInventory.notifyObservers();

	llinfos << "Inventory item " << item->getName() << " saved into "
		<< content["new_asset"].asString() << llendl;

	LLInventoryType::EType inventory_type = new_item->getInventoryType();
	switch(inventory_type)
	{
		case LLInventoryType::IT_NOTECARD:
			{

				// Update the UI with the new asset.
				LLPreviewNotecard* nc;
				nc = (LLPreviewNotecard*)LLPreview::find(new_item->getUUID());
				if(nc)
				{
					// *HACK: we have to delete the asset in the VFS so
					// that the viewer will redownload it. This is only
					// really necessary if the asset had to be modified by
					// the uploader, so this can be optimized away in some
					// cases. A better design is to have a new uuid if the
					// script actually changed the asset.
					if(nc->hasEmbeddedInventory())
					{
						gVFS->removeFile(
							content["new_asset"].asUUID(),
							LLAssetType::AT_NOTECARD);
					}
					nc->refreshFromInventory();
				}
			}
			break;
		case LLInventoryType::IT_LSL:
			{
				// Find our window and close it if requested.
				LLPreviewLSL* preview = (LLPreviewLSL*)LLPreview::find(item_id);
				if (preview)
				{
					// Bytecode save completed
					if (content["compiled"])
					{
						preview->callbackLSLCompileSucceeded();
					}
					else
					{
						preview->callbackLSLCompileFailed(content["errors"]);
					}
				}
			}
			break;
		case LLInventoryType::IT_WEARABLE:
		default:
			break;
	}
}


LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
																 const LLUUID& vfile_id,
																 LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}

LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
															   const std::string& file_name)
: LLAssetUploadResponder(post_data, file_name)
{
}

//virtual 
void LLUpdateTaskInventoryResponder::uploadComplete(const LLSD& content)
{
	llinfos << "LLUpdateTaskInventoryResponder::result from capabilities" << llendl;
	LLUUID item_id = mPostData["item_id"];
	LLUUID task_id = mPostData["task_id"];

	LLViewerObject* object = gObjectList.findObject(task_id);
	if (!object)
	{
		llwarns << "LLUpdateTaskInventoryResponder::uploadComplete task " << task_id
			<< " no longer exist." << llendl;
		return;
	}
	LLViewerInventoryItem* item = (LLViewerInventoryItem*)object->getInventoryObject(item_id);
	if (!item)
	{
		llwarns << "LLUpdateTaskInventoryResponder::uploadComplete item "
			<< item_id << " is no longer in task " << task_id
			<< "'s inventory." << llendl;
		return;
	}
	LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem(item);
	// Update Viewer inventory
	object->updateViewerInventoryAsset(new_item, content["new_asset"]);
	dialog_refresh_all();
	
	LLInventoryType::EType inventory_type = new_item->getInventoryType();
	switch(inventory_type)
	{
		case LLInventoryType::IT_NOTECARD:
			{

				// Update the UI with the new asset.
				LLPreviewNotecard* nc;
				nc = (LLPreviewNotecard*)LLPreview::find(new_item->getUUID());
				if(nc)
				{
					// *HACK: we have to delete the asset in the VFS so
					// that the viewer will redownload it. This is only
					// really necessary if the asset had to be modified by
					// the uploader, so this can be optimized away in some
					// cases. A better design is to have a new uuid if the
					// script actually changed the asset.
					if(nc->hasEmbeddedInventory())
					{
						gVFS->removeFile(
							content["new_asset"].asUUID(),
							LLAssetType::AT_NOTECARD);
					}

					nc->refreshFromInventory();
				}
			}
			break;
		case LLInventoryType::IT_LSL:
			{
				LLLiveLSLEditor* preview = LLLiveLSLEditor::find(item_id, task_id);
				if (preview)
				{
					// Bytecode save completed
					if (content["compiled"])
					{
						preview->callbackLSLCompileSucceeded(
							task_id,
							item_id,
							mPostData["is_script_running"]);
					}
					else
					{
						preview->callbackLSLCompileFailed(content["errors"]);
					}
				}
			}
			break;
		case LLInventoryType::IT_WEARABLE:
		default:
			break;
	}
}
