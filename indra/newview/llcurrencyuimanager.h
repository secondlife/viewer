/** 
 * @file llcurrencyuimanager.h
 * @brief LLCurrencyUIManager class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLCURRENCYUIMANAGER_H
#define LL_LLCURRENCYUIMANAGER_H

class LLPanel;


class LLCurrencyUIManager
	// manages the currency purchase portion of any dialog
	// takes control of, and assumes repsonsibility for several
	// fields:
	// 	'currency_action' - the text "Buy L$" before the entry field
	// 	'currency_amt' - the line editor for the entry amount
	// 	'currency_est' - the estimated cost from the web site
{
public:
	LLCurrencyUIManager(LLPanel& parent);
	virtual ~LLCurrencyUIManager();

	void setAmount(int, bool noEstimate = false);
	int getAmount();
		// the amount in L$ to purchase
		// setting it overwrites the user's entry
		// if noEstimate is true, than no web request is made
	
	void setZeroMessage(const std::string& message);
		// sets the gray message to show when zero
		
	void setEstimate(int);
	int getEstimate();
		// the amount in US$ * 100 (in otherwords, in cents)
		// use set when you get this information from elsewhere
		
	void prepare();
		// call once after dialog is built, from postBuild()
	void updateUI(bool show = true);
		// update all UI elements, if show is false, they are all set not visible
		// normally, this is done automatically, but you can force it
		// the show/hidden state is remembered
	bool process();
		// call periodically, for example, from draw()
		// returns true if the UI needs to be updated
	
	void buy(const LLString& buy_msg);
		// call to initiate the purchase
	
	bool inProcess();	// is a transaction in process
	bool canCancel();	// can we cancel it (by destructing this object)
	bool canBuy();		// can the user choose to buy now?
	bool buying();		// are we in the process of buying?
	bool bought();		// did the buy() transaction complete successfully

	bool hasError();
	std::string errorMessage();
	std::string errorURI();
		// error information for the user, the URI may be blank
		// the technical error details will have already been logged

private:
	class Impl;
	Impl& impl;
};

#endif 


