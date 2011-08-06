/**
 * @file llfloaterobjectweights.cpp
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
#include "llviewerprecompiledheaders.h"

#include "llfloaterobjectweights.h"

#include "llparcel.h"

#include "llfloaterreg.h"
#include "lltextbox.h"

#include "llselectmgr.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

struct LLCrossParcelFunctor : public LLSelectedObjectFunctor
{
	/*virtual*/ bool apply(LLViewerObject* obj)
	{
		return obj->crossesParcelBounds();
	}
};

/**
 * Class LLLandImpactsObserver
 *
 * An observer class to monitor parcel selection and update
 * the land impacts data from a parcel containing the selected object.
 */
class LLLandImpactsObserver : public LLParcelObserver
{
public:
	virtual void changed()
	{
		LLFloaterObjectWeights* object_weights_floater = LLFloaterReg::getTypedInstance<LLFloaterObjectWeights>("object_weights");
		if(object_weights_floater)
		{
			object_weights_floater->updateLandImpacts();
		}
	}
};


LLFloaterObjectWeights::LLFloaterObjectWeights(const LLSD& key)
:	LLFloater(key),
	mSelectedObjects(NULL),
	mSelectedPrims(NULL),
	mSelectedDownloadWeight(NULL),
	mSelectedPhysicsWeight(NULL),
	mSelectedServerWeight(NULL),
	mSelectedDisplayWeight(NULL),
	mSelectedOnLand(NULL),
	mRezzedOnLand(NULL),
	mRemainingCapacity(NULL),
	mTotalCapacity(NULL),
	mLandImpactsObserver(NULL)
{
	mLandImpactsObserver = new LLLandImpactsObserver();
	LLViewerParcelMgr::getInstance()->addObserver(mLandImpactsObserver);
}

LLFloaterObjectWeights::~LLFloaterObjectWeights()
{
	mObjectSelection = NULL;
	mParcelSelection = NULL;

	mSelectMgrConnection.disconnect();

	LLViewerParcelMgr::getInstance()->removeObserver(mLandImpactsObserver);
	delete mLandImpactsObserver;
}

// virtual
BOOL LLFloaterObjectWeights::postBuild()
{
	mSelectedObjects = getChild<LLTextBox>("objects");
	mSelectedPrims = getChild<LLTextBox>("prims");

	mSelectedDownloadWeight = getChild<LLTextBox>("download");
	mSelectedPhysicsWeight = getChild<LLTextBox>("physics");
	mSelectedServerWeight = getChild<LLTextBox>("server");
	mSelectedDisplayWeight = getChild<LLTextBox>("display");

	mSelectedOnLand = getChild<LLTextBox>("selected");
	mRezzedOnLand = getChild<LLTextBox>("rezzed_on_land");
	mRemainingCapacity = getChild<LLTextBox>("remaining_capacity");
	mTotalCapacity = getChild<LLTextBox>("total_capacity");

	return TRUE;
}

// virtual
void LLFloaterObjectWeights::onOpen(const LLSD& key)
{
	mSelectMgrConnection = LLSelectMgr::instance().mUpdateSignal.connect(boost::bind(&LLFloaterObjectWeights::refresh, this));

	mObjectSelection = LLSelectMgr::getInstance()->getEditSelection();
	mParcelSelection = LLViewerParcelMgr::getInstance()->getFloatingParcelSelection();

	refresh();
}

// virtual
void LLFloaterObjectWeights::onClose(bool app_quitting)
{
	mSelectMgrConnection.disconnect();

	mObjectSelection = NULL;
	mParcelSelection = NULL;
}

void LLFloaterObjectWeights::updateLandImpacts()
{
	LLParcel *parcel = mParcelSelection->getParcel();
	if (!parcel || LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		updateIfNothingSelected();
	}
	else
	{
		S32 selected_prims = parcel->getSelectedPrimCount();
		S32 rezzed_prims = parcel->getSimWidePrimCount();
		S32 total_capacity = parcel->getSimWideMaxPrimCapacity();

		mSelectedOnLand->setText(llformat("%d", selected_prims));
		mRezzedOnLand->setText(llformat("%d", rezzed_prims));
		mRemainingCapacity->setText(llformat("%d", total_capacity - rezzed_prims));
		mTotalCapacity->setText(llformat("%d", total_capacity));

		toggleLandImpactsLoadingIndicators(false);
	}
}

void LLFloaterObjectWeights::refresh()
{
	if (LLSelectMgr::getInstance()->getSelection()->isEmpty())
	{
		updateIfNothingSelected();
	}
	else
	{
		S32 prim_count = LLSelectMgr::getInstance()->getSelection()->getObjectCount();
		S32 link_count = LLSelectMgr::getInstance()->getSelection()->getRootObjectCount();

		mSelectedObjects->setText(llformat("%d", link_count));
		mSelectedPrims->setText(llformat("%d", prim_count));

		LLCrossParcelFunctor func;
		if (LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func, true))
		{
			// Some of the selected objects cross parcel bounds.
			// We don't display land impacts in this case.
			const std::string text = getString("nothing_selected");

			mSelectedOnLand->setText(text);
			mRezzedOnLand->setText(text);
			mRemainingCapacity->setText(text);
			mTotalCapacity->setText(text);

			toggleLandImpactsLoadingIndicators(false);
		}
		else
		{
			LLViewerObject* selected_object = mObjectSelection->getFirstObject();
			if (selected_object)
			{
				// Select a parcel at the currently selected object's position.
				LLViewerParcelMgr::getInstance()->selectParcelAt(selected_object->getPositionGlobal());

				toggleLandImpactsLoadingIndicators(true);
			}
		}
	}
}

void LLFloaterObjectWeights::toggleWeightsLoadingIndicators(bool visible)
{
	childSetVisible("download_loading_indicator", visible);
	childSetVisible("physics_loading_indicator", visible);
	childSetVisible("server_loading_indicator", visible);
	childSetVisible("display_loading_indicator", visible);

	mSelectedDownloadWeight->setVisible(!visible);
	mSelectedPhysicsWeight->setVisible(!visible);
	mSelectedServerWeight->setVisible(!visible);
	mSelectedDisplayWeight->setVisible(!visible);
}

void LLFloaterObjectWeights::toggleLandImpactsLoadingIndicators(bool visible)
{
	childSetVisible("selected_loading_indicator", visible);
	childSetVisible("rezzed_on_land_loading_indicator", visible);
	childSetVisible("remaining_capacity_loading_indicator", visible);
	childSetVisible("total_capacity_loading_indicator", visible);

	mSelectedOnLand->setVisible(!visible);
	mRezzedOnLand->setVisible(!visible);
	mRemainingCapacity->setVisible(!visible);
	mTotalCapacity->setVisible(!visible);
}

void LLFloaterObjectWeights::updateIfNothingSelected()
{
	const std::string text = getString("nothing_selected");

	mSelectedObjects->setText(text);
	mSelectedPrims->setText(text);

	mSelectedDownloadWeight->setText(text);
	mSelectedPhysicsWeight->setText(text);
	mSelectedServerWeight->setText(text);
	mSelectedDisplayWeight->setText(text);

	mSelectedOnLand->setText(text);
	mRezzedOnLand->setText(text);
	mRemainingCapacity->setText(text);
	mTotalCapacity->setText(text);

	toggleWeightsLoadingIndicators(false);
	toggleLandImpactsLoadingIndicators(false);
}
