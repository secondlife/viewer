/** 
 * @file llmutelist.cpp
 * @author Richard Nelson, James Cook
 * @brief Management of list of muted players
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

/*
 * How should muting work?
 * Mute an avatar
 * Mute a specific object (accidentally spamming)
 *
 * right-click avatar, mute
 * see list of recent chatters, mute
 * type a name to mute?
 *
 * show in list whether chatter is avatar or object
 *
 * need fast lookup by id
 * need lookup by name, doesn't have to be fast
 */

#include "llviewerprecompiledheaders.h"

#include "llmutelist.h"

#include "pipeline.h"

#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "lldispatcher.h"
#include "llxfermanager.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llviewergenericmessage.h"	// for gGenericDispatcher
#include "llworld.h" //for particle system banning
#include "llimview.h"
#include "llnotifications.h"
#include "llviewercontrol.h"
#include "llviewerobjectlist.h"
#include "lltrans.h"

namespace 
{
	// This method is used to return an object to mute given an object id.
	// Its used by the LLMute constructor and LLMuteList::isMuted.
	LLViewerObject* get_object_to_mute_from_id(LLUUID object_id)
	{
		LLViewerObject *objectp = gObjectList.findObject(object_id);
		if ((objectp) && (!objectp->isAvatar()))
		{
			LLViewerObject *parentp = (LLViewerObject *)objectp->getParent();
			if (parentp && parentp->getID() != gAgent.getID())
			{
				objectp = parentp;
			}
		}
		return objectp;
	}
}

// "emptymutelist"
class LLDispatchEmptyMuteList : public LLDispatchHandler
{
public:
	virtual bool operator()(
		const LLDispatcher* dispatcher,
		const std::string& key,
		const LLUUID& invoice,
		const sparam_t& strings)
	{
		LLMuteList::getInstance()->setLoaded();
		return true;
	}
};

static LLDispatchEmptyMuteList sDispatchEmptyMuteList;

//-----------------------------------------------------------------------------
// LLMute()
//-----------------------------------------------------------------------------

LLMute::LLMute(const LLUUID& id, const std::string& name, EType type, U32 flags)
  : mID(id),
	mName(name),
	mType(type),
	mFlags(flags)
{
	// muting is done by root objects only - try to find this objects root
	LLViewerObject* mute_object = get_object_to_mute_from_id(id);
	if(mute_object && mute_object->getID() != id)
	{
		mID = mute_object->getID();
		LLNameValue* firstname = mute_object->getNVPair("FirstName");
		LLNameValue* lastname = mute_object->getNVPair("LastName");
		if (firstname && lastname)
		{
			mName = LLCacheName::buildFullName(
				firstname->getString(), lastname->getString());
		}
		mType = mute_object->isAvatar() ? AGENT : OBJECT;
	}

}


std::string LLMute::getDisplayType() const
{
	switch (mType)
	{
		case BY_NAME:
		default:
			return LLTrans::getString("MuteByName");
			break;
		case AGENT:
			return LLTrans::getString("MuteAgent");
			break;
		case OBJECT:
			return LLTrans::getString("MuteObject");
			break;
		case GROUP:
			return LLTrans::getString("MuteGroup");
			break;
		case EXTERNAL:
			return LLTrans::getString("MuteExternal");
			break;
	}
}

//-----------------------------------------------------------------------------
// LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::LLMuteList() :
	mIsLoaded(FALSE)
{
	gGenericDispatcher.addHandler("emptymutelist", &sDispatchEmptyMuteList);

	// Register our callbacks. We may be constructed before gMessageSystem, so
	// use callWhenReady() to register them as soon as gMessageSystem becomes
	// available.
	// When using bind(), must be explicit about default arguments such as
	// that last NULL.
	gMessageSystem.callWhenReady(boost::bind(&LLMessageSystem::setHandlerFuncFast, _1,
											 _PREHASH_MuteListUpdate, processMuteListUpdate,
											 static_cast<void**>(NULL)));
	gMessageSystem.callWhenReady(boost::bind(&LLMessageSystem::setHandlerFuncFast, _1,
											 _PREHASH_UseCachedMuteList, processUseCachedMuteList,
											 static_cast<void**>(NULL)));
}

//-----------------------------------------------------------------------------
// ~LLMuteList()
//-----------------------------------------------------------------------------
LLMuteList::~LLMuteList()
{

}

BOOL LLMuteList::isLinden(const std::string& name) const
{
	std::string username = boost::replace_all_copy(name, ".", " ");
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(username, sep);
	tokenizer::iterator token_iter = tokens.begin();
	
	if (token_iter == tokens.end()) return FALSE;
	token_iter++;
	if (token_iter == tokens.end()) return FALSE;
	
	std::string last_name = *token_iter;
	LLStringUtil::toLower(last_name);
	return last_name == "linden";
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
	LLViewerObject *obj = gObjectList.findObject(id);
	while (obj && obj->isAttachment())
	{
		obj = (LLViewerObject *)obj->getParent();
	}

	if (obj && obj->isAvatar())
	{
		return (LLVOAvatar*)obj;
	}
	else
	{
		return NULL;
	}
}

BOOL LLMuteList::add(const LLMute& mute, U32 flags)
{
	// Can't mute text from Lindens
	if ((mute.mType == LLMute::AGENT)
		&& isLinden(mute.mName) && (flags & LLMute::flagTextChat || flags == 0))
	{
        LL_WARNS() << "Trying to mute a Linden; ignored" << LL_ENDL;
		LLNotifications::instance().add("MuteLinden", LLSD(), LLSD());
		return FALSE;
	}
	
	// Can't mute self.
	if (mute.mType == LLMute::AGENT
		&& mute.mID == gAgent.getID())
	{
        LL_WARNS() << "Trying to self; ignored" << LL_ENDL;
		return FALSE;
	}
	
	S32 mute_list_limit = gSavedSettings.getS32("MuteListLimit");
	if (getMutes().size() >= mute_list_limit)
	{
		LL_WARNS() << "Mute limit is reached; ignored" << LL_ENDL;
		LLSD args;
		args["MUTE_LIMIT"] = mute_list_limit;
		LLNotifications::instance().add(LLNotification::Params("MuteLimitReached").substitutions(args));
		return FALSE;
	}

	if (mute.mType == LLMute::BY_NAME)
	{		
		// Can't mute empty string by name
		if (mute.mName.empty()) 
		{
			LL_WARNS() << "Trying to mute empty string by-name" << LL_ENDL;
			return FALSE;
		}

		// Null mutes must have uuid null
		if (mute.mID.notNull())
		{
			LL_WARNS() << "Trying to add by-name mute with non-null id" << LL_ENDL;
			return FALSE;
		}

		std::pair<string_set_t::iterator, bool> result = mLegacyMutes.insert(mute.mName);
		if (result.second)
		{
			LL_INFOS() << "Muting by name " << mute.mName << LL_ENDL;
			updateAdd(mute);
			notifyObservers();
			notifyObserversDetailed(mute);
			return TRUE;
		}
		else
		{
			LL_INFOS() << "duplicate mute ignored" << LL_ENDL;
			// was duplicate
			return FALSE;
		}
	}
	else
	{
		// Need a local (non-const) copy to set up flags properly.
		LLMute localmute = mute;
		
		// If an entry for the same entity is already in the list, remove it, saving flags as necessary.
		mute_set_t::iterator it = mMutes.find(localmute);
		if (it != mMutes.end())
		{
			// This mute is already in the list.  Save the existing entry's flags if that's warranted.
			localmute.mFlags = it->mFlags;
			
			mMutes.erase(it);
			// Don't need to call notifyObservers() here, since it will happen after the entry has been re-added below.
		}
		else
		{
			// There was no entry in the list previously.  Fake things up by making it look like the previous entry had all properties unmuted.
			localmute.mFlags = LLMute::flagAll;
		}

		if(flags)
		{
			// The user passed some combination of flags.  Make sure those flag bits are turned off (i.e. those properties will be muted).
			localmute.mFlags &= (~flags);
		}
		else
		{
			// The user passed 0.  Make sure all flag bits are turned off (i.e. all properties will be muted).
			localmute.mFlags = 0;
		}
		
		// (re)add the mute entry.
		{			
			std::pair<mute_set_t::iterator, bool> result = mMutes.insert(localmute);
			if (result.second)
			{
				LL_INFOS() << "Muting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
				updateAdd(localmute);
				notifyObservers();
				notifyObserversDetailed(localmute);
				if(!(localmute.mFlags & LLMute::flagParticles))
				{
					//Kill all particle systems owned by muted task
					if(localmute.mType == LLMute::AGENT || localmute.mType == LLMute::OBJECT)
					{
						LLViewerPartSim::getInstance()->clearParticlesByOwnerID(localmute.mID);
					}
				}
				//mute local lights that are attached to the avatar
				LLVOAvatar *avatarp = find_avatar(localmute.mID);
				if (avatarp)
				{
					LLPipeline::removeMutedAVsLights(avatarp);
				}
				//remove agent's notifications as well
				if (localmute.mType == LLMute::AGENT)
				{
					LLNotifications::instance().cancelByOwner(localmute.mID);
				}
				return TRUE;
			}
		}
	}
	
	// If we were going to return success, we'd have done it by now.
	return FALSE;
}

void LLMuteList::updateAdd(const LLMute& mute)
{
	// External mutes (e.g. Avaline callers) are local only, don't send them to the server.
	if (mute.mType == LLMute::EXTERNAL)
	{
		return;
	}

	// Update the database
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_UpdateMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addStringFast(_PREHASH_MuteName, mute.mName);
	msg->addS32("MuteType", mute.mType);
	msg->addU32("MuteFlags", mute.mFlags);
	gAgent.sendReliableMessage();

	mIsLoaded = TRUE; // why is this here? -MG
}


BOOL LLMuteList::remove(const LLMute& mute, U32 flags)
{
	BOOL found = FALSE;
	
	// First, remove from main list.
	mute_set_t::iterator it = mMutes.find(mute);
	if (it != mMutes.end())
	{
		LLMute localmute = *it;
		bool remove = true;
		if(flags)
		{
			// If the user passed mute flags, we may only want to turn some flags on.
			localmute.mFlags |= flags;
			
			if(localmute.mFlags == LLMute::flagAll)
			{
				// Every currently available mute property has been masked out.
				// Remove the mute entry entirely.
			}
			else
			{
				// Only some of the properties are masked out.  Update the entry.
				remove = false;
			}
		}
		else
		{
			// The caller didn't pass any flags -- just remove the mute entry entirely.
		}
		
		// Always remove the entry from the set -- it will be re-added with new flags if necessary.
		mMutes.erase(it);

		if(remove)
		{
			// The entry was actually removed.  Notify the server.
			updateRemove(localmute);
			LL_INFOS() << "Unmuting " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
		}
		else
		{
			// Flags were updated, the mute entry needs to be retransmitted to the server and re-added to the list.
			mMutes.insert(localmute);
			updateAdd(localmute);
			LL_INFOS() << "Updating mute entry " << localmute.mName << " id " << localmute.mID << " flags " << localmute.mFlags << LL_ENDL;
		}
		
		// Must be after erase.
		notifyObserversDetailed(localmute);
		setLoaded();  // why is this here? -MG
	}
	else
	{
		// Clean up any legacy mutes
		string_set_t::iterator legacy_it = mLegacyMutes.find(mute.mName);
		if (legacy_it != mLegacyMutes.end())
		{
			// Database representation of legacy mute is UUID null.
			LLMute mute(LLUUID::null, *legacy_it, LLMute::BY_NAME);
			updateRemove(mute);
			mLegacyMutes.erase(legacy_it);
			// Must be after erase.
			notifyObserversDetailed(mute);
			setLoaded(); // why is this here? -MG
		}
	}
	
	return found;
}


void LLMuteList::updateRemove(const LLMute& mute)
{
	// External mutes are not sent to the server anyway, no need to remove them.
	if (mute.mType == LLMute::EXTERNAL)
	{
		return;
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RemoveMuteListEntry);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addUUIDFast(_PREHASH_MuteID, mute.mID);
	msg->addString("MuteName", mute.mName);
	gAgent.sendReliableMessage();
}

void notify_automute_callback(const LLUUID& agent_id, const LLAvatarName& full_name, LLMuteList::EAutoReason reason)
{
	std::string notif_name;
	switch (reason)
	{
	default:
	case LLMuteList::AR_IM:
		notif_name = "AutoUnmuteByIM";
		break;
	case LLMuteList::AR_INVENTORY:
		notif_name = "AutoUnmuteByInventory";
		break;
	case LLMuteList::AR_MONEY:
		notif_name = "AutoUnmuteByMoney";
		break;
	}

	LLSD args;
	args["NAME"] = full_name.getUserName();
    
	LLNotificationPtr notif_ptr = LLNotifications::instance().add(notif_name, args, LLSD());
	if (notif_ptr)
	{
		std::string message = notif_ptr->getMessage();

		if (reason == LLMuteList::AR_IM)
		{
			LLIMModel::getInstance()->addMessage(agent_id, SYSTEM_FROM, LLUUID::null, message);
		}
	}
}


BOOL LLMuteList::autoRemove(const LLUUID& agent_id, const EAutoReason reason)
{
	BOOL removed = FALSE;

	if (isMuted(agent_id))
	{
		LLMute automute(agent_id, LLStringUtil::null, LLMute::AGENT);
		removed = TRUE;
		remove(automute);

		LLAvatarName av_name;
		if (LLAvatarNameCache::get(agent_id, &av_name))
		{
			// name in cache, call callback directly
			notify_automute_callback(agent_id, av_name, reason);
		}
		else
		{
			// not in cache, lookup name from cache
			LLAvatarNameCache::get(agent_id,
				boost::bind(&notify_automute_callback, _1, _2, reason));
		}
	}

	return removed;
}


std::vector<LLMute> LLMuteList::getMutes() const
{
	std::vector<LLMute> mutes;
	
	for (mute_set_t::const_iterator it = mMutes.begin();
		 it != mMutes.end();
		 ++it)
	{
		mutes.push_back(*it);
	}
	
	for (string_set_t::const_iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end();
		 ++it)
	{
		LLMute legacy(LLUUID::null, *it);
		mutes.push_back(legacy);
	}
	
	std::sort(mutes.begin(), mutes.end(), compare_by_name());
	return mutes;
}

//-----------------------------------------------------------------------------
// loadFromFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::loadFromFile(const std::string& filename)
{
	if(!filename.size())
	{
		LL_WARNS() << "Mute List Filename is Empty!" << LL_ENDL;
		return FALSE;
	}

	LLFILE* fp = LLFile::fopen(filename, "rb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		LL_WARNS() << "Couldn't open mute list " << filename << LL_ENDL;
		return FALSE;
	}

	// *NOTE: Changing the size of these buffers will require changes
	// in the scanf below.
	char id_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char name_buffer[MAX_STRING];		/*Flawfinder: ignore*/
	char buffer[MAX_STRING];		/*Flawfinder: ignore*/
	while (!feof(fp) 
		   && fgets(buffer, MAX_STRING, fp))
	{
		id_buffer[0] = '\0';
		name_buffer[0] = '\0';
		S32 type = 0;
		U32 flags = 0;
		sscanf(	/* Flawfinder: ignore */
			buffer, " %d %254s %254[^|]| %u\n", &type, id_buffer, name_buffer,
			&flags);
		LLUUID id = LLUUID(id_buffer);
		LLMute mute(id, std::string(name_buffer), (LLMute::EType)type, flags);
		if (mute.mID.isNull()
			|| mute.mType == LLMute::BY_NAME)
		{
			mLegacyMutes.insert(mute.mName);
		}
		else
		{
			mMutes.insert(mute);
		}
	}
	fclose(fp);
	setLoaded();
	return TRUE;
}

//-----------------------------------------------------------------------------
// saveToFile()
//-----------------------------------------------------------------------------
BOOL LLMuteList::saveToFile(const std::string& filename)
{
	if(!filename.size())
	{
		LL_WARNS() << "Mute List Filename is Empty!" << LL_ENDL;
		return FALSE;
	}

	LLFILE* fp = LLFile::fopen(filename, "wb");		/*Flawfinder: ignore*/
	if (!fp)
	{
		LL_WARNS() << "Couldn't open mute list " << filename << LL_ENDL;
		return FALSE;
	}
	// legacy mutes have null uuid
	std::string id_string;
	LLUUID::null.toString(id_string);
	for (string_set_t::iterator it = mLegacyMutes.begin();
		 it != mLegacyMutes.end();
		 ++it)
	{
		fprintf(fp, "%d %s %s|\n", (S32)LLMute::BY_NAME, id_string.c_str(), it->c_str());
	}
	for (mute_set_t::iterator it = mMutes.begin();
		 it != mMutes.end();
		 ++it)
	{
		// Don't save external mutes as they are not sent to the server and probably won't
		//be valid next time anyway.
		if (it->mType != LLMute::EXTERNAL)
		{
			it->mID.toString(id_string);
			const std::string& name = it->mName;
			fprintf(fp, "%d %s %s|%u\n", (S32)it->mType, id_string.c_str(), name.c_str(), it->mFlags);
		}
	}
	fclose(fp);
	return TRUE;
}


BOOL LLMuteList::isMuted(const LLUUID& id, const std::string& name, U32 flags) const
{
	// for objects, check for muting on their parent prim
	LLViewerObject* mute_object = get_object_to_mute_from_id(id);
	LLUUID id_to_check  = (mute_object) ? mute_object->getID() : id;

	// don't need name or type for lookup
	LLMute mute(id_to_check);
	mute_set_t::const_iterator mute_it = mMutes.find(mute);
	if (mute_it != mMutes.end())
	{
		// If any of the flags the caller passed are set, this item isn't considered muted for this caller.
		if(flags & mute_it->mFlags)
		{
			return FALSE;
		}
		return TRUE;
	}

	// empty names can't be legacy-muted
	bool avatar = mute_object && mute_object->isAvatar();
	if (name.empty() || avatar) return FALSE;

	// Look in legacy pile
	string_set_t::const_iterator legacy_it = mLegacyMutes.find(name);
	return legacy_it != mLegacyMutes.end();
}

BOOL LLMuteList::isMuted(const std::string& username, U32 flags) const
{
	mute_set_t::const_iterator mute_iter = mMutes.begin();
	while(mute_iter != mMutes.end())
	{
		// can't convert "leha.test" into "LeHa TesT" so username comparison is more reliable
		if (mute_iter->mType == LLMute::AGENT
			&& LLCacheName::buildUsername(mute_iter->mName) == username)
		{
			return TRUE;
		}
		mute_iter++;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// requestFromServer()
//-----------------------------------------------------------------------------
void LLMuteList::requestFromServer(const LLUUID& agent_id)
{
	std::string agent_id_string;
	std::string filename;
	agent_id.toString(agent_id_string);
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
	LLCRC crc;
	crc.update(filename);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_MuteListRequest);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, agent_id);
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_MuteData);
	msg->addU32Fast(_PREHASH_MuteCRC, crc.getCRC());
	gAgent.sendReliableMessage();
}

//-----------------------------------------------------------------------------
// cache()
//-----------------------------------------------------------------------------

void LLMuteList::cache(const LLUUID& agent_id)
{
	// Write to disk even if empty.
	if(mIsLoaded)
	{
		std::string agent_id_string;
		std::string filename;
		agent_id.toString(agent_id_string);
		filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
		saveToFile(filename);
	}
}

//-----------------------------------------------------------------------------
// Static message handlers
//-----------------------------------------------------------------------------

void LLMuteList::processMuteListUpdate(LLMessageSystem* msg, void**)
{
	LL_INFOS() << "LLMuteList::processMuteListUpdate()" << LL_ENDL;
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_MuteData, _PREHASH_AgentID, agent_id);
	if(agent_id != gAgent.getID())
	{
		LL_WARNS() << "Got an mute list update for the wrong agent." << LL_ENDL;
		return;
	}
	std::string unclean_filename;
	msg->getStringFast(_PREHASH_MuteData, _PREHASH_Filename, unclean_filename);
	std::string filename = LLDir::getScrubbedFileName(unclean_filename);
	
	std::string *local_filename_and_path = new std::string(gDirUtilp->getExpandedFilename( LL_PATH_CACHE, filename ));
	gXferManager->requestFile(*local_filename_and_path,
							  filename,
							  LL_PATH_CACHE,
							  msg->getSender(),
							  TRUE, // make the remote file temporary.
							  onFileMuteList,
							  (void**)local_filename_and_path,
							  LLXferManager::HIGH_PRIORITY);
}

void LLMuteList::processUseCachedMuteList(LLMessageSystem* msg, void**)
{
	LL_INFOS() << "LLMuteList::processUseCachedMuteList()" << LL_ENDL;

	std::string agent_id_string;
	gAgent.getID().toString(agent_id_string);
	std::string filename;
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,agent_id_string) + ".cached_mute";
	LLMuteList::getInstance()->loadFromFile(filename);
}

void LLMuteList::onFileMuteList(void** user_data, S32 error_code, LLExtStat ext_status)
{
	LL_INFOS() << "LLMuteList::processMuteListFile()" << LL_ENDL;

	std::string* local_filename_and_path = (std::string*)user_data;
	if(local_filename_and_path && !local_filename_and_path->empty() && (error_code == 0))
	{
		LLMuteList::getInstance()->loadFromFile(*local_filename_and_path);
		LLFile::remove(*local_filename_and_path);
	}
	delete local_filename_and_path;
}

void LLMuteList::addObserver(LLMuteListObserver* observer)
{
	mObservers.insert(observer);
}

void LLMuteList::removeObserver(LLMuteListObserver* observer)
{
	mObservers.erase(observer);
}

void LLMuteList::setLoaded()
{
	mIsLoaded = TRUE;
	notifyObservers();
}

void LLMuteList::notifyObservers()
{
	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLMuteListObserver* observer = *it;
		observer->onChange();
		// In case onChange() deleted an entry.
		it = mObservers.upper_bound(observer);
	}
}

void LLMuteList::notifyObserversDetailed(const LLMute& mute)
{
	for (observer_set_t::iterator it = mObservers.begin();
		it != mObservers.end();
		)
	{
		LLMuteListObserver* observer = *it;
		observer->onChangeDetailed(mute);
		// In case onChange() deleted an entry.
		it = mObservers.upper_bound(observer);
	}
}

LLRenderMuteList::LLRenderMuteList()
{}

bool LLRenderMuteList::saveToFile()
{
    std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "render_mute_settings.txt");
    LLFILE* fp = LLFile::fopen(filename, "wb");
    if (!fp)
    {
        LL_WARNS() << "Couldn't open render mute list file: " << filename << LL_ENDL;
        return false;
    }

    for (std::map<LLUUID, S32>::iterator it = sVisuallyMuteSettingsMap.begin(); it != sVisuallyMuteSettingsMap.end(); ++it)
    {
        if (it->second != 0)
        {
            std::string id_string;
            it->first.toString(id_string);
            fprintf(fp, "%d %s [%d]\n", (S32)it->second, id_string.c_str(), (S32)sVisuallyMuteDateMap[it->first]);
        }
    }
    fclose(fp);
    return true;
}

bool LLRenderMuteList::loadFromFile()
{
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "render_mute_settings.txt");
	LLFILE* fp = LLFile::fopen(filename, "rb");
	if (!fp)
	{
		LL_WARNS() << "Couldn't open render mute list file: " << filename << LL_ENDL;
		return false;
	}

	char id_buffer[MAX_STRING];
	char buffer[MAX_STRING];
	while (!feof(fp) && fgets(buffer, MAX_STRING, fp))
	{
		id_buffer[0] = '\0';
		S32 setting = 0;
		S32 time = 0;
		sscanf(buffer, " %d %254s [%d]\n", &setting, id_buffer, &time);
		sVisuallyMuteSettingsMap[LLUUID(id_buffer)] = setting;
		sVisuallyMuteDateMap[LLUUID(id_buffer)] = (time == 0) ? (S32)time_corrected() : time;
	}
	fclose(fp);
    return true;
}

void LLRenderMuteList::saveVisualMuteSetting(const LLUUID& agent_id, S32 setting)
{
    if(setting == 0)
    {
        sVisuallyMuteSettingsMap.erase(agent_id);
        sVisuallyMuteDateMap.erase(agent_id);
    }
    else
    {
        sVisuallyMuteSettingsMap[agent_id] = setting;
        if (sVisuallyMuteDateMap.find(agent_id) == sVisuallyMuteDateMap.end())
        {
            sVisuallyMuteDateMap[agent_id] =  (S32)time_corrected();
        }
    }
    saveToFile();
    notifyObservers();
}

S32 LLRenderMuteList::getSavedVisualMuteSetting(const LLUUID& agent_id)
{
    std::map<LLUUID, S32>::iterator iter = sVisuallyMuteSettingsMap.find(agent_id);
    if (iter != sVisuallyMuteSettingsMap.end())
    {
        return iter->second;
    }

    return 0;
}

S32 LLRenderMuteList::getVisualMuteDate(const LLUUID& agent_id)
{
    std::map<LLUUID, S32>::iterator iter = sVisuallyMuteDateMap.find(agent_id);
    if (iter != sVisuallyMuteDateMap.end())
    {
        return iter->second;
    }

    return 0;
}

void LLRenderMuteList::addObserver(LLMuteListObserver* observer)
{
    mObservers.insert(observer);
}

void LLRenderMuteList::removeObserver(LLMuteListObserver* observer)
{
    mObservers.erase(observer);
}

void LLRenderMuteList::notifyObservers()
{
    for (observer_set_t::iterator it = mObservers.begin();
        it != mObservers.end();
        )
    {
        LLMuteListObserver* observer = *it;
        observer->onChange();
        // In case onChange() deleted an entry.
        it = mObservers.upper_bound(observer);
    }
}
