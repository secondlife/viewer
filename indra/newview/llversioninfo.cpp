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
#include "llversioninfo.h"

#include "llversionviewer.h"

//
// Set the version numbers in indra/llcommon/llversionviewer.h
//

//static
S32 LLVersionInfo::getMajor()
{
	return LL_VERSION_MAJOR;
}

//static
S32 LLVersionInfo::getMinor()
{
	return LL_VERSION_MINOR;
}

//static
S32 LLVersionInfo::getPatch()
{
	return LL_VERSION_PATCH;
}

//static
S32 LLVersionInfo::getBuild()
{
	return LL_VERSION_BUILD;
}

//static
const std::string &LLVersionInfo::getVersion()
{
	static std::string version("");

	if (version.empty())
	{
		// cache the version string
		std::ostringstream stream;
		stream << LL_VERSION_MAJOR << "."
		       << LL_VERSION_MINOR << "."
		       << LL_VERSION_PATCH << "."
		       << LL_VERSION_BUILD;
		version = stream.str();
	}

	return version;
}

//static
const std::string &LLVersionInfo::getShortVersion()
{
	static std::string version("");

	if (version.empty())
	{
		// cache the version string
		std::ostringstream stream;
		stream << LL_VERSION_MAJOR << "."
		       << LL_VERSION_MINOR << "."
		       << LL_VERSION_PATCH;
		version = stream.str();
	}

	return version;
}

namespace
{
	/// Storage of the channel name the viewer is using.
	//  The channel name is set by hardcoded constant, 
	//  or by calling LLVersionInfo::resetChannel()
	std::string sWorkingChannelName(LL_CHANNEL);

	// Storage for the "version and channel" string.
	// This will get reset too.
	std::string sVersionChannel("");
}

//static
const std::string &LLVersionInfo::getVersionAndChannel()
{
	if (sVersionChannel.empty())
	{
		// cache the version string
		std::ostringstream stream;
		stream << LLVersionInfo::getVersion() 
			   << " "
			   << LLVersionInfo::getChannel();
		sVersionChannel = stream.str();
	}

	return sVersionChannel;
}

//static
const std::string &LLVersionInfo::getChannel()
{
	return sWorkingChannelName;
}

void LLVersionInfo::resetChannel(const std::string& channel)
{
	sWorkingChannelName = channel;
	sVersionChannel.clear(); // Reset version and channel string til next use.
}
