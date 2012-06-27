/** 
 * @file llviewermessage.h
 * @brief Message system callbacks for viewer.
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

#ifndef LL_LLVIEWERMESSAGE_H
#define LL_LLVIEWERMESSAGE_H

#include "llassettype.h"
#include "llinstantmessage.h"
#include "llpointer.h"
#include "lltransactiontypes.h"
#include "lluuid.h"
#include "message.h"
#include "stdenums.h"
#include "llnotifications.h"
#include "llextendedstatus.h"

#include <boost/function.hpp>
#include <boost/signals2.hpp>

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

void process_mean_collision_alert_message(LLMessageSystem* msg, void**);

void process_frozen_message(LLMessageSystem* msg, void**);

void process_derez_container(LLMessageSystem *msg, void**);
void container_inventory_arrived(LLViewerObject* object,
								 std::list<LLPointer<LLInventoryObject> >* inventory, //LLInventoryObject::object_list_t
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
void handle_lure(const uuid_vec_t& ids);

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
void open_inventory_offer(const uuid_vec_t& items, const std::string& from_name);

// Returns true if item is not in certain "quiet" folder which don't need UI
// notification (e.g. trash, cof, lost-and-found) and agent is not AFK, false otherwise.
// Returns false if item is not found.
bool highlight_offered_object(const LLUUID& obj_id);

void set_dad_inventory_item(LLInventoryItem* inv_item, const LLUUID& into_folder_uuid);
void set_dad_inbox_object(const LLUUID& object_id);

class LLViewerMessage : public  LLSingleton<LLViewerMessage>
{
public:
	typedef boost::function<void()> teleport_started_callback_t;
	typedef boost::signals2::signal<void()> teleport_started_signal_t;
	boost::signals2::connection setTeleportStartedCallback(teleport_started_callback_t cb);

	teleport_started_signal_t	mTeleportStartedSignal;
};

class LLOfferInfo : public LLNotificationResponderInterface
{
public:
	LLOfferInfo();
	LLOfferInfo(const LLSD& sd);

	LLOfferInfo(const LLOfferInfo& info);

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
	bool mPersist;

	// LLNotificationResponderInterface implementation
	/*virtual*/ LLSD asLLSD();
	/*virtual*/ void fromLLSD(const LLSD& params);
	/*virtual*/ void handleRespond(const LLSD& notification, const LLSD& response);

	void send_auto_receive_response(void);

	// TODO - replace all references with handleRespond()
	bool inventory_offer_callback(const LLSD& notification, const LLSD& response);
	bool inventory_task_offer_callback(const LLSD& notification, const LLSD& response);

private:

	void initRespondFunctionMap();

	typedef boost::function<bool (const LLSD&, const LLSD&)> respond_function_t;
	typedef std::map<std::string, respond_function_t> respond_function_map_t;

	respond_function_map_t mRespondFunctions;
};

void process_feature_disabled_message(LLMessageSystem* msg, void**);

#endif
