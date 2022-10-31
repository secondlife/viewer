/** 
 * @file llmessageconfig.cpp
 * @brief Live file handling for messaging
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "llmessageconfig.h"
#include "llfile.h"
#include "lllivefile.h"
#include "llsd.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "message.h"

static const char messageConfigFileName[] = "message.xml";
static const F32 messageConfigRefreshRate = 5.0; // seconds

static std::string sServerName = "";
static std::string sConfigDir = "";

static std::string sServerDefault;
static LLSD sMessages;


class LLMessageConfigFile : public LLLiveFile
{
public:
    LLMessageConfigFile() :
        LLLiveFile(filename(), messageConfigRefreshRate),
        mMaxQueuedEvents(0)
            { }

    static std::string filename();

    LLSD mMessages;
    std::string mServerDefault;
    
    static LLMessageConfigFile& instance();
        // return the singleton configuration file

    /* virtual */ bool loadFile();
    void loadServerDefaults(const LLSD& data);
    void loadMaxQueuedEvents(const LLSD& data);
    void loadMessages(const LLSD& data);
    void loadCapBans(const LLSD& blacklist);
    void loadMessageBans(const LLSD& blacklist);
    bool isCapBanned(const std::string& cap_name) const;

public:
    LLSD mCapBans;
    S32 mMaxQueuedEvents;

private:
    static const S32 DEFAULT_MAX_QUEUED_EVENTS = 100;
};

std::string LLMessageConfigFile::filename()
{
    std::ostringstream ostr;
    ostr << sConfigDir//gAppSimp->getOption("configdir").asString()
        << "/" << messageConfigFileName;
    return ostr.str();
}

LLMessageConfigFile& LLMessageConfigFile::instance()
{
    static LLMessageConfigFile the_file;
    the_file.checkAndReload();
    return the_file;
}

// virtual
bool LLMessageConfigFile::loadFile()
{
    LLSD data;
    {
        llifstream file(filename().c_str());
        
        if (file.is_open())
        {
            LL_DEBUGS("AppInit") << "Loading message.xml file at " << filename() << LL_ENDL;
            LLSDSerialize::fromXML(data, file);
        }

        if (data.isUndefined())
        {
            LL_INFOS("AppInit") << "LLMessageConfigFile::loadFile: file missing,"
                " ill-formed, or simply undefined; not changing the"
                " file" << LL_ENDL;
            return false;
        }
    }
    loadServerDefaults(data);
    loadMaxQueuedEvents(data);
    loadMessages(data);
    loadCapBans(data);
    loadMessageBans(data);
    return true;
}

void LLMessageConfigFile::loadServerDefaults(const LLSD& data)
{
    mServerDefault = data["serverDefaults"][sServerName].asString();
}

void LLMessageConfigFile::loadMaxQueuedEvents(const LLSD& data)
{
     if (data.has("maxQueuedEvents"))
     {
          mMaxQueuedEvents = data["maxQueuedEvents"].asInteger();
     }
     else
     {
          mMaxQueuedEvents = DEFAULT_MAX_QUEUED_EVENTS;
     }
}

void LLMessageConfigFile::loadMessages(const LLSD& data)
{
    mMessages = data["messages"];

#ifdef DEBUG
    std::ostringstream out;
    LLSDXMLFormatter *formatter = new LLSDXMLFormatter;
    formatter->format(mMessages, out);
    LL_INFOS() << "loading ... " << out.str()
            << " LLMessageConfigFile::loadMessages loaded "
            << mMessages.size() << " messages" << LL_ENDL;
#endif
}

void LLMessageConfigFile::loadCapBans(const LLSD& data)
{
    LLSD bans = data["capBans"];
    if (!bans.isMap())
    {
        LL_INFOS("AppInit") << "LLMessageConfigFile::loadCapBans: missing capBans section"
            << LL_ENDL;
        return;
    }
    
    mCapBans = bans;
    
    LL_DEBUGS("AppInit") << "LLMessageConfigFile::loadCapBans: "
        << bans.size() << " ban tests" << LL_ENDL;
}

void LLMessageConfigFile::loadMessageBans(const LLSD& data)
{
    LLSD bans = data["messageBans"];
    if (!bans.isMap())
    {
        LL_INFOS("AppInit") << "LLMessageConfigFile::loadMessageBans: missing messageBans section"
            << LL_ENDL;
        return;
    }
    
    gMessageSystem->setMessageBans(bans["trusted"], bans["untrusted"]);
}

bool LLMessageConfigFile::isCapBanned(const std::string& cap_name) const
{
    LL_DEBUGS() << "mCapBans is " << LLSDNotationStreamer(mCapBans) << LL_ENDL;
    return mCapBans[cap_name];
}

//---------------------------------------------------------------
// LLMessageConfig
//---------------------------------------------------------------

//static
void LLMessageConfig::initClass(const std::string& server_name,
                                const std::string& config_dir)
{
    sServerName = server_name;
    sConfigDir = config_dir;
    (void) LLMessageConfigFile::instance();
    LL_DEBUGS("AppInit") << "LLMessageConfig::initClass config file "
            << config_dir << "/" << messageConfigFileName << LL_ENDL;
}

//static
void LLMessageConfig::useConfig(const LLSD& config)
{
    LLMessageConfigFile &the_file = LLMessageConfigFile::instance();
    the_file.loadServerDefaults(config);
    the_file.loadMaxQueuedEvents(config);
    the_file.loadMessages(config);
    the_file.loadCapBans(config);
    the_file.loadMessageBans(config);
}

//static
LLMessageConfig::Flavor LLMessageConfig::getServerDefaultFlavor()
{
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    if (file.mServerDefault == "llsd")
    {
        return LLSD_FLAVOR;
    }
    if (file.mServerDefault == "template")
    {
        return TEMPLATE_FLAVOR;
    }
    return NO_FLAVOR;
}

//static
S32 LLMessageConfig::getMaxQueuedEvents()
{
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    return file.mMaxQueuedEvents;
}

//static
LLMessageConfig::Flavor LLMessageConfig::getMessageFlavor(const std::string& msg_name)
{
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    LLSD config = file.mMessages[msg_name];
    if (config["flavor"].asString() == "llsd")
    {
        return LLSD_FLAVOR;
    }
    if (config["flavor"].asString() == "template")
    {
        return TEMPLATE_FLAVOR;
    }
    return NO_FLAVOR;
}

//static
LLMessageConfig::SenderTrust LLMessageConfig::getSenderTrustedness(
    const std::string& msg_name)
{
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    LLSD config = file.mMessages[msg_name];
    if (config.has("trusted-sender"))
    {
        return config["trusted-sender"].asBoolean() ? TRUSTED : UNTRUSTED;
    }
    return NOT_SET;
}

//static
bool LLMessageConfig::isValidMessage(const std::string& msg_name)
{
    if (sServerName.empty())
    {
        LL_ERRS() << "LLMessageConfig::initClass() not called" << LL_ENDL;
    }
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    return file.mMessages.has(msg_name);
}

//static
bool LLMessageConfig::onlySendLatest(const std::string& msg_name)
{
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    LLSD config = file.mMessages[msg_name];
    return config["only-send-latest"].asBoolean();
}

bool LLMessageConfig::isCapBanned(const std::string& cap_name)
{
    return LLMessageConfigFile::instance().isCapBanned(cap_name);
}

// return the web-service path to use for a given
// message. This entry *should* match the entry
// in simulator.xml!
LLSD LLMessageConfig::getConfigForMessage(const std::string& msg_name)
{
    if (sServerName.empty())
    {
        LL_ERRS() << "LLMessageConfig::isMessageTrusted(name) before"
                << " LLMessageConfig::initClass()" << LL_ENDL;
    }
    LLMessageConfigFile& file = LLMessageConfigFile::instance();
    // LLSD for the CamelCase message name
    LLSD config = file.mMessages[msg_name];
    return config;
}
