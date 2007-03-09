/** 
 * @file llbase64.cpp
 * @brief Wrapper for apr base64 encoding that returns a std::string
 * @author James Cook
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llbase64.h"

#include <string>

#include "apr-1/apr_base64.h"


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

