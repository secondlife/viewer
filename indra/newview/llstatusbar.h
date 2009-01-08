/** 
 * @file llstatusbar.h
 * @brief LLStatusBar class definition
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

#ifndef LL_LLSTATUSBAR_H
#define LL_LLSTATUSBAR_H

#include "llpanel.h"

// "Constants" loaded from settings.xml at start time
extern S32 STATUS_BAR_HEIGHT;

class LLButton;
class LLLineEditor;
class LLMessageSystem;
class LLTextBox;
class LLTextEditor;
class LLUICtrl;
class LLUUID;
class LLFrameTimer;
class LLStatGraph;

// used by LCD screen
class LLRegionDetails
{
public:
	LLRegionDetails() :
		mRegionName("Unknown"),
		mParcelName("Unknown"),
		mAccessString("Unknown"),
		mX(0),
		mY(0),
		mZ(0),
		mArea (0),
		mForSale(FALSE),
		mOwner("Unknown"),
		mTraffic(0),
		mBalance(0),
		mPing(0)
	{
	}
	std::string mRegionName;
	std::string	mParcelName;
	std::string	mAccessString;
	S32		mX;
	S32		mY;
	S32		mZ;
	S32		mArea;
	BOOL	mForSale;
	std::string	mOwner;
	F32		mTraffic;
	S32		mBalance;
	std::string mTime;
	U32		mPing;
};

class LLStatusBar
:	public LLPanel
{
public:
	LLStatusBar(const std::string& name, const LLRect& rect );
	/*virtual*/ ~LLStatusBar();
	
	/*virtual*/ void draw();

	// MANIPULATORS
	void		setBalance(S32 balance);
	void		debitBalance(S32 debit);
	void		creditBalance(S32 credit);

	void		setHealth(S32 percent);

	void setLandCredit(S32 credit);
	void setLandCommitted(S32 committed);

	void		refresh();
	void setVisibleForMouselook(bool visible);
		// some elements should hide in mouselook

	// ACCESSORS
	S32			getBalance() const;
	S32			getHealth() const;

	BOOL isUserTiered() const;
	S32 getSquareMetersCredit() const;
	S32 getSquareMetersCommitted() const;
	S32 getSquareMetersLeft() const;
	LLRegionDetails mRegionDetails;

private:
	// simple method to setup the part that holds the date
	void setupDate();

	static void onCommitSearch(LLUICtrl*, void* data);
	static void onClickSearch(void* data);
	static void onClickStatGraph(void* data);

private:
	LLTextBox	*mTextBalance;
	LLTextBox	*mTextHealth;
	LLTextBox	*mTextTime;

	LLTextBox*	mTextParcelName;

	LLStatGraph *mSGBandwidth;
	LLStatGraph *mSGPacketLoss;

	LLButton	*mBtnBuyCurrency;

	S32				mBalance;
	S32				mHealth;
	S32				mSquareMetersCredit;
	S32				mSquareMetersCommitted;
	LLFrameTimer*	mBalanceTimer;
	LLFrameTimer*	mHealthTimer;
	
	static std::vector<std::string> sDays;
	static std::vector<std::string> sMonths;
	static const U32 MAX_DATE_STRING_LENGTH;
};

// *HACK: Status bar owns your cached money balance. JC
BOOL can_afford_transaction(S32 cost);

extern LLStatusBar *gStatusBar;

#endif
