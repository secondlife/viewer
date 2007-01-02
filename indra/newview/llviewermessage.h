/** 
 * @file llviewermessage.h
 * @brief Message system callbacks for viewer.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERMESSAGE_H
#define LL_LLVIEWERMESSAGE_H

//#include "linked_lists.h"
#include "llinstantmessage.h"
#include "lltransactiontypes.h"
#include "lluuid.h"
#include "stdenums.h"
#include "message.h"

//
// Forward declarations
//
class LLColor4;
class LLViewerObject;
class LLInventoryObject;
class LLInventoryItem;
class LLViewerRegion;

//
// Prototypes
//

BOOL can_afford_transaction(S32 cost);
void give_money(const LLUUID& uuid, LLViewerRegion* region, S32 amount, BOOL is_group = FALSE,
				S32 trx_type = TRANS_GIFT, const LLString& desc = LLString::null);
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
//void process_attached_sound_cutoff_radius(	LLMessageSystem *mesgsys, void **user_data);
void process_energy_statistics(LLMessageSystem *mesgsys, void **user_data);
void process_health_message(LLMessageSystem *mesgsys, void **user_data);
void process_sim_stats(LLMessageSystem *mesgsys, void **user_data);
void process_shooter_agent_hit(LLMessageSystem* msg, void** user_data);
void process_avatar_info_request(LLMessageSystem *mesgsys, void **user_data);
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

void process_alert_message(LLMessageSystem* msg, void**);
void process_agent_alert_message(LLMessageSystem* msgsystem, void** user_data);
void process_alert_core(const char* buffer, BOOL modal);

// "Mean" or player-vs-player abuse
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
					const char* message,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					const LLUUID& id = LLUUID::null);

void send_group_notice(const LLUUID& group_id,
					   const char* subject,
					   const char* message,
					   const LLInventoryItem* item);

void handle_lure(const LLUUID& invitee);
void handle_lure(LLDynamicArray<LLUUID>& ids);

// always from gAgent and 
// routes through the gAgent's current simulator
void send_improved_im(const LLUUID& to_id,
					const char* name,
					const char* message,
					U8 offline = IM_ONLINE,
					EInstantMessage dialog = IM_NOTHING_SPECIAL,
					const LLUUID& id = LLUUID::null,
					U32 timestamp = NO_TIMESTAMP, 
					const U8* binary_bucket = (U8*)EMPTY_BINARY_BUCKET,
					S32 binary_bucket_size = EMPTY_BINARY_BUCKET_SIZE);

void process_user_info_reply(LLMessageSystem* msg, void**);

// method to format the time. Buffer should be at least
// TIME_STR_LENGTH long, and the function reutnrs buffer (for use in
// sprintf and the like)
const S32 TIME_STR_LENGTH = 30;
char* formatted_time(const time_t& the_time, char* buffer);

void send_places_query(const LLUUID& query_id,
					   const LLUUID& trans_id,
					   const char* query_text,
					   U32 query_flags,
					   S32 category, 
					   const char* sim_name);
void process_script_dialog(LLMessageSystem* msg, void**);
void process_load_url(LLMessageSystem* msg, void**);
void process_script_teleport_request(LLMessageSystem* msg, void**);
void process_covenant_reply(LLMessageSystem* msg, void**);
void onCovenantLoadComplete(LLVFS *vfs,
							const LLUUID& asset_uuid,
							LLAssetType::EType type,
							void* user_data, S32 status);
void callbackCacheEstateOwnerName(
		const LLUUID& id,
		const char* first,
		const char* last,
		BOOL is_group,
		void*);

// calling cards
void process_offer_callingcard(LLMessageSystem* msg, void**);
void process_accept_callingcard(LLMessageSystem* msg, void**);
void process_decline_callingcard(LLMessageSystem* msg, void**);

// Message system exception prototypes
void invalid_message_callback(LLMessageSystem*, void*, EMessageException);

void send_lure_911(void** user_data, S32 result);

void process_initiate_download(LLMessageSystem* msg, void**);
void inventory_offer_callback(S32 option, void* user_data);

struct LLOfferInfo
{
	EInstantMessage mIM;
	LLUUID mFromID;
	BOOL mFromGroup;
	BOOL mFromObject;
	LLUUID mTransactionID;
	LLUUID mFolderID;
	LLUUID mObjectID;
	LLAssetType::EType mType;
	LLString mFromName;
	LLString mDesc;
	LLHost mHost;
};

void send_generic_message(const char* method,
						  const std::vector<std::string>& strings,
						  const LLUUID& invoice = LLUUID::null);

void process_generic_message(LLMessageSystem* msg, void**);

void process_feature_disabled_message(LLMessageSystem* msg, void**);

extern LLDispatcher gGenericDispatcher;

#endif

