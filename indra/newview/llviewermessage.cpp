/** 
 * @file llviewermessage.cpp
 * @brief Dumping ground for viewer-side message system callbacks.
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
#include "llviewermessage.h"
#include "boost/lexical_cast.hpp"

// Linden libraries
#include "llanimationstates.h"
#include "llaudioengine.h" 
#include "llavataractions.h"
#include "llavatarnamecache.h"		// IDEVO HACK
#include "lscript_byteformat.h"
#include "lleconomy.h"
#include "lleventtimer.h"
#include "llfloaterreg.h"
#include "llfolderview.h"
#include "llfollowcamparams.h"
#include "llinventorydefines.h"
#include "lllslconstants.h"
#include "llregionhandle.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llteleportflags.h"
#include "lltransactionflags.h"
#include "llvfile.h"
#include "llvfs.h"
#include "llxfermanager.h"
#include "mean_collision_data.h"

#include "llagent.h"
#include "llagentcamera.h"
#include "llcallingcard.h"
#include "llbuycurrencyhtml.h"
#include "llfirstuse.h"
#include "llfloaterbuyland.h"
#include "llfloaterland.h"
#include "llfloaterregioninfo.h"
#include "llfloaterlandholdings.h"
#include "llfloaterpreference.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloatersnapshot.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventorypanel.h"
#include "llnearbychat.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanelgrouplandmoney.h"
#include "llrecentpeople.h"
#include "llscriptfloater.h"
#include "llselectmgr.h"
#include "llstartup.h"
#include "llsky.h"
#include "llslurl.h"
#include "llstatenums.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "lltranslate.h"
#include "llviewerfoldertype.h"
#include "llvoavatar.h"				// IDEVO HACK
#include "lluri.h"
#include "llviewergenericmessage.h"
#include "llviewermenu.h"
#include "llviewerinventory.h"
#include "llviewerjoystick.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvlmanager.h"
#include "llvoavatarself.h"
#include "llworld.h"
#include "pipeline.h"
#include "llfloaterworldmap.h"
#include "llviewerdisplay.h"
#include "llkeythrottle.h"
#include "llgroupactions.h"
#include "llagentui.h"
#include "llpanelblockedlist.h"
#include "llpanelplaceprofile.h"
#include "llviewerregion.h"

#include <boost/algorithm/string/split.hpp> //
#include <boost/regex.hpp>

#include "llnotificationmanager.h" //

#if LL_MSVC
// disable boost::lexical_cast warning
#pragma warning (disable:4702)
#endif

//
// Constants
//
const F32 BIRD_AUDIBLE_RADIUS = 32.0f;
const F32 SIT_DISTANCE_FROM_TARGET = 0.25f;
const F32 CAMERA_POSITION_THRESHOLD_SQUARED = 0.001f * 0.001f;
static const F32 LOGOUT_REPLY_TIME = 3.f;	// Wait this long after LogoutReply before quitting.

// Determine how quickly residents' scripts can issue question dialogs
// Allow bursts of up to 5 dialogs in 10 seconds. 10*2=20 seconds recovery if throttle kicks in
static const U32 LLREQUEST_PERMISSION_THROTTLE_LIMIT	= 5;     // requests
static const F32 LLREQUEST_PERMISSION_THROTTLE_INTERVAL	= 10.0f; // seconds

extern BOOL gDebugClicks;

// function prototypes
bool check_offer_throttle(const std::string& from_name, bool check_only);
bool check_asset_previewable(const LLAssetType::EType asset_type);
static void process_money_balance_reply_extended(LLMessageSystem* msg);

//inventory offer throttle globals
LLFrameTimer gThrottleTimer;
const U32 OFFER_THROTTLE_MAX_COUNT=5; //number of items per time period
const F32 OFFER_THROTTLE_TIME=10.f; //time period in seconds

//script permissions
const std::string SCRIPT_QUESTIONS[SCRIPT_PERMISSION_EOF] = 
	{ 
		"ScriptTakeMoney",
		"ActOnControlInputs",
		"RemapControlInputs",
		"AnimateYourAvatar",
		"AttachToYourAvatar",
		"ReleaseOwnership",
		"LinkAndDelink",
		"AddAndRemoveJoints",
		"ChangePermissions",
		"TrackYourCamera",
		"ControlYourCamera",
		"TeleportYourAgent"
	};

const BOOL SCRIPT_QUESTION_IS_CAUTION[SCRIPT_PERMISSION_EOF] = 
{
	TRUE,	// ScriptTakeMoney,
	FALSE,	// ActOnControlInputs
	FALSE,	// RemapControlInputs
	FALSE,	// AnimateYourAvatar
	FALSE,	// AttachToYourAvatar
	FALSE,	// ReleaseOwnership,
	FALSE,	// LinkAndDelink,
	FALSE,	// AddAndRemoveJoints
	FALSE,	// ChangePermissions
	FALSE,	// TrackYourCamera,
	FALSE,	// ControlYourCamera
	FALSE	// TeleportYourAgent
};

bool friendship_offer_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLMessageSystem* msg = gMessageSystem;
	const LLSD& payload = notification["payload"];

	// add friend to recent people list
	LLRecentPeople::instance().add(payload["from_id"]);

	switch(option)
	{
	case 0:
	{
		// accept
		LLAvatarTracker::formFriendship(payload["from_id"]);

		const LLUUID fid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);

		// This will also trigger an onlinenotification if the user is online
		msg->newMessageFast(_PREHASH_AcceptFriendship);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, payload["session_id"]);
		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, fid);
		msg->sendReliable(LLHost(payload["sender"].asString()));

		LLSD payload = notification["payload"];
		payload["SUPPRESS_TOAST"] = true;
		LLNotificationsUtil::add("FriendshipAcceptedByMe",
				notification["substitutions"], payload);
		break;
	}
	case 1: // Decline
	{
		LLSD payload = notification["payload"];
		payload["SUPPRESS_TOAST"] = true;
		LLNotificationsUtil::add("FriendshipDeclinedByMe",
				notification["substitutions"], payload);
	}
	// fall-through
	case 2: // Send IM - decline and start IM session
		{
			// decline
			// We no longer notify other viewers, but we DO still send
			// the rejection to the simulator to delete the pending userop.
			msg->newMessageFast(_PREHASH_DeclineFriendship);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_TransactionBlock);
			msg->addUUIDFast(_PREHASH_TransactionID, payload["session_id"]);
			msg->sendReliable(LLHost(payload["sender"].asString()));

			// start IM session
			if(2 == option)
			{
				LLAvatarActions::startIM(payload["from_id"].asUUID());
			}
	}
	default:
		// close button probably, possibly timed out
		break;
	}

	return false;
}
static LLNotificationFunctorRegistration friendship_offer_callback_reg("OfferFriendship", friendship_offer_callback);
static LLNotificationFunctorRegistration friendship_offer_callback_reg_nm("OfferFriendshipNoMessage", friendship_offer_callback);

//const char BUSY_AUTO_RESPONSE[] =	"The Resident you messaged is in 'busy mode' which means they have "
//									"requested not to be disturbed. Your message will still be shown in their IM "
//									"panel for later viewing.";

//
// Functions
//

void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group,
				S32 trx_type, const std::string& desc)
{
	if(0 == amount || !region) return;
	amount = abs(amount);
	LL_INFOS("Messaging") << "give_money(" << uuid << "," << amount << ")"<< LL_ENDL;
	if(can_afford_transaction(amount))
	{
//		gStatusBar->debitBalance(amount);
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_MoneyTransferRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_MoneyData);
		msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_DestID, uuid);
		msg->addU8Fast(_PREHASH_Flags, pack_transaction_flags(FALSE, is_group));
		msg->addS32Fast(_PREHASH_Amount, amount);
		msg->addU8Fast(_PREHASH_AggregatePermNextOwner, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addU8Fast(_PREHASH_AggregatePermInventory, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addS32Fast(_PREHASH_TransactionType, trx_type );
		msg->addStringFast(_PREHASH_Description, desc);
		msg->sendReliable(region->getHost());
	}
	else
	{
		LLStringUtil::format_map_t args;
		args["AMOUNT"] = llformat("%d", amount);
		LLBuyCurrencyHTML::openCurrencyFloater( LLTrans::getString("giving", args), amount );
	}
}

void send_complete_agent_movement(const LLHost& sim_host)
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_CompleteAgentMovement);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addU32Fast(_PREHASH_CircuitCode, msg->mOurCircuitCode);
	msg->sendReliable(sim_host);
}

void process_logout_reply(LLMessageSystem* msg, void**)
{
	// The server has told us it's ok to quit.
	LL_DEBUGS("Messaging") << "process_logout_reply" << LL_ENDL;

	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	LLUUID session_id;
	msg->getUUID("AgentData", "SessionID", session_id);
	if((agent_id != gAgent.getID()) || (session_id != gAgent.getSessionID()))
	{
		LL_WARNS("Messaging") << "Bogus Logout Reply" << LL_ENDL;
	}

	LLInventoryModel::update_map_t parents;
	S32 count = msg->getNumberOfBlocksFast( _PREHASH_InventoryData );
	for(S32 i = 0; i < count; ++i)
	{
		LLUUID item_id;
		msg->getUUIDFast(_PREHASH_InventoryData, _PREHASH_ItemID, item_id, i);

		if( (1 == count) && item_id.isNull() )
		{
			// Detect dummy item.  Indicates an empty list.
			break;
		}

		// We do not need to track the asset ids, just account for an
		// updated inventory version.
		LL_INFOS("Messaging") << "process_logout_reply itemID=" << item_id << LL_ENDL;
		LLInventoryItem* item = gInventory.getItem( item_id );
		if( item )
		{
			parents[item->getParentUUID()] = 0;
			gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		}
		else
		{
			LL_INFOS("Messaging") << "process_logout_reply item not found: " << item_id << LL_ENDL;
		}
	}
    LLAppViewer::instance()->forceQuit();
}

void process_layer_data(LLMessageSystem *mesgsys, void **user_data)
{
	LLViewerRegion *regionp = LLWorld::getInstance()->getRegion(mesgsys->getSender());

	if(!regionp)
	{
		llwarns << "Invalid region for layer data." << llendl;
		return;
	}
	S32 size;
	S8 type;

	mesgsys->getS8Fast(_PREHASH_LayerID, _PREHASH_Type, type);
	size = mesgsys->getSizeFast(_PREHASH_LayerData, _PREHASH_Data);
	if (0 == size)
	{
		LL_WARNS("Messaging") << "Layer data has zero size." << LL_ENDL;
		return;
	}
	if (size < 0)
	{
		// getSizeFast() is probably trying to tell us about an error
		LL_WARNS("Messaging") << "getSizeFast() returned negative result: "
			<< size
			<< LL_ENDL;
		return;
	}
	U8 *datap = new U8[size];
	mesgsys->getBinaryDataFast(_PREHASH_LayerData, _PREHASH_Data, datap, size);
	LLVLData *vl_datap = new LLVLData(regionp, type, datap, size);
	if (mesgsys->getReceiveCompressedSize())
	{
		gVLManager.addLayerData(vl_datap, mesgsys->getReceiveCompressedSize());
	}
	else
	{
		gVLManager.addLayerData(vl_datap, mesgsys->getReceiveSize());
	}
}

// S32 exported_object_count = 0;
// S32 exported_image_count = 0;
// S32 current_object_count = 0;
// S32 current_image_count = 0;

// extern LLNotifyBox *gExporterNotify;
// extern LLUUID gExporterRequestID;
// extern std::string gExportDirectory;

// extern LLUploadDialog *gExportDialog;

// std::string gExportedFile;

// std::map<LLUUID, std::string> gImageChecksums;

// void export_complete()
// {
// 		LLUploadDialog::modalUploadFinished();
// 		gExporterRequestID.setNull();
// 		gExportDirectory = "";

// 		LLFILE* fXML = LLFile::fopen(gExportedFile, "rb");		/* Flawfinder: ignore */
// 		fseek(fXML, 0, SEEK_END);
// 		long length = ftell(fXML);
// 		fseek(fXML, 0, SEEK_SET);
// 		U8 *buffer = new U8[length + 1];
// 		size_t nread = fread(buffer, 1, length, fXML);
// 		if (nread < (size_t) length)
// 		{
// 			LL_WARNS("Messaging") << "Short read" << LL_ENDL;
// 		}
// 		buffer[nread] = '\0';
// 		fclose(fXML);

// 		char *pos = (char *)buffer;
// 		while ((pos = strstr(pos+1, "<sl:image ")) != 0)
// 		{
// 			char *pos_check = strstr(pos, "checksum=\"");

// 			if (pos_check)
// 			{
// 				char *pos_uuid = strstr(pos_check, "\">");

// 				if (pos_uuid)
// 				{
// 					char image_uuid_str[UUID_STR_SIZE];		/* Flawfinder: ignore */
// 					memcpy(image_uuid_str, pos_uuid+2, UUID_STR_SIZE-1);		/* Flawfinder: ignore */
// 					image_uuid_str[UUID_STR_SIZE-1] = 0;
					
// 					LLUUID image_uuid(image_uuid_str);

// 					LL_INFOS("Messaging") << "Found UUID: " << image_uuid << LL_ENDL;

// 					std::map<LLUUID, std::string>::iterator itor = gImageChecksums.find(image_uuid);
// 					if (itor != gImageChecksums.end())
// 					{
// 						LL_INFOS("Messaging") << "Replacing with checksum: " << itor->second << LL_ENDL;
// 						if (!itor->second.empty())
// 						{
// 							memcpy(&pos_check[10], itor->second.c_str(), 32);		/* Flawfinder: ignore */
// 						}
// 					}
// 				}
// 			}
// 		}

// 		LLFILE* fXMLOut = LLFile::fopen(gExportedFile, "wb");		/* Flawfinder: ignore */
// 		if (fwrite(buffer, 1, length, fXMLOut) != length)
// 		{
// 			LL_WARNS("Messaging") << "Short write" << LL_ENDL;
// 		}
// 		fclose(fXMLOut);

// 		delete [] buffer;
// }


// void exported_item_complete(const LLTSCode status, void *user_data)
// {
// 	//std::string *filename = (std::string *)user_data;

// 	if (status < LLTS_OK)
// 	{
// 		LL_WARNS("Messaging") << "Export failed!" << LL_ENDL;
// 	}
// 	else
// 	{
// 		++current_object_count;
// 		if (current_image_count == exported_image_count && current_object_count == exported_object_count)
// 		{
// 			LL_INFOS("Messaging") << "*** Export complete ***" << LL_ENDL;

// 			export_complete();
// 		}
// 		else
// 		{
// 			gExportDialog->setMessage(llformat("Exported %d/%d object files, %d/%d textures.", current_object_count, exported_object_count, current_image_count, exported_image_count));
// 		}
// 	}
// }

// struct exported_image_info
// {
// 	LLUUID image_id;
// 	std::string filename;
// 	U32 image_num;
// };

// void exported_j2c_complete(const LLTSCode status, void *user_data)
// {
// 	exported_image_info *info = (exported_image_info *)user_data;
// 	LLUUID image_id = info->image_id;
// 	U32 image_num = info->image_num;
// 	std::string filename = info->filename;
// 	delete info;

// 	if (status < LLTS_OK)
// 	{
// 		LL_WARNS("Messaging") << "Image download failed!" << LL_ENDL;
// 	}
// 	else
// 	{
// 		LLFILE* fIn = LLFile::fopen(filename, "rb");		/* Flawfinder: ignore */
// 		if (fIn) 
// 		{
// 			LLPointer<LLImageJ2C> ImageUtility = new LLImageJ2C;
// 			LLPointer<LLImageTGA> TargaUtility = new LLImageTGA;

// 			fseek(fIn, 0, SEEK_END);
// 			S32 length = ftell(fIn);
// 			fseek(fIn, 0, SEEK_SET);
// 			U8 *buffer = ImageUtility->allocateData(length);
// 			if (fread(buffer, 1, length, fIn) != length)
// 			{
// 				LL_WARNS("Messaging") << "Short read" << LL_ENDL;
// 			}
// 			fclose(fIn);
// 			LLFile::remove(filename);

// 			// Convert to TGA
// 			LLPointer<LLImageRaw> image = new LLImageRaw();

// 			ImageUtility->updateData();
// 			ImageUtility->decode(image, 100000.0f);
			
// 			TargaUtility->encode(image);
// 			U8 *data = TargaUtility->getData();
// 			S32 data_size = TargaUtility->getDataSize();

// 			std::string file_path = gDirUtilp->getDirName(filename);
			
// 			std::string output_file = llformat("%s/image-%03d.tga", file_path.c_str(), image_num);//filename;
// 			//S32 name_len = output_file.length();
// 			//strcpy(&output_file[name_len-3], "tga");
// 			LLFILE* fOut = LLFile::fopen(output_file, "wb");		/* Flawfinder: ignore */
// 			char md5_hash_string[33];		/* Flawfinder: ignore */
// 			strcpy(md5_hash_string, "00000000000000000000000000000000");		/* Flawfinder: ignore */
// 			if (fOut)
// 			{
// 				if (fwrite(data, 1, data_size, fOut) != data_size)
// 				{
// 					LL_WARNS("Messaging") << "Short write" << LL_ENDL;
// 				}
// 				fseek(fOut, 0, SEEK_SET);
// 				fclose(fOut);
// 				fOut = LLFile::fopen(output_file, "rb");		/* Flawfinder: ignore */
// 				LLMD5 my_md5_hash(fOut);
// 				my_md5_hash.hex_digest(md5_hash_string);
// 			}

// 			gImageChecksums.insert(std::pair<LLUUID, std::string>(image_id, md5_hash_string));
// 		}
// 	}

// 	++current_image_count;
// 	if (current_image_count == exported_image_count && current_object_count == exported_object_count)
// 	{
// 		LL_INFOS("Messaging") << "*** Export textures complete ***" << LL_ENDL;
// 			export_complete();
// 	}
// 	else
// 	{
// 		gExportDialog->setMessage(llformat("Exported %d/%d object files, %d/%d textures.", current_object_count, exported_object_count, current_image_count, exported_image_count));
// 	}
//}

void process_derez_ack(LLMessageSystem*, void**)
{
	if(gViewerWindow) gViewerWindow->getWindow()->decBusyCount();
}

void process_places_reply(LLMessageSystem* msg, void** data)
{
	LLUUID query_id;

	msg->getUUID("AgentData", "QueryID", query_id);
	if (query_id.isNull())
	{
		LLFloaterLandHoldings::processPlacesReply(msg, data);
	}
	else if(gAgent.isInGroup(query_id))
	{
		LLPanelGroupLandMoney::processPlacesReply(msg, data);
	}
	else
	{
		LL_WARNS("Messaging") << "Got invalid PlacesReply message" << LL_ENDL;
	}
}

void send_sound_trigger(const LLUUID& sound_id, F32 gain)
{
	if (sound_id.isNull() || gAgent.getRegion() == NULL)
	{
		// disconnected agent or zero guids don't get sent (no sound)
		return;
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_SoundTrigger);
	msg->nextBlockFast(_PREHASH_SoundData);
	msg->addUUIDFast(_PREHASH_SoundID, sound_id);
	// Client untrusted, ids set on sim
	msg->addUUIDFast(_PREHASH_OwnerID, LLUUID::null );
	msg->addUUIDFast(_PREHASH_ObjectID, LLUUID::null );
	msg->addUUIDFast(_PREHASH_ParentID, LLUUID::null );

	msg->addU64Fast(_PREHASH_Handle, gAgent.getRegion()->getHandle());

	LLVector3 position = gAgent.getPositionAgent();
	msg->addVector3Fast(_PREHASH_Position, position);
	msg->addF32Fast(_PREHASH_Gain, gain);

	gAgent.sendMessage();
}

bool join_group_response(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	BOOL delete_context_data = TRUE;
	bool accept_invite = false;

	LLUUID group_id = notification["payload"]["group_id"].asUUID();
	LLUUID transaction_id = notification["payload"]["transaction_id"].asUUID();
	std::string name = notification["payload"]["name"].asString();
	std::string message = notification["payload"]["message"].asString();
	S32 fee = notification["payload"]["fee"].asInteger();

	if (option == 2 && !group_id.isNull())
	{
		LLGroupActions::show(group_id);
		LLSD args;
		args["MESSAGE"] = message;
		LLNotificationsUtil::add("JoinGroup", args, notification["payload"]);
		return false;
	}
	if(option == 0 && !group_id.isNull())
	{
		// check for promotion or demotion.
		S32 max_groups = gMaxAgentGroups;
		if(gAgent.isInGroup(group_id)) ++max_groups;

		if(gAgent.mGroups.count() < max_groups)
		{
			accept_invite = true;
		}
		else
		{
			delete_context_data = FALSE;
			LLSD args;
			args["NAME"] = name;
			LLNotificationsUtil::add("JoinedTooManyGroupsMember", args, notification["payload"]);
		}
	}

	if (accept_invite)
	{
		// If there is a fee to join this group, make
		// sure the user is sure they want to join.
		if (fee > 0)
		{
			delete_context_data = FALSE;
			LLSD args;
			args["COST"] = llformat("%d", fee);
			// Set the fee for next time to 0, so that we don't keep
			// asking about a fee.
			LLSD next_payload = notification["payload"];
			next_payload["fee"] = 0;
			LLNotificationsUtil::add("JoinGroupCanAfford",
									args,
									next_payload);
		}
		else
		{
			send_improved_im(group_id,
							 std::string("name"),
							 std::string("message"),
							IM_ONLINE,
							IM_GROUP_INVITATION_ACCEPT,
							transaction_id);
		}
	}
	else
	{
		send_improved_im(group_id,
						 std::string("name"),
						 std::string("message"),
						IM_ONLINE,
						IM_GROUP_INVITATION_DECLINE,
						transaction_id);
	}

	return false;
}

static void highlight_inventory_objects_in_panel(const std::vector<LLUUID>& items, LLInventoryPanel *inventory_panel)
{
	if (NULL == inventory_panel) return;

	for (std::vector<LLUUID>::const_iterator item_iter = items.begin();
		item_iter != items.end();
		++item_iter)
	{
		const LLUUID& item_id = (*item_iter);
		if(!highlight_offered_object(item_id))
		{
			continue;
		}

		LLInventoryObject* item = gInventory.getObject(item_id);
		llassert(item);
		if (!item) {
			continue;
		}

		LL_DEBUGS("Inventory_Move") << "Highlighting inventory item: " << item->getName() << ", " << item_id  << LL_ENDL;
		LLFolderView* fv = inventory_panel->getRootFolder();
		if (fv)
		{
			LLFolderViewItem* fv_item = fv->getItemByID(item_id);
			if (fv_item)
			{
				LLFolderViewItem* fv_folder = fv_item->getParentFolder();
				if (fv_folder)
				{
					// Parent folders can be different in case of 2 consecutive drag and drop
					// operations when the second one is started before the first one completes.
					LL_DEBUGS("Inventory_Move") << "Open folder: " << fv_folder->getName() << LL_ENDL;
					fv_folder->setOpen(TRUE);
					if (fv_folder->isSelected())
					{
						fv->changeSelection(fv_folder, FALSE);
					}
				}
				fv->changeSelection(fv_item, TRUE);
			}
		}
	}
}

static LLNotificationFunctorRegistration jgr_1("JoinGroup", join_group_response);
static LLNotificationFunctorRegistration jgr_2("JoinedTooManyGroupsMember", join_group_response);
static LLNotificationFunctorRegistration jgr_3("JoinGroupCanAfford", join_group_response);


//-----------------------------------------------------------------------------
// Instant Message
//-----------------------------------------------------------------------------
class LLOpenAgentOffer : public LLInventoryFetchItemsObserver
{
public:
	LLOpenAgentOffer(const LLUUID& object_id,
					 const std::string& from_name) : 
		LLInventoryFetchItemsObserver(object_id),
		mFromName(from_name) {}
	/*virtual*/ void startFetch()
	{
		for (uuid_vec_t::const_iterator it = mIDs.begin(); it < mIDs.end(); ++it)
		{
			LLViewerInventoryCategory* cat = gInventory.getCategory(*it);
			if (cat)
			{
				mComplete.push_back((*it));
			}
		}
		LLInventoryFetchItemsObserver::startFetch();
	}
	/*virtual*/ void done()
	{
		open_inventory_offer(mComplete, mFromName);
		gInventory.removeObserver(this);
		delete this;
	}
private:
	std::string mFromName;
};

/**
 * Class to observe adding of new items moved from the world to user's inventory to select them in inventory.
 *
 * We can't create it each time items are moved because "drop" event is sent separately for each
 * element even while multi-dragging. We have to have the only instance of the observer. See EXT-4347.
 */
class LLViewerInventoryMoveFromWorldObserver : public LLInventoryAddItemByAssetObserver
{
public:
	LLViewerInventoryMoveFromWorldObserver()
		: LLInventoryAddItemByAssetObserver()
	{

	}

	void setMoveIntoFolderID(const LLUUID& into_folder_uuid) {mMoveIntoFolderID = into_folder_uuid; }

private:
	/*virtual */void onAssetAdded(const LLUUID& asset_id)
	{
		// Store active Inventory panel.
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mActivePanel = LLInventoryPanel::getActiveInventoryPanel()->getHandle();
		}

		// Store selected items (without destination folder)
		mSelectedItems.clear();
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mSelectedItems = LLInventoryPanel::getActiveInventoryPanel()->getRootFolder()->getSelectionList();
		}
		mSelectedItems.erase(mMoveIntoFolderID);
	}

	/**
	 * Selects added inventory items watched by their Asset UUIDs if selection was not changed since
	 * all items were started to watch (dropped into a folder).
	 */
	void done()
	{
		LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());

		// if selection is not changed since watch started lets hightlight new items.
		if (active_panel && !isSelectionChanged())
		{
			LL_DEBUGS("Inventory_Move") << "Selecting new items..." << LL_ENDL;
			active_panel->clearSelection();
			highlight_inventory_objects_in_panel(mAddedItems, active_panel);
		}
	}

	/**
	 * Returns true if selected inventory items were changed since moved inventory items were started to watch.
	 */
	bool isSelectionChanged()
	{	
		LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());

		if (NULL == active_panel)
		{
			return true;
		}

		// get selected items (without destination folder)
		selected_items_t selected_items = active_panel->getRootFolder()->getSelectionList();
		selected_items.erase(mMoveIntoFolderID);

		// compare stored & current sets of selected items
		selected_items_t different_items;
		std::set_symmetric_difference(mSelectedItems.begin(), mSelectedItems.end(),
			selected_items.begin(), selected_items.end(), std::inserter(different_items, different_items.begin()));

		LL_DEBUGS("Inventory_Move") << "Selected firstly: " << mSelectedItems.size()
			<< ", now: " << selected_items.size() << ", difference: " << different_items.size() << LL_ENDL;

		return different_items.size() > 0;
	}

	LLHandle<LLPanel> mActivePanel;
	typedef std::set<LLUUID> selected_items_t;
	selected_items_t mSelectedItems;

	/**
	 * UUID of FolderViewFolder into which watched items are moved.
	 *
	 * Destination FolderViewFolder becomes selected while mouse hovering (when dragged items are dropped).
	 * 
	 * If mouse is moved out it set unselected and number of selected items is changed 
	 * even if selected items in Inventory stay the same.
	 * So, it is used to update stored selection list.
	 *
	 * @see onAssetAdded()
	 * @see isSelectionChanged()
	 */
	LLUUID mMoveIntoFolderID;
};

LLViewerInventoryMoveFromWorldObserver* gInventoryMoveObserver = NULL;

void set_dad_inventory_item(LLInventoryItem* inv_item, const LLUUID& into_folder_uuid)
{
	start_new_inventory_observer();

	gInventoryMoveObserver->setMoveIntoFolderID(into_folder_uuid);
	gInventoryMoveObserver->watchAsset(inv_item->getAssetUUID());
}


/**
 * Class to observe moving of items and to select them in inventory.
 *
 * Used currently for dragging from inbox to regular inventory folders
 */

class LLViewerInventoryMoveObserver : public LLInventoryObserver
{
public:

	LLViewerInventoryMoveObserver(const LLUUID& object_id)
		: LLInventoryObserver()
		, mObjectID(object_id)
	{
		if (LLInventoryPanel::getActiveInventoryPanel())
		{
			mActivePanel = LLInventoryPanel::getActiveInventoryPanel()->getHandle();
		}
	}

	virtual ~LLViewerInventoryMoveObserver() {}
	virtual void changed(U32 mask);
	
private:
	LLUUID mObjectID;
	LLHandle<LLPanel> mActivePanel;

};

void LLViewerInventoryMoveObserver::changed(U32 mask)
{
	LLInventoryPanel* active_panel = dynamic_cast<LLInventoryPanel*>(mActivePanel.get());

	if (NULL == active_panel)
	{
		gInventory.removeObserver(this);
		return;
	}

	if((mask & (LLInventoryObserver::STRUCTURE)) != 0)
	{
		const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

		std::set<LLUUID>::const_iterator id_it = changed_items.begin();
		std::set<LLUUID>::const_iterator id_end = changed_items.end();
		for (;id_it != id_end; ++id_it)
		{
			if ((*id_it) == mObjectID)
			{
				active_panel->clearSelection();			
				std::vector<LLUUID> items;
				items.push_back(mObjectID);
				highlight_inventory_objects_in_panel(items, active_panel);
				active_panel->getRootFolder()->scrollToShowSelection();
				
				gInventory.removeObserver(this);
				break;
			}
		}
	}
}

void set_dad_inbox_object(const LLUUID& object_id)
{
	LLViewerInventoryMoveObserver* move_observer = new LLViewerInventoryMoveObserver(object_id);
	gInventory.addObserver(move_observer);
}

//unlike the FetchObserver for AgentOffer, we only make one 
//instance of the AddedObserver for TaskOffers
//and it never dies.  We do this because we don't know the UUID of 
//task offers until they are accepted, so we don't wouldn't 
//know what to watch for, so instead we just watch for all additions.
class LLOpenTaskOffer : public LLInventoryAddedObserver
{
protected:
	/*virtual*/ void done()
	{
		for (uuid_vec_t::iterator it = mAdded.begin(); it != mAdded.end();)
		{
			const LLUUID& item_uuid = *it;
			bool was_moved = false;
			LLInventoryObject* added_object = gInventory.getObject(item_uuid);
			if (added_object)
			{
				// cast to item to get Asset UUID
				LLInventoryItem* added_item = dynamic_cast<LLInventoryItem*>(added_object);
				if (added_item)
				{
					const LLUUID& asset_uuid = added_item->getAssetUUID();
					if (gInventoryMoveObserver->isAssetWatched(asset_uuid))
					{
						LL_DEBUGS("Inventory_Move") << "Found asset UUID: " << asset_uuid << LL_ENDL;
						was_moved = true;
					}
				}
			}

			if (was_moved)
			{
				it = mAdded.erase(it);
			}
			else ++it;
		}

		open_inventory_offer(mAdded, "");
		mAdded.clear();
	}
 };

class LLOpenTaskGroupOffer : public LLInventoryAddedObserver
{
protected:
	/*virtual*/ void done()
	{
		open_inventory_offer(mAdded, "group_offer");
		mAdded.clear();
		gInventory.removeObserver(this);
		delete this;
	}
};

//one global instance to bind them
LLOpenTaskOffer* gNewInventoryObserver=NULL;
class LLNewInventoryHintObserver : public LLInventoryAddedObserver
{
protected:
	/*virtual*/ void done()
	{
		LLFirstUse::newInventory();
	}
};

LLNewInventoryHintObserver* gNewInventoryHintObserver=NULL;

void start_new_inventory_observer()
{
	if (!gNewInventoryObserver) //task offer observer 
	{
		// Observer is deleted by gInventory
		gNewInventoryObserver = new LLOpenTaskOffer;
		gInventory.addObserver(gNewInventoryObserver);
	}

	if (!gInventoryMoveObserver) //inventory move from the world observer 
	{
		// Observer is deleted by gInventory
		gInventoryMoveObserver = new LLViewerInventoryMoveFromWorldObserver;
		gInventory.addObserver(gInventoryMoveObserver);
	}

	if (!gNewInventoryHintObserver)
	{
		// Observer is deleted by gInventory
		gNewInventoryHintObserver = new LLNewInventoryHintObserver();
		gInventory.addObserver(gNewInventoryHintObserver);
	}
}

class LLDiscardAgentOffer : public LLInventoryFetchItemsObserver
{
	LOG_CLASS(LLDiscardAgentOffer);

public:
	LLDiscardAgentOffer(const LLUUID& folder_id, const LLUUID& object_id) :
		LLInventoryFetchItemsObserver(object_id),
		mFolderID(folder_id),
		mObjectID(object_id) {}

	virtual void done()
	{
		LL_DEBUGS("Messaging") << "LLDiscardAgentOffer::done()" << LL_ENDL;

		// We're invoked from LLInventoryModel::notifyObservers().
		// If we now try to remove the inventory item, it will cause a nested
		// notifyObservers() call, which won't work.
		// So defer moving the item to trash until viewer gets idle (in a moment).
		// Use removeObject() rather than removeItem() because at this level,
		// the object could be either an item or a folder.
		LLAppViewer::instance()->addOnIdleCallback(boost::bind(&LLInventoryModel::removeObject, &gInventory, mObjectID));
		gInventory.removeObserver(this);
		delete this;
	}

protected:
	LLUUID mFolderID;
	LLUUID mObjectID;
};


//Returns TRUE if we are OK, FALSE if we are throttled
//Set check_only true if you want to know the throttle status 
//without registering a hit
bool check_offer_throttle(const std::string& from_name, bool check_only)
{
	static U32 throttle_count;
	static bool throttle_logged;
	LLChat chat;
	std::string log_message;

	if (!gSavedSettings.getBOOL("ShowNewInventory"))
		return false;

	if (check_only)
	{
		return gThrottleTimer.hasExpired();
	}
	
	if(gThrottleTimer.checkExpirationAndReset(OFFER_THROTTLE_TIME))
	{
		LL_DEBUGS("Messaging") << "Throttle Expired" << LL_ENDL;
		throttle_count=1;
		throttle_logged=false;
		return true;
	}
	else //has not expired
	{
		LL_DEBUGS("Messaging") << "Throttle Not Expired, Count: " << throttle_count << LL_ENDL;
		// When downloading the initial inventory we get a lot of new items
		// coming in and can't tell that from spam.
		if (LLStartUp::getStartupState() >= STATE_STARTED
			&& throttle_count >= OFFER_THROTTLE_MAX_COUNT)
		{
			if (!throttle_logged)
			{
				// Use the name of the last item giver, who is probably the person
				// spamming you.

				LLStringUtil::format_map_t arg;
				std::string log_msg;
				std::ostringstream time ;
				time<<OFFER_THROTTLE_TIME;

				arg["APP_NAME"] = LLAppViewer::instance()->getSecondLifeTitle();
				arg["TIME"] = time.str();

				if (!from_name.empty())
				{
					arg["FROM_NAME"] = from_name;
					log_msg = LLTrans::getString("ItemsComingInTooFastFrom", arg);
				}
				else
				{
					log_msg = LLTrans::getString("ItemsComingInTooFast", arg);
				}
				
				//this is kinda important, so actually put it on screen
				LLSD args;
				args["MESSAGE"] = log_msg;
				LLNotificationsUtil::add("SystemMessage", args);

				throttle_logged=true;
			}
			return false;
		}
		else
		{
			throttle_count++;
			return true;
		}
	}
}

// Return "true" if we have a preview method for that asset type, "false" otherwise
bool check_asset_previewable(const LLAssetType::EType asset_type)
{
	return	(asset_type == LLAssetType::AT_NOTECARD)  || 
			(asset_type == LLAssetType::AT_LANDMARK)  ||
			(asset_type == LLAssetType::AT_TEXTURE)   ||
			(asset_type == LLAssetType::AT_ANIMATION) ||
			(asset_type == LLAssetType::AT_SCRIPT)    ||
			(asset_type == LLAssetType::AT_SOUND);
}

void open_inventory_offer(const uuid_vec_t& objects, const std::string& from_name)
{
	for (uuid_vec_t::const_iterator obj_iter = objects.begin();
		 obj_iter != objects.end();
		 ++obj_iter)
	{
		const LLUUID& obj_id = (*obj_iter);
		if(!highlight_offered_object(obj_id))
		{
			continue;
		}

		const LLInventoryObject *obj = gInventory.getObject(obj_id);
		if (!obj)
		{
			llwarns << "Cannot find object [ itemID:" << obj_id << " ] to open." << llendl;
			continue;
		}

		const LLAssetType::EType asset_type = obj->getActualType();

		// Either an inventory item or a category.
		const LLInventoryItem* item = dynamic_cast<const LLInventoryItem*>(obj);
		if (item && check_asset_previewable(asset_type))
		{
			////////////////////////////////////////////////////////////////////////////////
			// Special handling for various types.
			if (check_offer_throttle(from_name, false)) // If we are throttled, don't display
			{
				LL_DEBUGS("Messaging") << "Highlighting inventory item: " << item->getUUID()  << LL_ENDL;
				// If we opened this ourselves, focus it
				const BOOL take_focus = from_name.empty() ? TAKE_FOCUS_YES : TAKE_FOCUS_NO;
				switch(asset_type)
				{
					case LLAssetType::AT_NOTECARD:
					{
						LLFloaterReg::showInstance("preview_notecard", LLSD(obj_id), take_focus);
						break;
					}
					case LLAssetType::AT_LANDMARK:
					{
						LLInventoryCategory* parent_folder = gInventory.getCategory(item->getParentUUID());
						if ("inventory_handler" == from_name)
						{
							LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "landmark").with("id", item->getUUID()));
						}
						else if("group_offer" == from_name)
						{
							// "group_offer" is passed by LLOpenTaskGroupOffer
							// Notification about added landmark will be generated under the "from_name.empty()" called from LLOpenTaskOffer::done().
							LLSD args;
							args["type"] = "landmark";
							args["id"] = obj_id;
							LLFloaterSidePanelContainer::showPanel("places", args);

							continue;
						}
						else if(from_name.empty())
						{
							std::string folder_name;
							if (parent_folder)
							{
								// Localize folder name.
								// *TODO: share this code?
								folder_name = parent_folder->getName();
								if (LLFolderType::lookupIsProtectedType(parent_folder->getPreferredType()))
								{
									LLTrans::findString(folder_name, "InvFolder " + folder_name);
								}
							}
							else
							{
								 folder_name = LLTrans::getString("Unknown");
							}

							// we receive a message from LLOpenTaskOffer, it mean that new landmark has been added.
							LLSD args;
							args["LANDMARK_NAME"] = item->getName();
							args["FOLDER_NAME"] = folder_name;
							LLNotificationsUtil::add("LandmarkCreated", args);
						}
					}
					break;
					case LLAssetType::AT_TEXTURE:
					{
						LLFloaterReg::showInstance("preview_texture", LLSD(obj_id), take_focus);
						break;
					}
					case LLAssetType::AT_ANIMATION:
						LLFloaterReg::showInstance("preview_anim", LLSD(obj_id), take_focus);
						break;
					case LLAssetType::AT_SCRIPT:
						LLFloaterReg::showInstance("preview_script", LLSD(obj_id), take_focus);
						break;
					case LLAssetType::AT_SOUND:
						LLFloaterReg::showInstance("preview_sound", LLSD(obj_id), take_focus);
						break;
					default:
						LL_DEBUGS("Messaging") << "No preview method for previewable asset type : " << LLAssetType::lookupHumanReadable(asset_type)  << LL_ENDL;
						break;
				}
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		// Highlight item
		const BOOL auto_open = 
			gSavedSettings.getBOOL("ShowInInventory") && // don't open if showininventory is false
			!from_name.empty(); // don't open if it's not from anyone.
		LLInventoryPanel::openInventoryPanelAndSetSelection(auto_open, obj_id);
	}
}

bool highlight_offered_object(const LLUUID& obj_id)
{
	const LLInventoryObject* obj = gInventory.getObject(obj_id);
	if(!obj)
	{
		LL_WARNS("Messaging") << "Unable to show inventory item: " << obj_id << LL_ENDL;
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	// Don't highlight if it's in certain "quiet" folders which don't need UI
	// notification (e.g. trash, cof, lost-and-found).
	if(!gAgent.getAFK())
	{
		const LLViewerInventoryCategory *parent = gInventory.getFirstNondefaultParent(obj_id);
		if (parent)
		{
			const LLFolderType::EType parent_type = parent->getPreferredType();
			if (LLViewerFolderType::lookupIsQuietType(parent_type))
			{
				return false;
			}
		}
	}

	return true;
}

void inventory_offer_mute_callback(const LLUUID& blocked_id,
								   const std::string& full_name,
								   bool is_group)
{
	// *NOTE: blocks owner if the offer came from an object
	LLMute::EType mute_type = is_group ? LLMute::GROUP : LLMute::AGENT;

	LLMute mute(blocked_id, full_name, mute_type);
	if (LLMuteList::getInstance()->add(mute))
	{
		LLPanelBlockedList::showPanelAndSelect(blocked_id);
	}

	// purge the message queue of any previously queued inventory offers from the same source.
	class OfferMatcher : public LLNotificationsUI::LLScreenChannel::Matcher
	{
	public:
		OfferMatcher(const LLUUID& to_block) : blocked_id(to_block) {}
		bool matches(const LLNotificationPtr notification) const
		{
			if(notification->getName() == "ObjectGiveItem" 
				|| notification->getName() == "OwnObjectGiveItem"
				|| notification->getName() == "UserGiveItem")
			{
				return (notification->getPayload()["from_id"].asUUID() == blocked_id);
			}
			return FALSE;
		}
	private:
		const LLUUID& blocked_id;
	};

	LLNotificationsUI::LLChannelManager::getInstance()->killToastsFromChannel(LLUUID(
			gSavedSettings.getString("NotificationChannelUUID")), OfferMatcher(blocked_id));
}

LLOfferInfo::LLOfferInfo()
 : LLNotificationResponderInterface()
 , mFromGroup(FALSE)
 , mFromObject(FALSE)
 , mIM(IM_NOTHING_SPECIAL)
 , mType(LLAssetType::AT_NONE)
 , mPersist(false)
{
}

LLOfferInfo::LLOfferInfo(const LLSD& sd)
{
	mIM = (EInstantMessage)sd["im_type"].asInteger();
	mFromID = sd["from_id"].asUUID();
	mFromGroup = sd["from_group"].asBoolean();
	mFromObject = sd["from_object"].asBoolean();
	mTransactionID = sd["transaction_id"].asUUID();
	mFolderID = sd["folder_id"].asUUID();
	mObjectID = sd["object_id"].asUUID();
	mType = LLAssetType::lookup(sd["type"].asString().c_str());
	mFromName = sd["from_name"].asString();
	mDesc = sd["description"].asString();
	mHost = LLHost(sd["sender"].asString());
	mPersist = sd["persist"].asBoolean();
}

LLOfferInfo::LLOfferInfo(const LLOfferInfo& info)
{
	mIM = info.mIM;
	mFromID = info.mFromID;
	mFromGroup = info.mFromGroup;
	mFromObject = info.mFromObject;
	mTransactionID = info.mTransactionID;
	mFolderID = info.mFolderID;
	mObjectID = info.mObjectID;
	mType = info.mType;
	mFromName = info.mFromName;
	mDesc = info.mDesc;
	mHost = info.mHost;
	mPersist = info.mPersist;
}

LLSD LLOfferInfo::asLLSD()
{
	LLSD sd;
	sd["im_type"] = mIM;
	sd["from_id"] = mFromID;
	sd["from_group"] = mFromGroup;
	sd["from_object"] = mFromObject;
	sd["transaction_id"] = mTransactionID;
	sd["folder_id"] = mFolderID;
	sd["object_id"] = mObjectID;
	sd["type"] = LLAssetType::lookup(mType);
	sd["from_name"] = mFromName;
	sd["description"] = mDesc;
	sd["sender"] = mHost.getIPandPort();
	sd["persist"] = mPersist;
	return sd;
}

void LLOfferInfo::fromLLSD(const LLSD& params)
{
	*this = params;
}

void LLOfferInfo::send_auto_receive_response(void)
{	
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, mFromID);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addUUIDFast(_PREHASH_ID, mTransactionID);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary
	std::string name;
	LLAgentUI::buildFullname(name);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, ""); 
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
	
	// Auto Receive Message. The math for the dialog works, because the accept
	// for inventory_offered, task_inventory_offer or
	// group_notice_inventory is 1 greater than the offer integer value.
	// Generates IM_INVENTORY_ACCEPTED, IM_TASK_INVENTORY_ACCEPTED, 
	// or IM_GROUP_NOTICE_INVENTORY_ACCEPTED
	msg->addU8Fast(_PREHASH_Dialog, (U8)(mIM + 1));
	msg->addBinaryDataFast(_PREHASH_BinaryBucket, &(mFolderID.mData),
						   sizeof(mFolderID.mData));
	// send the message
	msg->sendReliable(mHost);
	
	if(IM_INVENTORY_OFFERED == mIM)
	{
		// add buddy to recent people list
		LLRecentPeople::instance().add(mFromID);
	}
}

void LLOfferInfo::handleRespond(const LLSD& notification, const LLSD& response)
{
	initRespondFunctionMap();

	const std::string name = notification["name"].asString();
	if(mRespondFunctions.find(name) == mRespondFunctions.end())
	{
		llwarns << "Unexpected notification name : " << name << llendl;
		llassert(!"Unexpected notification name");
		return;
	}

	mRespondFunctions[name](notification, response);
}

bool LLOfferInfo::inventory_offer_callback(const LLSD& notification, const LLSD& response)
{
	LLChat chat;
	std::string log_message;
	S32 button = LLNotificationsUtil::getSelectedOption(notification, response);

	LLInventoryObserver* opener = NULL;
	LLViewerInventoryCategory* catp = NULL;
	catp = (LLViewerInventoryCategory*)gInventory.getCategory(mObjectID);
	LLViewerInventoryItem* itemp = NULL;
	if(!catp)
	{
		itemp = (LLViewerInventoryItem*)gInventory.getItem(mObjectID);
	}
	 
	// For muting, we need to add the mute, then decline the offer.
	// This must be done here because:
	// * callback may be called immediately,
	// * adding the mute sends a message,
	// * we can't build two messages at once.
	if (2 == button) // Block
	{
		LLNotificationPtr notification_ptr = LLNotifications::instance().find(notification["id"].asUUID());

		llassert(notification_ptr != NULL);
		if (notification_ptr != NULL)
		{
			gCacheName->get(mFromID, mFromGroup, boost::bind(&inventory_offer_mute_callback, _1, _2, _3));
		}
	}

	std::string from_string; // Used in the pop-up.
	std::string chatHistory_string;  // Used in chat history.
	
	// TODO: when task inventory offers can also be handled the new way, migrate the code that sets these strings here:
	from_string = chatHistory_string = mFromName;
	
	bool busy = gAgent.getBusy();
	
	switch(button)
	{
	case IOR_SHOW:
		// we will want to open this item when it comes back.
		LL_DEBUGS("Messaging") << "Initializing an opener for tid: " << mTransactionID
				 << LL_ENDL;
		switch (mIM)
		{
		case IM_INVENTORY_OFFERED:
			{
				// This is an offer from an agent. In this case, the back
				// end has already copied the items into your inventory,
				// so we can fetch it out of our inventory.
				if (gSavedSettings.getBOOL("ShowOfferedInventory"))
				{
					LLOpenAgentOffer* open_agent_offer = new LLOpenAgentOffer(mObjectID, from_string);
					open_agent_offer->startFetch();
					if(catp || (itemp && itemp->isFinished()))
					{
						open_agent_offer->done();
					}
					else
					{
						opener = open_agent_offer;
					}
				}
			}
			break;
		case IM_GROUP_NOTICE:
			opener = new LLOpenTaskGroupOffer;
			send_auto_receive_response();
			break;
		case IM_TASK_INVENTORY_OFFERED:
		case IM_GROUP_NOTICE_REQUESTED:
			// This is an offer from a task or group.
			// We don't use a new instance of an opener
			// We instead use the singular observer gOpenTaskOffer
			// Since it already exists, we don't need to actually do anything
			break;
		default:
			LL_WARNS("Messaging") << "inventory_offer_callback: unknown offer type" << LL_ENDL;
			break;
		}
		break;
		// end switch (mIM)
			
	case IOR_ACCEPT:
		//don't spam them if they are getting flooded
		if (check_offer_throttle(mFromName, true))
		{
			log_message = chatHistory_string + " " + LLTrans::getString("InvOfferGaveYou") + " " + mDesc + LLTrans::getString(".");
			LLSD args;
			args["MESSAGE"] = log_message;
			LLNotificationsUtil::add("SystemMessageTip", args);
		}
		break;

	case IOR_MUTE:
		// MUTE falls through to decline
	case IOR_DECLINE:
		{
			{
				LLStringUtil::format_map_t log_message_args;
				log_message_args["DESC"] = mDesc;
				log_message_args["NAME"] = mFromName;
				log_message = LLTrans::getString("InvOfferDecline", log_message_args);
			}
			chat.mText = log_message;
			if( LLMuteList::getInstance()->isMuted(mFromID ) && ! LLMuteList::getInstance()->isLinden(mFromName) )  // muting for SL-42269
			{
				chat.mMuted = TRUE;
			}

			// *NOTE dzaporozhan
			// Disabled logging to old chat floater to fix crash in group notices - EXT-4149
			// LLFloaterChat::addChatHistory(chat);
			
			LLDiscardAgentOffer* discard_agent_offer = new LLDiscardAgentOffer(mFolderID, mObjectID);
			discard_agent_offer->startFetch();
			if (catp || (itemp && itemp->isFinished()))
			{
				discard_agent_offer->done();
			}
			else
			{
				opener = discard_agent_offer;
			}
			
			
			if (busy &&	(!mFromGroup && !mFromObject))
			{
				busy_message(gMessageSystem, mFromID);
			}
			break;
		}
	default:
		// close button probably
		// The item has already been fetched and is in your inventory, we simply won't highlight it
		// OR delete it if the notification gets killed, since we don't want that to be a vector for 
		// losing inventory offers.
		break;
	}

	if(opener)
	{
		gInventory.addObserver(opener);
	}

	if(!mPersist)
	{
		delete this;
	}
	return false;
}

bool LLOfferInfo::inventory_task_offer_callback(const LLSD& notification, const LLSD& response)
{
	LLChat chat;
	std::string log_message;
	S32 button = LLNotification::getSelectedOption(notification, response);
	
	// For muting, we need to add the mute, then decline the offer.
	// This must be done here because:
	// * callback may be called immediately,
	// * adding the mute sends a message,
	// * we can't build two messages at once.
	if (2 == button)
	{
		LLNotificationPtr notification_ptr = LLNotifications::instance().find(notification["id"].asUUID());

		llassert(notification_ptr != NULL);
		if (notification_ptr != NULL)
		{
			gCacheName->get(mFromID, mFromGroup, boost::bind(&inventory_offer_mute_callback, _1, _2, _3));
		}
	}
	
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, mFromID);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addUUIDFast(_PREHASH_ID, mTransactionID);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary
	std::string name;
	LLAgentUI::buildFullname(name);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, ""); 
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
	LLInventoryObserver* opener = NULL;
	
	std::string from_string; // Used in the pop-up.
	std::string chatHistory_string;  // Used in chat history.
	if (mFromObject == TRUE)
	{
		if (mFromGroup)
		{
			std::string group_name;
			if (gCacheName->getGroupName(mFromID, group_name))
			{
				from_string = LLTrans::getString("InvOfferAnObjectNamed") + " "+"'" 
				+ mFromName + LLTrans::getString("'") +" " + LLTrans::getString("InvOfferOwnedByGroup") 
				+ " "+ "'" + group_name + "'";
				
				chatHistory_string = mFromName + " " + LLTrans::getString("InvOfferOwnedByGroup") 
				+ " " + group_name + "'";
			}
			else
			{
				from_string = LLTrans::getString("InvOfferAnObjectNamed") + " "+"'"
				+ mFromName +"'"+ " " + LLTrans::getString("InvOfferOwnedByUnknownGroup");
				chatHistory_string = mFromName + " " + LLTrans::getString("InvOfferOwnedByUnknownGroup");
			}
		}
		else
		{
			std::string full_name;
			if (gCacheName->getFullName(mFromID, full_name))
			{
				from_string = LLTrans::getString("InvOfferAnObjectNamed") + " "+ LLTrans::getString("'") + mFromName 
					+ LLTrans::getString("'")+" " + LLTrans::getString("InvOfferOwnedBy") + full_name;
				chatHistory_string = mFromName + " " + LLTrans::getString("InvOfferOwnedBy") + " " + full_name;
			}
			else
			{
				from_string = LLTrans::getString("InvOfferAnObjectNamed") + " "+LLTrans::getString("'") 
				+ mFromName + LLTrans::getString("'")+" " + LLTrans::getString("InvOfferOwnedByUnknownUser");
				chatHistory_string = mFromName + " " + LLTrans::getString("InvOfferOwnedByUnknownUser");
			}
		}
	}
	else
	{
		from_string = chatHistory_string = mFromName;
	}
	
	bool busy = gAgent.getBusy();
	
	switch(button)
	{
		case IOR_ACCEPT:
			// ACCEPT. The math for the dialog works, because the accept
			// for inventory_offered, task_inventory_offer or
			// group_notice_inventory is 1 greater than the offer integer value.
			// Generates IM_INVENTORY_ACCEPTED, IM_TASK_INVENTORY_ACCEPTED, 
			// or IM_GROUP_NOTICE_INVENTORY_ACCEPTED
			msg->addU8Fast(_PREHASH_Dialog, (U8)(mIM + 1));
			msg->addBinaryDataFast(_PREHASH_BinaryBucket, &(mFolderID.mData),
								   sizeof(mFolderID.mData));
			// send the message
			msg->sendReliable(mHost);
			
			//don't spam them if they are getting flooded
			if (check_offer_throttle(mFromName, true))
			{
				log_message = chatHistory_string + " " + LLTrans::getString("InvOfferGaveYou") + " " + mDesc + LLTrans::getString(".");
				LLSD args;
				args["MESSAGE"] = log_message;
				LLNotificationsUtil::add("SystemMessageTip", args);
			}
			
			// we will want to open this item when it comes back.
			LL_DEBUGS("Messaging") << "Initializing an opener for tid: " << mTransactionID
			<< LL_ENDL;
			switch (mIM)
		{
			case IM_TASK_INVENTORY_OFFERED:
			case IM_GROUP_NOTICE:
			case IM_GROUP_NOTICE_REQUESTED:
			{
				// This is an offer from a task or group.
				// We don't use a new instance of an opener
				// We instead use the singular observer gOpenTaskOffer
				// Since it already exists, we don't need to actually do anything
			}
				break;
			default:
				LL_WARNS("Messaging") << "inventory_offer_callback: unknown offer type" << LL_ENDL;
				break;
		}	// end switch (mIM)
			break;
			
		case IOR_MUTE:
			// MUTE falls through to decline
		case IOR_DECLINE:
			// DECLINE. The math for the dialog works, because the decline
			// for inventory_offered, task_inventory_offer or
			// group_notice_inventory is 2 greater than the offer integer value.
			// Generates IM_INVENTORY_DECLINED, IM_TASK_INVENTORY_DECLINED,
			// or IM_GROUP_NOTICE_INVENTORY_DECLINED
		default:
			// close button probably (or any of the fall-throughs from above)
			msg->addU8Fast(_PREHASH_Dialog, (U8)(mIM + 2));
			msg->addBinaryDataFast(_PREHASH_BinaryBucket, EMPTY_BINARY_BUCKET, EMPTY_BINARY_BUCKET_SIZE);
			// send the message
			msg->sendReliable(mHost);

			if (gSavedSettings.getBOOL("LogInventoryDecline"))
			{
				LLStringUtil::format_map_t log_message_args;
				log_message_args["DESC"] = mDesc;
				log_message_args["NAME"] = mFromName;
				log_message = LLTrans::getString("InvOfferDecline", log_message_args);

				LLSD args;
				args["MESSAGE"] = log_message;
				LLNotificationsUtil::add("SystemMessageTip", args);
			}
			
			if (busy &&	(!mFromGroup && !mFromObject))
			{
				busy_message(msg,mFromID);
			}
			break;
	}
	
	if(opener)
	{
		gInventory.addObserver(opener);
	}

	if(!mPersist)
	{
		delete this;
	}
	return false;
}

class LLPostponedOfferNotification: public LLPostponedNotification
{
protected:
	/* virtual */
	void modifyNotificationParams()
	{
		LLSD substitutions = mParams.substitutions;
		substitutions["NAME"] = mName;
		mParams.substitutions = substitutions;
	}
};

void LLOfferInfo::initRespondFunctionMap()
{
	if(mRespondFunctions.empty())
	{
		mRespondFunctions["ObjectGiveItem"] = boost::bind(&LLOfferInfo::inventory_task_offer_callback, this, _1, _2);
		mRespondFunctions["OwnObjectGiveItem"] = boost::bind(&LLOfferInfo::inventory_task_offer_callback, this, _1, _2);
		mRespondFunctions["UserGiveItem"] = boost::bind(&LLOfferInfo::inventory_offer_callback, this, _1, _2);
	}
}

void inventory_offer_handler(LLOfferInfo* info)
{
	// If muted, don't even go through the messaging stuff.  Just curtail the offer here.
	// Passing in a null UUID handles the case of where you have muted one of your own objects by_name.
	// The solution for STORM-1297 seems to handle the cases where the object is owned by someone else.
	if (LLMuteList::getInstance()->isMuted(info->mFromID, info->mFromName) ||
		LLMuteList::getInstance()->isMuted(LLUUID::null, info->mFromName))
	{
		info->forceResponse(IOR_MUTE);
		return;
	}

	// Avoid the Accept/Discard dialog if the user so desires. JC
	if (gSavedSettings.getBOOL("AutoAcceptNewInventory")
		&& (info->mType == LLAssetType::AT_NOTECARD
			|| info->mType == LLAssetType::AT_LANDMARK
			|| info->mType == LLAssetType::AT_TEXTURE))
	{
		// For certain types, just accept the items into the inventory,
		// and possibly open them on receipt depending upon "ShowNewInventory".
		info->forceResponse(IOR_ACCEPT);
		return;
	}

	// Strip any SLURL from the message display. (DEV-2754)
	std::string msg = info->mDesc;
	int indx = msg.find(" ( http://slurl.com/secondlife/");
	if(indx == std::string::npos)
	{
		// try to find new slurl host
		indx = msg.find(" ( http://maps.secondlife.com/secondlife/");
	}
	if(indx >= 0)
	{
		LLStringUtil::truncate(msg, indx);
	}

	LLSD args;
	args["[OBJECTNAME]"] = msg;

	LLSD payload;

	// must protect against a NULL return from lookupHumanReadable()
	std::string typestr = ll_safe_string(LLAssetType::lookupHumanReadable(info->mType));
	if (!typestr.empty())
	{
		// human readable matches string name from strings.xml
		// lets get asset type localized name
		args["OBJECTTYPE"] = LLTrans::getString(typestr);
	}
	else
	{
		LL_WARNS("Messaging") << "LLAssetType::lookupHumanReadable() returned NULL - probably bad asset type: " << info->mType << LL_ENDL;
		args["OBJECTTYPE"] = "";

		// This seems safest, rather than propagating bogosity
		LL_WARNS("Messaging") << "Forcing an inventory-decline for probably-bad asset type." << LL_ENDL;
		info->forceResponse(IOR_DECLINE);
		return;
	}

	// If mObjectID is null then generate the object_id based on msg to prevent
	// multiple creation of chiclets for same object.
	LLUUID object_id = info->mObjectID;
	if (object_id.isNull())
		object_id.generate(msg);

	payload["from_id"] = info->mFromID;
	// Needed by LLScriptFloaterManager to bind original notification with 
	// faked for toast one.
	payload["object_id"] = object_id;
	// Flag indicating that this notification is faked for toast.
	payload["give_inventory_notification"] = FALSE;
	args["OBJECTFROMNAME"] = info->mFromName;
	args["NAME"] = info->mFromName;
	if (info->mFromGroup)
	{
		args["NAME_SLURL"] = LLSLURL("group", info->mFromID, "about").getSLURLString();
	}
	else
	{
		args["NAME_SLURL"] = LLSLURL("agent", info->mFromID, "about").getSLURLString();
	}
	std::string verb = "select?name=" + LLURI::escape(msg);
	args["ITEM_SLURL"] = LLSLURL("inventory", info->mObjectID, verb.c_str()).getSLURLString();

	LLNotification::Params p;

	// Object -> Agent Inventory Offer
	if (info->mFromObject)
	{
		// Inventory Slurls don't currently work for non agent transfers, so only display the object name.
		args["ITEM_SLURL"] = msg;
		// Note: sets inventory_task_offer_callback as the callback
		p.substitutions(args).payload(payload).functor.responder(LLNotificationResponderPtr(info));
		info->mPersist = true;

		// Offers from your own objects need a special notification template.
		p.name = info->mFromID == gAgentID ? "OwnObjectGiveItem" : "ObjectGiveItem";

		// Pop up inv offer chiclet and let the user accept (keep), or reject (and silently delete) the inventory.
	    LLPostponedNotification::add<LLPostponedOfferNotification>(p, info->mFromID, info->mFromGroup == TRUE);
	}
	else // Agent -> Agent Inventory Offer
	{
		p.responder = info;
		// Note: sets inventory_offer_callback as the callback
		// *TODO fix memory leak
		// inventory_offer_callback() is not invoked if user received notification and 
		// closes viewer(without responding the notification)
		p.substitutions(args).payload(payload).functor.responder(LLNotificationResponderPtr(info));
		info->mPersist = true;
		p.name = "UserGiveItem";
		
		// Prefetch the item into your local inventory.
		LLInventoryFetchItemsObserver* fetch_item = new LLInventoryFetchItemsObserver(info->mObjectID);
		fetch_item->startFetch();
		if(fetch_item->isFinished())
		{
			fetch_item->done();
		}
		else
		{
			gInventory.addObserver(fetch_item);
		}
		
		// In viewer 2 we're now auto receiving inventory offers and messaging as such (not sending reject messages).
		info->send_auto_receive_response();

		// Inform user that there is a script floater via toast system
		{
			payload["give_inventory_notification"] = TRUE;
		    p.payload = payload;
		    LLPostponedNotification::add<LLPostponedOfferNotification>(p, info->mFromID, false);
		}
	}

	LLFirstUse::newInventory();
}

bool lure_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = 0;
	if (response.isInteger()) 
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}
	
	LLUUID from_id = notification["payload"]["from_id"].asUUID();
	LLUUID lure_id = notification["payload"]["lure_id"].asUUID();
	BOOL godlike = notification["payload"]["godlike"].asBoolean();

	switch(option)
	{
	case 0:
		{
			// accept
			gAgent.teleportViaLure(lure_id, godlike);
		}
		break;
	case 1:
	default:
		// decline
		send_simple_im(from_id,
					   LLStringUtil::null,
					   IM_LURE_DECLINED,
					   lure_id);
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration lure_callback_reg("TeleportOffered", lure_callback);

bool mature_lure_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = 0;
	if (response.isInteger()) 
	{
		option = response.asInteger();
	}
	else
	{
		option = LLNotificationsUtil::getSelectedOption(notification, response);
	}
	
	LLUUID from_id = notification["payload"]["from_id"].asUUID();
	LLUUID lure_id = notification["payload"]["lure_id"].asUUID();
	BOOL godlike = notification["payload"]["godlike"].asBoolean();
	U8 region_access = static_cast<U8>(notification["payload"]["region_maturity"].asInteger());

	switch(option)
	{
	case 0:
		{
			// accept
			gSavedSettings.setU32("PreferredMaturity", static_cast<U32>(region_access));
			gAgent.setMaturityRatingChangeDuringTeleport(region_access);
			gAgent.teleportViaLure(lure_id, godlike);
		}
		break;
	case 1:
	default:
		// decline
		send_simple_im(from_id,
					   LLStringUtil::null,
					   IM_LURE_DECLINED,
					   lure_id);
		break;
	}
	return false;
}
static LLNotificationFunctorRegistration mature_lure_callback_reg("TeleportOffered_MaturityExceeded", mature_lure_callback);

bool goto_url_callback(const LLSD& notification, const LLSD& response)
{
	std::string url = notification["payload"]["url"].asString();
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if(1 == option)
	{
		LLWeb::loadURL(url);
	}
	return false;
}
static LLNotificationFunctorRegistration goto_url_callback_reg("GotoURL", goto_url_callback);

bool inspect_remote_object_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (0 == option)
	{
		LLFloaterReg::showInstance("inspect_remote_object", notification["payload"]);
	}
	return false;
}
static LLNotificationFunctorRegistration inspect_remote_object_callback_reg("ServerObjectMessage", inspect_remote_object_callback);

class LLPostponedServerObjectNotification: public LLPostponedNotification
{
protected:
	/* virtual */
	void modifyNotificationParams()
	{
		LLSD payload = mParams.payload;
		mParams.payload = payload;
	}
};

static bool parse_lure_bucket(const std::string& bucket,
							  U64& region_handle,
							  LLVector3& pos,
							  LLVector3& look_at,
							  U8& region_access)
{
	// tokenize the bucket
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
	tokenizer tokens(bucket, sep);
	tokenizer::iterator iter = tokens.begin();

	S32 gx,gy,rx,ry,rz,lx,ly,lz;
	try
	{
		gx = boost::lexical_cast<S32>((*(iter)).c_str());
		gy = boost::lexical_cast<S32>((*(++iter)).c_str());
		rx = boost::lexical_cast<S32>((*(++iter)).c_str());
		ry = boost::lexical_cast<S32>((*(++iter)).c_str());
		rz = boost::lexical_cast<S32>((*(++iter)).c_str());
		lx = boost::lexical_cast<S32>((*(++iter)).c_str());
		ly = boost::lexical_cast<S32>((*(++iter)).c_str());
		lz = boost::lexical_cast<S32>((*(++iter)).c_str());
	}
	catch( boost::bad_lexical_cast& )
	{
		LL_WARNS("parse_lure_bucket")
			<< "Couldn't parse lure bucket."
			<< LL_ENDL;
		return false;
	}
	// Grab region access
	region_access = SIM_ACCESS_MIN;
	if (++iter != tokens.end())
	{
		std::string access_str((*iter).c_str());
		LLStringUtil::trim(access_str);
		if ( access_str == "A" )
		{
			region_access = SIM_ACCESS_ADULT;
		}
		else if ( access_str == "M" )
		{
			region_access = SIM_ACCESS_MATURE;
		}
		else if ( access_str == "PG" )
		{
			region_access = SIM_ACCESS_PG;
		}
	}

	pos.setVec((F32)rx, (F32)ry, (F32)rz);
	look_at.setVec((F32)lx, (F32)ly, (F32)lz);

	region_handle = to_region_handle(gx, gy);
	return true;
}

// Strip out "Resident" for display, but only if the message came from a user
// (rather than a script)
static std::string clean_name_from_im(const std::string& name, EInstantMessage type)
{
	switch(type)
	{
	case IM_NOTHING_SPECIAL:
	case IM_MESSAGEBOX:
	case IM_GROUP_INVITATION:
	case IM_INVENTORY_OFFERED:
	case IM_INVENTORY_ACCEPTED:
	case IM_INVENTORY_DECLINED:
	case IM_GROUP_VOTE:
	case IM_GROUP_MESSAGE_DEPRECATED:
	//IM_TASK_INVENTORY_OFFERED
	//IM_TASK_INVENTORY_ACCEPTED
	//IM_TASK_INVENTORY_DECLINED
	case IM_NEW_USER_DEFAULT:
	case IM_SESSION_INVITE:
	case IM_SESSION_P2P_INVITE:
	case IM_SESSION_GROUP_START:
	case IM_SESSION_CONFERENCE_START:
	case IM_SESSION_SEND:
	case IM_SESSION_LEAVE:
	//IM_FROM_TASK
	case IM_BUSY_AUTO_RESPONSE:
	case IM_CONSOLE_AND_CHAT_HISTORY:
	case IM_LURE_USER:
	case IM_LURE_ACCEPTED:
	case IM_LURE_DECLINED:
	case IM_GODLIKE_LURE_USER:
	case IM_YET_TO_BE_USED:
	case IM_GROUP_ELECTION_DEPRECATED:
	//IM_GOTO_URL
	//IM_FROM_TASK_AS_ALERT
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_INVENTORY_ACCEPTED:
	case IM_GROUP_NOTICE_INVENTORY_DECLINED:
	case IM_GROUP_INVITATION_ACCEPT:
	case IM_GROUP_INVITATION_DECLINE:
	case IM_GROUP_NOTICE_REQUESTED:
	case IM_FRIENDSHIP_OFFERED:
	case IM_FRIENDSHIP_ACCEPTED:
	case IM_FRIENDSHIP_DECLINED_DEPRECATED:
	//IM_TYPING_START
	//IM_TYPING_STOP
		return LLCacheName::cleanFullName(name);
	default:
		return name;
	}
}

static std::string clean_name_from_task_im(const std::string& msg,
										   BOOL from_group)
{
	boost::smatch match;
	static const boost::regex returned_exp(
		"(.*been returned to your inventory lost and found folder by )(.+)( (from|near).*)");
	if (boost::regex_match(msg, match, returned_exp))
	{
		// match objects are 1-based for groups
		std::string final = match[1].str();
		std::string name = match[2].str();
		// Don't try to clean up group names
		if (!from_group)
		{
			if (LLAvatarNameCache::useDisplayNames())
			{
				// ...just convert to username
				final += LLCacheName::buildUsername(name);
			}
			else
			{
				// ...strip out legacy "Resident" name
				final += LLCacheName::cleanFullName(name);
			}
		}
		final += match[3].str();
		return final;
	}
	return msg;
}

void notification_display_name_callback(const LLUUID& id,
					  const LLAvatarName& av_name,
					  const std::string& name, 
					  LLSD& substitutions, 
					  const LLSD& payload)
{
	substitutions["NAME"] = av_name.mDisplayName;
	LLNotificationsUtil::add(name, substitutions, payload);
}

class LLPostponedIMSystemTipNotification: public LLPostponedNotification
{
protected:
	/* virtual */
	void modifyNotificationParams()
	{
		LLSD payload = mParams.payload;
		payload["SESSION_NAME"] = mName;
		mParams.payload = payload;
	}

};

// Callback for name resolution of a god/estate message
void god_message_name_cb(const LLAvatarName& av_name, LLChat chat, std::string message)
{	
	LLSD args;
	args["NAME"] = av_name.getCompleteName();
	args["MESSAGE"] = message;
	LLNotificationsUtil::add("GodMessage", args);

	// Treat like a system message and put in chat history.
	chat.mText = av_name.getCompleteName() + ": " + message;

	LLNearbyChat* nearby_chat = LLNearbyChat::getInstance();
	if(nearby_chat)
	{
		nearby_chat->addMessage(chat);
	}

}

void process_improved_im(LLMessageSystem *msg, void **user_data)
{
	LLUUID from_id;
	BOOL from_group;
	LLUUID to_id;
	U8 offline;
	U8 d = 0;
	LLUUID session_id;
	U32 timestamp;
	std::string name;
	std::string message;
	U32 parent_estate_id = 0;
	LLUUID region_id;
	LLVector3 position;
	U8 binary_bucket[MTUBYTES];
	S32 binary_bucket_size;
	LLChat chat;
	std::string buffer;
	
	// *TODO: Translate - need to fix the full name to first/last (maybe)
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, from_id);
	msg->getBOOLFast(_PREHASH_MessageBlock, _PREHASH_FromGroup, from_group);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ToAgentID, to_id);
	msg->getU8Fast(  _PREHASH_MessageBlock, _PREHASH_Offline, offline);
	msg->getU8Fast(  _PREHASH_MessageBlock, _PREHASH_Dialog, d);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ID, session_id);
	msg->getU32Fast( _PREHASH_MessageBlock, _PREHASH_Timestamp, timestamp);
	//msg->getData("MessageBlock", "Count",		&count);
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_FromAgentName, name);
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_Message,		message);
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_RegionID, region_id);
	msg->getVector3Fast(_PREHASH_MessageBlock, _PREHASH_Position, position);
	msg->getBinaryDataFast(  _PREHASH_MessageBlock, _PREHASH_BinaryBucket, binary_bucket, 0, 0, MTUBYTES);
	binary_bucket_size = msg->getSizeFast(_PREHASH_MessageBlock, _PREHASH_BinaryBucket);
	EInstantMessage dialog = (EInstantMessage)d;

    // make sure that we don't have an empty or all-whitespace name
	LLStringUtil::trim(name);
	if (name.empty())
	{
        name = LLTrans::getString("Unnamed");
	}

	// Preserve the unaltered name for use in group notice mute checking.
	std::string original_name = name;

	// IDEVO convert new-style "Resident" names for display
	name = clean_name_from_im(name, dialog);

	BOOL is_busy = gAgent.getBusy();
	BOOL is_muted = LLMuteList::getInstance()->isMuted(from_id, name, LLMute::flagTextChat)
		// object IMs contain sender object id in session_id (STORM-1209)
		|| dialog == IM_FROM_TASK && LLMuteList::getInstance()->isMuted(session_id);
	BOOL is_linden = LLMuteList::getInstance()->isLinden(name);
	BOOL is_owned_by_me = FALSE;
	BOOL is_friend = (LLAvatarTracker::instance().getBuddyInfo(from_id) == NULL) ? false : true;
	BOOL accept_im_from_only_friend = gSavedSettings.getBOOL("VoiceCallsFriendsOnly");
	
	chat.mMuted = is_muted && !is_linden;
	chat.mFromID = from_id;
	chat.mFromName = name;
	chat.mSourceType = (from_id.isNull() || (name == std::string(SYSTEM_FROM))) ? CHAT_SOURCE_SYSTEM : CHAT_SOURCE_AGENT;

	LLViewerObject *source = gObjectList.findObject(session_id); //Session ID is probably the wrong thing.
	if (source)
	{
		is_owned_by_me = source->permYouOwner();
	}

	std::string separator_string(": ");

	LLSD args;
	LLSD payload;
	LLNotification::Params params;

	switch(dialog)
	{
	case IM_CONSOLE_AND_CHAT_HISTORY:
		args["MESSAGE"] = message;
		payload["from_id"] = from_id;

		params.name = "IMSystemMessageTip";
		params.substitutions = args;
		params.payload = payload;
	    LLPostponedNotification::add<LLPostponedIMSystemTipNotification>(params, from_id, false);
		break;

	case IM_NOTHING_SPECIAL: 
		// Don't show dialog, just do IM
		if (!gAgent.isGodlike()
				&& gAgent.getRegion()->isPrelude() 
				&& to_id.isNull() )
		{
			// do nothing -- don't distract newbies in
			// Prelude with global IMs
		}
		else if (offline == IM_ONLINE && !is_linden && is_busy && name != SYSTEM_FROM)
		{
			// return a standard "busy" message, but only do it to online IM 
			// (i.e. not other auto responses and not store-and-forward IM)
			if (!gIMMgr->hasSession(session_id))
			{
				// if there is not a panel for this conversation (i.e. it is a new IM conversation
				// initiated by the other party) then...
				std::string my_name;
				LLAgentUI::buildFullname(my_name);
				std::string response = gSavedPerAccountSettings.getString("BusyModeResponse");
				pack_instant_message(
					gMessageSystem,
					gAgent.getID(),
					FALSE,
					gAgent.getSessionID(),
					from_id,
					my_name,
					response,
					IM_ONLINE,
					IM_BUSY_AUTO_RESPONSE,
					session_id);
				gAgent.sendReliableMessage();
			}

			// now store incoming IM in chat history

			buffer = message;
	
			LL_INFOS("Messaging") << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;

			// add to IM panel, but do not bother the user
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				LLStringUtil::null,
				dialog,
				parent_estate_id,
				region_id,
				position,
				true);
		}
		else if (from_id.isNull())
		{
			LLSD args;
			args["MESSAGE"] = message;
			LLNotificationsUtil::add("SystemMessage", args);
		}
		else if (to_id.isNull())
		{
			// Message to everyone from GOD, look up the fullname since
			// server always slams name to legacy names
			LLAvatarNameCache::get(from_id, boost::bind(god_message_name_cb, _2, chat, message));
		}
		else
		{
			// standard message, not from system
			std::string saved;
			if(offline == IM_OFFLINE)
			{
				LLStringUtil::format_map_t args;
				args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
				saved = LLTrans::getString("Saved_message", args);
			}
			buffer = saved + message;

			LL_INFOS("Messaging") << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << LL_ENDL;

			bool mute_im = is_muted;
			if (accept_im_from_only_friend && !is_friend)
			{
				if (!gIMMgr->isNonFriendSessionNotified(session_id))
				{
					std::string message = LLTrans::getString("IM_unblock_only_groups_friends");
					gIMMgr->addMessage(session_id, from_id, name, message);
					gIMMgr->addNotifiedNonFriendSessionID(session_id);
				}

				mute_im = true;
			}
			if (!mute_im || is_linden) 
			{
				gIMMgr->addMessage(
					session_id,
					from_id,
					name,
					buffer,
					LLStringUtil::null,
					dialog,
					parent_estate_id,
					region_id,
					position,
					true);
			}
			else
			{
				/*
				EXT-5099
				currently there is no way to store in history only...
				using  LLNotificationsUtil::add will add message to Nearby Chat

				// muted user, so don't start an IM session, just record line in chat
				// history.  Pretend the chat is from a local agent,
				// so it will go into the history but not be shown on screen.

				LLSD args;
				args["MESSAGE"] = buffer;
				LLNotificationsUtil::add("SystemMessageTip", args);
				*/
			}
		}
		break;

	case IM_TYPING_START:
		{
			LLPointer<LLIMInfo> im_info = new LLIMInfo(gMessageSystem);
			gIMMgr->processIMTypingStart(im_info);
		}
		break;

	case IM_TYPING_STOP:
		{
			LLPointer<LLIMInfo> im_info = new LLIMInfo(gMessageSystem);
			gIMMgr->processIMTypingStop(im_info);
		}
		break;

	case IM_MESSAGEBOX:
		{
			// This is a block, modeless dialog.
			//*TODO: Translate
			args["MESSAGE"] = message;
			LLNotificationsUtil::add("SystemMessageTip", args);
		}
		break;
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_REQUESTED:
		{
			LL_INFOS("Messaging") << "Received IM_GROUP_NOTICE message." << LL_ENDL;
			// Read the binary bucket for more information.
			struct notice_bucket_header_t
			{
				U8 has_inventory;
				U8 asset_type;
				LLUUID group_id;
			};
			struct notice_bucket_full_t
			{
				struct notice_bucket_header_t header;
				U8 item_name[DB_INV_ITEM_NAME_BUF_SIZE];
			}* notice_bin_bucket;

			// Make sure the binary bucket is big enough to hold the header 
			// and a null terminated item name.
			if ( (binary_bucket_size < (S32)((sizeof(notice_bucket_header_t) + sizeof(U8))))
				|| (binary_bucket[binary_bucket_size - 1] != '\0') )
			{
				LL_WARNS("Messaging") << "Malformed group notice binary bucket" << LL_ENDL;
				break;
			}

			// The group notice packet does not have an AgentID.  Obtain one from the name cache.
			// If last name is "Resident" strip it out so the cache name lookup works.
			U32 index = original_name.find(" Resident");
			if (index != std::string::npos)
			{
				original_name = original_name.substr(0, index);
			}
			std::string legacy_name = gCacheName->buildLegacyName(original_name);
			LLUUID agent_id;
			gCacheName->getUUID(legacy_name, agent_id);

			if (agent_id.isNull())
			{
				LL_WARNS("Messaging") << "buildLegacyName returned null while processing " << original_name << LL_ENDL;
			}
			else if (LLMuteList::getInstance()->isMuted(agent_id))
			{
				break;
			}

			notice_bin_bucket = (struct notice_bucket_full_t*) &binary_bucket[0];
			U8 has_inventory = notice_bin_bucket->header.has_inventory;
			U8 asset_type = notice_bin_bucket->header.asset_type;
			LLUUID group_id = notice_bin_bucket->header.group_id;
			std::string item_name = ll_safe_string((const char*) notice_bin_bucket->item_name);

			// If there is inventory, give the user the inventory offer.
			LLOfferInfo* info = NULL;

			if (has_inventory)
			{
				info = new LLOfferInfo();
				
				info->mIM = IM_GROUP_NOTICE;
				info->mFromID = from_id;
				info->mFromGroup = from_group;
				info->mTransactionID = session_id;
				info->mType = (LLAssetType::EType) asset_type;
				info->mFolderID = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(info->mType));
				std::string from_name;

				from_name += "A group member named ";
				from_name += name;

				info->mFromName = from_name;
				info->mDesc = item_name;
				info->mHost = msg->getSender();
			}
			
			std::string str(message);

			// Tokenize the string.
			// TODO: Support escaped tokens ("||" -> "|")
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			boost::char_separator<char> sep("|","",boost::keep_empty_tokens);
			tokenizer tokens(str, sep);
			tokenizer::iterator iter = tokens.begin();

			std::string subj(*iter++);
			std::string mes(*iter++);

			// Send the notification down the new path.
			// For requested notices, we don't want to send the popups.
			if (dialog != IM_GROUP_NOTICE_REQUESTED)
			{
				payload["subject"] = subj;
				payload["message"] = mes;
				payload["sender_name"] = name;
				payload["group_id"] = group_id;
				payload["inventory_name"] = item_name;
				payload["inventory_offer"] = info ? info->asLLSD() : LLSD();

				LLSD args;
				args["SUBJECT"] = subj;
				args["MESSAGE"] = mes;
				LLNotifications::instance().add(LLNotification::Params("GroupNotice").substitutions(args).payload(payload).time_stamp(timestamp));
			}

			// Also send down the old path for now.
			if (IM_GROUP_NOTICE_REQUESTED == dialog)
			{
				
				LLPanelGroup::showNotice(subj,mes,group_id,has_inventory,item_name,info);
			}
			else
			{
				delete info;
			}
		}
		break;
	case IM_GROUP_INVITATION:
		{
			//if (!is_linden && (is_busy || is_muted))
			if ((is_busy || is_muted))
			{
				LLMessageSystem *msg = gMessageSystem;
				busy_message(msg,from_id);
			}
			else
			{
				LL_INFOS("Messaging") << "Received IM_GROUP_INVITATION message." << LL_ENDL;
				// Read the binary bucket for more information.
				struct invite_bucket_t
				{
					S32 membership_fee;
					LLUUID role_id;
				}* invite_bucket;

				// Make sure the binary bucket is the correct size.
				if (binary_bucket_size != sizeof(invite_bucket_t))
				{
					LL_WARNS("Messaging") << "Malformed group invite binary bucket" << LL_ENDL;
					break;
				}

				invite_bucket = (struct invite_bucket_t*) &binary_bucket[0];
				S32 membership_fee = ntohl(invite_bucket->membership_fee);

				LLSD payload;
				payload["transaction_id"] = session_id;
				payload["group_id"] = from_id;
				payload["name"] = name;
				payload["message"] = message;
				payload["fee"] = membership_fee;

				LLSD args;
				args["MESSAGE"] = message;
				// we shouldn't pass callback functor since it is registered in LLFunctorRegistration
				LLNotificationsUtil::add("JoinGroup", args, payload);
			}
		}
		break;

	case IM_INVENTORY_OFFERED:
	case IM_TASK_INVENTORY_OFFERED:
		// Someone has offered us some inventory.
		{
			LLOfferInfo* info = new LLOfferInfo;
			if (IM_INVENTORY_OFFERED == dialog)
			{
				struct offer_agent_bucket_t
				{
					S8		asset_type;
					LLUUID	object_id;
				}* bucketp;

				if (sizeof(offer_agent_bucket_t) != binary_bucket_size)
				{
					LL_WARNS("Messaging") << "Malformed inventory offer from agent" << LL_ENDL;
					delete info;
					break;
				}
				bucketp = (struct offer_agent_bucket_t*) &binary_bucket[0];
				info->mType = (LLAssetType::EType) bucketp->asset_type;
				info->mObjectID = bucketp->object_id;
				info->mFromObject = FALSE;
			}
			else // IM_TASK_INVENTORY_OFFERED
			{
				if (sizeof(S8) != binary_bucket_size)
				{
					LL_WARNS("Messaging") << "Malformed inventory offer from object" << LL_ENDL;
					delete info;
					break;
				}
				info->mType = (LLAssetType::EType) binary_bucket[0];
				info->mObjectID = LLUUID::null;
				info->mFromObject = TRUE;
			}

			info->mIM = dialog;
			info->mFromID = from_id;
			info->mFromGroup = from_group;
			info->mTransactionID = session_id;
			info->mFolderID = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(info->mType));

			info->mFromName = name;
			info->mDesc = message;
			info->mHost = msg->getSender();
			//if (((is_busy && !is_owned_by_me) || is_muted))
			if (is_muted)
			{
				// Prefetch the offered item so that it can be discarded by the appropriate observer. (EXT-4331)
				LLInventoryFetchItemsObserver* fetch_item = new LLInventoryFetchItemsObserver(info->mObjectID);
				fetch_item->startFetch();
				delete fetch_item;

				// Same as closing window
				info->forceResponse(IOR_DECLINE);
			}
			else if (is_busy && dialog != IM_TASK_INVENTORY_OFFERED) // busy mode must not affect interaction with objects (STORM-565)
			{
				// Until throttling is implemented, busy mode should reject inventory instead of silently
				// accepting it.  SEE SL-39554
				info->forceResponse(IOR_DECLINE);
			}
			else
			{
				inventory_offer_handler(info);
			}
		}
		break;

	case IM_INVENTORY_ACCEPTED:
	{
		args["NAME"] = LLSLURL("agent", from_id, "completename").getSLURLString();;
		LLSD payload;
		payload["from_id"] = from_id;
		// Passing the "SESSION_NAME" to use it for IM notification logging
		// in LLTipHandler::processNotification(). See STORM-941.
		payload["SESSION_NAME"] = name;
		LLNotificationsUtil::add("InventoryAccepted", args, payload);
		break;
	}
	case IM_INVENTORY_DECLINED:
	{
		args["NAME"] = LLSLURL("agent", from_id, "completename").getSLURLString();;
		LLSD payload;
		payload["from_id"] = from_id;
		LLNotificationsUtil::add("InventoryDeclined", args, payload);
		break;
	}
	// TODO: _DEPRECATED suffix as part of vote removal - DEV-24856
	case IM_GROUP_VOTE:
		{
			LL_WARNS("Messaging") << "Received IM: IM_GROUP_VOTE_DEPRECATED" << LL_ENDL;
		}
		break;

	case IM_GROUP_ELECTION_DEPRECATED:
	{
		LL_WARNS("Messaging") << "Received IM: IM_GROUP_ELECTION_DEPRECATED" << LL_ENDL;
	}
	break;
	
	case IM_SESSION_SEND:
	{
		if (is_busy)
		{
			return;
		}

		// Only show messages if we have a session open (which
		// should happen after you get an "invitation"
		if ( !gIMMgr->hasSession(session_id) )
		{
			return;
		}

		// standard message, not from system
		std::string saved;
		if(offline == IM_OFFLINE)
		{
			saved = llformat("(Saved %s) ", formatted_time(timestamp).c_str());
		}
		buffer = saved + message;
		BOOL is_this_agent = FALSE;
		if(from_id == gAgentID)
		{
			is_this_agent = TRUE;
		}
		gIMMgr->addMessage(
			session_id,
			from_id,
			name,
			buffer,
			ll_safe_string((char*)binary_bucket),
			IM_SESSION_INVITE,
			parent_estate_id,
			region_id,
			position,
			true);
	}
	break;

	case IM_FROM_TASK:
		{
			if (is_busy && !is_owned_by_me)
			{
				return;
			}

			// Build a link to open the object IM info window.
			std::string location = ll_safe_string((char*)binary_bucket, binary_bucket_size-1);

			if (session_id.notNull())
			{
				chat.mFromID = session_id;
			}
			else
			{
				// This message originated on a region without the updated code for task id and slurl information.
				// We just need a unique ID for this object that isn't the owner ID.
				// If it is the owner ID it will overwrite the style that contains the link to that owner's profile.
				// This isn't ideal - it will make 1 style for all objects owned by the the same person/group.
				// This works because the only thing we can really do in this case is show the owner name and link to their profile.
				chat.mFromID = from_id ^ gAgent.getSessionID();
			}

			chat.mSourceType = CHAT_SOURCE_OBJECT;

			// To conclude that the source type of message is CHAT_SOURCE_SYSTEM it's not
			// enough to check only from name (i.e. fromName = "Second Life"). For example
			// source type of messages from objects called "Second Life" should not be CHAT_SOURCE_SYSTEM.
			bool chat_from_system = (SYSTEM_FROM == name) && region_id.isNull() && position.isNull();
			if(chat_from_system)
			{
				// System's UUID is NULL (fixes EXT-4766)
				chat.mFromID = LLUUID::null;
				chat.mSourceType = CHAT_SOURCE_SYSTEM;
			}

			// IDEVO Some messages have embedded resident names
			message = clean_name_from_task_im(message, from_group);

			LLSD query_string;
			query_string["owner"] = from_id;
			query_string["slurl"] = location;
			query_string["name"] = name;
			if (from_group)
			{
				query_string["groupowned"] = "true";
			}	

			chat.mURL = LLSLURL("objectim", session_id, "").getSLURLString();
			chat.mText = message;

			// Note: lie to Nearby Chat, pretending that this is NOT an IM, because
			// IMs from obejcts don't open IM sessions.
			LLNearbyChat* nearby_chat = LLNearbyChat::getInstance();
			if(!chat_from_system && nearby_chat)
			{
				chat.mOwnerID = from_id;
				LLSD args;
				args["slurl"] = location;
				args["type"] = LLNotificationsUI::NT_NEARBYCHAT;

				// Look for IRC-style emotes here so object name formatting is correct
				std::string prefix = message.substr(0, 4);
				if (prefix == "/me " || prefix == "/me'")
				{
					chat.mChatStyle = CHAT_STYLE_IRC;
				}

				LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
			}


			//Object IMs send with from name: 'Second Life' need to be displayed also in notification toasts (EXT-1590)
			if (!chat_from_system) break;
			
			LLSD substitutions;
			substitutions["NAME"] = name;
			substitutions["MSG"] = message;

			LLSD payload;
			payload["object_id"] = session_id;
			payload["owner_id"] = from_id;
			payload["from_id"] = from_id;
			payload["slurl"] = location;
			payload["name"] = name;
			std::string session_name;
			if (from_group)
			{
				payload["group_owned"] = "true";
			}

			LLNotification::Params params("ServerObjectMessage");
			params.substitutions = substitutions;
			params.payload = payload;

			LLPostponedNotification::add<LLPostponedServerObjectNotification>(params, from_id, from_group);
		}
		break;
	case IM_FROM_TASK_AS_ALERT:
		if (is_busy && !is_owned_by_me)
		{
			return;
		}
		{
			// Construct a viewer alert for this message.
			args["NAME"] = name;
			args["MESSAGE"] = message;
			LLNotificationsUtil::add("ObjectMessage", args);
		}
		break;
	case IM_BUSY_AUTO_RESPONSE:
		if (is_muted)
		{
			LL_DEBUGS("Messaging") << "Ignoring busy response from " << from_id << LL_ENDL;
			return;
		}
		else
		{
			// TODO: after LLTrans hits release, get "busy response" into translatable file
			buffer = llformat("%s (%s): %s", name.c_str(), "busy response", message.c_str());
			gIMMgr->addMessage(session_id, from_id, name, buffer);
		}
		break;
		
	case IM_LURE_USER:
		{
			if (is_muted)
			{ 
				return;
			}
			else if (is_busy) 
			{
				busy_message(msg,from_id);
			}
			else
			{
				LLVector3 pos, look_at;
				U64 region_handle(0);
				U8 region_access(SIM_ACCESS_MIN);
				std::string region_info = ll_safe_string((char*)binary_bucket, binary_bucket_size);
				std::string region_access_str = LLStringUtil::null;
				std::string region_access_icn = LLStringUtil::null;
				std::string region_access_lc  = LLStringUtil::null;

				bool canUserAccessDstRegion = true;
				bool doesUserRequireMaturityIncrease = false;

				if (parse_lure_bucket(region_info, region_handle, pos, look_at, region_access))
				{
					region_access_str = LLViewerRegion::accessToString(region_access);
					region_access_icn = LLViewerRegion::getAccessIcon(region_access);
					region_access_lc  = region_access_str;
					LLStringUtil::toLower(region_access_lc);

					if (!gAgent.isGodlike())
					{
						switch (region_access)
						{
						case SIM_ACCESS_MIN :
						case SIM_ACCESS_PG :
							break;
						case SIM_ACCESS_MATURE :
							if (gAgent.isTeen())
							{
								canUserAccessDstRegion = false;
							}
							else if (gAgent.prefersPG())
							{
								doesUserRequireMaturityIncrease = true;
							}
							break;
						case SIM_ACCESS_ADULT :
							if (!gAgent.isAdult())
							{
								canUserAccessDstRegion = false;
							}
							else if (!gAgent.prefersAdult())
							{
								doesUserRequireMaturityIncrease = true;
							}
							break;
						default :
							llassert(0);
							break;
						}
					}
				}

				LLSD args;
				// *TODO: Translate -> [FIRST] [LAST] (maybe)
				args["NAME_SLURL"] = LLSLURL("agent", from_id, "about").getSLURLString();
				args["MESSAGE"] = message;
				args["MATURITY_STR"] = region_access_str;
				args["MATURITY_ICON"] = region_access_icn;
				args["REGION_CONTENT_MATURITY"] = region_access_lc;
				LLSD payload;
				payload["from_id"] = from_id;
				payload["lure_id"] = session_id;
				payload["godlike"] = FALSE;
				payload["region_maturity"] = region_access;

				if (!canUserAccessDstRegion)
				{
					LLNotification::Params params("TeleportOffered_MaturityBlocked");
					params.substitutions = args;
					params.payload = payload;
					LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
					send_simple_im(from_id, LLTrans::getString("TeleportMaturityExceeded"), IM_NOTHING_SPECIAL, session_id);
					send_simple_im(from_id, LLStringUtil::null, IM_LURE_DECLINED, session_id);
				}
				else if (doesUserRequireMaturityIncrease)
				{
					LLNotification::Params params("TeleportOffered_MaturityExceeded");
					params.substitutions = args;
					params.payload = payload;
					LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
				}
				else
				{
					LLNotification::Params params("TeleportOffered");
					params.substitutions = args;
					params.payload = payload;
					LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
				}

			}
		}
		break;

	case IM_GODLIKE_LURE_USER:
		{
			LLVector3 pos, look_at;
			U64 region_handle(0);
			U8 region_access(SIM_ACCESS_MIN);
			std::string region_info = ll_safe_string((char*)binary_bucket, binary_bucket_size);
			std::string region_access_str = LLStringUtil::null;
			std::string region_access_icn = LLStringUtil::null;
			std::string region_access_lc  = LLStringUtil::null;

			bool canUserAccessDstRegion = true;
			bool doesUserRequireMaturityIncrease = false;

			if (parse_lure_bucket(region_info, region_handle, pos, look_at, region_access))
			{
				region_access_str = LLViewerRegion::accessToString(region_access);
				region_access_icn = LLViewerRegion::getAccessIcon(region_access);
				region_access_lc  = region_access_str;
				LLStringUtil::toLower(region_access_lc);

				if (!gAgent.isGodlike())
				{
					switch (region_access)
					{
					case SIM_ACCESS_MIN :
					case SIM_ACCESS_PG :
						break;
					case SIM_ACCESS_MATURE :
						if (gAgent.isTeen())
						{
							canUserAccessDstRegion = false;
						}
						else if (gAgent.prefersPG())
						{
							doesUserRequireMaturityIncrease = true;
						}
						break;
					case SIM_ACCESS_ADULT :
						if (!gAgent.isAdult())
						{
							canUserAccessDstRegion = false;
						}
						else if (!gAgent.prefersAdult())
						{
							doesUserRequireMaturityIncrease = true;
						}
						break;
					default :
						llassert(0);
						break;
					}
				}
			}

			LLSD args;
			// *TODO: Translate -> [FIRST] [LAST] (maybe)
			args["NAME_SLURL"] = LLSLURL("agent", from_id, "about").getSLURLString();
			args["MESSAGE"] = message;
			args["MATURITY_STR"] = region_access_str;
			args["MATURITY_ICON"] = region_access_icn;
			args["REGION_CONTENT_MATURITY"] = region_access_lc;
			LLSD payload;
			payload["from_id"] = from_id;
			payload["lure_id"] = session_id;
			payload["godlike"] = TRUE;
			payload["region_maturity"] = region_access;

			if (!canUserAccessDstRegion)
			{
				LLNotification::Params params("TeleportOffered_MaturityBlocked");
				params.substitutions = args;
				params.payload = payload;
				LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
				send_simple_im(from_id, LLTrans::getString("TeleportMaturityExceeded"), IM_NOTHING_SPECIAL, session_id);
				send_simple_im(from_id, LLStringUtil::null, IM_LURE_DECLINED, session_id);
			}
			else if (doesUserRequireMaturityIncrease)
			{
				LLNotification::Params params("TeleportOffered_MaturityExceeded");
				params.substitutions = args;
				params.payload = payload;
				LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
			}
			else
			{
				// do not show a message box, because you're about to be
				// teleported.
				LLNotifications::instance().forceResponse(LLNotification::Params("TeleportOffered").payload(payload), 0);
			}
		}
		break;

	case IM_GOTO_URL:
		{
			LLSD args;
			// n.b. this is for URLs sent by the system, not for
			// URLs sent by scripts (i.e. llLoadURL)
			if (binary_bucket_size <= 0)
			{
				LL_WARNS("Messaging") << "bad binary_bucket_size: "
					<< binary_bucket_size
					<< " - aborting function." << LL_ENDL;
				return;
			}

			std::string url;
			
			url.assign((char*)binary_bucket, binary_bucket_size-1);
			args["MESSAGE"] = message;
			args["URL"] = url;
			LLSD payload;
			payload["url"] = url;
			LLNotificationsUtil::add("GotoURL", args, payload );
		}
		break;

	case IM_FRIENDSHIP_OFFERED:
		{
			LLSD payload;
			payload["from_id"] = from_id;
			payload["session_id"] = session_id;;
			payload["online"] = (offline == IM_ONLINE);
			payload["sender"] = msg->getSender().getIPandPort();

			if (is_busy)
			{
				busy_message(msg, from_id);
				LLNotifications::instance().forceResponse(LLNotification::Params("OfferFriendship").payload(payload), 1);
			}
			else if (is_muted)
			{
				LLNotifications::instance().forceResponse(LLNotification::Params("OfferFriendship").payload(payload), 1);
			}
			else
			{
				args["NAME_SLURL"] = LLSLURL("agent", from_id, "about").getSLURLString();
				if(message.empty())
				{
					//support for frienship offers from clients before July 2008
				        LLNotificationsUtil::add("OfferFriendshipNoMessage", args, payload);
				}
				else
				{
					args["[MESSAGE]"] = message;
				    LLNotification::Params params("OfferFriendship");
				    params.substitutions = args;
				    params.payload = payload;
				    LLPostponedNotification::add<LLPostponedOfferNotification>(	params, from_id, false);
				}
			}
		}
		break;

	case IM_FRIENDSHIP_ACCEPTED:
		{
			// In the case of an offline IM, the formFriendship() may be extraneous
			// as the database should already include the relationship.  But it
			// doesn't hurt for dupes.
			LLAvatarTracker::formFriendship(from_id);
			
			std::vector<std::string> strings;
			strings.push_back(from_id.asString());
			send_generic_message("requestonlinenotification", strings);
			
			args["NAME"] = name;
			LLSD payload;
			payload["from_id"] = from_id;
			LLAvatarNameCache::get(from_id, boost::bind(&notification_display_name_callback,
														 _1,
														 _2,
														 "FriendshipAccepted",
														 args,
														 payload));
		}
		break;

	case IM_FRIENDSHIP_DECLINED_DEPRECATED:
	default:
		LL_WARNS("Messaging") << "Instant message calling for unknown dialog "
				<< (S32)dialog << LL_ENDL;
		break;
	}

	LLWindow* viewer_window = gViewerWindow->getWindow();
	if (viewer_window && viewer_window->getMinimized())
	{
		viewer_window->flashIcon(5.f);
	}
}

void busy_message (LLMessageSystem* msg, LLUUID from_id) 
{
	if (gAgent.getBusy())
	{
		std::string my_name;
		LLAgentUI::buildFullname(my_name);
		std::string response = gSavedPerAccountSettings.getString("BusyModeResponse");
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			from_id,
			my_name,
			response,
			IM_ONLINE,
			IM_BUSY_AUTO_RESPONSE);
		gAgent.sendReliableMessage();
	}
}

bool callingcard_offer_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLUUID fid;
	LLUUID from_id;
	LLMessageSystem* msg = gMessageSystem;
	switch(option)
	{
	case 0:
		// accept
		msg->newMessageFast(_PREHASH_AcceptCallingCard);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, notification["payload"]["transaction_id"].asUUID());
		fid = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, fid);
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
		break;
	case 1:
		// decline		
		msg->newMessageFast(_PREHASH_DeclineCallingCard);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, notification["payload"]["transaction_id"].asUUID());
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
		busy_message(msg, notification["payload"]["source_id"].asUUID());
		break;
	default:
		// close button probably, possibly timed out
		break;
	}

	return false;
}
static LLNotificationFunctorRegistration callingcard_offer_cb_reg("OfferCallingCard", callingcard_offer_callback);

void process_offer_callingcard(LLMessageSystem* msg, void**)
{
	// someone has offered to form a friendship
	LL_DEBUGS("Messaging") << "callingcard offer" << LL_ENDL;

	LLUUID source_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, source_id);
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_TransactionID, tid);

	LLSD payload;
	payload["transaction_id"] = tid;
	payload["source_id"] = source_id;
	payload["sender"] = msg->getSender().getIPandPort();

	LLViewerObject* source = gObjectList.findObject(source_id);
	LLSD args;
	std::string source_name;
	if(source && source->isAvatar())
	{
		LLNameValue* nvfirst = source->getNVPair("FirstName");
		LLNameValue* nvlast  = source->getNVPair("LastName");
		if (nvfirst && nvlast)
		{
			source_name = LLCacheName::buildFullName(
				nvfirst->getString(), nvlast->getString());
		}
	}

	if(!source_name.empty())
	{
		if (gAgent.getBusy() 
			|| LLMuteList::getInstance()->isMuted(source_id, source_name, LLMute::flagTextChat))
		{
			// automatically decline offer
			LLNotifications::instance().forceResponse(LLNotification::Params("OfferCallingCard").payload(payload), 1);
		}
		else
		{
			args["NAME"] = source_name;
			LLNotificationsUtil::add("OfferCallingCard", args, payload);
		}
	}
	else
	{
		LL_WARNS("Messaging") << "Calling card offer from an unknown source." << LL_ENDL;
	}
}

void process_accept_callingcard(LLMessageSystem* msg, void**)
{
	LLNotificationsUtil::add("CallingCardAccepted");
}

void process_decline_callingcard(LLMessageSystem* msg, void**)
{
	LLNotificationsUtil::add("CallingCardDeclined");
}

class ChatTranslationReceiver : public LLTranslate::TranslationReceiver
{
public :
	ChatTranslationReceiver(const std::string &from_lang, const std::string &to_lang, const std::string &mesg,
							const LLChat &chat, const LLSD &toast_args)
		: LLTranslate::TranslationReceiver(from_lang, to_lang),
		m_chat(chat),
		m_toastArgs(toast_args),
		m_origMesg(mesg)
	{
	}

	static boost::intrusive_ptr<ChatTranslationReceiver> build(const std::string &from_lang, const std::string &to_lang, const std::string &mesg, const LLChat &chat, const LLSD &toast_args)
	{
		return boost::intrusive_ptr<ChatTranslationReceiver>(new ChatTranslationReceiver(from_lang, to_lang, mesg, chat, toast_args));
	}

protected:
	void handleResponse(const std::string &translation, const std::string &detected_language)
	{
		// filter out non-interesting responeses
		if ( !translation.empty()
			&& (mToLang != detected_language)
			&& (LLStringUtil::compareInsensitive(translation, m_origMesg) != 0) )
		{
			m_chat.mText += " (" + translation + ")";
		}

		LLNotificationsUI::LLNotificationManager::instance().onChat(m_chat, m_toastArgs);
	}

	void handleFailure(int status, const std::string& err_msg)
	{
		llwarns << "Translation failed for mesg " << m_origMesg << " toLang " << mToLang << " fromLang " << mFromLang << llendl;

		std::string msg = LLTrans::getString("TranslationFailed", LLSD().with("[REASON]", err_msg));
		LLStringUtil::replaceString(msg, "\n", " "); // we want one-line error messages
		m_chat.mText += " (" + msg + ")";

		LLNotificationsUI::LLNotificationManager::instance().onChat(m_chat, m_toastArgs);
	}

private:
	LLChat m_chat;
	std::string m_origMesg;
	LLSD m_toastArgs;		
};
void process_chat_from_simulator(LLMessageSystem *msg, void **user_data)
{
	LLChat	chat;
	std::string		mesg;
	std::string		from_name;
	U8			source_temp;
	U8			type_temp;
	U8			audible_temp;
	LLColor4	color(1.0f, 1.0f, 1.0f, 1.0f);
	LLUUID		from_id;
	LLUUID		owner_id;
	BOOL		is_owned_by_me = FALSE;
	LLViewerObject*	chatter;

	msg->getString("ChatData", "FromName", from_name);
	
	msg->getUUID("ChatData", "SourceID", from_id);
	chat.mFromID = from_id;
	
	// Object owner for objects
	msg->getUUID("ChatData", "OwnerID", owner_id);

	msg->getU8Fast(_PREHASH_ChatData, _PREHASH_SourceType, source_temp);
	chat.mSourceType = (EChatSourceType)source_temp;

	msg->getU8("ChatData", "ChatType", type_temp);
	chat.mChatType = (EChatType)type_temp;

	msg->getU8Fast(_PREHASH_ChatData, _PREHASH_Audible, audible_temp);
	chat.mAudible = (EChatAudible)audible_temp;
	
	chat.mTime = LLFrameTimer::getElapsedSeconds();
	
	// IDEVO Correct for new-style "Resident" names
	if (chat.mSourceType == CHAT_SOURCE_AGENT)
	{
		// I don't know if it's OK to change this here, if 
		// anything downstream does lookups by name, for instance
		
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(from_id, &av_name))
		{
			chat.mFromName = av_name.mDisplayName;
		}
		else
		{
			chat.mFromName = LLCacheName::cleanFullName(from_name);
		}
	}
	else
	{
		chat.mFromName = from_name;
	}

	BOOL is_busy = gAgent.getBusy();

	BOOL is_muted = FALSE;
	BOOL is_linden = FALSE;
	is_muted = LLMuteList::getInstance()->isMuted(
		from_id,
		from_name,
		LLMute::flagTextChat) 
		|| LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagTextChat);
	is_linden = chat.mSourceType != CHAT_SOURCE_OBJECT &&
		LLMuteList::getInstance()->isLinden(from_name);

	BOOL is_audible = (CHAT_AUDIBLE_FULLY == chat.mAudible);
	chatter = gObjectList.findObject(from_id);
	if (chatter)
	{
		chat.mPosAgent = chatter->getPositionAgent();

		// Make swirly things only for talking objects. (not script debug messages, though)
		if (chat.mSourceType == CHAT_SOURCE_OBJECT 
			&& chat.mChatType != CHAT_TYPE_DEBUG_MSG
			&& gSavedSettings.getBOOL("EffectScriptChatParticles") )
		{
			LLPointer<LLViewerPartSourceChat> psc = new LLViewerPartSourceChat(chatter->getPositionAgent());
			psc->setSourceObject(chatter);
			psc->setColor(color);
			//We set the particles to be owned by the object's owner, 
			//just in case they should be muted by the mute list
			psc->setOwnerUUID(owner_id);
			LLViewerPartSim::getInstance()->addPartSource(psc);
		}

		// record last audible utterance
		if (is_audible
			&& (is_linden || (!is_muted && !is_busy)))
		{
			if (chat.mChatType != CHAT_TYPE_START 
				&& chat.mChatType != CHAT_TYPE_STOP)
			{
				gAgent.heardChat(chat.mFromID);
			}
		}

		is_owned_by_me = chatter->permYouOwner();
	}

	if (is_audible)
	{
		BOOL visible_in_chat_bubble = FALSE;

		color.setVec(1.f,1.f,1.f,1.f);
		msg->getStringFast(_PREHASH_ChatData, _PREHASH_Message, mesg);

		BOOL ircstyle = FALSE;

		// Look for IRC-style emotes here so chatbubbles work
		std::string prefix = mesg.substr(0, 4);
		if (prefix == "/me " || prefix == "/me'")
		{
			ircstyle = TRUE;
		}
		chat.mText = mesg;

		// Look for the start of typing so we can put "..." in the bubbles.
		if (CHAT_TYPE_START == chat.mChatType)
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, TRUE);

			// Might not have the avatar constructed yet, eg on login.
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->startTyping();
			}
			return;
		}
		else if (CHAT_TYPE_STOP == chat.mChatType)
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, FALSE);

			// Might not have the avatar constructed yet, eg on login.
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->stopTyping();
			}
			return;
		}

		// Look for IRC-style emotes
		if (ircstyle)
		{
			// set CHAT_STYLE_IRC to avoid adding Avatar Name as author of message. See EXT-656
			chat.mChatStyle = CHAT_STYLE_IRC;

			// Do nothing, ircstyle is fixed above for chat bubbles
		}
		else
		{
			chat.mText = "";
			switch(chat.mChatType)
			{
			case CHAT_TYPE_WHISPER:
				chat.mText = LLTrans::getString("whisper") + " ";
				break;
			case CHAT_TYPE_DEBUG_MSG:
			case CHAT_TYPE_OWNER:
			case CHAT_TYPE_NORMAL:
			case CHAT_TYPE_DIRECT:
				break;
			case CHAT_TYPE_SHOUT:
				chat.mText = LLTrans::getString("shout") + " ";
				break;
			case CHAT_TYPE_START:
			case CHAT_TYPE_STOP:
				LL_WARNS("Messaging") << "Got chat type start/stop in main chat processing." << LL_ENDL;
				break;
			default:
				LL_WARNS("Messaging") << "Unknown type " << chat.mChatType << " in chat!" << LL_ENDL;
				break;
			}

			chat.mText += mesg;
		}
		
		// We have a real utterance now, so can stop showing "..." and proceed.
		if (chatter && chatter->isAvatar())
		{
			LLLocalSpeakerMgr::getInstance()->setSpeakerTyping(from_id, FALSE);
			((LLVOAvatar*)chatter)->stopTyping();
			
			if (!is_muted && !is_busy)
			{
				visible_in_chat_bubble = gSavedSettings.getBOOL("UseChatBubbles");
				std::string formated_msg = "";
				LLViewerChat::formatChatMsg(chat, formated_msg);
				LLChat chat_bubble = chat;
				chat_bubble.mText = formated_msg;
				((LLVOAvatar*)chatter)->addChat(chat_bubble);
			}
		}
		
		if (chatter)
		{
			chat.mPosAgent = chatter->getPositionAgent();
		}

		// truth table:
		// LINDEN	BUSY	MUTED	OWNED_BY_YOU	TASK		DISPLAY		STORE IN HISTORY
		// F		F		F		F				*			Yes			Yes
		// F		F		F		T				*			Yes			Yes
		// F		F		T		F				*			No			No
		// F		F		T		T				*			No			No
		// F		T		F		F				*			No			Yes
		// F		T		F		T				*			Yes			Yes
		// F		T		T		F				*			No			No
		// F		T		T		T				*			No			No
		// T		*		*		*				F			Yes			Yes

		chat.mMuted = is_muted && !is_linden;

		// pass owner_id to chat so that we can display the remote
		// object inspect for an object that is chatting with you
		LLSD args;
		args["type"] = LLNotificationsUI::NT_NEARBYCHAT;
		chat.mOwnerID = owner_id;

		if (gSavedSettings.getBOOL("TranslateChat") && chat.mSourceType != CHAT_SOURCE_SYSTEM)
		{
			if (chat.mChatStyle == CHAT_STYLE_IRC)
			{
				mesg = mesg.substr(4, std::string::npos);
			}
			const std::string from_lang = ""; // leave empty to trigger autodetect
			const std::string to_lang = LLTranslate::getTranslateLanguage();

			LLTranslate::TranslationReceiverPtr result = ChatTranslationReceiver::build(from_lang, to_lang, mesg, chat, args);
			LLTranslate::translateMessage(result, from_lang, to_lang, mesg);
		}
		else
		{
			LLNotificationsUI::LLNotificationManager::instance().onChat(chat, args);
		}
	}
}


// Simulator we're on is informing the viewer that the agent
// is starting to teleport (perhaps to another sim, perhaps to the 
// same sim). If we initiated the teleport process by sending some kind 
// of TeleportRequest, then this info is redundant, but if the sim 
// initiated the teleport (via a script call, being killed, etc.) 
// then this info is news to us.
void process_teleport_start(LLMessageSystem *msg, void**)
{
	// on teleport, don't tell them about destination guide anymore
	LLFirstUse::notUsingDestinationGuide(false);
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);

	LL_DEBUGS("Messaging") << "Got TeleportStart with TeleportFlags=" << teleport_flags << ". gTeleportDisplay: " << gTeleportDisplay << ", gAgent.mTeleportState: " << gAgent.getTeleportState() << LL_ENDL;

	// *NOTE: The server sends two StartTeleport packets when you are teleporting to a LM
	LLViewerMessage::getInstance()->mTeleportStartedSignal();

	if (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL)
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE);
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, LLTrans::getString("Cancel"));
	}

	// Freeze the UI and show progress bar
	// Note: could add data here to differentiate between normal teleport and death.

	if( gAgent.getTeleportState() == LLAgent::TELEPORT_NONE )
	{
		gTeleportDisplay = TRUE;
		gAgent.setTeleportState( LLAgent::TELEPORT_START );
		make_ui_sound("UISndTeleportOut");
		
		LL_INFOS("Messaging") << "Teleport initiated by remote TeleportStart message with TeleportFlags: " <<  teleport_flags << LL_ENDL;

		// Don't call LLFirstUse::useTeleport here because this could be
		// due to being killed, which would send you home, not to a Telehub
	}
}

boost::signals2::connection LLViewerMessage::setTeleportStartedCallback(teleport_started_callback_t cb)
{
	return mTeleportStartedSignal.connect(cb);
}

void process_teleport_progress(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if((gAgent.getID() != agent_id)
	   || (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE))
	{
		LL_WARNS("Messaging") << "Unexpected teleport progress message." << LL_ENDL;
		return;
	}
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);
	if (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL)
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE);
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, LLTrans::getString("Cancel"));
	}
	std::string buffer;
	msg->getString("Info", "Message", buffer);
	LL_DEBUGS("Messaging") << "teleport progress: " << buffer << LL_ENDL;

	//Sorta hacky...default to using simulator raw messages
	//if we don't find the coresponding mapping in our progress mappings
	std::string message = buffer;

	if (LLAgent::sTeleportProgressMessages.find(buffer) != 
		LLAgent::sTeleportProgressMessages.end() )
	{
		message = LLAgent::sTeleportProgressMessages[buffer];
	}

	gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages[message]);
}

class LLFetchInWelcomeArea : public LLInventoryFetchDescendentsObserver
{
public:
	LLFetchInWelcomeArea(const uuid_vec_t &ids) :
		LLInventoryFetchDescendentsObserver(ids)
	{}
	virtual void done()
	{
		LLIsType is_landmark(LLAssetType::AT_LANDMARK);
		LLIsType is_card(LLAssetType::AT_CALLINGCARD);

		LLInventoryModel::cat_array_t	card_cats;
		LLInventoryModel::item_array_t	card_items;
		LLInventoryModel::cat_array_t	land_cats;
		LLInventoryModel::item_array_t	land_items;

		uuid_vec_t::iterator it = mComplete.begin();
		uuid_vec_t::iterator end = mComplete.end();
		for(; it != end; ++it)
		{
			gInventory.collectDescendentsIf(
				(*it),
				land_cats,
				land_items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_landmark);
			gInventory.collectDescendentsIf(
				(*it),
				card_cats,
				card_items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_card);
		}
		LLSD args;
		if ( land_items.count() > 0 )
		{	// Show notification that they can now teleport to landmarks.  Use a random landmark from the inventory
			S32 random_land = ll_rand( land_items.count() - 1 );
			args["NAME"] = land_items[random_land]->getName();
			LLNotificationsUtil::add("TeleportToLandmark",args);
		}
		if ( card_items.count() > 0 )
		{	// Show notification that they can now contact people.  Use a random calling card from the inventory
			S32 random_card = ll_rand( card_items.count() - 1 );
			args["NAME"] = card_items[random_card]->getName();
			LLNotificationsUtil::add("TeleportToPerson",args);
		}

		gInventory.removeObserver(this);
		delete this;
	}
};



class LLPostTeleportNotifiers : public LLEventTimer 
{
public:
	LLPostTeleportNotifiers();
	virtual ~LLPostTeleportNotifiers();

	//function to be called at the supplied frequency
	virtual BOOL tick();
};

LLPostTeleportNotifiers::LLPostTeleportNotifiers() : LLEventTimer( 2.0 )
{
};

LLPostTeleportNotifiers::~LLPostTeleportNotifiers()
{
}

BOOL LLPostTeleportNotifiers::tick()
{
	BOOL all_done = FALSE;
	if ( gAgent.getTeleportState() == LLAgent::TELEPORT_NONE )
	{
		// get callingcards and landmarks available to the user arriving.
		uuid_vec_t folders;
		const LLUUID callingcard_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_CALLINGCARD);
		if(callingcard_id.notNull()) 
			folders.push_back(callingcard_id);
		const LLUUID folder_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_LANDMARK);
		if(folder_id.notNull()) 
			folders.push_back(folder_id);
		if(!folders.empty())
		{
			LLFetchInWelcomeArea* fetcher = new LLFetchInWelcomeArea(folders);
			fetcher->startFetch();
			if(fetcher->isFinished())
			{
				fetcher->done();
			}
			else
			{
				gInventory.addObserver(fetcher);
			}
		}
		all_done = TRUE;
	}

	return all_done;
}



// Teleport notification from the simulator
// We're going to pretend to be a new agent
void process_teleport_finish(LLMessageSystem* msg, void**)
{
	LL_DEBUGS("Messaging") << "Got teleport location message" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_Info, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Got teleport notification for wrong agent!" << LL_ENDL;
		return;
	}
	
	// Teleport is finished; it can't be cancelled now.
	gViewerWindow->setProgressCancelButtonVisible(FALSE);

	// Do teleport effect for where you're leaving
	// VEFFECT: TeleportStart
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	LLHUDManager::getInstance()->sendEffects();

	U32 location_id;
	U32 sim_ip;
	U16 sim_port;
	LLVector3 pos, look_at;
	U64 region_handle;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_LocationID, location_id);
	msg->getIPAddrFast(_PREHASH_Info, _PREHASH_SimIP, sim_ip);
	msg->getIPPortFast(_PREHASH_Info, _PREHASH_SimPort, sim_port);
	//msg->getVector3Fast(_PREHASH_Info, _PREHASH_Position, pos);
	//msg->getVector3Fast(_PREHASH_Info, _PREHASH_LookAt, look_at);
	msg->getU64Fast(_PREHASH_Info, _PREHASH_RegionHandle, region_handle);
	U32 teleport_flags;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_TeleportFlags, teleport_flags);
	
	
	std::string seedCap;
	msg->getStringFast(_PREHASH_Info, _PREHASH_SeedCapability, seedCap);

	// update home location if we are teleporting out of prelude - specific to teleporting to welcome area 
	if((teleport_flags & TELEPORT_FLAGS_SET_HOME_TO_TARGET)
	   && (!gAgent.isGodlike()))
	{
		gAgent.setHomePosRegion(region_handle, pos);

		// Create a timer that will send notices when teleporting is all finished.  Since this is 
		// based on the LLEventTimer class, it will be managed by that class and not orphaned or leaked.
		new LLPostTeleportNotifiers();
	}

	LLHost sim_host(sim_ip, sim_port);

	// Viewer trusts the simulator.
	gMessageSystem->enableCircuit(sim_host, TRUE);
	LLViewerRegion* regionp =  LLWorld::getInstance()->addRegion(region_handle, sim_host);

/*
	// send camera update to new region
	gAgentCamera.updateCamera();

	// likewise make sure the camera is behind the avatar
	gAgentCamera.resetView(TRUE);
	LLVector3 shift_vector = regionp->getPosRegionFromGlobal(gAgent.getRegion()->getOriginGlobal());
	gAgent.setRegion(regionp);
	gObjectList.shiftObjects(shift_vector);

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->clearChatText();
		gAgentCamera.slamLookAt(look_at);
	}
	gAgent.setPositionAgent(pos);
	gAssetStorage->setUpstream(sim);
	gCacheName->setUpstream(sim);
*/

	// Make sure we're standing
	gAgent.standUp();

	// now, use the circuit info to tell simulator about us!
	LL_INFOS("Messaging") << "process_teleport_finish() Enabling "
			<< sim_host << " with code " << msg->mOurCircuitCode << LL_ENDL;
	msg->newMessageFast(_PREHASH_UseCircuitCode);
	msg->nextBlockFast(_PREHASH_CircuitCode);
	msg->addU32Fast(_PREHASH_Code, msg->getOurCircuitCode());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_ID, gAgent.getID());
	msg->sendReliable(sim_host);

	send_complete_agent_movement(sim_host);
	gAgent.setTeleportState( LLAgent::TELEPORT_MOVING );
	gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["contacting"]);

	regionp->setSeedCapability(seedCap);

	// Don't send camera updates to the new region until we're
	// actually there...


	// Now do teleport effect for where you're going.
	// VEFFECT: TeleportEnd
	effectp = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());

	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	LLHUDManager::getInstance()->sendEffects();

//	gTeleportDisplay = TRUE;
//	gTeleportDisplayTimer.reset();
//	gViewerWindow->setShowProgress(TRUE);
}

// stuff we have to do every time we get an AvatarInitComplete from a sim
/*
void process_avatar_init_complete(LLMessageSystem* msg, void**)
{
	LLVector3 agent_pos;
	msg->getVector3Fast(_PREHASH_AvatarData, _PREHASH_Position, agent_pos);
	agent_movement_complete(msg->getSender(), agent_pos);
}
*/

void process_agent_movement_complete(LLMessageSystem* msg, void**)
{
	gAgentMovementCompleted = true;

	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		LL_WARNS("Messaging") << "Incorrect id in process_agent_movement_complete()"
				<< LL_ENDL;
		return;
	}

	LL_DEBUGS("Messaging") << "process_agent_movement_complete()" << LL_ENDL;

	// *TODO: check timestamp to make sure the movement compleation
	// makes sense.
	LLVector3 agent_pos;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_Position, agent_pos);
	LLVector3 look_at;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_LookAt, look_at);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_Data, _PREHASH_RegionHandle, region_handle);
	
	std::string version_channel;
	msg->getString("SimData", "ChannelVersion", version_channel);

	if (!isAgentAvatarValid())
	{
		// Could happen if you were immediately god-teleported away on login,
		// maybe other cases.  Continue, but warn.
		LL_WARNS("Messaging") << "agent_movement_complete() with NULL avatarp." << LL_ENDL;
	}

	F32 x, y;
	from_region_handle(region_handle, &x, &y);
	LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromHandle(region_handle);
	if (!regionp)
	{
		if (gAgent.getRegion())
		{
			LL_WARNS("Messaging") << "current region " << gAgent.getRegion()->getOriginGlobal() << LL_ENDL;
		}

		LL_WARNS("Messaging") << "Agent being sent to invalid home region: " 
			<< x << ":" << y 
			<< " current pos " << gAgent.getPositionGlobal()
			<< LL_ENDL;
		LLAppViewer::instance()->forceDisconnect(LLTrans::getString("SentToInvalidRegion"));
		return;

	}

	LL_INFOS("Messaging") << "Changing home region to " << x << ":" << y << LL_ENDL;

	// set our upstream host the new simulator and shuffle things as
	// appropriate.
	LLVector3 shift_vector = regionp->getPosRegionFromGlobal(
		gAgent.getRegion()->getOriginGlobal());
	gAgent.setRegion(regionp);
	gObjectList.shiftObjects(shift_vector);
	gAssetStorage->setUpstream(msg->getSender());
	gCacheName->setUpstream(msg->getSender());
	gViewerThrottle.sendToSim();
	gViewerWindow->sendShapeToSim();

	bool is_teleport = gAgent.getTeleportState() == LLAgent::TELEPORT_MOVING;

	if( is_teleport )
	{
		if (gAgent.getTeleportKeepsLookAt())
		{
			// *NOTE: the LookAt data we get from the sim here doesn't
			// seem to be useful, so get it from the camera instead
			look_at = LLViewerCamera::getInstance()->getAtAxis();
		}
		// Force the camera back onto the agent, don't animate.
		gAgentCamera.setFocusOnAvatar(TRUE, FALSE);
		gAgentCamera.slamLookAt(look_at);
		gAgentCamera.updateCamera();

		gAgent.setTeleportState( LLAgent::TELEPORT_START_ARRIVAL );

		// set the appearance on teleport since the new sim does not
		// know what you look like.
		gAgent.sendAgentSetAppearance();

		if (isAgentAvatarValid())
		{
			// Chat the "back" SLURL. (DEV-4907)

			LLSLURL slurl;
			gAgent.getTeleportSourceSLURL(slurl);
			LLSD substitution = LLSD().with("[T_SLURL]", slurl.getSLURLString());
			std::string completed_from = LLAgent::sTeleportProgressMessages["completed_from"];
			LLStringUtil::format(completed_from, substitution);

			LLSD args;
			args["MESSAGE"] = completed_from;
			LLNotificationsUtil::add("SystemMessageTip", args);

			// Set the new position
			gAgentAvatarp->setPositionAgent(agent_pos);
			gAgentAvatarp->clearChat();
			gAgentAvatarp->slamPosition();
		}
	}
	else
	{
		// This is initial log-in or a region crossing
		gAgent.setTeleportState( LLAgent::TELEPORT_NONE );

		if(LLStartUp::getStartupState() < STATE_STARTED)
		{	// This is initial log-in, not a region crossing:
			// Set the camera looking ahead of the AV so send_agent_update() below 
			// will report the correct location to the server.
			LLVector3 look_at_point = look_at;
			look_at_point = agent_pos + look_at_point.rotVec(gAgent.getQuat());

			static LLVector3 up_direction(0.0f, 0.0f, 1.0f);
			LLViewerCamera::getInstance()->lookAt(agent_pos, look_at_point, up_direction);
		}
	}

	if ( LLTracker::isTracking(NULL) )
	{
		// Check distance to beacon, if < 5m, remove beacon
		LLVector3d beacon_pos = LLTracker::getTrackedPositionGlobal();
		LLVector3 beacon_dir(agent_pos.mV[VX] - (F32)fmod(beacon_pos.mdV[VX], 256.0), agent_pos.mV[VY] - (F32)fmod(beacon_pos.mdV[VY], 256.0), 0);
		if (beacon_dir.magVecSquared() < 25.f)
		{
			LLTracker::stopTracking(NULL);
		}
		else if ( is_teleport && !gAgent.getTeleportKeepsLookAt() )
		{
			//look at the beacon
			LLVector3 global_agent_pos = agent_pos;
			global_agent_pos[0] += x;
			global_agent_pos[1] += y;
			look_at = (LLVector3)beacon_pos - global_agent_pos;
			look_at.normVec();
			gAgentCamera.slamLookAt(look_at);
		}
	}

	// TODO: Put back a check for flying status! DK 12/19/05
	// Sim tells us whether the new position is off the ground
	/*
	if (teleport_flags & TELEPORT_FLAGS_IS_FLYING)
	{
		gAgent.setFlying(TRUE);
	}
	else
	{
		gAgent.setFlying(FALSE);
	}
	*/

	send_agent_update(TRUE, TRUE);

	if (gAgent.getRegion()->getBlockFly())
	{
		gAgent.setFlying(gAgent.canFly());
	}

	// force simulator to recognize busy state
	if (gAgent.getBusy())
	{
		gAgent.setBusy();
	}
	else
	{
		gAgent.clearBusy();
	}

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->mFootPlane.clearVec();
	}
	
	// send walk-vs-run status
	gAgent.sendWalkRun(gAgent.getRunning() || gAgent.getAlwaysRun());

	// If the server version has changed, display an info box and offer
	// to display the release notes, unless this is the initial log in.
	if (gLastVersionChannel == version_channel)
	{
		return;
	}

	gLastVersionChannel = version_channel;
}

void process_crossed_region(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		LL_WARNS("Messaging") << "Incorrect id in process_crossed_region()"
				<< LL_ENDL;
		return;
	}
	LL_INFOS("Messaging") << "process_crossed_region()" << LL_ENDL;
	gAgentAvatarp->resetRegionCrossingTimer();

	U32 sim_ip;
	msg->getIPAddrFast(_PREHASH_RegionData, _PREHASH_SimIP, sim_ip);
	U16 sim_port;
	msg->getIPPortFast(_PREHASH_RegionData, _PREHASH_SimPort, sim_port);
	LLHost sim_host(sim_ip, sim_port);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_RegionData, _PREHASH_RegionHandle, region_handle);
	
	std::string seedCap;
	msg->getStringFast(_PREHASH_RegionData, _PREHASH_SeedCapability, seedCap);

	send_complete_agent_movement(sim_host);

	LLViewerRegion* regionp = LLWorld::getInstance()->addRegion(region_handle, sim_host);
	regionp->setSeedCapability(seedCap);
}



// Sends avatar and camera information to simulator.
// Sent roughly once per frame, or 20 times per second, whichever is less often

const F32 THRESHOLD_HEAD_ROT_QDOT = 0.9997f;	// ~= 2.5 degrees -- if its less than this we need to update head_rot
const F32 MAX_HEAD_ROT_QDOT = 0.99999f;			// ~= 0.5 degrees -- if its greater than this then no need to update head_rot
												// between these values we delay the updates (but no more than one second)

static LLFastTimer::DeclareTimer FTM_AGENT_UPDATE_SEND("Send Message");

void send_agent_update(BOOL force_send, BOOL send_reliable)
{
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		// We don't care if they want to send an agent update, they're not allowed to until the simulator
		// that's the target is ready to receive them (after avatar_init_complete is received)
		return;
	}

	// We have already requested to log out.  Don't send agent updates.
	if(LLAppViewer::instance()->logoutRequestSent())
	{
		return;
	}

	// no region to send update to
	if(gAgent.getRegion() == NULL)
	{
		return;
	}

	const F32 TRANSLATE_THRESHOLD = 0.01f;

	// NOTA BENE: This is (intentionally?) using the small angle sine approximation to test for rotation
	//			  Plus, there is an extra 0.5 in the mix since the perpendicular between last_camera_at and getAtAxis() bisects cam_rot_change
	//			  Thus, we're actually testing against 0.2 degrees
	const F32 ROTATION_THRESHOLD = 0.1f * 2.f*F_PI/360.f;			//  Rotation thresh 0.2 deg, see note above

	const U8 DUP_MSGS = 1;				//  HACK!  number of times to repeat data on motionless agent

	//  Store data on last sent update so that if no changes, no send
	static LLVector3 last_camera_pos_agent, 
					 last_camera_at, 
					 last_camera_left,
					 last_camera_up;
	
	static LLVector3 cam_center_chg,
					 cam_rot_chg;

	static LLQuaternion last_head_rot;
	static U32 last_control_flags = 0;
	static U8 last_render_state;
	static U8 duplicate_count = 0;
	static F32 head_rot_chg = 1.0;
	static U8 last_flags;

	LLMessageSystem	*msg = gMessageSystem;
	LLVector3		camera_pos_agent;				// local to avatar's region
	U8				render_state;

	LLQuaternion body_rotation = gAgent.getFrameAgent().getQuaternion();
	LLQuaternion head_rotation = gAgent.getHeadRotation();

	camera_pos_agent = gAgentCamera.getCameraPositionAgent();

	render_state = gAgent.getRenderState();

	U32		control_flag_change = 0;
	U8		flag_change = 0;

	cam_center_chg = last_camera_pos_agent - camera_pos_agent;
	cam_rot_chg = last_camera_at - LLViewerCamera::getInstance()->getAtAxis();

	// If a modifier key is held down, turn off
	// LBUTTON and ML_LBUTTON so that using the camera (alt-key) doesn't
	// trigger a control event.
	U32 control_flags = gAgent.getControlFlags();

	MASK	key_mask = gKeyboard->currentMask(TRUE);

	if (key_mask & MASK_ALT || key_mask & MASK_CONTROL)
	{
		control_flags &= ~(	AGENT_CONTROL_LBUTTON_DOWN |
							AGENT_CONTROL_ML_LBUTTON_DOWN );
		control_flags |= 	AGENT_CONTROL_LBUTTON_UP |
							AGENT_CONTROL_ML_LBUTTON_UP ;
	}

	control_flag_change = last_control_flags ^ control_flags;

	U8 flags = AU_FLAGS_NONE;
	if (gAgent.isGroupTitleHidden())
	{
		flags |= AU_FLAGS_HIDETITLE;
	}
	if (gAgent.getAutoPilot())
	{
		flags |= AU_FLAGS_CLIENT_AUTOPILOT;
	}

	flag_change = last_flags ^ flags;

	head_rot_chg = dot(last_head_rot, head_rotation);

	//static S32 msg_number = 0;		// Used for diagnostic log messages

	if (force_send || 
		(cam_center_chg.magVec() > TRANSLATE_THRESHOLD) || 
		(head_rot_chg < THRESHOLD_HEAD_ROT_QDOT) ||	
		(last_render_state != render_state) ||
		(cam_rot_chg.magVec() > ROTATION_THRESHOLD) ||
		control_flag_change != 0 ||
		flag_change != 0)  
	{
		/* Diagnotics to show why we send the AgentUpdate message.  Also un-commment the msg_number code above and below this block
		msg_number += 1;
		if (head_rot_chg < THRESHOLD_HEAD_ROT_QDOT)
		{
			//LL_INFOS("Messaging") << " head rot " << head_rotation << LL_ENDL;
			LL_INFOS("Messaging") << "msg " << msg_number << ", frame " << LLFrameTimer::getFrameCount() << ", head_rot_chg " << head_rot_chg << LL_ENDL;
		}
		if (cam_rot_chg.magVec() > ROTATION_THRESHOLD) 
		{
			LL_INFOS("Messaging") << "msg " << msg_number << ", frame " << LLFrameTimer::getFrameCount() << ", cam rot " <<  cam_rot_chg.magVec() << LL_ENDL;
		}
		if (cam_center_chg.magVec() > TRANSLATE_THRESHOLD)
		{
			LL_INFOS("Messaging") << "msg " << msg_number << ", frame " << LLFrameTimer::getFrameCount() << ", cam center " << cam_center_chg.magVec() << LL_ENDL;
		}
//		if (drag_delta_chg.magVec() > TRANSLATE_THRESHOLD)
//		{
//			LL_INFOS("Messaging") << "drag delta " << drag_delta_chg.magVec() << LL_ENDL;
//		}
		if (control_flag_change)
		{
			LL_INFOS("Messaging") << "msg " << msg_number << ", frame " << LLFrameTimer::getFrameCount() << ", dcf = " << control_flag_change << LL_ENDL;
		}
		*/

		duplicate_count = 0;
	}
	else
	{
		duplicate_count++;

		if (head_rot_chg < MAX_HEAD_ROT_QDOT  &&  duplicate_count < AGENT_UPDATES_PER_SECOND)
		{
			// The head_rotation is sent for updating things like attached guns.
			// We only trigger a new update when head_rotation deviates beyond
			// some threshold from the last update, however this can break fine
			// adjustments when trying to aim an attached gun, so what we do here
			// (where we would normally skip sending an update when nothing has changed)
			// is gradually reduce the threshold to allow a better update to 
			// eventually get sent... should update to within 0.5 degrees in less 
			// than a second.
			if (head_rot_chg < THRESHOLD_HEAD_ROT_QDOT + (MAX_HEAD_ROT_QDOT - THRESHOLD_HEAD_ROT_QDOT) * duplicate_count / AGENT_UPDATES_PER_SECOND)
			{
				duplicate_count = 0;
			}
			else
			{
				return;
			}
		}
		else
		{
			return;
		}
	}

	if (duplicate_count < DUP_MSGS && !gDisconnected)
	{
		/* More diagnostics to count AgentUpdate messages
		static S32 update_sec = 0;
		static S32 update_count = 0;
		static S32 max_update_count = 0;
		S32 cur_sec = lltrunc( LLTimer::getTotalSeconds() );
		update_count += 1;
		if (cur_sec != update_sec)
		{
			if (update_sec != 0)
			{
				update_sec = cur_sec;
				//msg_number = 0;
				max_update_count = llmax(max_update_count, update_count);
				llinfos << "Sent " << update_count << " AgentUpdate messages per second, max is " << max_update_count << llendl;
			}
			update_sec = cur_sec;
			update_count = 0;
		}
		*/

		LLFastTimer t(FTM_AGENT_UPDATE_SEND);
		// Build the message
		msg->newMessageFast(_PREHASH_AgentUpdate);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addQuatFast(_PREHASH_BodyRotation, body_rotation);
		msg->addQuatFast(_PREHASH_HeadRotation, head_rotation);
		msg->addU8Fast(_PREHASH_State, render_state);
		msg->addU8Fast(_PREHASH_Flags, flags);

//		if (camera_pos_agent.mV[VY] > 255.f)
//		{
//			LL_INFOS("Messaging") << "Sending camera center " << camera_pos_agent << LL_ENDL;
//		}
		
		msg->addVector3Fast(_PREHASH_CameraCenter, camera_pos_agent);
		msg->addVector3Fast(_PREHASH_CameraAtAxis, LLViewerCamera::getInstance()->getAtAxis());
		msg->addVector3Fast(_PREHASH_CameraLeftAxis, LLViewerCamera::getInstance()->getLeftAxis());
		msg->addVector3Fast(_PREHASH_CameraUpAxis, LLViewerCamera::getInstance()->getUpAxis());
		msg->addF32Fast(_PREHASH_Far, gAgentCamera.mDrawDistance);
		
		msg->addU32Fast(_PREHASH_ControlFlags, control_flags);

		if (gDebugClicks)
		{
			if (control_flags & AGENT_CONTROL_LBUTTON_DOWN)
			{
				LL_INFOS("Messaging") << "AgentUpdate left button down" << LL_ENDL;
			}

			if (control_flags & AGENT_CONTROL_LBUTTON_UP)
			{
				LL_INFOS("Messaging") << "AgentUpdate left button up" << LL_ENDL;
			}
		}

		gAgent.enableControlFlagReset();

		if (!send_reliable)
		{
			gAgent.sendMessage();
		}
		else
		{
			gAgent.sendReliableMessage();
		}

//		LL_DEBUGS("Messaging") << "agent " << avatar_pos_agent << " cam " << camera_pos_agent << LL_ENDL;

		// Copy the old data 
		last_head_rot = head_rotation;
		last_render_state = render_state;
		last_camera_pos_agent = camera_pos_agent;
		last_camera_at = LLViewerCamera::getInstance()->getAtAxis();
		last_camera_left = LLViewerCamera::getInstance()->getLeftAxis();
		last_camera_up = LLViewerCamera::getInstance()->getUpAxis();
		last_control_flags = control_flags;
		last_flags = flags;
	}
}



// *TODO: Remove this dependency, or figure out a better way to handle
// this hack.
extern U32 gObjectBits;

void process_object_update(LLMessageSystem *mesgsys, void **user_data)
{	
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	// Update the data counters
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectBits += mesgsys->getReceiveCompressedSize() * 8;
	}
	else
	{
		gObjectBits += mesgsys->getReceiveSize() * 8;
	}

	// Update the object...
	gObjectList.processObjectUpdate(mesgsys, user_data, OUT_FULL);
}

void process_compressed_object_update(LLMessageSystem *mesgsys, void **user_data)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	// Update the data counters
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectBits += mesgsys->getReceiveCompressedSize() * 8;
	}
	else
	{
		gObjectBits += mesgsys->getReceiveSize() * 8;
	}

	// Update the object...
	gObjectList.processCompressedObjectUpdate(mesgsys, user_data, OUT_FULL_COMPRESSED);
}

void process_cached_object_update(LLMessageSystem *mesgsys, void **user_data)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	// Update the data counters
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectBits += mesgsys->getReceiveCompressedSize() * 8;
	}
	else
	{
		gObjectBits += mesgsys->getReceiveSize() * 8;
	}

	// Update the object...
	gObjectList.processCachedObjectUpdate(mesgsys, user_data, OUT_FULL_CACHED);
}


void process_terse_object_update_improved(LLMessageSystem *mesgsys, void **user_data)
{
	LLMemType mt(LLMemType::MTYPE_OBJECT);
	if (mesgsys->getReceiveCompressedSize())
	{
		gObjectBits += mesgsys->getReceiveCompressedSize() * 8;
	}
	else
	{
		gObjectBits += mesgsys->getReceiveSize() * 8;
	}

	gObjectList.processCompressedObjectUpdate(mesgsys, user_data, OUT_TERSE_IMPROVED);
}

static LLFastTimer::DeclareTimer FTM_PROCESS_OBJECTS("Process Objects");


void process_kill_object(LLMessageSystem *mesgsys, void **user_data)
{
	LLFastTimer t(FTM_PROCESS_OBJECTS);

	LLUUID		id;
	U32			local_id;
	S32			i;
	S32			num_objects;

	num_objects = mesgsys->getNumberOfBlocksFast(_PREHASH_ObjectData);

	for (i = 0; i < num_objects; i++)
	{
		mesgsys->getU32Fast(_PREHASH_ObjectData, _PREHASH_ID, local_id, i);

		LLViewerObjectList::getUUIDFromLocal(id,
											local_id,
											gMessageSystem->getSenderIP(),
											gMessageSystem->getSenderPort());
		if (id == LLUUID::null)
		{
			LL_DEBUGS("Messaging") << "Unknown kill for local " << local_id << LL_ENDL;
			gObjectList.mNumUnknownKills++;
			continue;
		}
		else
		{
			LL_DEBUGS("Messaging") << "Kill message for local " << local_id << LL_ENDL;
		}

		// ...don't kill the avatar
		if (!(id == gAgentID))
		{
			LLViewerObject *objectp = gObjectList.findObject(id);
			if (objectp)
			{
				// Display green bubble on kill
				if ( gShowObjectUpdates )
				{
					LLColor4 color(0.f,1.f,0.f,1.f);
					gPipeline.addDebugBlip(objectp->getPositionAgent(), color);
				}

				// Do the kill
				gObjectList.killObject(objectp);
			}
			else
			{
				LL_WARNS("Messaging") << "Object in UUID lookup, but not on object list in kill!" << LL_ENDL;
				gObjectList.mNumUnknownKills++;
			}
		}

		// We should remove the object from selection after it is marked dead by gObjectList to make LLToolGrab,
        // which is using the object, release the mouse capture correctly when the object dies.
        // See LLToolGrab::handleHoverActive() and LLToolGrab::handleHoverNonPhysical().
		LLSelectMgr::getInstance()->removeObjectFromSelections(id);

	}
}

void process_time_synch(LLMessageSystem *mesgsys, void **user_data)
{
	LLVector3 sun_direction;
	LLVector3 sun_ang_velocity;
	F32 phase;
	U64	space_time_usec;

    U32 seconds_per_day;
    U32 seconds_per_year;

	// "SimulatorViewerTimeMessage"
	mesgsys->getU64Fast(_PREHASH_TimeInfo, _PREHASH_UsecSinceStart, space_time_usec);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerDay, seconds_per_day);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerYear, seconds_per_year);

	// This should eventually be moved to an "UpdateHeavenlyBodies" message
	mesgsys->getF32Fast(_PREHASH_TimeInfo, _PREHASH_SunPhase, phase);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunDirection, sun_direction);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunAngVelocity, sun_ang_velocity);

	LLWorld::getInstance()->setSpaceTimeUSec(space_time_usec);

	LL_DEBUGS("Windlight Sync") << "Sun phase: " << phase << " rad = " << fmodf(phase / F_TWO_PI + 0.25, 1.f) * 24.f << " h" << LL_ENDL;

	gSky.setSunPhase(phase);
	gSky.setSunTargetDirection(sun_direction, sun_ang_velocity);
	if ( !(gSavedSettings.getBOOL("SkyOverrideSimSunPosition") || gSky.getOverrideSun()) )
	{
		gSky.setSunDirection(sun_direction, sun_ang_velocity);
	}
}

void process_sound_trigger(LLMessageSystem *msg, void **)
{
	if (!gAudiop) return;

	U64		region_handle = 0;
	F32		gain = 0;
	LLUUID	sound_id;
	LLUUID	owner_id;
	LLUUID	object_id;
	LLUUID	parent_id;
	LLVector3	pos_local;

	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_OwnerID, owner_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_ParentID, parent_id);
	msg->getU64Fast(_PREHASH_SoundData, _PREHASH_Handle, region_handle);
	msg->getVector3Fast(_PREHASH_SoundData, _PREHASH_Position, pos_local);
	msg->getF32Fast(_PREHASH_SoundData, _PREHASH_Gain, gain);

	// adjust sound location to true global coords
	LLVector3d	pos_global = from_region_handle(region_handle);
	pos_global.mdV[VX] += pos_local.mV[VX];
	pos_global.mdV[VY] += pos_local.mV[VY];
	pos_global.mdV[VZ] += pos_local.mV[VZ];

	// Don't play a trigger sound if you can't hear it due
	// to parcel "local audio only" settings.
	if (!LLViewerParcelMgr::getInstance()->canHearSound(pos_global)) return;

	// Don't play sounds triggered by someone you muted.
	if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	
	// Don't play sounds from an object you muted
	if (LLMuteList::getInstance()->isMuted(object_id)) return;

	// Don't play sounds from an object whose parent you muted
	if (parent_id.notNull()
		&& LLMuteList::getInstance()->isMuted(parent_id))
	{
		return;
	}

	// Don't play sounds from a region with maturity above current agent maturity
	if( !gAgent.canAccessMaturityInRegion( region_handle ) )
	{
		return;
	}
		
	// Don't play sounds from gestures if they are not enabled.
	if (object_id == owner_id && !gSavedSettings.getBOOL("EnableGestureSounds"))
	{
		return;
	}

	gAudiop->triggerSound(sound_id, owner_id, gain, LLAudioEngine::AUDIO_TYPE_SFX, pos_global);
}

void process_preload_sound(LLMessageSystem *msg, void **user_data)
{
	if (!gAudiop)
	{
		return;
	}

	LLUUID sound_id;
	LLUUID object_id;
	LLUUID owner_id;

	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_OwnerID, owner_id);

	LLViewerObject *objectp = gObjectList.findObject(object_id);
	if (!objectp) return;

	if (LLMuteList::getInstance()->isMuted(object_id)) return;
	if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	
	LLAudioSource *sourcep = objectp->getAudioSource(owner_id);
	if (!sourcep) return;
	
	LLAudioData *datap = gAudiop->getAudioData(sound_id);

	// Note that I don't actually do any loading of the
	// audio data into a buffer at this point, as it won't actually
	// help us out.

	// Don't play sounds from a region with maturity above current agent maturity
	LLVector3d pos_global = objectp->getPositionGlobal();
	if (gAgent.canAccessMaturityAtGlobal(pos_global))
	{
	// Add audioData starts a transfer internally.
	sourcep->addAudioData(datap, FALSE);
}
}

void process_attached_sound(LLMessageSystem *msg, void **user_data)
{
	F32 gain = 0;
	LLUUID sound_id;
	LLUUID object_id;
	LLUUID owner_id;
	U8 flags;

	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_SoundID, sound_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_id);
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_OwnerID, owner_id);
	msg->getF32Fast(_PREHASH_DataBlock, _PREHASH_Gain, gain);
	msg->getU8Fast(_PREHASH_DataBlock, _PREHASH_Flags, flags);

	LLViewerObject *objectp = gObjectList.findObject(object_id);
	if (!objectp)
	{
		// we don't know about this object, just bail
		return;
	}
	
	if (LLMuteList::getInstance()->isMuted(object_id)) return;
	
	if (LLMuteList::getInstance()->isMuted(owner_id, LLMute::flagObjectSounds)) return;

	
	// Don't play sounds from a region with maturity above current agent maturity
	LLVector3d pos = objectp->getPositionGlobal();
	if( !gAgent.canAccessMaturityAtGlobal(pos) )
	{
		return;
	}
	
	objectp->setAttachedSound(sound_id, owner_id, gain, flags);
}


void process_attached_sound_gain_change(LLMessageSystem *mesgsys, void **user_data)
{
	F32 gain = 0;
	LLUUID object_guid;
	LLViewerObject *objectp = NULL;

	mesgsys->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_guid);

	if (!((objectp = gObjectList.findObject(object_guid))))
	{
		// we don't know about this object, just bail
		return;
	}

 	mesgsys->getF32Fast(_PREHASH_DataBlock, _PREHASH_Gain, gain);

	objectp->adjustAudioGain(gain);
}


void process_health_message(LLMessageSystem *mesgsys, void **user_data)
{
	F32 health;

	mesgsys->getF32Fast(_PREHASH_HealthData, _PREHASH_Health, health);

	if (gStatusBar)
	{
		gStatusBar->setHealth((S32)health);
	}
}


void process_sim_stats(LLMessageSystem *msg, void **user_data)
{	
	S32 count = msg->getNumberOfBlocks("Stat");
	for (S32 i = 0; i < count; ++i)
	{
		U32 stat_id;
		F32 stat_value;
		msg->getU32("Stat", "StatID", stat_id, i);
		msg->getF32("Stat", "StatValue", stat_value, i);
		switch (stat_id)
		{
		case LL_SIM_STAT_TIME_DILATION:
			LLViewerStats::getInstance()->mSimTimeDilation.addValue(stat_value);
			break;
		case LL_SIM_STAT_FPS:
			LLViewerStats::getInstance()->mSimFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PHYSFPS:
			LLViewerStats::getInstance()->mSimPhysicsFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTUPS:
			LLViewerStats::getInstance()->mSimAgentUPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_FRAMEMS:
			LLViewerStats::getInstance()->mSimFrameMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NETMS:
			LLViewerStats::getInstance()->mSimNetMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMOTHERMS:
			LLViewerStats::getInstance()->mSimSimOtherMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSMS:
			LLViewerStats::getInstance()->mSimSimPhysicsMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTMS:
			LLViewerStats::getInstance()->mSimAgentMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_IMAGESMS:
			LLViewerStats::getInstance()->mSimImagesMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SCRIPTMS:
			LLViewerStats::getInstance()->mSimScriptMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKS:
			LLViewerStats::getInstance()->mSimObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKSACTIVE:
			LLViewerStats::getInstance()->mSimActiveObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTMAIN:
			LLViewerStats::getInstance()->mSimMainAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTCHILD:
			LLViewerStats::getInstance()->mSimChildAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMSCRIPTSACTIVE:
			LLViewerStats::getInstance()->mSimActiveScripts.addValue(stat_value);
			break;
		case LL_SIM_STAT_SCRIPT_EPS:
			LLViewerStats::getInstance()->mSimScriptEPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_INPPS:
			LLViewerStats::getInstance()->mSimInPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_OUTPPS:
			LLViewerStats::getInstance()->mSimOutPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_DOWNLOADS:
			LLViewerStats::getInstance()->mSimPendingDownloads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_UPLOADS:
			LLViewerStats::getInstance()->mSimPendingUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_LOCAL_UPLOADS:
			LLViewerStats::getInstance()->mSimPendingLocalUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_TOTAL_UNACKED_BYTES:
			LLViewerStats::getInstance()->mSimTotalUnackedBytes.addValue(stat_value / 1024.f);
			break;
		case LL_SIM_STAT_PHYSICS_PINNED_TASKS:
			LLViewerStats::getInstance()->mPhysicsPinnedTasks.addValue(stat_value);
			break;
		case LL_SIM_STAT_PHYSICS_LOD_TASKS:
			LLViewerStats::getInstance()->mPhysicsLODTasks.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSSTEPMS:
			LLViewerStats::getInstance()->mSimSimPhysicsStepMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSSHAPEMS:
			LLViewerStats::getInstance()->mSimSimPhysicsShapeUpdateMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSOTHERMS:
			LLViewerStats::getInstance()->mSimSimPhysicsOtherMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSMEMORY:
			LLViewerStats::getInstance()->mPhysicsMemoryAllocated.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMSPARETIME:
			LLViewerStats::getInstance()->mSimSpareMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMSLEEPTIME:
			LLViewerStats::getInstance()->mSimSleepMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_IOPUMPTIME:
			LLViewerStats::getInstance()->mSimPumpIOMsec.addValue(stat_value);
			break;
		default:
			// Used to be a commented out warning.
 			LL_DEBUGS("Messaging") << "Unknown stat id" << stat_id << LL_ENDL;
		  break;
		}
	}

	/*
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_PhysicsTimeDilation, time_dilation);
	LLViewerStats::getInstance()->mSimTDStat.addValue(time_dilation);

	// Process information
	//	{	CpuUsage			F32				}
	//	{	SimMemTotal			F32				}
	//	{	SimMemRSS			F32				}
	//	{	ProcessUptime		F32				}
	F32 cpu_usage;
	F32 sim_mem_total;
	F32 sim_mem_rss;
	F32 process_uptime;
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_CpuUsage, cpu_usage);
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_SimMemTotal, sim_mem_total);
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_SimMemRSS, sim_mem_rss);
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_ProcessUptime, process_uptime);
	LLViewerStats::getInstance()->mSimCPUUsageStat.addValue(cpu_usage);
	LLViewerStats::getInstance()->mSimMemTotalStat.addValue(sim_mem_total);
	LLViewerStats::getInstance()->mSimMemRSSStat.addValue(sim_mem_rss);
	*/

	//
	// Various hacks that aren't statistics, but are being handled here.
	//
	U32 max_tasks_per_region;
	U32 region_flags;
	msg->getU32("Region", "ObjectCapacity", max_tasks_per_region);
	msg->getU32("Region", "RegionFlags", region_flags);

	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		BOOL was_flying = gAgent.getFlying();
		regionp->setRegionFlags(region_flags);
		regionp->setMaxTasks(max_tasks_per_region);
		// HACK: This makes agents drop from the sky if the region is 
		// set to no fly while people are still in the sim.
		if (was_flying && regionp->getBlockFly())
		{
			gAgent.setFlying(gAgent.canFly());
		}
	}
}



void process_avatar_animation(LLMessageSystem *mesgsys, void **user_data)
{
	LLUUID	animation_id;
	LLUUID	uuid;
	S32		anim_sequence_id;
	LLVOAvatar *avatarp;
	
	mesgsys->getUUIDFast(_PREHASH_Sender, _PREHASH_ID, uuid);

	//clear animation flags
	avatarp = (LLVOAvatar *)gObjectList.findObject(uuid);

	if (!avatarp)
	{
		// no agent by this ID...error?
		LL_WARNS("Messaging") << "Received animation state for unknown avatar" << uuid << LL_ENDL;
		return;
	}

	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationList);
	S32 num_source_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationSourceList);

	avatarp->mSignaledAnimations.clear();
	
	if (avatarp->isSelf())
	{
		LLUUID object_id;

		for( S32 i = 0; i < num_blocks; i++ )
		{
			mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
			mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);

			LL_DEBUGS("Messaging") << "Anim sequence ID: " << anim_sequence_id << LL_ENDL;

			avatarp->mSignaledAnimations[animation_id] = anim_sequence_id;

			// *HACK: Disabling flying mode if it has been enabled shortly before the agent
			// stand up animation is signaled. In this case we don't get a signal to start
			// flying animation from server, the AGENT_CONTROL_FLY flag remains set but the
			// avatar does not play flying animation, so we switch flying mode off.
			// See LLAgent::setFlying(). This may cause "Stop Flying" button to blink.
			// See EXT-2781.
			if (animation_id == ANIM_AGENT_STANDUP && gAgent.getFlying())
			{
				gAgent.setFlying(FALSE);
			}

			if (i < num_source_blocks)
			{
				mesgsys->getUUIDFast(_PREHASH_AnimationSourceList, _PREHASH_ObjectID, object_id, i);
			
				LLViewerObject* object = gObjectList.findObject(object_id);
				if (object)
				{
					object->mFlags |= FLAGS_ANIM_SOURCE;

					BOOL anim_found = FALSE;
					LLVOAvatar::AnimSourceIterator anim_it = avatarp->mAnimationSources.find(object_id);
					for (;anim_it != avatarp->mAnimationSources.end(); ++anim_it)
					{
						if (anim_it->second == animation_id)
						{
							anim_found = TRUE;
							break;
						}
					}

					if (!anim_found)
					{
						avatarp->mAnimationSources.insert(LLVOAvatar::AnimationSourceMap::value_type(object_id, animation_id));
					}
				}
			}
		}
	}
	else
	{
		for( S32 i = 0; i < num_blocks; i++ )
		{
			mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
			mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);
			avatarp->mSignaledAnimations[animation_id] = anim_sequence_id;
		}
	}

	if (num_blocks)
	{
		avatarp->processAnimationStateChanges();
	}
}

void process_avatar_appearance(LLMessageSystem *mesgsys, void **user_data)
{
	LLUUID uuid;
	mesgsys->getUUIDFast(_PREHASH_Sender, _PREHASH_ID, uuid);

	LLVOAvatar* avatarp = (LLVOAvatar *)gObjectList.findObject(uuid);
	if (avatarp)
	{
		avatarp->processAvatarAppearance( mesgsys );
	}
	else
	{
		LL_WARNS("Messaging") << "avatar_appearance sent for unknown avatar " << uuid << LL_ENDL;
	}
}

void process_camera_constraint(LLMessageSystem *mesgsys, void **user_data)
{
	LLVector4 cameraCollidePlane;
	mesgsys->getVector4Fast(_PREHASH_CameraCollidePlane, _PREHASH_Plane, cameraCollidePlane);

	gAgentCamera.setCameraCollidePlane(cameraCollidePlane);
}

void near_sit_object(BOOL success, void *data)
{
	if (success)
	{
		// Send message to sit on object
		gMessageSystem->newMessageFast(_PREHASH_AgentSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
	}
}

void process_avatar_sit_response(LLMessageSystem *mesgsys, void **user_data)
{
	LLVector3 sitPosition;
	LLQuaternion sitRotation;
	LLUUID sitObjectID;
	BOOL use_autopilot;
	mesgsys->getUUIDFast(_PREHASH_SitObject, _PREHASH_ID, sitObjectID);
	mesgsys->getBOOLFast(_PREHASH_SitTransform, _PREHASH_AutoPilot, use_autopilot);
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_SitPosition, sitPosition);
	mesgsys->getQuatFast(_PREHASH_SitTransform, _PREHASH_SitRotation, sitRotation);
	LLVector3 camera_eye;
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_CameraEyeOffset, camera_eye);
	LLVector3 camera_at;
	mesgsys->getVector3Fast(_PREHASH_SitTransform, _PREHASH_CameraAtOffset, camera_at);
	BOOL force_mouselook;
	mesgsys->getBOOLFast(_PREHASH_SitTransform, _PREHASH_ForceMouselook, force_mouselook);

	if (isAgentAvatarValid() && dist_vec_squared(camera_eye, camera_at) > CAMERA_POSITION_THRESHOLD_SQUARED)
	{
		gAgentCamera.setSitCamera(sitObjectID, camera_eye, camera_at);
	}
	
	gAgentCamera.setForceMouselook(force_mouselook);
	// Forcing turning off flying here to prevent flying after pressing "Stand"
	// to stand up from an object. See EXT-1655.
	gAgent.setFlying(FALSE);

	LLViewerObject* object = gObjectList.findObject(sitObjectID);
	if (object)
	{
		LLVector3 sit_spot = object->getPositionAgent() + (sitPosition * object->getRotation());
		if (!use_autopilot || isAgentAvatarValid() && gAgentAvatarp->isSitting() && gAgentAvatarp->getRoot() == object->getRoot())
		{
			//we're already sitting on this object, so don't autopilot
		}
		else
		{
			gAgent.startAutoPilotGlobal(gAgent.getPosGlobalFromAgent(sit_spot), "Sit", &sitRotation, near_sit_object, NULL, 0.5f);
		}
	}
	else
	{
		LL_WARNS("Messaging") << "Received sit approval for unknown object " << sitObjectID << LL_ENDL;
	}
}

void process_clear_follow_cam_properties(LLMessageSystem *mesgsys, void **user_data)
{
	LLUUID		source_id;

	mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, source_id);

	LLFollowCamMgr::removeFollowCamParams(source_id);
}

void process_set_follow_cam_properties(LLMessageSystem *mesgsys, void **user_data)
{
	S32			type;
	F32			value;
	bool		settingPosition = false;
	bool		settingFocus	= false;
	bool		settingFocusOffset = false;
	LLVector3	position;
	LLVector3	focus;
	LLVector3	focus_offset;

	LLUUID		source_id;

	mesgsys->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, source_id);

	LLViewerObject* objectp = gObjectList.findObject(source_id);
	if (objectp)
	{
		objectp->mFlags |= FLAGS_CAMERA_SOURCE;
	}

	S32 num_objects = mesgsys->getNumberOfBlocks("CameraProperty");
	for (S32 block_index = 0; block_index < num_objects; block_index++)
	{
		mesgsys->getS32("CameraProperty", "Type", type, block_index);
		mesgsys->getF32("CameraProperty", "Value", value, block_index);
		switch(type)
		{
		case FOLLOWCAM_PITCH:
			LLFollowCamMgr::setPitch(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_OFFSET_X:
			focus_offset.mV[VX] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_FOCUS_OFFSET_Y:
			focus_offset.mV[VY] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_FOCUS_OFFSET_Z:
			focus_offset.mV[VZ] = value;
			settingFocusOffset = true;
			break;
		case FOLLOWCAM_POSITION_LAG:
			LLFollowCamMgr::setPositionLag(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_LAG:
			LLFollowCamMgr::setFocusLag(source_id, value);
			break;
		case FOLLOWCAM_DISTANCE:
			LLFollowCamMgr::setDistance(source_id, value);
			break;
		case FOLLOWCAM_BEHINDNESS_ANGLE:
			LLFollowCamMgr::setBehindnessAngle(source_id, value);
			break;
		case FOLLOWCAM_BEHINDNESS_LAG:
			LLFollowCamMgr::setBehindnessLag(source_id, value);
			break;
		case FOLLOWCAM_POSITION_THRESHOLD:
			LLFollowCamMgr::setPositionThreshold(source_id, value);
			break;
		case FOLLOWCAM_FOCUS_THRESHOLD:
			LLFollowCamMgr::setFocusThreshold(source_id, value);
			break;
		case FOLLOWCAM_ACTIVE:
			//if 1, set using followcam,. 
			LLFollowCamMgr::setCameraActive(source_id, value != 0.f);
			break;
		case FOLLOWCAM_POSITION_X:
			settingPosition = true;
			position.mV[ 0 ] = value;
			break;
		case FOLLOWCAM_POSITION_Y:
			settingPosition = true;
			position.mV[ 1 ] = value;
			break;
		case FOLLOWCAM_POSITION_Z:
			settingPosition = true;
			position.mV[ 2 ] = value;
			break;
		case FOLLOWCAM_FOCUS_X:
			settingFocus = true;
			focus.mV[ 0 ] = value;
			break;
		case FOLLOWCAM_FOCUS_Y:
			settingFocus = true;
			focus.mV[ 1 ] = value;
			break;
		case FOLLOWCAM_FOCUS_Z:
			settingFocus = true;
			focus.mV[ 2 ] = value;
			break;
		case FOLLOWCAM_POSITION_LOCKED:
			LLFollowCamMgr::setPositionLocked(source_id, value != 0.f);
			break;
		case FOLLOWCAM_FOCUS_LOCKED:
			LLFollowCamMgr::setFocusLocked(source_id, value != 0.f);
			break;

		default:
			break;
		}
	}

	if ( settingPosition )
	{
		LLFollowCamMgr::setPosition(source_id, position);
	}
	if ( settingFocus )
	{
		LLFollowCamMgr::setFocus(source_id, focus);
	}
	if ( settingFocusOffset )
	{
		LLFollowCamMgr::setFocusOffset(source_id, focus_offset);
	}
}
//end Ventrella 


// Culled from newsim lltask.cpp
void process_name_value(LLMessageSystem *mesgsys, void **user_data)
{
	std::string	temp_str;
	LLUUID	id;
	S32		i, num_blocks;

	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);

	LLViewerObject* object = gObjectList.findObject(id);

	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, temp_str, i);
			LL_INFOS("Messaging") << "Added to object Name Value: " << temp_str << LL_ENDL;
			object->addNVPair(temp_str);
		}
	}
	else
	{
		LL_INFOS("Messaging") << "Can't find object " << id << " to add name value pair" << LL_ENDL;
	}
}

void process_remove_name_value(LLMessageSystem *mesgsys, void **user_data)
{
	std::string	temp_str;
	LLUUID	id;
	S32		i, num_blocks;

	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);

	LLViewerObject* object = gObjectList.findObject(id);

	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, temp_str, i);
			LL_INFOS("Messaging") << "Removed from object Name Value: " << temp_str << LL_ENDL;
			object->removeNVPair(temp_str);
		}
	}
	else
	{
		LL_INFOS("Messaging") << "Can't find object " << id << " to remove name value pair" << LL_ENDL;
	}
}

void process_kick_user(LLMessageSystem *msg, void** /*user_data*/)
{
	std::string message;

	msg->getStringFast(_PREHASH_UserInfo, _PREHASH_Reason, message);

	LLAppViewer::instance()->forceDisconnect(message);
}


/*
void process_user_list_reply(LLMessageSystem *msg, void **user_data)
{
	LLUserList::processUserListReply(msg, user_data);
	return;
	char	firstname[MAX_STRING+1];
	char	lastname[MAX_STRING+1];
	U8		status;
	S32		user_count;

	user_count = msg->getNumberOfBlocks("UserBlock");

	for (S32 i = 0; i < user_count; i++)
	{
		msg->getData("UserBlock", i, "FirstName", firstname);
		msg->getData("UserBlock", i, "LastName", lastname);
		msg->getData("UserBlock", i, "Status", &status);

		if (status & 0x01)
		{
			dialog_friends_add_friend(buffer, TRUE);
		}
		else
		{
			dialog_friends_add_friend(buffer, FALSE);
		}
	}

	dialog_friends_done_adding();
}
*/

// this is not handled in processUpdateMessage
/*
void process_time_dilation(LLMessageSystem *msg, void **user_data)
{
	// get the time_dilation
	U16 foo;
	msg->getData("TimeDilation", "TimeDilation", &foo);
	F32 time_dilation = ((F32) foo) / 65535.f;

	// get the pointer to the right region
	U32 ip = msg->getSenderIP();
	U32 port = msg->getSenderPort();
	LLViewerRegion *regionp = LLWorld::getInstance()->getRegion(ip, port);
	if (regionp)
	{
		regionp->setTimeDilation(time_dilation);
	}
}
*/


void process_money_balance_reply( LLMessageSystem* msg, void** )
{
	S32 balance = 0;
	S32 credit = 0;
	S32 committed = 0;
	std::string desc;
	LLUUID tid;

	msg->getUUID("MoneyData", "TransactionID", tid);
	msg->getS32("MoneyData", "MoneyBalance", balance);
	msg->getS32("MoneyData", "SquareMetersCredit", credit);
	msg->getS32("MoneyData", "SquareMetersCommitted", committed);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_Description, desc);
	LL_INFOS("Messaging") << "L$, credit, committed: " << balance << " " << credit << " "
			<< committed << LL_ENDL;
    
	if (gStatusBar)
	{
		gStatusBar->setBalance(balance);
		gStatusBar->setLandCredit(credit);
		gStatusBar->setLandCommitted(committed);
	}

	if (desc.empty()
		|| !gSavedSettings.getBOOL("NotifyMoneyChange"))
	{
		// ...nothing to display
		return;
	}

	// Suppress duplicate messages about the same transaction
	static std::deque<LLUUID> recent;
	if (std::find(recent.rbegin(), recent.rend(), tid) != recent.rend())
	{
		return;
	}

	// Once the 'recent' container gets large enough, chop some
	// off the beginning.
	const U32 MAX_LOOKBACK = 30;
	const S32 POP_FRONT_SIZE = 12;
	if(recent.size() > MAX_LOOKBACK)
	{
		LL_DEBUGS("Messaging") << "Removing oldest transaction records" << LL_ENDL;
		recent.erase(recent.begin(), recent.begin() + POP_FRONT_SIZE);
	}
	//LL_DEBUGS("Messaging") << "Pushing back transaction " << tid << LL_ENDL;
	recent.push_back(tid);

	if (msg->has("TransactionInfo"))
	{
		// ...message has extended info for localization
		process_money_balance_reply_extended(msg);
	}
	else
	{
		// Only old dev grids will not supply the TransactionInfo block,
		// so we can just use the hard-coded English string.
		LLSD args;
		args["MESSAGE"] = desc;
		LLNotificationsUtil::add("SystemMessage", args);
	}
}

static std::string reason_from_transaction_type(S32 transaction_type,
												const std::string& item_desc)
{
	// *NOTE: The keys for the reason strings are unusual because
	// an earlier version of the code used English language strings
	// extracted from hard-coded server English descriptions.
	// Keeping them so we don't have to re-localize them.
	switch (transaction_type)
	{
		case TRANS_OBJECT_SALE:
		{
			LLStringUtil::format_map_t arg;
			arg["ITEM"] = item_desc;
			return LLTrans::getString("for item", arg);
		}
		case TRANS_LAND_SALE:
			return LLTrans::getString("for a parcel of land");
			
		case TRANS_LAND_PASS_SALE:
			return LLTrans::getString("for a land access pass");
			
		case TRANS_GROUP_LAND_DEED:
			return LLTrans::getString("for deeding land");
			
		case TRANS_GROUP_CREATE:
			return LLTrans::getString("to create a group");
			
		case TRANS_GROUP_JOIN:
			return LLTrans::getString("to join a group");
			
		case TRANS_UPLOAD_CHARGE:
			return LLTrans::getString("to upload");

		case TRANS_CLASSIFIED_CHARGE:
			return LLTrans::getString("to publish a classified ad");
			
		// These have no reason to display, but are expected and should not
		// generate warnings
		case TRANS_GIFT:
		case TRANS_PAY_OBJECT:
		case TRANS_OBJECT_PAYS:
			return std::string();

		default:
			llwarns << "Unknown transaction type " 
				<< transaction_type << llendl;
			return std::string();
	}
}

static void money_balance_group_notify(const LLUUID& group_id,
									   const std::string& name,
									   bool is_group,
									   std::string notification,
									   LLSD args,
									   LLSD payload)
{
	// Message uses name SLURLs, don't actually have to substitute in
	// the name.  We're just making sure it's available.
	// Notification is either PaymentReceived or PaymentSent
	LLNotificationsUtil::add(notification, args, payload);
}

static void money_balance_avatar_notify(const LLUUID& agent_id,
										const LLAvatarName& av_name,
									   	std::string notification,
									   	LLSD args,
									   	LLSD payload)
{
	// Message uses name SLURLs, don't actually have to substitute in
	// the name.  We're just making sure it's available.
	// Notification is either PaymentReceived or PaymentSent
	LLNotificationsUtil::add(notification, args, payload);
}

static void process_money_balance_reply_extended(LLMessageSystem* msg)
{
    // Added in server 1.40 and viewer 2.1, support for localization
    // and agent ids for name lookup.
    S32 transaction_type = 0;
    LLUUID source_id;
	BOOL is_source_group = FALSE;
    LLUUID dest_id;
	BOOL is_dest_group = FALSE;
    S32 amount = 0;
    std::string item_description;
	BOOL success = FALSE;

    msg->getS32("TransactionInfo", "TransactionType", transaction_type);
    msg->getUUID("TransactionInfo", "SourceID", source_id);
	msg->getBOOL("TransactionInfo", "IsSourceGroup", is_source_group);
    msg->getUUID("TransactionInfo", "DestID", dest_id);
	msg->getBOOL("TransactionInfo", "IsDestGroup", is_dest_group);
    msg->getS32("TransactionInfo", "Amount", amount);
    msg->getString("TransactionInfo", "ItemDescription", item_description);
	msg->getBOOL("MoneyData", "TransactionSuccess", success);
    LL_INFOS("Money") << "MoneyBalanceReply source " << source_id 
		<< " dest " << dest_id
		<< " type " << transaction_type
		<< " item " << item_description << LL_ENDL;

	if (source_id.isNull() && dest_id.isNull())
	{
		// this is a pure balance update, no notification required
		return;
	}

	std::string source_slurl;
	if (is_source_group)
	{
		source_slurl =
			LLSLURL( "group", source_id, "inspect").getSLURLString();
	}
	else
	{
		source_slurl =
			LLSLURL( "agent", source_id, "completename").getSLURLString();
	}

	std::string dest_slurl;
	if (is_dest_group)
	{
		dest_slurl =
			LLSLURL( "group", dest_id, "inspect").getSLURLString();
	}
	else
	{
		dest_slurl =
			LLSLURL( "agent", dest_id, "completename").getSLURLString();
	}

	std::string reason =
		reason_from_transaction_type(transaction_type, item_description);
	
	LLStringUtil::format_map_t args;
	args["REASON"] = reason; // could be empty
	args["AMOUNT"] = llformat("%d", amount);
	
	// Need to delay until name looked up, so need to know whether or not
	// is group
	bool is_name_group = false;
	LLUUID name_id;
	std::string message;
	std::string notification;
	LLSD final_args;
	LLSD payload;
	
	bool you_paid_someone = (source_id == gAgentID);
	if (you_paid_someone)
	{
		args["NAME"] = dest_slurl;
		is_name_group = is_dest_group;
		name_id = dest_id;
		if (!reason.empty())
		{
			if (dest_id.notNull())
			{
				message = success ? LLTrans::getString("you_paid_ldollars", args) :
									LLTrans::getString("you_paid_failure_ldollars", args);
			}
			else
			{
				// transaction fee to the system, eg, to create a group
				message = success ? LLTrans::getString("you_paid_ldollars_no_name", args) :
									LLTrans::getString("you_paid_failure_ldollars_no_name", args);
			}
		}
		else
		{
			if (dest_id.notNull())
			{
				message = success ? LLTrans::getString("you_paid_ldollars_no_reason", args) :
									LLTrans::getString("you_paid_failure_ldollars_no_reason", args);
			}
			else
			{
				// no target, no reason, you just paid money
				message = success ? LLTrans::getString("you_paid_ldollars_no_info", args) :
									LLTrans::getString("you_paid_failure_ldollars_no_info", args);
			}
		}
		final_args["MESSAGE"] = message;
		notification = success ? "PaymentSent" : "PaymentFailure";
	}
	else {
		// ...someone paid you
		args["NAME"] = source_slurl;
		is_name_group = is_source_group;
		name_id = source_id;
		if (!reason.empty())
		{
			message = LLTrans::getString("paid_you_ldollars", args);
		}
		else {
			message = LLTrans::getString("paid_you_ldollars_no_reason", args);
		}
		final_args["MESSAGE"] = message;

		// make notification loggable
		payload["from_id"] = source_id;
		notification = "PaymentReceived";
	}

	// Despite using SLURLs, wait until the name is available before
	// showing the notification, otherwise the UI layout is strange and
	// the user sees a "Loading..." message
	if (is_name_group)
	{
		gCacheName->getGroup(name_id,
						boost::bind(&money_balance_group_notify,
									_1, _2, _3,
									notification, final_args, payload));
	}
	else {
		LLAvatarNameCache::get(name_id,
							   boost::bind(&money_balance_avatar_notify,
										   _1, _2,
										   notification, final_args, payload));										   
	}
}

bool handle_prompt_for_maturity_level_change_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	
	if (0 == option)
	{
		// set the preference to the maturity of the region we're calling
		U8 preferredMaturity = static_cast<U8>(notification["payload"]["_region_access"].asInteger());
		gSavedSettings.setU32("PreferredMaturity", static_cast<U32>(preferredMaturity));
	}
	
	return false;
}

bool handle_prompt_for_maturity_level_change_and_reteleport_callback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	
	if (0 == option)
	{
		// set the preference to the maturity of the region we're calling
		U8 preferredMaturity = static_cast<U8>(notification["payload"]["_region_access"].asInteger());
		gSavedSettings.setU32("PreferredMaturity", static_cast<U32>(preferredMaturity));
		gAgent.setMaturityRatingChangeDuringTeleport(preferredMaturity);
		gAgent.restartFailedTeleportRequest();
	}
	else
	{
		gAgent.clearTeleportRequest();
	}
	
	return false;
}

// some of the server notifications need special handling. This is where we do that.
bool handle_special_notification(std::string notificationID, LLSD& llsdBlock)
{
	U8 regionAccess = static_cast<U8>(llsdBlock["_region_access"].asInteger());
	std::string regionMaturity = LLViewerRegion::accessToString(regionAccess);
	LLStringUtil::toLower(regionMaturity);
	llsdBlock["REGIONMATURITY"] = regionMaturity;
	
	bool returnValue = false;
	LLNotificationPtr maturityLevelNotification;
	std::string notifySuffix = "_Notify";
	if (regionAccess == SIM_ACCESS_MATURE)
	{
		if (gAgent.isTeen())
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_AdultsOnlyContent", llsdBlock);
			returnValue = true;

			notifySuffix = "_NotifyAdultsOnly";
		}
		else if (gAgent.prefersPG())
		{
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
			returnValue = true;
		}
		else if (LLStringUtil::compareStrings(notificationID, "RegionEntryAccessBlocked") == 0)
		{
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_PreferencesOutOfSync", llsdBlock, llsdBlock);
			returnValue = true;
		}
	}
	else if (regionAccess == SIM_ACCESS_ADULT)
	{
		if (!gAgent.isAdult())
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_AdultsOnlyContent", llsdBlock);
			returnValue = true;

			notifySuffix = "_NotifyAdultsOnly";
		}
		else if (gAgent.prefersPG() || gAgent.prefersMature())
		{
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
			returnValue = true;
		}
		else if (LLStringUtil::compareStrings(notificationID, "RegionEntryAccessBlocked") == 0)
		{
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_PreferencesOutOfSync", llsdBlock, llsdBlock);
			returnValue = true;
		}
	}

	if ((maturityLevelNotification == NULL) || maturityLevelNotification->isIgnored())
	{
		// Given a simple notification if no maturityLevelNotification is set or it is ignore
		LLNotificationsUtil::add(notificationID + notifySuffix, llsdBlock);
	}

	return returnValue;
}

// some of the server notifications need special handling. This is where we do that.
bool handle_teleport_access_blocked(LLSD& llsdBlock)
{
	std::string notificationID("TeleportEntryAccessBlocked");
	U8 regionAccess = static_cast<U8>(llsdBlock["_region_access"].asInteger());
	std::string regionMaturity = LLViewerRegion::accessToString(regionAccess);
	LLStringUtil::toLower(regionMaturity);
	llsdBlock["REGIONMATURITY"] = regionMaturity;
	
	bool returnValue = false;
	LLNotificationPtr maturityLevelNotification;
	std::string notifySuffix = "_Notify";
	if (regionAccess == SIM_ACCESS_MATURE)
	{
		if (gAgent.isTeen())
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_AdultsOnlyContent", llsdBlock);
			returnValue = true;

			notifySuffix = "_NotifyAdultsOnly";
		}
		else if (gAgent.prefersPG())
		{
			if (gAgent.hasRestartableFailedTeleportRequest())
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_ChangeAndReTeleport", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_and_reteleport_callback);
				returnValue = true;
			}
			else
			{
				gAgent.clearTeleportRequest();
				maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
				returnValue = true;
			}
		}
		else
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_PreferencesOutOfSync", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
			returnValue = true;
		}
	}
	else if (regionAccess == SIM_ACCESS_ADULT)
	{
		if (!gAgent.isAdult())
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_AdultsOnlyContent", llsdBlock);
			returnValue = true;

			notifySuffix = "_NotifyAdultsOnly";
		}
		else if (gAgent.prefersPG() || gAgent.prefersMature())
		{
			if (gAgent.hasRestartableFailedTeleportRequest())
			{
				maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_ChangeAndReTeleport", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_and_reteleport_callback);
				returnValue = true;
			}
			else
			{
				gAgent.clearTeleportRequest();
				maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_Change", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
				returnValue = true;
			}
		}
		else
		{
			gAgent.clearTeleportRequest();
			maturityLevelNotification = LLNotificationsUtil::add(notificationID+"_PreferencesOutOfSync", llsdBlock, llsdBlock, handle_prompt_for_maturity_level_change_callback);
			returnValue = true;
		}
	}

	if ((maturityLevelNotification == NULL) || maturityLevelNotification->isIgnored())
	{
		// Given a simple notification if no maturityLevelNotification is set or it is ignore
		LLNotificationsUtil::add(notificationID + notifySuffix, llsdBlock);
	}

	return returnValue;
}

bool attempt_standard_notification(LLMessageSystem* msgsystem)
{
	// if we have additional alert data
	if (msgsystem->has(_PREHASH_AlertInfo) && msgsystem->getNumberOfBlocksFast(_PREHASH_AlertInfo) > 0)
	{
		// notification was specified using the new mechanism, so we can just handle it here
		std::string notificationID;
		msgsystem->getStringFast(_PREHASH_AlertInfo, _PREHASH_Message, notificationID);
		if (!LLNotifications::getInstance()->templateExists(notificationID))
		{
			return false;
		}

		std::string llsdRaw;
		LLSD llsdBlock;
		msgsystem->getStringFast(_PREHASH_AlertInfo, _PREHASH_Message, notificationID);
		msgsystem->getStringFast(_PREHASH_AlertInfo, _PREHASH_ExtraParams, llsdRaw);
		if (llsdRaw.length())
		{
			std::istringstream llsdData(llsdRaw);
			if (!LLSDSerialize::deserialize(llsdBlock, llsdData, llsdRaw.length()))
			{
				llwarns << "attempt_standard_notification: Attempted to read notification parameter data into LLSD but failed:" << llsdRaw << llendl;
			}
		}
		
		if (
			(notificationID == "RegionEntryAccessBlocked") ||
			(notificationID == "LandClaimAccessBlocked") ||
			(notificationID == "LandBuyAccessBlocked")
		   )
		{
			/*---------------------------------------------------------------------
			 (Commented so a grep will find the notification strings, since
			 we construct them on the fly; if you add additional notifications,
			 please update the comment.)
			 
			 Could throw any of the following notifications:
			 
				RegionEntryAccessBlocked
				RegionEntryAccessBlocked_Notify
				RegionEntryAccessBlocked_NotifyAdultsOnly
				RegionEntryAccessBlocked_Change
				RegionEntryAccessBlocked_AdultsOnlyContent
				RegionEntryAccessBlocked_ChangeAndReTeleport
				LandClaimAccessBlocked 
				LandClaimAccessBlocked_Notify 
				LandClaimAccessBlocked_NotifyAdultsOnly
				LandClaimAccessBlocked_Change 
				LandClaimAccessBlocked_AdultsOnlyContent 
				LandBuyAccessBlocked
				LandBuyAccessBlocked_Notify
				LandBuyAccessBlocked_NotifyAdultsOnly
				LandBuyAccessBlocked_Change
				LandBuyAccessBlocked_AdultsOnlyContent
			 
			-----------------------------------------------------------------------*/ 
			if (handle_special_notification(notificationID, llsdBlock))
			{
				return true;
			}
		}
		
		LLNotificationsUtil::add(notificationID, llsdBlock);
		return true;
	}	
	return false;
}


void process_agent_alert_message(LLMessageSystem* msgsystem, void** user_data)
{
	// make sure the cursor is back to the usual default since the
	// alert is probably due to some kind of error.
	gViewerWindow->getWindow()->resetBusyCount();
	
	if (!attempt_standard_notification(msgsystem))
	{
		BOOL modal = FALSE;
		msgsystem->getBOOL("AlertData", "Modal", modal);
		std::string buffer;
		msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, buffer);
		process_alert_core(buffer, modal);
	}
}

// The only difference between this routine and the previous is the fact that
// for this routine, the modal parameter is always false. Sadly, for the message
// handled by this routine, there is no "Modal" parameter on the message, and
// there's no API to tell if a message has the given parameter or not.
// So we can't handle the messages with the same handler.
void process_alert_message(LLMessageSystem *msgsystem, void **user_data)
{
	// make sure the cursor is back to the usual default since the
	// alert is probably due to some kind of error.
	gViewerWindow->getWindow()->resetBusyCount();
		
	if (!attempt_standard_notification(msgsystem))
	{
		BOOL modal = FALSE;
		std::string buffer;
		msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, buffer);
		process_alert_core(buffer, modal);
	}
}

bool handle_not_age_verified_alert(const std::string &pAlertName)
{
	LLNotificationPtr notification = LLNotificationsUtil::add(pAlertName);
	if ((notification == NULL) || notification->isIgnored())
	{
		LLNotificationsUtil::add(pAlertName + "_Notify");
	}

	return true;
}

bool handle_special_alerts(const std::string &pAlertName)
{
	bool isHandled = false;

	if (LLStringUtil::compareStrings(pAlertName, "NotAgeVerified") == 0)
	{
		
		isHandled = handle_not_age_verified_alert(pAlertName);
	}

	return isHandled;
}

void process_alert_core(const std::string& message, BOOL modal)
{
	// HACK -- handle callbacks for specific alerts. It also is localized in notifications.xml
	if ( message == "You died and have been teleported to your home location")
	{
		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_KILLED_COUNT);
	}
	else if( message == "Home position set." )
	{
		// save the home location image to disk
		std::string snap_filename = gDirUtilp->getLindenUserDir();
		snap_filename += gDirUtilp->getDirDelimiter();
		snap_filename += SCREEN_HOME_FILENAME;
		gViewerWindow->saveSnapshot(snap_filename, gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw(), FALSE, FALSE);
	}

	const std::string ALERT_PREFIX("ALERT: ");
	const std::string NOTIFY_PREFIX("NOTIFY: ");
	if (message.find(ALERT_PREFIX) == 0)
	{
		// Allow the server to spawn a named alert so that server alerts can be
		// translated out of English.
		std::string alert_name(message.substr(ALERT_PREFIX.length()));
		if (!handle_special_alerts(alert_name))
		{
			LLNotificationsUtil::add(alert_name);
		}
	}
	else if (message.find(NOTIFY_PREFIX) == 0)
	{
		// Allow the server to spawn a named notification so that server notifications can be
		// translated out of English.
		std::string notify_name(message.substr(NOTIFY_PREFIX.length()));
		LLNotificationsUtil::add(notify_name);
	}
	else if (message[0] == '/')
	{
		// System message is important, show in upper-right box not tip
		std::string text(message.substr(1));
		LLSD args;
		if (text.substr(0,17) == "RESTART_X_MINUTES")
		{
			S32 mins = 0;
			LLStringUtil::convertToS32(text.substr(18), mins);
			args["MINUTES"] = llformat("%d",mins);
			LLNotificationsUtil::add("RegionRestartMinutes", args);
		}
		else if (text.substr(0,17) == "RESTART_X_SECONDS")
		{
			S32 secs = 0;
			LLStringUtil::convertToS32(text.substr(18), secs);
			args["SECONDS"] = llformat("%d",secs);
			LLNotificationsUtil::add("RegionRestartSeconds", args);
		}
		else
		{
			std::string new_msg =LLNotifications::instance().getGlobalString(text);
			args["MESSAGE"] = new_msg;
			LLNotificationsUtil::add("SystemMessage", args);
		}
	}
	else if (modal)
	{
		LLSD args;
		std::string new_msg =LLNotifications::instance().getGlobalString(message);
		args["ERROR_MESSAGE"] = new_msg;
		LLNotificationsUtil::add("ErrorMessage", args);
	}
	else
	{
		// Hack fix for EXP-623 (blame fix on RN :)) to avoid a sim deploy
		const std::string AUTOPILOT_CANCELED_MSG("Autopilot canceled");
		if (message.find(AUTOPILOT_CANCELED_MSG) == std::string::npos )
		{
			LLSD args;
			std::string new_msg =LLNotifications::instance().getGlobalString(message);

			std::string localized_msg;
			bool is_message_localized = LLTrans::findString(localized_msg, new_msg);

			args["MESSAGE"] = is_message_localized ? localized_msg : new_msg;
			LLNotificationsUtil::add("SystemMessageTip", args);
		}
	}
}

mean_collision_list_t				gMeanCollisionList;
time_t								gLastDisplayedTime = 0;

void handle_show_mean_events(void *)
{
	LLFloaterReg::showInstance("bumps");
	//LLFloaterBump::showInstance();
}

void mean_name_callback(const LLUUID &id, const std::string& full_name, bool is_group)
{
	static const U32 max_collision_list_size = 20;
	if (gMeanCollisionList.size() > max_collision_list_size)
	{
		mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
		for (U32 i=0; i<max_collision_list_size; i++) iter++;
		for_each(iter, gMeanCollisionList.end(), DeletePointer());
		gMeanCollisionList.erase(iter, gMeanCollisionList.end());
	}

	for (mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
		 iter != gMeanCollisionList.end(); ++iter)
	{
		LLMeanCollisionData *mcd = *iter;
		if (mcd->mPerp == id)
		{
			mcd->mFullName = full_name;
		}
	}
}

void process_mean_collision_alert_message(LLMessageSystem *msgsystem, void **user_data)
{
	if (gAgent.inPrelude())
	{
		// In prelude, bumping is OK.  This dialog is rather confusing to 
		// newbies, so we don't show it.  Drop the packet on the floor.
		return;
	}

	// make sure the cursor is back to the usual default since the
	// alert is probably due to some kind of error.
	gViewerWindow->getWindow()->resetBusyCount();

	LLUUID perp;
	U32	   time;
	U8	   u8type;
	EMeanCollisionType	   type;
	F32    mag;

	S32 i, num = msgsystem->getNumberOfBlocks(_PREHASH_MeanCollision);

	for (i = 0; i < num; i++)
	{
		msgsystem->getUUIDFast(_PREHASH_MeanCollision, _PREHASH_Perp, perp);
		msgsystem->getU32Fast(_PREHASH_MeanCollision, _PREHASH_Time, time);
		msgsystem->getF32Fast(_PREHASH_MeanCollision, _PREHASH_Mag, mag);
		msgsystem->getU8Fast(_PREHASH_MeanCollision, _PREHASH_Type, u8type);

		type = (EMeanCollisionType)u8type;

		BOOL b_found = FALSE;

		for (mean_collision_list_t::iterator iter = gMeanCollisionList.begin();
			 iter != gMeanCollisionList.end(); ++iter)
		{
			LLMeanCollisionData *mcd = *iter;
			if ((mcd->mPerp == perp) && (mcd->mType == type))
			{
				mcd->mTime = time;
				mcd->mMag = mag;
				b_found = TRUE;
				break;
			}
		}

		if (!b_found)
		{
			LLMeanCollisionData *mcd = new LLMeanCollisionData(gAgentID, perp, time, type, mag);
			gMeanCollisionList.push_front(mcd);
			gCacheName->get(perp, false, boost::bind(&mean_name_callback, _1, _2, _3));
		}
	}
}

void process_frozen_message(LLMessageSystem *msgsystem, void **user_data)
{
	// make sure the cursor is back to the usual default since the
	// alert is probably due to some kind of error.
	gViewerWindow->getWindow()->resetBusyCount();
	BOOL b_frozen;
	
	msgsystem->getBOOL("FrozenData", "Data", b_frozen);

	// TODO: make being frozen change view
	if (b_frozen)
	{
	}
	else
	{
	}
}

// do some extra stuff once we get our economy data
void process_economy_data(LLMessageSystem *msg, void** /*user_data*/)
{
	LLGlobalEconomy::processEconomyData(msg, LLGlobalEconomy::Singleton::getInstance());

	S32 upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();

	LL_INFOS_ONCE("Messaging") << "EconomyData message arrived; upload cost is L$" << upload_cost << LL_ENDL;

	gMenuHolder->getChild<LLUICtrl>("Upload Image")->setLabelArg("[COST]", llformat("%d", upload_cost));
	gMenuHolder->getChild<LLUICtrl>("Upload Sound")->setLabelArg("[COST]", llformat("%d", upload_cost));
	gMenuHolder->getChild<LLUICtrl>("Upload Animation")->setLabelArg("[COST]", llformat("%d", upload_cost));
	gMenuHolder->getChild<LLUICtrl>("Bulk Upload")->setLabelArg("[COST]", llformat("%d", upload_cost));
}

void notify_cautioned_script_question(const LLSD& notification, const LLSD& response, S32 orig_questions, BOOL granted)
{
	// only continue if at least some permissions were requested
	if (orig_questions)
	{
		// check to see if the person we are asking

		// "'[OBJECTNAME]', an object owned by '[OWNERNAME]', 
		// located in [REGIONNAME] at [REGIONPOS], 
		// has been <granted|denied> permission to: [PERMISSIONS]."

		LLUIString notice(LLTrans::getString(granted ? "ScriptQuestionCautionChatGranted" : "ScriptQuestionCautionChatDenied"));

		// always include the object name and owner name 
		notice.setArg("[OBJECTNAME]", notification["payload"]["object_name"].asString());
		notice.setArg("[OWNERNAME]", notification["payload"]["owner_name"].asString());

		// try to lookup viewerobject that corresponds to the object that
		// requested permissions (here, taskid->requesting object id)
		BOOL foundpos = FALSE;
		LLViewerObject* viewobj = gObjectList.findObject(notification["payload"]["task_id"].asUUID());
		if (viewobj)
		{
			// found the viewerobject, get it's position in its region
			LLVector3 objpos(viewobj->getPosition());
			
			// try to lookup the name of the region the object is in
			LLViewerRegion* viewregion = viewobj->getRegion();
			if (viewregion)
			{
				// got the region, so include the region and 3d coordinates of the object
				notice.setArg("[REGIONNAME]", viewregion->getName());				
				std::string formatpos = llformat("%.1f, %.1f,%.1f", objpos[VX], objpos[VY], objpos[VZ]);
				notice.setArg("[REGIONPOS]", formatpos);

				foundpos = TRUE;
			}
		}

		if (!foundpos)
		{
			// unable to determine location of the object
			notice.setArg("[REGIONNAME]", "(unknown region)");
			notice.setArg("[REGIONPOS]", "(unknown position)");
		}

		// check each permission that was requested, and list each 
		// permission that has been flagged as a caution permission
		BOOL caution = FALSE;
		S32 count = 0;
		std::string perms;
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if ((orig_questions & LSCRIPTRunTimePermissionBits[i]) && SCRIPT_QUESTION_IS_CAUTION[i])
			{
				count++;
				caution = TRUE;

				// add a comma before the permission description if it is not the first permission
				// added to the list or the last permission to check
				if ((count > 1) && (i < SCRIPT_PERMISSION_EOF))
				{
					perms.append(", ");
				}

				perms.append(LLTrans::getString(SCRIPT_QUESTIONS[i]));
			}
		}

		notice.setArg("[PERMISSIONS]", perms);

		// log a chat message as long as at least one requested permission
		// is a caution permission
		if (caution)
		{
			LLChat chat(notice.getString());
	//		LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
	}
}

bool script_question_cb(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	LLMessageSystem *msg = gMessageSystem;
	S32 orig = notification["payload"]["questions"].asInteger();
	S32 new_questions = orig;

	if (response["Details"])
	{
		// respawn notification...
		LLNotificationsUtil::add(notification["name"], notification["substitutions"], notification["payload"]);

		// ...with description on top
		LLNotificationsUtil::add("DebitPermissionDetails");
		return false;
	}

	// check whether permissions were granted or denied
	BOOL allowed = TRUE;
	// the "yes/accept" button is the first button in the template, making it button 0
	// if any other button was clicked, the permissions were denied
	if (option != 0)
	{
		new_questions = 0;
		allowed = FALSE;
	}	

	LLUUID task_id = notification["payload"]["task_id"].asUUID();
	LLUUID item_id = notification["payload"]["item_id"].asUUID();

	// reply with the permissions granted or denied
	msg->newMessageFast(_PREHASH_ScriptAnswerYes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_TaskID, task_id);
	msg->addUUIDFast(_PREHASH_ItemID, item_id);
	msg->addS32Fast(_PREHASH_Questions, new_questions);
	msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));

	// only log a chat message if caution prompts are enabled
	if (gSavedSettings.getBOOL("PermissionsCautionEnabled"))
	{
		// log a chat message, if appropriate
		notify_cautioned_script_question(notification, response, orig, allowed);
	}

	if ( response["Mute"] ) // mute
	{
		LLMuteList::getInstance()->add(LLMute(item_id, notification["payload"]["object_name"].asString(), LLMute::OBJECT));

		// purge the message queue of any previously queued requests from the same source. DEV-4879
		class OfferMatcher : public LLNotificationsUI::LLScreenChannel::Matcher
		{
		public:
			OfferMatcher(const LLUUID& to_block) : blocked_id(to_block) {}
			bool matches(const LLNotificationPtr notification) const
			{
				if (notification->getName() == "ScriptQuestionCaution"
					|| notification->getName() == "ScriptQuestion")
				{
					return (notification->getPayload()["item_id"].asUUID() == blocked_id);
				}
				return false;
			}
		private:
			const LLUUID& blocked_id;
		};

		LLNotificationsUI::LLChannelManager::getInstance()->killToastsFromChannel(LLUUID(
				gSavedSettings.getString("NotificationChannelUUID")), OfferMatcher(item_id));
	}

	return false;
}
static LLNotificationFunctorRegistration script_question_cb_reg_1("ScriptQuestion", script_question_cb);
static LLNotificationFunctorRegistration script_question_cb_reg_2("ScriptQuestionCaution", script_question_cb);

void process_script_question(LLMessageSystem *msg, void **user_data)
{
	// *TODO: Translate owner name -> [FIRST] [LAST]

	LLHost sender = msg->getSender();

	LLUUID taskid;
	LLUUID itemid;
	S32		questions;
	std::string object_name;
	std::string owner_name;

	// taskid -> object key of object requesting permissions
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_TaskID, taskid );
	// itemid -> script asset key of script requesting permissions
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_ItemID, itemid );
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectName, object_name);
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectOwner, owner_name);
	msg->getS32Fast(_PREHASH_Data, _PREHASH_Questions, questions );

	// Special case. If the objects are owned by this agent, throttle per-object instead
	// of per-owner. It's common for residents to reset a ton of scripts that re-request
	// permissions, as with tier boxes. UUIDs can't be valid agent names and vice-versa,
	// so we'll reuse the same namespace for both throttle types.
	std::string throttle_name = owner_name;
	std::string self_name;
	LLAgentUI::buildFullname( self_name );
	if( owner_name == self_name )
	{
		throttle_name = taskid.getString();
	}
	
	// don't display permission requests if this object is muted
	if (LLMuteList::getInstance()->isMuted(taskid)) return;
	
	// throttle excessive requests from any specific user's scripts
	typedef LLKeyThrottle<std::string> LLStringThrottle;
	static LLStringThrottle question_throttle( LLREQUEST_PERMISSION_THROTTLE_LIMIT, LLREQUEST_PERMISSION_THROTTLE_INTERVAL );

	switch (question_throttle.noteAction(throttle_name))
	{
		case LLStringThrottle::THROTTLE_NEWLY_BLOCKED:
			LL_INFOS("Messaging") << "process_script_question throttled"
					<< " owner_name:" << owner_name
					<< LL_ENDL;
			// Fall through

		case LLStringThrottle::THROTTLE_BLOCKED:
			// Escape altogether until we recover
			return;

		case LLStringThrottle::THROTTLE_OK:
			break;
	}

	std::string script_question;
	if (questions)
	{
		BOOL caution = FALSE;
		S32 count = 0;
		LLSD args;
		args["OBJECTNAME"] = object_name;
		args["NAME"] = LLCacheName::cleanFullName(owner_name);

		BOOL has_not_only_debit = questions ^ LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_DEBIT];
		// check the received permission flags against each permission
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if (questions & LSCRIPTRunTimePermissionBits[i])
			{
				count++;

				// check whether permission question should cause special caution dialog
				caution |= (SCRIPT_QUESTION_IS_CAUTION[i]);

				if (("ScriptTakeMoney" ==  SCRIPT_QUESTIONS[i]) && has_not_only_debit)
					continue;

				script_question += "    " + LLTrans::getString(SCRIPT_QUESTIONS[i]) + "\n";
			}
		}
		args["QUESTIONS"] = script_question;

		LLSD payload;
		payload["task_id"] = taskid;
		payload["item_id"] = itemid;
		payload["sender"] = sender.getIPandPort();
		payload["questions"] = questions;
		payload["object_name"] = object_name;
		payload["owner_name"] = owner_name;

		// check whether cautions are even enabled or not
		if (gSavedSettings.getBOOL("PermissionsCautionEnabled"))
		{
			if (caution)
			{
				args["FOOTERTEXT"] = (count > 1) ? LLTrans::getString("AdditionalPermissionsRequestHeader") + "\n\n" + script_question : "";
			}
			// display the caution permissions prompt
			LLNotificationsUtil::add(caution ? "ScriptQuestionCaution" : "ScriptQuestion", args, payload);
		}
		else
		{
			// fall back to default behavior if cautions are entirely disabled
			LLNotificationsUtil::add("ScriptQuestion", args, payload);
		}

	}
}


void process_derez_container(LLMessageSystem *msg, void**)
{
	LL_WARNS("Messaging") << "call to deprecated process_derez_container" << LL_ENDL;
}

void container_inventory_arrived(LLViewerObject* object,
								 LLInventoryObject::object_list_t* inventory,
								 S32 serial_num,
								 void* data)
{
	LL_DEBUGS("Messaging") << "container_inventory_arrived()" << LL_ENDL;
	if( gAgentCamera.cameraMouselook() )
	{
		gAgentCamera.changeCameraToDefault();
	}

	LLInventoryPanel *active_panel = LLInventoryPanel::getActiveInventoryPanel();

	if (inventory->size() > 2)
	{
		// create a new inventory category to put this in
		LLUUID cat_id;
		cat_id = gInventory.createNewCategory(gInventory.getRootFolderID(),
											  LLFolderType::FT_NONE,
											  LLTrans::getString("AcquiredItems"));

		LLInventoryObject::object_list_t::const_iterator it = inventory->begin();
		LLInventoryObject::object_list_t::const_iterator end = inventory->end();
		for ( ; it != end; ++it)
		{
			if ((*it)->getType() != LLAssetType::AT_CATEGORY)
			{
				LLInventoryObject* obj = (LLInventoryObject*)(*it);
				LLInventoryItem* item = (LLInventoryItem*)(obj);
				LLUUID item_id;
				item_id.generate();
				time_t creation_date_utc = time_corrected();
				LLPointer<LLViewerInventoryItem> new_item
					= new LLViewerInventoryItem(item_id,
												cat_id,
												item->getPermissions(),
												item->getAssetUUID(),
												item->getType(),
												item->getInventoryType(),
												item->getName(),
												item->getDescription(),
												LLSaleInfo::DEFAULT,
												item->getFlags(),
												creation_date_utc);
				new_item->updateServer(TRUE);
				gInventory.updateItem(new_item);
			}
		}
		gInventory.notifyObservers();
		if(active_panel)
		{
			active_panel->setSelection(cat_id, TAKE_FOCUS_NO);
		}
	}
	else if (inventory->size() == 2)
	{
		// we're going to get one fake root category as well as the
		// one actual object
		LLInventoryObject::object_list_t::iterator it = inventory->begin();

		if ((*it)->getType() == LLAssetType::AT_CATEGORY)
		{
			++it;
		}

		LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
		const LLUUID category = gInventory.findCategoryUUIDForType(LLFolderType::assetTypeToFolderType(item->getType()));

		LLUUID item_id;
		item_id.generate();
		time_t creation_date_utc = time_corrected();
		LLPointer<LLViewerInventoryItem> new_item
			= new LLViewerInventoryItem(item_id, category,
										item->getPermissions(),
										item->getAssetUUID(),
										item->getType(),
										item->getInventoryType(),
										item->getName(),
										item->getDescription(),
										LLSaleInfo::DEFAULT,
										item->getFlags(),
										creation_date_utc);
		new_item->updateServer(TRUE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();
		if(active_panel)
		{
			active_panel->setSelection(item_id, TAKE_FOCUS_NO);
		}
	}

	// we've got the inventory, now delete this object if this was a take
	BOOL delete_object = (BOOL)(intptr_t)data;
	LLViewerRegion *region = gAgent.getRegion();
	if (delete_object && region)
	{
		gMessageSystem->newMessageFast(_PREHASH_ObjectDelete);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		const U8 NO_FORCE = 0;
		gMessageSystem->addU8Fast(_PREHASH_Force, NO_FORCE);
		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
		gMessageSystem->sendReliable(region->getHost());
	}
}

// method to format the time.
std::string formatted_time(const time_t& the_time)
{
	std::string dateStr = "["+LLTrans::getString("LTimeWeek")+"] ["
						+LLTrans::getString("LTimeMonth")+"] ["
						+LLTrans::getString("LTimeDay")+"] ["
						+LLTrans::getString("LTimeHour")+"]:["
						+LLTrans::getString("LTimeMin")+"]:["
						+LLTrans::getString("LTimeSec")+"] ["
						+LLTrans::getString("LTimeYear")+"]";

	LLSD substitution;
	substitution["datetime"] = (S32) the_time;
	LLStringUtil::format (dateStr, substitution);
	return dateStr;
}


void process_teleport_failed(LLMessageSystem *msg, void**)
{
	std::string reason;
	std::string big_reason;
	LLSD args;

	// Let the interested parties know that teleport failed.
	LLViewerParcelMgr::getInstance()->onTeleportFailed();

	// if we have additional alert data
	if (msg->has(_PREHASH_AlertInfo) && msg->getSizeFast(_PREHASH_AlertInfo, _PREHASH_Message) > 0)
	{
		// Get the message ID
		msg->getStringFast(_PREHASH_AlertInfo, _PREHASH_Message, reason);
		big_reason = LLAgent::sTeleportErrorMessages[reason];
		if ( big_reason.size() > 0 )
		{	// Substitute verbose reason from the local map
			args["REASON"] = big_reason;
		}
		else
		{	// Nothing found in the map - use what the server returned in the original message block
			msg->getStringFast(_PREHASH_Info, _PREHASH_Reason, reason);
			args["REASON"] = reason;
		}

		LLSD llsd_block;
		std::string llsd_raw;
		msg->getStringFast(_PREHASH_AlertInfo, _PREHASH_ExtraParams, llsd_raw);
		if (llsd_raw.length())
		{
			std::istringstream llsd_data(llsd_raw);
			if (!LLSDSerialize::deserialize(llsd_block, llsd_data, llsd_raw.length()))
			{
				llwarns << "process_teleport_failed: Attempted to read alert parameter data into LLSD but failed:" << llsd_raw << llendl;
			}
			else
			{
				// change notification name in this special case
				if (handle_teleport_access_blocked(llsd_block))
				{
					if( gAgent.getTeleportState() != LLAgent::TELEPORT_NONE )
					{
						gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
					}
					return;
				}
			}
		}

	}
	else
	{
		msg->getStringFast(_PREHASH_Info, _PREHASH_Reason, reason);

		big_reason = LLAgent::sTeleportErrorMessages[reason];
		if ( big_reason.size() > 0 )
		{	// Substitute verbose reason from the local map
			args["REASON"] = big_reason;
		}
		else
		{	// Nothing found in the map - use what the server returned
			args["REASON"] = reason;
		}
	}

	LLNotificationsUtil::add("CouldNotTeleportReason", args);

	if( gAgent.getTeleportState() != LLAgent::TELEPORT_NONE )
	{
		gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
	}
}

void process_teleport_local(LLMessageSystem *msg,void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_Info, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Got teleport notification for wrong agent!" << LL_ENDL;
		return;
	}

	U32 location_id;
	LLVector3 pos, look_at;
	U32 teleport_flags;
	msg->getU32Fast(_PREHASH_Info, _PREHASH_LocationID, location_id);
	msg->getVector3Fast(_PREHASH_Info, _PREHASH_Position, pos);
	msg->getVector3Fast(_PREHASH_Info, _PREHASH_LookAt, look_at);
	msg->getU32Fast(_PREHASH_Info, _PREHASH_TeleportFlags, teleport_flags);

	if( gAgent.getTeleportState() != LLAgent::TELEPORT_NONE )
	{
		if( gAgent.getTeleportState() == LLAgent::TELEPORT_LOCAL )
		{
			// To prevent TeleportStart messages re-activating the progress screen right
			// after tp, keep the teleport state and let progress screen clear it after a short delay
			// (progress screen is active but not visible)  *TODO: remove when SVC-5290 is fixed
			gTeleportDisplayTimer.reset();
			gTeleportDisplay = TRUE;
		}
		else
		{
			gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
		}
	}

	// Sim tells us whether the new position is off the ground
	if (teleport_flags & TELEPORT_FLAGS_IS_FLYING)
	{
		gAgent.setFlying(TRUE);
	}
	else
	{
		gAgent.setFlying(FALSE);
	}

	gAgent.setPositionAgent(pos);
	gAgentCamera.slamLookAt(look_at);

	if ( !(gAgent.getTeleportKeepsLookAt() && LLViewerJoystick::getInstance()->getOverrideCamera()) )
	{
		gAgentCamera.resetView(TRUE, TRUE);
	}

	// send camera update to new region
	gAgentCamera.updateCamera();

	send_agent_update(TRUE, TRUE);

	// Let the interested parties know we've teleported.
	// Vadim *HACK: Agent position seems to get reset (to render position?)
	//              on each frame, so we have to pass the new position manually.
	LLViewerParcelMgr::getInstance()->onTeleportFinished(true, gAgent.getPosGlobalFromAgent(pos));
}

void send_simple_im(const LLUUID& to_id,
					const std::string& message,
					EInstantMessage dialog,
					const LLUUID& id)
{
	std::string my_name;
	LLAgentUI::buildFullname(my_name);
	send_improved_im(to_id,
					 my_name,
					 message,
					 IM_ONLINE,
					 dialog,
					 id,
					 NO_TIMESTAMP,
					 (U8*)EMPTY_BINARY_BUCKET,
					 EMPTY_BINARY_BUCKET_SIZE);
}

void send_group_notice(const LLUUID& group_id,
					   const std::string& subject,
					   const std::string& message,
					   const LLInventoryItem* item)
{
	// Put this notice into an instant message form.
	// This will mean converting the item to a binary bucket,
	// and the subject/message into a single field.
	std::string my_name;
	LLAgentUI::buildFullname(my_name);

	// Combine subject + message into a single string.
	std::ostringstream subject_and_message;
	// TODO: turn all existing |'s into ||'s in subject and message.
	subject_and_message << subject << "|" << message;

	// Create an empty binary bucket.
	U8 bin_bucket[MAX_INVENTORY_BUFFER_SIZE];
	U8* bucket_to_send = bin_bucket;
	bin_bucket[0] = '\0';
	S32 bin_bucket_size = EMPTY_BINARY_BUCKET_SIZE;
	// If there is an item being sent, pack it into the binary bucket.
	if (item)
	{
		LLSD item_def;
		item_def["item_id"] = item->getUUID();
		item_def["owner_id"] = item->getPermissions().getOwner();
		std::ostringstream ostr;
		LLSDSerialize::serialize(item_def, ostr, LLSDSerialize::LLSD_XML);
		bin_bucket_size = ostr.str().copy(
			(char*)bin_bucket, ostr.str().size());
		bin_bucket[bin_bucket_size] = '\0';
	}
	else
	{
		bucket_to_send = (U8*) EMPTY_BINARY_BUCKET;
	}
   

	send_improved_im(
			group_id,
			my_name,
			subject_and_message.str(),
			IM_ONLINE,
			IM_GROUP_NOTICE,
			LLUUID::null,
			NO_TIMESTAMP,
			bucket_to_send,
			bin_bucket_size);
}

bool handle_lure_callback(const LLSD& notification, const LLSD& response)
{
	static const unsigned OFFER_RECIPIENT_LIMIT = 250;
	if(notification["payload"]["ids"].size() > OFFER_RECIPIENT_LIMIT) 
	{
		// More than OFFER_RECIPIENT_LIMIT targets will overload the message
		// producing an llerror.
		LLSD args;
		args["OFFERS"] = notification["payload"]["ids"].size();
		args["LIMIT"] = static_cast<int>(OFFER_RECIPIENT_LIMIT);
		LLNotificationsUtil::add("TooManyTeleportOffers", args);
		return false;
	}
	
	std::string text = response["message"].asString();
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	text.append("\r\n").append(slurl.getSLURLString());
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if(0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_StartLure);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Info);
		msg->addU8Fast(_PREHASH_LureType, (U8)0); // sim will fill this in.
		msg->addStringFast(_PREHASH_Message, text);
		for(LLSD::array_const_iterator it = notification["payload"]["ids"].beginArray();
			it != notification["payload"]["ids"].endArray();
			++it)
		{
			LLUUID target_id = it->asUUID();

			msg->nextBlockFast(_PREHASH_TargetData);
			msg->addUUIDFast(_PREHASH_TargetID, target_id);

			// Record the offer.
			{
				std::string target_name;
				gCacheName->getFullName(target_id, target_name);  // for im log filenames
				LLSD args;
				args["TO_NAME"] = LLSLURL("agent", target_id, "displayname").getSLURLString();;
	
				LLSD payload;
				
				//*TODO please rewrite all keys to the same case, lower or upper
				payload["from_id"] = target_id;
				payload["SUPPRESS_TOAST"] = true;
				LLNotificationsUtil::add("TeleportOfferSent", args, payload);

				// Add the recepient to the recent people list.
				LLRecentPeople::instance().add(target_id);
			}
		}
		gAgent.sendReliableMessage();
	}

	return false;
}

void handle_lure(const LLUUID& invitee)
{
	LLDynamicArray<LLUUID> ids;
	ids.push_back(invitee);
	handle_lure(ids);
}

// Prompt for a message to the invited user.
void handle_lure(const uuid_vec_t& ids)
{
	if (ids.empty()) return;

	if (!gAgent.getRegion()) return;

	LLSD edit_args;
	edit_args["REGION"] = gAgent.getRegion()->getName();

	LLSD payload;
	for (LLDynamicArray<LLUUID>::const_iterator it = ids.begin();
		it != ids.end();
		++it)
	{
		payload["ids"].append(*it);
	}
	if (gAgent.isGodlike())
	{
		LLNotificationsUtil::add("OfferTeleportFromGod", edit_args, payload, handle_lure_callback);
	}
	else
	{
		LLNotificationsUtil::add("OfferTeleport", edit_args, payload, handle_lure_callback);
	}
}


void send_improved_im(const LLUUID& to_id,
							const std::string& name,
							const std::string& message,
							U8 offline,
							EInstantMessage dialog,
							const LLUUID& id,
							U32 timestamp, 
							const U8* binary_bucket,
							S32 binary_bucket_size)
{
	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		to_id,
		name,
		message,
		offline,
		dialog,
		id,
		0,
		LLUUID::null,
		gAgent.getPositionAgent(),
		timestamp,
		binary_bucket,
		binary_bucket_size);
	gAgent.sendReliableMessage();
}


void send_places_query(const LLUUID& query_id,
					   const LLUUID& trans_id,
					   const std::string& query_text,
					   U32 query_flags,
					   S32 category,
					   const std::string& sim_name)
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessage("PlacesQuery");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUID("QueryID", query_id);
	msg->nextBlock("TransactionData");
	msg->addUUID("TransactionID", trans_id);
	msg->nextBlock("QueryData");
	msg->addString("QueryText", query_text);
	msg->addU32("QueryFlags", query_flags);
	msg->addS8("Category", (S8)category);
	msg->addString("SimName", sim_name);
	gAgent.sendReliableMessage();
}


void process_user_info_reply(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "process_user_info_reply - "
				<< "wrong agent id." << LL_ENDL;
	}
	
	BOOL im_via_email;
	msg->getBOOLFast(_PREHASH_UserData, _PREHASH_IMViaEMail, im_via_email);
	std::string email;
	msg->getStringFast(_PREHASH_UserData, _PREHASH_EMail, email);
	std::string dir_visibility;
	msg->getString( "UserData", "DirectoryVisibility", dir_visibility);

	LLFloaterPreference::updateUserInfo(dir_visibility, im_via_email, email);
	LLFloaterSnapshot::setAgentEmail(email);
}


//---------------------------------------------------------------------------
// Script Dialog
//---------------------------------------------------------------------------

const S32 SCRIPT_DIALOG_MAX_BUTTONS = 12;
const S32 SCRIPT_DIALOG_BUTTON_STR_SIZE = 24;
const S32 SCRIPT_DIALOG_MAX_MESSAGE_SIZE = 512;
const char* SCRIPT_DIALOG_HEADER = "Script Dialog:\n";

bool callback_script_dialog(const LLSD& notification, const LLSD& response)
{
	LLNotificationForm form(notification["form"]);

	std::string rtn_text;
	S32 button_idx;
	button_idx = LLNotification::getSelectedOption(notification, response);
	if (response[TEXTBOX_MAGIC_TOKEN].isDefined())
	{
		if (response[TEXTBOX_MAGIC_TOKEN].isString())
			rtn_text = response[TEXTBOX_MAGIC_TOKEN].asString();
		else
			rtn_text.clear(); // bool marks empty string
	}
	else
	{
		rtn_text = LLNotification::getSelectedOptionName(response);
	}

	// Button -2 = Mute
	// Button -1 = Ignore - no processing needed for this button
	// Buttons 0 and above = dialog choices

	if (-2 == button_idx)
	{
		std::string object_name = notification["payload"]["object_name"].asString();
		LLUUID object_id = notification["payload"]["object_id"].asUUID();
		LLMute mute(object_id, object_name, LLMute::OBJECT);
		if (LLMuteList::getInstance()->add(mute))
		{
			// This call opens the sidebar, displays the block list, and highlights the newly blocked
			// object in the list so the user can see that their block click has taken effect.
			LLPanelBlockedList::showPanelAndSelect(object_id);
		}
	}

	if (0 <= button_idx)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ScriptDialogReply");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("ObjectID", notification["payload"]["object_id"].asUUID());
		msg->addS32("ChatChannel", notification["payload"]["chat_channel"].asInteger());
		msg->addS32("ButtonIndex", button_idx);
		msg->addString("ButtonLabel", rtn_text);
		msg->sendReliable(LLHost(notification["payload"]["sender"].asString()));
	}

	return false;
}
static LLNotificationFunctorRegistration callback_script_dialog_reg_1("ScriptDialog", callback_script_dialog);
static LLNotificationFunctorRegistration callback_script_dialog_reg_2("ScriptDialogGroup", callback_script_dialog);

void process_script_dialog(LLMessageSystem* msg, void**)
{
	S32 i;
	LLSD payload;

	LLUUID object_id;
	msg->getUUID("Data", "ObjectID", object_id);

//	For compability with OS grids first check for presence of extended packet before fetching data.
    LLUUID owner_id;
	if (gMessageSystem->getNumberOfBlocks("OwnerData") > 0)
	{
    msg->getUUID("OwnerData", "OwnerID", owner_id);
	}

	if (LLMuteList::getInstance()->isMuted(object_id) || LLMuteList::getInstance()->isMuted(owner_id))
	{
		return;
	}

	std::string message; 
	std::string first_name;
	std::string last_name;
	std::string object_name;

	S32 chat_channel;
	msg->getString("Data", "FirstName", first_name);
	msg->getString("Data", "LastName", last_name);
	msg->getString("Data", "ObjectName", object_name);
	msg->getString("Data", "Message", message);
	msg->getS32("Data", "ChatChannel", chat_channel);

		// unused for now
	LLUUID image_id;
	msg->getUUID("Data", "ImageID", image_id);

	payload["sender"] = msg->getSender().getIPandPort();
	payload["object_id"] = object_id;
	payload["chat_channel"] = chat_channel;
	payload["object_name"] = object_name;

	// build up custom form
	S32 button_count = msg->getNumberOfBlocks("Buttons");
	if (button_count > SCRIPT_DIALOG_MAX_BUTTONS)
	{
		llwarns << "Too many script dialog buttons - omitting some" << llendl;
		button_count = SCRIPT_DIALOG_MAX_BUTTONS;
	}

	LLNotificationForm form;
	for (i = 0; i < button_count; i++)
	{
		std::string tdesc;
		msg->getString("Buttons", "ButtonLabel", tdesc, i);
		form.addElement("button", std::string(tdesc));
	}

	LLSD args;
	args["TITLE"] = object_name;
	args["MESSAGE"] = message;
	LLNotificationPtr notification;
	if (!first_name.empty())
	{
		args["NAME"] = LLCacheName::buildFullName(first_name, last_name);
		notification = LLNotifications::instance().add(
			LLNotification::Params("ScriptDialog").substitutions(args).payload(payload).form_elements(form.asLLSD()));
	}
	else
	{
		args["GROUPNAME"] = last_name;
		notification = LLNotifications::instance().add(
			LLNotification::Params("ScriptDialogGroup").substitutions(args).payload(payload).form_elements(form.asLLSD()));
	}
}

//---------------------------------------------------------------------------


std::vector<LLSD> gLoadUrlList;

bool callback_load_url(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);

	if (0 == option)
	{
		LLWeb::loadURL(notification["payload"]["url"].asString());
	}

	return false;
}
static LLNotificationFunctorRegistration callback_load_url_reg("LoadWebPage", callback_load_url);


// We've got the name of the person who owns the object hurling the url.
// Display confirmation dialog.
void callback_load_url_name(const LLUUID& id, const std::string& full_name, bool is_group)
{
	std::vector<LLSD>::iterator it;
	for (it = gLoadUrlList.begin(); it != gLoadUrlList.end(); )
	{
		LLSD load_url_info = *it;
		if (load_url_info["owner_id"].asUUID() == id)
		{
			it = gLoadUrlList.erase(it);

			std::string owner_name;
			if (is_group)
			{
				owner_name = full_name + LLTrans::getString("Group");
			}
			else
			{
				owner_name = full_name;
			}

			// For legacy name-only mutes.
			if (LLMuteList::getInstance()->isMuted(LLUUID::null, owner_name))
			{
				continue;
			}
			LLSD args;
			args["URL"] = load_url_info["url"].asString();
			args["MESSAGE"] = load_url_info["message"].asString();;
			args["OBJECTNAME"] = load_url_info["object_name"].asString();
			args["NAME"] = owner_name;

			LLNotificationsUtil::add("LoadWebPage", args, load_url_info);
		}
		else
		{
			++it;
		}
	}
}

void process_load_url(LLMessageSystem* msg, void**)
{
	LLUUID object_id;
	LLUUID owner_id;
	BOOL owner_is_group;
	char object_name[256];		/* Flawfinder: ignore */
	char message[256];		/* Flawfinder: ignore */
	char url[256];		/* Flawfinder: ignore */

	msg->getString("Data", "ObjectName", 256, object_name);
	msg->getUUID(  "Data", "ObjectID", object_id);
	msg->getUUID(  "Data", "OwnerID", owner_id);
	msg->getBOOL(  "Data", "OwnerIsGroup", owner_is_group);
	msg->getString("Data", "Message", 256, message);
	msg->getString("Data", "URL", 256, url);

	LLSD payload;
	payload["object_id"] = object_id;
	payload["owner_id"] = owner_id;
	payload["owner_is_group"] = owner_is_group;
	payload["object_name"] = object_name;
	payload["message"] = message;
	payload["url"] = url;

	// URL is safety checked in load_url above

	// Check if object or owner is muted
	if (LLMuteList::getInstance()->isMuted(object_id, object_name) ||
	    LLMuteList::getInstance()->isMuted(owner_id))
	{
		LL_INFOS("Messaging")<<"Ignoring load_url from muted object/owner."<<LL_ENDL;
		return;
	}

	// Add to list of pending name lookups
	gLoadUrlList.push_back(payload);

	gCacheName->get(owner_id, owner_is_group,
		boost::bind(&callback_load_url_name, _1, _2, _3));
}


void callback_download_complete(void** data, S32 result, LLExtStat ext_status)
{
	std::string* filepath = (std::string*)data;
	LLSD args;
	args["DOWNLOAD_PATH"] = *filepath;
	LLNotificationsUtil::add("FinishedRawDownload", args);
	delete filepath;
}


void process_initiate_download(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if (agent_id != gAgent.getID())
	{
		LL_WARNS("Messaging") << "Initiate download for wrong agent" << LL_ENDL;
		return;
	}

	std::string sim_filename;
	std::string viewer_filename;
	msg->getString("FileData", "SimFilename", sim_filename);
	msg->getString("FileData", "ViewerFilename", viewer_filename);

	if (!gXferManager->validateFileForRequest(viewer_filename))
	{
		llwarns << "SECURITY: Unauthorized download to local file " << viewer_filename << llendl;
		return;
	}
	gXferManager->requestFile(viewer_filename,
		sim_filename,
		LL_PATH_NONE,
		msg->getSender(),
		FALSE,	// don't delete remote
		callback_download_complete,
		(void**)new std::string(viewer_filename));
}


void process_script_teleport_request(LLMessageSystem* msg, void**)
{
	if (!gSavedSettings.getBOOL("ScriptsCanShowUI")) return;

	std::string object_name;
	std::string sim_name;
	LLVector3 pos;
	LLVector3 look_at;

	msg->getString("Data", "ObjectName", object_name);
	msg->getString("Data", "SimName", sim_name);
	msg->getVector3("Data", "SimPosition", pos);
	msg->getVector3("Data", "LookAt", look_at);

	LLFloaterWorldMap* instance = LLFloaterWorldMap::getInstance();
	if(instance)
	{
		instance->trackURL(
						   sim_name, (S32)pos.mV[VX], (S32)pos.mV[VY], (S32)pos.mV[VZ]);
		LLFloaterReg::showInstance("world_map", "center");
	}
	
	// remove above two lines and replace with below line
	// to re-enable parcel browser for llMapDestination()
	// LLURLDispatcher::dispatch(LLSLURL::buildSLURL(sim_name, (S32)pos.mV[VX], (S32)pos.mV[VY], (S32)pos.mV[VZ]), FALSE);
	
}

void process_covenant_reply(LLMessageSystem* msg, void**)
{
	LLUUID covenant_id, estate_owner_id;
	std::string estate_name;
	U32 covenant_timestamp;
	msg->getUUID("Data", "CovenantID", covenant_id);
	msg->getU32("Data", "CovenantTimestamp", covenant_timestamp);
	msg->getString("Data", "EstateName", estate_name);
	msg->getUUID("Data", "EstateOwnerID", estate_owner_id);

	LLPanelEstateCovenant::updateEstateName(estate_name);
	LLPanelLandCovenant::updateEstateName(estate_name);
	LLPanelEstateInfo::updateEstateName(estate_name);
	LLFloaterBuyLand::updateEstateName(estate_name);

	std::string owner_name =
		LLSLURL("agent", estate_owner_id, "inspect").getSLURLString();
	LLPanelEstateCovenant::updateEstateOwnerName(owner_name);
	LLPanelLandCovenant::updateEstateOwnerName(owner_name);
	LLPanelEstateInfo::updateEstateOwnerName(owner_name);
	LLFloaterBuyLand::updateEstateOwnerName(owner_name);

	LLPanelPlaceProfile* panel = LLFloaterSidePanelContainer::getPanel<LLPanelPlaceProfile>("places", "panel_place_profile");
	if (panel)
	{
		panel->updateEstateName(estate_name);
		panel->updateEstateOwnerName(owner_name);
	}

	// standard message, not from system
	std::string last_modified;
	if (covenant_timestamp == 0)
	{
		last_modified = LLTrans::getString("covenant_last_modified")+LLTrans::getString("never_text");
	}
	else
	{
		last_modified = LLTrans::getString("covenant_last_modified")+"["
						+LLTrans::getString("LTimeWeek")+"] ["
						+LLTrans::getString("LTimeMonth")+"] ["
						+LLTrans::getString("LTimeDay")+"] ["
						+LLTrans::getString("LTimeHour")+"]:["
						+LLTrans::getString("LTimeMin")+"]:["
						+LLTrans::getString("LTimeSec")+"] ["
						+LLTrans::getString("LTimeYear")+"]";
		LLSD substitution;
		substitution["datetime"] = (S32) covenant_timestamp;
		LLStringUtil::format (last_modified, substitution);
	}

	LLPanelEstateCovenant::updateLastModified(last_modified);
	LLPanelLandCovenant::updateLastModified(last_modified);
	LLFloaterBuyLand::updateLastModified(last_modified);

	// load the actual covenant asset data
	const BOOL high_priority = TRUE;
	if (covenant_id.notNull())
	{
		gAssetStorage->getEstateAsset(gAgent.getRegionHost(),
									gAgent.getID(),
									gAgent.getSessionID(),
									covenant_id,
                                    LLAssetType::AT_NOTECARD,
									ET_Covenant,
                                    onCovenantLoadComplete,
									NULL,
									high_priority);
	}
	else
	{
		std::string covenant_text;
		if (estate_owner_id.isNull())
		{
			// mainland
			covenant_text = LLTrans::getString("RegionNoCovenant");
		}
		else
		{
			covenant_text = LLTrans::getString("RegionNoCovenantOtherOwner");
		}
		LLPanelEstateCovenant::updateCovenantText(covenant_text, covenant_id);
		LLPanelLandCovenant::updateCovenantText(covenant_text);
		LLFloaterBuyLand::updateCovenantText(covenant_text, covenant_id);
		if (panel)
		{
			panel->updateCovenantText(covenant_text);
		}
	}
}

void onCovenantLoadComplete(LLVFS *vfs,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					void* user_data, S32 status, LLExtStat ext_status)
{
	LL_DEBUGS("Messaging") << "onCovenantLoadComplete()" << LL_ENDL;
	std::string covenant_text;
	if(0 == status)
	{
		LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
		
		S32 file_length = file.getSize();
		
		std::vector<char> buffer(file_length+1);
		file.read((U8*)&buffer[0], file_length);		
		// put a EOS at the end
		buffer[file_length] = '\0';
		
		if( (file_length > 19) && !strncmp( &buffer[0], "Linden text version", 19 ) )
		{
			LLViewerTextEditor::Params params;
			params.name("temp");
			params.max_text_length(file_length+1);
			LLViewerTextEditor * editor = LLUICtrlFactory::create<LLViewerTextEditor> (params);
			if( !editor->importBuffer( &buffer[0], file_length+1 ) )
			{
				LL_WARNS("Messaging") << "Problem importing estate covenant." << LL_ENDL;
				covenant_text = "Problem importing estate covenant.";
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				covenant_text = editor->getText();
			}
			delete editor;
		}
		else
		{
			LL_WARNS("Messaging") << "Problem importing estate covenant: Covenant file format error." << LL_ENDL;
			covenant_text = "Problem importing estate covenant: Covenant file format error.";
		}
	}
	else
	{
		LLViewerStats::getInstance()->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		
		if( LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE == status ||
		    LL_ERR_FILE_EMPTY == status)
		{
			covenant_text = "Estate covenant notecard is missing from database.";
		}
		else if (LL_ERR_INSUFFICIENT_PERMISSIONS == status)
		{
			covenant_text = "Insufficient permissions to view estate covenant.";
		}
		else
		{
			covenant_text = "Unable to load estate covenant at this time.";
		}
		
		LL_WARNS("Messaging") << "Problem loading notecard: " << status << LL_ENDL;
	}
	LLPanelEstateCovenant::updateCovenantText(covenant_text, asset_uuid);
	LLPanelLandCovenant::updateCovenantText(covenant_text);
	LLFloaterBuyLand::updateCovenantText(covenant_text, asset_uuid);

	LLPanelPlaceProfile* panel = LLFloaterSidePanelContainer::getPanel<LLPanelPlaceProfile>("places", "panel_place_profile");
	if (panel)
	{
		panel->updateCovenantText(covenant_text);
	}
}


void process_feature_disabled_message(LLMessageSystem* msg, void**)
{
	// Handle Blacklisted feature simulator response...
	LLUUID	agentID;
	LLUUID	transactionID;
	std::string	messageText;
	msg->getStringFast(_PREHASH_FailureInfo,_PREHASH_ErrorMessage, messageText,0);
	msg->getUUIDFast(_PREHASH_FailureInfo,_PREHASH_AgentID,agentID);
	msg->getUUIDFast(_PREHASH_FailureInfo,_PREHASH_TransactionID,transactionID);
	
	LL_WARNS("Messaging") << "Blacklisted Feature Response:" << messageText << LL_ENDL;
}

// ------------------------------------------------------------
// Message system exception callbacks
// ------------------------------------------------------------

void invalid_message_callback(LLMessageSystem* msg,
								   void*,
								   EMessageException exception)
{
    LLAppViewer::instance()->badNetworkHandler();
}

// Please do not add more message handlers here. This file is huge.
// Put them in a file related to the functionality you are implementing.

void LLOfferInfo::forceResponse(InventoryOfferResponse response)
{
	LLNotification::Params params("UserGiveItem");
	params.functor.function(boost::bind(&LLOfferInfo::inventory_offer_callback, this, _1, _2));
	LLNotifications::instance().forceResponse(params, response);
}

