/** 
* @file llpathfindingmanager.cpp
* @brief Implementation of llpathfindingmanager
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

#include "llpathfindingmanager.h"

#include <string>
#include <map>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

#include "llagent.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llpathfindingcharacterlist.h"
#include "llpathfindinglinkset.h"
#include "llpathfindinglinksetlist.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llpathfindingobject.h"
#include "llpathinglib.h"
#include "llsingleton.h"
#include "llsd.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llviewerregion.h"
#include "llweb.h"
#include "llcorehttputil.h"
#include "llworld.h"

#define CAP_SERVICE_RETRIEVE_NAVMESH        "RetrieveNavMeshSrc"

#define CAP_SERVICE_NAVMESH_STATUS          "NavMeshGenerationStatus"

#define CAP_SERVICE_GET_OBJECT_LINKSETS     "RegionObjects"
#define CAP_SERVICE_SET_OBJECT_LINKSETS     "ObjectNavMeshProperties"
#define CAP_SERVICE_TERRAIN_LINKSETS        "TerrainNavMeshProperties"

#define CAP_SERVICE_CHARACTERS              "CharacterProperties"

#define SIM_MESSAGE_NAVMESH_STATUS_UPDATE   "/message/NavMeshStatusUpdate"
#define SIM_MESSAGE_AGENT_STATE_UPDATE      "/message/AgentStateUpdate"
#define SIM_MESSAGE_BODY_FIELD              "body"

#define CAP_SERVICE_AGENT_STATE             "AgentState"

#define AGENT_STATE_CAN_REBAKE_REGION_FIELD "can_modify_navmesh"

//---------------------------------------------------------------------------
// LLNavMeshSimStateChangeNode
//---------------------------------------------------------------------------

class LLNavMeshSimStateChangeNode : public LLHTTPNode
{
public:
    virtual void post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const;
};

LLHTTPRegistration<LLNavMeshSimStateChangeNode> gHTTPRegistrationNavMeshSimStateChangeNode(SIM_MESSAGE_NAVMESH_STATUS_UPDATE);


//---------------------------------------------------------------------------
// LLAgentStateChangeNode
//---------------------------------------------------------------------------
class LLAgentStateChangeNode : public LLHTTPNode
{
public:
    virtual void post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const;
};

LLHTTPRegistration<LLAgentStateChangeNode> gHTTPRegistrationAgentStateChangeNode(SIM_MESSAGE_AGENT_STATE_UPDATE);

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------

class LinksetsResponder
{
public:
    LinksetsResponder(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested);
    virtual ~LinksetsResponder();

    void handleObjectLinksetsResult(const LLSD &pContent);
    void handleObjectLinksetsError();
    void handleTerrainLinksetsResult(const LLSD &pContent);
    void handleTerrainLinksetsError();

    typedef boost::shared_ptr<LinksetsResponder> ptr_t;

protected:

private:
    void sendCallback();

    typedef enum
    {
        kNotRequested,
        kWaiting,
        kReceivedGood,
        kReceivedError
    } EMessagingState;

    LLPathfindingManager::request_id_t              mRequestId;
    LLPathfindingManager::object_request_callback_t mLinksetsCallback;

    EMessagingState                                 mObjectMessagingState;
    EMessagingState                                 mTerrainMessagingState;

    LLPathfindingObjectListPtr                      mObjectLinksetListPtr;
    LLPathfindingObjectPtr                          mTerrainLinksetPtr;
};

typedef boost::shared_ptr<LinksetsResponder> LinksetsResponderPtr;

//---------------------------------------------------------------------------
// LLPathfindingManager
//---------------------------------------------------------------------------

LLPathfindingManager::LLPathfindingManager():
    mNavMeshMap(),
    mAgentStateSignal()
{
}

LLPathfindingManager::~LLPathfindingManager()
{   
    quitSystem();
}

void LLPathfindingManager::initSystem()
{
    if (LLPathingLib::getInstance() == NULL)
    {
        LLPathingLib::initSystem();
    }
}

void LLPathfindingManager::quitSystem()
{
    if (LLPathingLib::getInstance() != NULL)
    {
        LLPathingLib::quitSystem();
    }
}

bool LLPathfindingManager::isPathfindingViewEnabled() const
{
    return (LLPathingLib::getInstance() != NULL);
}

bool LLPathfindingManager::isPathfindingEnabledForCurrentRegion() const
{
    return isPathfindingEnabledForRegion(getCurrentRegion());
}

bool LLPathfindingManager::isPathfindingEnabledForRegion(LLViewerRegion *pRegion) const
{
    std::string retrieveNavMeshURL = getRetrieveNavMeshURLForRegion(pRegion);
    return !retrieveNavMeshURL.empty();
}

bool LLPathfindingManager::isAllowViewTerrainProperties() const
{
    LLViewerRegion* region = getCurrentRegion();
    return (gAgent.isGodlike() || ((region != NULL) && region->canManageEstate()));
}

LLPathfindingNavMesh::navmesh_slot_t LLPathfindingManager::registerNavMeshListenerForRegion(LLViewerRegion *pRegion, LLPathfindingNavMesh::navmesh_callback_t pNavMeshCallback)
{
    LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pRegion);
    return navMeshPtr->registerNavMeshListener(pNavMeshCallback);
}

void LLPathfindingManager::requestGetNavMeshForRegion(LLViewerRegion *pRegion, bool pIsGetStatusOnly)
{
    LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pRegion);

    if (pRegion == NULL)
    {
        navMeshPtr->handleNavMeshNotEnabled();
    }
    else if (!pRegion->capabilitiesReceived())
    {
        navMeshPtr->handleNavMeshWaitForRegionLoad();
        pRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetNavMeshForRegion, this, _1, pIsGetStatusOnly));
    }
    else if (!isPathfindingEnabledForRegion(pRegion))
    {
        navMeshPtr->handleNavMeshNotEnabled();
    }
    else
    {
        std::string navMeshStatusURL = getNavMeshStatusURLForRegion(pRegion);
        llassert(!navMeshStatusURL.empty());
        navMeshPtr->handleNavMeshCheckVersion();

        U64 regionHandle = pRegion->getHandle();
        std::string coroname = LLCoros::instance().launch("LLPathfindingManager::navMeshStatusRequestCoro",
            boost::bind(&LLPathfindingManager::navMeshStatusRequestCoro, this, navMeshStatusURL, regionHandle, pIsGetStatusOnly));
    }
}

void LLPathfindingManager::requestGetLinksets(request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const
{
    LLPathfindingObjectListPtr emptyLinksetListPtr;
    LLViewerRegion *currentRegion = getCurrentRegion();

    if (currentRegion == NULL)
    {
        pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
    }
    else if (!currentRegion->capabilitiesReceived())
    {
        pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);
        currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetLinksetsForRegion, this, _1, pRequestId, pLinksetsCallback));
    }
    else
    {
        std::string objectLinksetsURL = getRetrieveObjectLinksetsURLForCurrentRegion();
        std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
        if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
        {
            pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
        }
        else
        {
            pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);

            bool doRequestTerrain = isAllowViewTerrainProperties();
            LinksetsResponder::ptr_t linksetsResponderPtr(new LinksetsResponder(pRequestId, pLinksetsCallback, true, doRequestTerrain));

            std::string coroname = LLCoros::instance().launch("LLPathfindingManager::linksetObjectsCoro",
                boost::bind(&LLPathfindingManager::linksetObjectsCoro, this, objectLinksetsURL, linksetsResponderPtr, LLSD()));

            if (doRequestTerrain)
            {
                std::string coroname = LLCoros::instance().launch("LLPathfindingManager::linksetTerrainCoro",
                    boost::bind(&LLPathfindingManager::linksetTerrainCoro, this, terrainLinksetsURL, linksetsResponderPtr, LLSD()));
            }
        }
    }
}

void LLPathfindingManager::requestSetLinksets(request_id_t pRequestId, const LLPathfindingObjectListPtr &pLinksetListPtr, LLPathfindingLinkset::ELinksetUse pLinksetUse, S32 pA, S32 pB, S32 pC, S32 pD, object_request_callback_t pLinksetsCallback) const
{
    LLPathfindingObjectListPtr emptyLinksetListPtr;

    std::string objectLinksetsURL = getChangeObjectLinksetsURLForCurrentRegion();
    std::string terrainLinksetsURL = getTerrainLinksetsURLForCurrentRegion();
    if (objectLinksetsURL.empty() || terrainLinksetsURL.empty())
    {
        pLinksetsCallback(pRequestId, kRequestNotEnabled, emptyLinksetListPtr);
    }
    else if ((pLinksetListPtr == NULL) || pLinksetListPtr->isEmpty())
    {
        pLinksetsCallback(pRequestId, kRequestCompleted, emptyLinksetListPtr);
    }
    else 
    {
        const LLPathfindingLinksetList *linksetList = dynamic_cast<const LLPathfindingLinksetList *>(pLinksetListPtr.get());

        LLSD objectPostData = linksetList->encodeObjectFields(pLinksetUse, pA, pB, pC, pD);
        LLSD terrainPostData;
        if (isAllowViewTerrainProperties())
        {
            terrainPostData = linksetList->encodeTerrainFields(pLinksetUse, pA, pB, pC, pD);
        }

        if (objectPostData.isUndefined() && terrainPostData.isUndefined())
        {
            pLinksetsCallback(pRequestId, kRequestCompleted, emptyLinksetListPtr);
        }
        else
        {
            pLinksetsCallback(pRequestId, kRequestStarted, emptyLinksetListPtr);

            LinksetsResponder::ptr_t linksetsResponderPtr(new LinksetsResponder(pRequestId, pLinksetsCallback, !objectPostData.isUndefined(), !terrainPostData.isUndefined()));

            if (!objectPostData.isUndefined())
            {
                std::string coroname = LLCoros::instance().launch("LLPathfindingManager::linksetObjectsCoro",
                    boost::bind(&LLPathfindingManager::linksetObjectsCoro, this, objectLinksetsURL, linksetsResponderPtr, objectPostData));
            }

            if (!terrainPostData.isUndefined())
            {
                std::string coroname = LLCoros::instance().launch("LLPathfindingManager::linksetTerrainCoro",
                    boost::bind(&LLPathfindingManager::linksetTerrainCoro, this, terrainLinksetsURL, linksetsResponderPtr, terrainPostData));
            }
        }
    }
}

void LLPathfindingManager::requestGetCharacters(request_id_t pRequestId, object_request_callback_t pCharactersCallback) const
{
    LLPathfindingObjectListPtr emptyCharacterListPtr;

    LLViewerRegion *currentRegion = getCurrentRegion();

    if (currentRegion == NULL)
    {
        pCharactersCallback(pRequestId, kRequestNotEnabled, emptyCharacterListPtr);
    }
    else if (!currentRegion->capabilitiesReceived())
    {
        pCharactersCallback(pRequestId, kRequestStarted, emptyCharacterListPtr);
        currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetCharactersForRegion, this, _1, pRequestId, pCharactersCallback));
    }
    else
    {
        std::string charactersURL = getCharactersURLForCurrentRegion();
        if (charactersURL.empty())
        {
            pCharactersCallback(pRequestId, kRequestNotEnabled, emptyCharacterListPtr);
        }
        else
        {
            pCharactersCallback(pRequestId, kRequestStarted, emptyCharacterListPtr);

            std::string coroname = LLCoros::instance().launch("LLPathfindingManager::charactersCoro",
                boost::bind(&LLPathfindingManager::charactersCoro, this, charactersURL, pRequestId, pCharactersCallback));
        }
    }
}

LLPathfindingManager::agent_state_slot_t LLPathfindingManager::registerAgentStateListener(agent_state_callback_t pAgentStateCallback)
{
    return mAgentStateSignal.connect(pAgentStateCallback);
}

void LLPathfindingManager::requestGetAgentState()
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if (currentRegion == NULL)
    {
        mAgentStateSignal(FALSE);
    }
    else
    {
        if (!currentRegion->capabilitiesReceived())
        {
            currentRegion->setCapabilitiesReceivedCallback(boost::bind(&LLPathfindingManager::handleDeferredGetAgentStateForRegion, this, _1));
        }
        else if (!isPathfindingEnabledForRegion(currentRegion))
        {
            mAgentStateSignal(FALSE);
        }
        else
        {
            std::string agentStateURL = getAgentStateURLForRegion(currentRegion);
            llassert(!agentStateURL.empty());

            std::string coroname = LLCoros::instance().launch("LLPathfindingManager::navAgentStateRequestCoro",
                boost::bind(&LLPathfindingManager::navAgentStateRequestCoro, this, agentStateURL));
        }
    }
}

void LLPathfindingManager::requestRebakeNavMesh(rebake_navmesh_callback_t pRebakeNavMeshCallback)
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if (currentRegion == NULL)
    {
        pRebakeNavMeshCallback(false);
    }
    else if (!isPathfindingEnabledForRegion(currentRegion))
    {
        pRebakeNavMeshCallback(false);
    }
    else
    {
        std::string navMeshStatusURL = getNavMeshStatusURLForCurrentRegion();
        llassert(!navMeshStatusURL.empty());

        std::string coroname = LLCoros::instance().launch("LLPathfindingManager::navMeshRebakeCoro",
                boost::bind(&LLPathfindingManager::navMeshRebakeCoro, this, navMeshStatusURL, pRebakeNavMeshCallback));
    }
}

void LLPathfindingManager::handleDeferredGetAgentStateForRegion(const LLUUID &pRegionUUID)
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
    {
        requestGetAgentState();
    }
}

void LLPathfindingManager::handleDeferredGetNavMeshForRegion(const LLUUID &pRegionUUID, bool pIsGetStatusOnly)
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
    {
        requestGetNavMeshForRegion(currentRegion, pIsGetStatusOnly);
    }
}

void LLPathfindingManager::handleDeferredGetLinksetsForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pLinksetsCallback) const
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
    {
        requestGetLinksets(pRequestId, pLinksetsCallback);
    }
}

void LLPathfindingManager::handleDeferredGetCharactersForRegion(const LLUUID &pRegionUUID, request_id_t pRequestId, object_request_callback_t pCharactersCallback) const
{
    LLViewerRegion *currentRegion = getCurrentRegion();

    if ((currentRegion != NULL) && (currentRegion->getRegionID() == pRegionUUID))
    {
        requestGetCharacters(pRequestId, pCharactersCallback);
    }
}

void LLPathfindingManager::navMeshStatusRequestCoro(std::string url, U64 regionHandle, bool isGetStatusOnly)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("NavMeshStatusRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLViewerRegion *region = LLWorld::getInstance()->getRegionFromHandle(regionHandle);
    if (!region)
    {
        LL_WARNS("PathfindingManager") << "Attempting to retrieve navmesh status for region that has gone away." << LL_ENDL;
        return;
    }
    LLUUID regionUUID = region->getRegionID();

    region = NULL;
    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    region = LLWorld::getInstance()->getRegionFromHandle(regionHandle);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLPathfindingNavMeshStatus navMeshStatus(regionUUID);
    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() << 
            ". Building using empty status." << LL_ENDL;
    }
    else
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        navMeshStatus = LLPathfindingNavMeshStatus(regionUUID, result);
    }

    LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(regionUUID);

    if (!navMeshStatus.isValid())
    {
        navMeshPtr->handleNavMeshError();
        return;
    }
    else if (navMeshPtr->hasNavMeshVersion(navMeshStatus))
    {
        navMeshPtr->handleRefresh(navMeshStatus);
        return;
    }
    else if (isGetStatusOnly)
    {
        navMeshPtr->handleNavMeshNewVersion(navMeshStatus);
        return;
    }

    if ((!region) || !region->isAlive())
    {
        LL_WARNS("PathfindingManager") << "About to update navmesh status for region that has gone away." << LL_ENDL;
        navMeshPtr->handleNavMeshNotEnabled();
        return;
    }

    std::string navMeshURL = getRetrieveNavMeshURLForRegion(region);

    if (navMeshURL.empty())
    {
        navMeshPtr->handleNavMeshNotEnabled();
        return;
    }

    navMeshPtr->handleNavMeshStart(navMeshStatus);

    LLSD postData;
    result = httpAdapter->postAndSuspend(httpRequest, navMeshURL, postData);

    U32 navMeshVersion = navMeshStatus.getVersion();

    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". reporting error." << LL_ENDL;
        navMeshPtr->handleNavMeshError(navMeshVersion);
    }
    else
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        navMeshPtr->handleNavMeshResult(result, navMeshVersion);

    }

}

void LLPathfindingManager::navAgentStateRequestCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("NavAgentStateRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool canRebake = false;
    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". Building using empty status." << LL_ENDL;
    }
    else
    {
        llassert(result.has(AGENT_STATE_CAN_REBAKE_REGION_FIELD));
        llassert(result.get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).isBoolean());
        canRebake = result.get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).asBoolean();
    }

    handleAgentState(canRebake);
}

void LLPathfindingManager::navMeshRebakeCoro(std::string url, rebake_navmesh_callback_t rebakeNavMeshCallback)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("NavMeshRebake", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);


    LLSD postData = LLSD::emptyMap();
    postData["command"] = "rebuild";

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    bool success = true;
    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". Rebake failed." << LL_ENDL;
        success = false;
    }
       
    rebakeNavMeshCallback(success);
}

// If called with putData undefined this coroutine will issue a get.  If there 
// is data in putData it will be PUT to the URL.
void LLPathfindingManager::linksetObjectsCoro(std::string url, LinksetsResponder::ptr_t linksetsResponsderPtr, LLSD putData) const
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("LinksetObjects", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result;

    if (putData.isUndefined())
    {
        result = httpAdapter->getAndSuspend(httpRequest, url);
    }
    else 
    {
        result = httpAdapter->putAndSuspend(httpRequest, url, putData);
    }

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". linksetObjects failed." << LL_ENDL;
        linksetsResponsderPtr->handleObjectLinksetsError();
    }
    else
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        linksetsResponsderPtr->handleObjectLinksetsResult(result);
    }
}

// If called with putData undefined this coroutine will issue a GET.  If there 
// is data in putData it will be PUT to the URL.
void LLPathfindingManager::linksetTerrainCoro(std::string url, LinksetsResponder::ptr_t linksetsResponsderPtr, LLSD putData) const
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("LinksetTerrain", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result;

    if (putData.isUndefined())
    {
        result = httpAdapter->getAndSuspend(httpRequest, url);
    }
    else 
    {
        result = httpAdapter->putAndSuspend(httpRequest, url, putData);
    }

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". linksetTerrain failed." << LL_ENDL;
        linksetsResponsderPtr->handleTerrainLinksetsError();
    }
    else
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        linksetsResponsderPtr->handleTerrainLinksetsResult(result);
    }

}

void LLPathfindingManager::charactersCoro(std::string url, request_id_t requestId, object_request_callback_t callback) const
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("LinksetTerrain", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("PathfindingManager") << "HTTP status, " << status.toTerseString() <<
            ". characters failed." << LL_ENDL;

        LLPathfindingObjectListPtr characterListPtr = LLPathfindingObjectListPtr(new LLPathfindingCharacterList());
        callback(requestId, LLPathfindingManager::kRequestError, characterListPtr);
    }
    else
    {
        result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);
        LLPathfindingObjectListPtr characterListPtr = LLPathfindingObjectListPtr(new LLPathfindingCharacterList(result));
        callback(requestId, LLPathfindingManager::kRequestCompleted, characterListPtr);
    }
}

void LLPathfindingManager::handleNavMeshStatusUpdate(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
    LLPathfindingNavMeshPtr navMeshPtr = getNavMeshForRegion(pNavMeshStatus.getRegionUUID());

    if (!pNavMeshStatus.isValid())
    {
        navMeshPtr->handleNavMeshError();
    }
    else
    {
        navMeshPtr->handleNavMeshNewVersion(pNavMeshStatus);
    }
}

void LLPathfindingManager::handleAgentState(BOOL pCanRebakeRegion) 
{
    mAgentStateSignal(pCanRebakeRegion);
}

LLPathfindingNavMeshPtr LLPathfindingManager::getNavMeshForRegion(const LLUUID &pRegionUUID)
{
    LLPathfindingNavMeshPtr navMeshPtr;
    NavMeshMap::iterator navMeshIter = mNavMeshMap.find(pRegionUUID);
    if (navMeshIter == mNavMeshMap.end())
    {
        navMeshPtr = LLPathfindingNavMeshPtr(new LLPathfindingNavMesh(pRegionUUID));
        mNavMeshMap.insert(std::pair<LLUUID, LLPathfindingNavMeshPtr>(pRegionUUID, navMeshPtr));
    }
    else
    {
        navMeshPtr = navMeshIter->second;
    }

    return navMeshPtr;
}

LLPathfindingNavMeshPtr LLPathfindingManager::getNavMeshForRegion(LLViewerRegion *pRegion)
{
    LLUUID regionUUID;
    if (pRegion != NULL)
    {
        regionUUID = pRegion->getRegionID();
    }

    return getNavMeshForRegion(regionUUID);
}

std::string LLPathfindingManager::getNavMeshStatusURLForCurrentRegion() const
{
    return getNavMeshStatusURLForRegion(getCurrentRegion());
}

std::string LLPathfindingManager::getNavMeshStatusURLForRegion(LLViewerRegion *pRegion) const
{
    return getCapabilityURLForRegion(pRegion, CAP_SERVICE_NAVMESH_STATUS);
}

std::string LLPathfindingManager::getRetrieveNavMeshURLForRegion(LLViewerRegion *pRegion) const
{
    return getCapabilityURLForRegion(pRegion, CAP_SERVICE_RETRIEVE_NAVMESH);
}

std::string LLPathfindingManager::getRetrieveObjectLinksetsURLForCurrentRegion() const
{
    return getCapabilityURLForCurrentRegion(CAP_SERVICE_GET_OBJECT_LINKSETS);
}

std::string LLPathfindingManager::getChangeObjectLinksetsURLForCurrentRegion() const
{
    return getCapabilityURLForCurrentRegion(CAP_SERVICE_SET_OBJECT_LINKSETS);
}

std::string LLPathfindingManager::getTerrainLinksetsURLForCurrentRegion() const
{
    return getCapabilityURLForCurrentRegion(CAP_SERVICE_TERRAIN_LINKSETS);
}

std::string LLPathfindingManager::getCharactersURLForCurrentRegion() const
{
    return getCapabilityURLForCurrentRegion(CAP_SERVICE_CHARACTERS);
}

std::string LLPathfindingManager::getAgentStateURLForRegion(LLViewerRegion *pRegion) const
{
    return getCapabilityURLForRegion(pRegion, CAP_SERVICE_AGENT_STATE);
}

std::string LLPathfindingManager::getCapabilityURLForCurrentRegion(const std::string &pCapabilityName) const
{
    return getCapabilityURLForRegion(getCurrentRegion(), pCapabilityName);
}

std::string LLPathfindingManager::getCapabilityURLForRegion(LLViewerRegion *pRegion, const std::string &pCapabilityName) const
{
    std::string capabilityURL("");

    if (pRegion != NULL)
    {
        capabilityURL = pRegion->getCapability(pCapabilityName);
    }

    if (capabilityURL.empty())
    {
        LL_WARNS() << "cannot find capability '" << pCapabilityName << "' for current region '"
            << ((pRegion != NULL) ? pRegion->getName() : "<null>") << "'" << LL_ENDL;
    }

    return capabilityURL;
}

LLViewerRegion *LLPathfindingManager::getCurrentRegion() const
{
    return gAgent.getRegion();
}

//---------------------------------------------------------------------------
// LLNavMeshSimStateChangeNode
//---------------------------------------------------------------------------

void LLNavMeshSimStateChangeNode::post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const
{
    llassert(pInput.has(SIM_MESSAGE_BODY_FIELD));
    llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).isMap());
    LLPathfindingNavMeshStatus navMeshStatus(pInput.get(SIM_MESSAGE_BODY_FIELD));
    LLPathfindingManager::getInstance()->handleNavMeshStatusUpdate(navMeshStatus);
}

//---------------------------------------------------------------------------
// LLAgentStateChangeNode
//---------------------------------------------------------------------------

void LLAgentStateChangeNode::post(ResponsePtr pResponse, const LLSD &pContext, const LLSD &pInput) const
{
    llassert(pInput.has(SIM_MESSAGE_BODY_FIELD));
    llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).isMap());
    llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).has(AGENT_STATE_CAN_REBAKE_REGION_FIELD));
    llassert(pInput.get(SIM_MESSAGE_BODY_FIELD).get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).isBoolean());
    BOOL canRebakeRegion = pInput.get(SIM_MESSAGE_BODY_FIELD).get(AGENT_STATE_CAN_REBAKE_REGION_FIELD).asBoolean();
    
    LLPathfindingManager::getInstance()->handleAgentState(canRebakeRegion);
}

//---------------------------------------------------------------------------
// LinksetsResponder
//---------------------------------------------------------------------------
LinksetsResponder::LinksetsResponder(LLPathfindingManager::request_id_t pRequestId, LLPathfindingManager::object_request_callback_t pLinksetsCallback, bool pIsObjectRequested, bool pIsTerrainRequested)
    : mRequestId(pRequestId),
    mLinksetsCallback(pLinksetsCallback),
    mObjectMessagingState(pIsObjectRequested ? kWaiting : kNotRequested),
    mTerrainMessagingState(pIsTerrainRequested ? kWaiting : kNotRequested),
    mObjectLinksetListPtr(),
    mTerrainLinksetPtr()
{
}

LinksetsResponder::~LinksetsResponder()
{
}

void LinksetsResponder::handleObjectLinksetsResult(const LLSD &pContent)
{
    mObjectLinksetListPtr = LLPathfindingObjectListPtr(new LLPathfindingLinksetList(pContent));

    mObjectMessagingState = kReceivedGood;
    if (mTerrainMessagingState != kWaiting)
    {
        sendCallback();
    }
}

void LinksetsResponder::handleObjectLinksetsError()
{
    LL_WARNS() << "LinksetsResponder object linksets error" << LL_ENDL;
    mObjectMessagingState = kReceivedError;
    if (mTerrainMessagingState != kWaiting)
    {
        sendCallback();
    }
}

void LinksetsResponder::handleTerrainLinksetsResult(const LLSD &pContent)
{
    mTerrainLinksetPtr = LLPathfindingObjectPtr(new LLPathfindingLinkset(pContent));

    mTerrainMessagingState = kReceivedGood;
    if (mObjectMessagingState != kWaiting)
    {
        sendCallback();
    }
}

void LinksetsResponder::handleTerrainLinksetsError()
{
    LL_WARNS() << "LinksetsResponder terrain linksets error" << LL_ENDL;
    mTerrainMessagingState = kReceivedError;
    if (mObjectMessagingState != kWaiting)
    {
        sendCallback();
    }
}

void LinksetsResponder::sendCallback()
{
    llassert(mObjectMessagingState != kWaiting);
    llassert(mTerrainMessagingState != kWaiting);
    LLPathfindingManager::ERequestStatus requestStatus =
        ((((mObjectMessagingState == kReceivedGood) || (mObjectMessagingState == kNotRequested)) &&
          ((mTerrainMessagingState == kReceivedGood) || (mTerrainMessagingState == kNotRequested))) ?
         LLPathfindingManager::kRequestCompleted : LLPathfindingManager::kRequestError);

    if (mObjectMessagingState != kReceivedGood)
    {
        mObjectLinksetListPtr = LLPathfindingObjectListPtr(new LLPathfindingLinksetList());
    }

    if (mTerrainMessagingState == kReceivedGood)
    {
        mObjectLinksetListPtr->update(mTerrainLinksetPtr);
    }

    mLinksetsCallback(mRequestId, requestStatus, mObjectLinksetListPtr);
}
