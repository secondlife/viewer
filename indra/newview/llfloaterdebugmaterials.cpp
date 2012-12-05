/** 
* @file llfloaterdebugmaterials.cpp
* @brief Implementation of llfloaterdebugmaterials
* @author Stinson@lindenlab.com
*
* $LicenseInfo:firstyear=2012&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2012, Linden Research, Inc.
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

#include "llfloaterdebugmaterials.h"

#include <string>
#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llcolorswatch.h"
#include "llenvmanager.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llhttpclient.h"
#include "lllineeditor.h"
#include "llmaterialid.h"
#include "llresmgr.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "llstring.h"
#include "llstyle.h"
#include "lltextbase.h"
#include "lltexturectrl.h"
#include "lltextvalidate.h"
#include "llthread.h"
#include "lluicolortable.h"
#include "lluictrl.h"
#include "lluuid.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "v4color.h"
#include "v4coloru.h"

#define MATERIALS_CAPABILITY_NAME                 "RenderMaterials"

#define VIEWABLE_OBJECTS_REGION_ID_FIELD          "regionId"
#define VIEWABLE_OBJECTS_OBJECT_ID_FIELD          "objectId"
#define VIEWABLE_OBJECTS_MATERIAL_ID_FIELD        "materialId"

BOOL LLFloaterDebugMaterials::postBuild()
{
	mStatusText = findChild<LLTextBase>("material_status");
	llassert(mStatusText != NULL);

	mGetButton = findChild<LLButton>("get_button");
	llassert(mGetButton != NULL);
	mGetButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGetClicked, this));

	mParsingStatusText = findChild<LLTextBase>("loading_status");
	llassert(mParsingStatusText != NULL);

	mGetNormalMapScrollList = findChild<LLScrollListCtrl>("get_normal_map_scroll_list");
	llassert(mGetNormalMapScrollList != NULL);
	mGetNormalMapScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGetScrollListSelectionChange, this, _1));

	mGetSpecularMapScrollList = findChild<LLScrollListCtrl>("get_specular_map_scroll_list");
	llassert(mGetSpecularMapScrollList != NULL);
	mGetSpecularMapScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGetScrollListSelectionChange, this, _1));

	mGetOtherDataScrollList = findChild<LLScrollListCtrl>("get_other_data_scroll_list");
	llassert(mGetOtherDataScrollList != NULL);
	mGetOtherDataScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGetScrollListSelectionChange, this, _1));

	mNormalMap = findChild<LLTextureCtrl>("normal_map");
	llassert(mNormalMap != NULL);

	mNormalMapOffsetX = findChild<LLSpinCtrl>("normal_map_offset_x");
	llassert(mNormalMapOffsetX != NULL);

	mNormalMapOffsetY = findChild<LLSpinCtrl>("normal_map_offset_y");
	llassert(mNormalMapOffsetY != NULL);

	mNormalMapRepeatX = findChild<LLSpinCtrl>("normal_map_repeat_x");
	llassert(mNormalMapRepeatX != NULL);

	mNormalMapRepeatY = findChild<LLSpinCtrl>("normal_map_repeat_y");
	llassert(mNormalMapRepeatY != NULL);

	mNormalMapRotation = findChild<LLSpinCtrl>("normal_map_rotation");
	llassert(mNormalMapRotation != NULL);

	mSpecularMap = findChild<LLTextureCtrl>("specular_map");
	llassert(mSpecularMap != NULL);

	mSpecularMapOffsetX = findChild<LLSpinCtrl>("specular_map_offset_x");
	llassert(mSpecularMapOffsetX != NULL);

	mSpecularMapOffsetY = findChild<LLSpinCtrl>("specular_map_offset_y");
	llassert(mSpecularMapOffsetY != NULL);

	mSpecularMapRepeatX = findChild<LLSpinCtrl>("specular_map_repeat_x");
	llassert(mSpecularMapRepeatX != NULL);

	mSpecularMapRepeatY = findChild<LLSpinCtrl>("specular_map_repeat_y");
	llassert(mSpecularMapRepeatY != NULL);

	mSpecularMapRotation = findChild<LLSpinCtrl>("specular_map_rotation");
	llassert(mSpecularMapRotation != NULL);

	mSpecularColor = findChild<LLColorSwatchCtrl>("specular_color");
	llassert(mSpecularColor != NULL);

	mSpecularColorAlpha = findChild<LLSpinCtrl>("specular_color_alpha");
	llassert(mSpecularColorAlpha != NULL);

	mSpecularExponent = findChild<LLLineEditor>("specular_exponent");
	llassert(mSpecularExponent != NULL);
	mSpecularExponent->setPrevalidate(LLTextValidate::validateInt);
	mSpecularExponent->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mEnvironmentExponent = findChild<LLLineEditor>("environment_exponent");
	llassert(mEnvironmentExponent != NULL);
	mEnvironmentExponent->setPrevalidate(LLTextValidate::validateInt);
	mEnvironmentExponent->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mAlphaMaskCutoff = findChild<LLLineEditor>("alpha_mask_cutoff");
	llassert(mAlphaMaskCutoff != NULL);
	mAlphaMaskCutoff->setPrevalidate(LLTextValidate::validateInt);
	mAlphaMaskCutoff->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mDiffuseAlphaMode = findChild<LLLineEditor>("diffuse_alpha_mode");
	llassert(mDiffuseAlphaMode != NULL);
	mDiffuseAlphaMode->setPrevalidate(LLTextValidate::validateInt);
	mDiffuseAlphaMode->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mPutSetButton = findChild<LLButton>("put_set_button");
	llassert(mPutSetButton != NULL);
	mPutSetButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPutSetClicked, this));

	mPutClearButton = findChild<LLButton>("put_clear_button");
	llassert(mPutClearButton != NULL);
	mPutClearButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPutClearClicked, this));

	LLButton* resetPutValuesButton = findChild<LLButton>("reset_put_values_button");
	llassert(resetPutValuesButton != NULL);
	resetPutValuesButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onResetPutValuesClicked, this));

	mPutScrollList = findChild<LLScrollListCtrl>("put_scroll_list");
	llassert(mPutScrollList != NULL);

	mQueryViewableObjectsButton = findChild<LLButton>("query_viewable_objects_button");
	llassert(mQueryViewableObjectsButton != NULL);
	mQueryViewableObjectsButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onQueryVisibleObjectsClicked, this));

	mQueryStatusText = findChild<LLTextBase>("query_status");
	llassert(mQueryStatusText != NULL);

	mViewableObjectsScrollList = findChild<LLScrollListCtrl>("viewable_objects_scroll_list");
	llassert(mViewableObjectsScrollList != NULL);
	mViewableObjectsScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onViewableObjectsScrollListSelectionChange, this));

	mPostButton = findChild<LLButton>("post_button");
	llassert(mPostButton != NULL);
	mPostButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPostClicked, this));

	mPostNormalMapScrollList = findChild<LLScrollListCtrl>("post_normal_map_scroll_list");
	llassert(mPostNormalMapScrollList != NULL);
	mPostNormalMapScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPostScrollListSelectionChange, this, _1));

	mPostSpecularMapScrollList = findChild<LLScrollListCtrl>("post_specular_map_scroll_list");
	llassert(mPostSpecularMapScrollList != NULL);
	mPostSpecularMapScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPostScrollListSelectionChange, this, _1));

	mPostOtherDataScrollList = findChild<LLScrollListCtrl>("post_other_data_scroll_list");
	llassert(mPostOtherDataScrollList != NULL);
	mPostOtherDataScrollList->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onPostScrollListSelectionChange, this, _1));

	mDefaultSpecularColor = LLUIColorTable::instance().getColor("White");

	mWarningColor = LLUIColorTable::instance().getColor("MaterialWarningColor");
	mErrorColor = LLUIColorTable::instance().getColor("MaterialErrorColor");

	setState(kNoRegion);

	return LLFloater::postBuild();
}

void LLFloaterDebugMaterials::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);

	if (!mRegionCrossConnection.connected())
	{
		mRegionCrossConnection = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterDebugMaterials::onRegionCross, this));
	}

	if (!mTeleportFailedConnection.connected())
	{
		mTeleportFailedConnection = LLViewerParcelMgr::getInstance()->setTeleportFailedCallback(boost::bind(&LLFloaterDebugMaterials::onRegionCross, this));
	}

	if (!mSelectionUpdateConnection.connected())
	{
		mSelectionUpdateConnection = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterDebugMaterials::onInWorldSelectionChange, this));
	}

	checkRegionMaterialStatus();
	resetObjectEditInputs();
	clearGetResults();
	clearViewableObjectsResults();
	clearPostResults();
}

void LLFloaterDebugMaterials::onClose(bool pIsAppQuitting)
{
	resetObjectEditInputs();
	clearGetResults();
	clearViewableObjectsResults();
	clearPostResults();

	if (mSelectionUpdateConnection.connected())
	{
		mSelectionUpdateConnection.disconnect();
	}

	if (mTeleportFailedConnection.connected())
	{
		mTeleportFailedConnection.disconnect();
	}

	if (mRegionCrossConnection.connected())
	{
		mRegionCrossConnection.disconnect();
	}

	LLFloater::onClose(pIsAppQuitting);
}

void LLFloaterDebugMaterials::draw()
{
	if (mNextUnparsedQueryDataIndex >= 0)
	{
		parseQueryViewableObjects();
	}
	if (LLSelectMgr::instance().getSelection().notNull())
	{
		refreshObjectEdit();
	}
	LLFloater::draw();
}

LLFloaterDebugMaterials::LLFloaterDebugMaterials(const LLSD& pParams)
	: LLFloater(pParams),
	mStatusText(NULL),
	mGetButton(NULL),
	mParsingStatusText(NULL),
	mGetNormalMapScrollList(NULL),
	mGetSpecularMapScrollList(NULL),
	mGetOtherDataScrollList(NULL),
	mNormalMap(NULL),
	mNormalMapOffsetX(NULL),
	mNormalMapOffsetY(NULL),
	mNormalMapRepeatX(NULL),
	mNormalMapRepeatY(NULL),
	mNormalMapRotation(NULL),
	mSpecularMap(NULL),
	mSpecularMapOffsetX(NULL),
	mSpecularMapOffsetY(NULL),
	mSpecularMapRepeatX(NULL),
	mSpecularMapRepeatY(NULL),
	mSpecularMapRotation(NULL),
	mSpecularColor(NULL),
	mSpecularColorAlpha(NULL),
	mSpecularExponent(NULL),
	mEnvironmentExponent(NULL),
	mAlphaMaskCutoff(NULL),
	mDiffuseAlphaMode(NULL),
	mPutSetButton(NULL),
	mPutClearButton(NULL),
	mPutScrollList(NULL),
	mQueryViewableObjectsButton(NULL),
	mQueryStatusText(NULL),
	mViewableObjectsScrollList(NULL),
	mPostButton(NULL),
	mPostNormalMapScrollList(NULL),
	mPostSpecularMapScrollList(NULL),
	mPostOtherDataScrollList(NULL),
	mState(kNoRegion),
	mWarningColor(),
	mErrorColor(),
	mRegionCrossConnection(),
	mTeleportFailedConnection(),
	mSelectionUpdateConnection(),
	mNextUnparsedQueryDataIndex(-1)
{
}

LLFloaterDebugMaterials::~LLFloaterDebugMaterials()
{
}

void LLFloaterDebugMaterials::onGetClicked()
{
	requestGetMaterials();
}

void LLFloaterDebugMaterials::onValueEntered(LLUICtrl* pUICtrl)
{
	LLLineEditor *pLineEditor = static_cast<LLLineEditor *>(pUICtrl);
	llassert(pLineEditor != NULL);

	const std::string &valueString = pLineEditor->getText();

	S32 intValue = 0;
	bool doResetValue = (!valueString.empty() && !LLStringUtil::convertToS32(valueString, intValue));

	if (doResetValue)
	{
		LL_WARNS("debugMaterials") << "cannot parse string '" << valueString << "' to an S32 value" <<LL_ENDL;
		LLSD value = static_cast<LLSD::Integer>(intValue);
		pLineEditor->setValue(value);
	}
}

void LLFloaterDebugMaterials::onPutSetClicked()
{
	requestPutMaterials(true);
}

void LLFloaterDebugMaterials::onPutClearClicked()
{
	requestPutMaterials(false);
}

void LLFloaterDebugMaterials::onResetPutValuesClicked()
{
	resetObjectEditInputs();
}

void LLFloaterDebugMaterials::onQueryVisibleObjectsClicked()
{
	clearViewableObjectsResults();
	setUnparsedQueryData();
}

void LLFloaterDebugMaterials::onPostClicked()
{
	clearPostResults();

	std::vector<LLScrollListItem*> selectedItems = mViewableObjectsScrollList->getAllSelected();
	if (!selectedItems.empty())
	{
		for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
			selectedItemIter != selectedItems.end(); ++selectedItemIter)
		{
			const LLScrollListItem* selectedItem = *selectedItemIter;
			const LLSD& selectedItemValue = selectedItem->getValue();

			llassert(selectedItemValue.has(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD));
			llassert(selectedItemValue.get(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD).isBinary());
			const LLMaterialID material_id(selectedItemValue.get(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD).asBinary());

			LLMaterialMgr::instance().get(material_id, boost::bind(&LLFloaterDebugMaterials::onPostMaterial, _1, _2));
		}
	}
}

void LLFloaterDebugMaterials::onRegionCross()
{
	checkRegionMaterialStatus();
	clearGetResults();
	clearViewableObjectsResults();
	clearPostResults();
}

void LLFloaterDebugMaterials::onInWorldSelectionChange()
{
	updateControls();
}

void LLFloaterDebugMaterials::onGetScrollListSelectionChange(LLUICtrl* pUICtrl)
{
	LLScrollListCtrl* scrollListCtrl = dynamic_cast<LLScrollListCtrl*>(pUICtrl);
	llassert(scrollListCtrl != NULL);

	if (scrollListCtrl != mGetNormalMapScrollList)
	{
		mGetNormalMapScrollList->deselectAllItems(TRUE);
	}
	if (scrollListCtrl != mGetSpecularMapScrollList)
	{
		mGetSpecularMapScrollList->deselectAllItems(TRUE);
	}
	if (scrollListCtrl != mGetOtherDataScrollList)
	{
		mGetOtherDataScrollList->deselectAllItems(TRUE);
	}

	std::vector<LLScrollListItem*> selectedItems = scrollListCtrl->getAllSelected();
	if (!selectedItems.empty())
	{
		llassert(selectedItems.size() == 1);
		LLScrollListItem* selectedItem = selectedItems.front();

		llassert(selectedItem != NULL);
		const LLSD& selectedIdValue = selectedItem->getValue();

		if (scrollListCtrl != mGetNormalMapScrollList)
		{
			mGetNormalMapScrollList->selectByValue(selectedIdValue);
			mGetNormalMapScrollList->scrollToShowSelected();
		}
		if (scrollListCtrl != mGetSpecularMapScrollList)
		{
			mGetSpecularMapScrollList->selectByValue(selectedIdValue);
			mGetSpecularMapScrollList->scrollToShowSelected();
		}
		if (scrollListCtrl != mGetOtherDataScrollList)
		{
			mGetOtherDataScrollList->selectByValue(selectedIdValue);
			mGetOtherDataScrollList->scrollToShowSelected();
		}
	}
}

void LLFloaterDebugMaterials::onPostScrollListSelectionChange(LLUICtrl* pUICtrl)
{
	LLScrollListCtrl* scrollListCtrl = dynamic_cast<LLScrollListCtrl*>(pUICtrl);
	llassert(scrollListCtrl != NULL);

	if (scrollListCtrl != mPostNormalMapScrollList)
	{
		mPostNormalMapScrollList->deselectAllItems(TRUE);
	}
	if (scrollListCtrl != mPostSpecularMapScrollList)
	{
		mPostSpecularMapScrollList->deselectAllItems(TRUE);
	}
	if (scrollListCtrl != mPostOtherDataScrollList)
	{
		mPostOtherDataScrollList->deselectAllItems(TRUE);
	}

	std::vector<LLScrollListItem*> selectedItems = scrollListCtrl->getAllSelected();
	if (!selectedItems.empty())
	{
		llassert(selectedItems.size() == 1);
		LLScrollListItem* selectedItem = selectedItems.front();

		llassert(selectedItem != NULL);
		const LLSD& selectedIdValue = selectedItem->getValue();

		if (scrollListCtrl != mPostNormalMapScrollList)
		{
			mPostNormalMapScrollList->selectByValue(selectedIdValue);
			mPostNormalMapScrollList->scrollToShowSelected();
		}
		if (scrollListCtrl != mPostSpecularMapScrollList)
		{
			mPostSpecularMapScrollList->selectByValue(selectedIdValue);
			mPostSpecularMapScrollList->scrollToShowSelected();
		}
		if (scrollListCtrl != mPostOtherDataScrollList)
		{
			mPostOtherDataScrollList->selectByValue(selectedIdValue);
			mPostOtherDataScrollList->scrollToShowSelected();
		}
	}
}

void LLFloaterDebugMaterials::onViewableObjectsScrollListSelectionChange()
{
	updateControls();
}

void LLFloaterDebugMaterials::onDeferredCheckRegionMaterialStatus(LLUUID regionId)
{
	checkRegionMaterialStatus(regionId);
}

void LLFloaterDebugMaterials::onDeferredRequestGetMaterials(LLUUID regionId)
{
	requestGetMaterials(regionId);
}

void LLFloaterDebugMaterials::onDeferredRequestPutMaterials(LLUUID regionId, bool pIsDoSet)
{
	requestPutMaterials(regionId, pIsDoSet);
}

void LLFloaterDebugMaterials::checkRegionMaterialStatus()
{
	LLViewerRegion *region = gAgent.getRegion();

	if (region == NULL)
	{
		LL_WARNS("debugMaterials") << "Region is NULL" << LL_ENDL;
		setState(kNoRegion);
	}
	else if (!region->capabilitiesReceived())
	{
		setState(kCapabilitiesLoading);
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterDebugMaterials::onDeferredCheckRegionMaterialStatus, this, region->getRegionID()));
	}
	else
	{
		std::string capURL = region->getCapability(MATERIALS_CAPABILITY_NAME);

		if (capURL.empty())
		{
			LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on the current region '" << region->getName() << "'" << LL_ENDL;
			setState(kNotEnabled);
		}
		else
		{
			setState(kReady);
		}
	}
}

void LLFloaterDebugMaterials::checkRegionMaterialStatus(const LLUUID& regionId)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		checkRegionMaterialStatus();
	}
}

void LLFloaterDebugMaterials::requestGetMaterials()
{
	LLViewerRegion *region = gAgent.getRegion();

	if (region == NULL)
	{
		LL_WARNS("debugMaterials") << "Region is NULL" << LL_ENDL;
		setState(kNoRegion);
	}
	else if (!region->capabilitiesReceived())
	{
		setState(kCapabilitiesLoading);
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterDebugMaterials::onDeferredRequestGetMaterials, this, region->getRegionID()));
	}
	else
	{
		std::string capURL = region->getCapability(MATERIALS_CAPABILITY_NAME);

		if (capURL.empty())
		{
			LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on the current region '" << region->getName() << "'" << LL_ENDL;
			setState(kNotEnabled);
		}
		else
		{
			setState(kReady);
			LLMaterialMgr::instance().getAll(region->getRegionID(), boost::bind(&LLFloaterDebugMaterials::onGetMaterials, _1, _2));
		}
	}
}

void LLFloaterDebugMaterials::requestGetMaterials(const LLUUID& regionId)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestGetMaterials();
	}
}

void LLFloaterDebugMaterials::requestPutMaterials(bool pIsDoSet)
{
	LLViewerRegion *region = gAgent.getRegion();

	if (region == NULL)
	{
		LL_WARNS("debugMaterials") << "Region is NULL" << LL_ENDL;
		setState(kNoRegion);
	}
	else if (!region->capabilitiesReceived())
	{
		setState(kCapabilitiesLoading);
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterDebugMaterials::onDeferredRequestPutMaterials, this, region->getRegionID(), pIsDoSet));
	}
	else
	{
		std::string capURL = region->getCapability(MATERIALS_CAPABILITY_NAME);

		if (capURL.empty())
		{
			LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on the current region '" << region->getName() << "'" << LL_ENDL;
			setState(kNotEnabled);
		}
		else
		{
			setState(kReady);

			LLMaterial material = (pIsDoSet) ? getMaterial() : LLMaterial::null;

			LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
			for (LLObjectSelection::valid_iterator objectIter = selectionHandle->valid_begin();
				objectIter != selectionHandle->valid_end(); ++objectIter)
			{
				LLSelectNode* objectNode = *objectIter;
				LLViewerObject* viewerObject = objectNode->getObject();

				if (viewerObject != NULL)
				{
					const LLViewerRegion* viewerRegion = viewerObject->getRegion();
					if (region != viewerRegion)
					{
						LL_ERRS("debugMaterials") << "cannot currently edit an object on a different region through the debug materials floater" << llendl;
					}
					S32 numTEs = llmin(static_cast<S32>(viewerObject->getNumTEs()), viewerObject->getNumFaces());
					for (S32 curTEIndex = 0; curTEIndex < numTEs; ++curTEIndex)
					{
						if (objectNode->isTESelected(curTEIndex))
						{
							LLMaterialMgr::instance().put(viewerObject->getID(), curTEIndex, material);
						}
					}
				}
			}
		}
	}
}

void LLFloaterDebugMaterials::requestPutMaterials(const LLUUID& regionId, bool pIsDoSet)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestPutMaterials(pIsDoSet);
	}
}

void LLFloaterDebugMaterials::parseQueryViewableObjects()
{
	llassert(mNextUnparsedQueryDataIndex >= 0);

	if (mNextUnparsedQueryDataIndex >= 0)
	{
		LLScrollListCell::Params cellParams;
		LLScrollListItem::Params rowParams;

		S32 numViewerObjects = gObjectList.getNumObjects();
		S32 viewerObjectIndex = mNextUnparsedQueryDataIndex;
		for (S32 currentParseCount = 0;
			(currentParseCount < 10) && (viewerObjectIndex < numViewerObjects);
			++currentParseCount, ++viewerObjectIndex)
		{
			const LLViewerObject *viewerObject = gObjectList.getObject(viewerObjectIndex);
			if ((viewerObject != NULL) && !viewerObject->isDead())
			{
				U8 objectNumTEs = viewerObject->getNumTEs();

				if (objectNumTEs > 0U)
				{
					const LLUUID& objectId = viewerObject->getID();
					U32 objectLocalId = viewerObject->getLocalID();
					const LLViewerRegion* objectRegion = viewerObject->getRegion();

					for (U8 curTEIndex = 0U; curTEIndex < objectNumTEs; ++curTEIndex)
					{
						const LLTextureEntry* objectTE = viewerObject->getTE(curTEIndex);
						llassert(objectTE != NULL);
						const LLMaterialID& objectMaterialID = objectTE->getMaterialID();
						if (!objectMaterialID.isNull())
						{
							cellParams.font = LLFontGL::getFontMonospace();

							cellParams.column = "object_id";
							cellParams.value = objectId.asString();
							rowParams.columns.add(cellParams);

							cellParams.font = LLFontGL::getFontSansSerif();

							cellParams.column = "region";
							cellParams.value = ((objectRegion == NULL) ? "<null>" : objectRegion->getName());
							rowParams.columns.add(cellParams);

							cellParams.column = "local_id";
							cellParams.value = llformat("%d", objectLocalId);
							rowParams.columns.add(cellParams);

							cellParams.column = "face_index";
							cellParams.value = llformat("%u", static_cast<unsigned int>(curTEIndex));
							rowParams.columns.add(cellParams);
							cellParams.font = LLFontGL::getFontMonospace();

							cellParams.column = "material_id";
							cellParams.value = objectMaterialID.asString();
							rowParams.columns.add(cellParams);

							LLSD rowValue = LLSD::emptyMap();
							rowValue[VIEWABLE_OBJECTS_REGION_ID_FIELD] = objectRegion->getRegionID();
							rowValue[VIEWABLE_OBJECTS_OBJECT_ID_FIELD] = objectId;
							rowValue[VIEWABLE_OBJECTS_MATERIAL_ID_FIELD] = objectMaterialID.asLLSD();

							rowParams.value = rowValue;

							mViewableObjectsScrollList->addRow(rowParams);
						}
					}
				}
			}
		}

		if (viewerObjectIndex < numViewerObjects)
		{
			mNextUnparsedQueryDataIndex = viewerObjectIndex;
			updateQueryParsingStatus();
		}
		else
		{
			clearUnparsedQueryData();
		}
	}
}

void LLFloaterDebugMaterials::onGetMaterials(const LLUUID& region_id, const LLMaterialMgr::material_map_t& materials)
{
	LLFloaterDebugMaterials* instancep = LLFloaterReg::findTypedInstance<LLFloaterDebugMaterials>("floater_debug_materials");
	if (!instancep)
	{
		return;
	}

	LLViewerRegion* regionp = gAgent.getRegion();
	if ( (!regionp) || (regionp->getRegionID() != region_id) )
	{
		return;
	}

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params normalMapRowParams;
	LLScrollListItem::Params specularMapRowParams;
	LLScrollListItem::Params otherDataRowParams;

	instancep->clearGetResults();
	for (LLMaterialMgr::material_map_t::const_iterator itMaterial = materials.begin(); itMaterial != materials.end(); ++itMaterial)
	{
		const LLMaterialID& material_id = itMaterial->first;
		const LLMaterialPtr material = itMaterial->second;

		F32 x, y;

		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "id";
		cellParams.value = material_id.asString();
		normalMapRowParams.columns.add(cellParams);
		specularMapRowParams.columns.add(cellParams);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_map";
		cellParams.value = material->getNormalID().asString();
		normalMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		material->getNormalOffset(x, y);
		cellParams.column = "normal_map_list_offset_x";
		cellParams.value = llformat("%f", x);
		normalMapRowParams.columns.add(cellParams);
		cellParams.column = "normal_map_list_offset_y";
		cellParams.value = llformat("%f", y);
		normalMapRowParams.columns.add(cellParams);

		material->getNormalRepeat(x, y);
		cellParams.column = "normal_map_list_repeat_x";
		cellParams.value = llformat("%f", x);
		normalMapRowParams.columns.add(cellParams);
		cellParams.column = "normal_map_list_repeat_y";
		cellParams.value = llformat("%f", y);
		normalMapRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_rotation";
		cellParams.value = llformat("%f", material->getNormalRotation());
		normalMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "specular_map_list_map";
		cellParams.value = material->getSpecularID().asString();
		specularMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		material->getSpecularOffset(x, y);
		cellParams.column = "specular_map_list_offset_x";
		cellParams.value = llformat("%f", x);
		specularMapRowParams.columns.add(cellParams);
		cellParams.column = "specular_map_list_offset_y";
		cellParams.value = llformat("%f", y);
		specularMapRowParams.columns.add(cellParams);

		material->getSpecularRepeat(x, y);
		cellParams.column = "specular_map_list_repeat_x";
		cellParams.value = llformat("%f", x);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_repeat_y";
		cellParams.value = llformat("%f", y);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_rotation";
		cellParams.value = llformat("%f", material->getSpecularRotation());
		specularMapRowParams.columns.add(cellParams);

		const LLColor4U& specularColor = material->getSpecularLightColor();
		cellParams.column = "specular_color";
		cellParams.value = llformat("(%d, %d, %d, %d)", specularColor.mV[0],
			specularColor.mV[1], specularColor.mV[2], specularColor.mV[3]);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "specular_exponent";
		cellParams.value = llformat("%d", material->getSpecularLightExponent());
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "env_intensity";
		cellParams.value = llformat("%d", material->getEnvironmentIntensity());
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "alpha_mask_cutoff";
		cellParams.value = llformat("%d", material->getAlphaMaskCutoff());
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "diffuse_alpha_mode";
		cellParams.value = llformat("%d", material->getDiffuseAlphaMode());
		otherDataRowParams.columns.add(cellParams);

		normalMapRowParams.value = cellParams.value;
		specularMapRowParams.value = cellParams.value;
		otherDataRowParams.value = cellParams.value;

		instancep->mGetNormalMapScrollList->addRow(normalMapRowParams);
		instancep->mGetSpecularMapScrollList->addRow(specularMapRowParams);
		instancep->mGetOtherDataScrollList->addRow(otherDataRowParams);
	}
}

void LLFloaterDebugMaterials::onPostMaterial(const LLMaterialID& material_id, const LLMaterialPtr materialp)
{
	LLFloaterDebugMaterials* instancep = LLFloaterReg::findTypedInstance<LLFloaterDebugMaterials>("floater_debug_materials");
	if ( (!instancep) || (!materialp.get()) )
	{
		return;
	}

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params normalMapRowParams;
	LLScrollListItem::Params specularMapRowParams;
	LLScrollListItem::Params otherDataRowParams;

	cellParams.font = LLFontGL::getFontMonospace();

	cellParams.column = "id";
	cellParams.value = material_id.asString();
	normalMapRowParams.columns.add(cellParams);
	specularMapRowParams.columns.add(cellParams);
	otherDataRowParams.columns.add(cellParams);

	cellParams.column = "normal_map_list_map";
	cellParams.value = materialp->getNormalID().asString();
	normalMapRowParams.columns.add(cellParams);

	cellParams.font = LLFontGL::getFontSansSerif();

	F32 x, y;
	materialp->getNormalOffset(x, y);
	cellParams.column = "normal_map_list_offset_x";
	cellParams.value = llformat("%f", x);
	normalMapRowParams.columns.add(cellParams);
	cellParams.column = "normal_map_list_offset_y";
	cellParams.value = llformat("%f", y);
	normalMapRowParams.columns.add(cellParams);

	materialp->getNormalRepeat(x, y);
	cellParams.column = "normal_map_list_repeat_x";
	cellParams.value = llformat("%f", x);
	normalMapRowParams.columns.add(cellParams);
	cellParams.column = "normal_map_list_repeat_y";
	cellParams.value = llformat("%f", y);
	normalMapRowParams.columns.add(cellParams);

	cellParams.column = "normal_map_list_rotation";
	cellParams.value = llformat("%f", materialp->getNormalRotation());
	normalMapRowParams.columns.add(cellParams);

	cellParams.font = LLFontGL::getFontMonospace();

	cellParams.column = "specular_map_list_map";
	cellParams.value = materialp->getSpecularID().asString();
	specularMapRowParams.columns.add(cellParams);

	cellParams.font = LLFontGL::getFontSansSerif();

	materialp->getSpecularOffset(x, y);
	cellParams.column = "specular_map_list_offset_x";
	cellParams.value = llformat("%f", x);
	specularMapRowParams.columns.add(cellParams);
	cellParams.column = "specular_map_list_offset_y";
	cellParams.value = llformat("%f", y);
	specularMapRowParams.columns.add(cellParams);

	materialp->getSpecularRepeat(x, y);
	cellParams.column = "specular_map_list_repeat_x";
	cellParams.value = llformat("%f", x);
	specularMapRowParams.columns.add(cellParams);
	cellParams.column = "specular_map_list_repeat_y";
	cellParams.value = llformat("%f", y);
	specularMapRowParams.columns.add(cellParams);

	cellParams.column = "specular_map_list_rotation";
	cellParams.value = llformat("%d", materialp->getSpecularRotation());
	specularMapRowParams.columns.add(cellParams);

	const LLColor4U& specularColor =materialp->getSpecularLightColor();
	cellParams.column = "specular_color";
	cellParams.value = llformat("(%d, %d, %d, %d)", specularColor.mV[0],
		specularColor.mV[1], specularColor.mV[2], specularColor.mV[3]);
	otherDataRowParams.columns.add(cellParams);

	cellParams.column = "specular_exponent";
	cellParams.value = llformat("%d", materialp->getSpecularLightExponent());
	otherDataRowParams.columns.add(cellParams);

	cellParams.column = "env_intensity";
	cellParams.value = llformat("%d", materialp->getEnvironmentIntensity());
	otherDataRowParams.columns.add(cellParams);

	cellParams.column = "alpha_mask_cutoff";
	cellParams.value = llformat("%d", materialp->getAlphaMaskCutoff());
	otherDataRowParams.columns.add(cellParams);

	cellParams.column = "diffuse_alpha_mode";
	cellParams.value = llformat("%d", materialp->getDiffuseAlphaMode());
	otherDataRowParams.columns.add(cellParams);

	normalMapRowParams.value = cellParams.value;
	specularMapRowParams.value = cellParams.value;
	otherDataRowParams.value = cellParams.value;

	instancep->mPostNormalMapScrollList->addRow(normalMapRowParams);
	instancep->mPostSpecularMapScrollList->addRow(specularMapRowParams);
	instancep->mPostOtherDataScrollList->addRow(otherDataRowParams);
}

void LLFloaterDebugMaterials::setState(EState pState)
{
	mState = pState;
	updateStatusMessage();
	updateControls();
}

void LLFloaterDebugMaterials::refreshObjectEdit()
{
	mPutScrollList->deleteAllItems();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params rowParams;

	LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
	for (LLObjectSelection::valid_iterator objectIter = selectionHandle->valid_begin();
			objectIter != selectionHandle->valid_end(); ++objectIter)
	{
		LLSelectNode* nodep = *objectIter;

		LLViewerObject* objectp = nodep->getObject();
		if (objectp != NULL)
		{
			S32 numTEs = llmin(static_cast<S32>(objectp->getNumTEs()), objectp->getNumFaces());
			for (S32 curTEIndex = 0; curTEIndex < numTEs; ++curTEIndex)
			{
				if (nodep->isTESelected(curTEIndex))
				{
					const LLTextureEntry* tep = objectp->getTE(curTEIndex);

					cellParams.font = LLFontGL::getFontMonospace();

					cellParams.column = "material_id";
					cellParams.value = tep->getMaterialID().asString();
					rowParams.columns.add(cellParams);

					cellParams.font = LLFontGL::getFontSansSerif();

					cellParams.column = "object_id";
					cellParams.value = objectp->getID().asString();
					rowParams.columns.add(cellParams);

					cellParams.column = "face_index";
					cellParams.value = llformat("%d", curTEIndex);
					rowParams.columns.add(cellParams);

					mPutScrollList->addRow(rowParams);
				}
			}
		}
	}
}

void LLFloaterDebugMaterials::resetObjectEditInputs()
{
	const LLSD zeroValue = static_cast<LLSD::Integer>(0);
	const LLSD maxAlphaValue = static_cast<LLSD::Integer>(255);

	mNormalMap->clear();
	mNormalMapOffsetX->setValue(zeroValue);
	mNormalMapOffsetY->setValue(zeroValue);
	mNormalMapRepeatX->setValue(zeroValue);
	mNormalMapRepeatY->setValue(zeroValue);
	mNormalMapRotation->setValue(zeroValue);

	mSpecularMap->clear();
	mSpecularMapOffsetX->setValue(zeroValue);
	mSpecularMapOffsetY->setValue(zeroValue);
	mSpecularMapRepeatX->setValue(zeroValue);
	mSpecularMapRepeatY->setValue(zeroValue);
	mSpecularMapRotation->setValue(zeroValue);

	mSpecularColor->set(mDefaultSpecularColor);
	mSpecularColorAlpha->setValue(maxAlphaValue);
	mSpecularExponent->setValue(zeroValue);
	mEnvironmentExponent->setValue(zeroValue);
	mAlphaMaskCutoff->setValue(zeroValue);
	mDiffuseAlphaMode->setValue(zeroValue);
}

void LLFloaterDebugMaterials::clearGetResults()
{
	mGetNormalMapScrollList->deleteAllItems();
	mGetSpecularMapScrollList->deleteAllItems();
	mGetOtherDataScrollList->deleteAllItems();
}

void LLFloaterDebugMaterials::clearPostResults()
{
	mPostNormalMapScrollList->deleteAllItems();
	mPostSpecularMapScrollList->deleteAllItems();
	mPostOtherDataScrollList->deleteAllItems();
}

void LLFloaterDebugMaterials::clearViewableObjectsResults()
{
	mViewableObjectsScrollList->deleteAllItems();
	clearUnparsedQueryData();
}

void LLFloaterDebugMaterials::setUnparsedQueryData()
{
	mNextUnparsedQueryDataIndex = 0;

	updateQueryParsingStatus();
}

void LLFloaterDebugMaterials::clearUnparsedQueryData()
{
	mNextUnparsedQueryDataIndex = -1;

	updateQueryParsingStatus();
}

void LLFloaterDebugMaterials::updateQueryParsingStatus()
{
	std::string queryStatus;

	if (mNextUnparsedQueryDataIndex >= 0)
	{
		LLLocale locale(LLStringUtil::getLocale());
		std::string numProcessedString;
		LLResMgr::getInstance()->getIntegerString(numProcessedString, mNextUnparsedQueryDataIndex);

		std::string numTotalString;
		LLResMgr::getInstance()->getIntegerString(numTotalString, gObjectList.getNumObjects());

		LLStringUtil::format_map_t stringArgs;
		stringArgs["[NUM_PROCESSED]"] = numProcessedString;
		stringArgs["[NUM_TOTAL]"] = numTotalString;

		queryStatus = getString("querying_status_in_progress", stringArgs);
	}
	else
	{
		queryStatus = getString("querying_status_done");
	}

	mQueryStatusText->setText(static_cast<const LLStringExplicit>(queryStatus));
}

void LLFloaterDebugMaterials::updateStatusMessage()
{
	std::string statusText;
	LLStyle::Params styleParams;

	switch (getState())
	{
	case kNoRegion :
		statusText = getString("status_no_region");
		styleParams.color = mErrorColor;
		break;
	case kCapabilitiesLoading :
		statusText = getString("status_capabilities_loading");
		styleParams.color = mWarningColor;
		break;
	case kReady :
		statusText = getString("status_ready");
		break;
	case kRequestStarted :
		statusText = getString("status_request_started");
		styleParams.color = mWarningColor;
		break;
	case kRequestCompleted :
		statusText = getString("status_request_completed");
		break;
	case kNotEnabled :
		statusText = getString("status_not_enabled");
		styleParams.color = mErrorColor;
		break;
	case kError :
		statusText = getString("status_error");
		styleParams.color = mErrorColor;
		break;
	default :
		statusText = getString("status_ready");
		llassert(0);
		break;
	}

	mStatusText->setText((LLStringExplicit)statusText, styleParams);
}

void LLFloaterDebugMaterials::updateControls()
{
	LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
	bool isPutEnabled = (selectionHandle->valid_begin() != selectionHandle->valid_end());
	bool isPostEnabled = (mViewableObjectsScrollList->getNumSelected() > 0);

	switch (getState())
	{
	case kNoRegion :
	case kCapabilitiesLoading :
	case kRequestStarted :
	case kNotEnabled :
		mGetButton->setEnabled(FALSE);
		mPutSetButton->setEnabled(FALSE);
		mPutClearButton->setEnabled(FALSE);
		mPostButton->setEnabled(FALSE);
		break;
	case kReady :
	case kRequestCompleted :
	case kError :
		mGetButton->setEnabled(TRUE);
		mPutSetButton->setEnabled(isPutEnabled);
		mPutClearButton->setEnabled(isPutEnabled);
		mPostButton->setEnabled(isPostEnabled);
		break;
	default :
		mGetButton->setEnabled(TRUE);
		mPutSetButton->setEnabled(isPutEnabled);
		mPutClearButton->setEnabled(isPutEnabled);
		mPostButton->setEnabled(isPostEnabled);
		llassert(0);
		break;
	}
}

template<typename T> T getLineEditorValue(const LLLineEditor *pLineEditor);

template<> U8 getLineEditorValue(const LLLineEditor *pLineEditor)
{
	U8 value = 0;

	LLStringUtil::convertToU8(pLineEditor->getText(), value);

	return value;
}

LLMaterial LLFloaterDebugMaterials::getMaterial() const
{
	LLMaterial material;

	material.setNormalID(mNormalMap->getImageAssetID());
	material.setNormalOffset(mNormalMapOffsetX->get(), mNormalMapOffsetY->get());
	material.setNormalRepeat(mNormalMapRepeatX->get(), mNormalMapRepeatY->get());
	material.setNormalRotation(mNormalMapRotation->get());

	material.setSpecularID(mSpecularMap->getImageAssetID());
	material.setSpecularOffset(mSpecularMapOffsetX->get(), mSpecularMapOffsetY->get());
	material.setSpecularRepeat(mSpecularMapRepeatX->get(), mSpecularMapRepeatY->get());
	material.setSpecularRotation(mSpecularMapRotation->get());

	const LLColor4& specularColor = mSpecularColor->get();
	LLColor4U specularColor4U = specularColor;
	specularColor4U.setAlpha(static_cast<U8>(llclamp(llround(mSpecularColorAlpha->get()), 0, 255)));
	material.setSpecularLightColor(specularColor4U);

	material.setSpecularLightExponent(getLineEditorValue<U8>(mSpecularExponent));
	material.setEnvironmentIntensity(getLineEditorValue<U8>(mEnvironmentExponent));
	material.setDiffuseAlphaMode(getLineEditorValue<U8>(mDiffuseAlphaMode));
	material.setAlphaMaskCutoff(getLineEditorValue<U8>(mAlphaMaskCutoff));

	return material;
}
