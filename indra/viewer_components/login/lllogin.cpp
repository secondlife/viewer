/**
 * @file lllogin.cpp
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

#include "llwin32headers.h"
#include "linden_common.h"
#include "llsd.h"
#include "llsdutil.h"

#include "lllogin.h"

#include <boost/bind.hpp>

#include "llcoros.h"
#include "llevents.h"
#include "lleventfilter.h"
#include "lleventcoro.h"
#include "llexception.h"
#include "stringize.h"

//*********************
// LLLogin
// *NOTE:Mani - Is this Impl needed now that the state machine runs the show?
class LLLogin::Impl
{
public:
    Impl():
        mPump("login", true) // Create the module's event pump with a tweaked (unique) name.
    {
        mValidAuthResponse["status"]        = LLSD();
        mValidAuthResponse["errorcode"]     = LLSD();
        mValidAuthResponse["error"]         = LLSD();
        mValidAuthResponse["transfer_rate"] = LLSD();
    }

    void connect(const std::string& uri, const LLSD& credentials);
    void disconnect();
    LLEventPump& getEventPump() { return mPump; }

private:
    LLSD hidePasswd(const LLSD& data)
    {
        LLSD result(data);
        if (result.has("params") && result["params"].has("passwd"))
        {
            result["params"]["passwd"] = "*******";
        }
        return result;
    }

    LLSD getProgressEventLLSD(const std::string& state, const std::string& change,
                           const LLSD& data = LLSD())
    {
        LLSD status_data;
        status_data["state"] = state;
        status_data["change"] = change;
        status_data["progress"] = 0.0f;

        if (mAuthResponse.has("transfer_rate"))
        {
            status_data["transfer_rate"] = mAuthResponse["transfer_rate"];
        }

        if (data.isDefined())
        {
            status_data["data"] = data;
        }

        return status_data;
    }

    void sendProgressEvent(const std::string& state, const std::string& change,
                           const LLSD& data = LLSD())
    {
        LLSD status_data = getProgressEventLLSD(state, change, data);
        mPump.post(status_data);
    }

    LLSD validateResponse(const std::string& pumpName, const LLSD& response)
    {
        // Validate the response. If we don't recognize it, things
        // could get ugly.
        std::string mismatch(llsd_matches(mValidAuthResponse, response));
        if (! mismatch.empty())
        {
            LL_ERRS("LLLogin") << "Received unrecognized event (" << mismatch << ") on "
                               << pumpName << "pump: " << response
                               << LL_ENDL;
            return LLSD();
        }

        return response;
    }

    // In a coroutine's top-level function args, do NOT NOT NOT accept
    // references (const or otherwise) to anything! Pass by value only!
    void loginCoro(std::string uri, LLSD credentials);

    LLEventStream mPump;
    LLSD mAuthResponse, mValidAuthResponse;
};

void LLLogin::Impl::connect(const std::string& uri, const LLSD& login_params)
{
    LL_DEBUGS("LLLogin") << " connect with uri '" << uri << "', login_params " << login_params << LL_ENDL;

    // Launch a coroutine with our login_() method. Run the coroutine until
    // its first wait; at that point, return here.
    std::string coroname =
        LLCoros::instance().launch("LLLogin::Impl::login_", [=, this]() { loginCoro(uri, login_params); });

    LL_DEBUGS("LLLogin") << " connected with uri '" << uri << "', login_params " << login_params << LL_ENDL;
}

namespace
{
// Instantiate this rendezvous point at namespace scope so it's already
// present no matter how early the updater might post to it.
// Use an LLEventMailDrop, which has future-like semantics: regardless of the
// relative order in which post() or listen() are called, it delivers each
// post() event to its listener(s) until one of them consumes that event.
static LLEventMailDrop sSyncPoint("LoginSync");
}

void LLLogin::Impl::loginCoro(std::string uri, LLSD login_params)
{
    LLSD printable_params = hidePasswd(login_params);

    try
    {
        LL_DEBUGS("LLLogin") << "Entering coroutine " << LLCoros::getName()
                             << " with uri '" << uri << "', parameters " << printable_params << LL_ENDL;

        LLEventPump& xmlrpcPump(LLEventPumps::instance().obtain("LLXMLRPCTransaction"));
        // EXT-4193: use a DIFFERENT reply pump than for the SRV request. We used
        // to share them -- but the EXT-3934 fix made it possible for an abandoned
        // SRV response to arrive just as we were expecting the XMLRPC response.
        LLEventStream loginReplyPump("loginreply", true);

        LLSD::Integer attempts = 0;

        LLSD request(login_params);
        request["reply"] = loginReplyPump.getName();
        request["uri"] = uri;
        std::string status;

        // Loop back to here if login attempt redirects to a different
        // request["uri"]
        for (;;)
        {
            ++attempts;
            LLSD progress_data;
            progress_data["attempt"] = attempts;
            progress_data["request"] = hidePasswd(request);
            sendProgressEvent("offline", "authenticating", progress_data);

            // We expect zero or more "Downloading" status events, followed by
            // exactly one event with some other status. Use postAndSuspend() the
            // first time, because -- at least in unit-test land -- it's
            // possible for the reply to arrive before the post() call
            // returns. Subsequent responses, of course, must be awaited
            // without posting again.
            for (mAuthResponse = validateResponse(loginReplyPump.getName(),
                                                  llcoro::postAndSuspend(request, xmlrpcPump, loginReplyPump, "reply"));
                 mAuthResponse["status"].asString() == "Downloading";
                 mAuthResponse = validateResponse(loginReplyPump.getName(),
                                                  llcoro::suspendUntilEventOn(loginReplyPump)))
            {
                // Still Downloading -- send progress update.
                sendProgressEvent("offline", "downloading");
            }

            LL_DEBUGS("LLLogin") << "Auth Response: " << mAuthResponse << LL_ENDL;
            status = mAuthResponse["status"].asString();

            // Okay, we've received our final status event for this
            // request. Unless we got a redirect response, break the retry
            // loop for the current rewrittenURIs entry.
            if (!(status == "Complete" &&
                  mAuthResponse["responses"]["login"].asString() == "indeterminate"))
            {
                break;
            }

            sendProgressEvent("offline", "indeterminate", mAuthResponse["responses"]);

            // Here the login service at the current URI is redirecting us
            // to some other URI ("indeterminate" -- why not "redirect"?).
            // The response should contain another uri to try, with its
            // own auth method.
            request["uri"] = mAuthResponse["responses"]["next_url"].asString();
            request["method"] = mAuthResponse["responses"]["next_method"].asString();
        } // loop back to try the redirected URI

        // Here we're done with redirects.
        if (status == "Complete")
        {
            // StatusComplete does not imply auth success. Check the
            // actual outcome of the request. We've already handled the
            // "indeterminate" case in the loop above.
            if (mAuthResponse["responses"]["login"].asString() == "true")
            {
                sendProgressEvent("online", "connect", mAuthResponse["responses"]);
            }
            else
            {
                // Synchronize here with the updater. We synchronize here rather
                // than in the fail.login handler, which actually examines the
                // response from login.cgi, because here we are definitely in a
                // coroutine and can definitely use suspendUntilBlah(). Whoever's
                // listening for fail.login might not be.

                // If the reason for login failure is that we must install a
                // required update, we definitely want to pass control to the
                // updater to manage that for us. We'll handle any other login
                // failure ourselves, as usual. We figure that no matter where you
                // are in the world, or what kind of network you're on, we can
                // reasonably expect the Viewer Version Manager to respond more or
                // less as quickly as login.cgi. This synchronization is only
                // intended to smooth out minor races between the two services.
                // But what if the updater crashes? Use a timeout so that
                // eventually we'll tire of waiting for it and carry on as usual.
                // Given the above, it can be a fairly short timeout, at least
                // from a human point of view.

                // Since sSyncPoint is an LLEventMailDrop, we DEFINITELY want to
                // consume the posted event.
                LLCoros::OverrideConsuming oc(true);
                LLSD responses(mAuthResponse["responses"]);
                LLSD updater;

                if (printable_params["wait_for_updater"].asBoolean())
                {
                    std::string reason_response = responses["data"]["reason"].asString();
                    // Timeout should produce the isUndefined() object passed here.
                    if (reason_response == "update")
                    {
                        LL_INFOS("LLLogin") << "Login failure, waiting for sync from updater" << LL_ENDL;
                        updater = llcoro::suspendUntilEventOnWithTimeout(sSyncPoint, 10, LLSD());
                    }
                    else
                    {
                        LL_DEBUGS("LLLogin") << "Login failure, waiting for sync from updater" << LL_ENDL;
                        updater = llcoro::suspendUntilEventOnWithTimeout(sSyncPoint, 3, LLSD());
                    }
                    if (updater.isUndefined())
                    {
                        LL_WARNS("LLLogin") << "Failed to hear from updater, proceeding with fail.login"
                                            << LL_ENDL;
                    }
                    else
                    {
                        LL_DEBUGS("LLLogin") << "Got responses from updater and login.cgi" << LL_ENDL;
                    }
                }

                // Let the fail.login handler deal with empty updater response.
                responses["updater"] = updater;
                sendProgressEvent("offline", "fail.login", responses);
            }
            return;             // Done!
        }

/*==========================================================================*|
        // Sometimes we end with "Started" here. Slightly slow server? Seems
        // to be ok to just skip it. Otherwise we'd error out and crash in the
        // if below.
        if( status == "Started")
        {
            LL_DEBUGS("LLLogin") << mAuthResponse << LL_ENDL;
            continue;
        }
|*==========================================================================*/

        // If we don't recognize status at all, trouble
        if (! (status == "CURLError"
               || status == "BadType"
               || status == "XMLRPCError"
               || status == "OtherError"))
        {
            LL_ERRS("LLLogin") << "Unexpected status " << status
                               << " from " << xmlrpcPump.getName()
                               << " pump: " << mAuthResponse << LL_ENDL;
            return;
        }

        if (status == "BadType")
        {
            // Invalid xmlrpc type
            // Dump this response into logs
            LL_WARNS("LLLogin") << "Failed to parse response"
                << " from " << xmlrpcPump.getName()
                << " pump: " << mAuthResponse << LL_ENDL;
        }

        // Here status IS one of the errors tested above.
        // Tell caller this didn't work out so well.

        // *NOTE: The response from LLXMLRPCListener's Poller::poll method returns an
        // llsd with no "responses" node. To make the output from an incomplete login symmetrical
        // to success, add a data/message and data/reason fields.
        LLSD error_response(LLSDMap
                            ("reason",    mAuthResponse["status"])
                            ("errorcode", mAuthResponse["errorcode"])
                            ("message",   mAuthResponse["error"]));
        if(mAuthResponse.has("certificate"))
        {
            error_response["certificate"] = mAuthResponse["certificate"];
        }
        sendProgressEvent("offline", "fail.login", error_response);
    }
    catch (...) {
        LOG_UNHANDLED_EXCEPTION(STRINGIZE("coroutine " << LLCoros::getName()
                                          << "('" << uri << "', " << printable_params << ")"));
        throw;
    }
}

void LLLogin::Impl::disconnect()
{
    sendProgressEvent("offline", "disconnect");
}

//*********************
// LLLogin
LLLogin::LLLogin() :
    mImpl(new LLLogin::Impl())
{
}

LLLogin::~LLLogin()
{
}

void LLLogin::connect(const std::string& uri, const LLSD& credentials)
{
    mImpl->connect(uri, credentials);
}


void LLLogin::disconnect()
{
    mImpl->disconnect();
}

LLEventPump& LLLogin::getEventPump()
{
    return mImpl->getEventPump();
}

// The following is the list of important functions that happen in the
// current login process that we want to move to this login module.

// The list associates to event with the original idle_startup() 'STATE'.

// Setup login
// State_LOGIN_AUTH_INIT

// Authenticate
// STATE_LOGIN_AUTHENTICATE
// Connect to the login server, presumably login.cgi, requesting the login
// and a slew of related initial connection information.
// This is an asynch action. The final response, whether success or error
// is handled by STATE_LOGIN_PROCESS_REPONSE.
// There is no immediate error or output from this call.
//
// Input:
//  URI
//  Credentials (first, last, password)
//  Start location
//  Bool Flags:
//    skip optional update
//    accept terms of service
//    accept critical message
//  Last exec event. (crash state of previous session)
//  requested optional data (inventory skel, initial outfit, etc.)
//  local mac address
//  viewer serial no. (md5 checksum?)

//sAuthUriNum = llclamp(sAuthUriNum, 0, (S32)sAuthUris.size()-1);
//LLUserAuth::getInstance()->authenticate(
//  sAuthUris[sAuthUriNum],
//  auth_method,
//  firstname,
//  lastname,
//  password, // web_login_key,
//  start.str(),
//  gSkipOptionalUpdate,
//  gAcceptTOS,
//  gAcceptCriticalMessage,
//  gLastExecEvent,
//  requested_options,
//  hashed_mac_string,
//  LLAppViewer::instance()->getSerialNumber());

//
// Download the Response
// STATE_LOGIN_NO_REPONSE_YET and STATE_LOGIN_DOWNLOADING
// I had assumed that this was default behavior of the message system. However...
// During login, the message system is checked only by these two states in idle_startup().
// I guess this avoids the overhead of checking network messages for those login states
// that don't need to do so, but geez!
// There are two states to do this one function just to update the login
// status text from 'Logging In...' to 'Downloading...'
//

//
// Handle Login Response
// STATE_LOGIN_PROCESS_RESPONSE
//
// This state handle the result of the request to login. There is a metric ton of
// code in this case. This state will transition to:
// STATE_WORLD_INIT, on success.
// STATE_AUTHENTICATE, on failure.
// STATE_UPDATE_CHECK, to handle user during login interaction like TOS display.
//
// Much of the code in this case belongs on the viewer side of the fence and not in login.
// Login should probably return with a couple of events, success and failure.
// Failure conditions can be specified in the events data pacet to allow the viewer
// to re-engauge login as is appropriate. (Or should there be multiple failure messages?)
// Success is returned with the data requested from the login. According to OGP specs
// there may be intermediate steps before reaching this result in future login
// implementations.
