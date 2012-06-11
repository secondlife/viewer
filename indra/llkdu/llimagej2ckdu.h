/** 
 * @file llimagej2ckdu.h
 * @brief This is an implementation of JPEG2000 encode/decode using Kakadu
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_LLIMAGEJ2CKDU_H
#define LL_LLIMAGEJ2CKDU_H

#include "llimagej2c.h"

//
// KDU core header files
//
#define KDU_NO_THREADS
#include "kdu_elementary.h"
#include "kdu_messaging.h"
#include "kdu_params.h"
#include "kdu_compressed.h"
#include "kdu_sample_processing.h"

class LLKDUDecodeState;
class LLKDUMemSource;

class LLImageJ2CKDU : public LLImageJ2CImpl
{	
public:
	enum ECodeStreamMode 
	{
		MODE_FAST = 0,
		MODE_RESILIENT = 1,
		MODE_FUSSY = 2
	};
	LLImageJ2CKDU();
	virtual ~LLImageJ2CKDU();
	
protected:
	/*virtual*/ BOOL getMetadata(LLImageJ2C &base);
	/*virtual*/ BOOL decodeImpl(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count);
	/*virtual*/ BOOL encodeImpl(LLImageJ2C &base, const LLImageRaw &raw_image, const char* comment_text, F32 encode_time=0.0,
								BOOL reversible=FALSE);
	/*virtual*/ BOOL initDecode(LLImageJ2C &base, LLImageRaw &raw_image, int discard_level = -1, int* region = NULL);
	/*virtual*/ BOOL initEncode(LLImageJ2C &base, LLImageRaw &raw_image, int blocks_size = -1, int precincts_size = -1, int levels = 0);
	void findDiscardLevelsBoundaries(LLImageJ2C &base);

private:
	BOOL initDecode(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, ECodeStreamMode mode, S32 first_channel, S32 max_channel_count, int discard_level = -1, int* region = NULL);
	void setupCodeStream(LLImageJ2C &base, BOOL keep_codestream, ECodeStreamMode mode);
	void cleanupCodeStream();

	// Encode variable
	LLKDUMemSource *mInputp;
	kdu_codestream *mCodeStreamp;
	kdu_coords *mTPosp; // tile position
	kdu_dims *mTileIndicesp;
	int mBlocksSize;
	int mPrecinctsSize;
	int mLevels;

	// Temporary variables for in-progress decodes...
	LLImageRaw *mRawImagep;
	LLKDUDecodeState *mDecodeState;
};

#if LL_WINDOWS
# define LLSYMEXPORT __declspec(dllexport)
#elif LL_LINUX
# define LLSYMEXPORT __attribute__ ((visibility("default")))
#else
# define LLSYMEXPORT
#endif

extern "C" LLSYMEXPORT const char* engineInfoLLImageJ2CKDU();
extern "C" LLSYMEXPORT LLImageJ2CKDU* createLLImageJ2CKDU();
extern "C" LLSYMEXPORT void destroyLLImageJ2CKDU(LLImageJ2CKDU* kdu);

#endif
