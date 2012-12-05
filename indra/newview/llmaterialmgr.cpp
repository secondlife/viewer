/**
 * @file llmaterialmgr.cpp
 * @brief Material manager
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#define MATERIALS_GET_TIMEOUT                     (60.f * 20)
#define MATERIALS_POST_TIMEOUT                    (60.f * 5)

/**
 * LLMaterialsResponder helper class
 */

class LLMaterialsResponder : public LLHTTPClient::Responder
{
public:
	typedef boost::function<void (bool, const LLSD&)> CallbackFunction;

	LLMaterialsResponder(const std::string& pMethod, const std::string& pCapabilityURL, CallbackFunction pCallback);
	virtual ~LLMaterialsResponder();

	virtual void result(const LLSD& pContent);
	virtual void error(U32 pStatus, const std::string& pReason);

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

void LLMaterialsResponder::result(const LLSD& pContent)
{
	mCallback(true, pContent);
}

void LLMaterialsResponder::error(U32 pStatus, const std::string& pReason)
{
	LL_WARNS("debugMaterials") << "--------------------------------------------------------------------------" << LL_ENDL;
	LL_WARNS("debugMaterials") << mMethod << " Error[" << pStatus << "] cannot access cap '" << MATERIALS_CAPABILITY_NAME
		<< "' with url '" << mCapabilityURL	<< "' because " << pReason << LL_ENDL;
	LL_WARNS("debugMaterials") << "--------------------------------------------------------------------------" << LL_ENDL;

	LLSD emptyResult;
	mCallback(false, emptyResult);
}

/**
 * LLMaterialMgr class
 */

LLMaterialMgr::LLMaterialMgr()
{
	gIdleCallbacks.addFunction(&LLMaterialMgr::onIdle, NULL);
}

LLMaterialMgr::~LLMaterialMgr()
{
	gIdleCallbacks.deleteFunction(&LLMaterialMgr::onIdle, NULL);
}

bool LLMaterialMgr::isGetPending(const LLMaterialID& material_id)
{
	get_pending_map_t::const_iterator itPending = mGetPending.find(material_id);
	return (mGetPending.end() != itPending) && (LLFrameTimer::getTotalSeconds() < itPending->second + MATERIALS_POST_TIMEOUT);
}

const LLMaterialPtr LLMaterialMgr::get(const LLMaterialID& material_id)
{
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (itMaterial != mMaterials.end())
	{
		return itMaterial->second;
	}

	if (!isGetPending(material_id))
	{
		mGetQueue.insert(material_id);
	}
	return LLMaterialPtr();
}

boost::signals2::connection LLMaterialMgr::get(const LLMaterialID& material_id, LLMaterialMgr::get_callback_t::slot_type cb)
{
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (itMaterial != mMaterials.end())
	{
		get_callback_t signal;
		signal.connect(cb);
		signal(material_id, itMaterial->second);
		return boost::signals2::connection();
	}

	if (!isGetPending(material_id))
	{
		mGetQueue.insert(material_id);
	}

	get_callback_t* signalp = NULL;
	get_callback_map_t::iterator itCallback = mGetCallbacks.find(material_id);
	if (itCallback == mGetCallbacks.end())
	{
		signalp = new get_callback_t();
		mGetCallbacks.insert(std::pair<LLMaterialID, get_callback_t*>(material_id, signalp));
	}
	else
	{
		signalp = itCallback->second;
	}
	return signalp->connect(cb);;
}

bool LLMaterialMgr::isGetAllPending(const LLUUID& region_id)
{
	getall_pending_map_t::const_iterator itPending = mGetAllPending.find(region_id);
	return (mGetAllPending.end() != itPending) && (LLFrameTimer::getTotalSeconds() < itPending->second + MATERIALS_GET_TIMEOUT);
}

void LLMaterialMgr::getAll(const LLUUID& region_id)
{
	if (!isGetAllPending(region_id))
	{
		mGetAllQueue.insert(region_id);
	}
}

boost::signals2::connection LLMaterialMgr::getAll(const LLUUID& region_id, LLMaterialMgr::getall_callback_t::slot_type cb)
{
	if (!isGetAllPending(region_id))
	{
		mGetAllQueue.insert(region_id);
	}

	getall_callback_t* signalp = NULL;
	getall_callback_map_t::iterator itCallback = mGetAllCallbacks.find(region_id);
	if (mGetAllCallbacks.end() == itCallback)
	{
		signalp = new getall_callback_t();
		mGetAllCallbacks.insert(std::pair<LLUUID, getall_callback_t*>(region_id, signalp));
	}
	else
	{
		signalp = itCallback->second;
	}
	return signalp->connect(cb);;
}

void LLMaterialMgr::put(const LLUUID& object_id, const U8 te, const LLMaterial& material)
{
	put_queue_t::iterator itQueue = mPutQueue.find(object_id);
	if (mPutQueue.end() == itQueue)
	{
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

const LLMaterialPtr LLMaterialMgr::setMaterial(const LLMaterialID& material_id, const LLSD& material_data)
{
	material_map_t::const_iterator itMaterial = mMaterials.find(material_id);
	if (mMaterials.end() == itMaterial)
	{
		LLMaterialPtr material(new LLMaterial(material_data));
		mMaterials[material_id] = material;
		return material;
	}
	return itMaterial->second;
}


void LLMaterialMgr::onGetResponse(bool success, const LLSD& content)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

	LLSD::Binary content_binary = content.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_ERRS("debugMaterials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}
	else
	{
		llassert(response_data.isArray());

		for (LLSD::array_const_iterator itMaterial = response_data.beginArray(); itMaterial != response_data.endArray(); ++itMaterial)
		{
			const LLSD& material_data = *itMaterial;
			llassert(material_data.isMap());

			llassert(material_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
			llassert(material_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).isBinary());
			LLMaterialID material_id(material_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).asBinary());

			llassert(material_data.has(MATERIALS_CAP_MATERIAL_FIELD));
			llassert(material_data.get(MATERIALS_CAP_MATERIAL_FIELD).isMap());
			LLMaterialPtr material = setMaterial(material_id, material_data.get(MATERIALS_CAP_MATERIAL_FIELD));

			get_callback_map_t::iterator itCallback = mGetCallbacks.find(material_id);
			if (itCallback != mGetCallbacks.end())
			{
				(*itCallback->second)(material_id, material);

				delete itCallback->second;
				mGetCallbacks.erase(itCallback);
			}
		}
	}
}

void LLMaterialMgr::onGetAllResponse(bool success, const LLSD& content, const LLUUID& region_id)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

	LLSD::Binary content_binary = content.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_ERRS("debugMaterials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}

	llassert(response_data.isArray());
	material_map_t materials;
	for (LLSD::array_const_iterator itMaterial = response_data.beginArray(); itMaterial != response_data.endArray(); ++itMaterial)
	{
		const LLSD& material_data = *itMaterial;
		llassert(material_data.isMap());

		llassert(material_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
		llassert(material_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).isBinary());
		LLMaterialID material_id(material_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).asBinary());

		llassert(material_data.has(MATERIALS_CAP_MATERIAL_FIELD));
		llassert(material_data.get(MATERIALS_CAP_MATERIAL_FIELD).isMap());
		LLMaterialPtr material = setMaterial(material_id, material_data.get(MATERIALS_CAP_MATERIAL_FIELD));
		
		materials[material_id] = material;
	}

	getall_callback_map_t::iterator itCallback = mGetAllCallbacks.find(region_id);
	if (itCallback != mGetAllCallbacks.end())
	{
		(*itCallback->second)(region_id, materials);

		delete itCallback->second;
		mGetAllCallbacks.erase(itCallback);
	}
}

void LLMaterialMgr::onPutResponse(bool success, const LLSD& content, const LLUUID& object_id)
{
	if (!success)
	{
		// *TODO: is there any kind of error handling we can do here?
		return;
	}

	LLViewerObject* objectp = gObjectList.findObject(object_id);
	if (!objectp)
	{
		LL_WARNS("debugMaterials") << "Received PUT response for unknown object" << LL_ENDL;
		return;
	}

	llassert(content.isMap());
	llassert(content.has(MATERIALS_CAP_ZIP_FIELD));
	llassert(content.get(MATERIALS_CAP_ZIP_FIELD).isBinary());

	LLSD::Binary content_binary = content.get(MATERIALS_CAP_ZIP_FIELD).asBinary();
	std::string content_string(reinterpret_cast<const char*>(content_binary.data()), content_binary.size());
	std::istringstream content_stream(content_string);

	LLSD response_data;
	if (!unzip_llsd(response_data, content_stream, content_binary.size()))
	{
		LL_ERRS("debugMaterials") << "Cannot unzip LLSD binary content" << LL_ENDL;
		return;
	}
	else
	{
		llassert(response_data.isArray());

		for (LLSD::array_const_iterator faceIter = response_data.beginArray(); faceIter != response_data.endArray(); ++faceIter)
		{
			const LLSD& face_data = *faceIter;
			llassert(face_data.isMap());

			llassert(face_data.has(MATERIALS_CAP_OBJECT_ID_FIELD));
			llassert(face_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).isInteger());
			U32 local_id = face_data.get(MATERIALS_CAP_OBJECT_ID_FIELD).asInteger();
			if (objectp->getLocalID() != local_id)
			{
				LL_ERRS("debugMaterials") << "Received PUT response for wrong object" << LL_ENDL;
				continue;
			}

			llassert(face_data.has(MATERIALS_CAP_FACE_FIELD));
			llassert(face_data.get(MATERIALS_CAP_FACE_FIELD).isInteger());
			S32 te = face_data.get(MATERIALS_CAP_FACE_FIELD).asInteger();

			llassert(face_data.has(MATERIALS_CAP_MATERIAL_ID_FIELD));
			llassert(face_data.get(MATERIALS_CAP_MATERIAL_ID_FIELD).isBinary());
			LLMaterialID material_id(face_data.get(MATERIALS_CAP_MATERIAL_ID_FIELD).asBinary());

			LL_INFOS("debugMaterials") << "Setting material '" << material_id.asString() << "' on object '" << local_id 
				<< "' face " << te << LL_ENDL;

			objectp->setTEMaterialID(te, material_id);
		}
	}
}

static LLFastTimer::DeclareTimer FTM_MATERIALS_IDLE("Materials");

void LLMaterialMgr::onIdle(void*)
{
	LLFastTimer t(FTM_MATERIALS_IDLE);

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
	LLViewerRegion* regionp = gAgent.getRegion();
	if (!regionp)
	{
		LL_WARNS("debugMaterials") << "Agent region is NULL" << LL_ENDL;
		return;
	}
	else if (!regionp->capabilitiesReceived())
	{
		return;
	}

	const std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
	if (capURL.empty())
	{
		LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
			<< "' is not defined on region '" << regionp->getName() << "'" << LL_ENDL;
		return;
	}

	LLSD materialsData = LLSD::emptyArray();

	for (get_queue_t::const_iterator itQueue = mGetQueue.begin(); itQueue != mGetQueue.end(); ++itQueue)
	{
		const LLMaterialID& material_id = *itQueue;
		materialsData.append(material_id.asLLSD());
	}
	mGetQueue.clear();

	std::string materialString = zip_llsd(materialsData);

	S32 materialSize = materialString.size();
	if (materialSize <= 0)
	{
		LL_ERRS("debugMaterials") << "cannot zip LLSD binary content" << LL_ENDL;
		return;
	}

	LLSD::Binary materialBinary;
	materialBinary.resize(materialSize);
	memcpy(materialBinary.data(), materialString.data(), materialSize);

	LLSD postData = LLSD::emptyMap();
	postData[MATERIALS_CAP_ZIP_FIELD] = materialBinary;

	LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("POST", capURL, boost::bind(&LLMaterialMgr::onGetResponse, this, _1, _2));
	LLHTTPClient::post(capURL, postData, materialsResponder);
}

void LLMaterialMgr::processGetAllQueue()
{
	getall_queue_t::iterator itRegion = mGetAllQueue.begin();
	while (mGetAllQueue.end() != itRegion)
	{
		getall_queue_t::iterator curRegion = itRegion++;

		LLViewerRegion* regionp = LLWorld::instance().getRegionFromID(*curRegion);
		if (regionp == NULL)
		{
			LL_WARNS("debugMaterials") << "Unknown region with id " << (*curRegion).asString() << LL_ENDL;
			mGetAllQueue.erase(curRegion);
			continue;
		}
		else if (!regionp->capabilitiesReceived())
		{
			continue;
		}

		std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
		if (capURL.empty())
		{
			LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on the current region '" << regionp->getName() << "'" << LL_ENDL;
			mGetAllQueue.erase(curRegion);
			continue;
		}

		LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("GET", capURL, boost::bind(&LLMaterialMgr::onGetAllResponse, this, _1, _2, *curRegion));
		LLHTTPClient::get(capURL, materialsResponder);
		mGetAllQueue.erase(curRegion);
	}
}

void LLMaterialMgr::processPutQueue()
{
	put_queue_t::iterator itQueue = mPutQueue.begin();
	while (itQueue != mPutQueue.end())
	{
		put_queue_t::iterator curQueue = itQueue++;

		const LLUUID& object_id = curQueue->first;
		const LLViewerObject* objectp = gObjectList.findObject(object_id);
		if ( (!objectp) || (!objectp->getRegion()) )
		{
			LL_WARNS("debugMaterials") << "Object or object region is NULL" << LL_ENDL;

			mPutQueue.erase(curQueue);
			continue;
		}

		const LLViewerRegion* regionp = objectp->getRegion();
		if (!regionp->capabilitiesReceived())
		{
			continue;
		}

		std::string capURL = regionp->getCapability(MATERIALS_CAPABILITY_NAME);
		if (capURL.empty())
		{
			LL_WARNS("debugMaterials") << "Capability '" << MATERIALS_CAPABILITY_NAME
				<< "' is not defined on region '" << regionp->getName() << "'" << LL_ENDL;

			mPutQueue.erase(curQueue);
			continue;
		}

		LLSD facesData = LLSD::emptyArray();
		for (facematerial_map_t::const_iterator itFace = curQueue->second.begin(); itFace != curQueue->second.end(); ++itFace)
		{
			LLSD faceData = LLSD::emptyMap();
			faceData[MATERIALS_CAP_FACE_FIELD] = static_cast<LLSD::Integer>(itFace->first);
			faceData[MATERIALS_CAP_OBJECT_ID_FIELD] = static_cast<LLSD::Integer>(objectp->getLocalID());
			if (!itFace->second.isNull())
			{
				faceData[MATERIALS_CAP_MATERIAL_FIELD] = itFace->second.asLLSD();
			}
			facesData.append(faceData);

			LL_INFOS("debugMaterials") << "Requesting material change on object '" << faceData[MATERIALS_CAP_OBJECT_ID_FIELD].asInteger()
				<< "' face " << faceData[MATERIALS_CAP_FACE_FIELD].asInteger() << LL_ENDL;
		}

		LLSD materialsData = LLSD::emptyMap();
		materialsData[MATERIALS_CAP_FULL_PER_FACE_FIELD] = facesData;

		std::string materialString = zip_llsd(materialsData);

		S32 materialSize = materialString.size();
		if (materialSize <= 0)
		{
			LL_ERRS("debugMaterials") << "cannot zip LLSD binary content" << LL_ENDL;

			mPutQueue.erase(curQueue);
			continue;
		}
		else
		{
			LLSD::Binary materialBinary;
			materialBinary.resize(materialSize);
			memcpy(materialBinary.data(), materialString.data(), materialSize);

			LLSD putData = LLSD::emptyMap();
			putData[MATERIALS_CAP_ZIP_FIELD] = materialBinary;

			// *HACK: the viewer can't lookup the local object id the cap returns so we'll pass the object's uuid along
			LLHTTPClient::ResponderPtr materialsResponder = new LLMaterialsResponder("PUT", capURL, boost::bind(&LLMaterialMgr::onPutResponse, this, _1, _2, object_id));
			LLHTTPClient::put(capURL, putData, materialsResponder);
		}
	}
}
