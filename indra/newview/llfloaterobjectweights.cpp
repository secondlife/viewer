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

#include "lltextbox.h"

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
	mTotalCapacity(NULL)
{
}

LLFloaterObjectWeights::~LLFloaterObjectWeights()
{
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

	mSelectedOnLand = getChild<LLTextBox>("used_download_weight");
	mRezzedOnLand = getChild<LLTextBox>("used_download_weight");
	mRemainingCapacity = getChild<LLTextBox>("used_download_weight");
	mTotalCapacity = getChild<LLTextBox>("used_download_weight");

	return TRUE;
}

// virtual
void LLFloaterObjectWeights::onOpen(const LLSD& key)
{
	updateIfNothingSelected();
}

void LLFloaterObjectWeights::toggleLoadingIndicators(bool visible)
{
	childSetVisible("download_loading_indicator", visible);
	childSetVisible("physics_loading_indicator", visible);
	childSetVisible("server_loading_indicator", visible);
	childSetVisible("display_loading_indicator", visible);
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

	toggleLoadingIndicators(false);
}
