/** 
* @file llfloaterstinson.cpp
* @brief Implementation of llfloaterstinson
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
#include "llenvmanager.h"
#include "llfloater.h"
#include "llfontgl.h"
#include "llhttpclient.h"
#include "llscrolllistcell.h"
#include "llscrolllistctrl.h"
#include "llscrolllistitem.h"
#include "llsd.h"
#include "llselectmgr.h"
#include "llstring.h"
#include "llstyle.h"
#include "lltextbase.h"
#include "lluicolortable.h"
#include "llviewerobject.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "v4color.h"
#include "v4coloru.h"

#define MATERIALS_CAPABILITY_NAME              "RenderMaterials"

#define MATERIALS_CAP_FULL_PER_FACE_FIELD      "FullMaterialsPerFace"
#define MATERIALS_CAP_FACE_FIELD               "Face"
#define MATERIALS_CAP_MATERIAL_FIELD           "Material"
#define MATERIALS_CAP_OBJECT_ID_FIELD          "ID"
#define MATERIALS_CAP_MATERIAL_ID_FIELD        "MaterialID"

#define MATERIALS_CAP_NORMAL_MAP_FIELD         "NormMap"
#define MATERIALS_CAP_SPECULAR_MAP_FIELD       "SpecMap"
#define MATERIALS_CAP_SPECULAR_COLOR_FIELD     "SpecColor"
#define MATERIALS_CAP_SPECULAR_EXP_FIELD       "SpecExp"
#define MATERIALS_CAP_ENV_INTENSITY_FIELD      "EnvIntensity"
#define MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD  "AlphaMaskCutoff"
#define MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD "DiffuseAlphaMode"

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

BOOL LLFloaterStinson::postBuild()
{
	mStatusText = findChild<LLTextBase>("material_status");
	llassert(mStatusText != NULL);

	mGetButton = findChild<LLButton>("get_button");
	llassert(mGetButton != NULL);
	mGetButton->setCommitCallback(boost::bind(&LLFloaterStinson::onGetClicked, this));

	mGetScrollList = findChild<LLScrollListCtrl>("get_scroll_list");
	llassert(mGetScrollList != NULL);
	mGetScrollList->setCommitCallback(boost::bind(&LLFloaterStinson::onGetResultsSelectionChange, this));

	mPutSetButton = findChild<LLButton>("put_set_button");
	llassert(mPutSetButton != NULL);
	mPutSetButton->setCommitCallback(boost::bind(&LLFloaterStinson::onPutSetClicked, this));

	mPutClearButton = findChild<LLButton>("put_clear_button");
	llassert(mPutClearButton != NULL);
	mPutClearButton->setCommitCallback(boost::bind(&LLFloaterStinson::onPutClearClicked, this));

	mPutScrollList = findChild<LLScrollListCtrl>("put_scroll_list");
	llassert(mPutScrollList != NULL);

	mGoodPostButton = findChild<LLButton>("good_post_button");
	llassert(mGoodPostButton != NULL);
	mGoodPostButton->setCommitCallback(boost::bind(&LLFloaterStinson::onGoodPostClicked, this));

	mBadPostButton = findChild<LLButton>("bad_post_button");
	llassert(mBadPostButton != NULL);
	mBadPostButton->setCommitCallback(boost::bind(&LLFloaterStinson::onBadPostClicked, this));

	mPostScrollList = findChild<LLScrollListCtrl>("post_scroll_list");
	llassert(mPostScrollList != NULL);

	mWarningColor = LLUIColorTable::instance().getColor("MaterialWarningColor");
	mErrorColor = LLUIColorTable::instance().getColor("MaterialErrorColor");

	setState(kNoRegion);

	return LLFloater::postBuild();
}

void LLFloaterStinson::onOpen(const LLSD& pKey)
{
	LLFloater::onOpen(pKey);

	if (!mRegionCrossConnection.connected())
	{
		mRegionCrossConnection = LLEnvManagerNew::instance().setRegionChangeCallback(boost::bind(&LLFloaterStinson::onRegionCross, this));
	}

	if (!mTeleportFailedConnection.connected())
	{
		mTeleportFailedConnection = LLViewerParcelMgr::getInstance()->setTeleportFailedCallback(boost::bind(&LLFloaterStinson::onRegionCross, this));
	}

	if (!mSelectionUpdateConnection.connected())
	{
		mSelectionUpdateConnection = LLSelectMgr::getInstance()->mUpdateSignal.connect(boost::bind(&LLFloaterStinson::onInWorldSelectionChange, this));
	}

	checkRegionMaterialStatus();
	clearGetResults();
	clearPutResults();
	clearPostResults();
	mGetScrollList->setCommitOnSelectionChange(TRUE);
}

void LLFloaterStinson::onClose(bool pIsAppQuitting)
{
	mGetScrollList->setCommitOnSelectionChange(FALSE);
	clearGetResults();
	clearPutResults();
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

LLFloaterStinson::LLFloaterStinson(const LLSD& pParams)
	: LLFloater(pParams),
	mStatusText(NULL),
	mGetButton(NULL),
	mGetScrollList(NULL),
	mPutSetButton(NULL),
	mPutClearButton(NULL),
	mPutScrollList(NULL),
	mGoodPostButton(NULL),
	mBadPostButton(NULL),
	mPostScrollList(NULL),
	mState(kNoRegion),
	mWarningColor(),
	mErrorColor(),
	mRegionCrossConnection(),
	mTeleportFailedConnection(),
	mSelectionUpdateConnection()
{
}

LLFloaterStinson::~LLFloaterStinson()
{
}

void LLFloaterStinson::onGetClicked()
{
	requestGetMaterials();
}

void LLFloaterStinson::onPutSetClicked()
{
	requestPutMaterials(true);
}

void LLFloaterStinson::onPutClearClicked()
{
	requestPutMaterials(false);
}

void LLFloaterStinson::onGoodPostClicked()
{
	requestPostMaterials(true);
}

void LLFloaterStinson::onBadPostClicked()
{
	requestPostMaterials(false);
}

void LLFloaterStinson::onRegionCross()
{
	checkRegionMaterialStatus();
	clearGetResults();
	clearPutResults();
	clearPostResults();
}

void LLFloaterStinson::onGetResultsSelectionChange()
{
	updateControls();
}

void LLFloaterStinson::onInWorldSelectionChange()
{
	updateControls();
}

void LLFloaterStinson::onDeferredCheckRegionMaterialStatus(LLUUID regionId)
{
	checkRegionMaterialStatus(regionId);
}

void LLFloaterStinson::onDeferredRequestGetMaterials(LLUUID regionId)
{
	requestGetMaterials(regionId);
}

void LLFloaterStinson::onDeferredRequestPutMaterials(LLUUID regionId, bool pIsDoSet)
{
	requestPutMaterials(regionId, pIsDoSet);
}

void LLFloaterStinson::onDeferredRequestPostMaterials(LLUUID regionId, bool pUseGoodData)
{
	requestPostMaterials(regionId, pUseGoodData);
}

void LLFloaterStinson::onGetResponse(bool pRequestStatus, const LLSD& pContent)
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

void LLFloaterStinson::onPutResponse(bool pRequestStatus, const LLSD& pContent)
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

void LLFloaterStinson::onPostResponse(bool pRequestStatus, const LLSD& pContent)
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

void LLFloaterStinson::checkRegionMaterialStatus()
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
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterStinson::onDeferredCheckRegionMaterialStatus, this, region->getRegionID()));
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

void LLFloaterStinson::checkRegionMaterialStatus(const LLUUID& regionId)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		checkRegionMaterialStatus();
	}
}

void LLFloaterStinson::requestGetMaterials()
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
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterStinson::onDeferredRequestGetMaterials, this, region->getRegionID()));
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
			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("GET", capURL, boost::bind(&LLFloaterStinson::onGetResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request GET to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "'" << llendl;
			LLHTTPClient::get(capURL, materialsResponder);
		}
	}
}

void LLFloaterStinson::requestGetMaterials(const LLUUID& regionId)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestGetMaterials();
	}
}

void LLFloaterStinson::requestPutMaterials(bool pIsDoSet)
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
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterStinson::onDeferredRequestPutMaterials, this, region->getRegionID(), pIsDoSet));
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


			LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
			for (LLObjectSelection::valid_iterator objectIter = selectionHandle->valid_begin();
				objectIter != selectionHandle->valid_end(); ++objectIter)
			{
				LLSelectNode* objectNode = *objectIter;
				LLViewerObject* viewerObject = objectNode->getObject();

				S32 numFaces = viewerObject->getNumFaces();
				for (S32 curFaceIndex = 0; curFaceIndex < numFaces; ++curFaceIndex)
				{
					LLSD materialData = LLSD::emptyMap();

#define FACE_MODULATOR 4
					if ((curFaceIndex % FACE_MODULATOR) == 0)
					{
						materialData[MATERIALS_CAP_NORMAL_MAP_FIELD] = LLUUID("dd88438d-895e-4cc4-3557-f8b6870be6e5"); // Library > Textures > Rock > Rock - Rippling
						materialData[MATERIALS_CAP_SPECULAR_MAP_FIELD] = LLUUID("c7f1beb3-4c5f-f70e-6d96-7668ff8aea0a"); // Library > Textures > Rock > Rock - Granite
						LLColor4U specularColor(255, 255, 255, 255);
						materialData[MATERIALS_CAP_SPECULAR_COLOR_FIELD] = specularColor.getValue();
						materialData[MATERIALS_CAP_SPECULAR_EXP_FIELD] = static_cast<LLSD::Integer>(100);
						materialData[MATERIALS_CAP_ENV_INTENSITY_FIELD] = static_cast<LLSD::Integer>(25);
						materialData[MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD] = static_cast<LLSD::Integer>(37);
						materialData[MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD] = static_cast<LLSD::Integer>(0);
					}
					else if ((curFaceIndex % FACE_MODULATOR) == 1)
					{
						materialData[MATERIALS_CAP_NORMAL_MAP_FIELD] = LLUUID("cfcd9d0b-f04b-f01a-8b29-519e27078896"); // Library > Textures > Terrain Textures > Terrain Textures - Winter > Primitive Island - Base Ice-rock
						materialData[MATERIALS_CAP_SPECULAR_MAP_FIELD] = LLUUID("fcad96ba-3495-d426-9713-21cf721332a4"); // Library > Textures > Terrain Textures > Terrain Textures - Winter > Primitive Island - Ice-rock
						LLColor4U specularColor(100, 50, 200, 128);
						materialData[MATERIALS_CAP_SPECULAR_COLOR_FIELD] = specularColor.getValue();
						materialData[MATERIALS_CAP_SPECULAR_EXP_FIELD] = static_cast<LLSD::Integer>(255);
						materialData[MATERIALS_CAP_ENV_INTENSITY_FIELD] = static_cast<LLSD::Integer>(0);
						materialData[MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD] = static_cast<LLSD::Integer>(5);
						materialData[MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD] = static_cast<LLSD::Integer>(1);
					}
					else if ((curFaceIndex % FACE_MODULATOR) == 2)
					{
						materialData[MATERIALS_CAP_NORMAL_MAP_FIELD] = LLUUID("6ed3abd3-527a-856d-3771-2a04ea4c16e1"); // Library > Textures > Waterfalls > Water - ripple layer 1
						materialData[MATERIALS_CAP_SPECULAR_MAP_FIELD] = LLUUID("e7c01539-4836-cd47-94ac-55af7502e4db"); // Library > Textures > Waterfalls > Water - ripple layer 2
						LLColor4U specularColor(128, 128, 128, 255);
						materialData[MATERIALS_CAP_SPECULAR_COLOR_FIELD] = specularColor.getValue();
						materialData[MATERIALS_CAP_SPECULAR_EXP_FIELD] = static_cast<LLSD::Integer>(1);
						materialData[MATERIALS_CAP_ENV_INTENSITY_FIELD] = static_cast<LLSD::Integer>(255);
						materialData[MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD] = static_cast<LLSD::Integer>(75);
						materialData[MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD] = static_cast<LLSD::Integer>(3);
					}
					else if ((curFaceIndex % FACE_MODULATOR) == 3)
					{
						// do nothing
					}

					if ((curFaceIndex % FACE_MODULATOR) != 3)
					{
						LLSD faceData = LLSD::emptyMap();
						faceData[MATERIALS_CAP_FACE_FIELD] = static_cast<LLSD::Integer>(curFaceIndex);
						faceData[MATERIALS_CAP_OBJECT_ID_FIELD] = static_cast<LLSD::Integer>(viewerObject->getLocalID());
						if (pIsDoSet)
						{
							faceData[MATERIALS_CAP_MATERIAL_FIELD] = materialData;
						}
						facesData.append(faceData);
					}
				}
			}

			LLSD putData = LLSD::emptyMap();
			putData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = facesData;

			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("PUT", capURL, boost::bind(&LLFloaterStinson::onPutResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request PUT to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "' and with data " << putData << llendl;
			LLHTTPClient::put(capURL, putData, materialsResponder);
		}
	}
}

void LLFloaterStinson::requestPutMaterials(const LLUUID& regionId, bool pIsDoSet)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestPutMaterials(pIsDoSet);
	}
}

void LLFloaterStinson::requestPostMaterials(bool pUseGoodData)
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
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterStinson::onDeferredRequestPostMaterials, this, region->getRegionID(), pUseGoodData));
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

			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsResponder("POST", capURL, boost::bind(&LLFloaterStinson::onPostResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request POST to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "' and with data " << postData << llendl;
			LLHTTPClient::post(capURL, postData, materialsResponder);
		}
	}
}

void LLFloaterStinson::requestPostMaterials(const LLUUID& regionId, bool pUseGoodData)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestPostMaterials(pUseGoodData);
	}
}

void LLFloaterStinson::parseGetResponse(const LLSD& pContent)
{
	printResponse("GET", pContent);
	clearGetResults();

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
		S32 diffuseAlphaMode = static_cast<BOOL>(materialData.get(MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD).asInteger());


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

		mGetScrollList->addRow(rowParams);
	}
}

void LLFloaterStinson::parsePutResponse(const LLSD& pContent)
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

void LLFloaterStinson::parsePostResponse(const LLSD& pContent)
{
	printResponse("POST", pContent);
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
}

void LLFloaterStinson::printResponse(const std::string& pRequestType, const LLSD& pContent) const
{
	llinfos << "--------------------------------------------------------------------------" << llendl;
	llinfos << pRequestType << " Response: '" << pContent << "'" << llendl;
	llinfos << "--------------------------------------------------------------------------" << llendl;
}

void LLFloaterStinson::setState(EState pState)
{
	mState = pState;
	updateStatusMessage();
	updateControls();
}

void LLFloaterStinson::clearGetResults()
{
	mGetScrollList->deleteAllItems();
}

void LLFloaterStinson::clearPutResults()
{
	mPutScrollList->deleteAllItems();
}

void LLFloaterStinson::clearPostResults()
{
	mPostScrollList->deleteAllItems();
}

void LLFloaterStinson::updateStatusMessage()
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

void LLFloaterStinson::updateControls()
{
	LLObjectSelectionHandle selectionHandle = LLSelectMgr::getInstance()->getEditSelection();
	bool isPutEnabled = (selectionHandle->valid_begin() != selectionHandle->valid_end());

	S32 numGetResultsSelected = mGetScrollList->getNumSelected();
	bool isGoodPostEnabled = (numGetResultsSelected > 0);

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
		mBadPostButton->setEnabled(TRUE);
		break;
	default :
		mGetButton->setEnabled(TRUE);
		mPutSetButton->setEnabled(isPutEnabled);
		mPutClearButton->setEnabled(isPutEnabled);
		mGoodPostButton->setEnabled(isGoodPostEnabled);
		mBadPostButton->setEnabled(TRUE);
		llassert(0);
		break;
	}
}

std::string LLFloaterStinson::convertToPrintableMaterialID(const LLSD& pBinaryHash) const
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
