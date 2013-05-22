/** 
 * @file llscrollingpanelparam.cpp
 * @brief UI panel for a list of visual param panels
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

#include "llviewerprecompiledheaders.h"

#include "llscrollingpanelparambase.h"
#include "llviewerjointmesh.h"
#include "llviewervisualparam.h"
#include "llwearable.h"
#include "llviewervisualparam.h"
#include "lltoolmorph.h"
#include "lltrans.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llagent.h"
#include "llviewborder.h"
#include "llvoavatarself.h"

LLScrollingPanelParamBase::LLScrollingPanelParamBase( const LLPanel::Params& panel_params,
						      LLViewerJointMesh* mesh, LLViewerVisualParam* param, BOOL allow_modify, LLWearable* wearable, LLJoint* jointp, BOOL use_hints)
	: LLScrollingPanel( panel_params ),
	  mParam(param),
	  mAllowModify(allow_modify),
	  mWearable(wearable)
{
	if (use_hints)
		buildFromFile( "panel_scrolling_param.xml");
	else
		buildFromFile( "panel_scrolling_param_base.xml");
	
	getChild<LLUICtrl>("param slider")->setValue(weightToPercent(param->getWeight()));

	std::string display_name = LLTrans::getString(param->getDisplayName());
	getChild<LLUICtrl>("param slider")->setLabelArg("[DESC]", display_name);
	getChildView("param slider")->setEnabled(mAllowModify);
	childSetCommitCallback("param slider", LLScrollingPanelParamBase::onSliderMoved, this);

	setVisible(FALSE);
	setBorderVisible( FALSE );
}

LLScrollingPanelParamBase::~LLScrollingPanelParamBase()
{
}

void LLScrollingPanelParamBase::updatePanel(BOOL allow_modify)
{
	LLViewerVisualParam* param = mParam;

	if (!mWearable)
	{
		// not editing a wearable just now, no update necessary
		return;
	}

	F32 current_weight = mWearable->getVisualParamWeight( param->getID() );
	getChild<LLUICtrl>("param slider")->setValue(weightToPercent( current_weight ) );
	mAllowModify = allow_modify;
	getChildView("param slider")->setEnabled(mAllowModify);
}

// static
void LLScrollingPanelParamBase::onSliderMoved(LLUICtrl* ctrl, void* userdata)
{
	LLSliderCtrl* slider = (LLSliderCtrl*) ctrl;
	LLScrollingPanelParamBase* self = (LLScrollingPanelParamBase*) userdata;
	LLViewerVisualParam* param = self->mParam;
	
	F32 current_weight = self->mWearable->getVisualParamWeight( param->getID() );
	F32 new_weight = self->percentToWeight( (F32)slider->getValue().asReal() );
	if (current_weight != new_weight )
	{
		self->mWearable->setVisualParamWeight( param->getID(), new_weight, FALSE );
		self->mWearable->writeToAvatar(gAgentAvatarp);
		gAgentAvatarp->updateVisualParams();
	}
}

F32 LLScrollingPanelParamBase::weightToPercent( F32 weight )
{
	LLViewerVisualParam* param = mParam;
	return (weight - param->getMinWeight()) /  (param->getMaxWeight() - param->getMinWeight()) * 100.f;
}

F32 LLScrollingPanelParamBase::percentToWeight( F32 percent )
{
	LLViewerVisualParam* param = mParam;
	return percent / 100.f * (param->getMaxWeight() - param->getMinWeight()) + param->getMinWeight();
}
