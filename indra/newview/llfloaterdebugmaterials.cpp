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

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llcolorswatch.h"
#include "llenvmanager.h"
#include "llfloater.h"
#include "llfontgl.h"
#include "llhttpclient.h"
#include "lllineeditor.h"
#include "llmaterialid.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsd.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "llstring.h"
#include "llstyle.h"
#include "lltextbase.h"
#include "lltexturectrl.h"
#include "lltextvalidate.h"
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

#define MATERIALS_CAP_FULL_PER_FACE_FIELD         "FullMaterialsPerFace"
#define MATERIALS_CAP_FACE_FIELD                  "Face"
#define MATERIALS_CAP_MATERIAL_FIELD              "Material"
#define MATERIALS_CAP_OBJECT_ID_FIELD             "ID"
#define MATERIALS_CAP_MATERIAL_ID_FIELD           "MaterialID"

#define MATERIALS_CAP_NORMAL_MAP_FIELD            "NormMap"
#define MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD   "NormOffsetX"
#define MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD   "NormOffsetY"
#define MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD   "NormRepeatX"
#define MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD   "NormRepeatY"
#define MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD   "NormRotation"

#define MATERIALS_CAP_SPECULAR_MAP_FIELD          "SpecMap"
#define MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD "SpecOffsetX"
#define MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD "SpecOffsetY"
#define MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD "SpecRepeatX"
#define MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD "SpecRepeatY"
#define MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD "SpecRotation"

#define MATERIALS_CAP_SPECULAR_COLOR_FIELD        "SpecColor"
#define MATERIALS_CAP_SPECULAR_EXP_FIELD          "SpecExp"
#define MATERIALS_CAP_ENV_INTENSITY_FIELD         "EnvIntensity"
#define MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD     "AlphaMaskCutoff"
#define MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD    "DiffuseAlphaMode"

class MaterialsResponder : public LLHTTPClient::Responder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	MaterialsResponder(const std::string& pMethod, const std::string& pCapabilityURL, CallbackFunction pCallback);
	virtual ~MaterialsResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

protected:

private:
	std::string      mMethod;
	std::string      mCapabilityURL;
	CallbackFunction mCallback;
};

BOOL LLFloaterDebugMaterials::postBuild()
{
	mStatusText = findChild<LLTextBase>("material_status");
	llassert(mStatusText != NULL);

	mGetButton = findChild<LLButton>("get_button");
	llassert(mGetButton != NULL);
	mGetButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGetClicked, this));

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

	mNormalMapOffsetX = findChild<LLLineEditor>("normal_map_offset_x");
	llassert(mNormalMapOffsetX != NULL);
	mNormalMapOffsetX->setPrevalidate(LLTextValidate::validateInt);
	mNormalMapOffsetX->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mNormalMapOffsetY = findChild<LLLineEditor>("normal_map_offset_y");
	llassert(mNormalMapOffsetY != NULL);
	mNormalMapOffsetY->setPrevalidate(LLTextValidate::validateInt);
	mNormalMapOffsetY->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mNormalMapRepeatX = findChild<LLLineEditor>("normal_map_repeat_x");
	llassert(mNormalMapRepeatX != NULL);
	mNormalMapRepeatX->setPrevalidate(LLTextValidate::validateInt);
	mNormalMapRepeatX->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mNormalMapRepeatY = findChild<LLLineEditor>("normal_map_repeat_y");
	llassert(mNormalMapRepeatY != NULL);
	mNormalMapRepeatY->setPrevalidate(LLTextValidate::validateInt);
	mNormalMapRepeatY->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mNormalMapRotation = findChild<LLLineEditor>("normal_map_rotation");
	llassert(mNormalMapRotation != NULL);
	mNormalMapRotation->setPrevalidate(LLTextValidate::validateInt);
	mNormalMapRotation->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mSpecularMap = findChild<LLTextureCtrl>("specular_map");
	llassert(mSpecularMap != NULL);

	mSpecularMapOffsetX = findChild<LLLineEditor>("specular_map_offset_x");
	llassert(mSpecularMapOffsetX != NULL);
	mSpecularMapOffsetX->setPrevalidate(LLTextValidate::validateInt);
	mSpecularMapOffsetX->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mSpecularMapOffsetY = findChild<LLLineEditor>("specular_map_offset_y");
	llassert(mSpecularMapOffsetY != NULL);
	mSpecularMapOffsetY->setPrevalidate(LLTextValidate::validateInt);
	mSpecularMapOffsetY->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mSpecularMapRepeatX = findChild<LLLineEditor>("specular_map_repeat_x");
	llassert(mSpecularMapRepeatX != NULL);
	mSpecularMapRepeatX->setPrevalidate(LLTextValidate::validateInt);
	mSpecularMapRepeatX->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mSpecularMapRepeatY = findChild<LLLineEditor>("specular_map_repeat_y");
	llassert(mSpecularMapRepeatY != NULL);
	mSpecularMapRepeatY->setPrevalidate(LLTextValidate::validateInt);
	mSpecularMapRepeatY->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

	mSpecularMapRotation = findChild<LLLineEditor>("specular_map_rotation");
	llassert(mSpecularMapRotation != NULL);
	mSpecularMapRotation->setPrevalidate(LLTextValidate::validateInt);
	mSpecularMapRotation->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onValueEntered, this, _1));

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

	mViewableObjectsScrollList = findChild<LLScrollListCtrl>("viewable_objects_scroll_list");
	llassert(mViewableObjectsScrollList != NULL);

	mGoodPostButton = findChild<LLButton>("good_post_button");
	llassert(mGoodPostButton != NULL);
	mGoodPostButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onGoodPostClicked, this));

	mBadPostButton = findChild<LLButton>("bad_post_button");
	llassert(mBadPostButton != NULL);
	mBadPostButton->setCommitCallback(boost::bind(&LLFloaterDebugMaterials::onBadPostClicked, this));

	mPostNormalMapScrollList = findChild<LLScrollListCtrl>("post_normal_map_scroll_list");
	llassert(mPostNormalMapScrollList != NULL);

	mPostSpecularMapScrollList = findChild<LLScrollListCtrl>("post_specular_map_scroll_list");
	llassert(mPostSpecularMapScrollList != NULL);

	mPostOtherDataScrollList = findChild<LLScrollListCtrl>("post_other_data_scroll_list");
	llassert(mPostOtherDataScrollList != NULL);

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
	clearPutResults();
	clearViewableObjectsResults();
	clearPostResults();
}

void LLFloaterDebugMaterials::onClose(bool pIsAppQuitting)
{
	resetObjectEditInputs();
	clearGetResults();
	clearPutResults();
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

LLFloaterDebugMaterials::LLFloaterDebugMaterials(const LLSD& pParams)
	: LLFloater(pParams),
	mStatusText(NULL),
	mGetButton(NULL),
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
	mViewableObjectsScrollList(NULL),
	mGoodPostButton(NULL),
	mBadPostButton(NULL),
	mPostNormalMapScrollList(NULL),
	mPostSpecularMapScrollList(NULL),
	mPostOtherDataScrollList(NULL),
	mState(kNoRegion),
	mWarningColor(),
	mErrorColor(),
	mRegionCrossConnection(),
	mTeleportFailedConnection(),
	mSelectionUpdateConnection()
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
		llwarns << "cannot parse string '" << valueString << "' to an S32 value" <<llendl;
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
	queryViewableObjects();
}

void LLFloaterDebugMaterials::onGoodPostClicked()
{
	requestPostMaterials(true);
}

void LLFloaterDebugMaterials::onBadPostClicked()
{
	requestPostMaterials(false);
}

void LLFloaterDebugMaterials::onRegionCross()
{
	checkRegionMaterialStatus();
	clearGetResults();
	clearPutResults();
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

		llinfos << "attempting to select by value '" << selectedIdValue << "'" << llendl;

		if (scrollListCtrl != mGetNormalMapScrollList)
		{
			mGetNormalMapScrollList->selectByValue(selectedIdValue);
		}
		if (scrollListCtrl != mGetSpecularMapScrollList)
		{
			mGetSpecularMapScrollList->selectByValue(selectedIdValue);
		}
		if (scrollListCtrl != mGetOtherDataScrollList)
		{
			mGetOtherDataScrollList->selectByValue(selectedIdValue);
		}
	}
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

void LLFloaterDebugMaterials::onDeferredRequestPostMaterials(LLUUID regionId, bool pUseGoodData)
{
	requestPostMaterials(regionId, pUseGoodData);
}

void LLFloaterDebugMaterials::onGetResponse(bool pRequestStatus, const LLSD& pContent)
{
	if (pRequestStatus)
	{
		setState(kRequestCompleted);
		parseGetResponse(pContent);
	}
	else
	{
		setState(kError);
	}
}

void LLFloaterDebugMaterials::onPutResponse(bool pRequestStatus, const LLSD& pContent)
{
	if (pRequestStatus)
	{
		setState(kRequestCompleted);
		parsePutResponse(pContent);
	}
	else
	{
		setState(kError);
	}
}

void LLFloaterDebugMaterials::onPostResponse(bool pRequestStatus, const LLSD& pContent)
{
	if (pRequestStatus)
	{
		setState(kRequestCompleted);
		parsePostResponse(pContent);
	}
	else
	{
		setState(kError);
	}
}

void LLFloaterDebugMaterials::checkRegionMaterialStatus()
{
	LLViewerRegion *region = gAgent.getRegion();

	if (region == NULL)
	{
		llwarns << "Region is NULL" << llendl;
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
			llwarns << "Capability '" << MATERIALS_CAPABILITY_NAME << "' is not defined on the current region '"
				<< region->getName() << "'" << llendl;
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
		llwarns << "Region is NULL" << llendl;
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
			llwarns << "Capability '" << MATERIALS_CAPABILITY_NAME << "' is not defined on the current region '"
				<< region->getName() << "'" << llendl;
			setState(kNotEnabled);
		}
		else
		{
			setState(kRequestStarted);
			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("GET", capURL, boost::bind(&LLFloaterDebugMaterials::onGetResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request GET to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "'" << llendl;
			LLHTTPClient::get(capURL, materialsResponder);
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
		llwarns << "Region is NULL" << llendl;
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
			llwarns << "Capability '" << MATERIALS_CAPABILITY_NAME << "' is not defined on the current region '"
				<< region->getName() << "'" << llendl;
			setState(kNotEnabled);
		}
		else
		{
			setState(kRequestStarted);

			LLSD facesData  = LLSD::emptyArray();

			LLSD materialData = LLSD::emptyMap();
			if (pIsDoSet)
			{
				materialData[MATERIALS_CAP_NORMAL_MAP_FIELD] = mNormalMap->getImageAssetID();
				materialData[MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD] = static_cast<LLSD::Integer>(getNormalMapOffsetX());
				materialData[MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD] = static_cast<LLSD::Integer>(getNormalMapOffsetY());
				materialData[MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD] = static_cast<LLSD::Integer>(getNormalMapRepeatX());
				materialData[MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD] = static_cast<LLSD::Integer>(getNormalMapRepeatY());
				materialData[MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD] = static_cast<LLSD::Integer>(getNormalMapRotation());

				materialData[MATERIALS_CAP_SPECULAR_MAP_FIELD] = mSpecularMap->getImageAssetID();
				materialData[MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD] = static_cast<LLSD::Integer>(getSpecularMapOffsetX());
				materialData[MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD] = static_cast<LLSD::Integer>(getSpecularMapOffsetY());
				materialData[MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD] = static_cast<LLSD::Integer>(getSpecularMapRepeatX());
				materialData[MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD] = static_cast<LLSD::Integer>(getSpecularMapRepeatY());
				materialData[MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD] = static_cast<LLSD::Integer>(getSpecularMapRotation());

				LLColor4U specularColor = getSpecularColor();
				materialData[MATERIALS_CAP_SPECULAR_COLOR_FIELD] = specularColor.getValue();
				materialData[MATERIALS_CAP_SPECULAR_EXP_FIELD] = static_cast<LLSD::Integer>(getSpecularExponent());
				materialData[MATERIALS_CAP_ENV_INTENSITY_FIELD] = static_cast<LLSD::Integer>(getEnvironmentExponent());
				materialData[MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD] = static_cast<LLSD::Integer>(getAlphMaskCutoff());
				materialData[MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD] = static_cast<LLSD::Integer>(getDiffuseAlphaMode());
			}

			LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
			for (LLObjectSelection::valid_iterator objectIter = selectionHandle->valid_begin();
				objectIter != selectionHandle->valid_end(); ++objectIter)
			{
				LLSelectNode* objectNode = *objectIter;
				LLViewerObject* viewerObject = objectNode->getObject();

				if (viewerObject != NULL)
				{
					S32 numTEs = llmin(static_cast<S32>(viewerObject->getNumTEs()), viewerObject->getNumFaces());
					for (S32 curTEIndex = 0; curTEIndex < numTEs; ++curTEIndex)
					{
						if (objectNode->isTESelected(curTEIndex))
						{
							LLSD faceData = LLSD::emptyMap();
							faceData[MATERIALS_CAP_FACE_FIELD] = static_cast<LLSD::Integer>(curTEIndex);
							faceData[MATERIALS_CAP_OBJECT_ID_FIELD] = static_cast<LLSD::Integer>(viewerObject->getLocalID());
							if (pIsDoSet)
							{
								faceData[MATERIALS_CAP_MATERIAL_FIELD] = materialData;
							}
							facesData.append(faceData);
						}
					}
				}
			}

			LLSD putData = LLSD::emptyMap();
			putData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = facesData;

			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("PUT", capURL, boost::bind(&LLFloaterDebugMaterials::onPutResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request PUT to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "' and with data " << putData << llendl;
			LLHTTPClient::put(capURL, putData, materialsResponder);
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

void LLFloaterDebugMaterials::requestPostMaterials(bool pUseGoodData)
{
#if 0
	LLViewerRegion *region = gAgent.getRegion();

	if (region == NULL)
	{
		llwarns << "Region is NULL" << llendl;
		setState(kNoRegion);
	}
	else if (!region->capabilitiesReceived())
	{
		setState(kCapabilitiesLoading);
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterDebugMaterials::onDeferredRequestPostMaterials, this, region->getRegionID(), pUseGoodData));
	}
	else
	{
		std::string capURL = region->getCapability(MATERIALS_CAPABILITY_NAME);

		if (capURL.empty())
		{
			llwarns << "Capability '" << MATERIALS_CAPABILITY_NAME << "' is not defined on the current region '"
				<< region->getName() << "'" << llendl;
			setState(kNotEnabled);
		}
		else
		{
			setState(kRequestStarted);
			LLSD postData = LLSD::emptyArray();

			if (pUseGoodData)
			{
				std::vector<LLScrollListItem*> selectedItems = mGetScrollList->getAllSelected();
				for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
					selectedItemIter != selectedItems.end(); ++selectedItemIter)
				{
					const LLScrollListItem* selectedItem = *selectedItemIter;
					postData.append(selectedItem->getValue());
				}
			}
			else
			{
				S32 crapArray[4];
				for (int i = 0; i < 4; ++i)
				{
					crapArray[i] = ll_rand();
					if (ll_frand() < 0.5)
					{
						crapArray[i] = -crapArray[i];
					}
				}

				std::vector<unsigned char> crapMem;
				crapMem.resize(16);
				memcpy(&crapMem[0], &crapArray, 16 * sizeof(unsigned char));

				LLSD::Binary crapBinary = crapMem;
				LLSD crapData = crapBinary;
				postData.append(crapData);
			}

			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("POST", capURL, boost::bind(&LLFloaterDebugMaterials::onPostResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request POST to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "' and with data " << postData << llendl;
			LLHTTPClient::post(capURL, postData, materialsResponder);
		}
	}
#endif
}

void LLFloaterDebugMaterials::requestPostMaterials(const LLUUID& regionId, bool pUseGoodData)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestPostMaterials(pUseGoodData);
	}
}

void LLFloaterDebugMaterials::queryViewableObjects()
{
	clearViewableObjectsResults();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params rowParams;

	S32 numViewerObjects = gObjectList.getNumObjects();
	for (S32 viewerObjectIndex = 0; viewerObjectIndex < numViewerObjects; ++viewerObjectIndex)
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

						std::string materialIDString = convertToPrintableMaterialID(objectMaterialID);
						cellParams.column = "material_id";
						cellParams.value = materialIDString;
						rowParams.columns.add(cellParams);

						rowParams.value = objectId;

						mViewableObjectsScrollList->addRow(rowParams);
					}
				}
			}

		}
	}
}

void LLFloaterDebugMaterials::parseGetResponse(const LLSD& pContent)
{
	printResponse("GET", pContent);
	clearGetResults();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params normalMapRowParams;
	LLScrollListItem::Params specularMapRowParams;
	LLScrollListItem::Params otherDataRowParams;

	llassert(pContent.isArray());
	for (LLSD::array_const_iterator materialIter = pContent.beginArray(); materialIter != pContent.endArray();
		++materialIter)
	{
		const LLSD &material = *materialIter;
		llassert(material.isMap());
		llassert(material.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(material.get(MATERIALS_CAP_OBJECT_ID_FIELD).isBinary());
		const LLSD &materialID = material.get(MATERIALS_CAP_OBJECT_ID_FIELD);
		std::string materialIDString = convertToPrintableMaterialID(materialID);

		llassert(material.has(MATERIALS_CAP_MATERIAL_FIELD));
		const LLSD &materialData = material.get(MATERIALS_CAP_MATERIAL_FIELD);
		llassert(materialData.isMap());

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_FIELD).isUUID());
		const LLUUID &normalMapID = materialData.get(MATERIALS_CAP_NORMAL_MAP_FIELD).asUUID();

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD).isInteger());
		S32 normalMapOffsetX = materialData.get(MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD).isInteger());
		S32 normalMapOffsetY = materialData.get(MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD).isInteger());
		S32 normalMapRepeatX = materialData.get(MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD).isInteger());
		S32 normalMapRepeatY = materialData.get(MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD).isInteger());
		S32 normalMapRotation = materialData.get(MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_FIELD).isUUID());
		const LLUUID &specularMapID = materialData.get(MATERIALS_CAP_SPECULAR_MAP_FIELD).asUUID();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD).isInteger());
		S32 specularMapOffsetX = materialData.get(MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD).isInteger());
		S32 specularMapOffsetY = materialData.get(MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD).isInteger());
		S32 specularMapRepeatX = materialData.get(MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD).isInteger());
		S32 specularMapRepeatY = materialData.get(MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD).isInteger());
		S32 specularMapRotation = materialData.get(MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_COLOR_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_COLOR_FIELD).isArray());
		LLColor4U specularColor;
		specularColor.setValue(materialData.get(MATERIALS_CAP_SPECULAR_COLOR_FIELD));

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_EXP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_EXP_FIELD).isInteger());
		S32 specularExp = materialData.get(MATERIALS_CAP_SPECULAR_EXP_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_ENV_INTENSITY_FIELD));
		llassert(materialData.get(MATERIALS_CAP_ENV_INTENSITY_FIELD).isInteger());
		S32 envIntensity = materialData.get(MATERIALS_CAP_ENV_INTENSITY_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD));
		llassert(materialData.get(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD).isInteger());
		S32 alphaMaskCutoff = materialData.get(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD));
		llassert(materialData.get(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD).isInteger());
		S32 diffuseAlphaMode = materialData.get(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD).asInteger();

		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "id";
		cellParams.value = materialIDString;
		normalMapRowParams.columns.add(cellParams);
		specularMapRowParams.columns.add(cellParams);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_map";
		cellParams.value = normalMapID.asString();
		normalMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		cellParams.column = "normal_map_list_offset_x";
		cellParams.value = llformat("%d", normalMapOffsetX);
		normalMapRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_offset_y";
		cellParams.value = llformat("%d", normalMapOffsetY);
		normalMapRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_repeat_x";
		cellParams.value = llformat("%d", normalMapRepeatX);
		normalMapRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_repeat_y";
		cellParams.value = llformat("%d", normalMapRepeatY);
		normalMapRowParams.columns.add(cellParams);

		cellParams.column = "normal_map_list_rotation";
		cellParams.value = llformat("%d", normalMapRotation);
		normalMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "specular_map_list_map";
		cellParams.value = specularMapID.asString();
		specularMapRowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		cellParams.column = "specular_map_list_offset_x";
		cellParams.value = llformat("%d", specularMapOffsetX);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_offset_y";
		cellParams.value = llformat("%d", specularMapOffsetY);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_repeat_x";
		cellParams.value = llformat("%d", specularMapRepeatX);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_repeat_y";
		cellParams.value = llformat("%d", specularMapRepeatY);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_map_list_rotation";
		cellParams.value = llformat("%d", specularMapRotation);
		specularMapRowParams.columns.add(cellParams);

		cellParams.column = "specular_color";
		cellParams.value = llformat("(%d, %d, %d, %d)", specularColor.mV[0],
			specularColor.mV[1], specularColor.mV[2], specularColor.mV[3]);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "specular_exponent";
		cellParams.value = llformat("%d", specularExp);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "env_intensity";
		cellParams.value = llformat("%d", envIntensity);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "alpha_mask_cutoff";
		cellParams.value = llformat("%d", alphaMaskCutoff);
		otherDataRowParams.columns.add(cellParams);

		cellParams.column = "diffuse_alpha_mode";
		cellParams.value = llformat("%d", diffuseAlphaMode);
		otherDataRowParams.columns.add(cellParams);

		normalMapRowParams.value = materialIDString;
		specularMapRowParams.value = materialIDString;
		otherDataRowParams.value = materialIDString;

		mGetNormalMapScrollList->addRow(normalMapRowParams);
		mGetSpecularMapScrollList->addRow(specularMapRowParams);
		mGetOtherDataScrollList->addRow(otherDataRowParams);
	}
}

void LLFloaterDebugMaterials::parsePutResponse(const LLSD& pContent)
{
	printResponse("PUT", pContent);
	clearPutResults();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params rowParams;

	llassert(pContent.isArray());
	for (LLSD::array_const_iterator faceIter = pContent.beginArray(); faceIter != pContent.endArray();
		++faceIter)
	{
		const LLSD &face = *faceIter;
		llassert(face.isMap());

		llassert(face.has(MATERIALS_CAP_FACE_FIELD));
		llassert(face.get(MATERIALS_CAP_FACE_FIELD).isInteger());
		S32 faceId = face.get(MATERIALS_CAP_FACE_FIELD).asInteger();

		llassert(face.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(face.get(MATERIALS_CAP_OBJECT_ID_FIELD).isInteger());
		S32 objectId = face.get(MATERIALS_CAP_OBJECT_ID_FIELD).asInteger();

		llassert(face.has(MATERIALS_CAP_MATERIAL_ID_FIELD));
		llassert(face.get(MATERIALS_CAP_MATERIAL_ID_FIELD).isBinary());
		std::string materialIDString = convertToPrintableMaterialID(face.get(MATERIALS_CAP_MATERIAL_ID_FIELD));

		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "material_id";
		cellParams.value = materialIDString;
		rowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		cellParams.column = "object_id";
		cellParams.value = llformat("%d", objectId);
		rowParams.columns.add(cellParams);

		cellParams.column = "face_index";
		cellParams.value = llformat("%d", faceId);
		rowParams.columns.add(cellParams);

		mPutScrollList->addRow(rowParams);
	}
}

void LLFloaterDebugMaterials::parsePostResponse(const LLSD& pContent)
{
	printResponse("POST", pContent);
#if 0
	clearPostResults();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params rowParams;

	llassert(pContent.isArray());
	for (LLSD::array_const_iterator materialIter = pContent.beginArray(); materialIter != pContent.endArray();
		++materialIter)
	{
		const LLSD &material = *materialIter;
		llassert(material.isMap());
		llassert(material.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(material.get(MATERIALS_CAP_OBJECT_ID_FIELD).isBinary());
		const LLSD &materialID = material.get(MATERIALS_CAP_OBJECT_ID_FIELD);
		std::string materialIDString = convertToPrintableMaterialID(materialID);

		llassert(material.has(MATERIALS_CAP_MATERIAL_FIELD));
		const LLSD &materialData = material.get(MATERIALS_CAP_MATERIAL_FIELD);
		llassert(materialData.isMap());

		llassert(materialData.has(MATERIALS_CAP_NORMAL_MAP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_NORMAL_MAP_FIELD).isUUID());
		const LLUUID &normalMapID = materialData.get(MATERIALS_CAP_NORMAL_MAP_FIELD).asUUID();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_MAP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_MAP_FIELD).isUUID());
		const LLUUID &specularMapID = materialData.get(MATERIALS_CAP_SPECULAR_MAP_FIELD).asUUID();

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_COLOR_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_COLOR_FIELD).isArray());
		LLColor4U specularColor;
		specularColor.setValue(materialData.get(MATERIALS_CAP_SPECULAR_COLOR_FIELD));

		llassert(materialData.has(MATERIALS_CAP_SPECULAR_EXP_FIELD));
		llassert(materialData.get(MATERIALS_CAP_SPECULAR_EXP_FIELD).isInteger());
		S32 specularExp = materialData.get(MATERIALS_CAP_SPECULAR_EXP_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_ENV_INTENSITY_FIELD));
		llassert(materialData.get(MATERIALS_CAP_ENV_INTENSITY_FIELD).isInteger());
		S32 envIntensity = materialData.get(MATERIALS_CAP_ENV_INTENSITY_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD));
		llassert(materialData.get(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD).isInteger());
		S32 alphaMaskCutoff = materialData.get(MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD).asInteger();

		llassert(materialData.has(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD));
		llassert(materialData.get(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD).isInteger());
		S32 diffuseAlphaMode = materialData.get(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD).asInteger();


		cellParams.font = LLFontGL::getFontMonospace();

		cellParams.column = "id";
		cellParams.value = materialIDString;
		rowParams.columns.add(cellParams);

		cellParams.column = "normal_map";
		cellParams.value = normalMapID.asString();
		rowParams.columns.add(cellParams);

		cellParams.column = "specular_map";
		cellParams.value = specularMapID.asString();
		rowParams.columns.add(cellParams);

		cellParams.font = LLFontGL::getFontSansSerif();

		cellParams.column = "specular_color";
		cellParams.value = llformat("(%d, %d, %d, %d)", specularColor.mV[0],
			specularColor.mV[1], specularColor.mV[2], specularColor.mV[3]);
		rowParams.columns.add(cellParams);

		cellParams.column = "specular_exponent";
		cellParams.value = llformat("%d", specularExp);
		rowParams.columns.add(cellParams);

		cellParams.column = "env_intensity";
		cellParams.value = llformat("%d", envIntensity);
		rowParams.columns.add(cellParams);

		cellParams.column = "alpha_mask_cutoff";
		cellParams.value = llformat("%d", alphaMaskCutoff);
		rowParams.columns.add(cellParams);

		cellParams.column = "diffuse_alpha_mode";
		cellParams.value = llformat("%d", diffuseAlphaMode);
		rowParams.columns.add(cellParams);
		rowParams.value = materialID;

		mPostScrollList->addRow(rowParams);
	}
#endif
}

void LLFloaterDebugMaterials::printResponse(const std::string& pRequestType, const LLSD& pContent) const
{
	llinfos << "--------------------------------------------------------------------------" << llendl;
	llinfos << pRequestType << " Response: '" << pContent << "'" << llendl;
	llinfos << "--------------------------------------------------------------------------" << llendl;
}

void LLFloaterDebugMaterials::setState(EState pState)
{
	mState = pState;
	updateStatusMessage();
	updateControls();
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

void LLFloaterDebugMaterials::clearPutResults()
{
	mPutScrollList->deleteAllItems();
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

	bool isGoodPostEnabled = false;

	switch (getState())
	{
	case kNoRegion :
	case kCapabilitiesLoading :
	case kRequestStarted :
	case kNotEnabled :
		mGetButton->setEnabled(FALSE);
		mPutSetButton->setEnabled(FALSE);
		mPutClearButton->setEnabled(FALSE);
		mGoodPostButton->setEnabled(FALSE);
		mBadPostButton->setEnabled(FALSE);
		break;
	case kReady :
	case kRequestCompleted :
	case kError :
		mGetButton->setEnabled(TRUE);
		mPutSetButton->setEnabled(isPutEnabled);
		mPutClearButton->setEnabled(isPutEnabled);
		mGoodPostButton->setEnabled(isGoodPostEnabled);
		mBadPostButton->setEnabled(FALSE && TRUE);
		break;
	default :
		mGetButton->setEnabled(TRUE);
		mPutSetButton->setEnabled(isPutEnabled);
		mPutClearButton->setEnabled(isPutEnabled);
		mGoodPostButton->setEnabled(isGoodPostEnabled);
		mBadPostButton->setEnabled(FALSE && TRUE);
		llassert(0);
		break;
	}
}

std::string LLFloaterDebugMaterials::convertToPrintableMaterialID(const LLSD& pBinaryHash) const
{
	llassert(pBinaryHash.isBinary());
	const LLSD::Binary &materialIDValue = pBinaryHash.asBinary();
	unsigned int valueSize = materialIDValue.size();

	llassert(valueSize == 16);
	std::string materialID(reinterpret_cast<const char *>(&materialIDValue[0]), valueSize);
	std::string materialIDString;
	for (unsigned int i = 0U; i < (valueSize / 4); ++i)
	{
		if (i != 0U)
		{
			materialIDString += "-";
		}
		const U32 *value = reinterpret_cast<const U32*>(&materialID.c_str()[i * 4]);
		materialIDString += llformat("%08x", *value);
	}
	return materialIDString;
}

std::string LLFloaterDebugMaterials::convertToPrintableMaterialID(const LLMaterialID& pMaterialID) const
{
	std::string materialID(reinterpret_cast<const char *>(pMaterialID.get()), 16);
	std::string materialIDString;
	for (unsigned int i = 0U; i < 4; ++i)
	{
		if (i != 0U)
		{
			materialIDString += "-";
		}
		const U32 *value = reinterpret_cast<const U32*>(&materialID.c_str()[i * 4]);
		materialIDString += llformat("%08x", *value);
	}
	return materialIDString;
}

S32 LLFloaterDebugMaterials::getNormalMapOffsetX() const
{
	return getLineEditorValue(mNormalMapOffsetX);
}

S32 LLFloaterDebugMaterials::getNormalMapOffsetY() const
{
	return getLineEditorValue(mNormalMapOffsetY);
}

S32 LLFloaterDebugMaterials::getNormalMapRepeatX() const
{
	return getLineEditorValue(mNormalMapRepeatX);
}

S32 LLFloaterDebugMaterials::getNormalMapRepeatY() const
{
	return getLineEditorValue(mNormalMapRepeatY);
}

S32 LLFloaterDebugMaterials::getNormalMapRotation() const
{
	return getLineEditorValue(mNormalMapRotation);
}

S32 LLFloaterDebugMaterials::getSpecularMapOffsetX() const
{
	return getLineEditorValue(mSpecularMapOffsetX);
}

S32 LLFloaterDebugMaterials::getSpecularMapOffsetY() const
{
	return getLineEditorValue(mSpecularMapOffsetY);
}

S32 LLFloaterDebugMaterials::getSpecularMapRepeatX() const
{
	return getLineEditorValue(mSpecularMapRepeatX);
}

S32 LLFloaterDebugMaterials::getSpecularMapRepeatY() const
{
	return getLineEditorValue(mSpecularMapRepeatY);
}

S32 LLFloaterDebugMaterials::getSpecularMapRotation() const
{
	return getLineEditorValue(mSpecularMapRotation);
}

LLColor4U LLFloaterDebugMaterials::getSpecularColor() const
{
	const LLColor4& specularColor = mSpecularColor->get();
	LLColor4U specularColor4U = specularColor;

	specularColor4U.setAlpha(static_cast<U8>(llclamp(llround(mSpecularColorAlpha->get()), 0, 255)));

	return specularColor4U;
}

S32 LLFloaterDebugMaterials::getSpecularExponent() const
{
	return getLineEditorValue(mSpecularExponent);
}

S32 LLFloaterDebugMaterials::getEnvironmentExponent() const
{
	return getLineEditorValue(mEnvironmentExponent);
}

S32 LLFloaterDebugMaterials::getAlphMaskCutoff() const
{
	return getLineEditorValue(mAlphaMaskCutoff);
}

S32 LLFloaterDebugMaterials::getDiffuseAlphaMode() const
{
	return getLineEditorValue(mDiffuseAlphaMode);
}

S32 LLFloaterDebugMaterials::getLineEditorValue(const LLLineEditor *pLineEditor) const
{
	S32 value = 0;

	LLStringUtil::convertToS32(pLineEditor->getText(), value);

	return value;
}

MaterialsResponder::MaterialsResponder(const std::string& pMethod, const std::string& pCapabilityURL, CallbackFunction pCallback)
	: LLHTTPClient::Responder(),
	mMethod(pMethod),
	mCapabilityURL(pCapabilityURL),
	mCallback(pCallback)
{
}

MaterialsResponder::~MaterialsResponder()
{
}

void MaterialsResponder::result(const LLSD& pContent)
{
	mCallback(true, pContent);
}

void MaterialsResponder::error(U32 pStatus, const std::string& pReason)
{
	llwarns << "--------------------------------------------------------------------------" << llendl;
	llwarns << mMethod << " Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "' with url '" << mCapabilityURL	<< "' because " << pReason << llendl;
	llwarns << "--------------------------------------------------------------------------" << llendl;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}
