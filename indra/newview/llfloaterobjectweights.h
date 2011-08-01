/**
 * @file llfloaterobjectweights.h
 * @brief Object weights advanced view floater
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

#ifndef LL_LLFLOATEROBJECTWEIGHTS_H
#define LL_LLFLOATEROBJECTWEIGHTS_H

#include "llfloater.h"

class LLLandImpactsObserver;
class LLObjectSelection;
class LLParcelSelection;
class LLTextBox;

class LLFloaterObjectWeights : public LLFloater
{
public:
	LOG_CLASS(LLFloaterObjectWeights);

	typedef LLSafeHandle<LLObjectSelection> LLObjectSelectionHandle;
	typedef LLSafeHandle<LLParcelSelection> LLParcelSelectionHandle;

	LLFloaterObjectWeights(const LLSD& key);
	~LLFloaterObjectWeights();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	void updateLandImpacts();

private:
	void refresh();

	void toggleWeightsLoadingIndicators(bool visible);
	void toggleLandImpactsLoadingIndicators(bool visible);

	void updateIfNothingSelected();

	LLTextBox		*mSelectedObjects;
	LLTextBox		*mSelectedPrims;

	LLTextBox		*mSelectedDownloadWeight;
	LLTextBox		*mSelectedPhysicsWeight;
	LLTextBox		*mSelectedServerWeight;
	LLTextBox		*mSelectedDisplayWeight;

	LLTextBox		*mSelectedOnLand;
	LLTextBox		*mRezzedOnLand;
	LLTextBox		*mRemainingCapacity;
	LLTextBox		*mTotalCapacity;

	LLLandImpactsObserver		*mLandImpactsObserver;

	LLObjectSelectionHandle		mObjectSelection;
	LLParcelSelectionHandle		mParcelSelection;

	boost::signals2::connection	mSelectMgrConnection;
};

#endif //LL_LLFLOATEROBJECTWEIGHTS_H
