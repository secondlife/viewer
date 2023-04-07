/**
 * @file   llxmlrpclistener.cpp
 * @author Nat Goodspeed
 * @date   2009-03-18
 * @brief  Implementation for llxmlrpclistener.
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
#include "llxmlrpclistener.h"
// STL headers
#include <map>
#include <set>
// std headers
// external library headers
#include <boost/scoped_ptr.hpp>
#include <boost/range.hpp>          // boost::begin(), boost::end()

#ifdef LL_USESYSTEMLIBS
#include <xmlrpc.h>
#else
#include <xmlrpc-epi/xmlrpc.h>
#endif

#include "curl/curl.h"

// other Linden headers
#include "llerror.h"
#include "lleventcoro.h"
#include "stringize.h"
#include "llxmlrpctransaction.h"
#include "llsecapi.h"

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

template <typename STATUS>
class StatusMapperBase
{
    typedef std::map<STATUS, std::string> MapType;

public:
    StatusMapperBase(const std::string& desc):
        mDesc(desc)
    {}

    std::string lookup(STATUS status) const
    {
        typename MapType::const_iterator found = mMap.find(status);
        if (found != mMap.end())
        {
            return found->second;
        }
        return STRINGIZE("<unknown " << mDesc << " " << status << ">");
    }

protected:
    std::string mDesc;
    MapType mMap;
};

class StatusMapper: public StatusMapperBase<LLXMLRPCTransaction::EStatus>
{
public:
    StatusMapper(): StatusMapperBase<LLXMLRPCTransaction::EStatus>("Status")
    {
		mMap[LLXMLRPCTransaction::StatusNotStarted]  = "NotStarted";
		mMap[LLXMLRPCTransaction::StatusStarted]     = "Started";
		mMap[LLXMLRPCTransaction::StatusDownloading] = "Downloading";
		mMap[LLXMLRPCTransaction::StatusComplete]    = "Complete";
		mMap[LLXMLRPCTransaction::StatusCURLError]   = "CURLError";
		mMap[LLXMLRPCTransaction::StatusXMLRPCError] = "XMLRPCError";
		mMap[LLXMLRPCTransaction::StatusOtherError]  = "OtherError";
    }
};

static const StatusMapper sStatusMapper;

class CURLcodeMapper: public StatusMapperBase<CURLcode>
{
public:
    CURLcodeMapper(): StatusMapperBase<CURLcode>("CURLcode")
    {
        // from curl.h
// skip the "CURLE_" prefix for each of these strings
#define def(sym) (mMap[sym] = &#sym[6])
        def(CURLE_OK);
        def(CURLE_UNSUPPORTED_PROTOCOL);    /* 1 */
        def(CURLE_FAILED_INIT);             /* 2 */
        def(CURLE_URL_MALFORMAT);           /* 3 */
        def(CURLE_COULDNT_RESOLVE_PROXY);   /* 5 */
        def(CURLE_COULDNT_RESOLVE_HOST);    /* 6 */
        def(CURLE_COULDNT_CONNECT);         /* 7 */
        def(CURLE_PARTIAL_FILE);            /* 18 */
        def(CURLE_HTTP_RETURNED_ERROR);     /* 22 */
        def(CURLE_WRITE_ERROR);             /* 23 */
        def(CURLE_UPLOAD_FAILED);           /* 25 - failed upload "command" */
        def(CURLE_READ_ERROR);              /* 26 - could open/read from file */
        def(CURLE_OUT_OF_MEMORY);           /* 27 */
        /* Note: CURLE_OUT_OF_MEMORY may sometimes indicate a conversion error
                 instead of a memory allocation error if CURL_DOES_CONVERSIONS
                 is defined
        */
        def(CURLE_OPERATION_TIMEDOUT);     /* 28 - the timeout time was reached */
        def(CURLE_HTTP_RANGE_ERROR);        /* 33 - RANGE "command" didn't work */
        def(CURLE_HTTP_POST_ERROR);         /* 34 */
        def(CURLE_SSL_CONNECT_ERROR);       /* 35 - wrong when connecting with SSL */
        def(CURLE_BAD_DOWNLOAD_RESUME);     /* 36 - couldn't resume download */
        def(CURLE_FILE_COULDNT_READ_FILE);  /* 37 */
        def(CURLE_LIBRARY_NOT_FOUND);       /* 40 */
        def(CURLE_FUNCTION_NOT_FOUND);      /* 41 */
        def(CURLE_ABORTED_BY_CALLBACK);     /* 42 */
        def(CURLE_BAD_FUNCTION_ARGUMENT);   /* 43 */
        def(CURLE_INTERFACE_FAILED);        /* 45 - CURLOPT_INTERFACE failed */
        def(CURLE_TOO_MANY_REDIRECTS );     /* 47 - catch endless re-direct loops */
        def(CURLE_SSL_PEER_CERTIFICATE);    /* 51 - peer's certificate wasn't ok */
        def(CURLE_GOT_NOTHING);             /* 52 - when this is a specific error */
        def(CURLE_SSL_ENGINE_NOTFOUND);     /* 53 - SSL crypto engine not found */
        def(CURLE_SSL_ENGINE_SETFAILED);    /* 54 - can not set SSL crypto engine as
                                          default */
        def(CURLE_SEND_ERROR);              /* 55 - failed sending network data */
        def(CURLE_RECV_ERROR);              /* 56 - failure in receiving network data */

        def(CURLE_SSL_CERTPROBLEM);         /* 58 - problem with the local certificate */
        def(CURLE_SSL_CIPHER);              /* 59 - couldn't use specified cipher */
        def(CURLE_SSL_CACERT);              /* 60 - problem with the CA cert (path?) */
        def(CURLE_BAD_CONTENT_ENCODING);    /* 61 - Unrecognized transfer encoding */

        def(CURLE_FILESIZE_EXCEEDED);       /* 63 - Maximum file size exceeded */

        def(CURLE_SEND_FAIL_REWIND);        /* 65 - Sending the data requires a rewind
                                          that failed */
        def(CURLE_SSL_ENGINE_INITFAILED);   /* 66 - failed to initialise ENGINE */
        def(CURLE_LOGIN_DENIED);            /* 67 - user); password or similar was not
                                          accepted and we failed to login */
        def(CURLE_CONV_FAILED);             /* 75 - conversion failed */
        def(CURLE_CONV_REQD);               /* 76 - caller must register conversion
                                          callbacks using curl_easy_setopt options
                                          CURLOPT_CONV_FROM_NETWORK_FUNCTION);
                                          CURLOPT_CONV_TO_NETWORK_FUNCTION); and
                                          CURLOPT_CONV_FROM_UTF8_FUNCTION */
        def(CURLE_SSL_CACERT_BADFILE);      /* 77 - could not load CACERT file); missing
                                          or wrong format */
        def(CURLE_REMOTE_FILE_NOT_FOUND);   /* 78 - remote file not found */
        def(CURLE_SSH);                     /* 79 - error from the SSH layer); somewhat
                                          generic so the error message will be of
                                          interest when this has happened */

        def(CURLE_SSL_SHUTDOWN_FAILED);     /* 80 - Failed to shut down the SSL
                                          connection */
#undef  def
    }
};

static const CURLcodeMapper sCURLcodeMapper;

LLXMLRPCListener::LLXMLRPCListener(const std::string& pumpname):
    mBoundListener(LLEventPumps::instance().
                   obtain(pumpname).
                   listen("LLXMLRPCListener", boost::bind(&LLXMLRPCListener::process, this, _1)))
{
}

/**
 * Capture an outstanding LLXMLRPCTransaction and poll it periodically until
 * done.
 *
 * The sequence is:
 * # Instantiate Poller, which instantiates, populates and initiates an
 *   LLXMLRPCTransaction. Poller self-registers on the LLEventPump named
 *   "mainloop".
 * # "mainloop" is conventionally pumped once per frame. On each such call,
 *   Poller checks its LLXMLRPCTransaction for completion.
 * # When the LLXMLRPCTransaction completes, Poller collects results (if any)
 *   and sends notification.
 * # The tricky part: Poller frees itself (and thus its LLXMLRPCTransaction)
 *   when done. The only external reference to it is the connection to the
 *   "mainloop" LLEventPump.
 */
class Poller
{
public:
    /// Validate the passed request for required fields, then use it to
    /// populate an XMLRPC_REQUEST and an associated LLXMLRPCTransaction. Send
    /// the request.
    Poller(const LLSD& command):
        mReqID(command),
        mUri(command["uri"]),
        mMethod(command["method"]),
        mReplyPump(command["reply"])
    {
        // LL_ERRS if any of these are missing
        const char* required[] = { "uri", "method", "reply" };
        // optional: "options" (array of string)
        // Validate the request
        std::set<std::string> missing;
        for (const char** ri = boost::begin(required); ri != boost::end(required); ++ri)
        {
            // If the command does not contain this required entry, add it to 'missing'.
            if (! command.has(*ri))
            {
                missing.insert(*ri);
            }
        }
        if (! missing.empty())
        {
            LL_ERRS("LLXMLRPCListener") << mMethod << " request missing params: ";
            const char* separator = "";
            for (std::set<std::string>::const_iterator mi(missing.begin()), mend(missing.end());
                 mi != mend; ++mi)
            {
                LL_CONT << separator << *mi;
                separator = ", ";
            }
            LL_CONT << LL_ENDL;
        }

        // Build the XMLRPC request.
        XMLRPC_REQUEST request = XMLRPC_RequestNew();
        XMLRPC_RequestSetMethodName(request, mMethod.c_str());
        XMLRPC_RequestSetRequestType(request, xmlrpc_request_call);
        XMLRPC_VALUE xparams = XMLRPC_CreateVector(NULL, xmlrpc_vector_struct);
        LLSD params(command["params"]);
        if (params.isMap())
        {
            for (LLSD::map_const_iterator pi(params.beginMap()), pend(params.endMap());
                 pi != pend; ++pi)
            {
                std::string name(pi->first);
                LLSD param(pi->second);
                if (param.isString())
                {
                    XMLRPC_VectorAppendString(xparams, name.c_str(), param.asString().c_str(), 0);
                }
                else if (param.isInteger() || param.isBoolean())
                {
                    XMLRPC_VectorAppendInt(xparams, name.c_str(), param.asInteger());
                }
                else if (param.isReal())
                {
                    XMLRPC_VectorAppendDouble(xparams, name.c_str(), param.asReal());
                }
                else
                {
                    LL_ERRS("LLXMLRPCListener") << mMethod << " request param "
                                                << name << " has unknown type: " << param << LL_ENDL;
                }
            }
        }
        LLSD options(command["options"]);
        if (options.isArray())
        {
            XMLRPC_VALUE xoptions = XMLRPC_CreateVector("options", xmlrpc_vector_array);
            for (LLSD::array_const_iterator oi(options.beginArray()), oend(options.endArray());
                 oi != oend; ++oi)
            {
                XMLRPC_VectorAppendString(xoptions, NULL, oi->asString().c_str(), 0);
            }
            XMLRPC_AddValueToVector(xparams, xoptions);
        }
        XMLRPC_RequestSetData(request, xparams);

        mTransaction.reset(new LLXMLRPCTransaction(mUri, request, true, command.has("http_params")? LLSD(command["http_params"]) : LLSD()));
		mPreviousStatus = mTransaction->status(NULL);

        // Free the XMLRPC_REQUEST object and the attached data values.
        XMLRPC_RequestFree(request, 1);

        // Now ensure that we get regular callbacks to poll for completion.
        mBoundListener =
            LLEventPumps::instance().
            obtain("mainloop").
            listen(LLEventPump::ANONYMOUS, boost::bind(&Poller::poll, this, _1));

        LL_INFOS("LLXMLRPCListener") << mMethod << " request sent to " << mUri << LL_ENDL;
    }

    /// called by "mainloop" LLEventPump
    bool poll(const LLSD&)
    {
        bool done = mTransaction->process();

        CURLcode curlcode;
        LLXMLRPCTransaction::EStatus status;
        {
            // LLXMLRPCTransaction::status() is defined to accept int* rather
            // than CURLcode*. I don't feel the urge to fix the signature, but
            // we want a CURLcode rather than an int. So fetch it as a local
            // int, but then assign to a CURLcode for the remainder of this
            // method.
            int curlint;
            status = mTransaction->status(&curlint);
            curlcode = CURLcode(curlint);
        }

        LLSD data(mReqID.makeResponse());
        data["status"] = sStatusMapper.lookup(status);
        data["errorcode"] = sCURLcodeMapper.lookup(curlcode);
        data["error"] = "";
        data["transfer_rate"] = 0.0;
        LLEventPump& replyPump(LLEventPumps::instance().obtain(mReplyPump));
		if (! done)
        {
            // Not done yet, carry on.
			if (status == LLXMLRPCTransaction::StatusDownloading
				&& status != mPreviousStatus)
			{
				// If a response has been received, send the 
				// 'downloading' status if it hasn't been sent.
				replyPump.post(data);
			}

			mPreviousStatus = status;
            return false;
        }

        // Here the transaction is complete. Check status.
        data["error"] = mTransaction->statusMessage();
		data["transfer_rate"] = mTransaction->transferRate();
        LL_INFOS("LLXMLRPCListener") << mMethod << " result from " << mUri << ": status "
                                     << data["status"].asString() << ", errorcode "
                                     << data["errorcode"].asString()
                                     << " (" << data["error"].asString() << ")"
                                     << LL_ENDL;
		
		switch (curlcode)
		{
#if CURLE_SSL_PEER_CERTIFICATE != CURLE_SSL_CACERT
			case CURLE_SSL_PEER_CERTIFICATE:
#endif
			case CURLE_SSL_CACERT:
                data["certificate"] = mTransaction->getErrorCertData();
				break;

			default:
				break;
		}
        // values of 'curlcode':
        // CURLE_COULDNT_RESOLVE_HOST,
        // CURLE_SSL_PEER_CERTIFICATE,
        // CURLE_SSL_CACERT,
        // CURLE_SSL_CONNECT_ERROR.
        // Given 'message', need we care?
        if (status == LLXMLRPCTransaction::StatusComplete)
        {
            // Success! Parse data.
            std::string status_string(data["status"]);
            data["responses"] = parseResponse(status_string);
            data["status"] = status_string;
        }

        // whether successful or not, send reply on requested LLEventPump
        replyPump.post(data);
        // need to wake up the loginCoro now
        llcoro::suspend();

        // Because mTransaction is a boost::scoped_ptr, deleting this object
        // frees our LLXMLRPCTransaction object.
        // Because mBoundListener is an LLTempBoundListener, deleting this
        // object disconnects it from "mainloop".
        // *** MUST BE LAST ***
        delete this;
        return false;
    }

private:
    /// Derived from LLUserAuth::parseResponse() and parseOptionInto()
    LLSD parseResponse(std::string& status_string)
    {
        // Extract every member into data["responses"] (a map of string
        // values).
        XMLRPC_REQUEST response = mTransaction->response();
        if (! response)
        {
            LL_DEBUGS("LLXMLRPCListener") << "No response" << LL_ENDL;
            return LLSD();
        }

        XMLRPC_VALUE param = XMLRPC_RequestGetData(response);
        if (! param)
        {
            LL_DEBUGS("LLXMLRPCListener") << "Response contains no data" << LL_ENDL;
            return LLSD();
        }

        // Now, parse everything
        return parseValues(status_string, "", param);
    }

    /**
     * Parse key/value pairs from a given XMLRPC_VALUE into an LLSD map.
     * @param key_pfx Used to describe a given key in log messages. At top
     * level, pass "". When parsing an options array, pass the top-level key
     * name of the array plus the index of the array entry; to this we'll
     * append the subkey of interest.
     * @param param XMLRPC_VALUE iterator. At top level, pass
     * XMLRPC_RequestGetData(XMLRPC_REQUEST).
     */
    LLSD parseValues(std::string& status_string, const std::string& key_pfx, XMLRPC_VALUE param)
    {
        LLSD responses;
        for (XMLRPC_VALUE current = XMLRPC_VectorRewind(param); current;
             current = XMLRPC_VectorNext(param))
        {
            std::string key(XMLRPC_GetValueID(current));
            LL_DEBUGS("LLXMLRPCListener") << "key: " << key_pfx << key << LL_ENDL;
            XMLRPC_VALUE_TYPE_EASY type = XMLRPC_GetValueTypeEasy(current);
            switch (type)
            {
            case xmlrpc_type_empty:
                LL_INFOS("LLXMLRPCListener") << "Empty result for key " << key_pfx << key << LL_ENDL;
                responses.insert(key, LLSD());
                break;
            case xmlrpc_type_base64:
                {
                    S32 len = XMLRPC_GetValueStringLen(current);
                    const char* buf = XMLRPC_GetValueBase64(current);
                    if ((len > 0) && buf)
                    {
                        // During implementation this code was not tested
                        // If you encounter this, please make sure this is correct,
                        // then remove llassert
                        llassert(0);

                        LLSD::Binary data;
                        data.resize(len);
                        memcpy((void*)&data[0], (void*)buf, len);
                        responses.insert(key, data);
                    }
                    else
                    {
                        LL_WARNS("LLXMLRPCListener") << "Potentially malformed xmlrpc_type_base64 for key "
                            << key_pfx << key << LL_ENDL;
                        responses.insert(key, LLSD());
                    }
                    break;
                }
            case xmlrpc_type_boolean:
                {
                    LLSD::Boolean val(XMLRPC_GetValueBoolean(current));
                    LL_DEBUGS("LLXMLRPCListener") << "val: " << val << LL_ENDL;
                    responses.insert(key, val);
                    break;
                }
            case xmlrpc_type_datetime:
                {
                    std::string iso8601_date(XMLRPC_GetValueDateTime_ISO8601(current));
                    LL_DEBUGS("LLXMLRPCListener") << "val: " << iso8601_date << LL_ENDL;
                    responses.insert(key, LLSD::Date(iso8601_date));
                    break;
                }
            case xmlrpc_type_double:
                {
                    LLSD::Real val(XMLRPC_GetValueDouble(current));
                    LL_DEBUGS("LLXMLRPCListener") << "val: " << val << LL_ENDL;
                    responses.insert(key, val);
                    break;
                }
            case xmlrpc_type_int:
                {
                    LLSD::Integer val(XMLRPC_GetValueInt(current));
                    LL_DEBUGS("LLXMLRPCListener") << "val: " << val << LL_ENDL;
                    responses.insert(key, val);
                    break;
                }
            case xmlrpc_type_string:
                {
                    LLSD::String val(XMLRPC_GetValueString(current));
                    LL_DEBUGS("LLXMLRPCListener") << "val: " << val << LL_ENDL;
                    responses.insert(key, val);
                    break;
                }
            case xmlrpc_type_mixed:
            case xmlrpc_type_array:
                {
                    // We expect this to be an array of submaps. Walk the array,
                    // recursively parsing each submap and collecting them.
                    LLSD array;
                    int i = 0;          // for descriptive purposes
                    for (XMLRPC_VALUE row = XMLRPC_VectorRewind(current); row;
                         row = XMLRPC_VectorNext(current), ++i)
                    {
                        // Recursive call. For the lower-level key_pfx, if 'key'
                        // is "foo", pass "foo[0]:", then "foo[1]:", etc. In the
                        // nested call, a subkey "bar" will then be logged as
                        // "foo[0]:bar", and so forth.
                        // Parse the scalar subkey/value pairs from this array
                        // entry into a temp submap. Collect such submaps in 'array'.
                        array.append(parseValues(status_string,
                                                 STRINGIZE(key_pfx << key << '[' << i << "]:"),
                                                 row));
                    }
                    // Having collected an 'array' of 'submap's, insert that whole
                    // 'array' as the value of this 'key'.
                    responses.insert(key, array);
                    break;
                }
            case xmlrpc_type_struct:
                {
                    LLSD submap = parseValues(status_string,
                                              STRINGIZE(key_pfx << key << ':'),
                                              current);
                    responses.insert(key, submap);
                    break;
                }
            case xmlrpc_type_none: // Not expected
            default:
                // whoops - unrecognized type
                LL_WARNS("LLXMLRPCListener") << "Unhandled xmlrpc type " << type << " for key "
                    << key_pfx << key << LL_ENDL;
                responses.insert(key, STRINGIZE("<bad XMLRPC type " << type << '>'));
                status_string = "BadType";
            }
        }
        return responses;
    }

    const LLReqID mReqID;
    const std::string mUri;
    const std::string mMethod;
    const std::string mReplyPump;
    LLTempBoundListener mBoundListener;
    boost::scoped_ptr<LLXMLRPCTransaction> mTransaction;
	LLXMLRPCTransaction::EStatus mPreviousStatus; // To detect state changes.
};

bool LLXMLRPCListener::process(const LLSD& command)
{
    // Allocate a new heap Poller, but do not save a pointer to it. Poller
    // will check its own status and free itself on completion of the request.
    (new Poller(command));
    // conventional event listener return
    return false;
}
