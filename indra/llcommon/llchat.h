/** 
 * @file llchat.h
 * @author James Cook
 * @brief Chat constants and data structures.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCHAT_H
#define LL_LLCHAT_H

#include "llstring.h"
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

// A piece of chat
class LLChat
{
public:
	LLChat(const LLString& text = LLString::null)
	:	mText(text),
		mFromName(),
		mFromID(),
		mSourceType(CHAT_SOURCE_AGENT),
		mChatType(CHAT_TYPE_NORMAL),
		mAudible(CHAT_AUDIBLE_FULLY),
		mMuted(FALSE),
		mTime(0.0),
		mPosAgent()
	{ }
	
	LLString		mText;		// UTF-8 line of text
	LLString		mFromName;	// agent or object name
	LLUUID			mFromID;	// agent id or object id
	EChatSourceType	mSourceType;
	EChatType		mChatType;
	EChatAudible	mAudible;
	BOOL			mMuted;		// pass muted chat to maintain list of chatters
	F64				mTime;		// viewer only, seconds from viewer start
	LLVector3		mPosAgent;
};

#endif
