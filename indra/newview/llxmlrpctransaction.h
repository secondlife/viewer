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

typedef struct _xmlrpc_request* XMLRPC_REQUEST;
typedef struct _xmlrpc_value* XMLRPC_VALUE;
    // foward decl of types from xmlrpc.h (this usage is type safe)
class LLCertificate;

class LLXMLRPCValue
    // a c++ wrapper around XMLRPC_VALUE
{
public:
    LLXMLRPCValue()                     : mV(NULL) { }
    LLXMLRPCValue(XMLRPC_VALUE value)   : mV(value) { }
    
    bool isValid() const;
    
    std::string asString()  const;
    int         asInt()     const;
    bool        asBool()    const;
    double      asDouble()  const;

    LLXMLRPCValue operator[](const char*) const;
    
    LLXMLRPCValue rewind();
    LLXMLRPCValue next();

    static LLXMLRPCValue createArray();
    static LLXMLRPCValue createStruct();

    void append(LLXMLRPCValue&);
    void appendString(const std::string&);
    void appendInt(int);
    void appendBool(bool);
    void appendDouble(double);
    void appendValue(LLXMLRPCValue&);

    void append(const char*, LLXMLRPCValue&);
    void appendString(const char*, const std::string&);
    void appendInt(const char*, int);
    void appendBool(const char*, bool);
    void appendDouble(const char*, double);
    void appendValue(const char*, LLXMLRPCValue&);
    
    void cleanup();
        // only call this on the top level created value
    
    XMLRPC_VALUE getValue() const;
    
private:
    XMLRPC_VALUE mV;
};


class LLXMLRPCTransaction
    // an asynchronous request and responses via XML-RPC
{
public:
    LLXMLRPCTransaction(const std::string& uri,
        XMLRPC_REQUEST request, bool useGzip = true, const LLSD& httpParams = LLSD());
        // does not take ownership of the request object
        // request can be freed as soon as the transaction is constructed

    LLXMLRPCTransaction(const std::string& uri,
        const std::string& method, LLXMLRPCValue params, bool useGzip = true);
        // *does* take control of the request value, you must not free it
        
    ~LLXMLRPCTransaction();
    
    typedef enum e_status {
        StatusNotStarted,
        StatusStarted,
        StatusDownloading,
        StatusComplete,
        StatusCURLError,
        StatusXMLRPCError,
        StatusOtherError
    } EStatus;

    bool process();
        // run the request a little, returns true when done
        
    EStatus status(int* curlCode);
        // return status, and extended CURL code, if code isn't null
    
    LLSD getErrorCertData();
    std::string statusMessage();
        // return a message string, suitable for showing the user
    std::string statusURI();
        // return a URI for the user with more information
        // can be empty

    XMLRPC_REQUEST response();
    LLXMLRPCValue responseValue();
        // only valid if StatusComplete, otherwise NULL
        // retains ownership of the result object, don't free it
    
    F64 transferRate();
        // only valid if StsatusComplete, otherwise 0.0
        
private:
    class Handler;
    class Impl;

    Impl& impl;
};



#endif // LLXMLRPCTRANSACTION_H
