/** 
 * @file llcurrencyuimanager.h
 * @brief LLCurrencyUIManager class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLCURRENCYUIMANAGER_H
#define LL_LLCURRENCYUIMANAGER_H

class LLPanel;


class LLCurrencyUIManager
    // manages the currency purchase portion of any dialog
    // takes control of, and assumes repsonsibility for several
    // fields:
    //  'currency_action' - the text "Buy L$" before the entry field
    //  'currency_amt' - the line editor for the entry amount
    //  'currency_est' - the estimated cost from the web site
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
        
    void setUSDEstimate(int);  // deprecated in 2.0
    int getUSDEstimate();      // deprecated in 2.0
        // the amount in US$ * 100 (in otherwords, in cents)
        // use set when you get this information from elsewhere
        
    void setLocalEstimate(const std::string &local_est);
    std::string getLocalEstimate() const;
        // the estimated cost in the user's local currency
        // for example, "US$ 10.00" or "10.00 Euros"
        
    void prepare();
        // call once after dialog is built, from postBuild()
    void updateUI(bool show = true);
        // update all UI elements, if show is false, they are all set not visible
        // normally, this is done automatically, but you can force it
        // the show/hidden state is remembered
    bool process();
        // call periodically, for example, from draw()
        // returns true if the UI needs to be updated
    
    void buy(const std::string& buy_msg);
        // call to initiate the purchase
    
    bool inProcess();   // is a transaction in process
    bool canCancel();   // can we cancel it (by destructing this object)
    bool canBuy();      // can the user choose to buy now?
    bool buying();      // are we in the process of buying?
    bool bought();      // did the buy() transaction complete successfully

    void clearError();

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


