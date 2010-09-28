/**
* @file llnotificationvisibility.h
* @brief Rules for 
* @author Monroe
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

#ifndef LL_LLNOTIFICATION_VISIBILITY_RULE_H
#define LL_LLNOTIFICATION_VISIBILITY_RULE_H

#include "llinitparam.h"
//#include "llnotifications.h"



// This is the class of object read from the XML file (notification_visibility.xml, 
// from the appropriate local language directory).
struct LLNotificationVisibilityRule
{
	struct Params : public LLInitParam::Block<Params>
	{
		Mandatory<bool>	visible;
		Optional<std::string> type;
		Optional<std::string> tag;

		Params()
		:	visible("visible"),
			type("type"),
			tag("tag")
		{}
	};


	struct Rules : public LLInitParam::Block<Rules>
	{
		Multiple<Params>	rules;

		Rules()
		:	rules("rule")
		{}
	};

	LLNotificationVisibilityRule(const Params& p);
	
    // If true, this rule makes matching notifications visible.  Otherwise, it makes them invisible.
    bool mVisible;

    // String to match against the notification's "type".  An empty string matches all notifications.
    std::string mType;
	
    // String to match against the notification's tag(s).  An empty string matches all notifications.
	std::string mTag;
};

#endif //LL_LLNOTIFICATION_VISIBILITY_RULE_H

