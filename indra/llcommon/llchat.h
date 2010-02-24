/** 
 * @file llchat.h
 * @author James Cook
 * @brief Chat constants and data structures.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLCHAT_H
#define LL_LLCHAT_H

#include "lluuid.h"
#include "v3math.h"

// enumerations used by the chat system
typedef enum e_chat_source_type
{
	CHAT_SOURCE_SYSTEM = 0,
	CHAT_SOURCE_AGENT = 1,
	CHAT_SOURCE_OBJECT = 2
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
	CHAT_TYPE_OWNER = 8
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
	CHAT_STYLE_IRC
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
