/** 
 * @file lltransientfloatermgr.h
 * @brief LLFocusMgr base class
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

#ifndef LL_LLTRANSIENTFLOATERMGR_H
#define LL_LLTRANSIENTFLOATERMGR_H

#include "llui.h"
#include "llsingleton.h"
#include "llfloater.h"

class LLTransientFloater;

/**
 * Provides functionality to hide transient floaters.
 */
class LLTransientFloaterMgr: public LLSingleton<LLTransientFloaterMgr>
{
protected:
	LLTransientFloaterMgr();
	friend class LLSingleton<LLTransientFloaterMgr>;

public:
	enum ETransientGroup
	{
		GLOBAL, IM
	};

	void registerTransientFloater(LLTransientFloater* floater);
	void unregisterTransientFloater(LLTransientFloater* floater);
	void addControlView(ETransientGroup group, LLView* view);
	void removeControlView(ETransientGroup group, LLView* view);
	void addControlView(LLView* view);
	void removeControlView(LLView* view);

private:
	void hideTransientFloaters(S32 x, S32 y);
	void leftMouseClickCallback(S32 x, S32 y, MASK mask);
	bool isControlClicked(std::set<LLView*>& set, S32 x, S32 y);
private:
	std::set<LLTransientFloater*> mTransSet;

	typedef std::set<LLView*> controls_set_t;
	typedef std::map<ETransientGroup, std::set<LLView*> > group_controls_t;
	group_controls_t mGroupControls;
};

/**
 * An abstract class declares transient floater interfaces.
 */
class LLTransientFloater
{
protected:
	/**
	 * Class initialization method.
	 * Should be called from descendant constructor.
	 */
	void init(LLFloater* thiz);
public:
	virtual LLTransientFloaterMgr::ETransientGroup getGroup() = 0;
	bool isTransientDocked() { return mFloater->isDocked(); };
	void setTransientVisible(BOOL visible) {mFloater->setVisible(visible); }

private:
	LLFloater* mFloater;
};

#endif  // LL_LLTRANSIENTFLOATERMGR_H
