/** 
 * @file llparticipantlist.h
 * @brief LLParticipantList intended to update view(LLAvatarList) according to incoming messages
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

#include "llviewerprecompiledheaders.h"
#include "llevent.h"

class LLSpeakerMgr;
class LLAvatarList;

class LLParticipantList
{
	public:
		LLParticipantList(LLSpeakerMgr* data_source, LLAvatarList* avatar_list);
		~LLParticipantList();

	protected:

		//List of listeners implementing LLOldEvents::LLSimpleListener.
		//There is no way to handle all the events in one listener as LLSpeakerMgr registers listeners in such a way
		//that one listener can handle only one type of event
		class SpeakerAddListener : public LLOldEvents::LLSimpleListener
		{
		public:
			SpeakerAddListener(LLAvatarList* avatar_list) : mAvatarList(avatar_list) {}

			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
			LLAvatarList* mAvatarList;
		};

		class SpeakerRemoveListener : public LLOldEvents::LLSimpleListener
		{
		public:
			SpeakerRemoveListener(LLAvatarList* avatar_list) : mAvatarList(avatar_list) {}

			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
			LLAvatarList* mAvatarList;
		};

		class SpeakerClearListener : public LLOldEvents::LLSimpleListener
		{
		public:
			SpeakerClearListener(LLAvatarList* avatar_list) : mAvatarList(avatar_list) {}

			/*virtual*/ bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata);
			LLAvatarList* mAvatarList;
		};
	private:
		void onAvatarListDoubleClicked(LLAvatarList* list);

		LLSpeakerMgr*		mSpeakerMgr;
		LLAvatarList* 		mAvatarList;

		SpeakerAddListener* mSpeakerAddListener;
		SpeakerRemoveListener* mSpeakerRemoveListener;
		SpeakerClearListener* mSpeakerClearListener;
};
