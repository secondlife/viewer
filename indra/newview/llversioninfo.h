/** 
 * @file llversioninfo.h
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#ifndef LL_LLVERSIONINFO_H
#define LL_LLVERSIONINFO_H

#include <string>

///
/// This API provides version information for the viewer.  This
/// includes access to the major, minor, patch, and build integer
/// values, as well as human-readable string representations. All
/// viewer code that wants to query the current version should 
/// use this API.
///
class LLVersionInfo
{
public:
	/// return the major verion number as an integer
	static S32 getMajor();

	/// return the minor verion number as an integer
	static S32 getMinor();

	/// return the patch verion number as an integer
	static S32 getPatch();

	/// return the build number as an integer
	static S32 getBuild();

	/// return the full viewer version as a string like "2.0.0.200030"
	static const std::string &getVersion();

	/// return the viewer version as a string like "2.0.0"
	static const std::string &getShortVersion();

	/// return the channel name, e.g. "Second Life"
	static const std::string &getChannel();
};

#endif
