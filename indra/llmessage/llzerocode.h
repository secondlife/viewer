/**
 * @file llzerocode.h
 * @brief Zero-code run-length compression used by the LLMessageSystem UDP protocol.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLZEROCODE_H
#define LL_LLZEROCODE_H

#include <cstring>

#include "stdtypes.h"

// Zero-coding compresses runs of zero bytes in a packet body, leaving the
// first header_size bytes of the buffer (the packet header - flags,
// sequence number, offset, etc.) untouched aside from the flag bit below.
//
//   Runs of zero bytes in the body are replaced by a two-byte token:
//     0x00 N        - represents N zero bytes, for N in 1..254
//     0x00 0x00 N   - represents (255 + N) zero bytes (wrap/overflow case,
//                     produced by decode()'s wire format but never emitted
//                     by encode(), which instead starts a fresh 0x00 token
//                     every 255 zero bytes)
namespace LLZeroCode
{
    // High bit of the first header byte: set by encode() and cleared by
    // decode() to indicate whether the body that follows is zero-coded.
    const U8 FLAG = 0x80;

    // Zero-codes src (src_size bytes, the first header_size of which are the
    // packet header and are copied verbatim) into dst.
    //
    // dst_capacity must be at least 2 * src_size: a pathological body of
    // isolated zero bytes can nearly double in size when encoded.
    //
    // Returns the encoded size (with FLAG set in dst[0]) if doing so made the
    // packet smaller. Returns -1 if compression would not help (or the
    // arguments are invalid), in which case dst is left untouched and the
    // caller should keep using the original, uncompressed buffer.
    inline S32 encode(const U8* src, U32 src_size, U8* dst, U32 dst_capacity, U32 header_size)
    {
        if (src_size < header_size || dst_capacity < 2 * src_size)
        {
            return -1;
        }

        S32 count = (S32)(src_size - header_size);
        S32 net_gain = 0;
        U8 num_zeroes = 0;

        const U8* inptr = src;
        U8* outptr = dst;

        // copy the header verbatim
        for (U32 ii = 0; ii < header_size; ++ii)
        {
            *outptr++ = *inptr++;
        }

        // sequential zero bytes are encoded as 0 [U8 count]; a run longer
        // than 254 bytes is split into consecutive 0 [U8 count] tokens.
        while (count--)
        {
            if (!(*inptr))   // in a zero count
            {
                if (num_zeroes)
                {
                    if (++num_zeroes > 254)
                    {
                        *outptr++ = num_zeroes;
                        num_zeroes = 0;
                    }
                    net_gain--;   // subsequent zeroes save one
                }
                else
                {
                    *outptr++ = 0;
                    net_gain++;  // starting a zero count adds one
                    num_zeroes = 1;
                }
                inptr++;
            }
            else
            {
                if (num_zeroes)
                {
                    *outptr++ = num_zeroes;
                    num_zeroes = 0;
                }
                *outptr++ = *inptr++;
            }
        }

        if (num_zeroes)
        {
            *outptr++ = num_zeroes;
        }

        if (net_gain >= 0)
        {
            // compression did not shrink the packet; caller should keep the original
            return -1;
        }

        dst[0] |= FLAG;
        return (S32)src_size + net_gain;
    }

    // Expands a zero-coded src (src_size bytes) into dst.
    //
    // If FLAG is not set in src[0], the body is not zero-coded: no work is
    // done and the function returns 0.
    //
    // On success, returns the number of bytes written to dst (always includes
    // the header_size header bytes, copied verbatim except for FLAG being
    // cleared from dst[0]).
    //
    // If expansion would write past dst_capacity - which only a malformed or
    // malicious packet should cause - decoding is aborted, *overflow is set
    // true, and the returned size reflects however much (if anything) was
    // salvaged; the caller should treat the packet as invalid.
    inline U32 decode(const U8* src, U32 src_size, U8* dst, U32 dst_capacity, U32 header_size, bool& overflow)
    {
        overflow = false;

        if (src_size < header_size || !(src[0] & FLAG))
        {
            return 0;
        }

        S32 count = (S32)(src_size - header_size);

        const U8* inptr = src;
        U8* outptr = dst;

        for (U32 ii = 0; ii < header_size; ++ii)
        {
            *outptr++ = *inptr++;
        }
        dst[0] &= ~FLAG;

        // reconstruct the body: a 0x00 byte starts a run; the byte(s) that
        // follow give its length (see the wire format described above).
        while (count--)
        {
            if (outptr > &dst[dst_capacity - 1])
            {
                overflow = true;
                outptr = dst;
                break;
            }

            if (!((*outptr++ = *inptr++)))
            {
                while ((count--) && (!(*inptr)))
                {
                    if (outptr > &dst[dst_capacity - 256])
                    {
                        overflow = true;
                        outptr = dst;
                        count = -1;
                        break;
                    }
                    *outptr++ = *inptr++;
                    memset(outptr, 0, 255);
                    outptr += 255;
                }

                if (count < 0)
                {
                    break;
                }
                else
                {
                    if (outptr > &dst[dst_capacity - (*inptr)])
                    {
                        overflow = true;
                        outptr = dst;
                    }
                    memset(outptr, 0, (*inptr) - 1);
                    outptr += ((*inptr) - 1);
                    inptr++;
                }
            }
        }

        return (U32)(outptr - dst);
    }
}

#endif // LL_LLZEROCODE_H
