/** 
 * @file llpanelland.h
 * @brief Land information in the tool floater, NOT the "About Land" floater
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPANELLAND_H
#define LL_LLPANELLAND_H

#include "llpanel.h"

class LLTextBox;
class LLCheckBoxCtrl;
class LLButton;
class LLSpinCtrl;
class LLLineEditor;
class LLPanelLandSelectObserver;

class LLPanelLandInfo
:	public LLPanel
{
public:
	LLPanelLandInfo(const std::string& name);
	virtual ~LLPanelLandInfo();

	void refresh();
	static void refreshAll();

protected:
	static void onClickClaim(void*);
	static void onClickRelease(void*);
	static void onClickDivide(void*);
	static void onClickJoin(void*);
	static void onClickAbout(void*);

protected:
	//LLTextBox*		mTextPriceLabel;
	//LLTextBox*		mTextPrice;

	//LLButton*		mBtnClaimLand;
	//LLButton*		mBtnReleaseLand;
	//LLButton*		mBtnDivideLand;
	//LLButton*		mBtnJoinLand;
	//LLButton*		mBtnAbout;
	
	virtual BOOL	postBuild();

	static LLPanelLandSelectObserver* sObserver;
	static LLPanelLandInfo* sInstance;
};

#endif
