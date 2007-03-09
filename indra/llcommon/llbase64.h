/** 
 * @file llbase64.h
 * @brief Wrapper for apr base64 encoding that returns a std::string
 * @author James Cook
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLBASE64_H
#define LLBASE64_h

class LLBase64
{
public:
	static std::string encode(const U8* input, size_t input_size);
};

#endif
