/** 
 * @file llmarketplacenotifications.cpp
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

// Precompiled header
#include "llviewerprecompiledheaders.h"

#include "llmarketplacenotifications.h"
#include "llnotificationsutil.h"

#include "llerror.h"

#include <boost/foreach.hpp>
#include <boost/signals2.hpp>


namespace LLMarketplaceInventoryNotifications
{
	typedef boost::signals2::signal<void (const LLSD& param)>	no_copy_payload_cb_signal_t;

	static no_copy_payload_cb_signal_t*	no_copy_cb_action = NULL;
	static bool							no_copy_notify_active = false;
	static std::list<LLSD>				no_copy_payloads;

	void notifyNoCopyCallback(const LLSD& notification, const LLSD& response)
	{
		const S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
		
		if (option == 0)
		{
			llassert(!no_copy_payloads.empty());
			llassert(no_copy_cb_action != NULL);
			
			BOOST_FOREACH(const LLSD& payload, no_copy_payloads)
			{
				(*no_copy_cb_action)(payload);
			}
		}
		
		delete no_copy_cb_action;
		no_copy_cb_action = NULL;
		
		no_copy_notify_active = false;
		no_copy_payloads.clear();
	}

	void update()
	{
		if (!no_copy_notify_active && !no_copy_payloads.empty())
		{
			no_copy_notify_active = true;
			
			LLNotificationsUtil::add("ConfirmNoCopyToOutbox", LLSD(), LLSD(), boost::bind(&notifyNoCopyCallback, _1, _2));
		}
	}
		
	void addNoCopyNotification(const LLSD& payload, const NoCopyCallbackFunction& cb)
	{
		if (no_copy_cb_action == NULL)
		{
			no_copy_cb_action = new no_copy_payload_cb_signal_t;
			no_copy_cb_action->connect(boost::bind(cb, _1));
		}

		no_copy_payloads.push_back(payload);
	}
}
