/** 
 * @file llblowfish_tut.cpp
 * @author James Cook, james@lindenlab.com
 * @date 2007-02-04
 *
 * Data files generated with:
 * openssl enc -bf-cbc -in blowfish.digits.txt -out blowfish.1.bin -K 00000000000000000000000000000000 -iv 0000000000000000 -p
 * openssl enc -bf-cbc -in blowfish.digits.txt -out blowfish.2.bin -K 526a1e07a19dbaed84c4ff08a488d15e -iv 0000000000000000 -p
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
#include "lltut.h"

#include "llblowfishcipher.h"

#include "lluuid.h"

namespace tut
{
    class LLData
    {
    public:
        unsigned char* mInput;
        int mInputSize;

        LLData()
        {
            // \n to make it easier to create text files
            // for testing with command line openssl
            mInput = (unsigned char*)"01234567890123456789012345678901234\n";
            mInputSize = 36;
        }

        bool matchFile(const std::string& filename,
                       const std::string& data)
        {
            LLFILE* fp = LLFile::fopen(filename, "rb");
            if (!fp) 
            {
                // sometimes test is run inside the indra directory
                std::string path = "test/";
                path += filename;
                fp = LLFile::fopen(path, "rb");
            }
            if (!fp)
            {
                LL_WARNS() << "unable to open " << filename << LL_ENDL;
                return false;
            }

            std::string good;
            good.resize(256);
            size_t got = fread(&good[0], 1, 256, fp);
            LL_DEBUGS() << "matchFile read " << got << LL_ENDL;
            fclose(fp);
            good.resize(got);
        
            return (good == data);
        }
    };
    typedef test_group<LLData> blowfish_test;
    typedef blowfish_test::object blowfish_object;
    // Create test with name that can be selected on
    // command line of test app.
    tut::blowfish_test blowfish("blowfish");

    template<> template<>
    void blowfish_object::test<1>()
    {
        LLUUID blank;
        LLBlowfishCipher cipher(&blank.mData[0], UUID_BYTES);

        U32 dst_len = cipher.requiredEncryptionSpace(36);
        ensure("encryption space 36",
                (dst_len == 40) );

        // Blowfish adds an additional 8-byte block if your
        // input is an exact multiple of 8
        dst_len = cipher.requiredEncryptionSpace(8);
        ensure("encryption space 8",
                (dst_len == 16)  );
    }

    template<> template<>
    void blowfish_object::test<2>()
    {
        LLUUID blank;
        LLBlowfishCipher cipher(&blank.mData[0], UUID_BYTES);

        std::string result;
        result.resize(256);
        U32 count = cipher.encrypt(mInput, mInputSize,
                (U8*) &result[0], 256);

        ensure("encrypt output count",
                (count == 40) );
        result.resize(count);

        ensure("encrypt null key", matchFile("blowfish.1.bin", result));
    }

    template<> template<>
    void blowfish_object::test<3>()
    {
        // same as base64 test id
        LLUUID id("526a1e07-a19d-baed-84c4-ff08a488d15e");
        LLBlowfishCipher cipher(&id.mData[0], UUID_BYTES);

        std::string result;
        result.resize(256);
        U32 count = cipher.encrypt(mInput, mInputSize,
                (U8*) &result[0], 256);

        ensure("encrypt output count",
                (count == 40) );
        result.resize(count);

        ensure("encrypt real key", matchFile("blowfish.2.bin", result));
    }
}
