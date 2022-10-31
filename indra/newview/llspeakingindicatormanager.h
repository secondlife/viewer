/** 
 * @file llspeakingindicatormanager.h
 * @author Mike Antipov
 * @brief Interfeace of LLSpeackerIndicator class to be processed depend on avatars are in the same voice channel.
 * Also register/unregister methods for this class are declared
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

#ifndef LL_LLSPEAKINGINDICATORMANAGER_H
#define LL_LLSPEAKINGINDICATORMANAGER_H

class SpeakingIndicatorManager;

class LLSpeakingIndicator
{
public:
    virtual ~LLSpeakingIndicator(){}
    virtual void switchIndicator(bool switch_on) = 0;

private:
    friend class SpeakingIndicatorManager;
    // Accessors for target voice session UUID.
    // They are intended to be used only from SpeakingIndicatorManager to ensure target session is 
    // the same indicator was registered with.
    void setTargetSessionID(const LLUUID& session_id) { mTargetSessionID = session_id; }
    const LLUUID& getTargetSessionID() { return mTargetSessionID; }

    /**
     * session UUID for which indicator should be shown only.
     *      If it is set, registered indicator will be shown only in voice channel
     *      which has the same session id (EXT-5562).
     */
    LLUUID mTargetSessionID;
};

// See EXT-3976.
namespace LLSpeakingIndicatorManager
{
    /**
     * Stores passed speaking indicator to control its visibility.
     *
     * Registered indicator is set visible if an appropriate avatar is in the same voice channel with Agent.
     * It ignores instances of Agent's indicator.
     *
     * @param speaker_id LLUUID of an avatar whose speaker indicator is registered.
     * @param speaking_indicator instance of the speaker indicator to be registered.
     * @param session_id session UUID for which indicator should be shown only.
     *      If this parameter is set registered indicator will be shown only in voice channel
     *      which has the same session id (EXT-5562).
     */
    void registerSpeakingIndicator(const LLUUID& speaker_id, LLSpeakingIndicator* const speaking_indicator,
                                   const LLUUID& session_id);

    /**
     * Removes passed speaking indicator from observing.
     *
     * @param speaker_id LLUUID of an avatar whose speaker indicator should be unregistered.
     * @param speaking_indicator instance of the speaker indicator to be unregistered.
     */
    void unregisterSpeakingIndicator(const LLUUID& speaker_id, const LLSpeakingIndicator* const speaking_indicator);

    /**
     * Switch on/off registered speaking indicator according to the most current voice client status
     */
     void updateSpeakingIndicators();
}

#endif // LL_LLSPEAKINGINDICATORMANAGER_H
