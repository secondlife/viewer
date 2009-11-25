/** 
 * @file llcallfloater.h
 * @author Mike Antipov
 * @brief Voice Control Panel in a Voice Chats (P2P, Group, Nearby...).
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLCALLFLOATER_H
#define LL_LLCALLFLOATER_H

#include "llfloater.h"

class LLAvatarList;
class LLParticipantList;
class LLSpeakerMgr;

/**
 * The Voice Control Panel is an ambient window summoned by clicking the flyout chevron on the Speak button.
 * It can be torn-off and freely positioned onscreen.
 *
 * When the Resident is engaged in Nearby Voice Chat, the Voice Control Panel provides control over 
 * the Resident's own microphone input volume, the audible volume of each of the other participants,
 * the Resident's own Voice Morphing settings (if she has subscribed to enable the feature), and Voice Recording.
 *
 * When the Resident is engaged in Group Voice Chat, the Voice Control Panel also provides an 
 * 'End Call' button to allow the Resident to leave that voice channel.
 */
class LLCallFloater : public LLFloater
{
public:
	LLCallFloater();
	~LLCallFloater();

	/*virtual*/ BOOL postBuild();


private:
	LLSpeakerMgr* mSpeakerManager;
	LLParticipantList* mPaticipants;
	LLAvatarList* mAvatarList;
};


#endif //LL_LLCALLFLOATER_H

