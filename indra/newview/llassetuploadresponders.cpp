/**
 * @file llassetuploadresponders.cpp
 * @brief Processes responses received for asset upload requests.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llassetuploadresponders.h"

#include "llagent.h"
#include "llcompilequeue.h"
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
#include "llpreviewgesture.h"
#include "llgesturemgr.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "lluploaddialog.h"
#include "llviewerobject.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "llviewermenufile.h"
#include "llviewerwindow.h"

// When uploading multiple files, don't display any of them when uploading more than this number.
static const S32 FILE_COUNT_DISPLAY_THRESHOLD = 5;

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
											   const std::string& file_name, 
											   LLAssetType::EType asset_type)
	: LLHTTPClient::Responder(),
	  mPostData(post_data),
	  mFileName(file_name),
	  mAssetType(asset_type)
{
}

LLAssetUploadResponder::~LLAssetUploadResponder()
{
	if (!mFileName.empty())
	{
		// Delete temp file
		LLFile::remove(mFileName);
	}
}

// virtual
void LLAssetUploadResponder::error(U32 statusNum, const std::string& reason)
{
	llinfos << "LLAssetUploadResponder::error " << statusNum 
			<< " reason: " << reason << llendl;
	LLSD args;
	switch(statusNum)
	{
		case 400:
			args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["REASON"] = "Error in upload request.  Please visit "
				"http://secondlife.com/support for help fixing this problem.";
			LLNotifications::instance().add("CannotUploadReason", args);
			break;
		case 500:
		default:
			args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
			args["REASON"] = "The server is experiencing unexpected "
				"difficulties.";
			LLNotifications::instance().add("CannotUploadReason", args);
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
		LLFloaterBuyCurrency::buyCurrency("Uploading costs", LLGlobalEconomy::Singleton::getInstance()->getPriceUpload());
	}
	else
	{
		LLSD args;
		args["FILE"] = (mFileName.empty() ? mVFileID.asString() : mFileName);
		args["REASON"] = content["message"].asString();
		LLNotifications::instance().add("CannotUploadReason", args);
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

LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(const LLSD& post_data, const std::string& file_name, LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, file_name, asset_type)
{
}

//virtual 
void LLNewAgentInventoryResponder::uploadComplete(const LLSD& content)
{
	lldebugs << "LLNewAgentInventoryResponder::result from capabilities" << llendl;
	
	//std::ostringstream llsdxml;
	//LLSDSerialize::toXML(content, llsdxml);
	//llinfos << "upload complete content:\n " << llsdxml.str() << llendl;

	LLAssetType::EType asset_type = LLAssetType::lookup(mPostData["asset_type"].asString());
	LLInventoryType::EType inventory_type = LLInventoryType::lookup(mPostData["inventory_type"].asString());
	S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();

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

		LLSD args;
		args["AMOUNT"] = llformat("%d", expected_upload_cost);
		LLNotifications::instance().add("UploadPayment", args);
	}

	// Actually add the upload to viewer inventory
	llinfos << "Adding " << content["new_inventory_item"].asUUID() << " "
			<< content["new_asset"].asUUID() << " to inventory." << llendl;
	if(mPostData["folder_id"].asUUID().notNull())
	{
		//std::ostringstream out;
		//LLSDXMLFormatter *formatter = new LLSDXMLFormatter;
		//formatter->format(mPostData, out, LLSDFormatter::OPTIONS_PRETTY);
		//llinfos << "Post Data: " << out.str() << llendl;

		U32 everyone_perms = PERM_NONE;
		U32 group_perms = PERM_NONE;
		U32 next_owner_perms = PERM_ALL;
		if(content.has("new_next_owner_mask"))
		{
			// This is a new sim that provides creation perms so use them.
			// Do not assume we got the perms we asked for in mPostData 
			// since the sim may not have granted them all.
			everyone_perms = content["new_everyone_mask"].asInteger();
			group_perms = content["new_group_mask"].asInteger();
			next_owner_perms = content["new_next_owner_mask"].asInteger();
		}
		else 
		{
			// This old sim doesn't provide creation perms so use old assumption-based perms.
			if(mPostData["inventory_type"].asString() != "snapshot")
			{
				next_owner_perms = PERM_MOVE | PERM_TRANSFER;
			}
		}
		LLPermissions new_perms;
		new_perms.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
		new_perms.initMasks(PERM_ALL, PERM_ALL, everyone_perms, group_perms, next_owner_perms);
		S32 creation_date_now = time_corrected();
		LLPointer<LLViewerInventoryItem> item
			= new LLViewerInventoryItem(content["new_inventory_item"].asUUID(),
										mPostData["folder_id"].asUUID(),
										new_perms,
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
			view->getPanel()->setSelection(content["new_inventory_item"].asUUID(), TAKE_FOCUS_NO);
			if((LLAssetType::AT_TEXTURE == asset_type || LLAssetType::AT_SOUND == asset_type)
				&& LLFilePicker::instance().getFileCount() <= FILE_COUNT_DISPLAY_THRESHOLD)
			{
				view->getPanel()->openSelected();
			}
			//LLInventoryView::dumpSelectionInformation((void*)view);
			// restore keyboard focus
			gFocusMgr.setKeyboardFocus(focus_ctrl);
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
	std::string next_file = LLFilePicker::instance().getNextFile();
	if(!next_file.empty())
	{
		std::string name = gDirUtilp->getBaseFileName(next_file, true);

		std::string asset_name = name;
		LLStringUtil::replaceNonstandardASCII( asset_name, '?' );
		LLStringUtil::replaceChar(asset_name, '|', '?');
		LLStringUtil::stripNonprintable(asset_name);
		LLStringUtil::trim(asset_name);

		// Continuing the horrible hack above, we need to extract the originally requested permissions data, if any,
		// and use them for each next file to be uploaded. Note the requested perms are not the same as the
		// granted ones found in the given "content" structure but can still be found in mPostData. -MG
		U32 everyone_perms   = mPostData.has("everyone_mask")   ? mPostData.get("everyone_mask"  ).asInteger() : PERM_NONE;
		U32 group_perms      = mPostData.has("group_mask")      ? mPostData.get("group_mask"     ).asInteger() : PERM_NONE;
		U32 next_owner_perms = mPostData.has("next_owner_mask") ? mPostData.get("next_owner_mask").asInteger() : PERM_NONE;
		std::string display_name = LLStringUtil::null;
		LLAssetStorage::LLStoreAssetCallback callback = NULL;
		void *userdata = NULL;
		upload_new_resource(next_file, asset_name, asset_name,
				    0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE,
				    next_owner_perms, group_perms,
				    everyone_perms, display_name,
				    callback, expected_upload_cost, userdata);
	}
}


LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(const LLSD& post_data,
																 const LLUUID& vfile_id,
																 LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, vfile_id, asset_type)
{
}

LLUpdateAgentInventoryResponder::LLUpdateAgentInventoryResponder(const LLSD& post_data,
																 const std::string& file_name,
																 LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, file_name, asset_type)
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

		case LLInventoryType::IT_GESTURE:
			{
				// If this gesture is active, then we need to update the in-memory
				// active map with the new pointer.				
				if (gGestureManager.isGestureActive(item_id))
				{
					LLUUID asset_id = new_item->getAssetUUID();
					gGestureManager.replaceGesture(item_id, asset_id);
					gInventory.notifyObservers();
				}				

				//gesture will have a new asset_id
				LLPreviewGesture* previewp = (LLPreviewGesture*)LLPreview::find(item_id);
				if(previewp)
				{
					previewp->onUpdateSucceeded();	
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
															   const std::string& file_name, 
															   LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, file_name, asset_type)
{
}

LLUpdateTaskInventoryResponder::LLUpdateTaskInventoryResponder(const LLSD& post_data,
															   const std::string& file_name,
															   const LLUUID& queue_id, 
															   LLAssetType::EType asset_type)
: LLAssetUploadResponder(post_data, file_name, asset_type), mQueueId(queue_id)
{
}

//virtual 
void LLUpdateTaskInventoryResponder::uploadComplete(const LLSD& content)
{
	llinfos << "LLUpdateTaskInventoryResponder::result from capabilities" << llendl;
	LLUUID item_id = mPostData["item_id"];
	LLUUID task_id = mPostData["task_id"];

	dialog_refresh_all();
	
	switch(mAssetType)
	{
		case LLAssetType::AT_NOTECARD:
			{

				// Update the UI with the new asset.
				LLPreviewNotecard* nc;
				nc = (LLPreviewNotecard*)LLPreview::find(item_id);
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
		case LLAssetType::AT_LSL_TEXT:
			{
				if(mQueueId.notNull())
				{
					LLFloaterCompileQueue* queue = 
						(LLFloaterCompileQueue*) LLFloaterScriptQueue::findInstance(mQueueId);
					if(NULL != queue)
					{
						queue->removeItemByItemID(item_id);
					}
				}
				else
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
			}
			break;
	default:
		break;
	}
}
