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

protected:	
	// simple method to setup the part that holds the date
	void setupDate();

protected:
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
