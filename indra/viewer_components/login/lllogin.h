/** 
 * @file lllogin.h
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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
		method			:	string, 
		first			:	string,
		last			:	string,
		passwd			:	string,
		start			:	string,
		skipoptional	:	bool,
		agree_to_tos	:	bool,
		read_critical	:	bool,
		last_exec_event	:	int,
		version			:	string,
		channel			:	string,
		mac				:	string,
		id0				:	string,
		options			:   [ strings ]
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
		state		:	string, // See below for the list of states.
		progress	:   real // for progress bar.
		data		:   LLSD // Dependent upon state.
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
