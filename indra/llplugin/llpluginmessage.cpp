/** 
 * @file llpluginmessage.cpp
 * @brief LLPluginMessage encapsulates the serialization/deserialization of messages passed to and from plugins.
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

#include "linden_common.h"

#include "llpluginmessage.h"
#include "llsdserialize.h"
#include "u64.h"

/**
 * Constructor.
 */
LLPluginMessage::LLPluginMessage()
{
}

/**
 * Constructor.
 *
 * @param[in] p Existing message
 */
LLPluginMessage::LLPluginMessage(const LLPluginMessage &p)
{
    mMessage = p.mMessage;
}

/**
 * Constructor.
 *
 * @param[in] message_class Message class
 * @param[in] message_name Message name
 */
LLPluginMessage::LLPluginMessage(const std::string &message_class, const std::string &message_name)
{
    setMessage(message_class, message_name);
}


/**
 * Destructor.
 */
LLPluginMessage::~LLPluginMessage()
{
}

/**
 * Reset all internal state.
 */
void LLPluginMessage::clear()
{
    mMessage = LLSD::emptyMap();
    mMessage["params"] = LLSD::emptyMap();
}

/**
 * Sets the message class and name. Also has the side-effect of clearing any key-value pairs in the message.
 *
 * @param[in] message_class Message class
 * @param[in] message_name Message name
 */
void LLPluginMessage::setMessage(const std::string &message_class, const std::string &message_name)
{
    clear();
    mMessage["class"] = message_class;
    mMessage["name"] = message_name;
}

/**
 * Sets a key/value pair in the message, where the value is a string.
 *
 * @param[in] key Key
 * @param[in] value String value
 */
void LLPluginMessage::setValue(const std::string &key, const std::string &value)
{
    mMessage["params"][key] = value;
}

/**
 * Sets a key/value pair in the message, where the value is LLSD.
 *
 * @param[in] key Key
 * @param[in] value LLSD value
 */
void LLPluginMessage::setValueLLSD(const std::string &key, const LLSD &value)
{
    mMessage["params"][key] = value;
}

/**
 * Sets a key/value pair in the message, where the value is signed 32-bit.
 *
 * @param[in] key Key
 * @param[in] value 32-bit signed value
 */
void LLPluginMessage::setValueS32(const std::string &key, S32 value)
{
    mMessage["params"][key] = value;
}

/**
 * Sets a key/value pair in the message, where the value is unsigned 32-bit. The value is stored as a string beginning with "0x".
 *
 * @param[in] key Key
 * @param[in] value 32-bit unsigned value
 */
void LLPluginMessage::setValueU32(const std::string &key, U32 value)
{
    std::stringstream temp;
    temp << "0x" << std::hex << value;
    setValue(key, temp.str());
}

/**
 * Sets a key/value pair in the message, where the value is a bool.
 *
 * @param[in] key Key
 * @param[in] value Boolean value
 */
void LLPluginMessage::setValueBoolean(const std::string &key, bool value)
{
    mMessage["params"][key] = value;
}

/**
 * Sets a key/value pair in the message, where the value is a double.
 *
 * @param[in] key Key
 * @param[in] value Boolean value
 */
void LLPluginMessage::setValueReal(const std::string &key, F64 value)
{
    mMessage["params"][key] = value;
}

/**
 * Sets a key/value pair in the message, where the value is a pointer. The pointer is stored as a string.
 *
 * @param[in] key Key
 * @param[in] value Pointer value
 */
void LLPluginMessage::setValuePointer(const std::string &key, void* value)
{
    std::stringstream temp;
    // iostreams should output pointer values in hex with an initial 0x by default.
    temp << value;
    setValue(key, temp.str());
}

/**
 * Gets the message class.
 *
 * @return Message class
 */
std::string LLPluginMessage::getClass(void) const
{
    return mMessage["class"];
}

/**
 * Gets the message name.
 *
 * @return Message name
 */
std::string LLPluginMessage::getName(void) const
{
    return mMessage["name"];
}

/**
 *  Returns true if the specified key exists in this message (useful for optional parameters).
 *
 * @param[in] key Key
 *
 * @return True if key exists, false otherwise.
 */
bool LLPluginMessage::hasValue(const std::string &key) const
{
    bool result = false;
    
    if(mMessage["params"].has(key))
    {
        result = true;
    }
    
    return result;
}

/**
 *  Gets the value of a key as a string. If the key does not exist, an empty string will be returned.
 *
 * @param[in] key Key
 *
 * @return String value of key if key exists, empty string if key does not exist.
 */
std::string LLPluginMessage::getValue(const std::string &key) const
{
    std::string result;
    
    if(mMessage["params"].has(key))
    {
        result = mMessage["params"][key].asString();
    }
    
    return result;
}

/**
 *  Gets the value of a key as LLSD. If the key does not exist, a null LLSD will be returned.
 *
 * @param[in] key Key
 *
 * @return LLSD value of key if key exists, null LLSD if key does not exist.
 */
LLSD LLPluginMessage::getValueLLSD(const std::string &key) const
{
    LLSD result;

    if(mMessage["params"].has(key))
    {
        result = mMessage["params"][key];
    }
    
    return result;
}

/**
 *  Gets the value of a key as signed 32-bit int. If the key does not exist, 0 will be returned.
 *
 * @param[in] key Key
 *
 * @return Signed 32-bit int value of key if key exists, 0 if key does not exist.
 */
S32 LLPluginMessage::getValueS32(const std::string &key) const
{
    S32 result = 0;

    if(mMessage["params"].has(key))
    {
        result = mMessage["params"][key].asInteger();
    }
    
    return result;
}

/**
 *  Gets the value of a key as unsigned 32-bit int. If the key does not exist, 0 will be returned.
 *
 * @param[in] key Key
 *
 * @return Unsigned 32-bit int value of key if key exists, 0 if key does not exist.
 */
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

/**
 *  Gets the value of a key as a bool. If the key does not exist, false will be returned.
 *
 * @param[in] key Key
 *
 * @return Boolean value of key if it exists, false otherwise.
 */
bool LLPluginMessage::getValueBoolean(const std::string &key) const
{
    bool result = false;

    if(mMessage["params"].has(key))
    {
        result = mMessage["params"][key].asBoolean();
    }
    
    return result;
}

/**
 *  Gets the value of a key as a double. If the key does not exist, 0 will be returned.
 *
 * @param[in] key Key
 *
 * @return Value as a double if key exists, 0 otherwise.
 */
F64 LLPluginMessage::getValueReal(const std::string &key) const
{
    F64 result = 0.0f;

    if(mMessage["params"].has(key))
    {
        result = mMessage["params"][key].asReal();
    }
    
    return result;
}

/**
 *  Gets the value of a key as a pointer. If the key does not exist, NULL will be returned.
 *
 * @param[in] key Key
 *
 * @return Pointer value if key exists, NULL otherwise.
 */
void* LLPluginMessage::getValuePointer(const std::string &key) const
{
    void* result = NULL;

    if(mMessage["params"].has(key))
    {
        std::string value = mMessage["params"][key].asString();
        
        result = (void*)llstrtou64(value.c_str(), NULL, 16);
    }
    
    return result;
}

/**
 *  Flatten the message into a string.
 *
 * @return Message as a string.
 */
std::string LLPluginMessage::generate(void) const
{
    std::ostringstream result;
    
    // Pretty XML may be slightly easier to deal with while debugging...
//  LLSDSerialize::toXML(mMessage, result);
    LLSDSerialize::toPrettyXML(mMessage, result);
    
    return result.str();
}

/**
 *  Parse an incoming message into component parts. Clears all existing state before starting the parse.
 *
 * @return Returns -1 on failure, otherwise returns the number of key/value pairs in the incoming message.
 */
int LLPluginMessage::parse(const std::string &message)
{
    // clear any previous state
    clear();

    std::istringstream input(message);
    
    S32 parse_result = LLSDSerialize::fromXML(mMessage, input);
    
    return (int)parse_result;
}


/**
 * Destructor
 */
LLPluginMessageListener::~LLPluginMessageListener()
{
    // TODO: should listeners have a way to ensure they're removed from dispatcher lists when deleted?
}


/**
 * Destructor
 */
LLPluginMessageDispatcher::~LLPluginMessageDispatcher()
{
    
}
    
/**
 * Add a message listener. TODO:DOC need more info on what uses this. when are multiple listeners needed?
 *
 * @param[in] listener Message listener
 */
void LLPluginMessageDispatcher::addPluginMessageListener(LLPluginMessageListener *listener)
{
    mListeners.insert(listener);
}

/**
 * Remove a message listener.
 *
 * @param[in] listener Message listener
 */
void LLPluginMessageDispatcher::removePluginMessageListener(LLPluginMessageListener *listener)
{
    mListeners.erase(listener);
}

/**
 * Distribute a message to all message listeners.
 *
 * @param[in] message Message
 */
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
