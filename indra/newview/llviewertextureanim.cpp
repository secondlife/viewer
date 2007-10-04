/** 
 * @file llviewertextureanim.cpp
 * @brief LLViewerTextureAnim class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llviewertextureanim.h"

#include "llmath.h"
#include "llerror.h"

LLViewerTextureAnim::LLViewerTextureAnim() : LLTextureAnim()
{
	mLastFrame = -1.f;	// Force an update initially
	mLastTime = 0.f;
	mOffS = mOffT = 0;
	mScaleS = mScaleT = 1;
	mRot = 0;
}

LLViewerTextureAnim::~LLViewerTextureAnim()
{
}

void LLViewerTextureAnim::reset()
{
	LLTextureAnim::reset();
	mTimer.reset();
}


S32 LLViewerTextureAnim::animateTextures(F32 &off_s, F32 &off_t,
										F32 &scale_s, F32 &scale_t,
										F32 &rot)
{
	S32 result = 0;
	if (!(mMode & ON))
	{
		mLastTime = 0.f;
		mLastFrame = -1.f;
		return result;
	}


	F32 num_frames = 1.0;
	F32 full_length = 1.0;

	if (mLength)
	{
		num_frames = mLength;
	}
	else
	{
		num_frames = llmax(1.f, (F32)(mSizeX * mSizeY));
	}

	if (mMode & PING_PONG)
	{
		if (mMode & SMOOTH)
		{
			full_length = 2.f*num_frames;
		}
		else if (mMode & LOOP)
		{
			full_length = 2.f*num_frames - 2.f;
			full_length = llmax(1.f, full_length);
		}
		else
		{
			full_length = 2.f*num_frames - 1.f;
			full_length = llmax(1.f, full_length);
		}
	}
	else
	{
		full_length = num_frames;
	}


	F32 frame_counter;
	if (mMode & SMOOTH)
	{
		frame_counter = mTimer.getElapsedTimeAndResetF32() * mRate + (F32)mLastTime;
	}
	else
	{
		frame_counter = mTimer.getElapsedTimeF32() * mRate;
	}
	mLastTime = frame_counter;

	if (mMode & LOOP)
	{
		frame_counter  = fmod(frame_counter, full_length);
	}
	else
	{
		frame_counter = llmin(full_length - 1.f, frame_counter);
	}

	if (!(mMode & SMOOTH))
	{
		frame_counter = (F32)llfloor(frame_counter + 0.01f);
	}

	if (mMode & PING_PONG)
	{
		if (frame_counter >= num_frames)
		{
			if (mMode & SMOOTH)
			{
				frame_counter = num_frames - (frame_counter - num_frames);
			}
			else
			{
				frame_counter = (num_frames - 1.99f) - (frame_counter - num_frames);
			}
		}
	}

	if (mMode & REVERSE)
	{
		if (mMode & SMOOTH)
		{
			frame_counter = num_frames - frame_counter;
		}
		else
		{
			frame_counter = (num_frames - 0.99f) - frame_counter;
		}
	}

	frame_counter += mStart;

	if (!(mMode & SMOOTH))
	{
		frame_counter = (F32)llround(frame_counter);
	}

	//
	// Now that we've calculated the frame time, do an update.
	// Will we correctly update stuff if the texture anim has
	// changed, but not the frame counter?
	//
	if (mLastFrame != frame_counter)
	{
		mLastFrame = frame_counter;
		if (mMode & ROTATE)
		{
			result |= ROTATE;
			mRot = rot = frame_counter;
		}
		else if (mMode & SCALE)
		{
			result |= SCALE;
			mScaleS = scale_s = frame_counter;
			mScaleT = scale_t = frame_counter;
		}
		else
		{
			result |= TRANSLATE;
			F32 x_frame;
			S32 y_frame;
			F32 x_pos;
			F32 y_pos;

			if (  (mSizeX)
				&&(mSizeY))
			{
				result |= SCALE;
				mScaleS = scale_s = 1.f/mSizeX;
				mScaleT = scale_t = 1.f/mSizeY;
				x_frame = fmod(frame_counter, mSizeX);
				y_frame = (S32)(frame_counter / mSizeX);
				x_pos = x_frame * scale_s;
				y_pos = y_frame * scale_t;
				mOffS = off_s = (-0.5f + 0.5f*scale_s)+ x_pos;
				mOffT = off_t = (0.5f - 0.5f*scale_t) - y_pos;
			}
			else
			{
				mScaleS = scale_s = 1.f;
				mScaleT = scale_t = 1.f;
				x_pos = frame_counter * scale_s;
				mOffS = off_s = (-0.5f + 0.5f*scale_s)+ x_pos;
				mOffT = off_t = 0.f;
			}
		}
	}
	return result;
}
