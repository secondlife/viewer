/** 
 * @file llmessageconfig.cpp
 * @brief Live file handling for messaging
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
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
	LLMessageConfigFile()
        : LLLiveFile(filename(), messageConfigRefreshRate)
            { }

    static std::string filename();

    LLSD mMessages;
	std::string mServerDefault;
	
	static LLMessageConfigFile& instance();
		// return the singleton configuration file

	/* virtual */ void loadFile();
	void loadServerDefaults(const LLSD& data);
	void loadMessages(const LLSD& data);
	void loadCapBans(const LLSD& blacklist);
	void loadMessageBans(const LLSD& blacklist);
	bool isCapBanned(const std::string& cap_name) const;

public:
	LLSD mCapBans;
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
void LLMessageConfigFile::loadFile()
{
	LLSD data;
    {
        llifstream file(filename().c_str());
        
        if (file.is_open())
        {
			llinfos << "Loading message.xml file at " << filename() << llendl;
            LLSDSerialize::fromXML(data, file);
        }

        if (data.isUndefined())
        {
            llinfos << "LLMessageConfigFile::loadFile: file missing,"
				" ill-formed, or simply undefined; not changing the"
				" file" << llendl;
            return;
        }
    }
	loadServerDefaults(data);
	loadMessages(data);
	loadCapBans(data);
	loadMessageBans(data);
}

void LLMessageConfigFile::loadServerDefaults(const LLSD& data)
{
	mServerDefault = data["serverDefaults"][sServerName].asString();
}

void LLMessageConfigFile::loadMessages(const LLSD& data)
{
	mMessages = data["messages"];

#ifdef DEBUG
	std::ostringstream out;
	LLSDXMLFormatter *formatter = new LLSDXMLFormatter;
	formatter->format(mMessages, out);
	llinfos << "loading ... " << out.str()
			<< " LLMessageConfigFile::loadMessages loaded "
			<< mMessages.size() << " messages" << llendl;
#endif
}

void LLMessageConfigFile::loadCapBans(const LLSD& data)
{
    LLSD bans = data["capBans"];
    if (!bans.isMap())
    {
        llinfos << "LLMessageConfigFile::loadCapBans: missing capBans section"
            << llendl;
        return;
    }
    
	mCapBans = bans;
    
    llinfos << "LLMessageConfigFile::loadCapBans: "
        << bans.size() << " ban tests" << llendl;
}

void LLMessageConfigFile::loadMessageBans(const LLSD& data)
{
    LLSD bans = data["messageBans"];
    if (!bans.isMap())
    {
        llinfos << "LLMessageConfigFile::loadMessageBans: missing messageBans section"
            << llendl;
        return;
    }
    
	gMessageSystem->setMessageBans(bans["trusted"], bans["untrusted"]);
}

bool LLMessageConfigFile::isCapBanned(const std::string& cap_name) const
{
	lldebugs << "mCapBans is " << LLSDNotationStreamer(mCapBans) << llendl;
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
	llinfos << "LLMessageConfig::initClass config file "
			<< config_dir << "/" << messageConfigFileName << llendl;
}

//static
void LLMessageConfig::useConfig(const LLSD& config)
{
	LLMessageConfigFile &the_file = LLMessageConfigFile::instance();
	the_file.loadServerDefaults(config);
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
		llerrs << "LLMessageConfig::initClass() not called" << llendl;
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
		llerrs << "LLMessageConfig::isMessageTrusted(name) before"
				<< " LLMessageConfig::initClass()" << llendl;
	}
	LLMessageConfigFile& file = LLMessageConfigFile::instance();
	// LLSD for the CamelCase message name
	LLSD config = file.mMessages[msg_name];
	return config;
}
