/** 
 * @file llscrollingpanelparam.h
 * @brief the scrolling panel containing a list of visual param 
 *  	  panels
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_SCROLLINGPANELPARAMBASE_H
#define LL_SCROLLINGPANELPARAMBASE_H

#include "llpanel.h"
#include "llscrollingpanellist.h"

class LLViewerJointMesh;
class LLViewerVisualParam;
class LLWearable;
class LLVisualParamHint;
class LLViewerVisualParam;
class LLJoint;

class LLScrollingPanelParamBase : public LLScrollingPanel
{
public:
	LLScrollingPanelParamBase( const LLPanel::Params& panel_params,
				   LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, LLJoint* jointp, BOOL use_hints = FALSE );
	virtual ~LLScrollingPanelParamBase();

	virtual void		updatePanel(BOOL allow_modify);

	static void			onSliderMoved(LLUICtrl* ctrl, void* userdata);

	F32					weightToPercent( F32 weight );
	F32					percentToWeight( F32 percent );

public:
	LLViewerVisualParam* mParam;
protected:
	BOOL mAllowModify;
	LLWearable *mWearable;
}; 

#endif
