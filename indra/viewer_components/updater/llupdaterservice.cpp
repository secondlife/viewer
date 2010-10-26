/** 
 * @file llupdaterservice.cpp
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llupdaterservice.h"

#include "llsingleton.h"
#include "llpluginprocessparent.h"
#include <boost/scoped_ptr.hpp>

class LLUpdaterServiceImpl : public LLPluginProcessParentOwner, 
							 public LLSingleton<LLUpdaterServiceImpl>
{
	std::string mUrl;
	std::string mChannel;
	std::string mVersion;
	
	unsigned int mUpdateCheckPeriod;
	bool mIsChecking;
	boost::scoped_ptr<LLPluginProcessParent> mPlugin;

public:
	LLUpdaterServiceImpl();
	virtual ~LLUpdaterServiceImpl() {}

	// LLPluginProcessParentOwner interfaces
	virtual void receivePluginMessage(const LLPluginMessage &message);
	virtual bool receivePluginMessageEarly(const LLPluginMessage &message);
	virtual void pluginLaunchFailed();
	virtual void pluginDied();

	void setURL(const std::string& url);
	void setChannel(const std::string& channel);
	void setVersion(const std::string& version);
	void setUpdateCheckPeriod(unsigned int seconds);
	void startChecking();
	void stopChecking();
};

LLUpdaterServiceImpl::LLUpdaterServiceImpl() :
	mIsChecking(false),
	mUpdateCheckPeriod(0),
	mPlugin(0)
{
	// Create the plugin parent, this is the owner.
	mPlugin.reset(new LLPluginProcessParent(this));
}

// LLPluginProcessParentOwner interfaces
void LLUpdaterServiceImpl::receivePluginMessage(const LLPluginMessage &message)
{
}

bool LLUpdaterServiceImpl::receivePluginMessageEarly(const LLPluginMessage &message) 
{
	return false;
};

void LLUpdaterServiceImpl::pluginLaunchFailed() 
{
};

void LLUpdaterServiceImpl::pluginDied() 
{
};

void LLUpdaterServiceImpl::setURL(const std::string& url)
{
	if(mUrl != url)
	{
		mUrl = url;
	}
}

void LLUpdaterServiceImpl::setChannel(const std::string& channel)
{
	if(mChannel != channel)
	{
		mChannel = channel;
	}
}

void LLUpdaterServiceImpl::setVersion(const std::string& version)
{
	if(mVersion != version)
	{
		mVersion = version;
	}
}

void LLUpdaterServiceImpl::setUpdateCheckPeriod(unsigned int seconds)
{
	if(mUpdateCheckPeriod != seconds)
	{
		mUpdateCheckPeriod = seconds;
	}
}

void LLUpdaterServiceImpl::startChecking()
{
	if(!mIsChecking)
	{
		mIsChecking = true;
	}
}

void LLUpdaterServiceImpl::stopChecking()
{
	if(mIsChecking)
	{
		mIsChecking = false;
	}
}

//-----------------------------------------------------------------------
// Facade interface
LLUpdaterService::LLUpdaterService()
{
}

LLUpdaterService::~LLUpdaterService()
{
}

void LLUpdaterService::setURL(const std::string& url)
{
	LLUpdaterServiceImpl::getInstance()->setURL(url);
}

void LLUpdaterService::setChannel(const std::string& channel)
{
	LLUpdaterServiceImpl::getInstance()->setChannel(channel);
}

void LLUpdaterService::setVersion(const std::string& version)
{
	LLUpdaterServiceImpl::getInstance()->setVersion(version);
}
	
void LLUpdaterService::setUpdateCheckPeriod(unsigned int seconds)
{
	LLUpdaterServiceImpl::getInstance()->setUpdateCheckPeriod(seconds);
}
	
void LLUpdaterService::startChecking()
{
	LLUpdaterServiceImpl::getInstance()->startChecking();
}

void LLUpdaterService::stopChecking()
{
	LLUpdaterServiceImpl::getInstance()->stopChecking();
}
