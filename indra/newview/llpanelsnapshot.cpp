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
#include "llcombobox.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltrans.h"

// newview
#include "llsidetraypanelcontainer.h"
#include "llviewercontrol.h" // gSavedSettings

const S32 MAX_TEXTURE_SIZE = 512 ; //max upload texture size 512 * 512

S32 power_of_two(S32 sz, S32 upper)
{
	S32 res = upper;
	while( upper >= sz)
	{
		res = upper;
		upper >>= 1;
	}
	return res;
}

// virtual
BOOL LLPanelSnapshot::postBuild()
{
	getChild<LLUICtrl>(getImageSizeComboName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onResolutionComboCommit, this, _1));
	getChild<LLUICtrl>(getWidthSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onCustomResolutionCommit, this));
	getChild<LLUICtrl>(getHeightSpinnerName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onCustomResolutionCommit, this));
	getChild<LLUICtrl>(getAspectRatioCBName())->setCommitCallback(boost::bind(&LLPanelSnapshot::onKeepAspectRatioCommit, this, _1));

	updateControls(LLSD());
	return TRUE;
}

// virtual
void LLPanelSnapshot::onOpen(const LLSD& key)
{
	S32 old_format = gSavedSettings.getS32("SnapshotFormat");
	S32 new_format = (S32) getImageFormat();

	gSavedSettings.setS32("SnapshotFormat", new_format);
	setCtrlsEnabled(true);

	// Switching panels will likely change image format.
	// Not updating preview right away may lead to errors,
	// e.g. attempt to send a large BMP image by email.
	if (old_format != new_format)
	{
		LLFloaterSnapshot::getInstance()->notify(LLSD().with("image-format-change", true));
	}
}

LLFloaterSnapshot::ESnapshotFormat LLPanelSnapshot::getImageFormat() const
{
	return LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG;
}

void LLPanelSnapshot::enableControls(BOOL enable)
{
	setCtrlsEnabled(enable);
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
		LL_WARNS() << "Cannot find panel container" << LL_ENDL;
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

void LLPanelSnapshot::goBack()
{
	LLSideTrayPanelContainer* parent = getParentContainer();
	if (parent)
	{
		parent->openPreviousPanel();
		parent->getCurrentPanel()->onOpen(LLSD());
	}
}

void LLPanelSnapshot::cancel()
{
	goBack();
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("set-ready", true));
}

void LLPanelSnapshot::onCustomResolutionCommit()
{
	LLSD info;
	LLSpinCtrl *widthSpinner = getChild<LLSpinCtrl>(getWidthSpinnerName());
	LLSpinCtrl *heightSpinner = getChild<LLSpinCtrl>(getHeightSpinnerName());
	if (getName() == "panel_snapshot_inventory")
	{
		S32 width = widthSpinner->getValue().asInteger();
		width = power_of_two(width, MAX_TEXTURE_SIZE);
		info["w"] = width;
		widthSpinner->setIncrement(width >> 1);
		widthSpinner->forceSetValue(width);
		S32 height =  heightSpinner->getValue().asInteger();
		height = power_of_two(height, MAX_TEXTURE_SIZE);
		heightSpinner->setIncrement(height >> 1);
		heightSpinner->forceSetValue(height);
		info["h"] = height;
	}
	else
	{
		info["w"] = widthSpinner->getValue().asInteger();
		info["h"] = heightSpinner->getValue().asInteger();
	}
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("custom-res-change", info));
}

void LLPanelSnapshot::onResolutionComboCommit(LLUICtrl* ctrl)
{
	LLSD info;
	info["combo-res-change"]["control-name"] = ctrl->getName();
	LLFloaterSnapshot::getInstance()->notify(info);
}

void LLPanelSnapshot::onKeepAspectRatioCommit(LLUICtrl* ctrl)
{
	LLFloaterSnapshot::getInstance()->notify(LLSD().with("keep-aspect-change", ctrl->getValue().asBoolean()));
}
