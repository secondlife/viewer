/** 
 * @file windgen.h
 * @brief Templated wind noise generation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#ifndef WINDGEN_H
#define WINDGEN_H

#include "llcommon.h"
#include "llrand.h"

template <class MIXBUFFERFORMAT_T>
class LLWindGen
{
public:
	LLWindGen() :
		mTargetGain(0.f),
		mTargetFreq(100.f),
		mTargetPanGainR(0.5f),
		mbuf0(0.0),
		mbuf1(0.0),
		mbuf2(0.0),
		mbuf3(0.0),
		mbuf4(0.0),
		mbuf5(0.0),
		mY0(0.0),
		mY1(0.0),
		mCurrentGain(0.f),
		mCurrentFreq(100.f),
		mCurrentPanGainR(0.5f) {};

	static const U32 getInputSamplingRate() {return mInputSamplingRate;}

	// newbuffer = the buffer passed from the previous DSP unit.
	// numsamples = length in samples-per-channel at this mix time.
	// stride = number of bytes between start of each sample.
	// NOTE: generates L/R interleaved stereo
	MIXBUFFERFORMAT_T* windGenerate(MIXBUFFERFORMAT_T *newbuffer, int numsamples, int stride)
	{
		U8 *cursamplep = (U8*)newbuffer;
		
		double bandwidth = 50.0F;
		double a0,b1,b2;
		
		// calculate resonant filter coeffs
		b2 = exp(-(F_TWO_PI) * (bandwidth / mInputSamplingRate));
		
		while (numsamples--)
		{
			mCurrentFreq = (float)((0.999 * mCurrentFreq) + (0.001 * mTargetFreq));
			mCurrentGain = (float)((0.999 * mCurrentGain) + (0.001 * mTargetGain));
			mCurrentPanGainR = (float)((0.999 * mCurrentPanGainR) + (0.001 * mTargetPanGainR));
			b1 = (-4.0 * b2) / (1.0 + b2) * cos(F_TWO_PI * (mCurrentFreq / mInputSamplingRate));
			a0 = (1.0 - b2) * sqrt(1.0 - (b1 * b1) / (4.0 * b2));
			double nextSample;
			
			// start with white noise
			nextSample = ll_frand(2.0f) - 1.0f;
			
			// apply pinking filter
			mbuf0 = 0.997f * mbuf0 + 0.0126502f * nextSample; 
			mbuf1 = 0.985f * mbuf1 + 0.0139083f * nextSample;
			mbuf2 = 0.950f * mbuf2 + 0.0205439f * nextSample;
			mbuf3 = 0.850f * mbuf3 + 0.0387225f * nextSample;
			mbuf4 = 0.620f * mbuf4 + 0.0465932f * nextSample;
			mbuf5 = 0.250f * mbuf5 + 0.1093477f * nextSample;
			
			nextSample = mbuf0 + mbuf1 + mbuf2 + mbuf3 + mbuf4 + mbuf5;
			
			// do a resonant filter on the noise
			nextSample = (double)( a0 * nextSample - b1 * mY0 - b2 * mY1 );
			mY1 = mY0;
			mY0 = nextSample;
			
			nextSample *= mCurrentGain;
			
			MIXBUFFERFORMAT_T	sample;
			
			sample = llfloor(((F32)nextSample*32768.f*(1.0f - mCurrentPanGainR))+0.5f);
			*(MIXBUFFERFORMAT_T*)cursamplep = llclamp(sample, (MIXBUFFERFORMAT_T)-32768, (MIXBUFFERFORMAT_T)32767);
			cursamplep += stride;

			sample = llfloor(((F32)nextSample*32768.f*mCurrentPanGainR)+0.5f);
			*(MIXBUFFERFORMAT_T*)cursamplep = llclamp(sample, (MIXBUFFERFORMAT_T)-32768, (MIXBUFFERFORMAT_T)32767);
			cursamplep += stride;
		}
		
		return newbuffer;
	}

	F32 mTargetGain;
	F32 mTargetFreq;
	F32 mTargetPanGainR;

private:
	static const U32 mInputSamplingRate = 44100;
	F64 mbuf0;
	F64 mbuf1;
	F64 mbuf2;
	F64 mbuf3;
	F64 mbuf4;
	F64 mbuf5;
	F64 mY0;
	F64 mY1;
	F32 mCurrentGain;
	F32 mCurrentFreq;
	F32 mCurrentPanGainR;
};

#endif
