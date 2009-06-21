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
#include <map>

/*
*TODO Vadim: This needs some refactoring:
- Remove EAvatarProcessorType in favor of separate observers, derived from a common parent (to get rid of void*).
*/

/*
*TODO: mantipov: get rid of sendDataRequest and sendDataUpdate methods. Use exact methods instead of.
*/

class LLMessageSystem;

enum EAvatarProcessorType
{
	APT_PROPERTIES,
	APT_NOTES,
	APT_GROUPS,
	APT_PICKS,
	APT_PICK_INFO
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
	std::string	born_on;
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
	std::string location_text;
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
	virtual ~LLAvatarPropertiesProcessor()
	{}

	void addObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer);
	
	void removeObserver(const LLUUID& avatar_id, LLAvatarPropertiesObserver* observer);
	
	void sendDataRequest(const LLUUID& avatar_id, EAvatarProcessorType type, const void * data = NULL);

	void sendDataUpdate(const void* data, EAvatarProcessorType type);

	void sendFriendRights(const LLUUID& avatar_id, S32 rights);

	void sendNotes(const LLUUID& avatar_id, const std::string notes);

	void sendPickDelete(const LLUUID& pick_id);

	static void processAvatarPropertiesReply(LLMessageSystem* msg, void**);

	static void processAvatarInterestsReply(LLMessageSystem* msg, void**);

	static void processAvatarClassifiedReply(LLMessageSystem* msg, void**);

	static void processAvatarGroupsReply(LLMessageSystem* msg, void**);

	static void processAvatarNotesReply(LLMessageSystem* msg, void**);

	static void processAvatarPicksReply(LLMessageSystem* msg, void**);

	static void processPickInfoReply(LLMessageSystem* msg, void**);
protected:

	void sendAvatarPropertiesRequest(const LLUUID& avatar_id);

	void sendGenericRequest(const LLUUID& avatar_id, const std::string method);
	
	void sendAvatarPropertiesUpdate(const void* data);

	void sendPickInfoRequest(const LLUUID& creator_id, const LLUUID& pick_id);
	
	void sendPicInfoUpdate(const void * pick_data);

	static void notifyObservers(const LLUUID& id,void* data, EAvatarProcessorType type);

	typedef void* (*processor_method_t)(LLMessageSystem*);
	static processor_method_t getProcessor(EAvatarProcessorType type);

protected:
	
	typedef std::multimap<LLUUID, LLAvatarPropertiesObserver*> observer_multimap_t;
	
	observer_multimap_t mObservers;
};

#endif  // LL_LLAVATARPROPERTIESPROCESSOR_H
