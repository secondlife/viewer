/** 
 * @file llgesture.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "linden_common.h"

#include "indra_constants.h"

#include "llgesture.h"
#include "llendianswizzle.h"
#include "message.h"
#include <boost/tokenizer.hpp>

// for allocating serialization buffers - these need to be updated when members change
const S32 LLGestureList::SERIAL_HEADER_SIZE = sizeof(S32);
const S32 LLGesture::MAX_SERIAL_SIZE = sizeof(KEY) + sizeof(MASK) + 16 + 26 + 41 + 41;

LLGesture::LLGesture()
:	mKey(KEY_NONE),
	mMask(MASK_NONE),
	mTrigger(),
	mTriggerLower(),
	mSoundItemID(),
	mAnimation(),
	mOutputString()
{ }

LLGesture::LLGesture(KEY key, MASK mask, const std::string &trigger,
					 const LLUUID &sound_item_id, 
					 const std::string &animation,
					 const std::string &output_string)
:
	mKey(key),
	mMask(mask),
	mTrigger(trigger),
	mTriggerLower(trigger),
	mSoundItemID(sound_item_id),
	mAnimation(animation),
	mOutputString(output_string)
{
	mTriggerLower = utf8str_tolower(mTriggerLower);
}

LLGesture::LLGesture(U8 **buffer, S32 max_size)
{
	*buffer = deserialize(*buffer, max_size);
}

LLGesture::LLGesture(const LLGesture &rhs)
{
	mKey			= rhs.mKey;
	mMask			= rhs.mMask;
	mTrigger		= rhs.mTrigger;
	mTriggerLower	= rhs.mTriggerLower;
	mSoundItemID	= rhs.mSoundItemID;
	mAnimation		= rhs.mAnimation;
	mOutputString	= rhs.mOutputString;
}

const LLGesture &LLGesture::operator =(const LLGesture &rhs)
{
	mKey			= rhs.mKey;
	mMask			= rhs.mMask;
	mTrigger		= rhs.mTrigger;
	mTriggerLower	= rhs.mTriggerLower;
	mSoundItemID	= rhs.mSoundItemID;
	mAnimation		= rhs.mAnimation;
	mOutputString	= rhs.mOutputString;
	return (*this);
}


BOOL LLGesture::trigger(KEY key, MASK mask)
{
	LL_WARNS() << "Parent class trigger called: you probably didn't mean this." << LL_ENDL;
	return FALSE;
}


BOOL LLGesture::trigger(const std::string& trigger_string)
{
	LL_WARNS() << "Parent class trigger called: you probably didn't mean this." << LL_ENDL;
	return FALSE;
}

// NOT endian-neutral
U8 *LLGesture::serialize(U8 *buffer) const
{
	htolememcpy(buffer, &mKey, MVT_S8, 1);
	buffer += sizeof(mKey);
	htolememcpy(buffer, &mMask, MVT_U32, 4);
	buffer += sizeof(mMask);
	htolememcpy(buffer, mSoundItemID.mData, MVT_LLUUID, 16);
	buffer += 16;
	
	memcpy(buffer, mTrigger.c_str(), mTrigger.length() + 1);		/* Flawfinder: ignore */
	buffer += mTrigger.length() + 1;
	memcpy(buffer, mAnimation.c_str(), mAnimation.length() + 1);		/* Flawfinder: ignore */
	buffer += mAnimation.length() + 1;
	memcpy(buffer, mOutputString.c_str(), mOutputString.length() + 1);		/* Flawfinder: ignore */
	buffer += mOutputString.length() + 1;

	return buffer;
}

U8 *LLGesture::deserialize(U8 *buffer, S32 max_size)
{
	U8 *tmp = buffer;

	if (tmp + sizeof(mKey) + sizeof(mMask) + 16 > buffer + max_size)
	{
		LL_WARNS() << "Attempt to read past end of buffer, bad data!!!!" << LL_ENDL;
		return buffer;
	}

	htolememcpy(&mKey, tmp, MVT_S8, 1);
	tmp += sizeof(mKey);
	htolememcpy(&mMask, tmp, MVT_U32, 4);
	tmp += sizeof(mMask);
	htolememcpy(mSoundItemID.mData, tmp, MVT_LLUUID, 16);
	tmp += 16;
	
	mTrigger.assign((char *)tmp);
	mTriggerLower = mTrigger;
	mTriggerLower = utf8str_tolower(mTriggerLower);
	tmp += mTrigger.length() + 1;
	mAnimation.assign((char *)tmp);
	//RN: force animation names to lower case
	// must do this for backwards compatibility
	mAnimation = utf8str_tolower(mAnimation);
	tmp += mAnimation.length() + 1;
	mOutputString.assign((char *)tmp);
	tmp += mOutputString.length() + 1;

	if (tmp > buffer + max_size)
	{
		LL_WARNS() << "Read past end of buffer, bad data!!!!" << LL_ENDL;
		return tmp;
	}

	return tmp;
}

S32 LLGesture::getMaxSerialSize()
{
	return MAX_SERIAL_SIZE;
}

//---------------------------------------------------------------------
// LLGestureList
//---------------------------------------------------------------------

LLGestureList::LLGestureList()
:	mList(0)
{}

LLGestureList::~LLGestureList()
{
	deleteAll();
}


void LLGestureList::deleteAll()
{
	delete_and_clear(mList);
}

// Iterates through space delimited tokens in string, triggering any gestures found.
// Generates a revised string that has the found tokens replaced by their replacement strings
// and (as a minor side effect) has multiple spaces in a row replaced by single spaces.
BOOL LLGestureList::triggerAndReviseString(const std::string &string, std::string* revised_string)
{
	std::string tokenized = string;

	BOOL found_gestures = FALSE;
	BOOL first_token = TRUE;

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(" ");
	tokenizer tokens(string, sep);
	tokenizer::iterator token_iter;

	for( token_iter = tokens.begin(); token_iter != tokens.end(); ++token_iter)
	{
		LLGesture* gesture = NULL;

		if( !found_gestures ) // Only pay attention to the first gesture in the string.
		{
			std::string cur_token_lower = *token_iter;
			LLStringUtil::toLower(cur_token_lower);

			for (U32 i = 0; i < mList.size(); i++)
			{
				gesture = mList.at(i);
				if (gesture->trigger(cur_token_lower))
				{
					if( !gesture->getOutputString().empty() )
					{
						if( !first_token )
						{
							revised_string->append( " " );
						}

						// Don't muck with the user's capitalization if we don't have to.
						const std::string& output = gesture->getOutputString();
						std::string output_lower = std::string(output.c_str());
						LLStringUtil::toLower(output_lower);
						if( cur_token_lower == output_lower )
						{
							revised_string->append(*token_iter);
						}
						else
						{
							revised_string->append(output);
						}

					}
					found_gestures = TRUE;
					break;
				}
				gesture = NULL;
			}
		}

		if( !gesture )
		{
			if( !first_token )
			{
				revised_string->append( " " );
			}
			revised_string->append( *token_iter );
		}

		first_token = FALSE;
	}
	return found_gestures;
}



BOOL LLGestureList::trigger(KEY key, MASK mask)
{
	for (U32 i = 0; i < mList.size(); i++)
	{
		LLGesture* gesture = mList.at(i);
		if( gesture )
		{
			if (gesture->trigger(key, mask))
			{
				return TRUE;
			}
		}
		else
		{
			LL_WARNS() << "NULL gesture in gesture list (" << i << ")" << LL_ENDL;
		}
	}
	return FALSE;
}

// NOT endian-neutral
U8 *LLGestureList::serialize(U8 *buffer) const
{
	// a single S32 serves as the header that tells us how many to read
	U32 count = mList.size();
	htolememcpy(buffer, &count, MVT_S32, 4);
	buffer += sizeof(count);

	for (S32 i = 0; i < count; i++)
	{
		buffer = mList[i]->serialize(buffer);
	}

	return buffer;
}

const S32 MAX_GESTURES = 4096;

U8 *LLGestureList::deserialize(U8 *buffer, S32 max_size)
{
	deleteAll();

	S32 count;
	U8 *tmp = buffer;

	if (tmp + sizeof(count) > buffer + max_size)
	{
		LL_WARNS() << "Invalid max_size" << LL_ENDL;
		return buffer;
	}

	htolememcpy(&count, tmp, MVT_S32, 4);

	if (count > MAX_GESTURES)
	{
		LL_WARNS() << "Unreasonably large gesture list count in deserialize: " << count << LL_ENDL;
		return tmp;
	}

	tmp += sizeof(count);

	mList.resize(count);

	for (S32 i = 0; i < count; i++)
	{
		mList[i] = create_gesture(&tmp, max_size - (S32)(tmp - buffer));
		if (tmp - buffer > max_size)
		{
			LL_WARNS() << "Deserialization read past end of buffer, bad data!!!!" << LL_ENDL;
			return tmp;
		}
	}

	return tmp;
}

// this is a helper for deserialize
// it gets overridden by LLViewerGestureList to create LLViewerGestures
// overridden by child class to use local LLGesture implementation
LLGesture *LLGestureList::create_gesture(U8 **buffer, S32 max_size)
{
	return new LLGesture(buffer, max_size);
}

S32 LLGestureList::getMaxSerialSize()
{
	return SERIAL_HEADER_SIZE + (count() * LLGesture::getMaxSerialSize());
}
