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
// LLLoadingIndicator::Data class
///////////////////////////////////////////////////////////////////////////////

/**
 * Pre-loaded images shared by all instances of the widget
 */
class LLLoadingIndicator::Data: public LLSingleton<LLLoadingIndicator::Data>
{
public:
	/*virtual*/ void		initSingleton(); // from LLSingleton

	LLPointer<LLUIImage>	getNextImage(S8& idx) const;
	U8						getImagesCount() const	{ return NIMAGES; }
private:

	static const U8			NIMAGES = 12;
	LLPointer<LLUIImage>	mImages[NIMAGES];
};

// virtual
// Called right after the instance gets constructed.
void LLLoadingIndicator::Data::initSingleton()
{
	// Load images.
	for (U8 i = 0; i < NIMAGES; ++i)
	{
		std::string img_name = llformat("Progress_%d", i+1);
		mImages[i] = LLUI::getUIImage(img_name, 0);
		llassert(mImages[i]);
	}
}

LLPointer<LLUIImage> LLLoadingIndicator::Data::getNextImage(S8& idx) const
{
	// Calculate next index, performing array bounds checking.
	idx = (idx >= NIMAGES || idx < 0) ? 0 : (idx + 1) % NIMAGES; 
	return mImages[idx];
}

///////////////////////////////////////////////////////////////////////////////
// LLLoadingIndicator class
///////////////////////////////////////////////////////////////////////////////

LLLoadingIndicator::LLLoadingIndicator(const Params& p)
:	LLUICtrl(p)
	, mRotationsPerSec(p.rotations_per_sec > 0 ? p.rotations_per_sec : 1.0f)
	, mCurImageIdx(-1)
{
	// Select initial image.
	mCurImagep = Data::instance().getNextImage(mCurImageIdx);

	// Start timer for switching images.
	start();
}

void LLLoadingIndicator::draw()
{
	// Time to switch to the next image?
	if (mImageSwitchTimer.getStarted() && mImageSwitchTimer.hasExpired())
	{
		// Switch to the next image.
		mCurImagep = Data::instance().getNextImage(mCurImageIdx);

		// Restart timer.
		start();
	}

	// Draw current image.
	if( mCurImagep.notNull() )
	{
		mCurImagep->draw(getLocalRect(), LLColor4::white % getDrawContext().mAlpha);
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
	F32 period = 1.0f / (Data::instance().getImagesCount() * mRotationsPerSec);
	mImageSwitchTimer.setTimerExpirySec(period);
}
