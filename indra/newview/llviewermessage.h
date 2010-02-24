/** 
 * @file llviewermessage.h
 * @brief Message system callbacks for viewer.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLVIEWERMESSAGE_H
#define LL_LLVIEWERMESSAGE_H

#include "llassettype.h"
#include "llinstantmessage.h"
#include "llpointer.h"
#include "lltransactiontypes.h"
#include "lluuid.h"
#include "message.h"
#include "stdenums.h"

//
// Forward declarations
//
class LLColor4;
class LLInventoryObject;
class LLInventoryItem;
class LLMeanCollisionData;
class LLMessageSystem;
class LLVFS;
class LLViewerObject;
class LLViewerRegion;

//
// Prototypes
//

enum InventoryOfferResponse
{
	IOR_ACCEPT,
	IOR_DECLINE,
	IOR_MUTE,
	IOR_BUSY,
	IOR_SHOW
};

BOOL can_afford_transaction(S32 cost);
void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group = FALSE,
				S32 trx_type = TRANS_GIFT, const std::string& desc = LLStringUtil::null);
void busy_message (LLMessageSystem* msg, LLUUID from_id);

void process_logout_reply(LLMessageSystem* msg, void**);
void process_layer_data(LLMessageSystem *mesgsys, void **user_data);
void process_derez_ack(LLMessageSystem*, void**);
void process_places_reply(LLMessageSystem* msg, void** data);
void send_sound_trigger(const LLUUID& sound_id, F32 gain);
void process_improved_im(LLMessageSystem *msg, void **user_data);
void process_script_question(LLMessageSystem *msg, void **user_data);
void process_chat_from_simulator(LLMessageSystem *mesgsys, void **user_data);

//void process_agent_to_new_region(LLMessageSystem *mesgsys, void **user_data);
void send_agent_update(BOOL force_send, BOOL send_reliable = FALSE);
void process_object_update(LLMessageSystem *mesgsys, void **user_data);
void process_compressed_object_update(LLMessageSystem *mesgsys, void **user_data);
void process_cached_object_update(LLMessageSystem *mesgsys, void **user_data);
void process_terse_object_update_improved(LLMessageSystem *mesgsys, void **user_data);

void send_simulator_throttle_settings(const LLHost &host);
void process_kill_object(	LLMessageSystem *mesgsys, void **user_data);
void process_time_synch(	LLMessageSystem *mesgsys, void **user_data);
void process_sound_trigger(LLMessageSystem *mesgsys, void **user_data);
void process_preload_sound(	LLMessageSystem *mesgsys, void **user_data);
void process_attached_sound(	LLMessageSystem *mesgsys, void **user_data);
void process_attached_sound_gain_change(	LLMessageSystem *mesgsys, void **user_data);
void process_energy_statistics(LLMessageSystem *mesgsys, void **user_data);
void process_health_message(LLMessageSystem *mesgsys, void **user_data);
void process_sim_stats(LLMessageSystem *mesgsys, void **user_data);
void process_shooter_agent_hit(LLMessageSystem* msg, void** user_data);
void process_avatar_animation(LLMessageSystem *mesgsys, void **user_data);
void process_avatar_appearance(LLMessageSystem *mesgsys, void **user_data);
void process_camera_constraint(LLMessageSystem *mesgsys, void **user_data);
void process_avatar_sit_response(LLMessageSystem *mesgsys, void **user_data);
void process_set_follow_cam_properties(LLMessageSystem *mesgsys, void **user_data);
void process_clear_follow_cam_properties(LLMessageSystem *mesgsys, void **user_data);
void process_name_value(LLMessageSystem *mesgsys, void **user_data);
void process_remove_name_value(LLMessageSystem *mesgsys, void **user_data);
void process_kick_user(LLMessageSystem *msg, void** /*user_data*/);
//void process_avatar_init_complete(LLMessageSystem *msg, void** /*user_data*/);

void process_economy_data(LLMessageSystem *msg, void** /*user_data*/);
void process_money_balance_reply(LLMessageSystem* msg_system, void**);
void process_adjust_balance(LLMessageSystem* msg_system, void**);

bool attempt_standard_notification(LLMessageSystem* msg);
void process_alert_message(LLMessageSystem* msg, void**);
void process_agent_alert_message(LLMessageSystem* msgsystem, void** user_data);
void process_alert_core(const std::string& message, BOOL modal);

// "Mean" or player-vs-player abuse
typedef std::list<LLMeanCollisionData*> mean_collision_list_t;
extern mean_collision_list_t gMeanCollisionList;

void handle_show_mean_events(void *);
void process_mean_collision_alert_message(LLMessageSystem* msg, void**);

void process_frozen_message(LLMessageSystem* msg, void**);

void process_derez_container(LLMessageSystem *msg, void**);
void container_inventory_arrived(LLViewerObject* object,
								 std::list<LLPointer<LLInventoryObject> >* inventory, //InventoryObjectList
								 S32 serial_num,
								 void* data);

// agent movement
void send_complete_agent_movement(const LLHost& sim_host);
void process_agent_movement_complete(LLMessageSystem* msg, void**);
void process_crossed_region(LLMessageSystem* msg, void**);
void process_teleport_start(LLMessageSystem* msg, void**);
void process_teleport_progress(LLMessageSystem* msg, void**);
void process_teleport_failed(LLMessageSystem *msg,void**);
void process_teleport_finish(LLMessageSystem *msg, void**);

//void process_user_sim_location_reply(LLMessageSystem *msg,void**);
void process_teleport_local(LLMessageSystem *msg,void**);
void process_user_sim_location_reply(LLMessageSystem *msg,void**);

void send_simple_im(const LLUUID& to_id,
					const std::string& message,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					const LLUUID& id = LLUUID::null);

void send_group_notice(const LLUUID& group_id,
					   const std::string& subject,
					   const std::string& message,
					   const LLInventoryItem* item);

void handle_lure(const LLUUID& invitee);
void handle_lure(const std::vector<LLUUID>& ids);

// always from gAgent and 
// routes through the gAgent's current simulator
void send_improved_im(const LLUUID& to_id,
					const std::string& name,
					const std::string& message,
					U8 offline = IM_ONLINE,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					const LLUUID& id = LLUUID::null,
					U32 timestamp = NO_TIMESTAMP, 
					const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET,
					S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);

void process_user_info_reply(LLMessageSystem* msg, void**);

// method to format the time. 
std::string formatted_time(const time_t& the_time);

void send_places_query(const LLUUID& query_id,
					   const LLUUID& trans_id,
					   const std::string& query_text,
					   U32 query_flags,
					   S32 category, 
					   const std::string& sim_name);
void process_script_dialog(LLMessageSystem* msg, void**);
void process_load_url(LLMessageSystem* msg, void**);
void process_script_teleport_request(LLMessageSystem* msg, void**);
void process_covenant_reply(LLMessageSystem* msg, void**);
void onCovenantLoadComplete(LLVFS *vfs,
							const LLUUID& asset_uuid,
							LLAssetType::EType type,
							void* user_data, S32 status, LLExtStat ext_status);

// calling cards
void process_offer_callingcard(LLMessageSystem* msg, void**);
void process_accept_callingcard(LLMessageSystem* msg, void**);
void process_decline_callingcard(LLMessageSystem* msg, void**);

// Message system exception prototypes
void invalid_message_callback(LLMessageSystem*, void*, EMessageException);

void process_initiate_download(LLMessageSystem* msg, void**);
void start_new_inventory_observer();
void open_inventory_offer(const std::vector<LLUUID>& items, const std::string& from_name);

// Returns true if item is not in certain "quiet" folder which don't need UI
// notification (e.g. trash, cof, lost-and-found) and agent is not AFK, false otherwise.
// Returns false if item is not found.
bool highlight_offered_item(const LLUUID& item_id);

struct LLOfferInfo
{
        LLOfferInfo()
	:	mFromGroup(FALSE), mFromObject(FALSE),
		mIM(IM_NOTHING_SPECIAL), mType(LLAssetType::AT_NONE) {};
	LLOfferInfo(const LLSD& sd);

	void forceResponse(InventoryOfferResponse response);

	EInstantMessage mIM;
	LLUUID mFromID;
	BOOL mFromGroup;
	BOOL mFromObject;
	LLUUID mTransactionID;
	LLUUID mFolderID;
	LLUUID mObjectID;
	LLAssetType::EType mType;
	std::string mFromName;
	std::string mDesc;
	LLHost mHost;

	LLSD asLLSD();
	void send_auto_receive_response(void);
	bool inventory_offer_callback(const LLSD& notification, const LLSD& response);
	bool inventory_task_offer_callback(const LLSD& notification, const LLSD& response);

};

void process_feature_disabled_message(LLMessageSystem* msg, void**);

#endif


