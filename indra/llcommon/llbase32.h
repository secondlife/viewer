/** 
 * @file llbase32.h
 * @brief base32 encoding that returns a std::string
 * @author James Cook
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLBASE32_H
#define LLBASE32_h

class LLBase32
{
public:
	static std::string encode(const U8* input, size_t input_size);
};

#endif
