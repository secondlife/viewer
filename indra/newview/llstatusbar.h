/** 
 * @file llstatusbar.h
 * @brief LLStatusBar class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
class cLLRegionDetails
{
public:
	LLString mRegionName;
	char	*mParcelName;
	char	*mAccesString;
	S32		mX;
	S32		mY;
	S32		mZ;
	S32		mArea;
	BOOL	mForSale;
	char	mOwner[MAX_STRING];
	F32		mTraffic;
	S32		mBalance;
	LLString	mTime;
	U32		mPing;
};

class LLStatusBar
:	public LLPanel
{
public:
	LLStatusBar(const std::string& name, const LLRect& rect );
	~LLStatusBar();
	virtual BOOL postBuild();
	
	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	// OVERRIDES
	virtual void draw();

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
	cLLRegionDetails mRegionDetails;

protected:
	static void onClickParcelInfo(void*);
	static void onClickBalance(void*);
	static void onClickBuyCurrency(void*);
	static void onClickRegionInfo(void*);
	static void onClickHealth(void*);
	static void onClickFly(void*);
	static void onClickPush(void*);
	static void onClickBuild(void*);
	static void onClickScripts(void*);
	static void onClickBuyLand(void*);
	static void onClickScriptDebug(void*);

protected:
	LLTextBox	*mTextBalance;
	LLTextBox	*mTextHealth;
	LLTextBox	*mTextTime;

	LLButton	*mBtnScriptOut;
	LLButton	*mBtnHealth;
	LLButton	*mBtnFly;
	LLButton	*mBtnBuild;
	LLButton	*mBtnScripts;
	LLButton	*mBtnPush;
	LLButton	*mBtnBuyLand;


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
};

extern LLStatusBar *gStatusBar;

#endif
