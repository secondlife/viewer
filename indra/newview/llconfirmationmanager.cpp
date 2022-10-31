/** 
 * @file llconfirmationmanager.cpp
 * @brief LLConfirmationManager class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llconfirmationmanager.h"

#include "lluictrlfactory.h"

// viewer includes
#include "llnotificationsutil.h"
#include "llstring.h"
#include "llxmlnode.h"

LLConfirmationManager::ListenerBase::~ListenerBase()
{
}


static bool onConfirmAlert(const LLSD& notification, const LLSD& response, LLConfirmationManager::ListenerBase* listener)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        listener->confirmed("");
    }
    
    delete listener;
    return false;
}

static bool onConfirmAlertPassword(const LLSD& notification, const LLSD& response, LLConfirmationManager::ListenerBase* listener)
{
    std::string text = response["message"].asString();
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
        
    if (option == 0)
    {
        listener->confirmed(text);
    }
    
    delete listener;
    return false;
}

 
void LLConfirmationManager::confirm(Type type,
    const std::string& action,
    ListenerBase* listener)
{
    LLSD args;
    args["ACTION"] = action;

    switch (type)
    {
        case TYPE_CLICK:
            LLNotificationsUtil::add("ConfirmPurchase", args, LLSD(), boost::bind(onConfirmAlert, _1, _2, listener));
          break;

        case TYPE_PASSWORD:
            LLNotificationsUtil::add("ConfirmPurchasePassword", args, LLSD(), boost::bind(onConfirmAlertPassword, _1, _2, listener));
          break;
        case TYPE_NONE:
        default:
          listener->confirmed("");
          break;
    }
}


void LLConfirmationManager::confirm(
    const std::string& type,
    const std::string& action,
    ListenerBase* listener)
{
    Type decodedType = TYPE_NONE;
    
    if (type == "click")
    {
        decodedType = TYPE_CLICK;
    }
    else if (type == "password")
    {
        decodedType = TYPE_PASSWORD;
    }
    
    confirm(decodedType, action, listener);
}

