/** 
 * @file llspeakingindicatormanager.cpp
 * @author Mike Antipov
 * @brief Implementation of SpeackerIndicatorManager class to process registered LLSpeackerIndicator
 * depend on avatars are in the same voice channel.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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
#include "llspeakingindicatormanager.h"


#include "llvoicechannel.h"
#include "llvoiceclient.h"

/**
 * This class intended to control visibility of avatar speaking indicators depend on whether avatars
 * are in the same voice channel.
 *
 * Speaking indicator should be visible for avatars in the same voice channel. See EXT-3976.
 *
 * It stores passed instances of LLOutputMonitorCtrl in a multimap by avatar LLUUID.
 * It observes changing of voice channel and changing of participant list in voice channel.
 * When voice channel or voice participant list is changed it updates visibility of an appropriate 
 * speaking indicator.
 *
 * Several indicators can be registered for the same avatar.
 */
class SpeakingIndicatorManager : public LLSingleton<SpeakingIndicatorManager>, LLVoiceClientParticipantObserver
{
	LOG_CLASS(SpeakingIndicatorManager);
public:

	/**
	 * Stores passed speaking indicator to control its visibility.
	 *
	 * Registered indicator is set visible if an appropriate avatar is in the same voice channel with Agent.
	 * It ignores instances of Agent's indicator.
	 *
	 * @param speaker_id LLUUID of an avatar whose speaking indicator is registered.
	 * @param speaking_indicator instance of the speaking indicator to be registered.
	 * @param session_id session UUID for which indicator should be shown only.
	 *		If this parameter is set registered indicator will be shown only in voice channel
	 *		which has the same session id (EXT-5562).
	 */
	void registerSpeakingIndicator(const LLUUID& speaker_id, LLSpeakingIndicator* const speaking_indicator,
		const LLUUID& session_id = LLUUID::null);

	/**
	 * Removes passed speaking indicator from observing.
	 *
	 * @param speaker_id LLUUID of an avatar whose speaking indicator should be unregistered.
	 * @param speaking_indicator instance of the speaking indicator to be unregistered.
	 */
	void unregisterSpeakingIndicator(const LLUUID& speaker_id, const LLSpeakingIndicator* const speaking_indicator);

private:
	typedef std::set<LLUUID> speaker_ids_t;
	typedef std::multimap<LLUUID, LLSpeakingIndicator*> speaking_indicators_mmap_t;
	typedef speaking_indicators_mmap_t::value_type speaking_indicator_value_t;
	typedef speaking_indicators_mmap_t::const_iterator indicator_const_iterator;
	typedef std::pair<indicator_const_iterator, indicator_const_iterator> indicator_range_t;

	friend class LLSingleton<SpeakingIndicatorManager>;
	SpeakingIndicatorManager();
	~SpeakingIndicatorManager();

	/**
	 * Callback to determine when voice channel is changed.
	 *
	 * It switches all registered speaking indicators off.
	 * To reduce overheads only switched on indicators are processed.
	 */
	void sOnCurrentChannelChanged(const LLUUID& session_id);

	/**
	 * Callback of changing voice participant list (from LLVoiceClientParticipantObserver).
	 *
	 * Switches off indicators had been switched on and switches on indicators of current participants list.
	 * There is only a few indicators in lists should be switched off/on.
	 * So, method does not calculate difference between these list it only switches off already 
	 * switched on indicators and switches on indicators of voice channel participants
	 */
	void onParticipantsChanged();

	/**
	 * Changes state of indicators specified by LLUUIDs
	 *
	 * @param speakers_uuids - avatars' LLUUIDs whose speaking indicators should be switched
	 * @param switch_on - if TRUE specified indicator will be switched on, off otherwise.
	 */
	void switchSpeakerIndicators(const speaker_ids_t& speakers_uuids, BOOL switch_on);

	/**
	 * Ensures that passed instance of Speaking Indicator does not exist among registered ones.
	 * If yes, it will be removed.
	 */
	void ensureInstanceDoesNotExist(LLSpeakingIndicator* const speaking_indicator);


	/**
	 * Multimap with all registered speaking indicators
	 */
	speaking_indicators_mmap_t mSpeakingIndicators;

	/**
	 * LUUIDs of avatar for which we have speaking indicators switched on.
	 *
	 * Is used to switch off all previously ON indicators when voice participant list is changed.
	 *
	 * @see onChange()
	 */
	speaker_ids_t mSwitchedIndicatorsOn;
};

//////////////////////////////////////////////////////////////////////////
// PUBLIC SECTION
//////////////////////////////////////////////////////////////////////////
void SpeakingIndicatorManager::registerSpeakingIndicator(const LLUUID& speaker_id, LLSpeakingIndicator* const speaking_indicator,
														 const LLUUID& session_id)
{
	// do not exclude agent's indicators. They should be processed in the same way as others. See EXT-3889.

	LL_DEBUGS("SpeakingIndicator") << "Registering indicator: " << speaker_id << "|"<< speaking_indicator << ", session: " << session_id << LL_ENDL;


	ensureInstanceDoesNotExist(speaking_indicator);

	speaking_indicator->setTargetSessionID(session_id);

	speaking_indicator_value_t value_type(speaker_id, speaking_indicator);
	mSpeakingIndicators.insert(value_type);

	speaker_ids_t speakers_uuids;
	BOOL is_in_same_voice = LLVoiceClient::getInstance()->isParticipant(speaker_id);

	speakers_uuids.insert(speaker_id);
	switchSpeakerIndicators(speakers_uuids, is_in_same_voice);
}

void SpeakingIndicatorManager::unregisterSpeakingIndicator(const LLUUID& speaker_id, const LLSpeakingIndicator* const speaking_indicator)
{
	LL_DEBUGS("SpeakingIndicator") << "Unregistering indicator: " << speaker_id << "|"<< speaking_indicator << LL_ENDL;
	speaking_indicators_mmap_t::iterator it;
	it = mSpeakingIndicators.find(speaker_id);
	for (;it != mSpeakingIndicators.end(); ++it)
	{
		if (it->second == speaking_indicator)
		{
			LL_DEBUGS("SpeakingIndicator") << "Unregistered." << LL_ENDL;
			mSpeakingIndicators.erase(it);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE SECTION
//////////////////////////////////////////////////////////////////////////
SpeakingIndicatorManager::SpeakingIndicatorManager()
{
	LLVoiceChannel::setCurrentVoiceChannelChangedCallback(boost::bind(&SpeakingIndicatorManager::sOnCurrentChannelChanged, this, _1));
	LLVoiceClient::getInstance()->addObserver(this);
}

SpeakingIndicatorManager::~SpeakingIndicatorManager()
{
	// Don't use LLVoiceClient::getInstance() here without check
	// singleton MAY have already been destroyed.
	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}
}

void SpeakingIndicatorManager::sOnCurrentChannelChanged(const LLUUID& /*session_id*/)
{
	switchSpeakerIndicators(mSwitchedIndicatorsOn, FALSE);
	mSwitchedIndicatorsOn.clear();
}

void SpeakingIndicatorManager::onParticipantsChanged()
{
	LL_DEBUGS("SpeakingIndicator") << "Voice participant list was changed, updating indicators" << LL_ENDL;

	speaker_ids_t speakers_uuids;
	LLVoiceClient::getInstance()->getParticipantList(speakers_uuids);

	LL_DEBUGS("SpeakingIndicator") << "Switching all OFF, count: " << mSwitchedIndicatorsOn.size() << LL_ENDL;
	// switch all indicators off
	switchSpeakerIndicators(mSwitchedIndicatorsOn, FALSE);
	mSwitchedIndicatorsOn.clear();

	LL_DEBUGS("SpeakingIndicator") << "Switching all ON, count: " << speakers_uuids.size() << LL_ENDL;
	// then switch current voice participants indicators on
	switchSpeakerIndicators(speakers_uuids, TRUE);
}

void SpeakingIndicatorManager::switchSpeakerIndicators(const speaker_ids_t& speakers_uuids, BOOL switch_on)
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	LLUUID session_id;
	if (voice_channel)
	{
		session_id = voice_channel->getSessionID();
	}

	speaker_ids_t::const_iterator it_uuid = speakers_uuids.begin(); 
	for (; it_uuid != speakers_uuids.end(); ++it_uuid)
	{
		LL_DEBUGS("SpeakingIndicator") << "Looking for indicator: " << *it_uuid << LL_ENDL;
		indicator_range_t it_range = mSpeakingIndicators.equal_range(*it_uuid);
		indicator_const_iterator it_indicator = it_range.first;
		bool was_found = false;
		bool was_switched_on = false;
		for (; it_indicator != it_range.second; ++it_indicator)
		{
			was_found = true;
			LLSpeakingIndicator* indicator = (*it_indicator).second;

			BOOL switch_current_on = switch_on;

			// we should show indicator for specified voice session only if this is current channel. EXT-5562.
			if (switch_current_on && indicator->getTargetSessionID().notNull())
			{
				switch_current_on = indicator->getTargetSessionID() == session_id;
				LL_DEBUGS("SpeakingIndicator") << "Session: " << session_id << ", target: " << indicator->getTargetSessionID() << ", the same? = " << switch_current_on << LL_ENDL;
			}
			was_switched_on = was_switched_on || switch_current_on;

			indicator->switchIndicator(switch_current_on);

		}

		if (was_found)
		{
			LL_DEBUGS("SpeakingIndicator") << mSpeakingIndicators.count(*it_uuid) << " indicators where found" << LL_ENDL;

			if (switch_on && !was_switched_on)
			{
				LL_DEBUGS("SpeakingIndicator") << "but non of them where switched on" << LL_ENDL;
			}

			if (was_switched_on)
			{
				// store switched on indicator to be able switch it off
				mSwitchedIndicatorsOn.insert(*it_uuid);
			}
		}
	}
}

void SpeakingIndicatorManager::ensureInstanceDoesNotExist(LLSpeakingIndicator* const speaking_indicator)
{
	LL_DEBUGS("SpeakingIndicator") << "Searching for an registered indicator instance: " << speaking_indicator << LL_ENDL;
	speaking_indicators_mmap_t::iterator it = mSpeakingIndicators.begin();
	for (;it != mSpeakingIndicators.end(); ++it)
	{
		if (it->second == speaking_indicator)
		{
			LL_DEBUGS("SpeakingIndicator") << "Found" << LL_ENDL;
			break;
		}
	}

	// It is possible with LLOutputMonitorCtrl the same instance of indicator is registered several
	// times with different UUIDs. This leads to crash after instance is destroyed because the
	// only one (specified by UUID in unregisterSpeakingIndicator()) is removed from the map.
	// So, using stored deleted pointer leads to crash. See EXT-4782.
	if (it != mSpeakingIndicators.end())
	{
		llwarns << "The same instance of indicator has already been registered, removing it: " << it->first << "|"<< speaking_indicator << llendl;
		llassert(it == mSpeakingIndicators.end());
		mSpeakingIndicators.erase(it);
	}
}


/************************************************************************/
/*         LLSpeakingIndicatorManager namespace implementation          */
/************************************************************************/

void LLSpeakingIndicatorManager::registerSpeakingIndicator(const LLUUID& speaker_id, LLSpeakingIndicator* const speaking_indicator,
														   const LLUUID& session_id)
{
	SpeakingIndicatorManager::instance().registerSpeakingIndicator(speaker_id, speaking_indicator, session_id);
}

void LLSpeakingIndicatorManager::unregisterSpeakingIndicator(const LLUUID& speaker_id, const LLSpeakingIndicator* const speaking_indicator)
{
	if(SpeakingIndicatorManager::instanceExists())
	{
		SpeakingIndicatorManager::instance().unregisterSpeakingIndicator(speaker_id, speaking_indicator);
	}
}

// EOF

