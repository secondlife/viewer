/**
 * @file llxmlrpctransaction.h
 * @brief LLXMLRPCTransaction and related class header file
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LLXMLRPCTRANSACTION_H
#define LLXMLRPCTRANSACTION_H

#include <string>

/// An asynchronous request and responses via XML-RPC
class LLXMLRPCTransaction
{
public:
    LLXMLRPCTransaction
    (
        const std::string& uri,
        const std::string& method,
        const LLSD& params,
        const LLSD& http_params = LLSD()
    );

    ~LLXMLRPCTransaction();

    typedef enum e_status
    {
        StatusNotStarted,
        StatusStarted,
        StatusDownloading,
        StatusComplete,
        StatusCURLError,
        StatusXMLRPCError,
        StatusOtherError
    } EStatus;

    /// Run the request a little, returns true when done
    bool process();

    /// Return a status, and extended CURL code, if code isn't null
    EStatus status(int* curlCode);

    LLSD getErrorCertData();

    /// Return a message string, suitable for showing the user
    std::string statusMessage();

    /// Return a URI for the user with more information (can be empty)
    std::string statusURI();

    /// Only non-empty if StatusComplete, otherwise Undefined
    const LLSD& response();

    /// Only valid if StsatusComplete, otherwise 0.0
    F64 transferRate();

private:
    class Handler;
    class Impl;

    Impl& impl;
};

#endif // LLXMLRPCTRANSACTION_H
