/**
 * @file httphandler.h
 * @brief Public-facing declarations for the HttpHandler class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef	_LLCORE_HTTP_HANDLER_H_
#define	_LLCORE_HTTP_HANDLER_H_


#include "httpcommon.h"


namespace LLCore
{

class HttpResponse;


/// HttpHandler defines an interface used by the library to
/// notify library callers of significant events, currently
/// request completion.  Callers must derive or mixin this class
/// then provide an implementation of the @see onCompleted
/// method to receive such notifications.  An instance may
/// be shared by any number of requests and across instances
/// of HttpRequest running in the same thread.
///
/// Threading:  HttpHandler itself is pure interface and is
/// tread-compatible.  Most derivations, however, will have
/// different constraints.
///
/// Allocation:  Not refcounted, may be stack allocated though
/// that is rarely a good idea.  Queued requests and replies keep
/// a naked pointer to the handler and this can result in a
/// dangling pointer if lifetimes aren't managed correctly.

class HttpHandler
{
public:
	virtual ~HttpHandler()
		{}

	/// Method invoked during calls to @see update().  Each invocation
	/// represents the completion of some requested operation.  Caller
	/// can identify the request from the handle and interrogate the
	/// response argument for success/failure, data and other information.
	///
	/// @param	handle			Identifier of the request generating
	///							the notification.
	/// @param	response		Supplies detailed information about
	///							the request including status codes
	///							(both programming and HTTP), HTTP body
	///							data and encodings, headers, etc.
	///							The response object is refcounted and
	///							the called code may retain the object
	///							by invoking @see addRef() on it.  The
	///							library itself drops all references to
	///							to object on return and never touches
	///							it again.
	///
	virtual void onCompleted(HttpHandle handle, HttpResponse * response) = 0;

};  // end class HttpHandler


}   // end namespace LLCore

#endif	// _LLCORE_HTTP_HANDLER_H_
