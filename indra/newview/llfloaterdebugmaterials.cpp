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

#define MATERIALS_CAP_ZIP_FIELD                   "Zipped"

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

#define MULTI_MATERIALS_STATUS_FIELD              "status"
#define MULTI_MATERIALS_DATA_FIELD                "data"

#define VIEWABLE_OBJECTS_REGION_ID_FIELD          "regionId"
#define VIEWABLE_OBJECTS_OBJECT_ID_FIELD          "objectId"
#define VIEWABLE_OBJECTS_MATERIAL_ID_FIELD        "materialId"

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

class MultiMaterialsResponder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	MultiMaterialsResponder(CallbackFunction pCallback, unsigned int pNumRequests);
	virtual ~MultiMaterialsResponder();

	void onMaterialsResponse(bool pRequestStatus, const LLSD& pContent);

protected:

private:
	bool appendRequestResults(bool pRequestStatus, const LLSD& pResults);
	void fireResponse();

	CallbackFunction mCallback;
	unsigned int     mNumRequests;

	bool             mRequestStatus;
	LLSD             mContent;
	LLMutex*         mMutex;
};

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

void LLFloaterDebugMaterials::draw()
{
	if (mUnparsedGetData.isDefined())
	{
		parseGetResponse();
	}
	if (mNextUnparsedQueryDataIndex >= 0)
	{
		parseQueryViewableObjects();
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
	mUnparsedGetData(),
	mNextUnparsedGetDataIndex(-1),
	mNextUnparsedQueryDataIndex(-1),
	mMultiMaterialsResponder()
{
}

LLFloaterDebugMaterials::~LLFloaterDebugMaterials()
{
	if (!mMultiMaterialsResponder)
	{
		mMultiMaterialsResponder.reset();
	}
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
	requestPostMaterials();
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

void LLFloaterDebugMaterials::onGetResponse(bool pRequestStatus, const LLSD& pContent)
{
	clearGetResults();
	if (pRequestStatus)
	{
		setState(kRequestCompleted);
		setUnparsedGetData(pContent);
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
	mMultiMaterialsResponder.reset();
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
			setState(kRequestStarted);
			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("GET", capURL, boost::bind(&LLFloaterDebugMaterials::onGetResponse, this, _1, _2));
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

			LLSD materialsData = LLSD::emptyMap();
			materialsData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = facesData;

			std::string materialString = zip_llsd(materialsData);
			S32 materialSize = materialString.size();

			if (materialSize <= 0)
			{
				LL_ERRS("debugMaterials") << "cannot zip LLSD binary content" << LL_ENDL;
			}
			else
			{
				LLSD::Binary materialBinary;
				materialBinary.resize(materialSize);
				memcpy(materialBinary.data(), materialString.data(), materialSize);

				LLSD putData = LLSD::emptyMap();
				putData[MATERIALS_CAP_ZIP_FIELD] = materialBinary;

				LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("PUT", capURL, boost::bind(&LLFloaterDebugMaterials::onPutResponse, this, _1, _2));
				LLHTTPClient::put(capURL, putData, materialsResponder);
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

void LLFloaterDebugMaterials::requestPostMaterials()
{
	llassert(!mMultiMaterialsResponder);

	std::vector<LLScrollListItem*> selectedItems = mViewableObjectsScrollList->getAllSelected();
	std::map<LLUUID, std::string> uniqueRegions;

	if (!selectedItems.empty())
	{
		for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
			selectedItemIter != selectedItems.end(); ++selectedItemIter)
		{
			const LLScrollListItem* selectedItem = *selectedItemIter;
			const LLSD& selectedItemValue = selectedItem->getValue();
			llassert(selectedItemValue.isMap());

			llassert(selectedItemValue.has(VIEWABLE_OBJECTS_REGION_ID_FIELD));
			llassert(selectedItemValue.get(VIEWABLE_OBJECTS_REGION_ID_FIELD).isUUID());
			const LLUUID& regionId = selectedItemValue.get(VIEWABLE_OBJECTS_REGION_ID_FIELD).asUUID();
			if (uniqueRegions.find(regionId) == uniqueRegions.end())
			{
				llassert(selectedItemValue.has(VIEWABLE_OBJECTS_OBJECT_ID_FIELD));
				llassert(selectedItemValue.get(VIEWABLE_OBJECTS_OBJECT_ID_FIELD).isUUID());
				const LLUUID& objectId = selectedItemValue.get(VIEWABLE_OBJECTS_OBJECT_ID_FIELD).asUUID();
				LLViewerObject* viewerObject = gObjectList.findObject(objectId);
				if (viewerObject != NULL)
				{
					LLViewerRegion* region = viewerObject->getRegion();
					if (region != NULL)
					{
						if (!region->capabilitiesReceived())
						{
							LL_WARNS("debugMaterials") << "region '" << region->getName() << "' (id:"
								<< region->getRegionID().asString() << ") has not received capabilities"
								<< LL_ENDL;
						}
						else
						{
							std::string capURL = region->getCapability(MATERIALS_CAPABILITY_NAME);

							if (capURL.empty())
							{
								LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
									<< "' is not defined on the current region '" << region->getName() << "'" << LL_ENDL;
							}
							else
							{
								uniqueRegions.insert(std::make_pair<LLUUID, std::string>(regionId, capURL));
							}
						}
					}
				}
			}
		}

		unsigned int numRegions = static_cast<unsigned int>(uniqueRegions.size());

		if (numRegions > 0U)
		{
			setState(kRequestStarted);
			mMultiMaterialsResponder = MultiMaterialsResponderPtr(new MultiMaterialsResponder(boost::bind(&LLFloaterDebugMaterials::onPostResponse, this, _1, _2), numRegions));

			for (std::map<LLUUID, std::string>::const_iterator regionIdIter = uniqueRegions.begin();
				regionIdIter != uniqueRegions.end(); ++regionIdIter)
			{
				const LLUUID& regionId = regionIdIter->first;
				std::string capURL = regionIdIter->second;

				LLSD materialIdsData = LLSD::emptyArray();

				for (std::vector<LLScrollListItem*>::const_iterator selectedItemIter = selectedItems.begin();
					selectedItemIter != selectedItems.end(); ++selectedItemIter)
				{
					const LLScrollListItem* selectedItem = *selectedItemIter;
					const LLSD& selectedItemValue = selectedItem->getValue();

					llassert(selectedItemValue.has(VIEWABLE_OBJECTS_REGION_ID_FIELD));
					llassert(selectedItemValue.get(VIEWABLE_OBJECTS_REGION_ID_FIELD).isUUID());
					const LLUUID& selectedItemRegionId = selectedItemValue.get(VIEWABLE_OBJECTS_REGION_ID_FIELD).asUUID();
					if (selectedItemRegionId == regionId)
					{
						llassert(selectedItemValue.has(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD));
						llassert(selectedItemValue.get(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD).isBinary());
						const LLSD& materidIdLLSD = selectedItemValue.get(VIEWABLE_OBJECTS_MATERIAL_ID_FIELD);

						materialIdsData.append(materidIdLLSD);
					}
				}

				if (materialIdsData.size() <= 0)
				{
					LL_ERRS("debugMaterials") << "no material IDs to POST to region id " << regionId.asString()
						<< LL_ENDL;
				}
				else
				{
					std::string materialsString = zip_llsd(materialIdsData);
					S32 materialsSize = materialsString.size();

					if (materialsSize <= 0)
					{
						LL_ERRS("debugMaterials") << "cannot zip LLSD binary content" << LL_ENDL;
					}
					else
					{
						LLSD::Binary materialsBinary;
						materialsBinary.resize(materialsSize);
						memcpy(materialsBinary.data(), materialsString.data(), materialsSize);

						LLSD postData = LLSD::emptyMap();
						postData[MATERIALS_CAP_ZIP_FIELD] = materialsBinary;

						LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("POST",
							capURL, boost::bind(&MultiMaterialsResponder::onMaterialsResponse, mMultiMaterialsResponder.get(), _1, _2));
						LLHTTPClient::post(capURL, postData, materialsResponder);
					}
				}
			}
		}
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

							std::string materialIDString = convertToPrintableMaterialID(objectMaterialID);
							cellParams.column = "material_id";
							cellParams.value = materialIDString;
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

void LLFloaterDebugMaterials::parseGetResponse()
{
	llassert(mUnparsedGetData.isDefined());
	llassert(mNextUnparsedGetDataIndex >= 0);

	if (mUnparsedGetData.isDefined())
	{
		LLScrollListCell::Params cellParams;
		LLScrollListItem::Params normalMapRowParams;
		LLScrollListItem::Params specularMapRowParams;
		LLScrollListItem::Params otherDataRowParams;

		llassert(mUnparsedGetData.isArray());
		LLSD::array_const_iterator materialIter = mUnparsedGetData.beginArray();
		S32 materialIndex;

		for (materialIndex = 0;
			(materialIndex < mNextUnparsedGetDataIndex) && (materialIter != mUnparsedGetData.endArray());
			++materialIndex, ++materialIter)
		{
		}

		for (S32 currentParseCount = 0;
			(currentParseCount < 10) && (materialIter != mUnparsedGetData.endArray());
			++currentParseCount, ++materialIndex, ++materialIter)
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

		if (materialIter != mUnparsedGetData.endArray())
		{
			mNextUnparsedGetDataIndex = materialIndex;
			updateGetParsingStatus();
		}
		else
		{
			clearUnparsedGetData();
		}
	}
}

void LLFloaterDebugMaterials::parsePutResponse(const LLSD& pContent)
{
	clearPutResults();

	LLScrollListCell::Params cellParams;
	LLScrollListItem::Params rowParams;

	llassert(pContent.isMap());
	llassert(pContent.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(pContent.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

	LLSD::Binary responseBinary = pContent.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
	S32 responseSize = static_cast<S32>(responseBinary.size());
	std::string responseString(reinterpret_cast<const char*>(responseBinary.data()), responseSize);

	std::istringstream responseStream(responseString);

	LLSD responseContent;
	if (!unzip_llsd(responseContent, responseStream, responseSize))
	{
		LL_ERRS("debugMaterials") << "cannot unzip LLSD binary content" << LL_ENDL;
	}
	else
	{
		llassert(responseContent.isArray());
		for (LLSD::array_const_iterator faceIter = responseContent.beginArray(); faceIter != responseContent.endArray();
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
}

void LLFloaterDebugMaterials::parsePostResponse(const LLSD& pMultiContent)
{
	clearPostResults();

	llassert(pMultiContent.isArray());
	for (LLSD::array_const_iterator contentIter = pMultiContent.beginArray();
		contentIter != pMultiContent.endArray(); ++contentIter)
	{
		const LLSD& content = *contentIter;

		llassert(content.isMap());
		llassert(content.has(MULTI_MATERIALS_STATUS_FIELD));
		llassert(content.get(MULTI_MATERIALS_STATUS_FIELD).isBoolean());
		if (content.get(MULTI_MATERIALS_STATUS_FIELD).asBoolean())
		{
			llassert(content.has(MULTI_MATERIALS_DATA_FIELD));
			llassert(content.get(MULTI_MATERIALS_DATA_FIELD).isMap());
			const LLSD& postData = content.get(MULTI_MATERIALS_DATA_FIELD);

			llassert(postData.has(MATERIALS_CAP_ZIP_FIELD));
			llassert(postData.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

			LLSD::Binary postDataBinary = postData.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
			S32 postDataSize = static_cast<S32>(postDataBinary.size());
			std::string postDataString(reinterpret_cast<const char*>(postDataBinary.data()), postDataSize);

			std::istringstream postDataStream(postDataString);

			LLSD unzippedPostData;
			if (!unzip_llsd(unzippedPostData, postDataStream, postDataSize))
			{
				LL_ERRS("debugMaterials") << "cannot unzip LLSD binary content" << LL_ENDL;
			}

			LLScrollListCell::Params cellParams;
			LLScrollListItem::Params normalMapRowParams;
			LLScrollListItem::Params specularMapRowParams;
			LLScrollListItem::Params otherDataRowParams;

			llassert(unzippedPostData.isArray());
			for (LLSD::array_const_iterator materialIter = unzippedPostData.beginArray();
				materialIter != unzippedPostData.endArray(); ++materialIter)
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

				mPostNormalMapScrollList->addRow(normalMapRowParams);
				mPostSpecularMapScrollList->addRow(specularMapRowParams);
				mPostOtherDataScrollList->addRow(otherDataRowParams);
			}
		}
	}
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
	clearUnparsedGetData();
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
	clearUnparsedQueryData();
}

void LLFloaterDebugMaterials::setUnparsedGetData(const LLSD& pGetData)
{
	llassert(pGetData.isMap());
	llassert(pGetData.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(pGetData.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

	LLSD::Binary getDataBinary = pGetData.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
	S32 getDataSize = static_cast<S32>(getDataBinary.size());
	std::string getDataString(reinterpret_cast<const char*>(getDataBinary.data()), getDataSize);

	std::istringstream getDataStream(getDataString);

	llassert(!mUnparsedGetData.isDefined());
	if (!unzip_llsd(mUnparsedGetData, getDataStream, getDataSize))
	{
		LL_ERRS("debugMaterials") << "cannot unzip LLSD binary content" << LL_ENDL;
	}
	mNextUnparsedGetDataIndex = 0;

	updateGetParsingStatus();
}

void LLFloaterDebugMaterials::clearUnparsedGetData()
{
	mUnparsedGetData.clear();
	mNextUnparsedGetDataIndex = -1;

	updateGetParsingStatus();
}

void LLFloaterDebugMaterials::updateGetParsingStatus()
{
	std::string parsingStatus;

	if (mUnparsedGetData.isDefined())
	{
		LLLocale locale(LLStringUtil::getLocale());
		std::string numProcessedString;
		LLResMgr::getInstance()->getIntegerString(numProcessedString, mNextUnparsedGetDataIndex);

		std::string numTotalString;
		LLResMgr::getInstance()->getIntegerString(numTotalString, mUnparsedGetData.size());

		LLStringUtil::format_map_t stringArgs;
		stringArgs["[NUM_PROCESSED]"] = numProcessedString;
		stringArgs["[NUM_TOTAL]"] = numTotalString;

		parsingStatus = getString("loading_status_in_progress", stringArgs);
	}
	else
	{
		parsingStatus = getString("loading_status_done");
	}

	mParsingStatusText->setText(static_cast<const LLStringExplicit>(parsingStatus));
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
	LL_WARNS("debugMaterials") << "--------------------------------------------------------------------------" << LL_ENDL;
	LL_WARNS("debugMaterials") << mMethod << " Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "' with url '" << mCapabilityURL	<< "' because " << pReason << LL_ENDL;
	LL_WARNS("debugMaterials") << "--------------------------------------------------------------------------" << LL_ENDL;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}

MultiMaterialsResponder::MultiMaterialsResponder(CallbackFunction pCallback, unsigned int pNumRequests)
	: mCallback(pCallback),
	mNumRequests(pNumRequests),
	mRequestStatus(true),
	mContent(LLSD::emptyArray()),
	mMutex(NULL)
{
	mMutex = new LLMutex(NULL);
	llassert(mMutex);
}

MultiMaterialsResponder::~MultiMaterialsResponder()
{
	llassert(mMutex);
	if (mMutex)
	{
		delete mMutex;
	}
}

void MultiMaterialsResponder::onMaterialsResponse(bool pRequestStatus, const LLSD& pContent)
{
	LLSD result = LLSD::emptyMap();

	result[MULTI_MATERIALS_STATUS_FIELD] = static_cast<LLSD::Boolean>(pRequestStatus);
	result[MULTI_MATERIALS_DATA_FIELD] = pContent;

	if (appendRequestResults(pRequestStatus, result))
	{
		fireResponse();
	}
}

bool MultiMaterialsResponder::appendRequestResults(bool pRequestStatus, const LLSD& pResults)
{
	llassert(mMutex);
	LLMutexLock mutexLock(mMutex);

	mRequestStatus = mRequestStatus && pRequestStatus;
	mContent.append(pResults);
	llassert(mNumRequests > 0U);
	return (--mNumRequests == 0U);
}

void MultiMaterialsResponder::fireResponse()
{
	mCallback(mRequestStatus, mContent);
}
