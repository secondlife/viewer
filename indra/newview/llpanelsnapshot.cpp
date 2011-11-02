/** 
 * @file llpanelsnapshot.cpp
 * @brief Snapshot panel base class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#include "llpanelsnapshot.h"

// libs
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltrans.h"

// newview
#include "llsidetraypanelcontainer.h"

LLFloaterSnapshot::ESnapshotFormat LLPanelSnapshot::getImageFormat() const
{
	return LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG;
}

LLSpinCtrl* LLPanelSnapshot::getWidthSpinner()
{
	return getChild<LLSpinCtrl>(getWidthSpinnerName());
}

LLSpinCtrl* LLPanelSnapshot::getHeightSpinner()
{
	return getChild<LLSpinCtrl>(getHeightSpinnerName());
}

S32 LLPanelSnapshot::getTypedPreviewWidth() const
{
	return getChild<LLUICtrl>(getWidthSpinnerName())->getValue().asInteger();
}

S32 LLPanelSnapshot::getTypedPreviewHeight() const
{
	return getChild<LLUICtrl>(getHeightSpinnerName())->getValue().asInteger();
}

void LLPanelSnapshot::enableAspectRatioCheckbox(BOOL enable)
{
	getChild<LLUICtrl>(getAspectRatioCBName())->setEnabled(enable);
}

LLSideTrayPanelContainer* LLPanelSnapshot::getParentContainer()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if (!parent)
	{
		llwarns << "Cannot find panel container" << llendl;
		return NULL;
	}

	return parent;
}

void LLPanelSnapshot::updateImageQualityLevel()
{
	LLSliderCtrl* quality_slider = getChild<LLSliderCtrl>("image_quality_slider");
	S32 quality_val = llfloor((F32) quality_slider->getValue().asReal());

	std::string quality_lvl;

	if (quality_val < 20)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_very_low");
	}
	else if (quality_val < 40)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_low");
	}
	else if (quality_val < 60)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_medium");
	}
	else if (quality_val < 80)
	{
		quality_lvl = LLTrans::getString("snapshot_quality_high");
	}
	else
	{
		quality_lvl = LLTrans::getString("snapshot_quality_very_high");
	}

	getChild<LLTextBox>("image_quality_level")->setTextArg("[QLVL]", quality_lvl);
}
