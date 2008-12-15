/** 
 * @file llprogressbar.h
 * @brief LLProgressBar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLPROGRESSBAR_H
#define LL_LLPROGRESSBAR_H

#include "llview.h"
#include "llframetimer.h"

class LLProgressBar
	: public LLView
{
public:
	LLProgressBar(const std::string& name, const LLRect &rect);
	virtual ~LLProgressBar();

	void setPercent(const F32 percent);

	void setImageBar(const std::string &bar_name);
	void setImageShadow(const std::string &shadow_name);

	void setColorBar(const LLColor4 &c);
	void setColorBar2(const LLColor4 &c);
	void setColorShadow(const LLColor4 &c);
	void setColorBackground(const LLColor4 &c);

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	/*virtual*/ void draw();

protected:
	F32 mPercentDone;

	LLPointer<LLImageGL>  mImageBar;
	//LLUUID                mImageBarID;
	//LLString              mImageBarName;
	LLColor4              mColorBar;
	LLColor4              mColorBar2;

	LLPointer<LLImageGL>  mImageShadow;
	//LLUUID                mImageShadowID;
	//LLString              mImageShadowName;
	LLColor4              mColorShadow;
	LLColor4              mColorBackground;
};

#endif // LL_LLPROGRESSBAR_H
