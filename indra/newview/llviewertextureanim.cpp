/** 
 * @file llviewertextureanim.cpp
 * @brief LLViewerTextureAnim class implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llviewertextureanim.h"
#include "llvovolume.h"

#include "llmath.h"
#include "llerror.h"

std::vector<LLViewerTextureAnim*> LLViewerTextureAnim::sInstanceList;

LLViewerTextureAnim::LLViewerTextureAnim(LLVOVolume* vobj) : LLTextureAnim()
{
	mVObj = vobj;
	mLastFrame = -1.f;	// Force an update initially
	mLastTime = 0.f;
	mOffS = mOffT = 0;
	mScaleS = mScaleT = 1;
	mRot = 0;

	mInstanceIndex = sInstanceList.size();
	sInstanceList.push_back(this);
}

LLViewerTextureAnim::~LLViewerTextureAnim()
{
	S32 end_idx = sInstanceList.size()-1;
	
	if (end_idx != mInstanceIndex)
	{
		sInstanceList[mInstanceIndex] = sInstanceList[end_idx];
		sInstanceList[mInstanceIndex]->mInstanceIndex = mInstanceIndex;
	}

	sInstanceList.pop_back();
}

void LLViewerTextureAnim::reset()
{
	LLTextureAnim::reset();
	mTimer.reset();
}

//static 
void LLViewerTextureAnim::updateClass()
{
	for (std::vector<LLViewerTextureAnim*>::iterator iter = sInstanceList.begin(); iter != sInstanceList.end(); ++iter)
	{
		(*iter)->mVObj->animateTextures();
	}
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
