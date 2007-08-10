/** 
 * @file llviewermessage.cpp
 * @brief Dumping ground for viewer-side message system callbacks.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewermessage.h"

#include <deque>
#include <stdio.h>
#include <string.h>

#include "audioengine.h" 
#include "audiosettings.h"
#include "indra_constants.h"
#include "lscript_byteformat.h"
#include "mean_collision_data.h"
#include "llfloaterbump.h"
#include "llassetstorage.h"
#include "llcachename.h"
#include "llchat.h"
#include "lldbstrings.h"
#include "lleconomy.h"
#include "llfilepicker.h"
#include "llfloaterimport.h"
#include "llfocusmgr.h"
#include "llfollowcamparams.h"
#include "llinstantmessage.h"
#include "llquantize.h"
#include "llregionflags.h"
#include "llregionhandle.h"
#include "llsdserialize.h"
#include "llstring.h"
#include "llteleportflags.h"
#include "lltracker.h"
#include "lltransactionflags.h"
#include "llwindow.h"			// shell_open()
#include "llxfermanager.h"
#include "message.h"
#include "sound_ids.h"
#include "lltimer.h"
#include "llmd5.h"

#include "llagent.h"
#include "llcallingcard.h"
#include "llconsole.h"
#include "llviewercontrol.h"
#include "lldrawpool.h"
#include "llfirstuse.h"
#include "llfloateractivespeakers.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterchat.h"
#include "llfloatergroupinfo.h"
#include "llfloaterimagepreview.h"
#include "llfloaterland.h"
#include "llfloaterregioninfo.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermute.h"
#include "llfloaterpostcard.h"
#include "llfloaterpreference.h"
#include "llfloaterreleasemsg.h"
#include "llfollowcam.h"
#include "llgroupnotify.h"
#include "llhudeffect.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimpanel.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llmenugl.h"
#include "llmutelist.h"
#include "llnetmap.h"
#include "llnotify.h"
#include "llpanelgrouplandmoney.h"
#include "llselectmgr.h"
#include "llstartup.h"
#include "llsky.h"
#include "llstatenums.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolmgr.h"
#include "llui.h"			// for make_ui_sound
#include "lluploaddialog.h"
#include "llviewercamera.h"
#include "llviewergenericmessage.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerpartsource.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewertexteditor.h"
#include "llviewerthrottle.h"
#include "llviewerwindow.h"
#include "llvlmanager.h"
#include "llvoavatar.h"
#include "llvotextbubble.h"
#include "llweb.h"
#include "llworld.h"
#include "pipeline.h"
#include "viewer.h"
#include "llfloaterworldmap.h"

#include <boost/tokenizer.hpp>

#if LL_WINDOWS // For Windows specific error handler
#include "llwindebug.h"	// For the invalid message handler
#endif

//
// Constants
//
const F32 BIRD_AUDIBLE_RADIUS = 32.0f;
const F32 SIT_DISTANCE_FROM_TARGET = 0.25f;
static const F32 LOGOUT_REPLY_TIME = 3.f;	// Wait this long after LogoutReply before quitting.
extern BOOL gDebugClicks;

extern void bad_network_handler();

// function prototypes
void open_offer(const std::vector<LLUUID>& items, const std::string& from_name);
void friendship_offer_callback(S32 option, void* user_data);
bool check_offer_throttle(const std::string& from_name, bool check_only);

//inventory offer throttle globals
LLFrameTimer gThrottleTimer;
const U32 OFFER_THROTTLE_MAX_COUNT=5; //number of items per time period
const F32 OFFER_THROTTLE_TIME=10.f; //time period in seconds

//script permissions
const LLString SCRIPT_QUESTIONS[SCRIPT_PERMISSION_EOF] = 
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
		"ControlYourCamera"
	};

struct LLFriendshipOffer
{
	LLUUID mFromID;
	LLUUID mTransactionID;
	BOOL mOnline;
	LLHost mHost;
};

//const char BUSY_AUTO_RESPONSE[] =	"The Resident you messaged is in 'busy mode' which means they have "
//									"requested not to be disturbed. Your message will still be shown in their IM "
//									"panel for later viewing.";

//
// Functions
//

void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group,
				S32 trx_type, const LLString& desc)
{
	if(0 == amount) return;
	amount = abs(amount);
	llinfos << "give_money(" << uuid << "," << amount << ")"<< llendl;
	if(can_afford_transaction(amount))
	{
//		gStatusBar->debitBalance(amount);
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_MoneyTransferRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, agent_get_id());
		msg->addUUIDFast(_PREHASH_SessionID, agent_get_session_id());
		msg->nextBlockFast(_PREHASH_MoneyData);
		msg->addUUIDFast(_PREHASH_SourceID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_DestID, uuid);
		msg->addU8Fast(_PREHASH_Flags, pack_transaction_flags(FALSE, is_group));
		msg->addS32Fast(_PREHASH_Amount, amount);
		msg->addU8Fast(_PREHASH_AggregatePermNextOwner, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addU8Fast(_PREHASH_AggregatePermInventory, (U8)LLAggregatePermissions::AP_EMPTY);
		msg->addS32Fast(_PREHASH_TransactionType, trx_type );
		msg->addStringFast(_PREHASH_Description, desc.c_str());
		msg->sendReliable(region->getHost());
	}
	else
	{
		LLFloaterBuyCurrency::buyCurrency("Giving", amount);
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
	llinfos << "process_logout_reply" << llendl;

	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	LLUUID session_id;
	msg->getUUID("AgentData", "SessionID", session_id);
	if((agent_id != agent_get_id()) || (session_id != agent_get_session_id()))
	{
		llwarns << "Bogus Logout Reply" << llendl;
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
		llinfos << "process_logout_reply itemID=" << item_id << llendl;
		LLInventoryItem* item = gInventory.getItem( item_id );
		if( item )
		{
			parents[item->getParentUUID()] = 0;
			gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		}
		else
		{
			llinfos << "process_logout_reply item not found: " << item_id << llendl;
		}
	}
	if(!parents.empty())
	{
		gInventory.accountForUpdate(parents);
		gInventory.notifyObservers();
	}
	app_force_quit(NULL);
}

void process_layer_data(LLMessageSystem *mesgsys, void **user_data)
{
	LLViewerRegion *regionp = gWorldp->getRegion(mesgsys->getSender());

	if (!regionp || gNoRender)
	{
		return;
	}


	S32 size;
	S8 type;

	mesgsys->getS8Fast(_PREHASH_LayerID, _PREHASH_Type, type);
	size = mesgsys->getSizeFast(_PREHASH_LayerData, _PREHASH_Data);
	if (0 == size)
	{
		llwarns << "Layer data has zero size." << llendl;
		return;
	}
	if (size < 0)
	{
		// getSizeFast() is probably trying to tell us about an error
		llwarns << "getSizeFast() returned negative result: "
			<< size
			<< llendl;
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

S32 exported_object_count = 0;
S32 exported_image_count = 0;
S32 current_object_count = 0;
S32 current_image_count = 0;

extern LLNotifyBox *gExporterNotify;
extern LLUUID gExporterRequestID;
extern LLString gExportDirectory;

extern LLUploadDialog *gExportDialog;

LLString gExportedFile;

std::map<LLUUID, LLString> gImageChecksums;

void export_complete()
{
		LLUploadDialog::modalUploadFinished();
		gExporterRequestID.setNull();
		gExportDirectory = "";

		FILE* fXML = LLFile::fopen(gExportedFile.c_str(), "rb");		/* Flawfinder: ignore */
		fseek(fXML, 0, SEEK_END);
		long length = ftell(fXML);
		fseek(fXML, 0, SEEK_SET);
		U8 *buffer = new U8[length + 1];
		size_t nread = fread(buffer, 1, length, fXML);
		if (nread < (size_t) length)
		{
			llwarns << "Short read" << llendl;
		}
		buffer[nread] = '\0';
		fclose(fXML);

		char *pos = (char *)buffer;
		while ((pos = strstr(pos+1, "<sl:image ")) != 0)
		{
			char *pos_check = strstr(pos, "checksum=\"");

			if (pos_check)
			{
				char *pos_uuid = strstr(pos_check, "\">");

				if (pos_uuid)
				{
					char image_uuid_str[UUID_STR_SIZE];		/* Flawfinder: ignore */
					memcpy(image_uuid_str, pos_uuid+2, UUID_STR_SIZE-1);		/* Flawfinder: ignore */
					image_uuid_str[UUID_STR_SIZE-1] = 0;
					
					LLUUID image_uuid(image_uuid_str);

					llinfos << "Found UUID: " << image_uuid << llendl;

					std::map<LLUUID, LLString>::iterator itor = gImageChecksums.find(image_uuid);
					if (itor != gImageChecksums.end())
					{
						llinfos << "Replacing with checksum: " << itor->second << llendl;
						if (itor->second.c_str() != NULL)
							{
								memcpy(&pos_check[10], itor->second.c_str(), 32);		/* Flawfinder: ignore */
							}
					}
				}
			}
		}

		FILE* fXMLOut = LLFile::fopen(gExportedFile.c_str(), "wb");		/* Flawfinder: ignore */
		if (fwrite(buffer, 1, length, fXMLOut) != length)
		{
			llwarns << "Short write" << llendl;
		}
		fclose(fXMLOut);

		delete [] buffer;
}


void exported_item_complete(const LLTSCode status, void *user_data)
{
	//LLString *filename = (LLString *)user_data;

	if (status < LLTS_OK)
	{
		llinfos << "Export failed!" << llendl;
	}
	else
	{
		++current_object_count;
		if (current_image_count == exported_image_count && current_object_count == exported_object_count)
		{
			llinfos << "*** Export complete ***" << llendl;

			export_complete();
		}
		else
		{
			gExportDialog->setMessage(llformat("Exported %d/%d object files, %d/%d textures.", current_object_count, exported_object_count, current_image_count, exported_image_count));
		}
	}
}

struct exported_image_info
{
	LLUUID image_id;
	LLString filename;
	U32 image_num;
};

void exported_j2c_complete(const LLTSCode status, void *user_data)
{
	exported_image_info *info = (exported_image_info *)user_data;
	LLUUID image_id = info->image_id;
	U32 image_num = info->image_num;
	LLString filename = info->filename;
	delete info;

	if (status < LLTS_OK)
	{
		llinfos << "Image download failed!" << llendl;
	}
	else
	{
		FILE* fIn = LLFile::fopen(filename.c_str(), "rb");		/* Flawfinder: ignore */
		if (fIn) 
		{
			LLPointer<LLImageJ2C> ImageUtility = new LLImageJ2C;
			LLPointer<LLImageTGA> TargaUtility = new LLImageTGA;

			fseek(fIn, 0, SEEK_END);
			S32 length = ftell(fIn);
			fseek(fIn, 0, SEEK_SET);
			U8 *buffer = ImageUtility->allocateData(length);
			if (fread(buffer, 1, length, fIn) != length)
			{
				llwarns << "Short read" << llendl;
			}
			fclose(fIn);
			LLFile::remove(filename.c_str());

			// Convert to TGA
			LLPointer<LLImageRaw> image = new LLImageRaw();

			ImageUtility->updateData();
			ImageUtility->decode(image, 100000.0f);
			
			TargaUtility->encode(image);
			U8 *data = TargaUtility->getData();
			S32 data_size = TargaUtility->getDataSize();

			char *file_path = new char[filename.size()+1];
			strcpy(file_path, filename.c_str());		/* Flawfinder: ignore */
			char *end = strrchr(file_path, gDirUtilp->getDirDelimiter()[0]);
			end[0] = 0;
			LLString output_file = llformat("%s/image-%03d.tga", file_path, image_num);//filename;
			delete [] file_path;
			//S32 name_len = output_file.length();
			//strcpy(&output_file[name_len-3], "tga");
			FILE* fOut = LLFile::fopen(output_file.c_str(), "wb");		/* Flawfinder: ignore */
			char md5_hash_string[33];		/* Flawfinder: ignore */
			strcpy(md5_hash_string, "00000000000000000000000000000000");		/* Flawfinder: ignore */
			if (fOut)
			{
				if (fwrite(data, 1, data_size, fOut) != data_size)
				{
					llwarns << "Short write" << llendl;
				}
				fseek(fOut, 0, SEEK_SET);
				fclose(fOut);
				fOut = LLFile::fopen(output_file.c_str(), "rb");		/* Flawfinder: ignore */
				LLMD5 my_md5_hash(fOut);
				my_md5_hash.hex_digest(md5_hash_string);
			}

			gImageChecksums.insert(std::pair<LLUUID, LLString>(image_id, md5_hash_string));
		}
	}

	++current_image_count;
	if (current_image_count == exported_image_count && current_object_count == exported_object_count)
	{
		llinfos << "*** Export textures complete ***" << llendl;
			export_complete();
	}
	else
	{
		gExportDialog->setMessage(llformat("Exported %d/%d object files, %d/%d textures.", current_object_count, exported_object_count, current_image_count, exported_image_count));
	}
}

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
		llwarns << "Got invalid PlacesReply message" << llendl;
	}
}

void send_sound_trigger(const LLUUID& sound_id, F32 gain)
{
	if (sound_id.isNull())
	{
		// zero guids don't get sent (no sound)
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

struct LLJoinGroupData
{
	LLUUID mGroupID;
	LLUUID mTransactionID;
	std::string mName;
	std::string mMessage;
	S32 mFee;
};

void join_group_callback(S32 option, void* user_data)
{
	LLJoinGroupData* data = (LLJoinGroupData*)user_data;
	BOOL delete_context_data = TRUE;
	bool accept_invite = false;
	if(option == 0 && data && !data->mGroupID.isNull())
	{
		// check for promotion or demotion.
		S32 max_groups = MAX_AGENT_GROUPS;
		if(gAgent.isInGroup(data->mGroupID)) ++max_groups;

		if(gAgent.mGroups.count() < max_groups)
		{
			accept_invite = true;
		}
		else
		{
			delete_context_data = FALSE;
			LLString::format_map_t args;
			args["[NAME]"] = data->mName;
			args["[INVITE]"] = data->mMessage;
			LLAlertDialog::showXml("JoinedTooManyGroupsMember", args, join_group_callback, (void*)data);
		}
	}

	if (accept_invite)
	{
		// If there is a fee to join this group, make
		// sure the user is sure they want to join.
		if (data->mFee > 0)
		{
			delete_context_data = FALSE;
			LLString::format_map_t args;
			args["[COST]"] = llformat("%d", data->mFee);
			// Set the fee to 0, so that we don't keep
			// asking about a fee.
			data->mFee = 0;
			gViewerWindow->alertXml("JoinGroupCanAfford",
									args,
									join_group_callback,
									(void*)data);
		}
		else
		{
			send_improved_im(data->mGroupID,
							"name",
							"message",
							IM_ONLINE,
							IM_GROUP_INVITATION_ACCEPT,
							data->mTransactionID);
		}
	}
	else if (data)
	{
		send_improved_im(data->mGroupID,
						"name",
						"message",
						IM_ONLINE,
						IM_GROUP_INVITATION_DECLINE,
						data->mTransactionID);
	}

	if(delete_context_data)
	{
		delete data;
		data = NULL;
	}
}


//-----------------------------------------------------------------------------
// Instant Message
//-----------------------------------------------------------------------------
class LLOpenAgentOffer : public LLInventoryFetchObserver
{
public:
	LLOpenAgentOffer(const std::string& from_name) : mFromName(from_name) {}
	/*virtual*/ void done()
	{
		open_offer(mComplete, mFromName);
		gInventory.removeObserver(this);
		delete this;
	}
private:
	std::string mFromName;
};

//unlike the FetchObserver for AgentOffer, we only make one 
//instance of the AddedObserver for TaskOffers
//and it never dies.  We do this because we don't know the UUID of 
//task offers until they are accepted, so we don't wouldn't 
//know what to watch for, so instead we just watch for all additions. -Gigs
class LLOpenTaskOffer : public LLInventoryAddedObserver
{
protected:
	/*virtual*/ void done()
	{
		open_offer(mAdded, "");
		mAdded.clear();
	}
 };

//one global instance to bind them
LLOpenTaskOffer* gNewInventoryObserver=NULL;

void start_new_inventory_observer()
{
	if (!gNewInventoryObserver) //task offer observer 
	{
		// Observer is deleted by gInventory
		gNewInventoryObserver = new LLOpenTaskOffer;
		gInventory.addObserver(gNewInventoryObserver);
	}
}

class LLDiscardAgentOffer : public LLInventoryFetchComboObserver
{
public:
	LLDiscardAgentOffer(const LLUUID& folder_id, const LLUUID& object_id) :
		mFolderID(folder_id),
		mObjectID(object_id) {}
	virtual ~LLDiscardAgentOffer() {}
	virtual void done()
	{
		lldebugs << "LLDiscardAgentOffer::done()" << llendl;
		LLUUID trash_id;
		trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
		bool notify = false;
		if(trash_id.notNull() && mObjectID.notNull())
		{
			LLInventoryModel::update_list_t update;
			LLInventoryModel::LLCategoryUpdate old_folder(mFolderID, -1);
			update.push_back(old_folder);
			LLInventoryModel::LLCategoryUpdate new_folder(trash_id, 1);
			update.push_back(new_folder);
			gInventory.accountForUpdate(update);
			gInventory.moveObject(mObjectID, trash_id);
			LLInventoryObject* obj = gInventory.getObject(mObjectID);
			if(obj)
			{
				// no need to restamp since this is already a freshly
				// stamped item.
				obj->updateParentOnServer(FALSE);
				notify = true;
			}
		}
		else
		{
			llwarns << "DiscardAgentOffer unable to find: "
					<< (trash_id.isNull() ? "trash " : "")
					<< (mObjectID.isNull() ? "object" : "") << llendl;
		}
		gInventory.removeObserver(this);
		if(notify)
		{
			gInventory.notifyObservers();
		}
		delete this;
	}
protected:
	LLUUID mFolderID;
	LLUUID mObjectID;
};


//Returns TRUE if we are OK, FALSE if we are throttled
//Set check_only true if you want to know the throttle status 
//without registering a hit -Gigs
bool check_offer_throttle(const std::string& from_name, bool check_only)
{
	static U32 throttle_count;
	static bool throttle_logged;
	LLChat chat;
	LLString log_message;

	if (!gSavedSettings.getBOOL("ShowNewInventory"))
		return false;

	if (check_only)
	{
		return gThrottleTimer.hasExpired();
	}
	
	if(gThrottleTimer.checkExpirationAndReset(OFFER_THROTTLE_TIME))
	{
		//llinfos << "Throttle Expired" << llendl;
		throttle_count=1;
		throttle_logged=false;
		return true;
	}
	else //has not expired
	{
		//llinfos << "Throttle Not Expired, Count: " << throttle_count << llendl;
		// When downloading the initial inventory we get a lot of new items
		// coming in and can't tell that from spam.  JC
		if (gStartupState >= STATE_STARTED
			&& throttle_count >= OFFER_THROTTLE_MAX_COUNT)
		{
			if (!throttle_logged)
			{
				// Use the name of the last item giver, who is probably the person
				// spamming you. JC
				std::ostringstream message;
				message << gSecondLife;
				if (!from_name.empty())
				{
					message << ": Items coming in too fast from " << from_name;
				}
				else
				{
					message << ": Items coming in too fast";
				}
				message << ", automatic preview disabled for "
					<< OFFER_THROTTLE_TIME << " seconds.";
				chat.mText = message.str();
				//this is kinda important, so actually put it on screen
				LLFloaterChat::addChat(chat, FALSE, FALSE);
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
 
void open_offer(const std::vector<LLUUID>& items, const std::string& from_name)
{
	std::vector<LLUUID>::const_iterator it = items.begin();
	std::vector<LLUUID>::const_iterator end = items.end();
	LLUUID trash_id(gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH));
	LLInventoryItem* item;
	for(; it != end; ++it)
	{
		item = gInventory.getItem(*it);
		if(!item)
		{
			llwarns << "Unable to show inventory item: " << *it << llendl;
			continue;
		}
		if(gInventory.isObjectDescendentOf(*it, trash_id))
		{
			continue;
		}
		//if we are throttled, don't display them - Gigs
		if (check_offer_throttle(from_name, false))
		{
			// I'm not sure this is a good idea.  JC
			bool show_keep_discard = item->getPermissions().getCreator() != gAgent.getID();
			//bool show_keep_discard = true;
			switch(item->getType())
			{
			case LLAssetType::AT_NOTECARD:
				open_notecard((LLViewerInventoryItem*)item, LLString("Note: ") + item->getName(), LLUUID::null, show_keep_discard, LLUUID::null, FALSE);
				break;
			case LLAssetType::AT_LANDMARK:
				open_landmark((LLViewerInventoryItem*)item, LLString("Landmark: ") + item->getName(), show_keep_discard, LLUUID::null, FALSE);
				break;
			case LLAssetType::AT_TEXTURE:
				open_texture(*it, LLString("Texture: ") + item->getName(), show_keep_discard, LLUUID::null, FALSE);
				break;
			default:
				break;
			}
		}
		//highlight item, if it's not in the trash or lost+found
		
		// Don't auto-open the inventory floater
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if(!view)
		{
			return;
		}

		//Trash Check
		LLUUID trash_id;
		trash_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
		if(gInventory.isObjectDescendentOf(item->getUUID(), trash_id))
		{
			return;
		}
		LLUUID lost_and_found_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
		//BOOL inventory_has_focus = gFocusMgr.childHasKeyboardFocus(view);
		BOOL user_is_away = gAwayTimer.getStarted();

		// don't select lost and found items if the user is active
		if (gInventory.isObjectDescendentOf(item->getUUID(), lost_and_found_id)
			&& !user_is_away)
		{
			return;
		}

		//Not sure about this check.  Could make it easy to miss incoming items. -Gigs
		//don't dick with highlight while the user is working
		//if(inventory_has_focus && !user_is_away)
		//	break;
		//llinfos << "Highlighting" << item->getUUID()  << llendl;
		//highlight item

		LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
		LLFocusMgr::FocusLostCallback callback;
		callback = gFocusMgr.getFocusCallback();
		view->getPanel()->setSelection(item->getUUID(), TAKE_FOCUS_NO);
		gFocusMgr.setKeyboardFocus(focus_ctrl, callback);
	}
}

void inventory_offer_mute_callback(const LLUUID& blocked_id,
								   const char* first_name,
								   const char* last_name,
								   BOOL is_group,
								   void* user_data)
{
	LLString from_name;
	LLMute::EType type;

	if (is_group)
	{
		type = LLMute::GROUP;
		from_name = first_name;
	}
	else
	{
		type = LLMute::AGENT;
		from_name += first_name;
		from_name += " ";
		from_name += last_name;
	}

	LLMute mute(blocked_id, from_name, type);
	if (gMuteListp->add(mute))
	{
		gFloaterMute->show();
		gFloaterMute->selectMute(blocked_id);
	}

	// purge the offer queue of any previously queued inventory offers from the same source.
	LLView::child_list_t notification_queue(*(gNotifyBoxView->getChildList()));
	for(LLView::child_list_iter_t iter = notification_queue.begin();
		iter != notification_queue.end();
		iter++)
	{
		LLNotifyBox* notification = (LLNotifyBox*)*iter;
		// scan for other inventory offers (i.e. ignore other types of notifications).
		// we can tell by looking for the associated callback they were created with.
		if(notification->getNotifyCallback() == inventory_offer_callback)
		{
			// found one.
			// safe to downcast user data because we know it's associated with offer callback.
			LLOfferInfo* offer_data = (LLOfferInfo*)notification->getUserData();
			if(offer_data == user_data)
			{
				continue; // don't remove the msg triggering us. it will be dequeued normally.
			}
			if(offer_data->mFromID == blocked_id) 
			{
				gNotifyBoxView->removeChild(notification);
			}
		}
	}
}

void inventory_offer_callback(S32 button, void* user_data)
 {
	LLChat chat;
	LLString log_message;

	LLOfferInfo* info = (LLOfferInfo*)user_data;
	if(!info) return;

	// For muting, we need to add the mute, then decline the offer.
	// This must be done here because:
	// * callback may be called immediately,
	// * adding the mute sends a message,
	// * we can't build two messages at once.  JC
	if (2 == button)
	{
		gCacheName->get(info->mFromID, info->mFromGroup, inventory_offer_mute_callback, user_data);
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, info->mFromID);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addUUIDFast(_PREHASH_ID, info->mTransactionID);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary
	std::string name;
	gAgent.buildFullname(name);
	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, ""); 
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
	LLInventoryObserver* opener = NULL;
	LLViewerInventoryCategory* catp = NULL;
	catp = (LLViewerInventoryCategory*)gInventory.getCategory(info->mObjectID);
	LLViewerInventoryItem* itemp = NULL;
	if(!catp)
	{
		itemp = (LLViewerInventoryItem*)gInventory.getItem(info->mObjectID);
	}

	// XUI:translate
	LLString from_string; // Used in the pop-up.
	LLString chatHistory_string;  // Used in chat history.
	if (info->mFromObject == TRUE)
	{
		if (info->mFromGroup)
		{
			char group_name[MAX_STRING];		/* Flawfinder: ignore */
			if (gCacheName->getGroupName(info->mFromID, group_name))
			{
				from_string = LLString("An object named '") + info->mFromName + "' owned by the group '" + group_name + "'";
				chatHistory_string = info->mFromName + " owned by the group '" + group_name + "'";
			}
			else
			{
				from_string = LLString("An object named '") + info->mFromName + "' owned by an unknown group";
				chatHistory_string = info->mFromName + " owned by an unknown group";
			}
		}
		else
		{
			char first_name[MAX_STRING];		/* Flawfinder: ignore */
			char last_name[MAX_STRING];		/* Flawfinder: ignore */
			if (gCacheName->getName(info->mFromID, first_name, last_name))
			{
				from_string = LLString("An object named '") + info->mFromName + "' owned by " + first_name + " " + last_name;
				chatHistory_string = info->mFromName + " owned by " + first_name + " " + last_name;
			}
			else
			{
				from_string = LLString("An object named '") + info->mFromName + "' owned by an unknown user";
				chatHistory_string = info->mFromName + " owned by an unknown user";
			}
		}
	}
	else
	{
		from_string = chatHistory_string = info->mFromName;
	}
	
	bool busy=FALSE;
	
	switch(button)
	{
	case IOR_ACCEPT:
		// ACCEPT. The math for the dialog works, because the accept
		// for inventory_offered, task_inventory_offer or
		// group_notice_inventory is 1 greater than the offer integer value.
		// Generates IM_INVENTORY_ACCEPTED, IM_TASK_INVENTORY_ACCEPTED, 
		// or IM_GROUP_NOTICE_INVENTORY_ACCEPTED
		msg->addU8Fast(_PREHASH_Dialog, (U8)(info->mIM + 1));
		msg->addBinaryDataFast(_PREHASH_BinaryBucket, &(info->mFolderID.mData),
					 sizeof(info->mFolderID.mData));
		// send the message
		msg->sendReliable(info->mHost);

		//don't spam them if they are getting flooded
		if (check_offer_throttle(info->mFromName, true))
		{
			log_message = chatHistory_string + " gave you " + info->mDesc + ".";
 			chat.mText = log_message;
 			LLFloaterChat::addChatHistory(chat);
		}

		// we will want to open this item when it comes back.
		lldebugs << "Initializing an opener for tid: " << info->mTransactionID
				 << llendl;
		switch (info->mIM)
		{
		case IM_INVENTORY_OFFERED:
		{
			// This is an offer from an agent. In this case, the back
			// end has already copied the items into your inventory,
			// so we can fetch it out of our inventory.
			LLInventoryFetchObserver::item_ref_t items;
			items.push_back(info->mObjectID);
			LLOpenAgentOffer* open_agent_offer = new LLOpenAgentOffer(from_string);
			open_agent_offer->fetchItems(items);
			if(catp || (itemp && itemp->isComplete()))
			{
				open_agent_offer->done();
			}
			else
			{
				opener = open_agent_offer;
			}
		}
			break;
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
			llwarns << "inventory_offer_callback: unknown offer type" << llendl;
			break;
		}	// end switch (info->mIM)
		break;

	case IOR_BUSY:
		//Busy falls through to decline.  Says to make busy message.
		busy=TRUE;
	case IOR_MUTE:
		// MUTE falls through to decline
	case IOR_DECLINE:
		// DECLINE. The math for the dialog works, because the decline
		// for inventory_offered, task_inventory_offer or
		// group_notice_inventory is 2 greater than the offer integer value.
		// Generates IM_INVENTORY_DECLINED, IM_TASK_INVENTORY_DECLINED,
		// or IM_GROUP_NOTICE_INVENTORY_DECLINED
	default:
		// close button probably
		msg->addU8Fast(_PREHASH_Dialog, (U8)(info->mIM + 2));
		msg->addBinaryDataFast(_PREHASH_BinaryBucket, EMPTY_BINARY_BUCKET, EMPTY_BINARY_BUCKET_SIZE);
		// send the message
		msg->sendReliable(info->mHost);

		log_message = "You decline " + info->mDesc + " from " + info->mFromName + ".";
		chat.mText = log_message;
		if( gMuteListp->isMuted(info->mFromID ) && ! gMuteListp->isLinden(info->mFromName) )  // muting for SL-42269
		{
			chat.mMuted = TRUE;
		}
		LLFloaterChat::addChatHistory(chat);

		// If it's from an agent, we have to fetch the item to throw
		// it away. If it's from a task or group, just denying the 
		// request will suffice to discard the item.
		if(IM_INVENTORY_OFFERED == info->mIM)
		{
			LLInventoryFetchComboObserver::folder_ref_t folders;
			LLInventoryFetchComboObserver::item_ref_t items;
			items.push_back(info->mObjectID);
			LLDiscardAgentOffer* discard_agent_offer;
			discard_agent_offer = new LLDiscardAgentOffer(info->mFolderID, info->mObjectID);
			discard_agent_offer->fetch(folders, items);
			if(catp || (itemp && itemp->isComplete()))
			{
				discard_agent_offer->done();
			}
			else
			{
				opener = discard_agent_offer;
			}
			
		}
		if (busy || (!info->mFromGroup && !info->mFromObject))
		{
			busy_message(msg,info->mFromID);
		}
		break;
	}

	if(opener)
	{
		gInventory.addObserver(opener);
	}

	delete info;
	info = NULL;

	// Allow these to stack up, but once you deal with one, reset the
	// position.
	gFloaterView->resetStartingFloaterPosition();
}


void inventory_offer_handler(LLOfferInfo* info, BOOL from_task)
{
	//Until throttling is implmented, busy mode should reject inventory instead of silently
	//accepting it.  SEE SL-39554
	if (gAgent.getBusy())
	{
		inventory_offer_callback(IOR_BUSY, info);
		return;
	}
	
	//If muted, don't even go through the messaging stuff.  Just curtail the offer here.
	if (gMuteListp->isMuted(info->mFromID, info->mFromName))
	{
		inventory_offer_callback(IOR_MUTE, info);
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
		inventory_offer_callback(IOR_ACCEPT, info);
		return;
	}

	LLString::format_map_t args;
	args["[OBJECTNAME]"] = info->mDesc;
	args["[OBJECTTYPE]"] = LLAssetType::lookupHumanReadable(info->mType);

	// Name cache callbacks don't store userdata, so can't save
	// off the LLOfferInfo.  Argh.  JC
	BOOL name_found = FALSE;
	char first_name[MAX_STRING];		/* Flawfinder: ignore */
	char last_name[MAX_STRING];		/* Flawfinder: ignore */
	if (info->mFromGroup)
	{
		if (gCacheName->getGroupName(info->mFromID, first_name))
		{
			args["[FIRST]"] = first_name;
			args["[LAST]"] = "";
			name_found = TRUE;
		}
	}
	else
	{
		if (gCacheName->getName(info->mFromID, first_name, last_name))
		{
			args["[FIRST]"] = first_name;
			args["[LAST]"] = last_name;
			name_found = TRUE;
		}
	}
	if (from_task)
	{
		args["[OBJECTFROMNAME]"] = info->mFromName;
		LLNotifyBox::showXml(name_found ? "ObjectGiveItem" : "ObjectGiveItemUnknownUser",
							args, &inventory_offer_callback, (void*)info);
	}
	else
	{
		// XUI:translate -> [FIRST] [LAST]
		args["[NAME]"] = info->mFromName;
		LLNotifyBox::showXml("UserGiveItem", args,
							&inventory_offer_callback, (void*)info);
	}
}


void group_vote_callback(S32 option, void *userdata)
{
	LLUUID *group_id = (LLUUID *)userdata;
	if (!group_id) return;

	switch(option)
	{
	case 0:
		// Vote Now
		// Open up the voting tab
		LLFloaterGroupInfo::showFromUUID(*group_id, "voting_tab");
		break;
	default:
		// Vote Later or
		// close button
		break;
	}
	delete group_id;
	group_id = NULL;
}

struct LLLureInfo
{
	LLLureInfo(const LLUUID& from, const LLUUID& lure_id, BOOL godlike) :
		mFromID(from),
		mLureID(lure_id),
		mGodlike(godlike)
	{}

	LLUUID mFromID;
	LLUUID mLureID;
	BOOL mGodlike;
};

void lure_callback(S32 option, void* user_data)
{
	LLLureInfo* info = (LLLureInfo*)user_data;
	if(!info) return;
	switch(option)
	{
	case 0:
		{
			// accept
			send_simple_im(info->mFromID,
						   "",
						   IM_LURE_ACCEPTED,
						   info->mLureID);
			gAgent.teleportViaLure(info->mLureID, info->mGodlike);
		}
		break;
	case 1:
	default:
		// decline
		send_simple_im(info->mFromID,
					   "",
					   IM_LURE_DECLINED,
					   info->mLureID);
		break;
	}
	delete info;
	info = NULL;
}

void goto_url_callback(S32 option, void* user_data)
{
	char* url = (char*)user_data;
	if(1 == option)
	{
		LLWeb::loadURL(url);
	}
	delete[] url;
}

void process_improved_im(LLMessageSystem *msg, void **user_data)
{
	if (gNoRender)
	{
		return;
	}
	LLUUID from_id;
	BOOL from_group;
	LLUUID to_id;
	U8 offline;
	U8 d = 0;
	LLUUID session_id;
	U32 t;
	char name[DB_FULL_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	char message[DB_IM_MSG_BUF_SIZE];		/* Flawfinder: ignore */
	U32 parent_estate_id = 0;
	LLUUID region_id;
	LLVector3 position;
	char buffer[DB_IM_MSG_BUF_SIZE * 2];		/* Flawfinder: ignore */
	U8 binary_bucket[MTUBYTES];
	S32 binary_bucket_size;
	LLChat chat;

	//XUI:translate - need to fix the full name to first/last
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, from_id);
	msg->getBOOLFast(_PREHASH_MessageBlock, _PREHASH_FromGroup, from_group);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ToAgentID, to_id);
	msg->getU8Fast(  _PREHASH_MessageBlock, _PREHASH_Offline, offline);
	msg->getU8Fast(  _PREHASH_MessageBlock, _PREHASH_Dialog, d);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_ID, session_id);
	msg->getU32Fast( _PREHASH_MessageBlock, _PREHASH_Timestamp, t);
	//msg->getData("MessageBlock", "Count",		&count);
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_FromAgentName, DB_FULL_NAME_BUF_SIZE, name);
	msg->getStringFast(_PREHASH_MessageBlock, _PREHASH_Message,		DB_IM_MSG_BUF_SIZE, message);
	msg->getU32Fast(_PREHASH_MessageBlock, _PREHASH_ParentEstateID, parent_estate_id);
	msg->getUUIDFast(_PREHASH_MessageBlock, _PREHASH_RegionID, region_id);
	msg->getVector3Fast(_PREHASH_MessageBlock, _PREHASH_Position, position);
	msg->getBinaryDataFast(  _PREHASH_MessageBlock, _PREHASH_BinaryBucket, binary_bucket, 0, 0, MTUBYTES);
	binary_bucket_size = msg->getSizeFast(_PREHASH_MessageBlock, _PREHASH_BinaryBucket);
	EInstantMessage dialog = (EInstantMessage)d;
	time_t timestamp = (time_t)t;

	BOOL is_busy = gAgent.getBusy();
	BOOL is_muted = gMuteListp->isMuted(from_id, name, LLMute::flagTextChat);
	BOOL is_linden = gMuteListp->isLinden(name);
	BOOL is_owned_by_me = FALSE;
	
	chat.mMuted = is_muted && !is_linden;
	chat.mFromID = from_id;
	chat.mFromName = name;

	LLViewerObject *source = gObjectList.findObject(session_id); //Session ID is probably the wrong thing.
	if (source)
	{
		is_owned_by_me = source->permYouOwner();
	}

	char separator_string[3]=": ";		/* Flawfinder: ignore */
	int message_offset=0;

		//Handle IRC styled /me messages.
	if (!strncmp(message, "/me ", 4) || !strncmp(message, "/me'", 4))
	{
		strcpy(separator_string,"");		/* Flawfinder: ignore */
		message_offset=3;
	}

	LLString::format_map_t args;
	switch(dialog)
	{
	case IM_CONSOLE_AND_CHAT_HISTORY:
		// These are used for system messages, hence don't need the name,
		// as it is always "Second Life".
	  	// XUI:translate
		args["[MESSAGE]"] = message;

		// Note: don't put the message in the IM history, even though was sent
		// via the IM mechanism.
		LLNotifyBox::showXml("SystemMessageTip",args);
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
		else if (offline == IM_ONLINE && !is_linden && is_busy && strcmp(name, SYSTEM_FROM))
		{
			// return a standard "busy" message, but only do it to online IM 
			// (i.e. not other auto responses and not store-and-forward IM)
			if (!gIMMgr->hasSession(session_id))
			{
				// if there is not a panel for this conversation (i.e. it is a new IM conversation
				// initiated by the other party) then...
				std::string my_name;
				gAgent.buildFullname(my_name);
				LLString response = gSavedPerAccountSettings.getText("BusyModeResponse");
				pack_instant_message(
					gMessageSystem,
					gAgent.getID(),
					FALSE,
					gAgent.getSessionID(),
					from_id,
					my_name.c_str(),
					response.c_str(),
					IM_ONLINE,
					IM_BUSY_AUTO_RESPONSE,
					session_id);
				gAgent.sendReliableMessage();
			}

			// now store incoming IM in chat history

			snprintf(buffer, sizeof(buffer), "%s%s%s", name, separator_string, (message+message_offset));		/* Flawfinder: ignore */
	
			llinfos << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << llendl;

			// add to IM panel, but do not bother the user
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				NULL,
				dialog,
				parent_estate_id,
				region_id,
				position);

			// pretend this is chat generated by self, so it does not show up on screen
			snprintf(buffer, sizeof(buffer), "IM: %s%s%s", name, separator_string, (message+message_offset));		/* Flawfinder: ignore */
			chat.mText = buffer;
			LLFloaterChat::addChat( chat, TRUE, TRUE );
		}
		else if (from_id.isNull())
		{
			// Messages from "Second Life" don't go to IM history
			snprintf(buffer, sizeof(buffer), "%s: %s", name, message);		/* Flawfinder: ignore */
			chat.mText = buffer;
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
		else if (to_id.isNull())
		{
			// Message to everyone from GOD
			args["[NAME]"] = name;
			args["[MESSAGE]"] = message;
			LLNotifyBox::showXml("GodMessage", args);

			// Treat like a system message and put in chat history.
			// Claim to be from a local agent so it doesn't go into
			// console.
			snprintf(buffer, sizeof(buffer), "%s%s%s", name, separator_string, (message+message_offset));		/* Flawfinder: ignore */
			chat.mText = buffer;
			BOOL local_agent = TRUE;
			LLFloaterChat::addChat(chat, FALSE, local_agent);
		}
		else
		{
			// standard message, not from system
			char saved[MAX_STRING];		/* Flawfinder: ignore */
			saved[0] = '\0';
			if(offline == IM_OFFLINE)
			{
				char time_buf[TIME_STR_LENGTH];		/* Flawfinder: ignore */
				snprintf(saved, MAX_STRING, "(Saved %s) ",		/* Flawfinder: ignore */
						formatted_time(timestamp, time_buf));
			}
			snprintf(buffer, sizeof(buffer), "%s%s%s%s", name, separator_string, saved,(message+message_offset));		/* Flawfinder: ignore */

			llinfos << "process_improved_im: session_id( " << session_id << " ), from_id( " << from_id << " )" << llendl;

			if (!is_muted || is_linden)
			{
				gIMMgr->addMessage(
					session_id,
					from_id,
					name,
					buffer,
					NULL,
					dialog,
					parent_estate_id,
					region_id,
					position);
				snprintf(buffer, sizeof(buffer), "IM: %s%s%s%s", name, separator_string, saved, (message+message_offset));		/* Flawfinder: ignore */

				chat.mText = buffer;
				BOOL local_agent = FALSE;
				LLFloaterChat::addChat( chat, TRUE, local_agent );
			}
			else
			{
				// muted user, so don't start an IM session, just record line in chat
				// history.  Pretend the chat is from a local agent,
				// so it will go into the history but not be shown on screen.
				chat.mText = buffer;
				BOOL local_agent = TRUE;
				LLFloaterChat::addChat( chat, TRUE, local_agent );
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
			//XUI:translate
			args["[MESSAGE]"] = message;
			LLNotifyBox::showXml("SystemMessage", args);
		}
		break;
	case IM_GROUP_NOTICE:
	case IM_GROUP_NOTICE_REQUESTED:
		{
			llinfos << "Received IM_GROUP_NOTICE message." << llendl;
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
				llwarns << "Malformed group notice binary bucket" << llendl;
				break;
			}

			notice_bin_bucket = (struct notice_bucket_full_t*) &binary_bucket[0];
			U8 has_inventory = notice_bin_bucket->header.has_inventory;
			U8 asset_type = notice_bin_bucket->header.asset_type;
			LLUUID group_id = notice_bin_bucket->header.group_id;
			const char* item_name = (const char*) notice_bin_bucket->item_name;

			// If there is inventory, give the user the inventory offer.
			LLOfferInfo* info = NULL;
			if (has_inventory)
			{
				info = new LLOfferInfo;
				info->mIM = IM_GROUP_NOTICE;
				info->mFromID = from_id;
				info->mFromGroup = from_group;
				info->mTransactionID = session_id;
				info->mType = (LLAssetType::EType) asset_type;
				info->mFolderID = gInventory.findCategoryUUIDForType(info->mType);
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

			LLString subj(*iter++);
			LLString mes(*iter++);

			if (IM_GROUP_NOTICE == dialog)
			{
				subj += "\n";
				mes = "\n\n" + mes;
				LLGroupNotifyBox::show(subj.c_str(),mes.c_str(),name,group_id,t,has_inventory,item_name,info);
			}
			else if (IM_GROUP_NOTICE_REQUESTED == dialog)
			{
				LLFloaterGroupInfo::showNotice(subj.c_str(),mes.c_str(),group_id,has_inventory,item_name,info);
			}
		}
		break;
	case IM_GROUP_INVITATION:
		{
			//if (!is_linden && (is_busy || is_muted))
			if ((is_busy || is_muted))
			{
				LLMessageSystem *msg = gMessageSystem;
				join_group_callback(1, NULL);
				busy_message(msg,from_id);
			}
			else
			{
				llinfos << "Received IM_GROUP_INVITATION message." << llendl;
				// Read the binary bucket for more information.
				struct invite_bucket_t
				{
					S32 membership_fee;
					LLUUID role_id;
				}* invite_bucket;

				// Make sure the binary bucket is the correct size.
				if (binary_bucket_size != sizeof(invite_bucket_t))
				{
					llwarns << "Malformed group invite binary bucket" << llendl;
					break;
				}

				invite_bucket = (struct invite_bucket_t*) &binary_bucket[0];
				S32 membership_fee = ntohl(invite_bucket->membership_fee);

				LLJoinGroupData* userdata = new LLJoinGroupData;
				userdata->mTransactionID = session_id;
				userdata->mGroupID = from_id;
				userdata->mName.assign(name);
				userdata->mMessage.assign(message);
				userdata->mFee = membership_fee;

				LLString::format_map_t args;
				args["[MESSAGE]"] = message;
				LLNotifyBox::showXml("JoinGroup", args,
									 &join_group_callback,
									 (void*)userdata);
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
					llwarns << "Malformed inventory offer from agent" << llendl;
					break;
				}
				bucketp = (struct offer_agent_bucket_t*) &binary_bucket[0];
				info->mType = (LLAssetType::EType) bucketp->asset_type;
				info->mObjectID = bucketp->object_id;
			}
			else
			{
				if (sizeof(S8) != binary_bucket_size)
				{
					llwarns << "Malformed inventory offer from object" << llendl;
					break;
				}
				info->mType = (LLAssetType::EType) binary_bucket[0];
				info->mObjectID = LLUUID::null;
			}

			info->mIM = dialog;
			info->mFromID = from_id;
			info->mFromGroup = from_group;
			info->mTransactionID = session_id;
			info->mFolderID = gInventory.findCategoryUUIDForType(info->mType);

			if (dialog == IM_TASK_INVENTORY_OFFERED)
			{
				info->mFromObject = TRUE;
			}
			else
			{
				info->mFromObject = FALSE;
			}
			info->mFromName = name;
			info->mDesc = message;
			info->mHost = msg->getSender();
			//if (((is_busy && !is_owned_by_me) || is_muted))
			if ( is_muted )
			{
				// Same as closing window
				inventory_offer_callback(-1, info);
			}
			else
			{
				inventory_offer_handler(info, dialog == IM_TASK_INVENTORY_OFFERED);
			}
		}
		break;

	case IM_INVENTORY_ACCEPTED:
	{
		args["[NAME]"] = name;
		LLNotifyBox::showXml("InventoryAccepted", args);
		break;
	}
	case IM_INVENTORY_DECLINED:
	{
		args["[NAME]"] = name;
		LLNotifyBox::showXml("InventoryDeclined", args);
		break;
	}
	case IM_GROUP_VOTE:
	{
		LLUUID *userdata = new LLUUID(session_id);
		args["[NAME]"] = name;
		args["[MESSAGE]"] = message;
		LLNotifyBox::showXml("GroupVote", args,
							 &group_vote_callback, userdata);
	}
	break;

	case IM_GROUP_ELECTION_DEPRECATED:
	{
		llwarns << "Received IM: IM_GROUP_ELECTION_DEPRECATED" << llendl;
	}
	break;
	
	case IM_SESSION_SEND:
	{
		if (!is_linden && is_busy)
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
		char saved[MAX_STRING];		/* Flawfinder: ignore */
		saved[0] = '\0';
		if(offline == IM_OFFLINE)
		{
			char time_buf[TIME_STR_LENGTH];		/* Flawfinder: ignore */
			snprintf(saved,		/* Flawfinder: ignore */
					 MAX_STRING, 
					 "(Saved %s) ", 
					 formatted_time(timestamp, time_buf));
		}
		snprintf(buffer, sizeof(buffer), "%s%s%s%s", name, separator_string, saved, (message+message_offset));		/* Flawfinder: ignore */
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
			(char*)binary_bucket,
			IM_SESSION_INVITE,
			parent_estate_id,
			region_id,
			position);

		snprintf(buffer, sizeof(buffer), "IM: %s%s%s%s", name, separator_string, saved, (message+message_offset));		/* Flawfinder: ignore */
		chat.mText = buffer;
		LLFloaterChat::addChat(chat, TRUE, is_this_agent);
	}
	break;

	case IM_FROM_TASK:
		if (is_busy && !is_owned_by_me)
		{
			return;
		}
		snprintf(buffer, sizeof(buffer), "%s%s%s", name, separator_string, (message+message_offset));		/* Flawfinder: ignore */
		// Note: lie to LLFloaterChat::addChat(), pretending that this is NOT an IM, because
		// IMs from objcts don't open IM sessions.
		chat.mText = buffer;
		chat.mSourceType = CHAT_SOURCE_OBJECT;
		LLFloaterChat::addChat(chat, FALSE, FALSE);
		break;
	case IM_FROM_TASK_AS_ALERT:
		if (is_busy && !is_owned_by_me)
		{
			return;
		}
		{
			// Construct a viewer alert for this message.
			args["[NAME]"] = name;
			args["[MESSAGE]"] = message;
			LLNotifyBox::showXml("ObjectMessage", args);
		}
		break;
	case IM_BUSY_AUTO_RESPONSE:
		// fix for JIRA issue VWR-20 submitted 13-JAN-2007 - Paul Churchill
		if (is_muted)
		{
			lldebugs << "Ignoring busy response from " << from_id << llendl;
			return;
		}
		else
		{
			// original code resumes
			gIMMgr->addMessage(session_id, from_id, name, message);
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
				// XUI:translate -> [FIRST] [LAST]
				LLLureInfo* info = new LLLureInfo(from_id, session_id, FALSE);
				args["[NAME]"] = name;
				args["[MESSAGE]"] = message;
				LLNotifyBox::showXml("OfferTeleport", args,
									 lure_callback, (void*)info);
			}
		}
		break;

	case IM_GODLIKE_LURE_USER:
		{
			LLLureInfo* info = new LLLureInfo(from_id, session_id, TRUE);
			// do not show a message box, because you're about to be
			// teleported.
			lure_callback(0, (void *)info);
		}
		break;

	case IM_GOTO_URL:
		{
			if (binary_bucket_size <= 0)
			{
				llwarns << "bad binary_bucket_size: "
					<< binary_bucket_size
					<< " - aborting function." << llendl;
				return;
			}

			char* url = new char[binary_bucket_size];
			if (url == NULL)
			{
				llerrs << "Memory Allocation failed" << llendl;
				return;
			}

			strncpy(url, (char*)binary_bucket, binary_bucket_size-1);		/* Flawfinder: ignore */
			url[binary_bucket_size-1] = '\0';
			args["[MESSAGE]"] = message;
			args["[URL]"] = url;
			LLNotifyBox::showXml("GotoURL", args,
								 goto_url_callback, (void*)url);
		}
		break;

	case IM_FRIENDSHIP_OFFERED:
		{
			LLFriendshipOffer* offer = new LLFriendshipOffer;
			offer->mFromID = from_id;
			offer->mTransactionID = session_id;
			offer->mOnline = (offline == IM_ONLINE);
			offer->mHost = msg->getSender();

			if (is_busy)
			{
				busy_message(msg, from_id);
				friendship_offer_callback(1, (void*)offer);
			}
			else if (is_muted)
			{
				friendship_offer_callback(1, (void*)offer);
			}
			else
			{
				args["[NAME]"] = name;
				LLNotifyBox::showXml("OfferFriendship", args,
					&friendship_offer_callback, (void*)offer);
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
			
			args["[NAME]"] = name;
			LLNotifyBox::showXml("FriendshipAccepted", args);
		}
		break;

	case IM_FRIENDSHIP_DECLINED:
		args["[NAME]"] = name;
		LLNotifyBox::showXml("FriendshipDeclined", args);
		break;

	default:
		llwarns << "Instant message calling for unknown dialog "
				<< (S32)dialog << llendl;
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
		LLString response = gSavedPerAccountSettings.getText("BusyModeResponse");
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			from_id,
			SYSTEM_FROM,
			response.c_str(),
			IM_ONLINE,
			IM_CONSOLE_AND_CHAT_HISTORY);
		gAgent.sendReliableMessage();
	}
}

void friendship_offer_callback(S32 option, void* user_data)
{
	LLFriendshipOffer* offer = (LLFriendshipOffer*)user_data;
	if(!offer) return;
	LLUUID fid;
	LLMessageSystem* msg = gMessageSystem;
	switch(option)
	{
	case 0:
		// accept
		LLAvatarTracker::formFriendship(offer->mFromID);

		fid = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);

		// This will also trigger an onlinenotification if the user is online
		msg->newMessageFast(_PREHASH_AcceptFriendship);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, offer->mTransactionID);
		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, fid);
		msg->sendReliable(offer->mHost);
		break;
	case 1:
		// decline
		msg->newMessageFast(_PREHASH_DeclineFriendship);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, offer->mTransactionID);
		msg->sendReliable(offer->mHost);
		break;
	default:
		// close button probably, possibly timed out
		break;
	}

	delete offer;
	offer = NULL;
}

struct LLCallingCardOfferData
{
	LLUUID mTransactionID;
	LLUUID mSourceID;
	LLHost mHost;
};

void callingcard_offer_callback(S32 option, void* user_data)
{
	LLCallingCardOfferData* offerdata = (LLCallingCardOfferData*)user_data;
	if(!offerdata) return;
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
		msg->addUUIDFast(_PREHASH_TransactionID, offerdata->mTransactionID);
		fid = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);
		msg->nextBlockFast(_PREHASH_FolderData);
		msg->addUUIDFast(_PREHASH_FolderID, fid);
		msg->sendReliable(offerdata->mHost);
		break;
	case 1:
		// decline		
		msg->newMessageFast(_PREHASH_DeclineCallingCard);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_TransactionBlock);
		msg->addUUIDFast(_PREHASH_TransactionID, offerdata->mTransactionID);
		msg->sendReliable(offerdata->mHost);
		busy_message(msg, offerdata->mSourceID);
		break;
	default:
		// close button probably, possibly timed out
		break;
	}

	delete offerdata;
	offerdata = NULL;
}

void process_offer_callingcard(LLMessageSystem* msg, void**)
{
	// someone has offered to form a friendship
	lldebugs << "callingcard offer" << llendl;

	LLUUID source_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, source_id);
	LLUUID tid;
	msg->getUUIDFast(_PREHASH_AgentBlock, _PREHASH_TransactionID, tid);

	LLCallingCardOfferData* offerdata = new LLCallingCardOfferData;
	offerdata->mTransactionID = tid;
	offerdata->mSourceID = source_id;
	offerdata->mHost = msg->getSender();

	LLViewerObject* source = gObjectList.findObject(source_id);
	LLString::format_map_t args;
	std::string source_name;
	if(source && source->isAvatar())
	{
		LLNameValue* nvfirst = source->getNVPair("FirstName");
		LLNameValue* nvlast  = source->getNVPair("LastName");
		if (nvfirst && nvlast)
		{
			args["[FIRST]"] = nvfirst->getString();
			args["[LAST]"] = nvlast->getString();
			source_name = std::string(nvfirst->getString()) + " " + nvlast->getString();
		}
	}

	if(!source_name.empty())
	{
		if (gAgent.getBusy() 
			|| gMuteListp->isMuted(source_id, source_name, LLMute::flagTextChat))
		{
			// automatically decline offer
			callingcard_offer_callback(1, (void*)offerdata);
			offerdata = NULL; // pointer was freed by callback
		}
		else
		{
			LLNotifyBox::showXml("OfferCallingCard", args,
					     &callingcard_offer_callback, (void*)offerdata);
			offerdata = NULL; // pointer ownership transferred
		}
	}
	else
	{
		llwarns << "Calling card offer from an unknown source." << llendl;
	}

	delete offerdata; // !=NULL if we didn't give ownership away
	offerdata = NULL;
}

void process_accept_callingcard(LLMessageSystem* msg, void**)
{
	LLNotifyBox::showXml("CallingCardAccepted");
}

void process_decline_callingcard(LLMessageSystem* msg, void**)
{
	LLNotifyBox::showXml("CallingCardDeclined");
}


void process_chat_from_simulator(LLMessageSystem *msg, void **user_data)
{
	LLChat		chat;
	char		mesg[DB_CHAT_MSG_BUF_SIZE];		/* Flawfinder: ignore */
	char		from_name[DB_FULL_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	U8			source_temp;
	U8			type_temp;
	U8			audible_temp;
	LLColor4	color(1.0f, 1.0f, 1.0f, 1.0f);
	LLUUID		from_id;
	LLUUID		owner_id;
	BOOL		is_owned_by_me = FALSE;
	LLViewerObject*	chatter;

	msg->getString("ChatData", "FromName", DB_FULL_NAME_BUF_SIZE, from_name);
	chat.mFromName = from_name;
	
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
	
	BOOL is_self = (from_id == gAgent.getID());
	BOOL is_busy = gAgent.getBusy();

	// Apparently you can receive chat before app is fully initialized, hence
	// gMuteListp can be null. JC
	BOOL is_muted = FALSE;
	BOOL is_linden = FALSE;
	if (gMuteListp)
	{
		is_muted = gMuteListp->isMuted(from_id, from_name, LLMute::flagTextChat)
				   || gMuteListp->isMuted(owner_id);
		is_linden = gMuteListp->isLinden(from_name);
	}

	BOOL is_audible = (CHAT_AUDIBLE_FULLY == chat.mAudible);
	chatter = gObjectList.findObject(from_id);
	if (chatter)
	{
		chat.mPosAgent = chatter->getPositionAgent();
		
		// Make swirly things only for talking objects. (not script debug messages, though)
		if (chat.mSourceType == CHAT_SOURCE_OBJECT 
			&& chat.mChatType != CHAT_TYPE_DEBUG_MSG)
		{
			LLPointer<LLViewerPartSourceChat> psc = new LLViewerPartSourceChat(chatter->getPositionAgent());
			psc->setSourceObject(chatter);
			psc->setColor(color);
			//We set the particles to be owned by the object's owner, 
			//just in case they should be muted by the mute list
			psc->setOwnerUUID(owner_id);
			gWorldPointer->mPartSim.addPartSource(psc);
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
		std::string verb;

		color.setVec(1,1,1,1);
		msg->getStringFast(_PREHASH_ChatData, _PREHASH_Message, DB_CHAT_MSG_BUF_SIZE, mesg);

		BOOL ircstyle = FALSE;

		// Look for IRC-style emotes here so chatbubbles work
		if (!strncmp(mesg, "/me ", 4) || !strncmp(mesg, "/me'", 4))
		{
			chat.mText = from_name;
			chat.mText += (mesg + 3);
			ircstyle = TRUE;
		}
		else
		{
			chat.mText = mesg;
		}

		// Look for the start of typing so we can put "..." in the bubbles.
		if (CHAT_TYPE_START == chat.mChatType)
		{
			gLocalSpeakerMgr->setSpeakerTyping(from_id, TRUE);

			// Might not have the avatar constructed yet, eg on login.
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->startTyping();
			}
			return;
		}
		else if (CHAT_TYPE_STOP == chat.mChatType)
		{
			gLocalSpeakerMgr->setSpeakerTyping(from_id, FALSE);

			// Might not have the avatar constructed yet, eg on login.
			if (chatter && chatter->isAvatar())
			{
				((LLVOAvatar*)chatter)->stopTyping();
			}
			return;
		}

		// We have a real utterance now, so can stop showing "..." and proceed.
		if (chatter && chatter->isAvatar())
		{
			gLocalSpeakerMgr->setSpeakerTyping(from_id, FALSE);
			((LLVOAvatar*)chatter)->stopTyping();

			if (!is_muted && !is_busy)
			{
				visible_in_chat_bubble = gSavedSettings.getBOOL("UseChatBubbles");
				((LLVOAvatar*)chatter)->addChat(chat);
			}
		}

		// Look for IRC-style emotes
		if (ircstyle)
		{
			// Do nothing, ircstyle is fixed above for chat bubbles
		}
		else
		{
			switch(chat.mChatType)
			{
			case CHAT_TYPE_WHISPER:
				if (is_self)
				{
					verb = " whisper: ";
				}
				else
				{
					verb = " whispers: ";
				}
				break;
			case CHAT_TYPE_DEBUG_MSG:
			case CHAT_TYPE_OWNER:
			case CHAT_TYPE_NORMAL:
				verb = ": ";
				break;
			case CHAT_TYPE_SHOUT:
				if (is_self)
				{
					verb = " shout: ";
				}
				else
				{
					verb = " shouts: ";
				}
				break;
			case CHAT_TYPE_START:
			case CHAT_TYPE_STOP:
				llwarns << "Got chat type start/stop in main chat processing." << llendl;
				break;
			default:
				llwarns << "Unknown type " << chat.mChatType << " in chat!" << llendl;
				verb = " say, ";
				break;
			}

			if (is_self)
			{
				chat.mText = "You";
			}
			else
			{
				chat.mText = from_name;
			}
			chat.mText += verb;
			chat.mText += mesg;
		}
		
		if (chatter)
		{
			chat.mPosAgent = chatter->getPositionAgent();
		}

		// truth table:
		// LINDEN	BUSY	MUTED	OWNED_BY_YOU			DISPLAY		STORE IN HISTORY
		// F		F		F		F						Yes			Yes
		// F		F		F		T						Yes			Yes
		// F		F		T		F						No			No
		// F		F		T		T						No			No
		// F		T		F		F						No			Yes
		// F		T		F		T						Yes			Yes
		// F		T		T		F						No			No
		// F		T		T		T						No			No
		// T		*		*		*						Yes			Yes

		chat.mMuted = is_muted && !is_linden;
		
		
		if (!visible_in_chat_bubble 
			&& (is_linden || !is_busy || is_owned_by_me))
		{
			// show on screen and add to history
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
		else
		{
			// just add to chat history
			LLFloaterChat::addChatHistory(chat);
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
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);

	if (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL)
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE, "");
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, "Cancel");
	}

	// Freeze the UI and show progress bar
	// Note: could add data here to differentiate between normal teleport and death.

	if( gAgent.getTeleportState() == LLAgent::TELEPORT_NONE )
	{
		gTeleportDisplay = TRUE;
		gAgent.setTeleportState( LLAgent::TELEPORT_START );
		make_ui_sound("UISndTeleportOut");
		
		// Don't call LLFirstUse::useTeleport here because this could be
		// due to being killed, which would send you home, not to a Telehub
	}
}

void process_teleport_progress(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if((gAgent.getID() != agent_id)
	   || (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE))
	{
		llwarns << "Unexpected teleport progress message." << llendl;
		return;
	}
	U32 teleport_flags = 0x0;
	msg->getU32("Info", "TeleportFlags", teleport_flags);
	if (teleport_flags & TELEPORT_FLAGS_DISABLE_CANCEL)
	{
		gViewerWindow->setProgressCancelButtonVisible(FALSE, "");
	}
	else
	{
		gViewerWindow->setProgressCancelButtonVisible(TRUE, "Cancel");
	}
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	msg->getString("Info", "Message", MAX_STRING, buffer);
	lldebugs << "teleport progress: " << buffer << llendl;

	//Sorta hacky...default to using simulator raw messages
	//if we don't find the coresponding mapping in our progress mappings
	LLString message = buffer;

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
	LLFetchInWelcomeArea() {}
	virtual void done()
	{
		LLIsType is_landmark(LLAssetType::AT_LANDMARK);
		LLIsType is_card(LLAssetType::AT_CALLINGCARD);
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		folder_ref_t::iterator it = mCompleteFolders.begin();
		folder_ref_t::iterator end = mCompleteFolders.end();
		for(; it != end; ++it)
		{
			gInventory.collectDescendentsIf(
				(*it),
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_landmark);
			gInventory.collectDescendentsIf(
				(*it),
				cats,
				items,
				LLInventoryModel::EXCLUDE_TRASH,
				is_card);
		}
		S32 count = items.count();
		for(S32 i = 0; i < count; ++i)
		{
			LLString::format_map_t args;
			args["[NAME]"] = items[i]->getName();
			switch(items[i]->getType())
			{
			case LLAssetType::AT_LANDMARK:
			  	LLNotifyBox::showXml("TeleportToLandmark",args);
				break;
			case LLAssetType::AT_CALLINGCARD:
			  	LLNotifyBox::showXml("TeleportToPerson",args);
				break;
			default:
				break;
			}
		}
		gInventory.removeObserver(this);
		delete this;
	}
};

// Teleport notification from the simulator
// We're going to pretend to be a new agent
void process_teleport_finish(LLMessageSystem* msg, void**)
{
	//llinfos << "Got teleport location message" << llendl;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_Info, _PREHASH_AgentID, agent_id);
	if (agent_id != gAgent.getID())
	{
		llwarns << "Got teleport notification for wrong agent!" << llendl;
		return;
	}

	// Do teleport effect for where you're leaving
	// VEFFECT: TeleportStart
	LLHUDEffectSpiral *effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());
	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gHUDManager->sendEffects();

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
	
	
	char seedCap[STD_STRING_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_Info, _PREHASH_SeedCapability,
		STD_STRING_BUF_SIZE, seedCap);

	// update home location if we are teleporting out of prelude - specific to teleporting to welcome area 
	if((teleport_flags & TELEPORT_FLAGS_SET_HOME_TO_TARGET)
	   && (!gAgent.isGodlike()))
	{
		gAgent.setHomePosRegion(region_handle, pos);

		// get callingcards and landmarks available to the user arriving.
		LLInventoryFetchDescendentsObserver::folder_ref_t folders;
		LLUUID folder_id;
		folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_CALLINGCARD);
		if(folder_id.notNull()) folders.push_back(folder_id);
		folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LANDMARK);
		if(folder_id.notNull()) folders.push_back(folder_id);
		if(!folders.empty())
		{
			LLFetchInWelcomeArea* fetcher = new LLFetchInWelcomeArea;
			fetcher->fetchDescendents(folders);
			if(fetcher->isEverythingComplete())
			{
				fetcher->done();
			}
			else
			{
				gInventory.addObserver(fetcher);
			}
		}
	}

	LLHost sim_host(sim_ip, sim_port);

	// Viewer trusts the simulator.
	gMessageSystem->enableCircuit(sim_host, TRUE);
	LLViewerRegion* regionp =  gWorldp->addRegion(region_handle, sim_host);

/*
	// send camera update to new region
	gAgent.updateCamera();

	// likewise make sure the camera is behind the avatar
	gAgent.resetView(TRUE);
	LLVector3 shift_vector = regionp->getPosRegionFromGlobal(gAgent.getRegion()->getOriginGlobal());
	gAgent.setRegion(regionp);
	gObjectList.shiftObjects(shift_vector);

	if (gAgent.getAvatarObject())
	{
		gAgent.getAvatarObject()->clearChatText();
		gAgent.slamLookAt(look_at);
	}
	gAgent.setPositionAgent(pos);
	gAssetStorage->setUpstream(sim);
	gCacheName->setUpstream(sim);
*/

	// now, use the circuit info to tell simulator about us!
	llinfos << "process_teleport_finish() Enabling "
			<< sim_host << " with code " << msg->mOurCircuitCode << llendl;
	msg->newMessageFast(_PREHASH_UseCircuitCode);
	msg->nextBlockFast(_PREHASH_CircuitCode);
	msg->addU32Fast(_PREHASH_Code, msg->getOurCircuitCode());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_ID, gAgent.getID());
	msg->sendReliable(sim_host);

	send_complete_agent_movement(sim_host);
	gAgent.setTeleportState( LLAgent::TELEPORT_MOVING );
	gAgent.setTeleportMessage(LLAgent::sTeleportProgressMessages["contacting"]);

	regionp->setSeedCapability(std::string(seedCap));

	// Don't send camera updates to the new region until we're
	// actually there...


	// Now do teleport effect for where you're going.
	// VEFFECT: TeleportEnd
	effectp = (LLHUDEffectSpiral *)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
	effectp->setPositionGlobal(gAgent.getPositionGlobal());

	effectp->setColor(LLColor4U(gAgent.getEffectColor()));
	gHUDManager->sendEffects();

//	gTeleportDisplay = TRUE;
//	gTeleportDisplayTimer.reset();
//	gViewerWindow->setShowProgress(TRUE);

	// This could be first use of teleport, so test for that
	LLFirstUse::useTeleport();
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
	gAgentMovementCompleted = TRUE;

	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		llwarns << "Incorrect id in process_agent_movement_complete()"
				<< llendl;
		return;
	}

	llinfos << "process_agent_movement_complete()" << llendl;

	// *TODO: check timestamp to make sure the movement compleation
	// makes sense.
	LLVector3 agent_pos;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_Position, agent_pos);
	LLVector3 look_at;
	msg->getVector3Fast(_PREHASH_Data, _PREHASH_LookAt, look_at);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_Data, _PREHASH_RegionHandle, region_handle);
	
	char version_channel_char[MAX_STRING];
	msg->getString("SimData", "ChannelVersion", MAX_STRING, version_channel_char);

	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp)
	{
		// Could happen if you were immediately god-teleported away on login,
		// maybe other cases.  Continue, but warn.  JC
		llwarns << "agent_movement_complete() with NULL avatarp." << llendl;
	}

	F32 x, y;
	from_region_handle(region_handle, &x, &y);
	LLViewerRegion* regionp = gWorldp->getRegionFromHandle(region_handle);
	if (!regionp)
	{
		if (gAgent.getRegion())
		{
			llwarns << "current region " << gAgent.getRegion()->getOriginGlobal() << llendl;
		}

		llwarns << "Agent being sent to invalid home region: " 
			<< x << ":" << y 
			<< " current pos " << gAgent.getPositionGlobal()
			<< llendl;
		do_disconnect("You were sent to an invalid region.");
		return;

	}

	llinfos << "Changing home region to " << x << ":" << y << llendl;

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

	bool is_teleport = false;

	if( gAgent.getTeleportState() == LLAgent::TELEPORT_MOVING )
	{
		// Force the camera back onto the agent, don't animate. JC
		gAgent.setFocusOnAvatar(TRUE, FALSE);
		gAgent.slamLookAt(look_at);
		gAgent.updateCamera();

		gAgent.setTeleportState( LLAgent::TELEPORT_START_ARRIVAL );

		// set the appearance on teleport since the new sim does not
		// know what you look like.
		gAgent.sendAgentSetAppearance();

		if (avatarp)
		{
			avatarp->setPositionAgent(agent_pos);
			avatarp->clearChat();
			avatarp->slamPosition();
		}

		is_teleport = true;
	}
	else
	{
		gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
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
		else if ( is_teleport )
		{
			//look at the beacon
			LLVector3 global_agent_pos = agent_pos;
			global_agent_pos[0] += x;
			global_agent_pos[1] += y;
			look_at = (LLVector3)beacon_pos - global_agent_pos;
			look_at.normVec();
			gAgent.slamLookAt(look_at);
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

	if (avatarp)
	{
		avatarp->mFootPlane.clearVec();
	}
	
	// reset always run status
	msg->newMessageFast(_PREHASH_SetAlwaysRun);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addBOOLFast(_PREHASH_AlwaysRun, gAgent.getAlwaysRun());
	gAgent.sendReliableMessage();

	
	LLString version_channel = LLString(version_channel_char);

	if (gLastVersionChannel != version_channel)
	{
		//show release message if not on initial login
		if (!gLastVersionChannel.empty())
		{ 
			gLastVersionChannel = version_channel;
			LLFloaterReleaseMsg::show();
		}
		else {
			gLastVersionChannel = version_channel;
		}
	}
	
}

void process_crossed_region(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((gAgent.getID() != agent_id) || (gAgent.getSessionID() != session_id))
	{
		llwarns << "Incorrect id in process_crossed_region()"
				<< llendl;
		return;
	}
	llinfos << "process_crossed_region()" << llendl;

	U32 sim_ip;
	msg->getIPAddrFast(_PREHASH_RegionData, _PREHASH_SimIP, sim_ip);
	U16 sim_port;
	msg->getIPPortFast(_PREHASH_RegionData, _PREHASH_SimPort, sim_port);
	LLHost sim_host(sim_ip, sim_port);
	U64 region_handle;
	msg->getU64Fast(_PREHASH_RegionData, _PREHASH_RegionHandle, region_handle);
	
	char seedCap[STD_STRING_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_RegionData, _PREHASH_SeedCapability, STD_STRING_BUF_SIZE, seedCap);

	send_complete_agent_movement(sim_host);

	LLViewerRegion* regionp = gWorldp->addRegion(region_handle, sim_host);
	regionp->setSeedCapability(std::string(seedCap));
}



// Sends avatar and camera information to simulator.
// Sent roughly once per frame, or 20 times per second, whichever is less often

const F32 THRESHOLD_HEAD_ROT_QDOT = 0.9997f;	// ~= 2.5 degrees -- if its less than this we need to update head_rot
const F32 MAX_HEAD_ROT_QDOT = 0.99999f;			// ~= 0.5 degrees -- if its greater than this then no need to update head_rot
												// between these values we delay the updates (but no more than one second)


void send_agent_update(BOOL force_send, BOOL send_reliable)
{
	if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
	{
		// We don't care if they want to send an agent update, they're not allowed to until the simulator
		// that's the target is ready to receive them (after avatar_init_complete is received)
		return;
	}

	// We have already requested to log out.  Don't send agent updates.
	if(gLogoutRequestSent)
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

	camera_pos_agent = gAgent.getCameraPositionAgent();

	render_state = gAgent.getRenderState();

	U32		control_flag_change = 0;
	U8		flag_change = 0;

	cam_center_chg = last_camera_pos_agent - camera_pos_agent;
	cam_rot_chg = last_camera_at - gCamera->getAtAxis();

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

	flag_change = last_flags ^ flags;

	head_rot_chg = dot(last_head_rot, head_rotation);

	if (force_send || 
		(cam_center_chg.magVec() > TRANSLATE_THRESHOLD) || 
		(head_rot_chg < THRESHOLD_HEAD_ROT_QDOT) ||	
		(last_render_state != render_state) ||
		(cam_rot_chg.magVec() > ROTATION_THRESHOLD) ||
		control_flag_change != 0 ||
		flag_change != 0)  
	{
/*
		if (head_rot_chg < THRESHOLD_HEAD_ROT_QDOT)
		{
			//llinfos << "head rot " << head_rotation << llendl;
			llinfos << "head_rot_chg = " << head_rot_chg << llendl;
		}
		if (cam_rot_chg.magVec() > ROTATION_THRESHOLD) 
		{
			llinfos << "cam rot " <<  cam_rot_chg.magVec() << llendl;
		}
		if (cam_center_chg.magVec() > TRANSLATE_THRESHOLD)
		{
			llinfos << "cam center " << cam_center_chg.magVec() << llendl;
		}
//		if (drag_delta_chg.magVec() > TRANSLATE_THRESHOLD)
//		{
//			llinfos << "drag delta " << drag_delta_chg.magVec() << llendl;
//		}
		if (control_flag_change)
		{
			llinfos << "dcf = " << control_flag_change << llendl;
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
//			llinfos << "Sending camera center " << camera_pos_agent << llendl;
//		}
		
		msg->addVector3Fast(_PREHASH_CameraCenter, camera_pos_agent);
		msg->addVector3Fast(_PREHASH_CameraAtAxis, gCamera->getAtAxis());
		msg->addVector3Fast(_PREHASH_CameraLeftAxis, gCamera->getLeftAxis());
		msg->addVector3Fast(_PREHASH_CameraUpAxis, gCamera->getUpAxis());
		msg->addF32Fast(_PREHASH_Far, gAgent.mDrawDistance);
		
		msg->addU32Fast(_PREHASH_ControlFlags, control_flags);

		if (gDebugClicks)
		{
			if (control_flags & AGENT_CONTROL_LBUTTON_DOWN)
			{
				llinfos << "AgentUpdate left button down" << llendl;
			}

			if (control_flags & AGENT_CONTROL_LBUTTON_UP)
			{
				llinfos << "AgentUpdate left button up" << llendl;
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

		//llinfos << "agent " << avatar_pos_agent << " cam " << camera_pos_agent << llendl;

		// Copy the old data 
		last_head_rot = head_rotation;
		last_render_state = render_state;
		last_camera_pos_agent = camera_pos_agent;
		last_camera_at = gCamera->getAtAxis();
		last_camera_left = gCamera->getLeftAxis();
		last_camera_up = gCamera->getUpAxis();
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
	stop_glerror();
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
	stop_glerror();
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
	stop_glerror();
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



void process_kill_object(LLMessageSystem *mesgsys, void **user_data)
{
	LLFastTimer t(LLFastTimer::FTM_PROCESS_OBJECTS);

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
			//llinfos << "Unknown kill for local " << local_id << llendl;
			gObjectList.mNumUnknownKills++;
			continue;
		}
		else
		{
			//llinfos << "Kill message for local " << local_id << llendl;
		}

		gSelectMgr->removeObjectFromSelections(id);

		// ...don't kill the avatar
		if (!(id == gAgentID))
		{
			LLViewerObject *objectp = gObjectList.findObject(id);
			if (objectp)
			{
				// Display green bubble on kill
				if ( gShowObjectUpdates )
				{
					LLViewerObject* newobject;
					newobject = gObjectList.createObjectViewer(LL_PCODE_LEGACY_TEXT_BUBBLE, objectp->getRegion());

					LLVOTextBubble* bubble = (LLVOTextBubble*) newobject;

					bubble->mColor.setVec(0.f, 1.f, 0.f, 1.f);
					bubble->setScale( 2.0f * bubble->getScale() );
					bubble->setPositionGlobal(objectp->getPositionGlobal());
					gPipeline.addObject(bubble);
				}

				// Do the kill
				gObjectList.killObject(objectp);
			}
			else
			{
				llwarns << "Object in UUID lookup, but not on object list in kill!" << llendl;
				gObjectList.mNumUnknownKills++;
			}
		}
	}
}

void process_time_synch(LLMessageSystem *mesgsys, void **user_data)
{
	LLVector3 sun_direction;
	LLVector3 sun_ang_velocity;
	F32 phase;
	U64	space_time_usec;

	// "SimulatorViewerTimeMessage"
	mesgsys->getU64Fast(_PREHASH_TimeInfo, _PREHASH_UsecSinceStart, space_time_usec);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerDay, gSecondsPerDay);
	mesgsys->getU32Fast(_PREHASH_TimeInfo, _PREHASH_SecPerYear, gSecondsPerYear);

	// This should eventually be moved to an "UpdateHeavenlyBodies" message
	mesgsys->getF32Fast(_PREHASH_TimeInfo, _PREHASH_SunPhase, phase);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunDirection, sun_direction);
	mesgsys->getVector3Fast(_PREHASH_TimeInfo, _PREHASH_SunAngVelocity, sun_ang_velocity);

	gWorldp->setSpaceTimeUSec(space_time_usec);

	//lldebugs << "time_synch() - " << sun_direction << ", " << sun_ang_velocity
	//		 << ", " << phase << llendl;

	gSky.setSunPhase(phase);
	gSky.setSunTargetDirection(sun_direction, sun_ang_velocity);
	if (!gNoRender && !(gSavedSettings.getBOOL("SkyOverrideSimSunPosition") || gSky.getOverrideSun()))
	{
		gSky.setSunDirection(sun_direction, sun_ang_velocity);
	}
}

void process_sound_trigger(LLMessageSystem *msg, void **)
{
	if (!gAudiop) return;
	if (!gParcelMgr) return;
	if (!gMuteListp) return;

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
	if (!gParcelMgr->canHearSound(pos_global)) return;

	// Don't play sounds triggered by someone you muted.
	if (gMuteListp->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	
	// Don't play sounds from an object you muted
	if (gMuteListp->isMuted(object_id)) return;

	// Don't play sounds from an object whose parent you muted
	if (parent_id.notNull()
		&& gMuteListp->isMuted(parent_id))
	{
		return;
	}

	F32 volume = gain * gSavedSettings.getF32("AudioLevelSFX");
	gAudiop->triggerSound(sound_id, owner_id, volume, pos_global);
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

	if (gMuteListp->isMuted(object_id)) return;
	if (gMuteListp->isMuted(owner_id, LLMute::flagObjectSounds)) return;
	
	LLAudioSource *sourcep = objectp->getAudioSource(owner_id);
	if (!sourcep) return;
	
	LLAudioData *datap = gAudiop->getAudioData(sound_id);

	// Note that I don't actually do any loading of the
	// audio data into a buffer at this point, as it won't actually
	// help us out.

	// Add audioData starts a transfer internally.
	sourcep->addAudioData(datap, FALSE);
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
	
	if (gMuteListp->isMuted(object_id)) return;
	
	if (gMuteListp->isMuted(owner_id, LLMute::flagObjectSounds)) return;

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

/* Unused July 2006
void process_attached_sound_cutoff_radius(LLMessageSystem *mesgsys, void **user_data)
{
	F32 radius = 0;
	LLUUID object_guid;
	LLViewerObject *objectp = NULL;

	mesgsys->getUUIDFast(_PREHASH_DataBlock, _PREHASH_ObjectID, object_guid);

	if (!((objectp = gObjectList.findObject(object_guid))))
	{
		// we don't know about this object, just bail
		return;
	}

	mesgsys->getF32Fast(_PREHASH_DataBlock, _PREHASH_Radius, radius);

	if (gAudiop)
	{
//		gAudiop->attachToObject(sound_guid, object_guid, gain, priority, flags);
	}
}
*/

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
			gViewerStats->mSimTimeDilation.addValue(stat_value);
			break;
		case LL_SIM_STAT_FPS:
			gViewerStats->mSimFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PHYSFPS:
			gViewerStats->mSimPhysicsFPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTUPS:
			gViewerStats->mSimAgentUPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_FRAMEMS:
			gViewerStats->mSimFrameMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NETMS:
			gViewerStats->mSimNetMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMOTHERMS:
			gViewerStats->mSimSimOtherMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SIMPHYSICSMS:
			gViewerStats->mSimSimPhysicsMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_AGENTMS:
			gViewerStats->mSimAgentMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_IMAGESMS:
			gViewerStats->mSimImagesMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_SCRIPTMS:
			gViewerStats->mSimScriptMsec.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKS:
			gViewerStats->mSimObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMTASKSACTIVE:
			gViewerStats->mSimActiveObjects.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTMAIN:
			gViewerStats->mSimMainAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMAGENTCHILD:
			gViewerStats->mSimChildAgents.addValue(stat_value);
			break;
		case LL_SIM_STAT_NUMSCRIPTSACTIVE:
			gViewerStats->mSimActiveScripts.addValue(stat_value);
			break;
		case LL_SIM_STAT_LSLIPS:
			gViewerStats->mSimLSLIPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_INPPS:
			gViewerStats->mSimInPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_OUTPPS:
			gViewerStats->mSimOutPPS.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_DOWNLOADS:
			gViewerStats->mSimPendingDownloads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_UPLOADS:
			gViewerStats->mSimPendingUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_PENDING_LOCAL_UPLOADS:
			gViewerStats->mSimPendingLocalUploads.addValue(stat_value);
			break;
		case LL_SIM_STAT_TOTAL_UNACKED_BYTES:
			gViewerStats->mSimTotalUnackedBytes.addValue(stat_value / 1024.f);
			break;
		default:
			llwarns << "Unknown stat id" << stat_id << llendl;
		}
	}

	/*
	msg->getF32Fast(_PREHASH_Statistics, _PREHASH_PhysicsTimeDilation, time_dilation);
	gViewerStats->mSimTDStat.addValue(time_dilation);

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
	gViewerStats->mSimCPUUsageStat.addValue(cpu_usage);
	gViewerStats->mSimMemTotalStat.addValue(sim_mem_total);
	gViewerStats->mSimMemRSSStat.addValue(sim_mem_rss);
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


// This info is requested by the simulator when the agent first logs in
// or when it moves into a simulator in which it did not already have
// a child agent.
/*
void process_avatar_info_request(LLMessageSystem *mesgsys, void **user_data)
{
	llinfos << "process_avatar_info_request()" << llendl;

	// Send the avatar appearance (parameters and texture entry UUIDs)
	gAgent.sendAgentSetAppearance();
	send_agent_update(TRUE, TRUE);
}*/


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
		llwarns << "Received animation state for unknown avatar" << uuid << llendl;
		return;
	}

	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationList);
	S32 num_source_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_AnimationSourceList);

	avatarp->mSignaledAnimations.clear();
	
	if (avatarp->mIsSelf)
	{
		LLUUID object_id;

		for( S32 i = 0; i < num_blocks; i++ )
		{
			mesgsys->getUUIDFast(_PREHASH_AnimationList, _PREHASH_AnimID, animation_id, i);
			mesgsys->getS32Fast(_PREHASH_AnimationList, _PREHASH_AnimSequenceID, anim_sequence_id, i);

			//llinfos << "Anim sequence ID: " << anim_sequence_id << llendl;

			avatarp->mSignaledAnimations[animation_id] = anim_sequence_id;

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
	if( avatarp )
	{
		avatarp->processAvatarAppearance( mesgsys );
	}
	else
	{
		llwarns << "avatar_appearance sent for unknown avatar " << uuid << llendl;
	}
}

void process_camera_constraint(LLMessageSystem *mesgsys, void **user_data)
{
	LLVector4 cameraCollidePlane;
	mesgsys->getVector4Fast(_PREHASH_CameraCollidePlane, _PREHASH_Plane, cameraCollidePlane);

	gAgent.setCameraCollidePlane(cameraCollidePlane);
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

	LLVOAvatar* avatar = gAgent.getAvatarObject();

	if (avatar && dist_vec_squared(camera_eye, camera_at) > 0.0001f)
	{
		gAgent.setSitCamera(sitObjectID, camera_eye, camera_at);
	}
	
	gAgent.mForceMouselook = force_mouselook;

	LLViewerObject* object = gObjectList.findObject(sitObjectID);
	if (object)
	{
		LLVector3 sit_spot = object->getPositionAgent() + (sitPosition * object->getRotation());
		if (!use_autopilot || (avatar && avatar->mIsSitting && avatar->getRoot() == object->getRoot()))
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
		llwarns << "Received sit approval for unknown object " << sitObjectID << llendl;
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
	char	temp_str[NAME_VALUE_BUF_SIZE];		/* Flawfinder: ignore */
	LLUUID	id;
	S32		i, num_blocks;

	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);

	LLViewerObject* object = gObjectList.findObject(id);

	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, NAME_VALUE_BUF_SIZE, temp_str, i);
			llinfos << "Added to object Name Value: " << temp_str << llendl;
			object->addNVPair(temp_str);
		}
	}
	else
	{
		llinfos << "Can't find object " << id << " to add name value pair" << llendl;
	}
}

void process_remove_name_value(LLMessageSystem *mesgsys, void **user_data)
{
	char	temp_str[NAME_VALUE_BUF_SIZE];		/* Flawfinder: ignore */
	LLUUID	id;
	S32		i, num_blocks;

	mesgsys->getUUIDFast(_PREHASH_TaskData, _PREHASH_ID, id);

	LLViewerObject* object = gObjectList.findObject(id);

	if (object)
	{
		num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_NameValueData);
		for (i = 0; i < num_blocks; i++)
		{
			mesgsys->getStringFast(_PREHASH_NameValueData, _PREHASH_NVPair, NAME_VALUE_BUF_SIZE, temp_str, i);
			llinfos << "Removed from object Name Value: " << temp_str << llendl;
			object->removeNVPair(temp_str);
		}
	}
	else
	{
		llinfos << "Can't find object " << id << " to remove name value pair" << llendl;
	}
}

void process_kick_user(LLMessageSystem *msg, void** /*user_data*/)
{
	char message[2048];		/* Flawfinder: ignore */
	message[0] = '\0';

	msg->getStringFast(_PREHASH_UserInfo, _PREHASH_Reason, 2048, message);

	do_disconnect(message);
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
	LLViewerRegion *regionp = gWorldp->getRegion(ip, port);
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
	char desc[STD_STRING_BUF_SIZE] = "";		/* Flawfinder: ignore */

	msg->getS32("MoneyData", "MoneyBalance", balance);
	msg->getS32("MoneyData", "SquareMetersCredit", credit);
	msg->getS32("MoneyData", "SquareMetersCommitted", committed);
	msg->getStringFast(_PREHASH_MoneyData, _PREHASH_Description,	STD_STRING_BUF_SIZE,	desc);
	llinfos << "L$, credit, committed: " << balance << " " << credit << " "
			<< committed << llendl;

	if (gStatusBar)
	{
		S32 old_balance = gStatusBar->getBalance();

		// This is an update, not the first transmission of balance
		if (old_balance != 0)
		{
			// this is actually an update
			if (balance > old_balance)
			{
				LLFirstUse::useBalanceIncrease(balance - old_balance);
			}
			else if (balance < old_balance)
			{
				LLFirstUse::useBalanceDecrease(balance - old_balance);
			}
		}

		gStatusBar->setBalance(balance);
		gStatusBar->setLandCredit(credit);
		gStatusBar->setLandCommitted(committed);
	}

	LLUUID tid;
	msg->getUUID("MoneyData", "TransactionID", tid);
	static std::deque<LLUUID> recent;
	if(desc[0] && gSavedSettings.getBOOL("NotifyMoneyChange")
	   && (std::find(recent.rbegin(), recent.rend(), tid) == recent.rend()))
	{
		// Make the user confirm the transaction, since they might
		// have missed something during an event.
		// XUI:translate
		LLString::format_map_t args;
		args["[MESSAGE]"] = desc;
		LLNotifyBox::showXml("SystemMessage", args);

		// Once the 'recent' container gets large enough, chop some
		// off the beginning.
		const U32 MAX_LOOKBACK = 30;
		const S32 POP_FRONT_SIZE = 12;
		if(recent.size() > MAX_LOOKBACK)
		{
			lldebugs << "Removing oldest transaction records" << llendl;
			recent.erase(recent.begin(), recent.begin() + POP_FRONT_SIZE);
		}
		//lldebugs << "Pushing back transaction " << tid << llendl;
		recent.push_back(tid);
	}
}

void process_agent_alert_message(LLMessageSystem* msgsystem, void** user_data)
{
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, MAX_STRING, buffer);
	BOOL modal = FALSE;
	msgsystem->getBOOL("AlertData", "Modal", modal);
	process_alert_core(buffer, modal);
}

void process_alert_message(LLMessageSystem *msgsystem, void **user_data)
{
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	msgsystem->getStringFast(_PREHASH_AlertData, _PREHASH_Message, MAX_STRING, buffer);
	BOOL modal = FALSE;
	process_alert_core(buffer, modal);
}

void process_alert_core(const char* buffer, BOOL modal)
{
	// make sure the cursor is back to the usual default since the
	// alert is probably due to some kind of error.
	gViewerWindow->getWindow()->resetBusyCount();

	// HACK -- handle callbacks for specific alerts 
	if( !strcmp( buffer, "You died and have been teleported to your home location" ) )
	{
		gViewerStats->incStat(LLViewerStats::ST_KILLED_COUNT);
	}
	else if( !strcmp( buffer, "Home position set." ) )
	{
		// save the home location image to disk
		char temp_str[LL_MAX_PATH];		/* Flawfinder: ignore */
		strncpy(temp_str, gDirUtilp->getLindenUserDir().c_str(), LL_MAX_PATH -1);		/* Flawfinder: ignore */
		temp_str[LL_MAX_PATH -1] = '\0';
		strcat(temp_str, "/");		/* Flawfinder: ignore */	
		strcat(temp_str,SCREEN_HOME_FILENAME);		/* Flawfinder: ignore */
		gViewerWindow->saveSnapshot(temp_str, gViewerWindow->getWindowWidth(), gViewerWindow->getWindowHeight(), FALSE, FALSE);
	}

	// Translate system messages here.
	const char ALERT_PREFIX[] = "ALERT: ";
	const size_t ALERT_PREFIX_LEN = sizeof(ALERT_PREFIX) - 1;
	if (!strncmp(buffer, ALERT_PREFIX, ALERT_PREFIX_LEN))
	{
		// Allow the server to spawn a named alert so that server alerts can be
		// translated out of English. JC
		std::string alert_name(buffer + ALERT_PREFIX_LEN);
		LLAlertDialog::showXml(alert_name);
	}
	else if (buffer[0] == '/')
	{
		// System message is important, show in upper-right box not tip
		LLString text(buffer+1);
		LLString::format_map_t args;
		if (text.substr(0,17) == "RESTART_X_MINUTES")
		{
			S32 mins = 0;
			LLString::convertToS32(text.substr(18), mins);
			args["[MINUTES]"] = llformat("%d",mins);
			LLNotifyBox::showXml("RegionRestartMinutes", args);
		}
		else if (text.substr(0,17) == "RESTART_X_SECONDS")
		{
			S32 secs = 0;
			LLString::convertToS32(text.substr(18), secs);
			args["[SECONDS]"] = llformat("%d",secs);
			LLNotifyBox::showXml("RegionRestartSeconds", args);
		}
		else
		{
			//XUI:translate
			args["[MESSAGE]"] = text;
			LLNotifyBox::showXml("SystemMessage", args);
		}
	}
	else if (modal)
	{
		//XUI:translate
		LLString::format_map_t args;
		args["[ERROR_MESSAGE]"] = buffer;
		gViewerWindow->alertXml("ErrorMessage", args);
	}
	else
	{
		//XUI:translate
		LLString::format_map_t args;
		args["[MESSAGE]"] = buffer;
		LLNotifyBox::showXml("SystemMessageTip", args);
	}
}

LLLinkedList<LLMeanCollisionData>	gMeanCollisionList;
time_t								gLastDisplayedTime = 0;

void handle_show_mean_events(void *)
{
	if (gNoRender)
	{
		return;
	}

	LLFloaterBump::show(NULL);
}

void mean_name_callback(const LLUUID &id, const char *first, const char *last, BOOL always_false, void* data)
{
	if (gNoRender)
	{
		return;
	}

	while(gMeanCollisionList.getLength() > 20)
	{
		gMeanCollisionList.getLastData();
		gMeanCollisionList.deleteCurrentData();
	}

	LLMeanCollisionData *mcd;
	for (mcd = gMeanCollisionList.getFirstData(); mcd; mcd = gMeanCollisionList.getNextData())
	{
		if (mcd->mPerp == id)
		{
			strncpy(mcd->mFirstName, first, DB_FIRST_NAME_BUF_SIZE -1);		/* Flawfinder: ignore */
			mcd->mFirstName[DB_FIRST_NAME_BUF_SIZE -1] = '\0';
			strncpy(mcd->mLastName, last, DB_LAST_NAME_BUF_SIZE -1);		/* Flawfinder: ignore */
			mcd->mLastName[DB_LAST_NAME_BUF_SIZE -1] = '\0';
		}
	}
}

void process_mean_collision_alert_message(LLMessageSystem *msgsystem, void **user_data)
{
	if (gAgent.inPrelude())
	{
		// JC: In prelude, bumping is OK.  This dialog is rather confusing to 
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

		LLMeanCollisionData *mcd;

		BOOL b_found = FALSE;

		for (mcd = gMeanCollisionList.getFirstData(); mcd; mcd = gMeanCollisionList.getNextData())
		{
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
			gMeanCollisionList.addData(mcd);
			const BOOL is_group = FALSE;
			gCacheName->get(perp, is_group, mean_name_callback);
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
	LLGlobalEconomy::processEconomyData(msg, (void**)gGlobalEconomy);

	S32 upload_cost = gGlobalEconomy->getPriceUpload();
	LLFloaterImagePreview::setUploadAmount(upload_cost);

	gMenuHolder->childSetLabelArg("Upload Image", "[COST]", llformat("%d", upload_cost));
	gMenuHolder->childSetLabelArg("Upload Sound", "[COST]", llformat("%d", upload_cost));
	gMenuHolder->childSetLabelArg("Upload Animation", "[COST]", llformat("%d", upload_cost));
	gMenuHolder->childSetLabelArg("Bulk Upload", "[COST]", llformat("%d", upload_cost));
}

class LLScriptQuestionCBData
{
public:
	LLScriptQuestionCBData(const LLUUID &taskid, const LLUUID &itemid, const LLHost &sender, S32 questions, const char *object_name, const char *owner_name)
		: mTaskID(taskid), mItemID(itemid), mSender(sender), mQuestions(questions), mObjectName(object_name), mOwnerName(owner_name)
	{
	}

	LLUUID mTaskID;
	LLUUID mItemID;
	LLHost mSender;
	S32	   mQuestions;
	LLString mObjectName;
	LLString mOwnerName;
};

void notify_cautioned_script_question(LLScriptQuestionCBData* cbdata, S32 orig_questions, BOOL granted)
{
	// only continue if at least some permissions were requested
	if (orig_questions)
	{
		// check to see if the person we are asking

		// "'[OBJECTNAME]', an object owned by '[OWNERNAME]', 
		// located in [REGIONNAME] at [REGIONPOS], 
		// has been <granted|denied> permission to: [PERMISSIONS]."

		LLUIString notice(LLNotifyBox::getTemplateMessage(granted ? "ScriptQuestionCautionChatGranted" : "ScriptQuestionCautionChatDenied"));

		// always include the object name and owner name 
		notice.setArg("[OBJECTNAME]", cbdata->mObjectName);
		notice.setArg("[OWNERNAME]", cbdata->mOwnerName);

		// try to lookup viewerobject that corresponds to the object that
		// requested permissions (here, taskid->requesting object id)
		BOOL foundpos = FALSE;
		LLViewerObject* viewobj = gObjectList.findObject(cbdata->mTaskID);
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
				LLString formatpos = llformat("%.1f, %.1f,%.1f", objpos[VX], objpos[VY], objpos[VZ]);
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
		LLString perms;
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if ((orig_questions & LSCRIPTRunTimePermissionBits[i]) && LLNotifyBox::getTemplateIsCaution(SCRIPT_QUESTIONS[i]))
			{
				count++;
				caution = TRUE;

				// add a comma before the permission description if it is not the first permission
				// added to the list or the last permission to check
				if ((count > 1) && (i < SCRIPT_PERMISSION_EOF))
				{
					perms.append(", ");
				}

				perms.append(LLNotifyBox::getTemplateMessage(SCRIPT_QUESTIONS[i]));
			}
		}

		notice.setArg("[PERMISSIONS]", perms);

		// log a chat message as long as at least one requested permission
		// is a caution permission
		if (caution)
		{
			LLChat chat(notice.getString());
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
	}
}

void script_question_decline_cb(S32 option, void* user_data)
{
	LLMessageSystem *msg = gMessageSystem;
	LLScriptQuestionCBData *cbdata = (LLScriptQuestionCBData *)user_data;
	
	// remember the permissions requested so they can be checked
	// when it comes time to log a chat message
	S32 orig = cbdata->mQuestions;

	// this callback will always decline all permissions requested
	// (any question flags set in the ScriptAnswerYes message
	// will be interpreted as having been granted, so clearing all
	// the bits will deny every permission)
	cbdata->mQuestions = 0;

	// respond with the permissions denial
	msg->newMessageFast(_PREHASH_ScriptAnswerYes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_TaskID, cbdata->mTaskID);
	msg->addUUIDFast(_PREHASH_ItemID, cbdata->mItemID);
	msg->addS32Fast(_PREHASH_Questions, cbdata->mQuestions);
	msg->sendReliable(cbdata->mSender);

	// log a chat message, if appropriate
	notify_cautioned_script_question(cbdata, orig, FALSE);

	delete cbdata;
}

void script_question_cb(S32 option, void* user_data)
{
	LLMessageSystem *msg = gMessageSystem;
	LLScriptQuestionCBData *cbdata = (LLScriptQuestionCBData *)user_data;
	S32 orig = cbdata->mQuestions;

	// check whether permissions were granted or denied
	BOOL allowed = TRUE;
	// the "yes/accept" button is the first button in the template, making it button 0
	// if any other button was clicked, the permissions were denied
	if (option != 0)
	{
		cbdata->mQuestions = 0;
		allowed = FALSE;
	}	

	// reply with the permissions granted or denied
	msg->newMessageFast(_PREHASH_ScriptAnswerYes);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_Data);
	msg->addUUIDFast(_PREHASH_TaskID, cbdata->mTaskID);
	msg->addUUIDFast(_PREHASH_ItemID, cbdata->mItemID);
	msg->addS32Fast(_PREHASH_Questions, cbdata->mQuestions);
	msg->sendReliable(cbdata->mSender);

	// only log a chat message if caution prompts are enabled
	if (gSavedSettings.getBOOL("PermissionsCautionEnabled"))
	{
		// log a chat message, if appropriate
		notify_cautioned_script_question(cbdata, orig, allowed);
	}

	delete cbdata;
}

void process_script_question(LLMessageSystem *msg, void **user_data)
{
	// XUI:translate owner name -> [FIRST] [LAST]

	LLHost sender = msg->getSender();

	LLUUID taskid;
	LLUUID itemid;
	S32		questions;
	char object_name[255];		/* Flawfinder: ignore */
	char owner_name[DB_FULL_NAME_BUF_SIZE];		/* Flawfinder: ignore */

	// taskid -> object key of object requesting permissions
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_TaskID, taskid );
	// itemid -> script asset key of script requesting permissions
	msg->getUUIDFast(_PREHASH_Data, _PREHASH_ItemID, itemid );
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectName, 255, object_name);
	msg->getStringFast(_PREHASH_Data, _PREHASH_ObjectOwner, DB_FULL_NAME_BUF_SIZE, owner_name);
	msg->getS32Fast(_PREHASH_Data, _PREHASH_Questions, questions );

	// don't display permission requests if this object is muted - JS.
	if (gMuteListp->isMuted(taskid)) return;

	LLString script_question;
	if (questions)
	{
		BOOL caution = FALSE;
		S32 count = 0;
		LLString::format_map_t args;
		args["[OBJECTNAME]"] = object_name;
		args["[NAME]"] = owner_name;

		// check the received permission flags against each permission
		for (S32 i = 0; i < SCRIPT_PERMISSION_EOF; i++)
		{
			if (questions & LSCRIPTRunTimePermissionBits[i])
			{
				count++;
				script_question += "    " + LLNotifyBox::getTemplateMessage(SCRIPT_QUESTIONS[i]) + "\n";

				// check whether permission question should cause special caution dialog
				caution |= LLNotifyBox::getTemplateIsCaution(SCRIPT_QUESTIONS[i]);
			}
		}
		args["[QUESTIONS]"] = script_question;

		LLScriptQuestionCBData *cbdata = new LLScriptQuestionCBData(taskid, itemid, sender, questions, object_name, owner_name);

		// check whether cautions are even enabled or not
		if (gSavedSettings.getBOOL("PermissionsCautionEnabled"))
		{
			if (caution)
			{
				// display the caution permissions prompt
				LLNotifyBox::showXml("ScriptQuestionCaution", args, TRUE, script_question_cb, cbdata);
			}
			else
			{
				// display the permissions request normally
				LLNotifyBox::showXml("ScriptQuestion", args, FALSE, script_question_cb, cbdata);
			}
		}
		else
		{
			// fall back to default behavior if cautions are entirely disabled
			LLNotifyBox::showXml("ScriptQuestion", args, FALSE, script_question_cb, cbdata);
		}

	}
}


void process_derez_container(LLMessageSystem *msg, void**)
{
	llwarns << "call to deprecated process_derez_container" << llendl;
}

void container_inventory_arrived(LLViewerObject* object,
								 InventoryObjectList* inventory,
								 S32 serial_num,
								 void* data)
{
	llinfos << "container_inventory_arrived()" << llendl;
	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
	}

	LLInventoryView* view = LLInventoryView::getActiveInventory();

	if (inventory->size() > 2)
	{
		// create a new inventory category to put this in
		LLUUID cat_id;
		cat_id = gInventory.createNewCategory(gAgent.getInventoryRootID(),
											  LLAssetType::AT_NONE,
											  "Acquired Items");

		InventoryObjectList::const_iterator it = inventory->begin();
		InventoryObjectList::const_iterator end = inventory->end();
		for ( ; it != end; ++it)
		{
			if ((*it)->getType() != LLAssetType::AT_CATEGORY &&
				(*it)->getType() != LLAssetType::AT_ROOT_CATEGORY)
			{
				LLInventoryObject* obj = (LLInventoryObject*)(*it);
				LLInventoryItem* item = (LLInventoryItem*)(obj);
				LLUUID item_id;
				item_id.generate();
				S32 creation_date_utc = time_corrected();
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
		if(view)
		{
			view->getPanel()->setSelection(cat_id, TAKE_FOCUS_NO);
		}
	}
	else if (inventory->size() == 2)
	{
		// we're going to get one fake root category as well as the
		// one actual object
		InventoryObjectList::iterator it = inventory->begin();

		if ((*it)->getType() == LLAssetType::AT_CATEGORY ||
			(*it)->getType() == LLAssetType::AT_ROOT_CATEGORY)
		{
			++it;
		}

		LLInventoryItem* item = (LLInventoryItem*)((LLInventoryObject*)(*it));
		LLUUID category = gInventory.findCategoryUUIDForType(item->getType());

		LLUUID item_id;
		item_id.generate();
		S32 creation_date_utc = time_corrected();
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
		if(view)
		{
			view->getPanel()->setSelection(item_id, TAKE_FOCUS_NO);
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

// method to format the time. Buffer should be at least
// TIME_STR_LENGTH long, and the function returns buffer (for use in
// sprintf and the like)
char* formatted_time(const time_t& the_time, char* buffer)
{
	LLString::copy(buffer, ctime(&the_time), TIME_STR_LENGTH);
	buffer[24] = '\0';
	return buffer;
}


void process_teleport_failed(LLMessageSystem *msg, void**)
{
	char reason[STD_STRING_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_Info, _PREHASH_Reason, STD_STRING_BUF_SIZE, reason);

	LLStringBase<char>::format_map_t args;
	args["[REASON]"] = LLAgent::sTeleportErrorMessages[reason];
	gViewerWindow->alertXml("CouldNotTeleportReason", args);

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
		llwarns << "Got teleport notification for wrong agent!" << llendl;
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
		gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
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
	gAgent.slamLookAt(look_at);

	// likewise make sure the camera is behind the avatar
	gAgent.resetView(TRUE);

	// send camera update to new region
	gAgent.updateCamera();

	send_agent_update(TRUE, TRUE);
}

void send_simple_im(const LLUUID& to_id,
					const char* message,
					EInstantMessage dialog,
					const LLUUID& id)
{
	std::string my_name;
	gAgent.buildFullname(my_name);
	send_improved_im(to_id,
					 my_name.c_str(),
					 message,
					 IM_ONLINE,
					 dialog,
					 id,
					 NO_TIMESTAMP,
					 (U8*)EMPTY_BINARY_BUCKET,
					 EMPTY_BINARY_BUCKET_SIZE);
}

void send_group_notice(const LLUUID& group_id,
					   const char* subject,
					   const char* message,
					   const LLInventoryItem* item)
{
	// Put this notice into an instant message form.
	// This will mean converting the item to a binary bucket,
	// and the subject/message into a single field.
	std::string my_name;
	gAgent.buildFullname(my_name);

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
			my_name.c_str(),
			subject_and_message.str().c_str(),
			IM_ONLINE,
			IM_GROUP_NOTICE,
			LLUUID::null,
			NO_TIMESTAMP,
			bucket_to_send,
			bin_bucket_size);
}

void handle_lure_callback(S32 option, const LLString& text, void* userdata)
{
	LLDynamicArray<LLUUID>* invitees = (LLDynamicArray<LLUUID>*)userdata;

	if(0 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_StartLure);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_Info);
		msg->addU8Fast(_PREHASH_LureType, (U8)0); // sim will fill this in.
		msg->addStringFast(_PREHASH_Message, text.c_str());
		for(LLDynamicArray<LLUUID>::iterator itr = invitees->begin(); itr != invitees->end(); ++itr)
		{
			msg->nextBlockFast(_PREHASH_TargetData);
			msg->addUUIDFast(_PREHASH_TargetID, *itr);
		}
		gAgent.sendReliableMessage();
	}

	delete invitees;
	invitees = NULL;
}

void handle_lure_callback_godlike(S32 option, void* userdata)
{
	handle_lure_callback(option, LLString::null, userdata);
}

void handle_lure(const LLUUID& invitee)
{
	LLDynamicArray<LLUUID> ids;
	ids.push_back(invitee);
	handle_lure(ids);
}

// Prompt for a message to the invited user.
void handle_lure(LLDynamicArray<LLUUID>& ids) 
{
	LLDynamicArray<LLUUID>* userdata = new LLDynamicArray<LLUUID>(ids);

	LLString::format_map_t edit_args;
	edit_args["[REGION]"] = gAgent.getRegion()->getName();
	if (gAgent.isGodlike())
	{
		gViewerWindow->alertXmlEditText("OfferTeleportFromGod", edit_args,
										&handle_lure_callback_godlike, userdata,
										NULL, NULL, edit_args);
	}
	else
	{
		gViewerWindow->alertXmlEditText("OfferTeleport", edit_args,
										NULL, NULL,
										handle_lure_callback, userdata, edit_args);
	}
}


void send_improved_im(const LLUUID& to_id,
							const char* name,
							const char* message,
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
					   const char* query_text,
					   U32 query_flags,
					   S32 category,
					   const char* sim_name)
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
		llwarns << "process_user_info_reply - "
				<< "wrong agent id." << llendl;
	}
	
	BOOL im_via_email;
	msg->getBOOLFast(_PREHASH_UserData, _PREHASH_IMViaEMail, im_via_email);
	char email[DB_USER_EMAIL_ADDR_BUF_SIZE];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_UserData, _PREHASH_EMail, DB_USER_EMAIL_ADDR_BUF_SIZE,
					   email);
	char dir_visibility[MAX_STRING];			/* Flawfinder: ignore */	
	msg->getString(
		"UserData", "DirectoryVisibility", MAX_STRING, dir_visibility);

	LLFloaterPreference::updateUserInfo(dir_visibility, im_via_email, email);
	LLFloaterPostcard::updateUserInfo(email);
}


//---------------------------------------------------------------------------
// Script Dialog
//---------------------------------------------------------------------------

const S32 SCRIPT_DIALOG_MAX_BUTTONS = 12;
const S32 SCRIPT_DIALOG_BUTTON_STR_SIZE = 24;
const S32 SCRIPT_DIALOG_MAX_MESSAGE_SIZE = 512;
const char* SCRIPT_DIALOG_HEADER = "Script Dialog:\n";

struct ScriptDialogInfo
{
	LLHost mSender;
	LLUUID mObjectID;
	S32 mChatChannel;
	std::vector<LLString> mButtons;
};

void callback_script_dialog(S32 option, void* data)
{
	ScriptDialogInfo* info = (ScriptDialogInfo*)data;
	if (!info) return;

	// Didn't click "Ignore"
	if (0 != option)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("ScriptDialogReply");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("ObjectID", info->mObjectID);
		msg->addS32("ChatChannel", info->mChatChannel);
		msg->addS32("ButtonIndex", option);
		msg->addString("ButtonLabel", info->mButtons[option-1]);
		msg->sendReliable(info->mSender);
	}

	delete info;
}

void process_script_dialog(LLMessageSystem* msg, void**)
{
	S32 i;

	ScriptDialogInfo* info = new ScriptDialogInfo;

	const S32 messageLength = SCRIPT_DIALOG_MAX_MESSAGE_SIZE + sizeof(SCRIPT_DIALOG_HEADER);
	char message[messageLength]; 		/* Flawfinder: ignore */		// Account for size of "Script Dialog:\n"

	char first_name[DB_FIRST_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	char last_name[DB_GROUP_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	char title[DB_INV_ITEM_NAME_BUF_SIZE];		/* Flawfinder: ignore */
	info->mSender = msg->getSender();

	msg->getUUID("Data", "ObjectID", info->mObjectID);
	msg->getString("Data", "FirstName", DB_FIRST_NAME_BUF_SIZE, first_name);
	msg->getString("Data", "LastName", DB_LAST_NAME_BUF_SIZE, last_name);
	msg->getString("Data", "ObjectName", DB_INV_ITEM_NAME_BUF_SIZE, title);
	msg->getString("Data", "Message", SCRIPT_DIALOG_MAX_MESSAGE_SIZE, message);
	msg->getS32("Data", "ChatChannel", info->mChatChannel);

		// unused for now
	LLUUID image_id;
	msg->getUUID("Data", "ImageID", image_id);

	S32 button_count = msg->getNumberOfBlocks("Buttons");
	if (button_count > SCRIPT_DIALOG_MAX_BUTTONS)
	{
		button_count = SCRIPT_DIALOG_MAX_BUTTONS;
	}

	for (i = 0; i < button_count; i++)
	{
		char tdesc[SCRIPT_DIALOG_BUTTON_STR_SIZE+1];		/* Flawfinder: ignore */
		msg->getString("Buttons", "ButtonLabel", SCRIPT_DIALOG_BUTTON_STR_SIZE + 1,  tdesc, i);
		info->mButtons.push_back(LLString(tdesc));
	}

	LLStringBase<char>::format_map_t args;
	args["[TITLE]"] = title;
	args["[MESSAGE]"] = message;
	if (strlen(first_name) > 0)		/* Flawfinder: ignore */
	{
		args["[FIRST]"] = first_name;
		args["[LAST]"] = last_name;
		LLNotifyBox::showXml("ScriptDialog", args,
							 callback_script_dialog, info,
							 info->mButtons,
							 TRUE);
	}
	else
	{
		args["[GROUPNAME]"] = last_name;
		LLNotifyBox::showXml("ScriptDialogGroup", args,
							 callback_script_dialog, info,
							 info->mButtons,
							 TRUE);
	}
}

//---------------------------------------------------------------------------

struct LoadUrlInfo
{
	LLUUID mObjectID;
	LLUUID mOwnerID;
	BOOL mOwnerIsGroup;
	char mObjectName[256];		/* Flawfinder: ignore */
	char mMessage[256];		/* Flawfinder: ignore */
	char mUrl[256];		/* Flawfinder: ignore */
};

std::vector<LoadUrlInfo*> gLoadUrlList;

void callback_load_url(S32 option, void* data)
{
	LoadUrlInfo* infop = (LoadUrlInfo*)data;
	if (!infop) return;

	if (0 == option)
	{
		LLWeb::loadURL(infop->mUrl);  
	}

	delete infop;
	infop = NULL;
}


// We've got the name of the person who owns the object hurling the url.
// Display confirmation dialog.
void callback_load_url_name(const LLUUID& id, const char* first, const char* last, BOOL is_group, void* data)
{
	std::vector<LoadUrlInfo*>::iterator it;
	for (it = gLoadUrlList.begin(); it != gLoadUrlList.end(); )
	{
		LoadUrlInfo* infop = *it;
		if (infop->mOwnerID == id)
		{
			it = gLoadUrlList.erase(it);

			std::string owner_name(first);
			if (is_group)
			{
				owner_name += " (group)";
			}
			else
			{
				owner_name += " ";
				owner_name += last;
			}

			// TODO: Talk to james about using an id instead of a name for this.
			if (gMuteListp->isMuted(LLUUID::null, owner_name))
			{
				delete infop;
				infop = NULL;
				continue;
			}
			LLString::format_map_t args;
			args["[URL]"] = infop->mUrl;
			args["[MESSAGE]"] = infop->mMessage;
			args["[OBJECTNAME]"] = infop->mObjectName;
			args["[NAME]"] = owner_name;
			LLNotifyBox::showXml("LoadWebPage", args, callback_load_url, infop);
		}
		else
		{
			++it;
		}
	}
}

void process_load_url(LLMessageSystem* msg, void**)
{
	LoadUrlInfo* infop = new LoadUrlInfo;

	msg->getString("Data", "ObjectName", 256, infop->mObjectName);
	msg->getUUID(  "Data", "ObjectID", infop->mObjectID);
	msg->getUUID(  "Data", "OwnerID", infop->mOwnerID);
	msg->getBOOL(  "Data", "OwnerIsGroup", infop->mOwnerIsGroup);
	msg->getString("Data", "Message", 256, infop->mMessage);
	msg->getString("Data", "URL", 256, infop->mUrl);

	// URL is safety checked in load_url above

	// Check if object or owner is muted
	if (gMuteListp->isMuted(infop->mObjectID, infop->mObjectName))
	{
		delete infop;
		infop = NULL;
		return;
	}

	// Add to list of pending name lookups
	gLoadUrlList.push_back(infop);

	gCacheName->get(infop->mOwnerID, infop->mOwnerIsGroup, callback_load_url_name);
}


void callback_download_complete(void** data, S32 result)
{
	LLString* filepath = (LLString*)data;
	LLString::format_map_t args;
	args["[DOWNLOAD_PATH]"] = *filepath;
	gViewerWindow->alertXml("FinishedRawDownload", args);
	delete filepath;
}


void process_initiate_download(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUID("AgentData", "AgentID", agent_id);
	if (agent_id != gAgent.getID())
	{
		llwarns << "Initiate download for wrong agent" << llendl;
		return;
	}

	char sim_filename[MAX_PATH];		/* Flawfinder: ignore */
	char viewer_filename[MAX_PATH];		/* Flawfinder: ignore */
	msg->getString("FileData", "SimFilename", MAX_PATH, sim_filename);
	msg->getString("FileData", "ViewerFilename", MAX_PATH, viewer_filename);

	gXferManager->requestFile(viewer_filename,
		sim_filename,
		LL_PATH_NONE,
		msg->getSender(),
		FALSE,	// don't delete remote
		callback_download_complete,
		(void**)new LLString(viewer_filename));
}


void process_script_teleport_request(LLMessageSystem* msg, void**)
{
	char object_name[256];	/* Flawfinder: ignore */		
	char sim_name[256];		/* Flawfinder: ignore */
	LLVector3 pos;
	LLVector3 look_at;

	msg->getString("Data", "ObjectName", 255, object_name);
	msg->getString(  "Data", "SimName", 255, sim_name);
	msg->getVector3("Data", "SimPosition", pos);
	msg->getVector3("Data", "LookAt", look_at);

	gFloaterWorldMap->trackURL(sim_name, (U32)pos.mV[VX], (U32)pos.mV[VY], (U32)pos.mV[VZ]);
	LLFloaterWorldMap::show(NULL, TRUE);
}

void process_covenant_reply(LLMessageSystem* msg, void**)
{
	LLUUID covenant_id, estate_owner_id;
	char estate_name[MAX_STRING];		/* Flawfinder: ignore */
	U32 covenant_timestamp;
	msg->getUUID("Data", "CovenantID", covenant_id);
	msg->getU32("Data", "CovenantTimestamp", covenant_timestamp);
	msg->getString("Data", "EstateName", MAX_STRING, estate_name);
	msg->getUUID("Data", "EstateOwnerID", estate_owner_id);

	LLPanelEstateCovenant::updateEstateName(estate_name);
	LLPanelLandCovenant::updateEstateName(estate_name);
	LLFloaterBuyLand::updateEstateName(estate_name);

	// standard message, not from system
	char last_modified[MAX_STRING];		/* Flawfinder: ignore */
	last_modified[0] = '\0';
	char time_buf[TIME_STR_LENGTH];		/* Flawfinder: ignore */
	snprintf(last_modified, MAX_STRING, "Last Modified %s",		/* Flawfinder: ignore */
	formatted_time((time_t)covenant_timestamp, time_buf));

	LLPanelEstateCovenant::updateLastModified(last_modified);
	LLPanelLandCovenant::updateLastModified(last_modified);
	LLFloaterBuyLand::updateLastModified(last_modified);

	gCacheName->getName(estate_owner_id, callbackCacheEstateOwnerName);
	
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
			covenant_text = "There is no Covenant provided for this Estate.";
		}
		else
		{
			covenant_text = "There is no Covenant provided for this Estate. The land on this estate is being sold by the Estate owner, not Linden Lab.  Please contact the Estate Owner for sales details.";
		}
		LLPanelEstateCovenant::updateCovenantText(covenant_text, covenant_id);
		LLPanelLandCovenant::updateCovenantText(covenant_text);
		LLFloaterBuyLand::updateCovenantText(covenant_text, covenant_id);
	}
}

void callbackCacheEstateOwnerName(
		const LLUUID& id,
		const char* first,
		const char* last,
		BOOL is_group,
		void*)
{
	std::string name;
	
	if (id.isNull())
	{
		name = "(none)";
	}
	else
	{
		name = first;
		name += " ";
		name += last;
	}
	LLPanelEstateCovenant::updateEstateOwnerName(name);
	LLPanelLandCovenant::updateEstateOwnerName(name);
	LLFloaterBuyLand::updateEstateOwnerName(name);
}

void onCovenantLoadComplete(LLVFS *vfs,
					const LLUUID& asset_uuid,
					LLAssetType::EType type,
					void* user_data, S32 status)
{
	llinfos << "onCovenantLoadComplete()" << llendl;
	std::string covenant_text;
	if(0 == status)
	{
		LLVFile file(vfs, asset_uuid, type, LLVFile::READ);
		
		S32 file_length = file.getSize();
		
		char* buffer = new char[file_length+1];
		if (buffer == NULL)
		{
			llerrs << "Memory Allocation failed" << llendl;
			return;
		}

		file.read((U8*)buffer, file_length);		/* Flawfinder: ignore */
		
		// put a EOS at the end
		buffer[file_length] = 0;
		
		if( (file_length > 19) && !strncmp( buffer, "Linden text version", 19 ) )
		{
			LLViewerTextEditor* editor =
				new LLViewerTextEditor("temp",
						       LLRect(0,0,0,0),
						       file_length+1);
			if( !editor->importBuffer( buffer ) )
			{
				llwarns << "Problem importing estate covenant." << llendl;
				covenant_text = "Problem importing estate covenant.";
			}
			else
			{
				// Version 0 (just text, doesn't include version number)
				covenant_text = editor->getText();
			}
			delete[] buffer;
			delete editor;
		}
		else
		{
			llwarns << "Problem importing estate covenant: Covenant file format error." << llendl;
			covenant_text = "Problem importing estate covenant: Covenant file format error.";
		}
	}
	else
	{
		if( gViewerStats )
		{
			gViewerStats->incStat( LLViewerStats::ST_DOWNLOAD_FAILED );
		}
		
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
		
		llwarns << "Problem loading notecard: " << status << llendl;
	}
	LLPanelEstateCovenant::updateCovenantText(covenant_text, asset_uuid);
	LLPanelLandCovenant::updateCovenantText(covenant_text);
	LLFloaterBuyLand::updateCovenantText(covenant_text, asset_uuid);
}


void process_feature_disabled_message(LLMessageSystem* msg, void**)
{
	// Handle Blacklisted feature simulator response...
	LLUUID	agentID;
	LLUUID	transactionID;
	char	messageText[MAX_STRING];		/* Flawfinder: ignore */
	msg->getStringFast(_PREHASH_FailureInfo,_PREHASH_ErrorMessage,MAX_STRING,&messageText[0],0);
	msg->getUUIDFast(_PREHASH_FailureInfo,_PREHASH_AgentID,agentID);
	msg->getUUIDFast(_PREHASH_FailureInfo,_PREHASH_TransactionID,transactionID);
	
	llwarns << "Blacklisted Feature Response:" << &messageText[0] << llendl;
}

// ------------------------------------------------------------
// Message system exception callbacks
// ------------------------------------------------------------

void invalid_message_callback(LLMessageSystem* msg,
								   void*,
								   EMessageException exception)
{
	bad_network_handler();
}

// Please do not add more message handlers here. This file is huge.
// Put them in a file related to the functionality you are implementing. JC
