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

#include "llupdatedownloader.h"
#include "llevents.h"
#include "lltimer.h"
#include "llupdatechecker.h"
#include "llupdateinstaller.h"
#include "llversionviewer.h"

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
	
	std::string install_script_path(void)
	{
#ifdef LL_WINDOWS
		std::string scriptFile = "update_install.bat";
#else
		std::string scriptFile = "update_install";
#endif
		return gDirUtilp->getExpandedFilename(LL_PATH_EXECUTABLE, scriptFile);
	}
	
	LLInstallScriptMode install_script_mode(void) 
	{
#ifdef LL_WINDOWS
		return LL_COPY_INSTALL_SCRIPT_TO_TEMP;
#else
		return LL_RUN_INSTALL_SCRIPT_IN_PLACE;
#endif
	};
	
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
	bool mIsDownloading;
	
	LLUpdateChecker mUpdateChecker;
	LLUpdateDownloader mUpdateDownloader;
	LLTimer mTimer;

	LLUpdaterService::app_exit_callback_t mAppExitCallback;
	
	LLUpdaterService::eUpdaterState mState;
	
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
	void setBandwidthLimit(U64 bytesPerSecond);

	void startChecking(bool install_if_ready);
	void stopChecking();
	bool isChecking();
	LLUpdaterService::eUpdaterState getState();
	
	void setAppExitCallback(LLUpdaterService::app_exit_callback_t aecb) { mAppExitCallback = aecb;}
	std::string updatedVersion(void);

	bool checkForInstall(bool launchInstaller); // Test if a local install is ready.
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
	std::string mNewVersion;
	
	void restartTimer(unsigned int seconds);
	void setState(LLUpdaterService::eUpdaterState state);
	void stopTimer();
};

const std::string LLUpdaterServiceImpl::sListenerName = "LLUpdaterServiceImpl";

LLUpdaterServiceImpl::LLUpdaterServiceImpl() :
	mIsChecking(false),
	mIsDownloading(false),
	mCheckPeriod(0),
	mUpdateChecker(*this),
	mUpdateDownloader(*this),
	mState(LLUpdaterService::INITIAL)
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
	if(mIsChecking || mIsDownloading)
	{
		throw LLUpdaterService::UsageError("LLUpdaterService::initialize call "
										   "while updater is running.");
	}
		
	mProtocolVersion = protocol_version;
	mUrl = url;
	mPath = path;
	mChannel = channel;
	mVersion = version;
}

void LLUpdaterServiceImpl::setCheckPeriod(unsigned int seconds)
{
	mCheckPeriod = seconds;
}

void LLUpdaterServiceImpl::setBandwidthLimit(U64 bytesPerSecond)
{
	mUpdateDownloader.setBandwidthLimit(bytesPerSecond);
}

void LLUpdaterServiceImpl::startChecking(bool install_if_ready)
{
	if(mUrl.empty() || mChannel.empty() || mVersion.empty())
	{
		throw LLUpdaterService::UsageError("Set params before call to "
			"LLUpdaterService::startCheck().");
	}

	mIsChecking = true;

    // Check to see if an install is ready.
	bool has_install = checkForInstall(install_if_ready);
	if(!has_install)
	{
		checkForResume(); // will set mIsDownloading to true if resuming

		if(!mIsDownloading)
		{
			setState(LLUpdaterService::CHECKING_FOR_UPDATE);
			
			// Checking can only occur during the mainloop.
			// reset the timer to 0 so that the next mainloop event 
			// triggers a check;
			restartTimer(0); 
		} 
		else
		{
			setState(LLUpdaterService::DOWNLOADING);
		}
	}
}

void LLUpdaterServiceImpl::stopChecking()
{
	if(mIsChecking)
	{
		mIsChecking = false;
		stopTimer();
	}

    if(mIsDownloading)
    {
        mUpdateDownloader.cancel();
		mIsDownloading = false;
    }
	
	setState(LLUpdaterService::TERMINAL);
}

bool LLUpdaterServiceImpl::isChecking()
{
	return mIsChecking;
}

LLUpdaterService::eUpdaterState LLUpdaterServiceImpl::getState()
{
	return mState;
}

std::string LLUpdaterServiceImpl::updatedVersion(void)
{
	return mNewVersion;
}

bool LLUpdaterServiceImpl::checkForInstall(bool launchInstaller)
{
	bool foundInstall = false; // return true if install is found.

	llifstream update_marker(update_marker_path(), 
							 std::ios::in | std::ios::binary);

	if(update_marker.is_open())
	{
		// Found an update info - now lets see if its valid.
		LLSD update_info;
		LLSDSerialize::fromXMLDocument(update_info, update_marker);
		update_marker.close();

		// Get the path to the installer file.
		LLSD path = update_info.get("path");
		if(update_info["current_version"].asString() != ll_get_version())
		{
			// This viewer is not the same version as the one that downloaded
			// the update.  Do not install this update.
			if(!path.asString().empty())
			{
				llinfos << "ignoring update dowloaded by different client version" << llendl;
				LLFile::remove(path.asString());
				LLFile::remove(update_marker_path());
			}
			else
			{
				; // Nothing to clean up.
			}
			
			foundInstall = false;
		} 
		else if(path.isDefined() && !path.asString().empty())
		{
			if(launchInstaller)
			{
				setState(LLUpdaterService::INSTALLING);
				
				LLFile::remove(update_marker_path());

				int result = ll_install_update(install_script_path(),
											   update_info["path"].asString(),
											   update_info["required"].asBoolean(),
											   install_script_mode());	
				
				if((result == 0) && mAppExitCallback)
				{
					mAppExitCallback();
				} else if(result != 0) {
					llwarns << "failed to run update install script" << LL_ENDL;
				} else {
					; // No op.
				}
			}
			
			foundInstall = true;
		}
	}
	return foundInstall;
}

bool LLUpdaterServiceImpl::checkForResume()
{
	bool result = false;
	std::string download_marker_path = mUpdateDownloader.downloadMarkerPath();
	if(LLFile::isfile(download_marker_path))
	{
		llifstream download_marker_stream(download_marker_path, 
								 std::ios::in | std::ios::binary);
		if(download_marker_stream.is_open())
		{
			LLSD download_info;
			LLSDSerialize::fromXMLDocument(download_info, download_marker_stream);
			download_marker_stream.close();
			if(download_info["current_version"].asString() == ll_get_version())
			{
				mIsDownloading = true;
				mNewVersion = download_info["update_version"].asString();
				mUpdateDownloader.resume();
				result = true;
			}
			else 
			{
				// The viewer that started this download is not the same as this viewer; ignore.
				llinfos << "ignoring partial download from different viewer version" << llendl;
				std::string path = download_info["path"].asString();
				if(!path.empty()) LLFile::remove(path);
				LLFile::remove(download_marker_path);
			}
		} 
	}
	return result;
}

void LLUpdaterServiceImpl::error(std::string const & message)
{
	if(mIsChecking)
	{
		setState(LLUpdaterService::TEMPORARY_ERROR);
		restartTimer(mCheckPeriod);
	}
}

void LLUpdaterServiceImpl::optionalUpdate(std::string const & newVersion,
										  LLURI const & uri,
										  std::string const & hash)
{
	stopTimer();
	mNewVersion = newVersion;
	mIsDownloading = true;
	setState(LLUpdaterService::DOWNLOADING);
	mUpdateDownloader.download(uri, hash, newVersion, false);
}

void LLUpdaterServiceImpl::requiredUpdate(std::string const & newVersion,
										  LLURI const & uri,
										  std::string const & hash)
{
	stopTimer();
	mNewVersion = newVersion;
	mIsDownloading = true;
	setState(LLUpdaterService::DOWNLOADING);
	mUpdateDownloader.download(uri, hash, newVersion, true);
}

void LLUpdaterServiceImpl::upToDate(void)
{
	if(mIsChecking)
	{
		restartTimer(mCheckPeriod);
	}
	
	setState(LLUpdaterService::UP_TO_DATE);
}

void LLUpdaterServiceImpl::downloadComplete(LLSD const & data) 
{ 
	mIsDownloading = false;

	// Save out the download data to the SecondLifeUpdateReady
	// marker file. 
	llofstream update_marker(update_marker_path());
	LLSDSerialize::toPrettyXML(data, update_marker);
	
	LLSD event;
	event["pump"] = LLUpdaterService::pumpName();
	LLSD payload;
	payload["type"] = LLSD(LLUpdaterService::DOWNLOAD_COMPLETE);
	payload["required"] = data["required"];
	payload["version"] = mNewVersion;
	event["payload"] = payload;
	LLEventPumps::instance().obtain("mainlooprepeater").post(event);
	
	setState(LLUpdaterService::TERMINAL);
}

void LLUpdaterServiceImpl::downloadError(std::string const & message) 
{ 
	LL_INFOS("UpdaterService") << "Error downloading: " << message << LL_ENDL;

	mIsDownloading = false;

	// Restart the timer on error
	if(mIsChecking)
	{
		restartTimer(mCheckPeriod); 
	}

	LLSD event;
	event["pump"] = LLUpdaterService::pumpName();
	LLSD payload;
	payload["type"] = LLSD(LLUpdaterService::DOWNLOAD_ERROR);
	payload["message"] = message;
	event["payload"] = payload;
	LLEventPumps::instance().obtain("mainlooprepeater").post(event);

	setState(LLUpdaterService::FAILURE);
}

void LLUpdaterServiceImpl::restartTimer(unsigned int seconds)
{
	LL_INFOS("UpdaterService") << "will check for update again in " << 
	seconds << " seconds" << LL_ENDL; 
	mTimer.start();
	mTimer.setTimerExpirySec((F32)seconds);
	LLEventPumps::instance().obtain("mainloop").listen(
		sListenerName, boost::bind(&LLUpdaterServiceImpl::onMainLoop, this, _1));
}

void LLUpdaterServiceImpl::setState(LLUpdaterService::eUpdaterState state)
{
	if(state != mState)
	{
		mState = state;
		
		LLSD event;
		event["pump"] = LLUpdaterService::pumpName();
		LLSD payload;
		payload["type"] = LLSD(LLUpdaterService::STATE_CHANGE);
		payload["state"] = state;
		event["payload"] = payload;
		LLEventPumps::instance().obtain("mainlooprepeater").post(event);
		
		LL_INFOS("UpdaterService") << "setting state to " << state << LL_ENDL;
	}
	else 
	{
		; // State unchanged; noop.
	}
}

void LLUpdaterServiceImpl::stopTimer()
{
	mTimer.stop();
	LLEventPumps::instance().obtain("mainloop").stopListening(sListenerName);
}

bool LLUpdaterServiceImpl::onMainLoop(LLSD const & event)
{
	if(mTimer.getStarted() && mTimer.hasExpired())
	{
		stopTimer();

		// Check for failed install.
		if(LLFile::isfile(ll_install_failed_marker_path()))
		{
			int requiredValue = 0; 
			{
				llifstream stream(ll_install_failed_marker_path());
				stream >> requiredValue;
				if(stream.fail()) requiredValue = 0;
			}
			// TODO: notify the user.
			llinfos << "found marker " << ll_install_failed_marker_path() << llendl;
			llinfos << "last install attempt failed" << llendl;
			LLFile::remove(ll_install_failed_marker_path());
			
			LLSD event;
			event["type"] = LLSD(LLUpdaterService::INSTALL_ERROR);
			event["required"] = LLSD(requiredValue);
			LLEventPumps::instance().obtain(LLUpdaterService::pumpName()).post(event);
			
			setState(LLUpdaterService::TERMINAL);
		}
		else
		{
			mUpdateChecker.checkVersion(mProtocolVersion, mUrl, mPath, mChannel, mVersion);
			setState(LLUpdaterService::CHECKING_FOR_UPDATE);
		}
	} 
	else 
	{
		// Keep on waiting...
	}
	
	return false;
}


//-----------------------------------------------------------------------
// Facade interface

std::string const & LLUpdaterService::pumpName(void)
{
	static std::string name("updater_service");
	return name;
}

bool LLUpdaterService::updateReadyToInstall(void)
{
	return LLFile::isfile(update_marker_path());
}

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

void LLUpdaterService::setBandwidthLimit(U64 bytesPerSecond)
{
	mImpl->setBandwidthLimit(bytesPerSecond);
}
	
void LLUpdaterService::startChecking(bool install_if_ready)
{
	mImpl->startChecking(install_if_ready);
}

void LLUpdaterService::stopChecking()
{
	mImpl->stopChecking();
}

bool LLUpdaterService::isChecking()
{
	return mImpl->isChecking();
}

LLUpdaterService::eUpdaterState LLUpdaterService::getState()
{
	return mImpl->getState();
}

void LLUpdaterService::setImplAppExitCallback(LLUpdaterService::app_exit_callback_t aecb)
{
	return mImpl->setAppExitCallback(aecb);
}

std::string LLUpdaterService::updatedVersion(void)
{
	return mImpl->updatedVersion();
}


std::string const & ll_get_version(void) {
	static std::string version("");
	
	if (version.empty()) {
		std::ostringstream stream;
		stream << LL_VERSION_MAJOR << "."
		<< LL_VERSION_MINOR << "."
		<< LL_VERSION_PATCH << "."
		<< LL_VERSION_BUILD;
		version = stream.str();
	}
	
	return version;
}

