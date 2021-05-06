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
#include "llfloaterhowto.h"
#include "llfloaterwebcontent.h"
#include "llsdserialize.h"
#include "llviewercontrol.h"
#include "llviewergenericmessage.h"
#include "llweb.h"

// values specified by server side's dispatcher
// for llopenfloater
const std::string MESSAGE_URL_FLOATER("URLFloater");
const std::string KEY_ACTION("action"); // "action" will be the string constant "OpenURL"
const std::string VALUE_OPEN_URL("OpenURL");
const std::string KEY_DATA("action_data");
const std::string KEY_FLOATER("floater_title");
const std::string KEY_URL("floater_url");
const std::string KEY_PARAMS("floater_params");

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
            return false;
        }
    }

    std::string floater;
    LLSD command_params;
    std::string url;

    if (message.has(KEY_ACTION) && message[KEY_ACTION].asString() == VALUE_OPEN_URL)
    {
        LLSD &action_data = message[KEY_DATA];
        if (action_data.isMap())
        {
            floater = action_data[KEY_FLOATER];
            command_params = action_data[KEY_PARAMS];
            url = action_data[KEY_URL];
        }
    }
    else if (message.has(KEY_FLOATER))
    {
        floater = message[KEY_FLOATER];
        command_params = message[KEY_PARAMS];
        url = message[KEY_URL];
    }
    else
    {
        LL_WARNS("URLFloater") << "Received " << MESSAGE_URL_FLOATER << " with unexpected data format: " << message << LL_ENDL;
        return false;
    }

    if (url.find("://") == std::string::npos)
    {
        // try unescaping
        url = LLURI::unescape(url);
    }

    LLFloaterWebContent::Params params;
    params.url = url;

    if (floater == "guidebook" || floater == "how_to")
    {
        if (command_params.isMap()) // by default is undefines
        {
            params.trusted_content = command_params.has("trusted_content") ? command_params["trusted_content"] : false;

            // Script's side argument list can't include other lists, neither
            // there is a LLRect type, so expect just width and height
            if (command_params.has("width") && command_params.has("height"))
            {
                LLRect rect(0, command_params["height"].asInteger(), command_params["width"].asInteger(), 0);
                params.preferred_media_size.setValue(rect);
            }
        }

        // Some locations will have customized guidebook, which this function easists for
        // only one instance of guidebook can exist at a time, so if this command arrives,
        // we need to close previous guidebook then reopen it.

        LLFloater* instance = LLFloaterReg::findInstance("how_to");
        if (instance)
        {
            instance->closeHostedFloater();
        }

        LLFloaterReg::toggleInstanceOrBringToFront("how_to", params);
    }
    else if (floater == "web_content")
    {
        if (command_params.isMap()) // by default is undefines, might be better idea to init params from command_params
        {
            params.trusted_content = command_params.has("trusted_content") ? command_params["trusted_content"] : false;
            params.show_page_title = command_params.has("show_page_title") ? command_params["show_page_title"] : true;
            params.allow_address_entry = command_params.has("allow_address_entry") ? command_params["allow_address_entry"] : true;
        }
        LLFloaterReg::showInstance("web_content", params);
    }
    else
    {
        if (LLFloaterReg::isRegistered(floater))
        {
            // A valid floater
            LL_INFOS("URLFloater") << "Floater " << floater << " is not supported by llopenfloater or URLFloater" << LL_ENDL;
        }
        else
        {
            // A valid message, but no such flaoter
            LL_WARNS("URLFloater") << "Recieved a command to open unknown floater: " << floater << LL_ENDL;
        }
    }

    return true;
}
