/** 
 * @file llpluginmessage.h
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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
 * @endcond
 */

#ifndef LL_LLPLUGINMESSAGE_H
#define LL_LLPLUGINMESSAGE_H

#include "llsd.h"

/**
 * @brief LLPluginMessage encapsulates the serialization/deserialization of messages passed to and from plugins.
 */
class LLPluginMessage
{
    LOG_CLASS(LLPluginMessage);
public:
    LLPluginMessage();
    LLPluginMessage(const LLPluginMessage &p);
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
    void setValuePointer(const std::string &key, void *value);
    
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

    // get the value of a key as a pointer.
    void* getValuePointer(const std::string &key) const;

    // Flatten the message into a string
    std::string generate(void) const;

    // Parse an incoming message into component parts
    // (this clears out all existing state before starting the parse)
    // Returns -1 on failure, otherwise returns the number of key/value pairs in the message.
    int parse(const std::string &message);
    
    
private:
    
    LLSD mMessage;

};

/**
 * @brief Listener for plugin messages.
 */
class LLPluginMessageListener
{
public:
    virtual ~LLPluginMessageListener();
   /** Plugin receives message from plugin loader shell. */
    virtual void receivePluginMessage(const LLPluginMessage &message) = 0;
    
};

/**
 * @brief Dispatcher for plugin messages.
 *
 * Manages the set of plugin message listeners and distributes messages to plugin message listeners.
 */
class LLPluginMessageDispatcher
{
public:
    virtual ~LLPluginMessageDispatcher();
    
    void addPluginMessageListener(LLPluginMessageListener *);
    void removePluginMessageListener(LLPluginMessageListener *);
protected:
    void dispatchPluginMessage(const LLPluginMessage &message);

   /** A set of message listeners. */
    typedef std::set<LLPluginMessageListener*> listener_set_t;
   /** The set of message listeners. */
    listener_set_t mListeners;
};


#endif // LL_LLPLUGINMESSAGE_H
