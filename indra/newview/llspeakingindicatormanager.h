/** 
 * @file llspeakingindicatormanager.h
 * @author Mike Antipov
 * @brief Interfeace of LLSpeackerIndicator class to be processed depend on avatars are in the same voice channel.
 * Also register/unregister methods for this class are declared
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#ifndef LL_LLSPEAKINGINDICATORMANAGER_H
#define LL_LLSPEAKINGINDICATORMANAGER_H

class LLSpeakingIndicator
{
public:
	virtual void switchIndicator(bool switch_on) = 0;
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
	 */
	void registerSpeakingIndicator(const LLUUID& speaker_id, LLSpeakingIndicator* const speaking_indicator);

	/**
	 * Removes passed speaking indicator from observing.
	 *
	 * @param speaker_id LLUUID of an avatar whose speaker indicator should be unregistered.
	 * @param speaking_indicator instance of the speaker indicator to be unregistered.
	 */
	void unregisterSpeakingIndicator(const LLUUID& speaker_id, const LLSpeakingIndicator* const speaking_indicator);
}

#endif // LL_LLSPEAKINGINDICATORMANAGER_H
