/** 
 * @file llmarketplacenotifications.h
 * @brief Handler for notifications related to marketplace file I/O
 * class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMARKETPLACENOTIFICATIONS_H
#define LL_LLMARKETPLACENOTIFICATIONS_H


#include <llsd.h>
#include <boost/function.hpp>


//
// This is a set of helper functions to handle a unique notification with multiple
// payloads, helpful when dragging and dropping items to the merchant outbox that
// trigger notifications that can potentially interfere with the current drag and
// drop operation.
//
// Notification payloads are cached locally when initiated, the notification itself
// is triggered on the following frame during the call to "update" and then the
// response is triggered once per payload.
//

namespace LLMarketplaceInventoryNotifications
{
	void update();

	typedef boost::function<void (const LLSD&)> NoCopyCallbackFunction;
	
	void addNoCopyNotification(const LLSD& payload, const NoCopyCallbackFunction& cb);
};


#endif // LL_LLMARKETPLACENOTIFICATIONS_H
