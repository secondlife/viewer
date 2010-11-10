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

#include "llupdatedownloader.h"
#include "llevents.h"
#include "lltimer.h"
#include "llupdaterservice.h"
#include "llupdatechecker.h"

#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include "lldir.h"
#include "llsdserialize.h"
#include "llfile.h"

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif


namespace 
{
	boost::weak_ptr<LLUpdaterServiceImpl> gUpdater;

	const std::string UPDATE_MARKER_FILENAME("SecondLifeUpdateReady.xml");
	std::string update_marker_path()
	{
		return gDirUtilp->getExpandedFilename(LL_PATH_LOGS, 
											  UPDATE_MARKER_FILENAME);
	}
}

class LLUpdaterServiceImpl : 
	public LLUpdateChecker::Client,
	public LLUpdateDownloader::Client
{
	static const std::string sListenerName;
	
	std::string mProtocolVersion;
	std::string mUrl;
	std::string mPath;
	std::string mChannel;
	std::string mVersion;
	
	unsigned int mCheckPeriod;
	bool mIsChecking;
	
	LLUpdateChecker mUpdateChecker;
	LLUpdateDownloader mUpdateDownloader;
	LLTimer mTimer;

	LLUpdaterService::app_exit_callback_t mAppExitCallback;
	
	LOG_CLASS(LLUpdaterServiceImpl);
	
public:
	LLUpdaterServiceImpl();
	virtual ~LLUpdaterServiceImpl();

	void initialize(const std::string& protocol_version,
				   const std::string& url, 
				   const std::string& path,
				   const std::string& channel,
				   const std::string& version);
	
	void setCheckPeriod(unsigned int seconds);

	void startChecking();
	void stopChecking();
	bool isChecking();
	
	void setAppExitCallback(LLUpdaterService::app_exit_callback_t aecb) { mAppExitCallback = aecb;}

	bool checkForInstall(); // Test if a local install is ready.
	bool checkForResume(); // Test for resumeable d/l.

	// LLUpdateChecker::Client:
	virtual void error(std::string const & message);
	virtual void optionalUpdate(std::string const & newVersion,
								LLURI const & uri,
								std::string const & hash);
	virtual void requiredUpdate(std::string const & newVersion,
								LLURI const & uri,
								std::string const & hash);
	virtual void upToDate(void);
	
	// LLUpdateDownloader::Client
	void downloadComplete(LLSD const & data);
	void downloadError(std::string const & message);

	bool onMainLoop(LLSD const & event);	

private:
	void retry(void);

};

const std::string LLUpdaterServiceImpl::sListenerName = "LLUpdaterServiceImpl";

LLUpdaterServiceImpl::LLUpdaterServiceImpl() :
	mIsChecking(false),
	mCheckPeriod(0),
	mUpdateChecker(*this),
	mUpdateDownloader(*this)
{
}

LLUpdaterServiceImpl::~LLUpdaterServiceImpl()
{
	LL_INFOS("UpdaterService") << "shutting down updater service" << LL_ENDL;
	LLEventPumps::instance().obtain("mainloop").stopListening(sListenerName);
}

void LLUpdaterServiceImpl::initialize(const std::string& protocol_version,
									  const std::string& url, 
									 const std::string& path,
									 const std::string& channel,
									 const std::string& version)
{
	if(mIsChecking)
	{
		throw LLUpdaterService::UsageError("Call LLUpdaterService::stopCheck()"
			" before setting params.");
	}
		
	mProtocolVersion = protocol_version;
	mUrl = url;
	mPath = path;
	mChannel = channel;
	mVersion = version;

	// Check to see if an install is ready.
	if(!checkForInstall())
	{
		checkForResume();
	}	
}

void LLUpdaterServiceImpl::setCheckPeriod(unsigned int seconds)
{
	mCheckPeriod = seconds;
}

void LLUpdaterServiceImpl::startChecking()
{
	if(!mIsChecking)
	{
		if(mUrl.empty() || mChannel.empty() || mVersion.empty())
		{
			throw LLUpdaterService::UsageError("Set params before call to "
				"LLUpdaterService::startCheck().");
		}
		mIsChecking = true;
	
		mUpdateChecker.check(mProtocolVersion, mUrl, mPath, mChannel, mVersion);
	}
}

void LLUpdaterServiceImpl::stopChecking()
{
	if(mIsChecking)
	{
		mIsChecking = false;
	}
}

bool LLUpdaterServiceImpl::isChecking()
{
	return mIsChecking;
}

bool LLUpdaterServiceImpl::checkForInstall()
{
	bool result = false; // return true if install is found.

	llifstream update_marker(update_marker_path(), 
							 std::ios::in | std::ios::binary);

	if(update_marker.is_open())
	{
		// Found an update info - now lets see if its valid.
		LLSD update_info;
		LLSDSerialize::fromXMLDocument(update_info, update_marker);

		// Get the path to the installer file.
		LLSD path = update_info.get("path");
		if(path.isDefined() && !path.asString().empty())
		{
			// install!
		}

		update_marker.close();
		LLFile::remove(update_marker_path());
		result = true;
	}
	return result;
}

bool LLUpdaterServiceImpl::checkForResume()
{
	bool result = false;
	llstat stat_info;
	if(0 == LLFile::stat(mUpdateDownloader.downloadMarkerPath(), &stat_info))
	{
		mUpdateDownloader.resume();
		result = true;
	}
	return false;
}

void LLUpdaterServiceImpl::error(std::string const & message)
{
	retry();
}

void LLUpdaterServiceImpl::optionalUpdate(std::string const & newVersion,
										  LLURI const & uri,
										  std::string const & hash)
{
	mUpdateDownloader.download(uri, hash);
}

void LLUpdaterServiceImpl::requiredUpdate(std::string const & newVersion,
										  LLURI const & uri,
										  std::string const & hash)
{
	mUpdateDownloader.download(uri, hash);
}

void LLUpdaterServiceImpl::upToDate(void)
{
	retry();
}

void LLUpdaterServiceImpl::downloadComplete(LLSD const & data) 
{ 
	// Save out the download data to the SecondLifeUpdateReady
	// marker file.
	llofstream update_marker(update_marker_path());
	LLSDSerialize::toPrettyXML(data, update_marker);

	// Stop checking.
	stopChecking();

	// Wait for restart...?
}

void LLUpdaterServiceImpl::downloadError(std::string const & message) 
{ 
	retry(); 
}

void LLUpdaterServiceImpl::retry(void)
{
	LL_INFOS("UpdaterService") << "will check for update again in " << 
	mCheckPeriod << " seconds" << LL_ENDL; 
	mTimer.start();
	mTimer.setTimerExpirySec(mCheckPeriod);
	LLEventPumps::instance().obtain("mainloop").listen(
		sListenerName, boost::bind(&LLUpdaterServiceImpl::onMainLoop, this, _1));
}

bool LLUpdaterServiceImpl::onMainLoop(LLSD const & event)
{
	if(mTimer.hasExpired())
	{
		mTimer.stop();
		LLEventPumps::instance().obtain("mainloop").stopListening(sListenerName);
		mUpdateChecker.check(mProtocolVersion, mUrl, mPath, mChannel, mVersion);
	} else {
		// Keep on waiting...
	}
	
	return false;
}


//-----------------------------------------------------------------------
// Facade interface
LLUpdaterService::LLUpdaterService()
{
	if(gUpdater.expired())
	{
		mImpl = 
			boost::shared_ptr<LLUpdaterServiceImpl>(new LLUpdaterServiceImpl());
		gUpdater = mImpl;
	}
	else
	{
		mImpl = gUpdater.lock();
	}
}

LLUpdaterService::~LLUpdaterService()
{
}

void LLUpdaterService::initialize(const std::string& protocol_version,
								 const std::string& url, 
								 const std::string& path,
								 const std::string& channel,
								 const std::string& version)
{
	mImpl->initialize(protocol_version, url, path, channel, version);
}

void LLUpdaterService::setCheckPeriod(unsigned int seconds)
{
	mImpl->setCheckPeriod(seconds);
}
	
void LLUpdaterService::startChecking()
{
	mImpl->startChecking();
}

void LLUpdaterService::stopChecking()
{
	mImpl->stopChecking();
}

bool LLUpdaterService::isChecking()
{
	return mImpl->isChecking();
}

void LLUpdaterService::setImplAppExitCallback(LLUpdaterService::app_exit_callback_t aecb)
{
	return mImpl->setAppExitCallback(aecb);
}
