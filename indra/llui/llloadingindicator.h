/** 
 * @file llloadingindicator.h
 * @brief Perpetual loading indicator
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

#ifndef LL_LLLOADINGINDICATOR_H
#define LL_LLLOADINGINDICATOR_H

#include "lluictrl.h"
#include "lluiimage.h"

///////////////////////////////////////////////////////////////////////////////
// class LLLoadingIndicator
///////////////////////////////////////////////////////////////////////////////

/**
 * Perpetual loading indicator (a la MacOSX or YouTube)
 * 
 * Number of rotations per second can be overridden
 * with the "images_per_sec" parameter.
 * 
 * Can start/stop spinning.
 * 
 * @see start()
 * @see stop()
 */
class LLLoadingIndicator
: public LLUICtrl
{
	LOG_CLASS(LLLoadingIndicator);
public:

	struct Images : public LLInitParam::Block<Images>
	{
		Multiple<LLUIImage*>	image;

		Images()
		:	image("image")
		{}
	};

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<F32>			images_per_sec;
		Batch<Images>			images;

		Params()
		:	images_per_sec("images_per_sec", 1.0f),
			images("images")
		{}
	};

	virtual ~LLLoadingIndicator() {}

	// llview overrides
	virtual void draw();

	/**
	 * Stop spinning.
	 */
	void stop();

	/**
	 * Start spinning.
	 */
	void start();

private:
	LLLoadingIndicator(const Params&);
	void initFromParams(const Params&);

	friend class LLUICtrlFactory;

	F32						mImagesPerSec;
	S8						mCurImageIdx;
	LLFrameTimer			mImageSwitchTimer;

	std::vector<LLUIImagePtr> mImages;
};

#endif // LL_LLLOADINGINDICATOR_H
