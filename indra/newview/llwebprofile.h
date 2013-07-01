/** 
 * @file llwebprofile.h
 * @brief Web profile access.
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLWEBPROFILE_H
#define LL_LLWEBPROFILE_H

#include "llimage.h"

namespace LLWebProfileResponders
{
    class ConfigResponder;
    class PostImageResponder;
    class PostImageRedirectResponder;
};

/**
 * @class LLWebProfile
 *
 * Manages interaction with, a web service allowing the upload of snapshot images
 * taken within the viewer.
 */
class LLWebProfile
{
	LOG_CLASS(LLWebProfile);

public:
	typedef boost::function<void(bool ok)> status_callback_t;

	static void uploadImage(LLPointer<LLImageFormatted> image, const std::string& caption, bool add_location);
	static void setAuthCookie(const std::string& cookie);
	static void setImageUploadResultCallback(status_callback_t cb) { mStatusCallback = cb; }

private:
	friend class LLWebProfileResponders::ConfigResponder;
	friend class LLWebProfileResponders::PostImageResponder;
	friend class LLWebProfileResponders::PostImageRedirectResponder;

	static void post(LLPointer<LLImageFormatted> image, const LLSD& config, const std::string& url);
	static void reportImageUploadStatus(bool ok);
	static std::string getAuthCookie();

	static std::string sAuthCookie;
	static status_callback_t mStatusCallback;
};

#endif // LL_LLWEBPROFILE_H
