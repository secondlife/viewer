/** 
 * @file llversioninfo.cpp
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llevents.h"
#include "lleventfilter.h"
#include "llversioninfo.h"
#include "stringize.h"
#include <boost/regex.hpp>

#if ! defined(LL_VIEWER_CHANNEL)       \
 || ! defined(LL_VIEWER_VERSION_MAJOR) \
 || ! defined(LL_VIEWER_VERSION_MINOR) \
 || ! defined(LL_VIEWER_VERSION_PATCH) \
 || ! defined(LL_VIEWER_VERSION_BUILD)
 #error "Channel or Version information is undefined"
#endif

//
// Set the version numbers in indra/VIEWER_VERSION
//

LLVersionInfo::LLVersionInfo():
	short_version(STRINGIZE(LL_VIEWER_VERSION_MAJOR << "."
							<< LL_VIEWER_VERSION_MINOR << "."
							<< LL_VIEWER_VERSION_PATCH)),
	// LL_VIEWER_CHANNEL is a macro defined on the compiler command line. The
	// macro expands to the string name of the channel, but without quotes. We
	// need to turn it into a quoted string. LL_TO_STRING() does that.
	mWorkingChannelName(LL_TO_STRING(LL_VIEWER_CHANNEL)),
	build_configuration(LLBUILD_CONFIG), // set in indra/cmake/BuildVersion.cmake
	// instantiate an LLEventMailDrop with canonical name to listen for news
	// from SLVersionChecker
	mPump{new LLEventMailDrop("relnotes")},
	// immediately listen on mPump, store arriving URL into mReleaseNotes
	mStore{new LLStoreListener<std::string>(*mPump, mReleaseNotes)}
{
}

void LLVersionInfo::initSingleton()
{
	// We override initSingleton() not because we have dependencies on other
	// LLSingletons, but because certain initializations call other member
	// functions. We should refrain from calling methods until this object is
	// fully constructed; such calls don't really belong in the constructor.

	// cache the version string
	version = STRINGIZE(getShortVersion() << "." << getBuild());
}

LLVersionInfo::~LLVersionInfo()
{
}

S32 LLVersionInfo::getMajor()
{
	return LL_VIEWER_VERSION_MAJOR;
}

S32 LLVersionInfo::getMinor()
{
	return LL_VIEWER_VERSION_MINOR;
}

S32 LLVersionInfo::getPatch()
{
	return LL_VIEWER_VERSION_PATCH;
}

S32 LLVersionInfo::getBuild()
{
	return LL_VIEWER_VERSION_BUILD;
}

std::string LLVersionInfo::getVersion()
{
	return version;
}

std::string LLVersionInfo::getShortVersion()
{
	return short_version;
}

std::string LLVersionInfo::getChannelAndVersion()
{
	if (mVersionChannel.empty())
	{
		// cache the version string
		mVersionChannel = getChannel() + " " + getVersion();
	}

	return mVersionChannel;
}

std::string LLVersionInfo::getChannel()
{
	return mWorkingChannelName;
}

void LLVersionInfo::resetChannel(const std::string& channel)
{
	mWorkingChannelName = channel;
	mVersionChannel.clear(); // Reset version and channel string til next use.
}

LLVersionInfo::ViewerMaturity LLVersionInfo::getViewerMaturity()
{
    ViewerMaturity maturity;
    
    std::string channel = getChannel();

	static const boost::regex is_test_channel("\\bTest\\b");
	static const boost::regex is_beta_channel("\\bBeta\\b");
	static const boost::regex is_project_channel("\\bProject\\b");
	static const boost::regex is_release_channel("\\bRelease\\b");

    if (boost::regex_search(channel, is_release_channel))
    {
        maturity = RELEASE_VIEWER;
    }
    else if (boost::regex_search(channel, is_beta_channel))
    {
        maturity = BETA_VIEWER;
    }
    else if (boost::regex_search(channel, is_project_channel))
    {
        maturity = PROJECT_VIEWER;
    }
    else if (boost::regex_search(channel, is_test_channel))
    {
        maturity = TEST_VIEWER;
    }
    else
    {
        LL_WARNS() << "Channel '" << channel
                   << "' does not follow naming convention, assuming Test"
                   << LL_ENDL;
        maturity = TEST_VIEWER;
    }
    return maturity;
}

    
std::string LLVersionInfo::getBuildConfig()
{
    return build_configuration;
}

std::string LLVersionInfo::getReleaseNotes()
{
    return mReleaseNotes;
}
