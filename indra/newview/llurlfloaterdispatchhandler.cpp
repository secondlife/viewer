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

#include <boost/regex.hpp>

// Example:
// llOpenFloater("My Help Title", "secondlife://guidebook", []);

// values specified by server side's dispatcher
// for llopenfloater
const std::string MESSAGE_URL_FLOATER("URLFloater");
const std::string KEY_ACTION("action"); // "action" will be the string constant "OpenURL"
const std::string VALUE_OPEN_URL("OpenURL");
const std::string KEY_DATA("action_data");
const std::string KEY_FLOATER_TITLE("floater_title");
const std::string KEY_URI("floater_url");
const std::string KEY_PARAMS("floater_params");

// Supported floaters, for now it's exact matching, later it might get extended
const std::string FLOATER_GUIDEBOOK("secondlife:///guidebook"); // translates to "how_to"
const std::string FLOATER_WEB_CONTENT("secondlife:///browser"); // translates to "web_content"

// Web content universal arguments
const std::string KEY_TRUSTED_CONTENT("trusted_content");
const std::string KEY_URL("url");

// Guidebook ("how_to") specific arguments
const std::string KEY_WIDTH("width");
const std::string KEY_HEGHT("height");
const std::string KEY_CAN_CLOSE("can_close");

// web_content specific arguments
const std::string KEY_SHOW_PAGE_TITLE("show_page_title");
const std::string KEY_ALLOW_ADRESS_ENTRY("allow_address_entry"); // It is not recomended to set this to true if trusted content is allowed

// expected format secondlife:///floater_alias
// intended to be extended to: secondlife:///floater_alias/instance_id
const boost::regex expression("secondlife:///[^ \n]{1,256}");


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

    std::string floater_title;
    LLSD command_params;
    std::string floater_uri;

    if (message.has(KEY_ACTION) && message[KEY_ACTION].asString() == VALUE_OPEN_URL)
    {
        LLSD &action_data = message[KEY_DATA];
        if (action_data.isMap())
        {
            floater_title = action_data[KEY_FLOATER_TITLE].asString();
            command_params = action_data[KEY_PARAMS];
            floater_uri = action_data[KEY_URI].asString();
        }
    }
    else if (message.has(KEY_FLOATER_TITLE))
    {
        floater_title = message[KEY_FLOATER_TITLE].asString();
        command_params = message[KEY_PARAMS];
        floater_uri = message[KEY_URI].asString();
    }
    else
    {
        LL_WARNS("URLFloater") << "Received " << MESSAGE_URL_FLOATER << " with unexpected data format: " << message << LL_ENDL;
        return false;
    }

    if (floater_uri.find(":///") == std::string::npos)
    {
        // try unescaping
        floater_uri = LLURI::unescape(floater_uri);
    }

    boost::cmatch what;
    if (!boost::regex_match(floater_uri.c_str(), what, expression))
    {
        LL_WARNS("URLFloater") << "Received " << MESSAGE_URL_FLOATER << " with invalid uri: " << floater_uri << LL_ENDL;
        return false;
    }

    LLFloaterWebContent::Params params;
    if (command_params.isMap())
    {
        if (command_params.has(KEY_TRUSTED_CONTENT))
        {
            params.trusted_content = command_params[KEY_TRUSTED_CONTENT].asBoolean();
        }
        if (command_params.has(KEY_URL))
        {
            params.url = command_params[KEY_URL].asString();
        }
    }

    if (floater_uri == FLOATER_GUIDEBOOK)
    {
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

        LLFloater* instance = LLFloaterReg::findInstance("how_to");
        if (instance)
        {
            instance->closeHostedFloater();
        }

        LLFloaterReg::toggleInstanceOrBringToFront("how_to", params);

        instance = LLFloaterReg::findInstance("how_to");
        instance->setTitle(floater_title);
        if (command_params.isMap() && command_params.has(KEY_CAN_CLOSE))
        {
            instance->setCanClose(command_params[KEY_CAN_CLOSE].asBoolean());
        }
    }
    else if (floater_uri == FLOATER_WEB_CONTENT)
    {
        if (command_params.has(KEY_URL))
        {
            if (command_params.isMap()) // by default is undefines, might be better idea to init params from command_params
            {
                params.trusted_content = command_params.has(KEY_TRUSTED_CONTENT) ? command_params[KEY_TRUSTED_CONTENT].asBoolean() : false;
                params.show_page_title = command_params.has(KEY_SHOW_PAGE_TITLE) ? command_params[KEY_SHOW_PAGE_TITLE].asBoolean() : true;
                params.allow_address_entry = command_params.has(KEY_ALLOW_ADRESS_ENTRY) ? command_params[KEY_ALLOW_ADRESS_ENTRY].asBoolean() : true;
            }
            LLFloater* instance = LLFloaterReg::showInstance("web_content", params);
            instance->setTitle(floater_title);
        }
    }
    else
    {
        LLSD path_array = LLURI(floater_uri).pathArray();
        S32 path_parts = path_array.size();
        if (path_parts == 0)
        {
            LL_INFOS("URLFloater") << "received an empty uri: " << floater_uri << LL_ENDL;
        }
        else if (LLFloaterReg::isRegistered(path_array[0]))
        {
            // A valid floater
            LL_INFOS("URLFloater") << "Floater " << path_array[0] << " is not supported by llopenfloater or URLFloater" << LL_ENDL;
        }
        else
        {
            // A valid message, but no such flaoter
            LL_WARNS("URLFloater") << "Recieved a command to open unknown floater: " << floater_uri << LL_ENDL;
        }
    }

    return true;
}
