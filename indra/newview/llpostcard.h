/** 
 * @file llpostcard.h
 * @brief Sending postcards.
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

#ifndef LL_LLPOSTCARD_H
#define LL_LLPOSTCARD_H

#include "llimage.h"
#include "lluuid.h"

class LLPostCard
{
	LOG_CLASS(LLPostCard);

public:
	typedef boost::function<void(bool ok)> result_callback_t;

	static void send(LLPointer<LLImageFormatted> image, const LLSD& postcard_data);
	static void setPostResultCallback(result_callback_t cb) { mResultCallback = cb; }
	static void reportPostResult(bool ok);

private:
	static result_callback_t mResultCallback;
};

#endif // LL_LLPOSTCARD_H
