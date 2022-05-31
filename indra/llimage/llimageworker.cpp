/** 
 * @file llimageworker.cpp
 * @brief Base class for images.
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

#include "linden_common.h"

#include "llimageworker.h"
#include "llimagedxt.h"

//----------------------------------------------------------------------------

// MAIN THREAD
LLImageDecodeThread::LLImageDecodeThread(bool threaded)
	: LLQueuedThread("imagedecode", threaded)
{
	mCreationMutex = new LLMutex();
}

//virtual 
LLImageDecodeThread::~LLImageDecodeThread()
{
	delete mCreationMutex ;
}

// MAIN THREAD
// virtual
S32 LLImageDecodeThread::update(F32 max_time_ms)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	S32 res = LLQueuedThread::update(max_time_ms);
	return res;
}

LLImageDecodeThread::handle_t LLImageDecodeThread::decodeImage(LLImageFormatted* image, 
	S32 discard, BOOL needs_aux, Responder* responder)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	handle_t handle = generateHandle();

    ImageRequest* req = new ImageRequest(handle, image,
        discard, needs_aux,
        responder);

    addRequest(req);

	return handle;
}

LLImageDecodeThread::Responder::~Responder()
{
}

//----------------------------------------------------------------------------

LLImageDecodeThread::ImageRequest::ImageRequest(handle_t handle, LLImageFormatted* image, 
												S32 discard, BOOL needs_aux,
												LLImageDecodeThread::Responder* responder)
	: LLQueuedThread::QueuedRequest(handle, FLAG_AUTO_COMPLETE),
	  mFormattedImage(image),
	  mDiscardLevel(discard),
	  mNeedsAux(needs_aux),
	  mDecodedRaw(FALSE),
	  mDecodedAux(FALSE),
	  mResponder(responder)
{
}

LLImageDecodeThread::ImageRequest::~ImageRequest()
{
	mDecodedImageRaw = NULL;
	mDecodedImageAux = NULL;
	mFormattedImage = NULL;
}

//----------------------------------------------------------------------------


// Returns true when done, whether or not decode was successful.
bool LLImageDecodeThread::ImageRequest::processRequest()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	const F32 decode_time_slice = 0.f; //disable time slicing
	bool done = true;
	if (!mDecodedRaw && mFormattedImage.notNull())
	{
		// Decode primary channels
		if (mDecodedImageRaw.isNull())
		{
			// parse formatted header
			if (!mFormattedImage->updateData())
			{
				return true; // done (failed)
			}
			if (!(mFormattedImage->getWidth() * mFormattedImage->getHeight() * mFormattedImage->getComponents()))
			{
				return true; // done (failed)
			}
			if (mDiscardLevel >= 0)
			{
				mFormattedImage->setDiscardLevel(mDiscardLevel);
			}
			mDecodedImageRaw = new LLImageRaw(mFormattedImage->getWidth(),
											  mFormattedImage->getHeight(),
											  mFormattedImage->getComponents());
		}
		done = mFormattedImage->decode(mDecodedImageRaw, decode_time_slice); // 1ms
		// some decoders are removing data when task is complete and there were errors
		mDecodedRaw = done && mDecodedImageRaw->getData();
	}
	if (done && mNeedsAux && !mDecodedAux && mFormattedImage.notNull())
	{
		// Decode aux channel
		if (!mDecodedImageAux)
		{
			mDecodedImageAux = new LLImageRaw(mFormattedImage->getWidth(),
											  mFormattedImage->getHeight(),
											  1);
		}
		done = mFormattedImage->decodeChannels(mDecodedImageAux, decode_time_slice, 4, 4); // 1ms
		mDecodedAux = done && mDecodedImageAux->getData();
	}

	return done;
}

void LLImageDecodeThread::ImageRequest::finishRequest(bool completed)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_TEXTURE;
	if (mResponder.notNull())
	{
		bool success = completed && mDecodedRaw && (!mNeedsAux || mDecodedAux);
		mResponder->completed(success, mDecodedImageRaw, mDecodedImageAux);
	}
	// Will automatically be deleted
}

// Used by unit test only
// Checks that a responder exists for this instance so that something can happen when completion is reached
bool LLImageDecodeThread::ImageRequest::tut_isOK()
{
	return mResponder.notNull();
}
