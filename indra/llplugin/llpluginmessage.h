/** 
 * @file llpluginmessage.h
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

#ifndef LL_LLPLUGINMESSAGE_H
#define LL_LLPLUGINMESSAGE_H

#include "llsd.h"


class LLPluginMessage
{
	LOG_CLASS(LLPluginMessage);
public:
	LLPluginMessage();
	LLPluginMessage(const std::string &message_class, const std::string &message_name);
	~LLPluginMessage();
	
	// reset all internal state
	void clear(void);
	
	// Sets the message class and name
	// Also has the side-effect of clearing any key/value pairs in the message.
	void setMessage(const std::string &message_class, const std::string &message_name);
	
	// Sets a key/value pair in the message
	void setValue(const std::string &key, const std::string &value);
	void setValueLLSD(const std::string &key, const LLSD &value);
	void setValueS32(const std::string &key, S32 value);
	void setValueU32(const std::string &key, U32 value);
	void setValueBoolean(const std::string &key, bool value);
	void setValueReal(const std::string &key, F64 value);
	
	std::string getClass(void) const;
	std::string getName(void) const;
	
	// Returns true if the specified key exists in this message (useful for optional parameters)
	bool hasValue(const std::string &key) const;
	
	// get the value of a particular key as a string.  If the key doesn't exist in the message, an empty string will be returned.
	std::string getValue(const std::string &key) const;

	// get the value of a particular key as LLSD.  If the key doesn't exist in the message, a null LLSD will be returned.
	LLSD getValueLLSD(const std::string &key) const;
	
	// get the value of a key as a S32.  If the value wasn't set as a S32, behavior is undefined.
	S32 getValueS32(const std::string &key) const;
	
	// get the value of a key as a U32.  Since there isn't an LLSD type for this, we use a hexadecimal string instead.
	U32 getValueU32(const std::string &key) const;

	// get the value of a key as a Boolean.
	bool getValueBoolean(const std::string &key) const;

	// get the value of a key as a float.
	F64 getValueReal(const std::string &key) const;

	// Flatten the message into a string
	std::string generate(void) const;

	// Parse an incoming message into component parts
	// (this clears out all existing state before starting the parse)
	// Returns -1 on failure, otherwise returns the number of key/value pairs in the message.
	int parse(const std::string &message);
	
	
private:
	
	LLSD mMessage;

};

class LLPluginMessageListener
{
public:
	virtual ~LLPluginMessageListener();
	virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
	
};

class LLPluginMessageDispatcher
{
public:
	virtual ~LLPluginMessageDispatcher();
	
	void addPluginMessageListener(LLPluginMessageListener *);
	void removePluginMessageListener(LLPluginMessageListener *);
protected:
	void dispatchPluginMessage(const LLPluginMessage &message);

	typedef std::set<LLPluginMessageListener*> listener_set_t;
	listener_set_t mListeners;
};


#endif // LL_LLPLUGINMESSAGE_H
