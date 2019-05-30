/** 
 * @file llversioninfo.h
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

#ifndef LL_LLVERSIONINFO_H
#define LL_LLVERSIONINFO_H

#include "stdtypes.h"
#include "llsingleton.h"
#include <string>
#include <memory>

class LLEventMailDrop;
template <typename T>
class LLStoreListener;

///
/// This API provides version information for the viewer.  This
/// includes access to the major, minor, patch, and build integer
/// values, as well as human-readable string representations. All
/// viewer code that wants to query the current version should 
/// use this API.
///
class LLVersionInfo: public LLSingleton<LLVersionInfo>
{
	LLSINGLETON(LLVersionInfo);
	void initSingleton();
public:
	~LLVersionInfo();

	/// return the major version number as an integer
	S32 getMajor();

	/// return the minor version number as an integer
	S32 getMinor();

	/// return the patch version number as an integer
	S32 getPatch();

	/// return the build number as an integer
	S32 getBuild();

	/// return the full viewer version as a string like "2.0.0.200030"
	std::string getVersion();

	/// return the viewer version as a string like "2.0.0"
	std::string getShortVersion();

	/// return the viewer version and channel as a string
	/// like "Second Life Release 2.0.0.200030"
	std::string getChannelAndVersion();

	/// return the channel name, e.g. "Second Life"
	std::string getChannel();
	
    /// return the CMake build type
    std::string getBuildConfig();

	/// reset the channel name used by the viewer.
	void resetChannel(const std::string& channel);

    /// return the bit width of an address
    S32 getAddressSize() { return ADDRESS_SIZE; }

    typedef enum
    {
        TEST_VIEWER,
        PROJECT_VIEWER,
        BETA_VIEWER,
        RELEASE_VIEWER
    } ViewerMaturity;
    ViewerMaturity getViewerMaturity();

	/// get the release-notes URL, once it becomes available -- until then,
	/// return empty string
	std::string getReleaseNotes();

private:
	std::string version;
	std::string short_version;
	/// Storage of the channel name the viewer is using.
	//  The channel name is set by hardcoded constant, 
	//  or by calling resetChannel()
	std::string mWorkingChannelName;
	// Storage for the "version and channel" string.
	// This will get reset too.
	std::string mVersionChannel;
	std::string build_configuration;
	std::string mReleaseNotes;
	// Store unique_ptrs to the next couple things so we don't have to explain
	// to every consumer of this header file all the details of each.
	// mPump is the LLEventMailDrop on which we listen for SLVersionChecker to
	// post the release-notes URL from the Viewer Version Manager.
	std::unique_ptr<LLEventMailDrop> mPump;
	// mStore is an adapter that stores the release-notes URL in mReleaseNotes.
	std::unique_ptr<LLStoreListener<std::string>> mStore;
};

#endif
