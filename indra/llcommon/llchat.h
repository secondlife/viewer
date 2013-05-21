/** 
 * @file llchat.h
 * @author James Cook
 * @brief Chat constants and data structures.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLCHAT_H
#define LL_LLCHAT_H

#include "lluuid.h"
#include "v3math.h"

// enumerations used by the chat system
typedef enum e_chat_source_type
{
	CHAT_SOURCE_SYSTEM = 0,
	CHAT_SOURCE_AGENT = 1,
	CHAT_SOURCE_OBJECT = 2,
	CHAT_SOURCE_UNKNOWN = 3
} EChatSourceType;

typedef enum e_chat_type
{
	CHAT_TYPE_WHISPER = 0,
	CHAT_TYPE_NORMAL = 1,
	CHAT_TYPE_SHOUT = 2,
	CHAT_TYPE_START = 4,
	CHAT_TYPE_STOP = 5,
	CHAT_TYPE_DEBUG_MSG = 6,
	CHAT_TYPE_REGION = 7,
	CHAT_TYPE_OWNER = 8,
	CHAT_TYPE_DIRECT = 9		// From llRegionSayTo()
} EChatType;

typedef enum e_chat_audible_level
{
	CHAT_AUDIBLE_NOT = -1,
	CHAT_AUDIBLE_BARELY = 0,
	CHAT_AUDIBLE_FULLY = 1
} EChatAudible;

typedef enum e_chat_style
{
	CHAT_STYLE_NORMAL,
	CHAT_STYLE_IRC,
	CHAT_STYLE_HISTORY
}EChatStyle;

// A piece of chat
class LLChat
{
public:
	LLChat(const std::string& text = std::string())
	:	mText(text),
		mFromName(),
		mFromID(),
		mNotifId(),
		mOwnerID(),
		mSourceType(CHAT_SOURCE_AGENT),
		mChatType(CHAT_TYPE_NORMAL),
		mAudible(CHAT_AUDIBLE_FULLY),
		mMuted(FALSE),
		mTime(0.0),
		mTimeStr(),
		mPosAgent(),
		mURL(),
		mChatStyle(CHAT_STYLE_NORMAL),
		mSessionID()
	{ }
	
	std::string		mText;		// UTF-8 line of text
	std::string		mFromName;	// agent or object name
	LLUUID			mFromID;	// agent id or object id
	LLUUID			mNotifId;
	LLUUID			mOwnerID;
	EChatSourceType	mSourceType;
	EChatType		mChatType;
	EChatAudible	mAudible;
	BOOL			mMuted;		// pass muted chat to maintain list of chatters
	F64				mTime;		// viewer only, seconds from viewer start
	std::string		mTimeStr;
	LLVector3		mPosAgent;
	std::string		mURL;
	EChatStyle		mChatStyle;
	LLUUID			mSessionID;
};

#endif
