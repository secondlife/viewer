/** 
 * @file llimageworker.cpp
 * @brief Base class for images.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#include "llimageworker.h"
#include "llimagedxt.h"

//----------------------------------------------------------------------------

//static
LLWorkerThread* LLImageWorker::sWorkerThread = NULL;
S32 LLImageWorker::sCount = 0;

//static
void LLImageWorker::initClass(LLWorkerThread* workerthread)
{
	sWorkerThread = workerthread;
}

//static
void LLImageWorker::cleanupClass()
{
}

//----------------------------------------------------------------------------

LLImageWorker::LLImageWorker(LLImageFormatted* image, U32 priority,
							 S32 discard,
							 LLPointer<LLResponder> responder)
	: LLWorkerClass(sWorkerThread, "Image"),
	  mFormattedImage(image),
	  mDecodedType(-1),
	  mDiscardLevel(discard),
	  mPriority(priority),
	  mResponder(responder)
{
	++sCount;
}

LLImageWorker::~LLImageWorker()
{
	mDecodedImage = NULL;
	mFormattedImage = NULL;
	--sCount;
}

//----------------------------------------------------------------------------

//virtual, main thread
void LLImageWorker::startWork(S32 param)
{
	llassert_always(mDecodedImage.isNull());
	mDecodedType = -1;
}

bool LLImageWorker::doWork(S32 param)
{
	bool decoded = false;
	if(mDecodedImage.isNull())
	{
		if (!mFormattedImage->updateData())
		{
			mDecodedType = -2; // failed
			return true;
		}
		if (mDiscardLevel >= 0)
		{
			mFormattedImage->setDiscardLevel(mDiscardLevel);
		}
		if (!(mFormattedImage->getWidth() * mFormattedImage->getHeight() * mFormattedImage->getComponents()))
		{
			decoded = true; // failed
		}
		else
		{
			mDecodedImage = new LLImageRaw(); // allow possibly smaller size set during decoding
		}
	}
	if (!decoded)
	{
		if (param == 0)
		{
			// Decode primary channels
			decoded = mFormattedImage->decode(mDecodedImage, .1f); // 1ms
		}
		else
		{
			// Decode aux channel
			decoded = mFormattedImage->decodeChannels(mDecodedImage, .1f, param, param); // 1ms
		}
	}
	if (decoded)
	{
		// Call the callback immediately; endWork doesn't get called until ckeckWork
		if (mResponder.notNull())
		{
			bool success = (!wasAborted() && mDecodedImage.notNull() && mDecodedImage->getDataSize() != 0);
			mResponder->completed(success);
		}
	}
	return decoded;
}

void LLImageWorker::endWork(S32 param, bool aborted)
{
	if (mDecodedType != -2)
	{
		mDecodedType = aborted ? -2 : param;
	}
}

//----------------------------------------------------------------------------


BOOL LLImageWorker::requestDecodedAuxData(LLPointer<LLImageRaw>& raw, S32 channel, S32 discard)
{
	// For most codecs, only mDiscardLevel data is available.
	//  (see LLImageDXT for exception)
	if (discard >= 0 && discard != mFormattedImage->getDiscardLevel())
	{
		llerrs << "Request for invalid discard level" << llendl;
	}
	checkWork();
	if (mDecodedType == -2)
	{
		return TRUE; // aborted, done
	}
	if (mDecodedType != channel)
	{
		if (!haveWork())
		{
			addWork(channel, mPriority);
		}
		return FALSE;
	}
	else
	{
		llassert_always(!haveWork());
		llassert_always(mDecodedType == channel);
		raw = mDecodedImage; // smart pointer acquires ownership of data
		mDecodedImage = NULL;
		return TRUE;
	}
}

BOOL LLImageWorker::requestDecodedData(LLPointer<LLImageRaw>& raw, S32 discard)
{
	if (mFormattedImage->getCodec() == IMG_CODEC_DXT)
	{
		// special case
		LLImageDXT* imagedxt = (LLImageDXT*)((LLImageFormatted*)mFormattedImage);
		return imagedxt->getMipData(raw, discard);
	}
	else
	{
		return requestDecodedAuxData(raw, 0, discard);
	}
}
