/** 
 * @file llloadingindicator.cpp
 * @brief Perpetual loading indicator
 *
 * $LicenseInfo:firstyear=2010&license=viewergpl$
 * 
 * Copyright (c) 2010, Linden Research, Inc.
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

#include "linden_common.h"

#include "llloadingindicator.h"

// Linden library includes
#include "llsingleton.h"

// Project includes
#include "lluictrlfactory.h"
#include "lluiimage.h"

static LLDefaultChildRegistry::Register<LLLoadingIndicator> r("loading_indicator");

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
