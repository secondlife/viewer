/** 
 * @file llbase32.cpp
 * @brief base32 encoding that returns a std::string
 * @author James Cook
 *
 * Based on code from bitter
 * http://ghostwhitecrab.com/bitter/
 *
 * Some parts of this file are:
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

/**
 * Copyright (c) 2006 Christian Biere <christianbiere@gmx.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "linden_common.h"

#include "llbase32.h"

#include <string>

// bitter - base32.c starts here

/*
 * See RFC 3548 for details about Base 32 encoding:
 *  http://www.faqs.org/rfcs/rfc3548.html
 */

static const char base32_alphabet[32] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '2', '3', '4', '5', '6', '7'
};

size_t
base32_encode(char *dst, size_t size, const void *data, size_t len)
{
  size_t i = 0;
  const U8 *p = (const U8*)data;
  const char *end = &dst[size];
  char *q = dst;

  do {
    size_t j, k;
    U8 x[5];
    char s[8];

    switch (len - i) {
    case 4: k = 7; break;
    case 3: k = 5; break;
    case 2: k = 3; break;
    case 1: k = 2; break;
    default:
      k = 8;
    }

    for (j = 0; j < 5; j++)
      x[j] = i < len ? p[i++] : 0;

/*
  +-------+-----------+--------+
  | target| source    | source |
  | byte  | bits      | byte   |
  +-------+-----------+--------+
  |     0 | 7 6 5 4 3 | 0      |
  |     1 | 2 1 0 7 6 | 0-1    |
  |     2 | 5 4 3 2 1 | 1      |
  |     3 | 0 7 6 5 4 | 1-2    |
  |     4 | 3 2 1 0 7 | 2-3    |
  |     5 | 6 5 4 3 2 | 3      |
  |     6 | 1 0 7 6 5 | 3-4    |
  |     7 | 4 3 2 1 0 | 4      |
  +-------+-----------+--------+
  
*/
    
    s[0] =  (x[0] >> 3);
    s[1] = ((x[0] & 0x07) << 2) | (x[1] >> 6);
    s[2] =  (x[1] >> 1) & 0x1f;
    s[3] = ((x[1] & 0x01) << 4) | (x[2] >> 4);
    s[4] = ((x[2] & 0x0f) << 1) | (x[3] >> 7);
    s[5] =  (x[3] >> 2) & 0x1f;
    s[6] = ((x[3] & 0x03) << 3) | (x[4] >> 5);
    s[7] =   x[4] & 0x1f;

    for (j = 0; j < k && q != end; j++)
      *q++ = base32_alphabet[(U8) s[j]];

  } while (i < len);

  return q - dst;
}

/* *TODO: Implement base32 encode.

#define ARRAY_LEN(a) (sizeof (a) / sizeof((a)[0]))

static inline int
ascii_toupper(int c)
{
  return c >= 97 && c <= 122 ? c - 32 : c;
}

static inline int
ascii_tolower(int c)
{
  return c >= 65 && c <= 90 ? c + 32 : c;
}


static char base32_map[(unsigned char) -1];

size_t
base32_decode(char *dst, size_t size, const void *data, size_t len)
{
  const char *end = &dst[size];
  const unsigned char *p = data;
  char *q = dst;
  size_t i;
  unsigned max_pad = 3;

  if (0 == base32_map[0]) {
    for (i = 0; i < ARRAY_LEN(base32_map); i++) {
      const char *x;
      
      x = memchr(base32_alphabet, ascii_toupper(i), sizeof base32_alphabet);
      base32_map[i] = x ? (x - base32_alphabet) : (unsigned char) -1;
    }
  }
  
  for (i = 0; i < len && max_pad > 0; i++) {
    unsigned char c;
    char s[8];
    size_t j;

    c = p[i];
    if ('=' == c) {
      max_pad--;
      c = 0;
    } else {
      c = base32_map[c];
      if ((unsigned char) -1 == c) {
        return -1;
      }
    }

    j = i % ARRAY_LEN(s);
    s[j] = c;

    if (7 == j) {
      char b[5];

      b[0] = ((s[0] << 3) & 0xf8) | ((s[1] >> 2) & 0x07);
      b[1] = ((s[1] & 0x03) << 6) | ((s[2] & 0x1f) << 1) | ((s[3] >> 4) & 1);
      b[2] = ((s[3] & 0x0f) << 4) | ((s[4] >> 1) & 0x0f);
      b[3] = ((s[4] & 1) << 7) | ((s[5] & 0x1f) << 2) | ((s[6] >> 3) & 0x03);
      b[4] = ((s[6] & 0x07) << 5) | (s[7] & 0x1f);

      for (j = 0; j < ARRAY_LEN(b); j++) {
        if (q != end)
          *q = b[j];
        q++;
      }
    }
  }

  return q - dst;
}
*/


// The following is
// Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
// static
std::string LLBase32::encode(const U8* input, size_t input_size)
{
	std::string output;
	if (input)
	{
		// Each 5 byte chunk of input is represented by an
		// 8 byte chunk of output.
		size_t input_chunks = (input_size + 4) / 5;
		size_t output_size = input_chunks * 8;

		output.resize(output_size);

		size_t encoded = base32_encode(&output[0], output_size, input, input_size);

		llinfos << "encoded " << encoded << " into buffer of size "
			<< output_size << llendl;
	}
	return output;
}
