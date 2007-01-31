/** 
 * @file llhttpclient.cpp
 * @brief Implementation of classes for making HTTP requests.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llhttpclient.h"

#include "llassetstorage.h"
#include "lliopipe.h"
#include "llurlrequest.h"
#include "llbufferstream.h"
#include "llsdserialize.h"
#include "llvfile.h"
#include "llvfs.h"

static const F32 HTTP_REQUEST_EXPIRY_SECS = 60.0f;

static std::string gCABundle;




LLHTTPClient::Responder::Responder()
	: mReferenceCount(0)
{
}

LLHTTPClient::Responder::~Responder()
{
}

// virtual
void LLHTTPClient::Responder::error(U32 status, const std::string& reason)
{
	llinfos << "LLHTTPClient::Responder::error "
		<< status << ": " << reason << llendl;
}

// virtual
void LLHTTPClient::Responder::result(const LLSD& content)
{
}

// virtual
void LLHTTPClient::Responder::completed(U32 status, const std::string& reason, const LLSD& content)
{
	if (200 <= status  &&  status < 300)
	{
		result(content);
	}
	else
	{
		error(status, reason);
	}
}


namespace
{
	class LLHTTPClientURLAdaptor : public LLURLRequestComplete
	{
	public:
		LLHTTPClientURLAdaptor(LLHTTPClient::ResponderPtr responder)
			: mResponder(responder),
				mStatus(499), mReason("LLURLRequest complete w/no status")
		{
		}
		
		~LLHTTPClientURLAdaptor()
		{
		}

		virtual void httpStatus(U32 status, const std::string& reason)
		{
			mStatus = status;
			mReason = reason;
		}

		virtual void complete(const LLChannelDescriptors& channels,
								const buffer_ptr_t& buffer)
		{
			LLBufferStream istr(channels, buffer.get());
			LLSD content;

			if (200 <= mStatus && mStatus < 300)
			{
				LLSDSerialize::fromXML(content, istr);
			}

			if (mResponder.get())
			{
				mResponder->completed(mStatus, mReason, content);
			}
		}

	private:
		LLHTTPClient::ResponderPtr mResponder;
		U32 mStatus;
		std::string mReason;
	};
	
	class Injector : public LLIOPipe
	{
	public:
		virtual const char* contentType() = 0;
	};

	class LLSDInjector : public Injector
	{
	public:
		LLSDInjector(const LLSD& sd) : mSD(sd) {}
		virtual ~LLSDInjector() {}

		const char* contentType() { return "application/xml"; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());
			LLSDSerialize::toXML(mSD, ostream);
			eos = true;
			return STATUS_DONE;
		}

		const LLSD mSD;
	};
	
	class FileInjector : public Injector
	{
	public:
		FileInjector(const std::string& filename) : mFilename(filename) {}
		virtual ~FileInjector() {}

		const char* contentType() { return "application/octet-stream"; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());

			llifstream fstream(mFilename.c_str(), std::iostream::binary | std::iostream::out);
            fstream.seekg(0, std::ios::end);
            U32 fileSize = fstream.tellg();
            fstream.seekg(0, std::ios::beg);
			char* fileBuffer;
			fileBuffer = new char [fileSize];
            fstream.read(fileBuffer, fileSize);
            ostream.write(fileBuffer, fileSize);
			fstream.close();
			eos = true;
			return STATUS_DONE;
		}

		const std::string mFilename;
	};
	
	class VFileInjector : public Injector
	{
	public:
		VFileInjector(const LLUUID& uuid, LLAssetType::EType asset_type) : mUUID(uuid), mAssetType(asset_type) {}
		virtual ~VFileInjector() {}

		const char* contentType() { return "application/octet-stream"; }

		virtual EStatus process_impl(const LLChannelDescriptors& channels,
			buffer_ptr_t& buffer, bool& eos, LLSD& context, LLPumpIO* pump)
		{
			LLBufferStream ostream(channels, buffer.get());
			
			LLVFile vfile(gVFS, mUUID, mAssetType, LLVFile::READ);
			S32 fileSize = vfile.getSize();
			U8* fileBuffer;
			fileBuffer = new U8 [fileSize];
            vfile.read(fileBuffer, fileSize);
            ostream.write((char*)fileBuffer, fileSize);
			eos = true;
			return STATUS_DONE;
		}

		const LLUUID mUUID;
		LLAssetType::EType mAssetType;
	};

	
	LLPumpIO* theClientPump = NULL;
}

static void request(const std::string& url, LLURLRequest::ERequestAction method,
	Injector* body_injector, LLHTTPClient::ResponderPtr responder)
{
	if (!LLHTTPClient::hasPump())
	{
		responder->completed(U32_MAX, "No pump", LLSD());
		return;
	}

	LLPumpIO::chain_t chain;

	LLURLRequest *req = new LLURLRequest(method, url);
	req->requestEncoding("");
	if (!gCABundle.empty())
	{
		req->checkRootCertificate(true, gCABundle.c_str());
	}
	req->setCallback(new LLHTTPClientURLAdaptor(responder));

	if (method == LLURLRequest::HTTP_PUT || method == LLURLRequest::HTTP_POST)
	{
		req->addHeader(llformat("Content-Type: %s", body_injector->contentType()).c_str());
		chain.push_back(LLIOPipe::ptr_t(body_injector));
	}
	chain.push_back(LLIOPipe::ptr_t(req));

	theClientPump->addChain(chain, HTTP_REQUEST_EXPIRY_SECS);
}

void LLHTTPClient::get(const std::string& url, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_GET, NULL, responder);
}

void LLHTTPClient::put(const std::string& url, const LLSD& body, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_PUT, new LLSDInjector(body), responder);
}

void LLHTTPClient::post(const std::string& url, const LLSD& body, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_POST, new LLSDInjector(body), responder);
}

void LLHTTPClient::del(const std::string& url, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_DELETE, NULL, responder);
}

#if 1
void LLHTTPClient::postFile(const std::string& url, const std::string& filename, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_POST, new FileInjector(filename), responder);
}

void LLHTTPClient::postFile(const std::string& url, const LLUUID& uuid,
							LLAssetType::EType asset_type, ResponderPtr responder)
{
	request(url, LLURLRequest::HTTP_POST, new VFileInjector(uuid, asset_type), responder);
}
#endif

void LLHTTPClient::setPump(LLPumpIO& pump)
{
	theClientPump = &pump;
}

bool LLHTTPClient::hasPump()
{
	return theClientPump != NULL;
}

void LLHTTPClient::setCABundle(const std::string& caBundle)
{
	gCABundle = caBundle;
}

namespace boost
{
	void intrusive_ptr_add_ref(LLHTTPClient::Responder* p)
	{
		++p->mReferenceCount;
	}
	
	void intrusive_ptr_release(LLHTTPClient::Responder* p)
	{
		if(0 == --p->mReferenceCount)
		{
			delete p;
		}
	}
};

