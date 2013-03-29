/** 
 * @file llhttpassetstorage.cpp
 * @brief Subclass capable of loading asset data to/from an external
 * source. Currently, a web server accessed via curl
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "linden_common.h"

#include "llhttpassetstorage.h"

#include <sys/stat.h>

#include "indra_constants.h"
#include "message.h"
#include "llproxy.h"
#include "llvfile.h"
#include "llvfs.h"

#ifdef LL_STANDALONE
# include <zlib.h>
#else
# include "zlib/zlib.h"
#endif

const U32 MAX_RUNNING_REQUESTS = 1;
const F32 MAX_PROCESSING_TIME = 0.005f;
const S32 CURL_XFER_BUFFER_SIZE = 65536;
// Try for 30 minutes for now.
const F32 GET_URL_TO_FILE_TIMEOUT = 1800.0f;

const S32 COMPRESSED_INPUT_BUFFER_SIZE = 4096;

const S32 HTTP_OK = 200;
const S32 HTTP_PUT_OK = 201;
const S32 HTTP_NO_CONTENT = 204;
const S32 HTTP_MISSING = 404;
const S32 HTTP_SERVER_BAD_GATEWAY = 502;
const S32 HTTP_SERVER_TEMP_UNAVAILABLE = 503;

/////////////////////////////////////////////////////////////////////////////////
// LLTempAssetData
// An asset not stored on central asset store, but on a simulator node somewhere.
/////////////////////////////////////////////////////////////////////////////////
struct LLTempAssetData
{
	LLUUID	mAssetID;
	LLUUID	mAgentID;
	std::string	mHostName;
};

/////////////////////////////////////////////////////////////////////////////////
// LLHTTPAssetRequest
/////////////////////////////////////////////////////////////////////////////////

class LLHTTPAssetRequest : public LLAssetRequest
{
public:
	LLHTTPAssetRequest(LLHTTPAssetStorage *asp, const LLUUID &uuid, 
					   LLAssetType::EType type, LLAssetStorage::ERequestType rt,
					   const std::string& url, CURLM *curl_multi);
	virtual ~LLHTTPAssetRequest();
	
	void setupCurlHandle();
	void cleanupCurlHandle();

	void   	prepareCompressedUpload();
	void	finishCompressedUpload();
	size_t	readCompressedData(void* data, size_t size);

	static size_t curlCompressedUploadCallback(
					void *data, size_t size, size_t nmemb, void *user_data);

	virtual LLSD getTerseDetails() const;
	virtual LLSD getFullDetails() const;

public:
	LLHTTPAssetStorage *mAssetStoragep;

	CURL  *mCurlHandle;
	CURLM *mCurlMultiHandle;
	std::string mURLBuffer;
	struct curl_slist *mHTTPHeaders;
	LLVFile *mVFile;
	LLUUID  mTmpUUID;
	LLAssetStorage::ERequestType mRequestType;

	bool		mZInitialized;
	z_stream	mZStream;
	char*		mZInputBuffer;
	bool		mZInputExhausted;

	FILE *mFP;
};


LLHTTPAssetRequest::LLHTTPAssetRequest(LLHTTPAssetStorage *asp, 
						const LLUUID &uuid, 
						LLAssetType::EType type, 
						LLAssetStorage::ERequestType rt,
						const std::string& url, 
						CURLM *curl_multi)
	: LLAssetRequest(uuid, type),
	  mZInitialized(false)
{
	memset(&mZStream, 0, sizeof(mZStream)); // we'll initialize this later, but for now zero the whole C-style struct to avoid debug/coverity noise
	mAssetStoragep = asp;
	mCurlHandle = NULL;
	mCurlMultiHandle = curl_multi;
	mVFile = NULL;
	mRequestType = rt;
	mHTTPHeaders = NULL;
	mFP = NULL;
	mZInputBuffer = NULL;
	mZInputExhausted = false;
	
	mURLBuffer = url;
}

LLHTTPAssetRequest::~LLHTTPAssetRequest()
{
	// Cleanup/cancel the request
	if (mCurlHandle)
	{
		curl_multi_remove_handle(mCurlMultiHandle, mCurlHandle);
		cleanupCurlHandle();
	}
	if (mHTTPHeaders)
	{
		curl_slist_free_all(mHTTPHeaders);
	}
	delete   mVFile;
	finishCompressedUpload();
}

// virtual
LLSD LLHTTPAssetRequest::getTerseDetails() const
{
	LLSD sd = LLAssetRequest::getTerseDetails();

	sd["url"] = mURLBuffer;

	return sd;
}

// virtual
LLSD LLHTTPAssetRequest::getFullDetails() const
{
	LLSD sd = LLAssetRequest::getFullDetails();

	if (mCurlHandle)
	{
		long curl_response = -1;
		long curl_connect = -1;
		double curl_total_time = -1.0f;
		double curl_size_upload = -1.0f;
		double curl_size_download = -1.0f;
		double curl_content_length_upload = -1.0f;
		double curl_content_length_download = -1.0f;
		long curl_request_size = -1;
		const char* curl_content_type = NULL;

		curl_easy_getinfo(mCurlHandle, CURLINFO_HTTP_CODE, &curl_response);
		curl_easy_getinfo(mCurlHandle, CURLINFO_HTTP_CONNECTCODE, &curl_connect);
		curl_easy_getinfo(mCurlHandle, CURLINFO_TOTAL_TIME, &curl_total_time);
		curl_easy_getinfo(mCurlHandle, CURLINFO_SIZE_UPLOAD,  &curl_size_upload);
		curl_easy_getinfo(mCurlHandle, CURLINFO_SIZE_DOWNLOAD, &curl_size_download);
		curl_easy_getinfo(mCurlHandle, CURLINFO_CONTENT_LENGTH_UPLOAD,   &curl_content_length_upload);
		curl_easy_getinfo(mCurlHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &curl_content_length_download);
		curl_easy_getinfo(mCurlHandle, CURLINFO_REQUEST_SIZE, &curl_request_size);
		curl_easy_getinfo(mCurlHandle, CURLINFO_CONTENT_TYPE, &curl_content_type);

		sd["curl_response_code"] = (int) curl_response;
		sd["curl_http_connect_code"] = (int) curl_connect;
		sd["curl_total_time"] = curl_total_time;
		sd["curl_size_upload"]   = curl_size_upload;
		sd["curl_size_download"] = curl_size_download;
		sd["curl_content_length_upload"]   =  curl_content_length_upload;
		sd["curl_content_length_download"] =  curl_content_length_download;
		sd["curl_request_size"] = (int) curl_request_size;
		if (curl_content_type)
		{
			sd["curl_content_type"] = curl_content_type;
		}
		else
		{
			sd["curl_content_type"] = "";
		}
	}

	sd["temp_id"] = mTmpUUID;
	sd["request_type"] = LLAssetStorage::getRequestName(mRequestType);
	sd["z_initialized"] = mZInitialized;
	sd["z_input_exhausted"] = mZInputExhausted;

	S32 file_size = -1;
	if (mFP)
	{
		struct stat file_stat;
		int file_desc = fileno(mFP);
		if ( fstat(file_desc, &file_stat) == 0)
		{
			file_size = file_stat.st_size;
		}
	}
	sd["file_size"] = file_size;

	return sd;
}


void LLHTTPAssetRequest::setupCurlHandle()
{
	// *NOTE: Similar code exists in mapserver/llcurlutil.cpp  JC
	mCurlHandle = LLCurl::newEasyHandle();
	llassert_always(mCurlHandle != NULL) ;

	// Apply proxy settings if configured to do so
	LLProxy::getInstance()->applyProxySettings(mCurlHandle);

	curl_easy_setopt(mCurlHandle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(mCurlHandle, CURLOPT_URL, mURLBuffer.c_str());
	curl_easy_setopt(mCurlHandle, CURLOPT_PRIVATE, this);
	if (LLAssetStorage::RT_DOWNLOAD == mRequestType)
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_ENCODING, "");
		// only do this on downloads, as uploads 
		// to some apache configs (like our test grids)
		// mistakenly claim the response is gzip'd if the resource
		// name ends in .gz, even though in a PUT, the response is
		// just plain HTML saying "created"
	}
	/* Remove the Pragma: no-cache header that libcurl inserts by default;
	   we want the cached version, if possible. */
	if (mZInitialized)
	{
		curl_easy_setopt(mCurlHandle, CURLOPT_PROXY, "");
			// disable use of proxy, which can't handle chunked transfers
	}
	mHTTPHeaders = curl_slist_append(mHTTPHeaders, "Pragma:");

	// bug in curl causes DNS to be cached for too long a time, 0 sets it to never cache DNS results internally (to curl)
	curl_easy_setopt(mCurlHandle, CURLOPT_DNS_CACHE_TIMEOUT, 0);
	
	// resist the temptation to explicitly add the Transfer-Encoding: chunked
	// header here - invokes a libCURL bug
	curl_easy_setopt(mCurlHandle, CURLOPT_HTTPHEADER, mHTTPHeaders);
	if (mAssetStoragep)
	{
		// Set the appropriate pending upload or download flag
		mAssetStoragep->addRunningRequest(mRequestType, this);
	}
	else
	{
		llerrs << "LLHTTPAssetRequest::setupCurlHandle - No asset storage associated with this request!" << llendl;
	}
}

void LLHTTPAssetRequest::cleanupCurlHandle()
{
	LLCurl::deleteEasyHandle(mCurlHandle);
	if (mAssetStoragep)
	{
		// Terminating a request.  Thus upload or download is no longer pending.
		mAssetStoragep->removeRunningRequest(mRequestType, this);
	}
	else
	{
		llerrs << "LLHTTPAssetRequest::~LLHTTPAssetRequest - No asset storage associated with this request!" << llendl;
	}
	mCurlHandle = NULL;
}

void LLHTTPAssetRequest::prepareCompressedUpload()
{
	mZStream.next_in = Z_NULL;
	mZStream.avail_in = 0;
	mZStream.zalloc = Z_NULL;
	mZStream.zfree = Z_NULL;
	mZStream.opaque = Z_NULL;

	int r = deflateInit2(&mZStream,
			1,			// compression level
			Z_DEFLATED,	// the only method defined
			15 + 16,	// the default windowBits + gzip header flag
			8,			// the default memLevel
			Z_DEFAULT_STRATEGY);

	if (r != Z_OK)
	{
		llerrs << "LLHTTPAssetRequest::prepareCompressedUpload defalateInit2() failed" << llendl;
	}

	mZInitialized = true;
	mZInputBuffer = new char[COMPRESSED_INPUT_BUFFER_SIZE];
	mZInputExhausted = false;

	mVFile = new LLVFile(gAssetStorage->mVFS,
					getUUID(), getType(), LLVFile::READ);
}

void LLHTTPAssetRequest::finishCompressedUpload()
{
	if (mZInitialized)
	{
		llinfos << "LLHTTPAssetRequest::finishCompressedUpload: "
			<< "read " << mZStream.total_in << " byte asset file, "
			<< "uploaded " << mZStream.total_out << " byte compressed asset"
			<< llendl;

		deflateEnd(&mZStream);
		delete[] mZInputBuffer;
	}
}

size_t LLHTTPAssetRequest::readCompressedData(void* data, size_t size)
{
	llassert(mZInitialized);

	mZStream.next_out = (Bytef*)data;
	mZStream.avail_out = size;

	while (mZStream.avail_out > 0)
	{
		if (mZStream.avail_in == 0 && !mZInputExhausted)
		{
			S32 to_read = llmin(COMPRESSED_INPUT_BUFFER_SIZE,
							(S32)(mVFile->getSize() - mVFile->tell()));
			
			if ( to_read > 0 )
			{
				mVFile->read((U8*)mZInputBuffer, to_read); /*Flawfinder: ignore*/
				mZStream.next_in = (Bytef*)mZInputBuffer;
				mZStream.avail_in = mVFile->getLastBytesRead();
			}

			mZInputExhausted = mZStream.avail_in == 0;
		}

		int r = deflate(&mZStream,
					mZInputExhausted ? Z_FINISH : Z_NO_FLUSH);

		if (r == Z_STREAM_END || r < 0 || mZInputExhausted)
		{
			if (r < 0)
			{
				llwarns << "LLHTTPAssetRequest::readCompressedData: deflate returned error code " 
						<< (S32) r << llendl;
			}
			break;
		}
	}

	return size - mZStream.avail_out;
}

//static
size_t LLHTTPAssetRequest::curlCompressedUploadCallback(
		void *data, size_t size, size_t nmemb, void *user_data)
{
	size_t num_read = 0;

	if (gAssetStorage)
	{
		CURL *curl_handle = (CURL *)user_data;
		LLHTTPAssetRequest *req = NULL;
		curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &req);
		if (req)
		{
			num_read = req->readCompressedData(data, size * nmemb);
		}
	}

	return num_read;
}

/////////////////////////////////////////////////////////////////////////////////
// LLHTTPAssetStorage
/////////////////////////////////////////////////////////////////////////////////


LLHTTPAssetStorage::LLHTTPAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
									 LLVFS *vfs, LLVFS *static_vfs, 
									 const LLHost &upstream_host,
									 const std::string& web_host,
									 const std::string& local_web_host,
									 const std::string& host_name)
	: LLAssetStorage(msg, xfer, vfs, static_vfs, upstream_host)
{
	_init(web_host, local_web_host, host_name);
}

LLHTTPAssetStorage::LLHTTPAssetStorage(LLMessageSystem *msg, LLXferManager *xfer,
									   LLVFS *vfs,
									   LLVFS *static_vfs,
									   const std::string& web_host,
									   const std::string& local_web_host,
									   const std::string& host_name)
	: LLAssetStorage(msg, xfer, vfs, static_vfs)
{
	_init(web_host, local_web_host, host_name);
}

void LLHTTPAssetStorage::_init(const std::string& web_host, const std::string& local_web_host, const std::string& host_name)
{
	mBaseURL = web_host;
	mLocalBaseURL = local_web_host;
	mHostName = host_name;

	// curl_global_init moved to LLCurl::initClass()
	
	mCurlMultiHandle = LLCurl::newMultiHandle() ;
	llassert_always(mCurlMultiHandle != NULL) ;
}

LLHTTPAssetStorage::~LLHTTPAssetStorage()
{
	LLCurl::deleteMultiHandle(mCurlMultiHandle);
	mCurlMultiHandle = NULL;
	
	// curl_global_cleanup moved to LLCurl::initClass()
}

// storing data is simpler than getting it, so we just overload the whole method
void LLHTTPAssetStorage::storeAssetData(
	const LLUUID& uuid,
	LLAssetType::EType type,
	LLAssetStorage::LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool store_local,
	const LLUUID& requesting_agent_id,
	bool user_waiting,
	F64 timeout)
{
	if (mVFS->getExists(uuid, type)) // VFS treats nonexistant and zero-length identically
	{
		LLAssetRequest *req = new LLAssetRequest(uuid, type);
		req->mUpCallback    = callback;
		req->mUserData      = user_data;
		req->mRequestingAgentID = requesting_agent_id;
		req->mIsUserWaiting = user_waiting;
		req->mTimeout       = timeout;

		// LLAssetStorage metric: Successful Request
		S32 size = mVFS->getSize(uuid, type);
		const char *message;
		if( store_local )
		{
			message = "Added to local upload queue";
		}
		else
		{
			message = "Added to upload queue";
		}
		reportMetric( uuid, type, LLStringUtil::null, requesting_agent_id, size, MR_OKAY, __FILE__, __LINE__, message );

		// this will get picked up and transmitted in checkForTimeouts
		if(store_local)
		{
			mPendingLocalUploads.push_back(req);
		}
		else if(is_priority)
		{
			mPendingUploads.push_front(req);
		}
		else
		{
			mPendingUploads.push_back(req);
		}
	}
	else
	{
		llwarns << "AssetStorage: attempt to upload non-existent vfile " << uuid << ":" << LLAssetType::lookup(type) << llendl;
		if (callback)
		{
			// LLAssetStorage metric: Zero size VFS
			reportMetric( uuid, type, LLStringUtil::null, requesting_agent_id, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file didn't exist or was zero length (VFS - can't tell which)" );
			callback(uuid, user_data,  LL_ERR_ASSET_REQUEST_NONEXISTENT_FILE, LL_EXSTAT_NONEXISTENT_FILE);
		}
	}
}

// virtual
void LLHTTPAssetStorage::storeAssetData(
	const std::string& filename,
	const LLUUID& asset_id,
	LLAssetType::EType asset_type,
	LLStoreAssetCallback callback,
	void* user_data,
	bool temp_file,
	bool is_priority,
	bool user_waiting,
	F64 timeout)
{
	llinfos << "LLAssetStorage::storeAssetData (legacy)" << asset_id << ":" << LLAssetType::lookup(asset_type) << llendl;

	LLLegacyAssetRequest *legacy = new LLLegacyAssetRequest;

	legacy->mUpCallback = callback;
	legacy->mUserData = user_data;

	FILE *fp = LLFile::fopen(filename, "rb"); /*Flawfinder: ignore*/
	S32 size = 0;
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
	}

	if( size )
	{
		LLVFile file(mVFS, asset_id, asset_type, LLVFile::WRITE);

		file.setMaxSize(size);

		const S32 buf_size = 65536;
		U8 copy_buf[buf_size];
		while ((size = (S32)fread(copy_buf, 1, buf_size, fp)))
		{
			file.write(copy_buf, size);
		}
		fclose(fp);

		// if this upload fails, the caller needs to setup a new tempfile for us
		if (temp_file)
		{
			LLFile::remove(filename);
		}
		
		// LLAssetStorage metric: Success not needed; handled in the overloaded method here:
		storeAssetData(
			asset_id,
			asset_type,
			legacyStoreDataCallback,
			(void**)legacy,
			temp_file,
			is_priority,
			false,
			LLUUID::null,
			user_waiting,
			timeout);
	}
	else // !size
	{
		if( fp )
		{
			// LLAssetStorage metric: Zero size
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_ZERO_SIZE, __FILE__, __LINE__, "The file was zero length" );
			fclose( fp );
		}
		else
		{
			// LLAssetStorage metric: Missing File
			reportMetric( asset_id, asset_type, filename, LLUUID::null, 0, MR_FILE_NONEXIST, __FILE__, __LINE__, "The file didn't exist" );
		}
		if (callback)
		{
			callback(LLUUID::null, user_data, LL_ERR_CANNOT_OPEN_FILE, LL_EXSTAT_BLOCKED_FILE);
		}
		delete legacy;
	}
}

// virtual
LLSD LLHTTPAssetStorage::getPendingDetails(LLAssetStorage::ERequestType rt,
										LLAssetType::EType asset_type,
										const std::string& detail_prefix) const
{
	LLSD sd = LLAssetStorage::getPendingDetails(rt, asset_type, detail_prefix);
	const request_list_t* running = getRunningList(rt);
	if (running)
	{
		// Loop through the pending requests sd, and add extra info about its running status.
		S32 num_pending = sd["requests"].size();
		S32 i;
		for (i = 0; i < num_pending; ++i)
		{
			LLSD& pending = sd["requests"][i];
			// See if this pending request is running.
			const LLAssetRequest* req = findRequest(running, 
									LLAssetType::lookup(pending["type"].asString()),
									pending["asset_id"]);
			if (req)
			{
				// Keep the detail_url so we don't have to rebuild it.
				LLURI detail_url = pending["detail"];
				pending = req->getTerseDetails();
				pending["detail"] = detail_url;
				pending["is_running"] = true;
			}
			else
			{
				pending["is_running"] = false;
			}
		}
	}
	return sd;
}

// virtual
LLSD LLHTTPAssetStorage::getPendingRequest(LLAssetStorage::ERequestType rt,
										LLAssetType::EType asset_type,
										const LLUUID& asset_id) const
{
	// Look for this asset in the running list first.
	const request_list_t* running = getRunningList(rt);
	if (running)
	{
		LLSD sd = LLAssetStorage::getPendingRequestImpl(running, asset_type, asset_id);
		if (sd)
		{
			sd["is_running"] = true;
			return sd;
		}
	}
	LLSD sd = LLAssetStorage::getPendingRequest(rt, asset_type, asset_id);
	if (sd)
	{
		sd["is_running"] = false;
	}
	return sd;
}

// virtual
bool LLHTTPAssetStorage::deletePendingRequest(LLAssetStorage::ERequestType rt,
											LLAssetType::EType asset_type,
											const LLUUID& asset_id)
{
	// Try removing this from the running list first.
	request_list_t* running = getRunningList(rt);
	if (running)
	{
		LLAssetRequest* req = findRequest(running, asset_type, asset_id);
		if (req)
		{
			// Remove this request from the running list to get it out of curl.
			running->remove(req);
			
			// Find this request in the pending list, so we can move it to the end of the line.
			request_list_t* pending = getRequestList(rt);
			if (pending)
			{
				request_list_t::iterator result = std::find_if(pending->begin(), pending->end(),
										std::bind2nd(ll_asset_request_equal<LLAssetRequest*>(), req));
				if (pending->end() != result)
				{
					// This request was found in the pending list.  Move it to the end!
					LLAssetRequest* pending_req = *result;
					pending->remove(pending_req);

					if (!pending_req->mIsUserWaiting)				//A user is waiting on this request.  Toss it.
					{
						pending->push_back(pending_req);
					}
					else
					{
						if (pending_req->mUpCallback)	//Clean up here rather than _callUploadCallbacks because this request is already cleared the req.
						{
							pending_req->mUpCallback(pending_req->getUUID(), pending_req->mUserData, -1, LL_EXSTAT_REQUEST_DROPPED);
						}

					}

					llinfos << "Asset " << getRequestName(rt) << " request for "
							<< asset_id << "." << LLAssetType::lookup(asset_type)
							<< " removed from curl and placed at the end of the pending queue."
							<< llendl;
				}
				else
				{
					llwarns << "Unable to find pending " << getRequestName(rt) << " request for "
							<< asset_id << "." << LLAssetType::lookup(asset_type) << llendl;
				}
			}
			delete req;

			return true;
		}
	}
	return LLAssetStorage::deletePendingRequest(rt, asset_type, asset_id);
}

// internal requester, used by getAssetData in superclass
void LLHTTPAssetStorage::_queueDataRequest(const LLUUID& uuid, LLAssetType::EType type,
										  void (*callback)(LLVFS *vfs, const LLUUID&, LLAssetType::EType, void *, S32, LLExtStat),
										  void *user_data, BOOL duplicate,
										   BOOL is_priority)
{
	// stash the callback info so we can find it after we get the response message
	LLAssetRequest *req = new LLAssetRequest(uuid, type);
	req->mDownCallback = callback;
	req->mUserData = user_data;
	req->mIsPriority = is_priority;

	// this will get picked up and downloaded in checkForTimeouts

	//
	// HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACK!  Asset requests were taking too long and timing out.
	// Since texture requests are the LEAST sensitive (on the simulator) to being delayed, add
	// non-texture requests to the front, and add texture requests to the back.  The theory is
	// that we always want them first, even if they're out of order.
	//
	
	if (req->getType() == LLAssetType::AT_TEXTURE)
	{
		mPendingDownloads.push_back(req);
	}
	else
	{
		mPendingDownloads.push_front(req);
	}
}

LLAssetRequest* LLHTTPAssetStorage::findNextRequest(LLAssetStorage::request_list_t& pending, 
													LLAssetStorage::request_list_t& running)
{
	// Early exit if the running list is full, or we don't have more pending than running.
	if (running.size() >= MAX_RUNNING_REQUESTS
		|| pending.size() <= running.size()) return NULL;

	// Look for the first pending request that is not already running.
	request_list_t::iterator running_begin = running.begin();
	request_list_t::iterator running_end   = running.end();

	request_list_t::iterator pending_iter = pending.begin();
	request_list_t::iterator pending_end  = pending.end();
	// Loop over all pending requests until we miss finding it in the running list.
	for (; pending_iter != pending_end; ++pending_iter)
	{
		LLAssetRequest* req = *pending_iter;
		// Look for this pending request in the running list.
		if (running_end == std::find_if(running_begin, running_end, 
						std::bind2nd(ll_asset_request_equal<LLAssetRequest*>(), req)))
		{
			// It isn't running!  Return it.
			return req;
		}
	}
	return NULL;
}

// overloaded to additionally move data to/from the webserver
void LLHTTPAssetStorage::checkForTimeouts()
{
	CURLMcode mcode;
	LLAssetRequest *req;
	while ( (req = findNextRequest(mPendingDownloads, mRunningDownloads)) )
	{
		// Setup this curl download request
		// We need to generate a new request here
		// since the one in the list could go away
		std::string tmp_url;
		std::string uuid_str;
		req->getUUID().toString(uuid_str);
		std::string base_url = getBaseURL(req->getUUID(), req->getType());
		tmp_url = llformat("%s/%36s.%s", base_url.c_str() , uuid_str.c_str(), LLAssetType::lookup(req->getType()));

		LLHTTPAssetRequest *new_req = new LLHTTPAssetRequest(this, req->getUUID(), 
										req->getType(), RT_DOWNLOAD, tmp_url, mCurlMultiHandle);
		new_req->mTmpUUID.generate();

		// Sets pending download flag internally
		new_req->setupCurlHandle();
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_FOLLOWLOCATION, TRUE);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_WRITEFUNCTION, &curlDownCallback);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_WRITEDATA, new_req->mCurlHandle);
	
		mcode = curl_multi_add_handle(mCurlMultiHandle, new_req->mCurlHandle);
		if (mcode > CURLM_OK)
		{
			// Failure.  Deleting the pending request will remove it from the running
			// queue, and push it to the end of the pending queue.
			new_req->cleanupCurlHandle();
			deletePendingRequest(RT_DOWNLOAD, new_req->getType(), new_req->getUUID());
			break;
		}
		else
		{
			llinfos << "Requesting " << new_req->mURLBuffer << llendl;
		}
	}

	while ( (req = findNextRequest(mPendingUploads, mRunningUploads)) )
	{
		// setup this curl upload request

		bool do_compress = req->getType() == LLAssetType::AT_OBJECT;

		std::string tmp_url;
		std::string uuid_str;
		req->getUUID().toString(uuid_str);
		tmp_url = mBaseURL + "/" + uuid_str + "." + LLAssetType::lookup(req->getType());
		if (do_compress) tmp_url += ".gz";

		LLHTTPAssetRequest *new_req = new LLHTTPAssetRequest(this, req->getUUID(), 
									req->getType(), RT_UPLOAD, tmp_url, mCurlMultiHandle);

		if (req->mIsUserWaiting) //If a user is waiting on a realtime response, we want to perserve information across upload attempts.
		{
			new_req->mTime          = req->mTime;
			new_req->mTimeout       = req->mTimeout;
			new_req->mIsUserWaiting = req->mIsUserWaiting;
		}

		if (do_compress)
		{
			new_req->prepareCompressedUpload();
		}

		// Sets pending upload flag internally
		new_req->setupCurlHandle();
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_UPLOAD, 1);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_WRITEFUNCTION, &nullOutputCallback);

		if (do_compress)
		{
			curl_easy_setopt(new_req->mCurlHandle, CURLOPT_READFUNCTION,
					&LLHTTPAssetRequest::curlCompressedUploadCallback);
		}
		else
		{
			LLVFile file(mVFS, req->getUUID(), req->getType());
			curl_easy_setopt(new_req->mCurlHandle, CURLOPT_INFILESIZE, file.getSize());
			curl_easy_setopt(new_req->mCurlHandle, CURLOPT_READFUNCTION,
					&curlUpCallback);
		}
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_READDATA, new_req->mCurlHandle);
	
		mcode = curl_multi_add_handle(mCurlMultiHandle, new_req->mCurlHandle);
		if (mcode > CURLM_OK)
		{
			// Failure.  Deleting the pending request will remove it from the running
			// queue, and push it to the end of the pending queue.
			new_req->cleanupCurlHandle();
			deletePendingRequest(RT_UPLOAD, new_req->getType(), new_req->getUUID());
			break;
		}
		else
		{
			// Get the uncompressed file size.
			LLVFile file(mVFS,new_req->getUUID(),new_req->getType());
			S32 size = file.getSize();
			llinfos << "Requesting PUT " << new_req->mURLBuffer << ", asset size: " << size << " bytes" << llendl;
			if (size == 0)
			{
				llwarns << "Rejecting zero size PUT request!" << llendl;
				new_req->cleanupCurlHandle();
				deletePendingRequest(RT_UPLOAD, new_req->getType(), new_req->getUUID());				
			}
		}
		// Pending upload will have been flagged by the request
	}

	while ( (req = findNextRequest(mPendingLocalUploads, mRunningLocalUploads)) )
	{
		// setup this curl upload request
		LLVFile file(mVFS, req->getUUID(), req->getType());

		std::string tmp_url;
		std::string uuid_str;
		req->getUUID().toString(uuid_str);
		
		// KLW - All temporary uploads are saved locally "http://localhost:12041/asset"
		tmp_url = llformat("%s/%36s.%s", mLocalBaseURL.c_str(), uuid_str.c_str(), LLAssetType::lookup(req->getType()));

		LLHTTPAssetRequest *new_req = new LLHTTPAssetRequest(this, req->getUUID(), 
										req->getType(), RT_LOCALUPLOAD, tmp_url, mCurlMultiHandle);
		new_req->mRequestingAgentID = req->mRequestingAgentID;

		// Sets pending upload flag internally
		new_req->setupCurlHandle();
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_PUT, 1);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_INFILESIZE, file.getSize());
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_WRITEFUNCTION, &nullOutputCallback);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_READFUNCTION, &curlUpCallback);
		curl_easy_setopt(new_req->mCurlHandle, CURLOPT_READDATA, new_req->mCurlHandle);
	
		mcode = curl_multi_add_handle(mCurlMultiHandle, new_req->mCurlHandle);
		if (mcode > CURLM_OK)
		{
			// Failure.  Deleting the pending request will remove it from the running
			// queue, and push it to the end of the pending queue.
			new_req->cleanupCurlHandle();
			deletePendingRequest(RT_LOCALUPLOAD, new_req->getType(), new_req->getUUID());
			break;
		}
		else
		{
			// Get the uncompressed file size.
			S32 size = file.getSize();

			llinfos << "TAT: LLHTTPAssetStorage::checkForTimeouts() : pending local!"
				<< " Requesting PUT " << new_req->mURLBuffer << ", asset size: " << size << " bytes" << llendl;
			if (size == 0)
			{
				
				llwarns << "Rejecting zero size PUT request!" << llendl;
				new_req->cleanupCurlHandle();
				deletePendingRequest(RT_UPLOAD, new_req->getType(), new_req->getUUID());				
			}

		}
		// Pending upload will have been flagged by the request
	}
	S32 count = 0;
	int queue_length;
	do
	{
		mcode = curl_multi_perform(mCurlMultiHandle, &queue_length);
		count++;
	} while (mcode == CURLM_CALL_MULTI_PERFORM && (count < 5));

	CURLMsg *curl_msg;
	do
	{
		curl_msg = curl_multi_info_read(mCurlMultiHandle, &queue_length);
		if (curl_msg && curl_msg->msg == CURLMSG_DONE)
		{
			long curl_result = 0;
			S32 xfer_result = LL_ERR_NOERR;

			LLHTTPAssetRequest *req = NULL;
			curl_easy_getinfo(curl_msg->easy_handle, CURLINFO_PRIVATE, &req);
								
			// TODO: Throw curl_result at all callbacks.
			curl_easy_getinfo(curl_msg->easy_handle, CURLINFO_HTTP_CODE, &curl_result);
			if (RT_UPLOAD == req->mRequestType || RT_LOCALUPLOAD == req->mRequestType)
			{
				if (curl_msg->data.result == CURLE_OK && 
					(   curl_result == HTTP_OK 
					 || curl_result == HTTP_PUT_OK 
					 || curl_result == HTTP_NO_CONTENT))
				{
					llinfos << "Success uploading " << req->getUUID() << " to " << req->mURLBuffer << llendl;
					if (RT_LOCALUPLOAD == req->mRequestType)
					{
						addTempAssetData(req->getUUID(), req->mRequestingAgentID, mHostName);
					}
				}
				else if (curl_msg->data.result == CURLE_COULDNT_CONNECT ||
						curl_msg->data.result == CURLE_OPERATION_TIMEOUTED ||
						curl_result == HTTP_SERVER_BAD_GATEWAY ||
						curl_result == HTTP_SERVER_TEMP_UNAVAILABLE)
				{
					llwarns << "Re-requesting upload for " << req->getUUID() << ".  Received upload error to " << req->mURLBuffer <<
						" with result " << curl_easy_strerror(curl_msg->data.result) << ", http result " << curl_result << llendl;

					////HACK (probably) I am sick of this getting requeued and driving me mad.
					//if (req->mIsUserWaiting)
					//{
					//	deletePendingRequest(RT_UPLOAD, req->getType(), req->getUUID());
					//}
				}
				else
				{
					llwarns << "Failure uploading " << req->getUUID() << " to " << req->mURLBuffer <<
						" with result " << curl_easy_strerror(curl_msg->data.result) << ", http result " << curl_result << llendl;

					xfer_result = LL_ERR_ASSET_REQUEST_FAILED;
				}

				if (!(curl_msg->data.result == CURLE_COULDNT_CONNECT ||
						curl_msg->data.result == CURLE_OPERATION_TIMEOUTED ||
						curl_result == HTTP_SERVER_BAD_GATEWAY ||
						curl_result == HTTP_SERVER_TEMP_UNAVAILABLE))
				{
					// shared upload finished callback
					// in the base class, this is called from processUploadComplete
					_callUploadCallbacks(req->getUUID(), req->getType(), (xfer_result == 0), LL_EXSTAT_CURL_RESULT | curl_result);
					// Pending upload flag will get cleared when the request is deleted
				}
			}
			else if (RT_DOWNLOAD == req->mRequestType)
			{
				if (curl_result == HTTP_OK && curl_msg->data.result == CURLE_OK)
				{
					if (req->mVFile && req->mVFile->getSize() > 0)
					{					
						llinfos << "Success downloading " << req->mURLBuffer << ", size " << req->mVFile->getSize() << llendl;

						req->mVFile->rename(req->getUUID(), req->getType());
					}
					else
					{
						// *TODO: if this actually indicates a bad asset on the server
						// (not certain at this point), then delete it
						llwarns << "Found " << req->mURLBuffer << " to be zero size" << llendl;
						xfer_result = LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE;
					}
				}
				else
				{
					// KLW - TAT See if an avatar owns this texture, and if so request re-upload.
					llwarns << "Failure downloading " << req->mURLBuffer << 
						" with result " << curl_easy_strerror(curl_msg->data.result) << ", http result " << curl_result << llendl;

					xfer_result = (curl_result == HTTP_MISSING) ? LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE : LL_ERR_ASSET_REQUEST_FAILED;

					if (req->mVFile)
					{
						req->mVFile->remove();
					}
				}

				// call the static callback for transfer completion
				// this will cleanup all requests for this asset, including ours
				downloadCompleteCallback(
					xfer_result,
					req->getUUID(),
					req->getType(),
					(void *)req,
					LL_EXSTAT_CURL_RESULT | curl_result);
				// Pending download flag will get cleared when the request is deleted
			}
			else
			{
				// nothing, just axe this request
				// currently this can only mean an asset delete
			}

			// Deleting clears the pending upload/download flag if it's set and the request is transferring
			delete req;
			req = NULL;
		}

	} while (curl_msg && queue_length > 0);
	

	// Cleanup 
	// We want to bump to the back of the line any running uploads that have timed out.
	bumpTimedOutUploads();

	LLAssetStorage::checkForTimeouts();
}

void LLHTTPAssetStorage::bumpTimedOutUploads()
{
	bool user_waiting=FALSE;

	F64 mt_secs = LLMessageSystem::getMessageTimeSeconds();

	if (mPendingUploads.size())
	{
		request_list_t::iterator it = mPendingUploads.begin();
		LLAssetRequest* req = *it;
		user_waiting=req->mIsUserWaiting;
	}

	// No point bumping currently running uploads if there are no others in line.
	if (!(mPendingUploads.size() > mRunningUploads.size()) && !user_waiting) 
	{
		return;
	}

	// deletePendingRequest will modify the mRunningUploads list so we don't want to iterate over it.
	request_list_t temp_running = mRunningUploads;

	request_list_t::iterator it = temp_running.begin();
	request_list_t::iterator end = temp_running.end();
	for ( ; it != end; ++it)
	{
		//request_list_t::iterator curiter = iter++;
		LLAssetRequest* req = *it;

		if ( req->mTimeout < (mt_secs - req->mTime) )
		{
			llwarns << "Asset upload request timed out for "
					<< req->getUUID() << "."
					<< LLAssetType::lookup(req->getType()) 
					<< ", bumping to the back of the line!" << llendl;

			deletePendingRequest(RT_UPLOAD, req->getType(), req->getUUID());
		}
	}
}

// static
size_t LLHTTPAssetStorage::curlDownCallback(void *data, size_t size, size_t nmemb, void *user_data)
{
	if (!gAssetStorage)
	{
		llwarns << "Missing gAssetStorage, aborting curl download callback!" << llendl;
		return 0;
	}
	S32 bytes = (S32)(size * nmemb);
	CURL *curl_handle = (CURL *)user_data;
	LLHTTPAssetRequest *req = NULL;
	curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &req);

	if (! req->mVFile)
	{
		req->mVFile = new LLVFile(gAssetStorage->mVFS, req->mTmpUUID, LLAssetType::AT_NONE, LLVFile::APPEND);
	}

	double content_length = 0.0;
	curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

	// sanitize content_length, reconcile w/ actual data
	S32 file_length = llmax(0, (S32)llmin(content_length, 20000000.0), bytes + req->mVFile->getSize());

	req->mVFile->setMaxSize(file_length);
	req->mVFile->write((U8*)data, bytes);

	return nmemb;
}

// static 
size_t LLHTTPAssetStorage::curlUpCallback(void *data, size_t size, size_t nmemb, void *user_data)
{
	if (!gAssetStorage)
	{
		llwarns << "Missing gAssetStorage, aborting curl download callback!" << llendl;
		return 0;
	}
	CURL *curl_handle = (CURL *)user_data;
	LLHTTPAssetRequest *req = NULL;
	curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &req);

	if (! req->mVFile)
	{
		req->mVFile = new LLVFile(gAssetStorage->mVFS, req->getUUID(), req->getType(), LLVFile::READ);
	}

	S32 bytes = llmin((S32)(size * nmemb), (S32)(req->mVFile->getSize() - req->mVFile->tell()));

	req->mVFile->read((U8*)data, bytes);/*Flawfinder: ignore*/

	return req->mVFile->getLastBytesRead();
}

// static
size_t LLHTTPAssetStorage::nullOutputCallback(void *data, size_t size, size_t nmemb, void *user_data)
{
	// do nothing, this is here to soak up script output so it doesn't end up on stdout

	return nmemb;
}



// blocking asset fetch which bypasses the VFS
// this is a very limited function for use by the simstate loader and other one-offs
S32 LLHTTPAssetStorage::getURLToFile(const LLUUID& uuid, LLAssetType::EType asset_type, const std::string &url, const std::string& filename, progress_callback callback, void *userdata)
{
	// *NOTE: There is no guarantee that the uuid and the asset_type match
	// - not that it matters. - Doug
	lldebugs << "LLHTTPAssetStorage::getURLToFile() - " << url << llendl;

	FILE *fp = LLFile::fopen(filename, "wb"); /*Flawfinder: ignore*/
	if (! fp)
	{
		llwarns << "Failed to open " << filename << " for writing" << llendl;
		return LL_ERR_ASSET_REQUEST_FAILED;
	}

	// make sure we use the normal curl setup, even though we don't really need a request object
	LLHTTPAssetRequest req(this, uuid, asset_type, RT_DOWNLOAD, url, mCurlMultiHandle);
	req.mFP = fp;
	
	req.setupCurlHandle();
	curl_easy_setopt(req.mCurlHandle, CURLOPT_FOLLOWLOCATION, TRUE);
	curl_easy_setopt(req.mCurlHandle, CURLOPT_WRITEFUNCTION, &curlFileDownCallback);
	curl_easy_setopt(req.mCurlHandle, CURLOPT_WRITEDATA, req.mCurlHandle);

	curl_multi_add_handle(mCurlMultiHandle, req.mCurlHandle);
	llinfos << "Requesting as file " << req.mURLBuffer << llendl;

	// braindead curl loop
	int queue_length;
	CURLMsg *curl_msg;
	LLTimer timeout;
	timeout.setTimerExpirySec(GET_URL_TO_FILE_TIMEOUT);
	bool success = false;
	S32 xfer_result = 0;
	do
	{
		curl_multi_perform(mCurlMultiHandle, &queue_length);
		curl_msg = curl_multi_info_read(mCurlMultiHandle, &queue_length);

		if (callback)
		{
			callback(userdata);
		}

		if ( curl_msg && (CURLMSG_DONE == curl_msg->msg) )
		{
			success = true;
		}
		else if (timeout.hasExpired())
		{
			llwarns << "Request for " << url << " has timed out." << llendl;
			success = false;
			xfer_result = LL_ERR_ASSET_REQUEST_FAILED;
			break;
		}
	} while (!success);

	if (success)
	{
		long curl_result = 0;
		curl_easy_getinfo(curl_msg->easy_handle, CURLINFO_HTTP_CODE, &curl_result);
		
		if (curl_result == HTTP_OK && curl_msg->data.result == CURLE_OK)
		{
			S32 size = ftell(req.mFP);
			if (size > 0)
			{
				// everything seems to be in order
				llinfos << "Success downloading " << req.mURLBuffer << " to file, size " << size << llendl;
			}
			else
			{
				llwarns << "Found " << req.mURLBuffer << " to be zero size" << llendl;
				xfer_result = LL_ERR_ASSET_REQUEST_FAILED;
			}
		}
		else
		{
			xfer_result = curl_result == HTTP_MISSING ? LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE : LL_ERR_ASSET_REQUEST_FAILED;
			llinfos << "Failure downloading " << req.mURLBuffer << 
				" with result " << curl_easy_strerror(curl_msg->data.result) << ", http result " << curl_result << llendl;
		}
	}

	fclose(fp);
	if (xfer_result)
	{
		LLFile::remove(filename);
	}
	return xfer_result;
}


// static
size_t LLHTTPAssetStorage::curlFileDownCallback(void *data, size_t size, size_t nmemb, void *user_data)
{	
	CURL *curl_handle = (CURL *)user_data;
	LLHTTPAssetRequest *req = NULL;
	curl_easy_getinfo(curl_handle, CURLINFO_PRIVATE, &req);

	if (! req->mFP)
	{
		llwarns << "Missing mFP, aborting curl file download callback!" << llendl;
		return 0;
	}

	return fwrite(data, size, nmemb, req->mFP);
}

LLAssetStorage::request_list_t* LLHTTPAssetStorage::getRunningList(LLAssetStorage::ERequestType rt)
{
	switch (rt)
	{
	case RT_DOWNLOAD:
		return &mRunningDownloads;
	case RT_UPLOAD:
		return &mRunningUploads;
	case RT_LOCALUPLOAD:
		return &mRunningLocalUploads;
	default:
		return NULL;
	}
}

const LLAssetStorage::request_list_t* LLHTTPAssetStorage::getRunningList(LLAssetStorage::ERequestType rt) const
{
	switch (rt)
	{
	case RT_DOWNLOAD:
		return &mRunningDownloads;
	case RT_UPLOAD:
		return &mRunningUploads;
	case RT_LOCALUPLOAD:
		return &mRunningLocalUploads;
	default:
		return NULL;
	}
}


void LLHTTPAssetStorage::addRunningRequest(ERequestType rt, LLHTTPAssetRequest* request)
{
	request_list_t* requests = getRunningList(rt);
	if (requests)
	{
		requests->push_back(request);
	}
	else
	{
		llerrs << "LLHTTPAssetStorage::addRunningRequest - Request is not an upload OR download, this is bad!" << llendl;
	}
}

void LLHTTPAssetStorage::removeRunningRequest(ERequestType rt, LLHTTPAssetRequest* request)
{
	request_list_t* requests = getRunningList(rt);
	if (requests)
	{
		requests->remove(request);
	}
	else
	{
		llerrs << "LLHTTPAssetStorage::removeRunningRequest - Destroyed request is not an upload OR download, this is bad!" << llendl;
	}
}

// virtual 
void LLHTTPAssetStorage::addTempAssetData(const LLUUID& asset_id, const LLUUID& agent_id, const std::string& host_name)
{
	if (agent_id.isNull() || asset_id.isNull())
	{
		llwarns << "TAT: addTempAssetData bad id's asset_id: " << asset_id << "  agent_id: " << agent_id << llendl;
		return;
	}

	LLTempAssetData temp_asset_data;
	temp_asset_data.mAssetID = asset_id;
	temp_asset_data.mAgentID = agent_id;
	temp_asset_data.mHostName = host_name;

	mTempAssets[asset_id] = temp_asset_data;
}

// virtual
BOOL LLHTTPAssetStorage::hasTempAssetData(const LLUUID& texture_id) const
{
	uuid_tempdata_map::const_iterator citer = mTempAssets.find(texture_id);
	BOOL found = (citer != mTempAssets.end());
	return found;
}

// virtual
std::string LLHTTPAssetStorage::getTempAssetHostName(const LLUUID& texture_id) const
{
	uuid_tempdata_map::const_iterator citer = mTempAssets.find(texture_id);
	if (citer != mTempAssets.end())
	{
		return citer->second.mHostName;
	}
	else
	{
		return std::string();
	}
}

// virtual 
LLUUID LLHTTPAssetStorage::getTempAssetAgentID(const LLUUID& texture_id) const
{
	uuid_tempdata_map::const_iterator citer = mTempAssets.find(texture_id);
	if (citer != mTempAssets.end())
	{
		return citer->second.mAgentID;
	}
	else
	{
		return LLUUID::null;
	}
}

// virtual 
void LLHTTPAssetStorage::removeTempAssetData(const LLUUID& asset_id)
{
	mTempAssets.erase(asset_id);
}

// virtual 
void LLHTTPAssetStorage::removeTempAssetDataByAgentID(const LLUUID& agent_id)
{
	uuid_tempdata_map::iterator it = mTempAssets.begin();
	uuid_tempdata_map::iterator end = mTempAssets.end();

	while (it != end)
	{
		const LLTempAssetData& asset_data = it->second;
		if (asset_data.mAgentID == agent_id)
		{
			mTempAssets.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

std::string LLHTTPAssetStorage::getBaseURL(const LLUUID& asset_id, LLAssetType::EType asset_type)
{
	if (LLAssetType::AT_TEXTURE == asset_type)
	{
		uuid_tempdata_map::const_iterator citer = mTempAssets.find(asset_id);
		if (citer != mTempAssets.end())
		{
			const std::string& host_name = citer->second.mHostName;
			std::string url = llformat(LOCAL_ASSET_URL_FORMAT, host_name.c_str());
			return url;
		}
	}

	return mBaseURL;
}

void LLHTTPAssetStorage::dumpTempAssetData(const LLUUID& avatar_id) const
{
	uuid_tempdata_map::const_iterator it = mTempAssets.begin();
	uuid_tempdata_map::const_iterator end = mTempAssets.end();
	S32 count = 0;
	for ( ; it != end; ++it)
	{
		const LLTempAssetData& temp_asset_data = it->second;
		if (avatar_id.isNull()
			|| avatar_id == temp_asset_data.mAgentID)
		{
			llinfos << "TAT: dump agent " << temp_asset_data.mAgentID
				<< " texture " << temp_asset_data.mAssetID
				<< " host " << temp_asset_data.mHostName
				<< llendl;
			count++;
		}
	}

	if (avatar_id.isNull())
	{
		llinfos << "TAT: dumped " << count << " entries for all avatars" << llendl;
	}
	else
	{
		llinfos << "TAT: dumped " << count << " entries for avatar " << avatar_id << llendl;
	}
}

void LLHTTPAssetStorage::clearTempAssetData()
{
	llinfos << "TAT: Clearing temp asset data map" << llendl;
	mTempAssets.clear();
}
