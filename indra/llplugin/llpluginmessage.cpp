/** 
 * @file llpluginmessage.cpp
 * @brief LLPluginMessage encapsulates the serialization/deserialization of messages passed to and from plugins.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "linden_common.h"

#include "llpluginmessage.h"
#include "llsdserialize.h"

LLPluginMessage::LLPluginMessage()
{
}

LLPluginMessage::LLPluginMessage(const std::string &message_class, const std::string &message_name)
{
	setMessage(message_class, message_name);
}


LLPluginMessage::~LLPluginMessage()
{
}

void LLPluginMessage::clear()
{
	mMessage = LLSD::emptyMap();
	mMessage["params"] = LLSD::emptyMap();
}

void LLPluginMessage::setMessage(const std::string &message_class, const std::string &message_name)
{
	clear();
	mMessage["class"] = message_class;
	mMessage["name"] = message_name;
}

void LLPluginMessage::setValue(const std::string &key, const std::string &value)
{
	mMessage["params"][key] = value;
}

void LLPluginMessage::setValueLLSD(const std::string &key, const LLSD &value)
{
	mMessage["params"][key] = value;
}

void LLPluginMessage::setValueS32(const std::string &key, S32 value)
{
	mMessage["params"][key] = value;
}

void LLPluginMessage::setValueU32(const std::string &key, U32 value)
{
	std::stringstream temp;
	temp << "0x" << std::hex << value;
	setValue(key, temp.str());
}

void LLPluginMessage::setValueBoolean(const std::string &key, bool value)
{
	mMessage["params"][key] = value;
}

void LLPluginMessage::setValueReal(const std::string &key, F64 value)
{
	mMessage["params"][key] = value;
}

std::string LLPluginMessage::getClass(void) const
{
	return mMessage["class"];
}

std::string LLPluginMessage::getName(void) const
{
	return mMessage["name"];
}

bool LLPluginMessage::hasValue(const std::string &key) const
{
	bool result = false;
	
	if(mMessage["params"].has(key))
	{
		result = true;
	}
	
	return result;
}

std::string LLPluginMessage::getValue(const std::string &key) const
{
	std::string result;
	
	if(mMessage["params"].has(key))
	{
		result = mMessage["params"][key].asString();
	}
	
	return result;
}

LLSD LLPluginMessage::getValueLLSD(const std::string &key) const
{
	LLSD result;

	if(mMessage["params"].has(key))
	{
		result = mMessage["params"][key];
	}
	
	return result;
}

S32 LLPluginMessage::getValueS32(const std::string &key) const
{
	S32 result = 0;

	if(mMessage["params"].has(key))
	{
		result = mMessage["params"][key].asInteger();
	}
	
	return result;
}

U32 LLPluginMessage::getValueU32(const std::string &key) const
{
	U32 result = 0;

	if(mMessage["params"].has(key))
	{
		std::string value = mMessage["params"][key].asString();
		
		result = (U32)strtoul(value.c_str(), NULL, 16);
	}
	
	return result;
}

bool LLPluginMessage::getValueBoolean(const std::string &key) const
{
	bool result = false;

	if(mMessage["params"].has(key))
	{
		result = mMessage["params"][key].asBoolean();
	}
	
	return result;
}

F64 LLPluginMessage::getValueReal(const std::string &key) const
{
	F64 result = 0.0f;

	if(mMessage["params"].has(key))
	{
		result = mMessage["params"][key].asReal();
	}
	
	return result;
}

std::string LLPluginMessage::generate(void) const
{
	std::ostringstream result;
	
	// Pretty XML may be slightly easier to deal with while debugging...
//	LLSDSerialize::toXML(mMessage, result);
	LLSDSerialize::toPrettyXML(mMessage, result);
	
	return result.str();
}


int LLPluginMessage::parse(const std::string &message)
{
	// clear any previous state
	clear();

	std::istringstream input(message);
	
	S32 parse_result = LLSDSerialize::fromXML(mMessage, input);
	
	return (int)parse_result;
}


LLPluginMessageListener::~LLPluginMessageListener()
{
	// TODO: should listeners have a way to ensure they're removed from dispatcher lists when deleted?
}


LLPluginMessageDispatcher::~LLPluginMessageDispatcher()
{
	
}
	
void LLPluginMessageDispatcher::addPluginMessageListener(LLPluginMessageListener *listener)
{
	mListeners.insert(listener);
}

void LLPluginMessageDispatcher::removePluginMessageListener(LLPluginMessageListener *listener)
{
	mListeners.erase(listener);
}

void LLPluginMessageDispatcher::dispatchPluginMessage(const LLPluginMessage &message)
{
	for (listener_set_t::iterator it = mListeners.begin();
		it != mListeners.end();
		)
	{
		LLPluginMessageListener* listener = *it;
		listener->receivePluginMessage(message);
		// In case something deleted an entry.
		it = mListeners.upper_bound(listener);
	}
}
