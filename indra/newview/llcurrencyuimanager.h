/** 
 * @file llcurrencyuimanager.h
 * @brief LLCurrencyUIManager class definition
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
	
	void buy();
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

