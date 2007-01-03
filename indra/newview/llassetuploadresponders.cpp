/** 
 * @file llmapresponders.h
 * @brief Processes responses received for asset upload requests.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
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
#include "lluploaddialog.h"
#include "llviewermenu.h"	// for upload_new_resource()
#include "llviewerwindow.h"
#include "viewer.h"

LLNewAgentInventoryResponder::LLNewAgentInventoryResponder(const LLUUID& uuid,
														   const LLSD &post_data)
	: LLHTTPClient::Responder()
{
	mUUID = uuid;
	mPostData = post_data;
}

// virtual
void LLNewAgentInventoryResponder::error(U32 statusNum, const std::string& reason)
{
	llinfos << "LLNewAgentInventoryResponder::error " << statusNum << llendl;
	LLStringBase<char>::format_map_t args;
	switch(statusNum)
	{
		case 400:
			args["[FILE]"] = mPostData["inventory_type"].asString();
			args["[REASON]"] = "invalid parameters in upload request";
			gViewerWindow->alertXml("CannotUploadReason", args);
			break;
		case 402:
			//(result["message"].asString() == "insufficient funds")
			LLFloaterBuyCurrency::buyCurrency("Uploading costs", gGlobalEconomy->getPriceUpload());
			break;
		case 500:
		default:
			args["[FILE]"] = mPostData["inventory_type"].asString();
			args["[REASON]"] = "the server is experiencing unexpected difficulties";
			gViewerWindow->alertXml("CannotUploadReason", args);
			break;
	}
	LLUploadDialog::modalUploadFinished();
}

//virtual 
void LLNewAgentInventoryResponder::result(const LLSD& result)
{
	lldebugs << "LLNewAgentInventoryResponder::result from capabilities" << llendl;

	if (!result["success"])
	{
		LLStringBase<char>::format_map_t args;
		args["[FILE]"] = mPostData["inventory_type"].asString();
		args["[REASON]"] = "the server is experiencing unexpected difficulties";
		gViewerWindow->alertXml("CannotUploadReason", args);
		return;
	}

	std::string uploader = result["uploader"];
	LLAssetType::EType asset_type = LLAssetType::lookup(mPostData["asset_type"].asString().c_str());
	LLInventoryType::EType inventory_type = LLInventoryType::lookup(mPostData["inventory_type"].asString().c_str());
	// request succeeded
	if (!uploader.empty())
	{
		LLHTTPClient::postFile(uploader, mUUID, asset_type, this);
	}
	// upload succeeded
	else
	{
		// rename the file in the VFS to the actual asset id
		gVFS->renameFile(mUUID, asset_type, result["new_asset"].asUUID(), asset_type);

		// TODO: only request for textures, sound, and animation uploads
		// Update money and ownership credit information
		// since it probably changed on the server
		if (mPostData["asset_type"].asString() == "texture" || 
			mPostData["asset_type"].asString() == "sound" ||
			mPostData["asset_type"].asString() == "animatn")
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
		llinfos << "Adding " << result["new_inventory_item"].asUUID() << " "
				<< result["new_asset"].asUUID() << " to inventory." << llendl;
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
				= new LLViewerInventoryItem(result["new_inventory_item"].asUUID(),
											mPostData["folder_id"].asUUID(),
											perm,
											result["new_asset"].asUUID(),
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

				view->getPanel()->setSelection(result["new_inventory_item"].asUUID(), TAKE_FOCUS_NO);
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
		
		// *NOTE: This is a pretty big hack. What this does is check
		// the file picker if there are any more pending uploads. If
		// so, upload that file.
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
				end_p = asset_name_str + strlen( asset_name_str );
			}
				
			S32 len = llmin( (S32) (DB_INV_ITEM_NAME_STR_LEN), (S32) (end_p - asset_name_str) );

			asset_name = asset_name.substr( 0, len );

			upload_new_resource(next_file, asset_name, asset_name,
								0, LLAssetType::AT_NONE, LLInventoryType::IT_NONE);
		}
	}
}
