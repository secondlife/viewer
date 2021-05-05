/** 
 * @file llurlfloaterdispatchhandler.cpp
 * @brief Handles URLFloater generic message from server
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#include "llurlfloaterdispatchhandler.h"

#include "llfloaterreg.h"
#include "llfloaterwebcontent.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llweb.h"

// values specified by server side's dispatcher
const std::string MESSAGE_URL_FLOATER("URLFloater");
const std::string KEY_ACTION("OpenURL");
const std::string KEY_PARAMS("floater_params");
const std::string KEY_FLOATER("floater_title");
const std::string KEY_URL("floater_url");

LLUrlFloaterDispatchHandler LLUrlFloaterDispatchHandler::sUrlDispatchhandler;

LLUrlFloaterDispatchHandler::LLUrlFloaterDispatchHandler()
{
}

LLUrlFloaterDispatchHandler::~LLUrlFloaterDispatchHandler()
{
}

void LLUrlFloaterDispatchHandler::registerInDispatcher()
{
    if (!gGenericDispatcher.isHandlerPresent(MESSAGE_URL_FLOATER))
    {
        gGenericDispatcher.addHandler(MESSAGE_URL_FLOATER, &sUrlDispatchhandler);
    }
}

//virtual
bool LLUrlFloaterDispatchHandler::operator()(const LLDispatcher *, const std::string& key, const LLUUID& invoice, const sparam_t& strings)
{
    // invoice - transaction id

    LLSD message;
    sparam_t::const_iterator it = strings.begin();

    if (it != strings.end())
    {
        const std::string& llsdRaw = *it++;
        std::istringstream llsdData(llsdRaw);
        if (!LLSDSerialize::deserialize(message, llsdData, llsdRaw.length()))
        {
            LL_WARNS("URLFloater") << "Attempted to read parameter data into LLSD but failed:" << llsdRaw << LL_ENDL;
        }
    }

    std::string floater = message[KEY_FLOATER];
    LLSD &command_params = message[KEY_PARAMS];

    LLFloaterWebContent::Params params;
    params.url = message[KEY_URL];

    if (floater == "guidebook" || floater == "how_to")
    {
        if (command_params.isMap()) // by default is undefines
        {
            params.trusted_content = command_params.has("trusted_content") ? command_params["trusted_content"] : false;
        }
        LLFloaterReg::toggleInstanceOrBringToFront("how_to", params);
    }
    else if (!params.url.getValue().empty())
    {
        if (command_params.isMap()) // by default is undefines
        {
            params.trusted_content = command_params.has("trusted_content") ? command_params["trusted_content"] : false;
            params.show_page_title = command_params.has("show_page_title") ? command_params["show_page_title"] : true;
            params.allow_address_entry = command_params.has("allow_address_entry") ? command_params["allow_address_entry"] : true;
        }
        LLFloaterReg::showInstance("web_content", params);
    }

    return true;
}
