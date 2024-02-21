/** 
 * @file llpanelsnapshot.h
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

#ifndef LL_LLPANELSNAPSHOT_H
#define LL_LLPANELSNAPSHOT_H

//#include "llfloatersnapshot.h"
#include "llpanel.h"
#include "llsnapshotmodel.h"

class LLSpinCtrl;
class LLSideTrayPanelContainer;
class LLFloaterSnapshotBase;

/**
 * Snapshot panel base class.
 */
class LLPanelSnapshot: public LLPanel
{
public:
	LLPanelSnapshot();

	bool postBuild() override;
	void onOpen(const LLSD& key) override;

	virtual std::string getWidthSpinnerName() const = 0;
	virtual std::string getHeightSpinnerName() const = 0;
	virtual std::string getAspectRatioCBName() const = 0;
	virtual std::string getImageSizeComboName() const = 0;
	virtual std::string getImageSizePanelName() const = 0;

	virtual S32 getTypedPreviewWidth() const;
	virtual S32 getTypedPreviewHeight() const;
	virtual LLSpinCtrl* getWidthSpinner();
	virtual LLSpinCtrl* getHeightSpinner();
	virtual void enableAspectRatioCheckbox(bool enable);
    virtual LLSnapshotModel::ESnapshotFormat getImageFormat() const;
	virtual LLSnapshotModel::ESnapshotType getSnapshotType();
	virtual void updateControls(const LLSD& info) = 0; ///< Update controls from saved settings
	void enableControls(bool enable);

protected:
	LLSideTrayPanelContainer* getParentContainer();
	void updateImageQualityLevel();
	void goBack(); ///< Switch to the default (Snapshot Options) panel
	virtual void cancel();

	// common UI callbacks
	void onCustomResolutionCommit();
	void onResolutionComboCommit(LLUICtrl* ctrl);
	void onKeepAspectRatioCommit(LLUICtrl* ctrl);

	LLFloaterSnapshotBase* mSnapshotFloater;
};

#endif // LL_LLPANELSNAPSHOT_H
