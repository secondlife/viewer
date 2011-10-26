/** 
 * @file lldockcontrol.h
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

#ifndef LL_DOCKCONTROL_H
#define LL_DOCKCONTROL_H

#include "llerror.h"
#include "llview.h"
#include "llfloater.h"
#include "lluiimage.h"

/**
 * Provides services for docking of specified floater.
 * This class should be used in case impossibility deriving from LLDockableFloater.
 */
class LLDockControl
{
public:
	enum DocAt
	{
		TOP,
		LEFT,
		RIGHT,
		BOTTOM
	};

public:
	// callback for a function getting a rect valid for control's position
	typedef boost::function<void (LLRect& )> get_allowed_rect_callback_t;

	LOG_CLASS(LLDockControl);
	LLDockControl(LLView* dockWidget, LLFloater* dockableFloater,
			const LLUIImagePtr& dockTongue, DocAt dockAt, get_allowed_rect_callback_t get_rect_callback = NULL);
	virtual ~LLDockControl();

public:
	void on();
	void off();
	void forceRecalculatePosition();
	void setDock(LLView* dockWidget);
	LLView* getDock()
	{
		return mDockWidget;
	}
	void repositionDockable();
	void drawToungue();
	bool isDockVisible();

	// gets a rect that bounds possible positions for a dockable control (EXT-1111)
	void getAllowedRect(LLRect& rect);

	S32 getTongueWidth() { return mDockTongue->getWidth(); }
	S32 getTongueHeight() { return mDockTongue->getHeight(); }

private:
	virtual void moveDockable();
private:
	get_allowed_rect_callback_t mGetAllowedRectCallback;
	bool mEnabled;
	bool mRecalculateDockablePosition;
	bool mDockWidgetVisible;
	DocAt mDockAt;
	LLView* mDockWidget;
	LLRect mPrevDockRect;
	LLRect mRootRect;
	LLRect mFloaterRect;
	LLFloater* mDockableFloater;
	LLUIImagePtr mDockTongue;
	S32 mDockTongueX;
	S32 mDockTongueY;
};

#endif /* LL_DOCKCONTROL_H */
