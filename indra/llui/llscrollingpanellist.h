/** 
 * @file llscrollingpanellist.h
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#ifndef LL_LLSCROLLINGPANELLIST_H
#define LL_LLSCROLLINGPANELLIST_H

#include <vector>

#include "llui.h"
#include "lluictrlfactory.h"
#include "llview.h"
#include "llpanel.h"

/*
 * Pure virtual class represents a scrolling panel.
 */
class LLScrollingPanel : public LLPanel
{
public:
	LLScrollingPanel(const LLPanel::Params& params) : LLPanel(params) {}
	virtual void updatePanel(BOOL allow_modify) = 0;
};


/*
 * A set of panels that are displayed in a vertical sequence inside a scroll container.
 */
class LLScrollingPanelList : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Params()
		{
			name = "scrolling_panel_list";
		}
	};
	LLScrollingPanelList(const Params& p)
	:	LLUICtrl(p) 
	{}
	
	static const S32 GAP_BETWEEN_PANELS = 6;

	typedef std::deque<LLScrollingPanel*>	panel_list_t;

	virtual void setValue(const LLSD& value) {};

	virtual void		draw();

	void				clearPanels();
	S32					addPanel( LLScrollingPanel* panel );
	void				removePanel( LLScrollingPanel* panel );
	void				removePanel( U32 panel_index );
	void				updatePanels(BOOL allow_modify);
	const panel_list_t&	getPanelList() { return mPanelList; }

private:
	void				updatePanelVisiblilty();

	/**
	 * Notify parent about size change, makes sense when used inside accordion
	 */
	void				notifySizeChanged(S32 height);

	panel_list_t		mPanelList;
};

#endif //LL_LLSCROLLINGPANELLIST_H
