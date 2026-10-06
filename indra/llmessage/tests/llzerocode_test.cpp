/**
 * @file llzerocode_test.cpp
 * @brief LLZeroCode test cases.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
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

#include "../llzerocode.h"

#include "../test/lltut.h"

#include <vector>

namespace tut
{
    struct zerocode_data
    {
    };
    typedef test_group<zerocode_data> zerocode_test;
    typedef zerocode_test::object zerocode_object;
    tut::zerocode_test zerocode_testcase("LLZeroCode");

    // Builds header_size bytes of header (with header[0] == header0) followed by body.
    static std::vector<U8> makeBuffer(U32 header_size, U8 header0, const std::vector<U8>& body)
    {
        std::vector<U8> buf(header_size, 0);
        if (header_size)
        {
            buf[0] = header0;
        }
        buf.insert(buf.end(), body.begin(), body.end());
        return buf;
    }

    // Runs src through encode() then decode(), and (when compression is expected to help)
    // asserts the round trip reproduces src exactly, including untouched header bits other
    // than the zero-code flag. When compression is not expected to help, asserts encode()
    // refuses it.
    static void ensureRoundTrip(const char* msg, U32 header_size, U8 header0,
                                 const std::vector<U8>& body, bool expect_compressed)
    {
        std::vector<U8> src = makeBuffer(header_size, header0, body);
        std::vector<U8> enc(2 * src.size() + 16, 0xAA);

        S32 enc_size = LLZeroCode::encode(src.data(), (U32)src.size(),
                                           enc.data(), (U32)enc.size(), header_size);

        if (!expect_compressed)
        {
            ensure(std::string(msg) + ": encode should refuse (no benefit)", enc_size < 0);
            return;
        }

        ensure(std::string(msg) + ": encode should succeed", enc_size >= 0);
        ensure(std::string(msg) + ": encoded size should be smaller", (U32)enc_size < src.size());
        ensure(std::string(msg) + ": FLAG should be set on encoded output", (enc[0] & LLZeroCode::FLAG) != 0);
        if (header_size > 0)
        {
            ensure_equals(std::string(msg) + ": non-flag header bits preserved on encode",
                          (U8)(enc[0] & ~LLZeroCode::FLAG), (U8)(header0 & ~LLZeroCode::FLAG));
        }

        std::vector<U8> dec(src.size() + 16, 0xBB);
        bool overflow = false;
        U32 dec_size = LLZeroCode::decode(enc.data(), (U32)enc_size,
                                           dec.data(), (U32)dec.size(), header_size, overflow);
        ensure(std::string(msg) + ": decode should not overflow", !overflow);
        ensure_equals(std::string(msg) + ": decoded size should match original", dec_size, (U32)src.size());
        ensure(std::string(msg) + ": decoded bytes should match original",
               memcmp(dec.data(), src.data(), src.size()) == 0);
    }

    // Basic mixed body; header carries an unrelated flag bit (0x40) that must survive untouched.
    template<> template<>
    void zerocode_object::test<1>()
    {
        ensureRoundTrip("mixed body", 6, 0x40,
                        {0,0,0,5,0,0,0,0,0,0,7,8,9,0,0}, true);
    }

    // A body with no zero bytes at all cannot benefit from zero-coding.
    template<> template<>
    void zerocode_object::test<2>()
    {
        ensureRoundTrip("no zero bytes", 1, 0x00, {1,2,3,4,5,6,7,8,9}, false);
    }

    // Isolated zero-byte runs of length 1 or 2 cost as much or more than they save
    // (marker + terminator == 2 bytes), so encode must refuse them.
    template<> template<>
    void zerocode_object::test<3>()
    {
        ensureRoundTrip("isolated single zero", 1, 0x00, {5, 0}, false);
        ensureRoundTrip("isolated double zero", 1, 0x00, {5, 0, 0, 9}, false);
    }

    // A long enough zero run followed by other data pays off.
    template<> template<>
    void zerocode_object::test<4>()
    {
        std::vector<U8> body(20, 0);
        body.push_back(99);
        ensureRoundTrip("long zero run", 1, 0x00, body, true);
    }

    // Wrap boundary: runs are split into chunks of at most 255 zero bytes each
    // (encode never emits the doubled-0x00 wire form; see test<7> for that).
    // A run only shrinks the packet once it is longer than 2 bytes.
    template<> template<>
    void zerocode_object::test<5>()
    {
        static const U32 lengths[] = {1, 2, 253, 254, 255, 256, 257, 300, 509, 510, 511, 1000};
        for (U32 n : lengths)
        {
            std::vector<U8> body(n, 0);
            body.push_back(42); // trailing nonzero byte
            ensureRoundTrip("wrap boundary (with trailing byte)", 1, 0x00, body, n > 2);
        }
    }

    // Same boundary check, but with the zero run flushed at end-of-buffer
    // (no trailing nonzero byte to force the terminator write mid-loop).
    template<> template<>
    void zerocode_object::test<6>()
    {
        static const U32 lengths[] = {1, 2, 254, 255, 256, 300};
        for (U32 n : lengths)
        {
            std::vector<U8> body(n, 0);
            ensureRoundTrip("wrap boundary (end-of-buffer flush)", 1, 0x00, body, n > 2);
        }
    }

    // decode() must also accept the "0x00 0x00 N" doubled-marker wire format
    // (worth +255 zero bytes per extra marker) for compatibility with any
    // encoder other than this one's chunking strategy, even though encode()
    // itself never emits it.
    template<> template<>
    void zerocode_object::test<7>()
    {
        // header (1 byte, FLAG set) + [0x00 marker][0x00 extra-wrap-marker][terminator=5]
        // decoded body length = 1 (marker) + 1 (extra marker) + 255 (memset) + (5 - 1) = 261
        std::vector<U8> encoded = {(U8)LLZeroCode::FLAG, 0x00, 0x00, 0x05};
        std::vector<U8> dec(1024, 0xDD);
        bool overflow = false;
        U32 dec_size = LLZeroCode::decode(encoded.data(), (U32)encoded.size(),
                                           dec.data(), (U32)dec.size(), 1, overflow);
        ensure("wrap format: no overflow", !overflow);
        ensure_equals("wrap format: decoded size", dec_size, (U32)262);
        ensure_equals("wrap format: FLAG cleared on header", dec[0], (U8)0x00);
        for (U32 i = 1; i < dec_size; ++i)
        {
            ensure_equals("wrap format: decoded byte is zero", dec[i], (U8)0);
        }
    }

    // decode() is a no-op (returns 0, does not touch dst) when FLAG is not set.
    template<> template<>
    void zerocode_object::test<8>()
    {
        std::vector<U8> src = makeBuffer(6, 0x00, {1,2,3,0,0,0});
        std::vector<U8> dec(64, 0xCC);
        bool overflow = false;
        U32 dec_size = LLZeroCode::decode(src.data(), (U32)src.size(),
                                           dec.data(), (U32)dec.size(), 6, overflow);
        ensure_equals("not zero-coded: decode returns 0", dec_size, (U32)0);
        ensure("not zero-coded: no overflow", !overflow);
    }

    // encode() refuses to write into an undersized destination buffer.
    template<> template<>
    void zerocode_object::test<9>()
    {
        std::vector<U8> body(50, 0);
        std::vector<U8> src = makeBuffer(1, 0, body);
        std::vector<U8> enc(src.size(), 0); // smaller than the required 2 * src_size
        S32 r = LLZeroCode::encode(src.data(), (U32)src.size(), enc.data(), (U32)enc.size(), 1);
        ensure("encode: capacity guard rejects undersized dst", r < 0);
    }

    // decode() reports overflow (rather than writing out of bounds) when dst is too small,
    // whether the destination is smaller than a single byte's worth of headroom...
    template<> template<>
    void zerocode_object::test<10>()
    {
        std::vector<U8> body(500, 0);
        std::vector<U8> src = makeBuffer(1, 0, body);
        std::vector<U8> enc(2 * src.size() + 16, 0);
        S32 enc_size = LLZeroCode::encode(src.data(), (U32)src.size(), enc.data(), (U32)enc.size(), 1);
        ensure("decode overflow setup: encode succeeded", enc_size >= 0);

        std::vector<U8> dec(1, 0); // capacity == header_size exactly
        bool overflow = false;
        U32 dec_size = LLZeroCode::decode(enc.data(), (U32)enc_size,
                                           dec.data(), (U32)dec.size(), 1, overflow);
        ensure("decode overflow (minimal capacity): overflow flagged", overflow);
        ensure("decode overflow (minimal capacity): size bounded by capacity", dec_size <= dec.size());
    }

    // ...or comfortably larger than 256 bytes but still short of the true decoded size.
    template<> template<>
    void zerocode_object::test<11>()
    {
        std::vector<U8> body(500, 0);
        std::vector<U8> src = makeBuffer(1, 0, body);
        std::vector<U8> enc(2 * src.size() + 16, 0);
        S32 enc_size = LLZeroCode::encode(src.data(), (U32)src.size(), enc.data(), (U32)enc.size(), 1);
        ensure("decode overflow setup: encode succeeded", enc_size >= 0);

        std::vector<U8> dec(300, 0); // > 256, but less than the true decoded size (~501)
        bool overflow = false;
        U32 dec_size = LLZeroCode::decode(enc.data(), (U32)enc_size,
                                           dec.data(), (U32)dec.size(), 1, overflow);
        ensure("decode overflow (insufficient capacity): overflow flagged", overflow);
        ensure("decode overflow (insufficient capacity): size bounded by capacity", dec_size <= dec.size());
    }

    // A pathological alternating zero/non-zero pattern grows under zero-coding
    // (every isolated zero costs 2 output bytes for 1 input byte), so encode
    // must refuse it and leave the original buffer in use.
    template<> template<>
    void zerocode_object::test<12>()
    {
        std::vector<U8> body;
        for (int i = 0; i < 200; ++i)
        {
            body.push_back(0);
            body.push_back((U8)(i + 1));
        }
        ensureRoundTrip("alternating pattern", 1, 0x00, body, false);
    }
}
