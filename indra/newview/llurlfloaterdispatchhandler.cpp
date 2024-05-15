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

// Example:
// llOpenFloater("guidebook", "http://page.com", []);

// values specified by server side's dispatcher
// for llopenfloater
const std::string MESSAGE_URL_FLOATER("URLFloater");
const std::string KEY_ACTION("action"); // "action" will be the string constant "OpenURL"
const std::string VALUE_OPEN_URL("OpenURL");
const std::string KEY_DATA("action_data");
const std::string KEY_FLOATER("floater_title"); // name of the floater, not title
const std::string KEY_URL("floater_url");
const std::string KEY_PARAMS("floater_params");

// Supported floaters
const std::string FLOATER_GUIDEBOOK("guidebook");
const std::string FLOATER_HOW_TO("how_to"); // alias for guidebook
const std::string FLOATER_WEB_CONTENT("web_content");

// All arguments are palceholders! Server side will need to add validation first.
// Web content universal argument
const std::string KEY_TRUSTED_CONTENT("trusted_content");

// Guidebook specific arguments
const std::string KEY_WIDTH("width");
const std::string KEY_HEGHT("height");
const std::string KEY_CAN_CLOSE("can_close");
const std::string KEY_TITLE("title");

// web_content specific arguments
const std::string KEY_SHOW_PAGE_TITLE("show_page_title");
const std::string KEY_ALLOW_ADRESS_ENTRY("allow_address_entry"); // It is not recomended to set this to true if trusted content is allowed


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

    // At the moment command_params is a placeholder and code treats it as map
    // Once server side adds argument validation this will be either a map or an array
    std::string floater;
    LLSD command_params;
    std::string url;

    if (message.has(KEY_ACTION) && message[KEY_ACTION].asString() == VALUE_OPEN_URL)
    {
        LLSD &action_data = message[KEY_DATA];
        if (action_data.isMap())
        {
            floater = action_data[KEY_FLOATER].asString();
            command_params = action_data[KEY_PARAMS];
            url = action_data[KEY_URL].asString();
        }
    }
    else if (message.has(KEY_FLOATER))
    {
        floater = message[KEY_FLOATER].asString();
        command_params = message[KEY_PARAMS];
        url = message[KEY_URL].asString();
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

    if (floater == FLOATER_GUIDEBOOK || floater == FLOATER_HOW_TO)
    {
        LL_DEBUGS("URLFloater") << "Opening how_to floater with parameters: " << message << LL_ENDL;
        if (command_params.isMap()) // by default is undefines
        {
            params.trusted_content = command_params.has(KEY_TRUSTED_CONTENT) ? command_params[KEY_TRUSTED_CONTENT].asBoolean() : false;

            // Script's side argument list can't include other lists, neither
            // there is a LLRect type, so expect just width and height
            if (command_params.has(KEY_WIDTH) && command_params.has(KEY_HEGHT))
            {
                LLRect rect(0, command_params[KEY_HEGHT].asInteger(), command_params[KEY_WIDTH].asInteger(), 0);
                params.preferred_media_size.setValue(rect);
            }
        }

        // Some locations will have customized guidebook, which this function easists for
        // only one instance of guidebook can exist at a time, so if this command arrives,
        // we need to close previous guidebook then reopen it.

        LLFloater* instance = LLFloaterReg::findInstance("guidebook");
        if (instance)
        {
            instance->closeHostedFloater();
        }

        LLFloaterReg::toggleInstanceOrBringToFront("guidebook", params);

        if (command_params.isMap())
        {
            LLFloater* instance = LLFloaterReg::findInstance("guidebook");
            if (command_params.has(KEY_CAN_CLOSE))
            {
                instance->setCanClose(command_params[KEY_CAN_CLOSE].asBoolean());
            }
            if (command_params.has(KEY_TITLE))
            {
                instance->setTitle(command_params[KEY_TITLE].asString());
            }
        }
    }
    else if (floater == FLOATER_WEB_CONTENT)
    {
        LL_DEBUGS("URLFloater") << "Opening web_content floater with parameters: " << message << LL_ENDL;
        if (command_params.isMap()) // by default is undefines, might be better idea to init params from command_params
        {
            params.trusted_content = command_params.has(KEY_TRUSTED_CONTENT) ? command_params[KEY_TRUSTED_CONTENT].asBoolean() : false;
            params.show_page_title = command_params.has(KEY_SHOW_PAGE_TITLE) ? command_params[KEY_SHOW_PAGE_TITLE].asBoolean() : true;
            params.allow_address_entry = command_params.has(KEY_ALLOW_ADRESS_ENTRY) ? command_params[KEY_ALLOW_ADRESS_ENTRY].asBoolean() : true;
        }
        LLFloaterReg::showInstance("web_content", params);
    }
    else
    {
        LL_DEBUGS("URLFloater") << "Unknow floater with parameters: " << message << LL_ENDL;
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
