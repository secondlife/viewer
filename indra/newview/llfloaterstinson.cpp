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

#include "llfloaterstinson.h"

#include <string>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llbutton.h"
#include "llenvmanager.h"
#include "llfloater.h"
#include "llhttpclient.h"
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

#define MATERIALS_CAPABILITY_NAME         "RenderMaterials"

#define MATERIALS_CAP_FULL_PER_FACE_FIELD "FullMaterialsPerFace"
#define MATERIALS_CAP_FACE_FIELD          "Face"
#define MATERIALS_CAP_MATERIAL_FIELD      "Material"
#define MATERIALS_CAP_OBJECT_ID_FIELD     "ID"

class MaterialsGetResponder : public LLHTTPClient::Responder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	MaterialsGetResponder(const std::string& pCapabilityURL, CallbackFunction pCallback);
	virtual ~MaterialsGetResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

protected:

private:
	std::string      mCapabilityURL;
	CallbackFunction mCallback;
};

class MaterialsPutResponder : public LLHTTPClient::Responder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	MaterialsPutResponder(const std::string& pCapabilityURL, CallbackFunction pCallback);
	virtual ~MaterialsPutResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

protected:

private:
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

	mPutButton = findChild<LLButton>("put_button");
	llassert(mPutButton != NULL);
	mPutButton->setCommitCallback(boost::bind(&LLFloaterStinson::onPutClicked, this));

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

	checkRegionMaterialStatus();
}

void LLFloaterStinson::onClose(bool pIsAppQuitting)
{
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
	mPutButton(NULL),
	mState(kNoRegion),
	mWarningColor(),
	mErrorColor(),
	mRegionCrossConnection(),
	mTeleportFailedConnection()
{
}

LLFloaterStinson::~LLFloaterStinson()
{
}

void LLFloaterStinson::onGetClicked()
{
	requestGetMaterials();
}

void LLFloaterStinson::onPutClicked()
{
	requestPutMaterials();
}

void LLFloaterStinson::onRegionCross()
{
	checkRegionMaterialStatus();
}

void LLFloaterStinson::onDeferredCheckRegionMaterialStatus(LLUUID regionId)
{
	checkRegionMaterialStatus(regionId);
}

void LLFloaterStinson::onDeferredRequestGetMaterials(LLUUID regionId)
{
	requestGetMaterials(regionId);
}

void LLFloaterStinson::onDeferredRequestPutMaterials(LLUUID regionId)
{
	requestPutMaterials(regionId);
}

void LLFloaterStinson::onGetResponse(bool pRequestStatus, const LLSD& pContent)
{
	if (pRequestStatus)
	{
		setState(kRequestCompleted);
		parseResponse("GET", pContent);
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
		parseResponse("PUT", pContent);
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
			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsGetResponder(capURL, boost::bind(&LLFloaterStinson::onGetResponse, this, _1, _2));
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

void LLFloaterStinson::requestPutMaterials()
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
		region->setCapabilitiesReceivedCallback(boost::bind(&LLFloaterStinson::onDeferredRequestPutMaterials, this, region->getRegionID()));
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
				
				LLSD faceData = LLSD::emptyMap();
				faceData[MATERIALS_CAP_FACE_FIELD] = static_cast<LLSD::Integer>(0);
				faceData[MATERIALS_CAP_MATERIAL_FIELD] = static_cast<LLSD::Integer>(0);
				faceData[MATERIALS_CAP_OBJECT_ID_FIELD] = viewerObject->getID();

				facesData.append(faceData);
			}

			LLSD putData = LLSD::emptyMap();
			putData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = facesData;

			LLHTTPClient::ResponderPtr materialsResponder = new MaterialsPutResponder(capURL, boost::bind(&LLFloaterStinson::onPutResponse, this, _1, _2));
			llinfos << "STINSON DEBUG: sending request PUT to capability '" << MATERIALS_CAPABILITY_NAME
				<< "' with url '" << capURL << "' and with data " << putData << llendl;
			LLHTTPClient::put(capURL, putData, materialsResponder);
		}
	}
}

void LLFloaterStinson::requestPutMaterials(const LLUUID& regionId)
{
	const LLViewerRegion *region = gAgent.getRegion();

	if ((region != NULL) && (region->getRegionID() == regionId))
	{
		requestPutMaterials();
	}
}

void LLFloaterStinson::parseResponse(const std::string& pRequestType, const LLSD& pContent) const
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
	switch (getState())
	{
	case kNoRegion :
	case kCapabilitiesLoading :
	case kRequestStarted :
	case kNotEnabled :
		mGetButton->setEnabled(FALSE);
		break;
	case kReady :
	case kRequestCompleted :
	case kError :
		mGetButton->setEnabled(TRUE);
		break;
	default :
		mGetButton->setEnabled(TRUE);
		llassert(0);
		break;
	}
}

MaterialsGetResponder::MaterialsGetResponder(const std::string& pCapabilityURL, CallbackFunction pCallback)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mCallback(pCallback)
{
}

MaterialsGetResponder::~MaterialsGetResponder()
{
}

void MaterialsGetResponder::result(const LLSD& pContent)
{
	mCallback(true, pContent);
}

void MaterialsGetResponder::error(U32 pStatus, const std::string& pReason)
{
	llwarns << "--------------------------------------------------------------------------" << llendl;
	llwarns << "GET Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "' with url '" << mCapabilityURL	<< "' because " << pReason << llendl;
	llwarns << "--------------------------------------------------------------------------" << llendl;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}

MaterialsPutResponder::MaterialsPutResponder(const std::string& pCapabilityURL, CallbackFunction pCallback)
	: LLHTTPClient::Responder(),
	mCapabilityURL(pCapabilityURL),
	mCallback(pCallback)
{
}

MaterialsPutResponder::~MaterialsPutResponder()
{
}

void MaterialsPutResponder::result(const LLSD& pContent)
{
	mCallback(true, pContent);
}

void MaterialsPutResponder::error(U32 pStatus, const std::string& pReason)
{
	llwarns << "--------------------------------------------------------------------------" << llendl;
	llwarns << "PUT Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "' with url '" << mCapabilityURL	<< "' because " << pReason << llendl;
	llwarns << "--------------------------------------------------------------------------" << llendl;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}
