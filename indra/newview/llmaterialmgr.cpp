/**
 * @file llmaterialmgr.cpp
 * @brief Material manager
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#include "llsdserialize.h"
#include "llsdutil.h"

#include "llagent.h"
#include "llcallbacklist.h"
#include "llmaterialmgr.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"

/**
 * Materials cap parameters
 */

#define MATERIALS_CAPABILITY_NAME                 "RenderMaterials"

#define MATERIALS_CAP_ZIP_FIELD                   "Zipped"

#define MATERIALS_CAP_FULL_PER_FACE_FIELD         "FullMaterialsPerFace"
#define MATERIALS_CAP_FACE_FIELD                  "Face"
#define MATERIALS_CAP_MATERIAL_FIELD              "Material"
#define MATERIALS_CAP_OBJECT_ID_FIELD             "ID"
#define MATERIALS_CAP_MATERIAL_ID_FIELD           "MaterialID"
#define SIM_FEATURE_MAX_MATERIALS_PER_TRANSACTION "MaxMaterialsPerTransaction"

#define MATERIALS_GET_MAX_ENTRIES                 50
#define MATERIALS_GET_TIMEOUT                     (60.f * 20)
#define MATERIALS_POST_MAX_ENTRIES                50
#define MATERIALS_POST_TIMEOUT                    (60.f * 5)
#define MATERIALS_PUT_THROTTLE_SECS               1.f
#define MATERIALS_PUT_MAX_ENTRIES                 50

/**
 * LLMaterialsResponder helper class
 */

class LLMaterialsResponder : public LLHTTPClient::Responder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	LLMaterialsResponder(const std::string& pMethod, const std::string& pCapabilityURL, CallbackFunction pCallback);
	virtual ~LLMaterialsResponder();

	virtual void httpSuccess();
	virtual void httpFailure();

private:
	std::string      mMethod;
	std::string      mCapabilityURL;
	CallbackFunction mCallback;
};

LLMaterialsResponder::LLMaterialsResponder(const std::string& pMethod, const std::string& pCapabilityURL, CallbackFunction pCallback)
	: LLHTTPClient::Responder()
	, mMethod(pMethod)
	, mCapabilityURL(pCapabilityURL)
	, mCallback(pCallback)
{
}

LLMaterialsResponder::~LLMaterialsResponder()
{
}

void LLMaterialsResponder::httpSuccess()
{
	const LLSD& pContent = getContent();

	LL_DEBUGS("Materials") << LL_ENDL;
	mCallback(true, pContent);
}

void LLMaterialsResponder::httpFailure()
{
	U32 pStatus = (U32) getStatus();
	const std::string& pReason = getReason();
	
	LL_WARNS("Materials")
		<< "\n--------------------------------------------------------------------------\n"
		<< mMethod << " Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "'\n  with url '" << mCapabilityURL	<< "' because " << pReason 
		<< "\n--------------------------------------------------------------------------"
		<< LL_ENDL;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}

/**
 * LLMaterialMgr class
 */

LLMaterialMgr::LLMaterialMgr()
{
	mMaterials.insert(std::pair<LLMaterialID, LLMaterialPtr>(LLMaterialID::null, LLMaterialPtr(NULL)));
	gIdleCallbacks.addFunction(&LLMaterialMgr::onIdle, NULL);
	LLWorld::instance().setRegionRemovedCallback(boost::bind(&LLMaterialMgr::onRegionRemoved, this, _1));
}

LLMaterialMgr::~LLMaterialMgr()
{
	gIdleCallbacks.deleteFunction(&LLMaterialMgr::onIdle, NULL);
}

bool LLMaterialMgr::isGetPending(const LLUUID& region_id, const LLMaterialID& material_id) const
{
	get_pending_map_t::const_iterator itPending = mGetPending.find(pending_material_t(region_id, material_id));
	return (mGetPending.end() != itPending) && (LLFrameTimer::getTotalSeconds() < itPending->second + MATERIALS_POST_TIMEOUT);
}

void LLMaterialMgr::markGetPending(const LLUUID& region_id, const LLMaterialID& material_id)
{
	get_pending_map_t::iterator itPending = mGetPending.find(pending_material_t(region_id, material_id));
	if (mGetPending.end() == itPending)
	{
		mGetPending.insert(std::pair<pending_material_t, F64>(pending_material_t(region_id, material_id), LLFrameTimer::getTotalSeconds()));
	}
	else
	{
		itPending->second = LLFrameTimer::getTotalSeconds();
	}
}

const LLMaterialPtr LLMaterialMgr::get(const LLUUID& region_id, const LLMaterialID& material_id)
{
	LL_DEBUGS("Materials") << "region " << region_id << " material id " << material_id << LL_ENDL;
	LLMaterialPtr material;
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (mMaterials.end() != itMaterial)
	{
		material = itMaterial->second;
		LL_DEBUGS("Materials") << " found material " << LL_ENDL;
	}
	else
	{
		if (!isGetPending(region_id, material_id))
		{
			LL_DEBUGS("Materials") << " material pending " << material_id << LL_ENDL;
			get_queue_t::iterator itQueue = mGetQueue.find(region_id);
			if (mGetQueue.end() == itQueue)
			{
				LL_DEBUGS("Materials") << "mGetQueue add region " << region_id << " pending " << material_id << LL_ENDL;
				std::pair<get_queue_t::iterator, bool> ret = mGetQueue.insert(std::pair<LLUUID, material_queue_t>(region_id, material_queue_t()));
				itQueue = ret.first;
			}
			itQueue->second.insert(material_id);
			markGetPending(region_id, material_id);
		}
		LL_DEBUGS("Materials") << " returning empty material " << LL_ENDL;
		material = LLMaterialPtr();
	}
	return material;
}

boost::signals2::connection LLMaterialMgr::get(const LLUUID& region_id, const LLMaterialID& material_id, LLMaterialMgr::get_callback_t::slot_type cb)
{
	boost::signals2::connection connection;
	
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (itMaterial != mMaterials.end())
	{
		LL_DEBUGS("Materials") << "region " << region_id << " found materialid " << material_id << LL_ENDL;
		get_callback_t signal;
		signal.connect(cb);
		signal(material_id, itMaterial->second);
		connection = boost::signals2::connection();
	}
	else
	{
		if (!isGetPending(region_id, material_id))
		{
			get_queue_t::iterator itQueue = mGetQueue.find(region_id);
			if (mGetQueue.end() == itQueue)
			{
				LL_DEBUGS("Materials") << "mGetQueue inserting region "<<region_id << LL_ENDL;
				std::pair<get_queue_t::iterator, bool> ret = mGetQueue.insert(std::pair<LLUUID, material_queue_t>(region_id, material_queue_t()));
				itQueue = ret.first;
			}
			LL_DEBUGS("Materials") << "adding material id " << material_id << LL_ENDL;
			itQueue->second.insert(material_id);
			markGetPending(region_id, material_id);
		}

		get_callback_map_t::iterator itCallback = mGetCallbacks.find(material_id);
		if (itCallback == mGetCallbacks.end())
		{
			std::pair<get_callback_map_t::iterator, bool> ret = mGetCallbacks.insert(std::pair<LLMaterialID, get_callback_t*>(material_id, new get_callback_t()));
			itCallback = ret.first;
		}
		connection = itCallback->second->connect(cb);;
	}
	
	return connection;
}

boost::signals2::connection LLMaterialMgr::getTE(const LLUUID& region_id, const LLMaterialID& material_id, U32 te, LLMaterialMgr::get_callback_te_t::slot_type cb)
{
	boost::signals2::connection connection;

	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (itMaterial != mMaterials.end())
	{
		LL_DEBUGS("Materials") << "region " << region_id << " found materialid " << material_id << LL_ENDL;
		get_callback_te_t signal;
		signal.connect(cb);
		signal(material_id, itMaterial->second, te);
		connection = boost::signals2::connection();
	}
	else
	{
		if (!isGetPending(region_id, material_id))
		{
			get_queue_t::iterator itQueue = mGetQueue.find(region_id);
			if (mGetQueue.end() == itQueue)
			{
				LL_DEBUGS("Materials") << "mGetQueue inserting region "<<region_id << LL_ENDL;
				std::pair<get_queue_t::iterator, bool> ret = mGetQueue.insert(std::pair<LLUUID, material_queue_t>(region_id, material_queue_t()));
				itQueue = ret.first;
			}
			LL_DEBUGS("Materials") << "adding material id " << material_id << LL_ENDL;
			itQueue->second.insert(material_id);
			markGetPending(region_id, material_id);
		}

		TEMaterialPair te_mat_pair;
		te_mat_pair.te = te;
		te_mat_pair.materialID = material_id;

		get_callback_te_map_t::iterator itCallback = mGetTECallbacks.find(te_mat_pair);
		if (itCallback == mGetTECallbacks.end())
		{
			std::pair<get_callback_te_map_t::iterator, bool> ret = mGetTECallbacks.insert(std::pair<TEMaterialPair, get_callback_te_t*>(te_mat_pair, new get_callback_te_t()));
			itCallback = ret.first;
		}
		connection = itCallback->second->connect(cb);
	}

	return connection;
}

bool LLMaterialMgr::isGetAllPending(const LLUUID& region_id) const
{
	getall_pending_map_t::const_iterator itPending = mGetAllPending.find(region_id);
	return (mGetAllPending.end() != itPending) && (LLFrameTimer::getTotalSeconds() < itPending->second + MATERIALS_GET_TIMEOUT);
}

void LLMaterialMgr::getAll(const LLUUID& region_id)
{
	if (!isGetAllPending(region_id))
	{
		LL_DEBUGS("Materials") << "queuing for region " << region_id << LL_ENDL;
		mGetAllQueue.insert(region_id);
	}
	else
	{
		LL_DEBUGS("Materials") << "already pending for region " << region_id << LL_ENDL;
	}
}

boost::signals2::connection LLMaterialMgr::getAll(const LLUUID& region_id, LLMaterialMgr::getall_callback_t::slot_type cb)
{
	if (!isGetAllPending(region_id))
	{
		mGetAllQueue.insert(region_id);
	}

	getall_callback_map_t::iterator itCallback = mGetAllCallbacks.find(region_id);
	if (mGetAllCallbacks.end() == itCallback)
	{
		std::pair<getall_callback_map_t::iterator, bool> ret = mGetAllCallbacks.insert(std::pair<LLUUID, getall_callback_t*>(region_id, new getall_callback_t()));
		itCallback = ret.first;
	}
	return itCallback->second->connect(cb);;
}

void LLMaterialMgr::put(const LLUUID& object_id, const U8 te, const LLMaterial& material)
{
	put_queue_t::iterator itQueue = mPutQueue.find(object_id);
	if (mPutQueue.end() == itQueue)
	{
		LL_DEBUGS("Materials") << "mPutQueue insert object " << object_id << LL_ENDL;
		mPutQueue.insert(std::pair<LLUUID, facematerial_map_t>(object_id, facematerial_map_t()));
		itQueue = mPutQueue.find(object_id);
	}

	facematerial_map_t::iterator itFace = itQueue->second.find(te);
	if (itQueue->second.end() == itFace)
	{
		itQueue->second.insert(std::pair<U8, LLMaterial>(te, material));
	}
	else
	{
		itFace->second = material;
	}
}

void LLMaterialMgr::remove(const LLUUID& object_id, const U8 te)
{
	put(object_id, te, LLMaterial::null);
}

const LLMaterialPtr LLMaterialMgr::setMaterial(const LLUUID& region_id, const LLMaterialID& material_id, const LLSD& material_data)
{
	LL_DEBUGS("Materials") << "region " << region_id << " material id " << material_id << LL_ENDL;
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (mMaterials.end() == itMaterial)
	{
		LL_DEBUGS("Materials") << "new material" << LL_ENDL;
		LLMaterialPtr newMaterial(new LLMaterial(material_data));
		std::pair<material_map_t::const_iterator, bool> ret = mMaterials.insert(std::pair<LLMaterialID, LLMaterialPtr>(material_id, newMaterial));
		itMaterial = ret.first;
	}

	TEMaterialPair te_mat_pair;
	te_mat_pair.materialID = material_id;

	U32 i = 0;
	while (i < LLTEContents::MAX_TES)
	{
		te_mat_pair.te = i++;
		get_callback_te_map_t::iterator itCallbackTE = mGetTECallbacks.find(te_mat_pair);
		if (itCallbackTE != mGetTECallbacks.end())
		{
			(*itCallbackTE->second)(material_id, itMaterial->second, te_mat_pair.te);
			delete itCallbackTE->second;
			mGetTECallbacks.erase(itCallbackTE);
		}
	}

	get_callback_map_t::iterator itCallback = mGetCallbacks.find(material_id);
	if (itCallback != mGetCallbacks.end())
	{
		(*itCallback->second)(material_id, itMaterial->second);

		delete itCallback->second;
		mGetCallbacks.erase(itCallback);
	}

	mGetPending.erase(pending_material_t(region_id, material_id));

	return itMaterial->second;
}

void LLMaterialMgr::onGetResponse(bool success, const LLSD& content, const LLUUID& region_id)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		LL_WARNS("Materials")<< "failed"<<LL_ENDL;
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content[MATERIALS_CAP_ZIP_FIELD].isBinary());

	LLSD::Binary content_binary = content[MATERIALS_CAP_ZIP_FIELD].asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_WARNS("Materials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}

	llassert(response_data.isArray());
	LL_DEBUGS("Materials") << "response has "<< response_data.size() << " materials" << LL_ENDL;
	for (LLSD::array_const_iterator itMaterial = response_data.beginArray(); itMaterial != response_data.endArray(); ++itMaterial)
	{
		const LLSD& material_data = *itMaterial;
		llassert(material_data.isMap());

		llassert(material_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(material_data[MATERIALS_CAP_OBJECT_ID_FIELD].isBinary());
		LLMaterialID material_id(material_data[MATERIALS_CAP_OBJECT_ID_FIELD].asBinary());

		llassert(material_data.has(MATERIALS_CAP_MATERIAL_FIELD));
		llassert(material_data[MATERIALS_CAP_MATERIAL_FIELD].isMap());
			
		setMaterial(region_id, material_id, material_data[MATERIALS_CAP_MATERIAL_FIELD]);
	}
}

void LLMaterialMgr::onGetAllResponse(bool success, const LLSD& content, const LLUUID& region_id)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		LL_WARNS("Materials")<< "failed"<<LL_ENDL;
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content[MATERIALS_CAP_ZIP_FIELD].isBinary());

	LLSD::Binary content_binary = content[MATERIALS_CAP_ZIP_FIELD].asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_WARNS("Materials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}

	get_queue_t::iterator itQueue = mGetQueue.find(region_id);
	material_map_t materials;

	llassert(response_data.isArray());
	LL_DEBUGS("Materials") << "response has "<< response_data.size() << " materials" << LL_ENDL;
	for (LLSD::array_const_iterator itMaterial = response_data.beginArray(); itMaterial != response_data.endArray(); ++itMaterial)
	{
		const LLSD& material_data = *itMaterial;
		llassert(material_data.isMap());

		llassert(material_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(material_data[MATERIALS_CAP_OBJECT_ID_FIELD].isBinary());
		LLMaterialID material_id(material_data[MATERIALS_CAP_OBJECT_ID_FIELD].asBinary());
		if (mGetQueue.end() != itQueue)
		{
			itQueue->second.erase(material_id);
		}

		llassert(material_data.has(MATERIALS_CAP_MATERIAL_FIELD));
		llassert(material_data[MATERIALS_CAP_MATERIAL_FIELD].isMap());
		LLMaterialPtr material = setMaterial(region_id, material_id, material_data[MATERIALS_CAP_MATERIAL_FIELD]);
		
		materials[material_id] = material;
	}

	getall_callback_map_t::iterator itCallback = mGetAllCallbacks.find(region_id);
	if (itCallback != mGetAllCallbacks.end())
	{
		(*itCallback->second)(region_id, materials);

		delete itCallback->second;
		mGetAllCallbacks.erase(itCallback);
	}

	if ( (mGetQueue.end() != itQueue) && (itQueue->second.empty()) )
	{
		mGetQueue.erase(itQueue);
	}

	LL_DEBUGS("Materials")<< "recording that getAll has been done for region id " << region_id << LL_ENDL;	
	mGetAllRequested.insert(region_id); // prevents subsequent getAll requests for this region
	mGetAllPending.erase(region_id);	// Invalidates region_id
}

void LLMaterialMgr::onPutResponse(bool success, const LLSD& content)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		LL_WARNS("Materials")<< "failed"<<LL_ENDL;
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content[MATERIALS_CAP_ZIP_FIELD].isBinary());

	LLSD::Binary content_binary = content[MATERIALS_CAP_ZIP_FIELD].asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_WARNS("Materials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}
	else
	{
		llassert(response_data.isArray());
		LL_DEBUGS("Materials") << "response has "<< response_data.size() << " materials" << LL_ENDL;
		for (LLSD::array_const_iterator faceIter = response_data.beginArray(); faceIter != response_data.endArray(); ++faceIter)
		{
#           ifndef LL_RELEASE_FOR_DOWNLOAD
			const LLSD& face_data = *faceIter; // conditional to avoid unused variable warning
#           endif
			llassert(face_data.isMap());

			llassert(face_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
			llassert(face_data[MATERIALS_CAP_OBJECT_ID_FIELD].isInteger());
			// U32 local_id = face_data[MATERIALS_CAP_OBJECT_ID_FIELD].asInteger();

			llassert(face_data.has(MATERIALS_CAP_FACE_FIELD));
			llassert(face_data[MATERIALS_CAP_FACE_FIELD].isInteger());
			// S32 te = face_data[MATERIALS_CAP_FACE_FIELD].asInteger();

			llassert(face_data.has(MATERIALS_CAP_MATERIAL_ID_FIELD));
			llassert(face_data[MATERIALS_CAP_MATERIAL_ID_FIELD].isBinary());
			// LLMaterialID material_id(face_data[MATERIALS_CAP_MATERIAL_ID_FIELD].asBinary());

			// *TODO: do we really still need to process this?
		}
	}
}

static LLTrace::BlockTimerStatHandle FTM_MATERIALS_IDLE("Idle Materials");

void LLMaterialMgr::onIdle(void*)
{
	LL_RECORD_BLOCK_TIME(FTM_MATERIALS_IDLE);

	LLMaterialMgr* instancep = LLMaterialMgr::getInstance();

	if (!instancep->mGetQueue.empty())
	{
		instancep->processGetQueue();
	}

	if (!instancep->mGetAllQueue.empty())
	{
		instancep->processGetAllQueue();
	}

	if (!instancep->mPutQueue.empty())
	{
		instancep->processPutQueue();
	}
}

void LLMaterialMgr::processGetQueue()
{
	get_queue_t::iterator loopRegionQueue = mGetQueue.begin();
	while (mGetQueue.end() != loopRegionQueue)
	{
		get_queue_t::iterator itRegionQueue = loopRegionQueue++;

		const LLUUID& region_id = itRegionQueue->first;
		if (isGetAllPending(region_id))
		{
			continue;
		}

		LLViewerRegion* regionp = LLWorld::instance().getRegionFromID(region_id);
		if (!regionp)
		{
			LL_WARNS("Materials") << "Unknown region with id " << region_id.asString() << LL_ENDL;
			mGetQueue.erase(itRegionQueue);
			continue;
		}
		else if (!regionp->capabilitiesReceived() || regionp->materialsCapThrottled())
		{
			continue;
		}
		else if (mGetAllRequested.end() == mGetAllRequested.find(region_id))
		{
			LL_DEBUGS("Materials") << "calling getAll for " << regionp->getName() << LL_ENDL;
			getAll(region_id);
			continue;
		}

		const std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
		if (capURL.empty())
		{
			LL_WARNS("Materials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on region '" << regionp->getName() << "'" << LL_ENDL;
			mGetQueue.erase(itRegionQueue);
			continue;
		}

		LLSD materialsData = LLSD::emptyArray();

		material_queue_t& materials = itRegionQueue->second;
		U32 max_entries = regionp->getMaxMaterialsPerTransaction();
		material_queue_t::iterator loopMaterial = materials.begin();
		while ( (materials.end() != loopMaterial) && (materialsData.size() < max_entries) )
		{
			material_queue_t::iterator itMaterial = loopMaterial++;
			materialsData.append((*itMaterial).asLLSD());
			materials.erase(itMaterial);
			markGetPending(region_id, *itMaterial);
		}
		if (materials.empty())
		{
			mGetQueue.erase(itRegionQueue);
		}
		
		std::string materialString = zip_llsd(materialsData);

		S32 materialSize = materialString.size();
		if (materialSize <= 0)
		{
			LL_ERRS("Materials") << "cannot zip LLSD binary content" << LL_ENDL;
			return;
		}

		LLSD::Binary materialBinary;
		materialBinary.resize(materialSize);
		memcpy(materialBinary.data(), materialString.data(), materialSize);

		LLSD postData = LLSD::emptyMap();
		postData[MATERIALS_CAP_ZIP_FIELD] = materialBinary;

		LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("POST", capURL, boost::bind(&LLMaterialMgr::onGetResponse, this, _1, _2, region_id));
		LL_DEBUGS("Materials") << "POSTing to region '" << regionp->getName() << "' at '"<< capURL << " for " << materialsData.size() << " materials." 
			<< "\ndata: " << ll_pretty_print_sd(materialsData) << LL_ENDL;
		LLHTTPClient::post(capURL, postData, materialsResponder);
		regionp->resetMaterialsCapThrottle();
	}
}

void LLMaterialMgr::processGetAllQueue()
{
	getall_queue_t::iterator loopRegion = mGetAllQueue.begin();
	while (mGetAllQueue.end() != loopRegion)
	{
		getall_queue_t::iterator itRegion = loopRegion++;

		const LLUUID& region_id = *itRegion;
		LLViewerRegion* regionp = LLWorld::instance().getRegionFromID(region_id);
		if (regionp == NULL)
		{
			LL_WARNS("Materials") << "Unknown region with id " << region_id.asString() << LL_ENDL;
			clearGetQueues(region_id);		// Invalidates region_id
			continue;
		}
		else if (!regionp->capabilitiesReceived() || regionp->materialsCapThrottled())
		{
			continue;
		}

		std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
		if (capURL.empty())
		{
			LL_WARNS("Materials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on the current region '" << regionp->getName() << "'" << LL_ENDL;
			clearGetQueues(region_id);		// Invalidates region_id
			continue;
		}

		LL_DEBUGS("Materials") << "GET all for region " << region_id << "url " << capURL << LL_ENDL;
		LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("GET", capURL, boost::bind(&LLMaterialMgr::onGetAllResponse, this, _1, _2, *itRegion));
		LLHTTPClient::get(capURL, materialsResponder);
		regionp->resetMaterialsCapThrottle();
		mGetAllPending.insert(std::pair<LLUUID, F64>(region_id, LLFrameTimer::getTotalSeconds()));
		mGetAllQueue.erase(itRegion);	// Invalidates region_id
	}
}

void LLMaterialMgr::processPutQueue()
{
	typedef std::map<LLViewerRegion*, LLSD> regionput_request_map;
	regionput_request_map requests;

	put_queue_t::iterator loopQueue = mPutQueue.begin();
	while (mPutQueue.end() != loopQueue)
	{
		put_queue_t::iterator itQueue = loopQueue++;

		const LLUUID& object_id = itQueue->first;
		const LLViewerObject* objectp = gObjectList.findObject(object_id);
		if ( !objectp )
		{
			LL_WARNS("Materials") << "Object is NULL" << LL_ENDL;
			mPutQueue.erase(itQueue);
		}
		else
		{
			LLViewerRegion* regionp = objectp->getRegion();
			if ( !regionp )
		{
				LL_WARNS("Materials") << "Object region is NULL" << LL_ENDL;
				mPutQueue.erase(itQueue);
		}
			else if ( regionp->capabilitiesReceived() && !regionp->materialsCapThrottled())
			{
		LLSD& facesData = requests[regionp];

		facematerial_map_t& face_map = itQueue->second;
				U32 max_entries = regionp->getMaxMaterialsPerTransaction();
		facematerial_map_t::iterator itFace = face_map.begin();
				while ( (face_map.end() != itFace) && (facesData.size() < max_entries) )
		{
			LLSD faceData = LLSD::emptyMap();
			faceData[MATERIALS_CAP_FACE_FIELD] = static_cast<LLSD::Integer>(itFace->first);
			faceData[MATERIALS_CAP_OBJECT_ID_FIELD] = static_cast<LLSD::Integer>(objectp->getLocalID());
			if (!itFace->second.isNull())
			{
				faceData[MATERIALS_CAP_MATERIAL_FIELD] = itFace->second.asLLSD();
			}
			facesData.append(faceData);
			face_map.erase(itFace++);
		}
		if (face_map.empty())
		{
			mPutQueue.erase(itQueue);
		}
	}
		}
	}

	for (regionput_request_map::const_iterator itRequest = requests.begin(); itRequest != requests.end(); ++itRequest)
	{
		LLViewerRegion* regionp = itRequest->first;
		std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
		if (capURL.empty())
		{
			LL_WARNS("Materials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on region '" << regionp->getName() << "'" << LL_ENDL;
			continue;
		}

		LLSD materialsData = LLSD::emptyMap();
		materialsData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = itRequest->second;

		std::string materialString = zip_llsd(materialsData);

		S32 materialSize = materialString.size();

		if (materialSize > 0)
		{
			LLSD::Binary materialBinary;
			materialBinary.resize(materialSize);
			memcpy(materialBinary.data(), materialString.data(), materialSize);

			LLSD putData = LLSD::emptyMap();
			putData[MATERIALS_CAP_ZIP_FIELD] = materialBinary;

			LL_DEBUGS("Materials") << "put for " << itRequest->second.size() << " faces to region " << itRequest->first->getName() << LL_ENDL;
			LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("PUT", capURL, boost::bind(&LLMaterialMgr::onPutResponse, this, _1, _2));
			LLHTTPClient::put(capURL, putData, materialsResponder);
			regionp->resetMaterialsCapThrottle();
		}
		else
		{
			LL_ERRS("debugMaterials") << "cannot zip LLSD binary content" << LL_ENDL;
		}
	}
}

void LLMaterialMgr::clearGetQueues(const LLUUID& region_id)
{
	mGetQueue.erase(region_id);
	for (get_pending_map_t::iterator itPending = mGetPending.begin(); itPending != mGetPending.end();)
	{
		if (region_id == itPending->first.first)
		{
			mGetPending.erase(itPending++);
		}
		else
		{
			++itPending;
		}
	}

	mGetAllQueue.erase(region_id);
	mGetAllRequested.erase(region_id);
	mGetAllPending.erase(region_id);
	mGetAllCallbacks.erase(region_id);
}
void LLMaterialMgr::onRegionRemoved(LLViewerRegion* regionp)
{
	clearGetQueues(regionp->getRegionID());
	// Put doesn't need clearing: objects that can't be found will clean up in processPutQueue()
}

