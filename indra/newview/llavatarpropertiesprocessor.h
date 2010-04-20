/** 
 * @file llavatarpropertiesprocessor.h
 * @brief LLAvatatIconCtrl base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLAVATARPROPERTIESPROCESSOR_H
#define LL_LLAVATARPROPERTIESPROCESSOR_H

#include "lluuid.h"
#include "llsingleton.h"
#include "v3dmath.h"	// LLVector3d
#include <list>
#include <map>

/*
*TODO Vadim: This needs some refactoring:
- Remove EAvatarProcessorType in favor of separate observers, derived from a common parent (to get rid of void*).
*/

class LLMessageSystem;

enum EAvatarProcessorType
{
	APT_PROPERTIES,
	APT_NOTES,
	APT_GROUPS,
	APT_PICKS,
	APT_PICK_INFO,
	APT_TEXTURES,
	APT_CLASSIFIEDS,
	APT_CLASSIFIED_INFO
};

struct LLAvatarData
{
	LLUUID 		agent_id;
	LLUUID		avatar_id; //target id
	LLUUID		image_id;
	LLUUID		fl_image_id;
	LLUUID		partner_id;
	std::string	about_text;
	std::string	fl_about_text;
	LLDate		born_on;
	std::string	profile_url;
	U8			caption_index;
	std::string	caption_text;
	U32			flags;
	BOOL		allow_publish;
};

struct LLAvatarPicks
{
	LLUUID agent_id;
	LLUUID target_id; //target id

	typedef std::pair<LLUUID,std::string> pick_data_t;
	typedef std::list< pick_data_t> picks_list_t;
	picks_list_t picks_list;
};

struct LLPickData
{
	LLUUID agent_id;
	LLUUID pick_id;
	LLUUID creator_id;
	BOOL top_pick;
	LLUUID parcel_id;
	std::string name;
	std::string desc;
	LLUUID snapshot_id;
	LLVector3d pos_global;
	S32 sort_order;
	BOOL enabled;

	//used only in read requests
	std::string user_name;
	std::string original_name;
	std::string sim_name;

	//used only in write (update) requests
	LLUUID session_id;

};

struct LLAvatarNotes
{
	LLUUID agent_id;
	LLUUID target_id; //target id
	std::string notes;
};

struct LLAvatarGroups
{
	LLUUID agent_id;
	LLUUID avatar_id; //target id
	BOOL list_in_profile;

	struct LLGroupData;
	typedef std::list<LLGroupData> group_list_t;

	group_list_t group_list;

	struct LLGroupData
	{
		U64 group_powers;
		BOOL accept_notices;
		std::string group_title;
		LLUUID group_id;
		std::string group_name;
		LLUUID group_insignia_id;
	};
};

struct LLAvatarClassifieds
{
	LLUUID agent_id;
	LLUUID target_id;

	struct classified_data;
	typedef std::list<classified_data> classifieds_list_t;

	classifieds_list_t classifieds_list;

	struct classified_data
	{
		LLUUID classified_id;
		std::string name;
	};
};

struct LLAvatarClassifiedInfo
{
	LLUUID agent_id;
	LLUUID classified_id;
	LLUUID creator_id;
	U32 creation_date;
	U32 expiration_date;
	U32 category;
	std::string name;
	std::string description;
	LLUUID parcel_id;
	U32 parent_estate;
	LLUUID snapshot_id;
	std::string sim_name;
	LLVector3d pos_global;
	std::string parcel_name;
	U8 flags;
	S32 price_for_listing;
};

class LLAvatarPropertiesObserver
{
public:
	virtual ~LLAvatarPropertiesObserver() {}
	virtual void processProperties(void* data, EAvatarProcessorType type) = 0;
};

class LLAvatarPropertiesProcessor
	: public LLSingleton<LLAvatarPropertiesProcessor>
{
public:
	
	LLAvatarPropertiesProcessor();
	virtual ~LLAvatarPropertiesProcessor();

	void addObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer);
	
	void removeObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer);

	// Request various types of avatar data.  Duplicate requests will be
	// suppressed while waiting for a response from the network.
	void sendAvatarPropertiesRequest(const LLUUID& avatar_id);
	void sendAvatarPicksRequest(const LLUUID& avatar_id);
	void sendAvatarNotesRequest(const LLUUID& avatar_id);
	void sendAvatarGroupsRequest(const LLUUID& avatar_id);
	void sendAvatarTexturesRequest(const LLUUID& avatar_id);
	void sendAvatarClassifiedsRequest(const LLUUID& avatar_id);

	// Duplicate pick info requests are not suppressed.
	void sendPickInfoRequest(const LLUUID& creator_id, const LLUUID& pick_id);

	void sendClassifiedInfoRequest(const LLUUID& classified_id);

	void sendAvatarPropertiesUpdate(const LLAvatarData* avatar_props);

	void sendPickInfoUpdate(const LLPickData* new_pick);

	void sendClassifiedInfoUpdate(const LLAvatarClassifiedInfo* c_data);

	void sendFriendRights(const LLUUID& avatar_id, S32 rights);

	void sendNotes(const LLUUID& avatar_id, const std::string notes);

	void sendPickDelete(const LLUUID& pick_id);

	void sendClassifiedDelete(const LLUUID& classified_id);

	// Returns translated, human readable string for account type, such
	// as "Resident" or "Linden Employee".  Used for profiles, inspectors.
	static std::string accountType(const LLAvatarData* avatar_data);

	// Returns translated, human readable string for payment info, such
	// as "Payment Info on File" or "Payment Info Used".
	// Used for profiles, inspectors.
	static std::string paymentInfo(const LLAvatarData* avatar_data);

	static void processAvatarPropertiesReply(LLMessageSystem* msg, void**);

	static void processAvatarInterestsReply(LLMessageSystem* msg, void**);

	static void processAvatarClassifiedsReply(LLMessageSystem* msg, void**);

	static void processClassifiedInfoReply(LLMessageSystem* msg, void**);

	static void processAvatarGroupsReply(LLMessageSystem* msg, void**);

	static void processAvatarNotesReply(LLMessageSystem* msg, void**);

	static void processAvatarPicksReply(LLMessageSystem* msg, void**);

	static void processPickInfoReply(LLMessageSystem* msg, void**);

protected:

	void sendGenericRequest(const LLUUID& avatar_id, EAvatarProcessorType type, const std::string method);

	void notifyObservers(const LLUUID& id,void* data, EAvatarProcessorType type);

	// Is there a pending, not timed out, request for this avatar's data?
	// Use this to suppress duplicate requests for data when a request is
	// pending.
	bool isPendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type);

	// Call this when a request has been sent
	void addPendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type);

	// Call this when the reply to the request is received
	void removePendingRequest(const LLUUID& avatar_id, EAvatarProcessorType type);

	typedef void* (*processor_method_t)(LLMessageSystem*);
	static processor_method_t getProcessor(EAvatarProcessorType type);

protected:
	
	typedef std::multimap<LLUUID, LLAvatarPropertiesObserver*> observer_multimap_t;
	
	observer_multimap_t mObservers;

	// Keep track of pending requests for data by avatar id and type.
	// Maintain a timestamp for each request so a request that receives no reply
	// does not block future requests forever.
	// Map avatar_id+request_type -> U32 timestamp in seconds
	typedef std::map< std::pair<LLUUID, EAvatarProcessorType>, U32> timestamp_map_t;
	timestamp_map_t mRequestTimestamps;
};

#endif  // LL_LLAVATARPROPERTIESPROCESSOR_H
