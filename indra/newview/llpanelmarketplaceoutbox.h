/** 
 * @file llpanelmarketplaceoutbox.h
 * @brief Panel for marketplace outbox
 *
* $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#ifndef LL_LLPANELMARKETPLACEOUTBOX_H
#define LL_LLPANELMARKETPLACEOUTBOX_H

#include "llpanel.h"


class LLButton;
class LLInventoryPanel;
class LLLoadingIndicator;


class LLPanelMarketplaceOutbox : public LLPanel
{
public:
	
	struct Params :	public LLInitParam::Block<Params, LLPanel::Params>
	{};
	
	LOG_CLASS(LLPanelMarketplaceOutbox);

	// RN: for some reason you can't just use LLUICtrlFactory::getDefaultParams as a default argument in VC8
	static const LLPanelMarketplaceOutbox::Params& getDefaultParams();
	
	LLPanelMarketplaceOutbox(const Params& p = getDefaultParams());
	~LLPanelMarketplaceOutbox();

	/*virtual*/ BOOL postBuild();

	LLInventoryPanel * setupInventoryPanel();

	bool isOutboxEmpty() const;
	bool isSyncInProgress() const;

	void onSyncComplete();

protected:
	void onSyncButtonClicked();
	void updateSyncButtonStatus();

	void handleLoginComplete();
	void onFocusReceived();
	void onSelectionChange();

private:
	LLInventoryPanel *		mInventoryPanel;

	LLButton *				mSyncButton;
	LLLoadingIndicator *	mSyncIndicator;
	bool					mSyncInProgress;
};


#endif //LL_LLPANELMARKETPLACEOUTBOX_H

