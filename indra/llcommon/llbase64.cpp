/** 
 * @file llbase64.cpp
 * @brief Wrapper for apr base64 encoding that returns a std::string
 * @author James Cook
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "linden_common.h"

#include "llbase64.h"

#include <string>

#include "apr_base64.h"


// static
std::string LLBase64::encode(const U8* input, size_t input_size)
{
    std::string output;
    if (input
        && input_size > 0)
    {
        // Yes, it returns int.
        int b64_buffer_length = apr_base64_encode_len(input_size);
        char* b64_buffer = new char[b64_buffer_length];
        
        // This is faster than apr_base64_encode() if you know
        // you're not on an EBCDIC machine.  Also, the output is
        // null terminated, even though the documentation doesn't
        // specify.  See apr_base64.c for details. JC
        b64_buffer_length = apr_base64_encode_binary(
            b64_buffer,
            input,
            input_size);
        output.assign(b64_buffer);
        delete[] b64_buffer;
    }
    return output;
}

