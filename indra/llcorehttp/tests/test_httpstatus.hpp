/** 
 * @file test_llrefcounted
 * @brief unit tests for HttpStatus struct
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012-2013, Linden Research, Inc.
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

#ifndef TEST_HTTP_STATUS_H_
#define TEST_HTTP_STATUS_H_

#include "httpcommon.h"

#include <curl/curl.h>
#include <curl/multi.h>

using namespace LLCore;

namespace tut
{

struct HttpStatusTestData
{
    HttpStatusTestData()
        {}
};

typedef test_group<HttpStatusTestData> HttpStatusTestGroupType;
typedef HttpStatusTestGroupType::object HttpStatusTestObjectType;

HttpStatusTestGroupType HttpStatusTestGroup("HttpStatus Tests");

template <> template <>
void HttpStatusTestObjectType::test<1>()
{
    set_test_name("HttpStatus construction");
    
    // auto allocation fine for this
    HttpStatus status;

    status = HttpStatus(HttpStatus::EXT_CURL_EASY, 0);
    
    ensure(bool(status));
    ensure(false == !(status));

    status = HttpStatus(HttpStatus::EXT_CURL_MULTI, 0);

    ensure(bool(status));
    ensure(false == !(status));

    status = HttpStatus(HttpStatus::LLCORE, HE_SUCCESS);
    
    ensure(bool(status));
    ensure(false == !(status));

    status = HttpStatus(HttpStatus::EXT_CURL_MULTI, -1);

    ensure(false == bool(status));
    ensure(!(status));

    status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_BAD_DOWNLOAD_RESUME);

    ensure(false == bool(status));
    ensure(!(status));
}


// template <> template <>
// void HttpStatusTestObjectType::test<2>()
// {
//  set_test_name("HttpStatus memory structure");
// 
//  // Require that an HttpStatus object can be trivially
//  // returned as a function return value in registers.
//  // One should fit in an int on all platforms.
// 
//  //ensure(sizeof(HttpStatus) <= sizeof(int));
// }


template <> template <>
void HttpStatusTestObjectType::test<2>()
{
    set_test_name("HttpStatus valid status string conversion");
    
    HttpStatus status = HttpStatus(HttpStatus::EXT_CURL_EASY, 0);
    std::string msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(msg.empty());

    status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_BAD_FUNCTION_ARGUMENT);
    msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());

    status = HttpStatus(HttpStatus::EXT_CURL_MULTI, CURLM_OUT_OF_MEMORY);
    msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());

    status = HttpStatus(HttpStatus::LLCORE, HE_SHUTTING_DOWN);
    msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());
}


template <> template <>
void HttpStatusTestObjectType::test<3>()
{
    set_test_name("HttpStatus invalid status string conversion");
    
    HttpStatus status = HttpStatus(HttpStatus::EXT_CURL_EASY, 32726);
    std::string msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());

    status = HttpStatus(HttpStatus::EXT_CURL_MULTI, -470);
    msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());

    status = HttpStatus(HttpStatus::LLCORE, 923);
    msg = status.toString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure(! msg.empty());
}

template <> template <>
void HttpStatusTestObjectType::test<4>()
{
    set_test_name("HttpStatus equality/inequality testing");

    // Make certain equality/inequality tests do not pass
    // through the bool conversion.  Distinct successful
    // and error statuses should compare unequal.

    HttpStatus status1(HttpStatus::LLCORE, HE_SUCCESS);
    HttpStatus status2(HttpStatus::EXT_CURL_EASY, HE_SUCCESS);
    ensure(status1 != status2);

    status1 = HttpStatus(HttpStatus::LLCORE, HE_REPLY_ERROR);
    status1 = HttpStatus(HttpStatus::LLCORE, HE_SHUTTING_DOWN);

    ensure(status1 != status2);
}

template <> template <>
void HttpStatusTestObjectType::test<5>()
{
    set_test_name("HttpStatus basic HTTP status encoding");
    
    HttpStatus status;

    status = HttpStatus(200, HE_SUCCESS);
    std::string msg = status.toString();
    ensure(msg.empty());
    ensure(bool(status));

    // Normally a success but application says error
    status = HttpStatus(200, HE_REPLY_ERROR);
    msg = status.toString();
    ensure(! msg.empty());
    ensure(! bool(status));
    ensure(status.toULong() > 1UL);             // Biggish number, not a bool-to-ulong

    // Same statuses with distinct success/fail are distinct
    status = HttpStatus(200, HE_SUCCESS);
    HttpStatus status2(200, HE_REPLY_ERROR);
    ensure(status != status2);

    // Normally an error but application says okay
    status = HttpStatus(406, HE_SUCCESS);
    msg = status.toString();
    ensure(msg.empty());
    ensure(bool(status));

    // Different statuses but both successful are distinct
    status = HttpStatus(200, HE_SUCCESS);
    status2 = HttpStatus(201, HE_SUCCESS);
    ensure(status != status2);

    // Different statuses but both failed are distinct
    status = HttpStatus(200, HE_REPLY_ERROR);
    status2 = HttpStatus(201, HE_REPLY_ERROR);
    ensure(status != status2);
}

template <> template <>
void HttpStatusTestObjectType::test<6>()
{
    set_test_name("HttpStatus HTTP status text strings");

    HttpStatus status(100, HE_REPLY_ERROR);
    std::string msg(status.toString());
    ensure(! msg.empty());              // Should be something
    ensure(msg == "Continue");

    status = HttpStatus(200, HE_SUCCESS);
    msg = status.toString();
    ensure(msg.empty());                // Success is empty

    status = HttpStatus(199, HE_REPLY_ERROR);
    msg = status.toString();
    ensure(msg == "Unknown error");

    status = HttpStatus(505, HE_REPLY_ERROR);
    msg = status.toString();
    ensure(msg == "HTTP Version not supported");

    status = HttpStatus(506, HE_REPLY_ERROR);
    msg = status.toString();
    ensure(msg == "Unknown error");

    status = HttpStatus(999, HE_REPLY_ERROR);
    msg = status.toString();
    ensure(msg == "Unknown error");
}


template <> template <>
void HttpStatusTestObjectType::test<7>()
{
    set_test_name("HttpStatus toHex() nominal function");
    
    HttpStatus status(404);
    std::string msg = status.toHex();
    // std::cout << "Result:  " << msg << std::endl;
    ensure_equals(msg, "01940001");
}


template <> template <>
void HttpStatusTestObjectType::test<8>()
{
    set_test_name("HttpStatus toTerseString() nominal function");
    
    HttpStatus status(404);
    std::string msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Normal HTTP 404", msg == "Http_404");

    status = HttpStatus(200);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Normal HTTP 200", msg == "Http_200");

    status = HttpStatus(200, HE_REPLY_ERROR);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Unsuccessful HTTP 200", msg == "Http_200");         // No distinction for error

    status = HttpStatus(HttpStatus::EXT_CURL_EASY, CURLE_COULDNT_CONNECT);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Easy couldn't connect error", msg == "Easy_7");

    status = HttpStatus(HttpStatus::EXT_CURL_MULTI, CURLM_OUT_OF_MEMORY);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Multi out-of-memory error", msg == "Multi_3");

    status = HttpStatus(HttpStatus::LLCORE, HE_OPT_NOT_SET);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Core option not set error", msg == "Core_7");

    status = HttpStatus(22000, 1);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Undecodable error", msg == "Unknown_1");

    status = HttpStatus(22000, -1);
    msg = status.toTerseString();
    // std::cout << "Result:  " << msg << std::endl;
    ensure("Undecodable error 65535", msg == "Unknown_65535");
}

} // end namespace tut

#endif  // TEST_HTTP_STATUS_H

