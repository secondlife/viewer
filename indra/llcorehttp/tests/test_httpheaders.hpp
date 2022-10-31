/** 
 * @file test_httpheaders.hpp
 * @brief unit tests for the LLCore::HttpHeaders class
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
#ifndef TEST_LLCORE_HTTP_HEADERS_H_
#define TEST_LLCORE_HTTP_HEADERS_H_

#include "httpheaders.h"

#include <iostream>


using namespace LLCoreInt;


namespace tut
{

struct HttpHeadersTestData
{
    // the test objects inherit from this so the member functions and variables
    // can be referenced directly inside of the test functions.
};

typedef test_group<HttpHeadersTestData> HttpHeadersTestGroupType;
typedef HttpHeadersTestGroupType::object HttpHeadersTestObjectType;
HttpHeadersTestGroupType HttpHeadersTestGroup("HttpHeaders Tests");

template <> template <>
void HttpHeadersTestObjectType::test<1>()
{
    set_test_name("HttpHeaders construction");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());
    ensure("Nothing in headers", 0 == headers->size());

    // release the implicit reference, causing the object to be released
    headers.reset();
}

template <> template <>
void HttpHeadersTestObjectType::test<2>()
{
    set_test_name("HttpHeaders construction");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());
    
    {
        // Append a few strings
        std::string str1n("Pragma");
        std::string str1v("");
        headers->append(str1n, str1v);
        std::string str2n("Accept");
        std::string str2v("application/json");
        headers->append(str2n, str2v);
    
        ensure("Headers retained", 2 == headers->size());
        HttpHeaders::container_t & c(headers->getContainerTESTONLY());
        
        ensure("First name is first name", c[0].first == str1n);
        ensure("First value is first value", c[0].second == str1v);
        ensure("Second name is second name", c[1].first == str2n);
        ensure("Second value is second value", c[1].second == str2v);
    }
    
    // release the implicit reference, causing the object to be released
    headers.reset();
}

template <> template <>
void HttpHeadersTestObjectType::test<3>()
{
    set_test_name("HttpHeaders basic find");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());
    
    {
        // Append a few strings
        std::string str1n("Uno");
        std::string str1v("1");
        headers->append(str1n, str1v);
        std::string str2n("doS");
        std::string str2v("2-2-2-2");
        headers->append(str2n, str2v);
        std::string str3n("TRES");
        std::string str3v("trois gymnopedie");
        headers->append(str3n, str3v);
    
        ensure("Headers retained", 3 == headers->size());

        const std::string * result(NULL);

        // Find a header
        result = headers->find("TRES");
        ensure("Found the last item", result != NULL);
        ensure("Last item is a nice", result != NULL && str3v == *result);

        // appends above are raw and find is case sensitive
        result = headers->find("TReS");
        ensure("Last item not found due to case", result == NULL);

        result = headers->find("TRE");
        ensure("Last item not found due to prefixing (1)", result == NULL);

        result = headers->find("TRESS");
        ensure("Last item not found due to prefixing (2)", result == NULL);
    }
    
    // release the implicit reference, causing the object to be released
    headers.reset();
}

template <> template <>
void HttpHeadersTestObjectType::test<4>()
{
    set_test_name("HttpHeaders normalized header entry");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());

    {
        static char line1[] = " AcCePT : image/yourfacehere";
        static char line1v[] = "image/yourfacehere";
        headers->appendNormal(line1, sizeof(line1) - 1);
        
        ensure("First append worked in some fashion", 1 == headers->size());

        const std::string * result(NULL);

        // Find a header
        result = headers->find("accept");
        ensure("Found 'accept'", result != NULL);
        ensure("accept value has face", result != NULL && *result == line1v);

        // Left-clean on value
        static char line2[] = " next : \t\tlinejunk \t";
        headers->appendNormal(line2, sizeof(line2) - 1);
        ensure("Second append worked", 2 == headers->size());
        result = headers->find("next");
        ensure("Found 'next'", result != NULL);
        ensure("next value is left-clean", result != NULL &&
               *result == "linejunk \t");

        // First value unmolested
        result = headers->find("accept");
        ensure("Found 'accept' again", result != NULL);
        ensure("accept value has face", result != NULL && *result == line1v);

        // Colons in value are okay
        static char line3[] = "FancY-PANTs::plop:-neuf-=vleem=";
        static char line3v[] = ":plop:-neuf-=vleem=";
        headers->appendNormal(line3, sizeof(line3) - 1);
        ensure("Third append worked", 3 == headers->size());
        result = headers->find("fancy-pants");
        ensure("Found 'fancy-pants'", result != NULL);
        ensure("fancy-pants value has colons", result != NULL && *result == line3v);

        // Zero-length value
        static char line4[] = "all-talk-no-walk:";
        headers->appendNormal(line4, sizeof(line4) - 1);
        ensure("Fourth append worked", 4 == headers->size());
        result = headers->find("all-talk-no-walk");
        ensure("Found 'all-talk'", result != NULL);
        ensure("al-talk value is zero-length", result != NULL && result->size() == 0);

        // Zero-length name
        static char line5[] = ":all-talk-no-walk";
        static char line5v[] = "all-talk-no-walk";
        headers->appendNormal(line5, sizeof(line5) - 1);
        ensure("Fifth append worked", 5 == headers->size());
        result = headers->find("");
        ensure("Found no-name", result != NULL);
        ensure("no-name value is something", result != NULL && *result == line5v);

        // Lone colon is still something
        headers->clear();
        static char line6[] = "  :";
        headers->appendNormal(line6, sizeof(line6) - 1);
        ensure("Sixth append worked", 1 == headers->size());
        result = headers->find("");
        ensure("Found 2nd no-name", result != NULL);
        ensure("2nd no-name value is nothing", result != NULL && result->size() == 0);

        // Line without colons is taken as-is and unstripped in name
        static char line7[] = " \toskdgioasdghaosdghoowg28342908tg8902hg0hwedfhqew890v7qh0wdebv78q0wdevbhq>?M>BNM<ZV>?NZ? \t";
        headers->appendNormal(line7, sizeof(line7) - 1);
        ensure("Seventh append worked", 2 == headers->size());
        result = headers->find(line7);
        ensure("Found whatsit line", result != NULL);
        ensure("Whatsit line has no value", result != NULL && result->size() == 0);

        // Normaling interface heeds the byte count, doesn't look for NUL-terminator
        static char line8[] = "binary:ignorestuffontheendofthis";
        headers->appendNormal(line8, 13);
        ensure("Eighth append worked", 3 == headers->size());
        result = headers->find("binary");
        ensure("Found 'binary'", result != NULL);
        ensure("binary value was limited to 'ignore'", result != NULL &&
               *result == "ignore");

    }
    
    // release the implicit reference, causing the object to be released
    headers.reset();
}

// Verify forward iterator finds everything as expected
template <> template <>
void HttpHeadersTestObjectType::test<5>()
{
    set_test_name("HttpHeaders iterator tests");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());

    HttpHeaders::iterator end(headers->end()), begin(headers->begin());
    ensure("Empty container has equal begin/end const iterators", end == begin);
    HttpHeaders::const_iterator cend(headers->end()), cbegin(headers->begin());
    ensure("Empty container has equal rbegin/rend const iterators", cend == cbegin);

    ensure("Empty container has equal begin/end iterators", headers->end() == headers->begin());
    
    {
        static char line1[] = " AcCePT : image/yourfacehere";
        static char line1v[] = "image/yourfacehere";
        headers->appendNormal(line1, sizeof(line1) - 1);

        static char line2[] = " next : \t\tlinejunk \t";
        static char line2v[] = "linejunk \t";
        headers->appendNormal(line2, sizeof(line2) - 1);

        static char line3[] = "FancY-PANTs::plop:-neuf-=vleem=";
        static char line3v[] = ":plop:-neuf-=vleem=";
        headers->appendNormal(line3, sizeof(line3) - 1);

        static char line4[] = "all-talk-no-walk:";
        static char line4v[] = "";
        headers->appendNormal(line4, sizeof(line4) - 1);

        static char line5[] = ":all-talk-no-walk";
        static char line5v[] = "all-talk-no-walk";
        headers->appendNormal(line5, sizeof(line5) - 1);

        static char line6[] = "  :";
        static char line6v[] = "";
        headers->appendNormal(line6, sizeof(line6) - 1);

        ensure("All entries accounted for", 6 == headers->size());

        static char * values[] = {
            line1v,
            line2v,
            line3v,
            line4v,
            line5v,
            line6v
        };
            
        int i(0);
        HttpHeaders::const_iterator cend(headers->end());
        for (HttpHeaders::const_iterator it(headers->begin());
             cend != it;
             ++it, ++i)
        {
            std::ostringstream str;
            str << "Const Iterator value # " << i << " was " << values[i];
            ensure(str.str(), (*it).second == values[i]);
        }

        // Rewind, do non-consts
        i = 0;
        HttpHeaders::iterator end(headers->end());
        for (HttpHeaders::iterator it(headers->begin());
             end != it;
             ++it, ++i)
        {
            std::ostringstream str;
            str << "Const Iterator value # " << i << " was " << values[i];
            ensure(str.str(), (*it).second == values[i]);
        }
    }
    
    // release the implicit reference, causing the object to be released
    headers.reset();
}

// Reverse iterators find everything as expected
template <> template <>
void HttpHeadersTestObjectType::test<6>()
{
    set_test_name("HttpHeaders reverse iterator tests");

    // create a new ref counted object with an implicit reference
    HttpHeaders::ptr_t headers = HttpHeaders::ptr_t(new HttpHeaders());

    HttpHeaders::reverse_iterator rend(headers->rend()), rbegin(headers->rbegin());
    ensure("Empty container has equal rbegin/rend const iterators", rend == rbegin);
    HttpHeaders::const_reverse_iterator crend(headers->rend()), crbegin(headers->rbegin());
    ensure("Empty container has equal rbegin/rend const iterators", crend == crbegin);
    
    {
        static char line1[] = " AcCePT : image/yourfacehere";
        static char line1v[] = "image/yourfacehere";
        headers->appendNormal(line1, sizeof(line1) - 1);

        static char line2[] = " next : \t\tlinejunk \t";
        static char line2v[] = "linejunk \t";
        headers->appendNormal(line2, sizeof(line2) - 1);

        static char line3[] = "FancY-PANTs::plop:-neuf-=vleem=";
        static char line3v[] = ":plop:-neuf-=vleem=";
        headers->appendNormal(line3, sizeof(line3) - 1);

        static char line4[] = "all-talk-no-walk:";
        static char line4v[] = "";
        headers->appendNormal(line4, sizeof(line4) - 1);

        static char line5[] = ":all-talk-no-walk";
        static char line5v[] = "all-talk-no-walk";
        headers->appendNormal(line5, sizeof(line5) - 1);

        static char line6[] = "  :";
        static char line6v[] = "";
        headers->appendNormal(line6, sizeof(line6) - 1);

        ensure("All entries accounted for", 6 == headers->size());

        static char * values[] = {
            line6v,
            line5v,
            line4v,
            line3v,
            line2v,
            line1v
        };
            
        int i(0);
        HttpHeaders::const_reverse_iterator cend(headers->rend());
        for (HttpHeaders::const_reverse_iterator it(headers->rbegin());
             cend != it;
             ++it, ++i)
        {
            std::ostringstream str;
            str << "Const Iterator value # " << i << " was " << values[i];
            ensure(str.str(), (*it).second == values[i]);
        }

        // Rewind, do non-consts
        i = 0;
        HttpHeaders::reverse_iterator end(headers->rend());
        for (HttpHeaders::reverse_iterator it(headers->rbegin());
             end != it;
             ++it, ++i)
        {
            std::ostringstream str;
            str << "Iterator value # " << i << " was " << values[i];
            ensure(str.str(), (*it).second == values[i]);
        }
    }
    
    // release the implicit reference, causing the object to be released
    headers.reset();
}

}  // end namespace tut


#endif  // TEST_LLCORE_HTTP_HEADERS_H_
