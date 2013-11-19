/** 
 * @file lldockablefloater.h
 * @brief Creates a panel of a specific kind for a toast.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_DOCKABLEFLOATER_H
#define LL_DOCKABLEFLOATER_H

#include "llerror.h"
#include "llfloater.h"
#include "lldockcontrol.h"

/**
 * Represents floater that can dock.
 * In case impossibility deriving from LLDockableFloater use LLDockControl.
 */
class LLDockableFloater : public LLFloater
{
	static const U32 UNDOCK_LEAP_HEIGHT = 12;

	static void init(LLDockableFloater* thiz);
public:
	LOG_CLASS(LLDockableFloater);
	LLDockableFloater(LLDockControl* dockControl, const LLSD& key,
			const Params& params = getDefaultParams());

	/**
	 * Constructor.
	 * @param dockControl a pointer to the doc control instance
	 * @param uniqueDocking - a flag defines is docking should work as tab(at one
	 * moment only one docked floater can be shown), also this flag defines is dock
	 * tongue should be used.
	 * @params key a floater key.
 	 * @params params a floater parameters
	 */
	LLDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
			const LLSD& key, const Params& params = getDefaultParams());

	/**
	 * Constructor.
	 * @param dockControl a pointer to the doc control instance
	 * @param uniqueDocking - a flag defines is docking should work as tab(at one
	 * moment only one docked floater can be shown).
	 * @praram useTongue - a flag defines is dock tongue should be used.
	 * @params key a floater key.
 	 * @params params a floater parameters
	 */
	LLDockableFloater(LLDockControl* dockControl, bool uniqueDocking,
			bool useTongue, const LLSD& key,
			const Params& params = getDefaultParams());

	virtual ~LLDockableFloater();

	static LLHandle<LLFloater> getInstanceHandle() { return sInstanceHandle; }

	static void toggleInstance(const LLSD& sdname);

	/**
	 *  If descendant class overrides postBuild() in order to perform specific
	 *  construction then it must still invoke its superclass' implementation.
	 */
	/* virtula */BOOL postBuild();
	/* virtual */void setDocked(bool docked, bool pop_on_undock = true);
	/* virtual */void draw();

	/**
	 *  If descendant class overrides setVisible() then it must still invoke its
	 *  superclass' implementation.
	 */
	/*virtual*/ void setVisible(BOOL visible);

	/**
	 *  If descendant class overrides setMinimized() then it must still invoke its
	 *  superclass' implementation.
	 */
	/*virtual*/ void setMinimized(BOOL minimize);

	LLView * getDockWidget();

	virtual void onDockHidden();
	virtual void onDockShown();

	LLDockControl* getDockControl();

	/**
	 * Returns true if screen channel should consider floater's size when drawing toasts.
	 *
	 * By default returns false.
	 */
	virtual bool overlapsScreenChannel() { return mOverlapsScreenChannel && getVisible() && isDocked(); }
	virtual void setOverlapsScreenChannel(bool overlaps) { mOverlapsScreenChannel = overlaps; }

	bool getUniqueDocking() { return mUniqueDocking;	}
	bool getUseTongue() { return mUseTongue; }

	void setUseTongue(bool use_tongue) { mUseTongue = use_tongue;}
private:
	/**
	 * Provides unique of dockable floater.
	 * If dockable floater already exists it should  be closed.
	 */
	void resetInstance();

protected:
	void setDockControl(LLDockControl* dockControl);
	const LLUIImagePtr& getDockTongue(LLDockControl::DocAt dock_side = LLDockControl::TOP);

	// Checks if docking should be forced.
	// It may be useful e.g. if floater created in mouselook mode (see EXT-5609)
	boost::function<BOOL ()> mIsDockedStateForcedCallback;

private:
	std::auto_ptr<LLDockControl> mDockControl;
	LLUIImagePtr mDockTongue;
	static LLHandle<LLFloater> sInstanceHandle;
	/**
	 * Provides possibility to define that dockable floaters can be docked
	 *  non exclusively.
	 */
	bool mUniqueDocking;

	bool mUseTongue;

	bool mOverlapsScreenChannel;

	// Force docking when the floater is being shown for the first time.
	bool mForceDocking;
};

#endif /* LL_DOCKABLEFLOATER_H */
