/** 
 * @file llloadingindicator.cpp
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

#include "linden_common.h"

#include "llloadingindicator.h"

// Linden library includes
#include "llsingleton.h"

// Project includes
#include "lluictrlfactory.h"
#include "lluiimage.h"

// registered in llui.cpp to avoid being left out by MS linker
//static LLDefaultChildRegistry::Register<LLLoadingIndicator> r("loading_indicator");

///////////////////////////////////////////////////////////////////////////////
// LLLoadingIndicator class
///////////////////////////////////////////////////////////////////////////////

LLLoadingIndicator::LLLoadingIndicator(const Params& p)
:	LLUICtrl(p), 
	mImagesPerSec(p.images_per_sec > 0 ? p.images_per_sec : 1.0f), 
	mCurImageIdx(0)
{
}

void LLLoadingIndicator::initFromParams(const Params& p)
{
	for (LLInitParam::ParamIterator<LLUIImage*>::const_iterator it = p.images().image.begin(), end_it = p.images().image.end();
		it != end_it;
		++it)
	{
		mImages.push_back(it->getValue());
	}

	// Start timer for switching images.
	start();
}

void LLLoadingIndicator::draw()
{
	// Time to switch to the next image?
	if (mImageSwitchTimer.getStarted() && mImageSwitchTimer.hasExpired())
	{
		// Switch to the next image.
		if (!mImages.empty())
		{
			mCurImageIdx = (mCurImageIdx + 1) % mImages.size();
		}

		// Restart timer.
		start();
	}

	LLUIImagePtr cur_image = mImages.empty() ? NULL : mImages[mCurImageIdx];

	// Draw current image.
	if( cur_image.notNull() )
	{
		cur_image->draw(getLocalRect(), LLColor4::white % getDrawContext().mAlpha);
	}

	LLUICtrl::draw();
}

void LLLoadingIndicator::stop()
{
	mImageSwitchTimer.stop();
}

void LLLoadingIndicator::start()
{
	mImageSwitchTimer.start();
	F32 period = 1.0f / (mImages.size() * mImagesPerSec);
	mImageSwitchTimer.setTimerExpirySec(period);
}
