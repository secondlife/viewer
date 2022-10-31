/** 
 * @file lllogin.h
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

#ifndef LL_LLLOGIN_H
#define LL_LLLOGIN_H

#include <boost/scoped_ptr.hpp>

class LLSD;
class LLEventPump;

/**
 * @class LLLogin
 * @brief Class to encapsulate the action and state of grid login.
 */
class LLLogin
{
public:
    LLLogin();
    ~LLLogin();

    /** 
     * Make a connection to a grid.
     * @param uri The 'well known and published' authentication URL.
     * @param credentials LLSD data that contians the credentials.
     * *NOTE:Mani The credential data can vary depending upon the authentication
     * method used. The current interface matches the values passed to
     * the XMLRPC login request.
     {
        method          :   string, 
        first           :   string,
        last            :   string,
        passwd          :   string,
        start           :   string,
        skipoptional    :   bool,
        agree_to_tos    :   bool,
        read_critical   :   bool,
        last_exec_event :   int,
        version         :   string,
        channel         :   string,
        mac             :   string,
        id0             :   string,
        options         :   [ strings ]
     }
     
     */
    void connect(const std::string& uri, const LLSD& credentials);
    
    /** 
     * Disconnect from a the current connection.
     */
    void disconnect();

    /** 
     * Retrieve the event pump from this login class.
     */
    LLEventPump& getEventPump();

    /*
    Event API

    LLLogin will issue multiple events to it pump to indicate the 
    progression of states through login. The most important 
    states are "offline" and "online" which indicate auth failure 
    and auth success respectively.

    pump: login (tweaked)
    These are the events posted to the 'login' 
    event pump from the login module.
    {
        state       :   string, // See below for the list of states.
        progress    :   real // for progress bar.
        data        :   LLSD // Dependent upon state.
    }
    
    States for method 'login_to_simulator'
    offline - set initially state and upon failure. data is the server response.
    srvrequest - upon uri rewrite request. no data.
    authenticating - upon auth request. data, 'attempt' number and 'request' llsd.
    downloading - upon ack from auth server, before completion. no data
    online - upon auth success. data is server response.


    Dependencies:
    pump: LLAres 
    LLLogin makes a request for a SRV record from the uri provided by the connect method.
    The following event pump should exist to service that request.
    pump name: LLAres
    request = {
        op : "rewriteURI"
        uri : string
        reply : string

    pump: LLXMLRPCListener
    The request merely passes the credentials LLSD along, with one additional 
    member, 'reply', which is the string name of the event pump to reply on. 
    
    */

private:
    class Impl;
    boost::scoped_ptr<Impl> mImpl;
};

#endif // LL_LLLOGIN_H
