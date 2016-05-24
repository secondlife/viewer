/** 
 * @file lltransientfloatermgr.h
 * @brief LLFocusMgr base class
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
		GLOBAL, DOCKED, IM
	};

	void registerTransientFloater(LLTransientFloater* floater);
	void unregisterTransientFloater(LLTransientFloater* floater);
	void addControlView(ETransientGroup group, LLView* view);
	void removeControlView(ETransientGroup group, LLView* view);
	void addControlView(LLView* view);
	void removeControlView(LLView* view);

private:
	typedef std::set<LLHandle<LLView> > controls_set_t;
	typedef std::map<ETransientGroup, controls_set_t > group_controls_t;

	void hideTransientFloaters(S32 x, S32 y);
	void leftMouseClickCallback(S32 x, S32 y, MASK mask);
	bool isControlClicked(ETransientGroup group, controls_set_t& set, S32 x, S32 y);

	std::set<LLTransientFloater*> mTransSet;

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
