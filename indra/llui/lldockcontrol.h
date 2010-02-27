/** 
 * @file lldockcontrol.h
 * @brief Creates a panel of a specific kind for a toast.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
	bool mRecalculateDocablePosition;
	bool mDockWidgetVisible;
	DocAt mDockAt;
	LLView* mDockWidget;
	LLRect mPrevDockRect;
	LLRect mRootRect;
	LLFloater* mDockableFloater;
	LLUIImagePtr mDockTongue;
	S32 mDockTongueX;
	S32 mDockTongueY;
};

#endif /* LL_DOCKCONTROL_H */
