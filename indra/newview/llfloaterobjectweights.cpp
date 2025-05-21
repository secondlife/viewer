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

#include "llagent.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"

static const std::string lod_strings[4] =
{
    "lowest_lod",
    "low_lod",
    "medium_lod",
    "high_lod",
};

// virtual
bool LLCrossParcelFunctor::apply(LLViewerObject* obj)
{
    // Add the root object box.
    mBoundingBox.addBBoxAgent(LLBBox(obj->getPositionRegion(), obj->getRotationRegion(), obj->getScale() * -0.5f, obj->getScale() * 0.5f).getAxisAligned());

    // Extend the bounding box across all the children.
    LLViewerObject::const_child_list_t children = obj->getChildren();
    for (LLViewerObject::const_child_list_t::const_iterator iter = children.begin();
         iter != children.end(); iter++)
    {
        LLViewerObject* child = *iter;
        mBoundingBox.addBBoxAgent(LLBBox(child->getPositionRegion(), child->getRotationRegion(), child->getScale() * -0.5f, child->getScale() * 0.5f).getAxisAligned());
    }

    bool result = false;

    LLViewerRegion* region = obj->getRegion();
    if (region)
    {
        std::vector<LLBBox> boxes;
        boxes.push_back(mBoundingBox);
        result = region->objectsCrossParcel(boxes);
    }

    return result;
}

LLFloaterObjectWeights::LLFloaterObjectWeights(const LLSD& key)
:   LLFloater(key),
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
    mLodLevel(nullptr),
    mTrianglesShown(nullptr),
    mPixelArea(nullptr)
{
}

LLFloaterObjectWeights::~LLFloaterObjectWeights()
{
}

// virtual
bool LLFloaterObjectWeights::postBuild()
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

    mLodLevel = getChild<LLTextBox>("lod_level");
    mTrianglesShown = getChild<LLTextBox>("triangles_shown");
    mPixelArea = getChild<LLTextBox>("pixel_area");

    return true;
}

// virtual
void LLFloaterObjectWeights::onOpen(const LLSD& key)
{
    refresh();
    updateLandImpacts(LLViewerParcelMgr::getInstance()->getFloatingParcelSelection()->getParcel());
}

// virtual
void LLFloaterObjectWeights::onWeightsUpdate(const SelectionCost& selection_cost)
{
    mSelectedDownloadWeight->setText(llformat("%.1f", selection_cost.mNetworkCost));
    mSelectedPhysicsWeight->setText(llformat("%.1f", selection_cost.mPhysicsCost));
    mSelectedServerWeight->setText(llformat("%.1f", selection_cost.mSimulationCost));

    S32 render_cost = LLSelectMgr::getInstance()->getSelection()->getSelectedObjectRenderCost();
    mSelectedDisplayWeight->setText(llformat("%d", render_cost));

    toggleWeightsLoadingIndicators(false);
}

//virtual
void LLFloaterObjectWeights::setErrorStatus(S32 status, const std::string& reason)
{
    const std::string text = getString("nothing_selected");

    mSelectedDownloadWeight->setText(text);
    mSelectedPhysicsWeight->setText(text);
    mSelectedServerWeight->setText(text);
    mSelectedDisplayWeight->setText(text);

    toggleWeightsLoadingIndicators(false);
}

void LLFloaterObjectWeights::draw()
{
    // Normally it's a bad idea to set text and visibility inside draw
    // since it can cause rect updates go to different, already drawn elements,
    // but floater is very simple and these elements are supposed to be isolated
    LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
    if (selection->isEmpty())
    {
        const std::string text = getString("nothing_selected");
        mLodLevel->setText(text);
        mTrianglesShown->setText(text);
        mPixelArea->setText(text);

        toggleRenderLoadingIndicators(false);
    }
    else
    {
        S32 object_lod = -1;
        bool multiple_lods = false;
        S32 total_tris = 0;
        F32 pixel_area = 0;
        for (LLObjectSelection::valid_root_iterator iter = selection->valid_root_begin();
            iter != selection->valid_root_end(); ++iter)
        {
            LLViewerObject* object = (*iter)->getObject();
            S32 lod = object->getLOD();
            if (object_lod < 0)
            {
                object_lod = lod;
            }
            else if (object_lod != lod)
            {
                multiple_lods = true;
            }

            if (object->isRootEdit())
            {
                total_tris += object->recursiveGetTriangleCount();
                pixel_area += object->getPixelArea();
            }
        }

        if (multiple_lods)
        {
            mLodLevel->setText(getString("multiple_lods"));
            toggleRenderLoadingIndicators(false);
        }
        else if (object_lod < 0)
        {
            // nodes are waiting for data
            toggleRenderLoadingIndicators(true);
        }
        else
        {
            mLodLevel->setText(getString(lod_strings[object_lod]));
            toggleRenderLoadingIndicators(false);
        }
        mTrianglesShown->setText(llformat("%d", total_tris));
        mPixelArea->setText(llformat("%d", pixel_area));
    }
    LLFloater::draw();
}

void LLFloaterObjectWeights::updateLandImpacts(const LLParcel* parcel)
{
    if (!parcel || LLSelectMgr::getInstance()->getSelection()->isEmpty())
    {
        updateIfNothingSelected();
    }
    else
    {
        S32 rezzed_prims = parcel->getSimWidePrimCount();
        S32 total_capacity = parcel->getSimWideMaxPrimCapacity();
        // Can't have more than region max tasks, regardless of parcel
        // object bonus factor.
        LLViewerRegion* region = LLViewerParcelMgr::getInstance()->getSelectionRegion();
        if (region)
        {
            S32 max_tasks_per_region = (S32)region->getMaxTasks();
            total_capacity = llmin(total_capacity, max_tasks_per_region);
        }

        mRezzedOnLand->setText(llformat("%d", rezzed_prims));
        mRemainingCapacity->setText(llformat("%d", total_capacity - rezzed_prims));
        mTotalCapacity->setText(llformat("%d", total_capacity));

        toggleLandImpactsLoadingIndicators(false);
    }
}

void LLFloaterObjectWeights::refresh()
{
    LLSelectMgr* sel_mgr = LLSelectMgr::getInstance();

    if (sel_mgr->getSelection()->isEmpty())
    {
        updateIfNothingSelected();
    }
    else
    {
        S32 prim_count = sel_mgr->getSelection()->getObjectCount();
        S32 link_count = sel_mgr->getSelection()->getRootObjectCount();
        F32 prim_equiv = sel_mgr->getSelection()->getSelectedLinksetCost();

        mSelectedObjects->setText(llformat("%d", link_count));
        mSelectedPrims->setText(llformat("%d", prim_count));
        mSelectedOnLand->setText(llformat("%.1d", (S32)prim_equiv));

        LLCrossParcelFunctor func;
        if (sel_mgr->getSelection()->applyToRootObjects(&func, true))
        {
            // Some of the selected objects cross parcel bounds.
            // We don't display object weights and land impacts in this case.
            const std::string text = getString("nothing_selected");

            mRezzedOnLand->setText(text);
            mRemainingCapacity->setText(text);
            mTotalCapacity->setText(text);

            toggleLandImpactsLoadingIndicators(false);
        }

        LLViewerRegion* region = gAgent.getRegion();
        if (region && region->capabilitiesReceived())
        {
            for (LLObjectSelection::valid_root_iterator iter = sel_mgr->getSelection()->valid_root_begin();
                    iter != sel_mgr->getSelection()->valid_root_end(); ++iter)
            {
                LLAccountingCostManager::getInstance()->addObject((*iter)->getObject()->getID());
            }

            std::string url = region->getCapability("ResourceCostSelected");
            if (!url.empty())
            {
                // Update the transaction id before the new fetch request
                generateTransactionID();

                LLAccountingCostManager::getInstance()->fetchCosts(Roots, url, getObserverHandle());
                toggleWeightsLoadingIndicators(true);
            }
        }
        else
        {
            LL_WARNS() << "Failed to get region capabilities" << LL_ENDL;
        }
    }
}

// virtual
void LLFloaterObjectWeights::generateTransactionID()
{
    mTransactionID.generate();
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

void LLFloaterObjectWeights::toggleRenderLoadingIndicators(bool visible)
{
    childSetVisible("lod_level_loading_indicator", visible);
    childSetVisible("triangles_shown_loading_indicator", visible);
    childSetVisible("pixel_area_loading_indicator", visible);

    mLodLevel->setVisible(!visible);
    mTrianglesShown->setVisible(!visible);
    mPixelArea->setVisible(!visible);
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

    mLodLevel->setText(text);
    mTrianglesShown->setText(text);
    mPixelArea->setText(text);

    toggleWeightsLoadingIndicators(false);
    toggleLandImpactsLoadingIndicators(false);
    toggleRenderLoadingIndicators(false);
}
