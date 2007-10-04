/** 
 * @file llblowfish_tut.cpp
 * @author James Cook, james@lindenlab.com
 * @date 2007-02-04
 *
 * Data files generated with:
 * openssl enc -bf-cbc -in blowfish.digits.txt -out blowfish.1.bin -K 00000000000000000000000000000000 -iv 0000000000000000 -p
 * openssl enc -bf-cbc -in blowfish.digits.txt -out blowfish.2.bin -K 526a1e07a19dbaed84c4ff08a488d15e -iv 0000000000000000 -p
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

		bool matchFile(const char* filename,
				const std::string& data)
		{
			FILE* fp = fopen(filename, "rb");
			if (!fp) 
			{
				// sometimes test is run inside the indra directory
				std::string path = "test/";
				path += filename;
				fp = fopen(path.c_str(), "rb");
			}
			if (!fp)
			{
				llwarns << "unabled to open " << filename << llendl;
				return false;
			}

			std::string good;
			good.resize(256);
			size_t got = fread(&good[0], 1, 256, fp);
			lldebugs << "matchFile read " << got << llendl;
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
