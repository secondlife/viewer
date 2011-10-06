/**
 * @file   lleventapi.h
 * @author Nat Goodspeed
 * @date   2009-10-28
 * @brief  LLEventAPI is the base class for every class that wraps a C++ API
 *         in an event API
 * (see https://wiki.lindenlab.com/wiki/Incremental_Viewer_Automation/Event_API).
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

#if ! defined(LL_LLEVENTAPI_H)
#define LL_LLEVENTAPI_H

#include "lleventdispatcher.h"
#include "llinstancetracker.h"
#include <string>

/**
 * LLEventAPI not only provides operation dispatch functionality, inherited
 * from LLDispatchListener -- it also gives us event API introspection.
 * Deriving from LLInstanceTracker lets us enumerate instances.
 */
class LL_COMMON_API LLEventAPI: public LLDispatchListener,
                  public LLInstanceTracker<LLEventAPI, std::string>
{
    typedef LLDispatchListener lbase;
    typedef LLInstanceTracker<LLEventAPI, std::string> ibase;

public:
    /**
     * @param name LLEventPump name on which this LLEventAPI will listen. This
     * also serves as the LLInstanceTracker instance key.
     * @param desc Documentation string shown to a client trying to discover
     * available event APIs.
     * @param field LLSD::Map key used by LLDispatchListener to look up the
     * subclass method to invoke [default "op"].
     */
    LLEventAPI(const std::string& name, const std::string& desc, const std::string& field="op");
    virtual ~LLEventAPI();

    /// Get the string name of this LLEventAPI
    std::string getName() const { return ibase::getKey(); }
    /// Get the documentation string
    std::string getDesc() const { return mDesc; }

    /**
     * Publish only selected add() methods from LLEventDispatcher.
     * Every LLEventAPI add() @em must have a description string.
     */
    template <typename CALLABLE>
    void add(const std::string& name,
             const std::string& desc,
             CALLABLE callable,
             const LLSD& required=LLSD())
    {
        LLEventDispatcher::add(name, desc, callable, required);
    }

    /**
     * Instantiate a Response object in any LLEventAPI subclass method that
     * wants to guarantee a reply (if requested) will be sent on exit from the
     * method. The reply will be sent if request.has(@a replyKey), default
     * "reply". If specified, the value of request[replyKey] is the name of
     * the LLEventPump on which to send the reply. Conventionally you might
     * code something like:
     *
     * @code
     * void MyEventAPI::someMethod(const LLSD& request)
     * {
     *     // Send a reply event as long as request.has("reply")
     *     Response response(LLSD(), request);
     *     // ...
     *     // will be sent in reply event
     *     response["somekey"] = some_data;
     * }
     * @endcode
     */
    class LL_COMMON_API Response
    {
    public:
        /**
         * Instantiating a Response object in an LLEventAPI subclass method
         * ensures that, if desired, a reply event will be sent.
         *
         * @a seed is the initial reply LLSD that will be further decorated before
         * being sent as the reply
         *
         * @a request is the incoming request LLSD; we particularly care about
         * [replyKey] and ["reqid"]
         *
         * @a replyKey [default "reply"] is the string name of the LLEventPump
         * on which the caller wants a reply. If <tt>(!
         * request.has(replyKey))</tt>, no reply will be sent.
         */
        Response(const LLSD& seed, const LLSD& request, const LLSD::String& replyKey="reply");
        ~Response();

        /**
         * @code
         * if (some condition)
         * {
         *     response.warn("warnings are logged and collected in [\"warnings\"]");
         * }
         * @endcode
         */
        void warn(const std::string& warning);
        /**
         * @code
         * if (some condition isn't met)
         * {
         *     // In a function returning void, you can validly 'return
         *     // expression' if the expression is itself of type void. But
         *     // returning is up to you; response.error() has no effect on
         *     // flow of control.
         *     return response.error("error message, logged and also sent as [\"error\"]");
         * }
         * @endcode
         */
        void error(const std::string& error);

        /**
         * set other keys...
         *
         * @code
         * // set any attributes you want to be sent in the reply
         * response["info"] = some_value;
         * // ...
         * response["ok"] = went_well;
         * @endcode
         */
        LLSD& operator[](const LLSD::String& key) { return mResp[key]; }
		
		 /**
		 * set the response to the given data
		 */
		void setResponse(LLSD const & response){ mResp = response; }

        LLSD mResp, mReq;
        LLSD::String mKey;
    };

private:
    std::string mDesc;
};

#endif /* ! defined(LL_LLEVENTAPI_H) */
