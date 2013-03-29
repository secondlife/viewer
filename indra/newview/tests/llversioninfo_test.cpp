/** 
 * @file llversioninfo_test.cpp
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

#include "../test/lltut.h"

#include "../llversioninfo.h"
#include "llversionviewer.h"

namespace tut
{
    struct versioninfo
    {
		versioninfo()
			: mResetChannel("Reset Channel")
		{
			std::ostringstream stream;
			stream << LL_VERSION_MAJOR << "."
				   << LL_VERSION_MINOR << "."
				   << LL_VERSION_PATCH << "."
				   << LL_VERSION_BUILD;
			mVersion = stream.str();
			stream.str("");

			stream << LL_VERSION_MAJOR << "."
				   << LL_VERSION_MINOR << "."
				   << LL_VERSION_PATCH;
			mShortVersion = stream.str();
			stream.str("");

			stream << LL_CHANNEL
				   << " "
				   << mVersion;
			mVersionAndChannel = stream.str();
			stream.str("");

			stream << mResetChannel
				   << " "
				   << mVersion;
			mResetVersionAndChannel = stream.str();
		}
		std::string mResetChannel;
		std::string mVersion;
		std::string mShortVersion;
		std::string mVersionAndChannel;
		std::string mResetVersionAndChannel;
    };
    
	typedef test_group<versioninfo> versioninfo_t;
	typedef versioninfo_t::object versioninfo_object_t;
	tut::versioninfo_t tut_versioninfo("LLVersionInfo");

	template<> template<>
	void versioninfo_object_t::test<1>()
	{
		ensure_equals("Major version", 
					  LLVersionInfo::getMajor(), 
					  LL_VERSION_MAJOR);
		ensure_equals("Minor version", 
					  LLVersionInfo::getMinor(), 
					  LL_VERSION_MINOR);
		ensure_equals("Patch version", 
					  LLVersionInfo::getPatch(), 
					  LL_VERSION_PATCH);
		ensure_equals("Build version", 
					  LLVersionInfo::getBuild(), 
					  LL_VERSION_BUILD);
		ensure_equals("Channel version", 
					  LLVersionInfo::getChannel(), 
					  LL_CHANNEL);

		ensure_equals("Version String", 
					  LLVersionInfo::getVersion(), 
					  mVersion);
		ensure_equals("Short Version String", 
					  LLVersionInfo::getShortVersion(), 
					  mShortVersion);
		ensure_equals("Version and channel String", 
					  LLVersionInfo::getChannelAndVersion(), 
					  mVersionAndChannel);

		LLVersionInfo::resetChannel(mResetChannel);
		ensure_equals("Reset channel version", 
					  LLVersionInfo::getChannel(), 
					  mResetChannel);

		ensure_equals("Reset Version and channel String", 
					  LLVersionInfo::getChannelAndVersion(), 
					  mResetVersionAndChannel);
	}
}
