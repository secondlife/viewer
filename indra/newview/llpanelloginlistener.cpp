/**
 * @file   llpanelloginlistener.cpp
 * @author Nat Goodspeed
 * @date   2009-12-10
 * @brief  Implementation for llpanelloginlistener.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

// Precompiled header
#include "llviewerprecompiledheaders.h"
// associated header
#include "llpanelloginlistener.h"
// STL headers
// std headers
#include <iomanip>
#include <memory>
// external library headers
// other Linden headers
#include "llcombobox.h"
#include "llpanellogin.h"
#include "llsdutil.h"
#include "llsecapi.h"
#include "llslurl.h"
#include "llstartup.h"
#include "lluictrl.h"
#include "llviewernetwork.h"
#include "stringize.h"

LLPanelLoginListener::LLPanelLoginListener(LLPanelLogin* instance):
    LLEventAPI("LLPanelLogin", "Access to LLPanelLogin methods"),
    mPanel(instance)
{
    add("onClickConnect",
        "Pretend user clicked the \"Log In\" button",
        &LLPanelLoginListener::onClickConnect);
    add("login",
        "Login to Second Life with saved credentials.\n"
        "Pass [\"username\"] to select credentials previously saved with that username.\n"
        "Pass [\"slurl\"] to specify target location.\n"
        "Pass [\"grid\"] to select one of the valid grids in grids.xml.",
        &LLPanelLoginListener::login,
        llsd::map("reply", LLSD::String()));
    add("savedLogins",
        "Return \"logins\" as a list of {} saved login names.\n"
        "Pass [\"grid\"] to select one of the valid grids in grids.xml.",
        &LLPanelLoginListener::savedLogins,
        llsd::map("reply", LLSD::String()));
}

void LLPanelLoginListener::onClickConnect(const LLSD&) const
{
    mPanel->onClickConnect(false);
}

namespace
{

    std::string gridkey(const std::string& grid)
    {
        if (grid.find('.') != std::string::npos)
        {
            return grid;
        }
        else
        {
            return stringize("util.", grid, ".lindenlab.com");
        }
    }

} // anonymous namespace

void LLPanelLoginListener::login(const LLSD& args) const
{
    // Although we require a "reply" key, don't instantiate a Response object:
    // if things go well, response to our invoker will come later, once the
    // viewer gets the response from the login server.
    std::string username(args["username"]), slurlstr(args["slurl"]), grid(args["grid"]);
    LL_DEBUGS("AppInit") << "Login with saved credentials";
    if (! username.empty())
    {
        LL_CONT << " for user " << std::quoted(username);
    }
    if (! slurlstr.empty())
    {
        LL_CONT << " to location " << std::quoted(slurlstr);
    }
    if (! grid.empty())
    {
        LL_CONT << " on grid " << std::quoted(grid);
    }
    LL_CONT << LL_ENDL;

    // change grid first, allowing slurl to override
    auto& gridmgr{ LLGridManager::instance() };
    if (! grid.empty())
    {
        grid = gridkey(grid);
        // setGridChoice() can throw LLInvalidGridName -- but if so, let it
        // propagate, trusting that LLEventAPI will catch it and send an
        // appropriate reply.
        auto server_combo = mPanel->getChild<LLComboBox>("server_combo");
        server_combo->setSimple(gridmgr.getGridLabel(grid));
        LLPanelLogin::updateServer();
    }
    if (! slurlstr.empty())
    {
        LLSLURL slurl(slurlstr);
        if (! slurl.isSpatial())
        {
            // don't bother with LLTHROW() because we expect LLEventAPI to
            // catch and report back to invoker
            throw DispatchError(stringize("Invalid start SLURL ", std::quoted(slurlstr)));
        }
        // Bypass LLStartUp::setStartSLURL() because, after validating as
        // above, it bounces right back to LLPanelLogin::onUpdateStartSLURL().
        // It also sets the "NextLoginLocation" user setting, but as with grid,
        // we don't yet know whether that's desirable for scripted login.
        LLPanelLogin::onUpdateStartSLURL(slurl);
    }
    if (! username.empty())
    {
        // Transform (e.g.) "Nat Linden" to the internal form expected by
        // loadFromCredentialMap(), e.g. "nat_linden"
        LLStringUtil::toLower(username);
        LLStringUtil::replaceChar(username, ' ', '_');
        LLStringUtil::replaceChar(username, '.', '_');

        // call gridmgr.getGrid() because our caller didn't necessarily pass
        // ["grid"] -- or it might have been overridden by the SLURL
        auto cred = gSecAPIHandler->loadFromCredentialMap("login_list",
                                                          gridmgr.getGrid(),
                                                          username);
        LLPanelLogin::setFields(cred);
    }

    if (mPanel->getChild<LLUICtrl>("username_combo")->getValue().asString().empty() ||
        mPanel->getChild<LLUICtrl>("password_edit") ->getValue().asString().empty())
    {
        // as above, let LLEventAPI report back to invoker
        throw DispatchError(stringize("Unrecognized username ", std::quoted(username)));
    }

    // Here we're about to trigger actual login, which is all we can do in
    // this method. All subsequent responses must come via the "login"
    // LLEventPump.
    LLEventPumps::instance().obtain("login").listen(
        "Lua login",
        [args]
        (const LLBoundListener& conn, const LLSD& update)
        {
            LLSD response{ llsd::map("ok", false) };
            if (update["change"] == "connect")
            {
                // success!
                response["ok"] = true;
            }
            else if (update["change"] == "disconnect")
            {
                // Does this ever actually happen?
                // LLLoginInstance::handleDisconnect() says it's a placeholder.
                response["error"] = "login disconnect";
            }
            else if (update["change"] == "fail.login")
            {
                // why?
                // LLLoginInstance::handleLoginFailure() has special code to
                // handle "tos", "update" and "mfa_challenge". Do not respond
                // automatically because these things are SUPPOSED to engage
                // the user.
                // Copy specific response fields because there's an enormous
                // chunk of stuff that comes back on success.
                LLSD data{ update["data"] };
                const char* fields[] = {
                    "reason",
                    "error_code"
                };
                for (auto field : fields)
                {
                    response[field] = data[field];
                }
                response["error"] = update["message"];
            }
            else
            {
                // Ignore all other "change" values: LLLogin sends multiple update
                // events. Only a few of them (above) indicate completion.
                return;
            }
            // For all the completion cases, disconnect from the "login"
            // LLEventPump.
            conn.disconnect();
            // and at last, send response to the original invoker
            sendReply(response, args);
        });

    // Turn the Login crank.
    mPanel->onClickConnect(false);
}

LLSD LLPanelLoginListener::savedLogins(const LLSD& args) const
{
    LL_INFOS() << "called with " << args << LL_ENDL;
    std::string grid = (args.has("grid")? gridkey(args["grid"].asString())
                        : LLGridManager::instance().getGrid());
    if (! gSecAPIHandler->hasCredentialMap("login_list", grid))
    {
        LL_INFOS() << "no creds for " << grid << LL_ENDL;
        return llsd::map("error",
                         stringize("No saved logins for grid ", std::quoted(grid)));
    }
    LLSecAPIHandler::credential_map_t creds;
    gSecAPIHandler->loadCredentialMap("login_list", grid, creds);
    auto result = llsd::map(
        "logins",
        llsd::toArray(
            creds,
            [](const auto& pair)
            {
                return llsd::map(
                    "name",
                    LLPanelLogin::getUserName(pair.second),
                    "key",
                    pair.first);
            }));
    LL_INFOS() << "returning creds " << result << LL_ENDL;
    return result;
}
