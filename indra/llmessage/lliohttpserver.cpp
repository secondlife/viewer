/** 
 * @file lliohttpserver.cpp
 * @author Phoenix
 * @date 2005-10-05
 * @brief Implementation of the http server classes
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "lliohttpserver.h"

#include "llapr.h"
#include "llbuffer.h"
#include "llbufferstream.h"
#include "llhttpnode.h"
#include "lliopipe.h"
#include "lliosocket.h"
#include "llioutil.h"
#include "llmemorystream.h"
#include "llpumpio.h"
#include "llsd.h"
#include "llsdserialize_xml.h"
#include "llstl.h"
#include "lltimer.h"

#include <sstream>

#include <boost/tokenizer.hpp>

static const char HTTP_VERSION_STR[] = "HTTP/1.0";
const std::string CONTEXT_REQUEST("request");
const std::string CONTEXT_RESPONSE("response");
const std::string CONTEXT_VERB("verb");
const std::string CONTEXT_HEADERS("headers");
const std::string HTTP_VERB_GET("GET");
const std::string HTTP_VERB_PUT("PUT");
const std::string HTTP_VERB_POST("POST");
const std::string HTTP_VERB_DELETE("DELETE");
const std::string HTTP_VERB_OPTIONS("OPTIONS");

static LLIOHTTPServer::timing_callback_t sTimingCallback = NULL;
static void* sTimingCallbackData = NULL;

class LLHTTPPipe : public LLIOPipe
{
public:
	LLHTTPPipe(const LLHTTPNode& node)
		: mNode(node),
		  mResponse(NULL),
		  mState(STATE_INVOKE),
		  mChainLock(0),
		  mLockedPump(NULL),
		  mStatusCode(0)
		{ }
	virtual ~LLHTTPPipe()
	{
		if (mResponse.notNull())
		{
			mResponse->nullPipe();
		}
	}

private:
	// LLIOPipe API implementation.
	virtual EStatus process_impl(
        const LLChannelDescriptors& channels,
        LLIOPipe::buffer_ptr_t& buffer,
        bool& eos,
        LLSD& context,
        LLPumpIO* pump);

	const LLHTTPNode& mNode;

	class Response : public LLHTTPNode::Response
	{
	public:

		static LLPointer<Response> create(LLHTTPPipe* pipe);
		virtual ~Response();

		// from LLHTTPNode::Response
		virtual void result(const LLSD&);
		virtual void extendedResult(S32 code, const std::string& body, const LLSD& headers);
		
		virtual void status(S32 code, const std::string& message);

		void nullPipe();

	private:
		Response() : mPipe(NULL) {} // Must be accessed through LLPointer.
		LLHTTPPipe* mPipe;
	};
	friend class Response;

	LLPointer<Response> mResponse;

	enum State
	{
		STATE_INVOKE,
		STATE_DELAYED,
		STATE_LOCKED,
		STATE_GOOD_RESULT,
		STATE_STATUS_RESULT,
		STATE_EXTENDED_RESULT
	};
	State mState;

	S32 mChainLock;
	LLPumpIO* mLockedPump;

	void lockChain(LLPumpIO*);
	void unlockChain();

	LLSD mGoodResult;
	S32 mStatusCode;
	std::string mStatusMessage;	
	LLSD mHeaders;
};

static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_PIPE("HTTP Pipe");
static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_GET("HTTP Get");
static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_PUT("HTTP Put");
static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_POST("HTTP Post");
static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_DELETE("HTTP Delete");

LLIOPipe::EStatus LLHTTPPipe::process_impl(
	const LLChannelDescriptors& channels,
    buffer_ptr_t& buffer,
    bool& eos,
    LLSD& context,
    LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_HTTP_PIPE);
	PUMP_DEBUG;
    lldebugs << "LLSDHTTPServer::process_impl" << llendl;

    // Once we have all the data, We need to read the sd on
    // the the in channel, and respond on  the out channel

    if(!eos) return STATUS_BREAK;
    if(!pump || !buffer) return STATUS_PRECONDITION_NOT_MET;

	PUMP_DEBUG;
	if (mState == STATE_INVOKE)
	{
		PUMP_DEBUG;
		mState = STATE_DELAYED;
			// assume deferred unless mResponse does otherwise
		mResponse = Response::create(this);

		// *TODO: Babbage: Parameterize parser?
		// *TODO: We should look at content-type and do the right
		// thing. Phoenix 2007-12-31
		LLBufferStream istr(channels, buffer.get());

		static LLTimer timer;
		timer.reset();

		std::string verb = context[CONTEXT_REQUEST][CONTEXT_VERB];
		if(verb == HTTP_VERB_GET)
		{
			LLFastTimer _(FTM_PROCESS_HTTP_GET);
			mNode.get(LLHTTPNode::ResponsePtr(mResponse), context);
		}
		else if(verb == HTTP_VERB_PUT)
		{
			LLFastTimer _(FTM_PROCESS_HTTP_PUT);
			LLSD input;
			if (mNode.getContentType() == LLHTTPNode::CONTENT_TYPE_LLSD)
			{
				LLSDSerialize::fromXML(input, istr);
			}
			else if (mNode.getContentType() == LLHTTPNode::CONTENT_TYPE_TEXT)
			{
				std::stringstream strstrm;
				strstrm << istr.rdbuf();
				input = strstrm.str();
			}
			mNode.put(LLHTTPNode::ResponsePtr(mResponse), context, input);
		}
		else if(verb == HTTP_VERB_POST)
		{
			LLFastTimer _(FTM_PROCESS_HTTP_POST);
			LLSD input;
			if (mNode.getContentType() == LLHTTPNode::CONTENT_TYPE_LLSD)
			{
				LLSDSerialize::fromXML(input, istr);
			}
			else if (mNode.getContentType() == LLHTTPNode::CONTENT_TYPE_TEXT)
			{
				std::stringstream strstrm;
				strstrm << istr.rdbuf();
				input = strstrm.str();
			}
			mNode.post(LLHTTPNode::ResponsePtr(mResponse), context, input);
		}
		else if(verb == HTTP_VERB_DELETE)
		{
			LLFastTimer _(FTM_PROCESS_HTTP_DELETE);
			mNode.del(LLHTTPNode::ResponsePtr(mResponse), context);
		}		
		else if(verb == HTTP_VERB_OPTIONS)
		{
			mNode.options(LLHTTPNode::ResponsePtr(mResponse), context);
		}		
		else 
		{
		    mResponse->methodNotAllowed();
		}

		F32 delta = timer.getElapsedTimeF32();
		if (sTimingCallback)
		{
			LLHTTPNode::Description desc;
			mNode.describe(desc);
			LLSD info = desc.getInfo();
			std::string timing_name = info["description"];
			timing_name += " ";
			timing_name += verb;
			sTimingCallback(timing_name.c_str(), delta, sTimingCallbackData);
		}

		// Log all HTTP transactions.
		// TODO: Add a way to log these to their own file instead of indra.log
		// It is just too spammy to be in indra.log.
		lldebugs << verb << " " << context[CONTEXT_REQUEST]["path"].asString()
			<< " " << mStatusCode << " " <<  mStatusMessage << " " << delta
			<< "s" << llendl;

		// Log Internal Server Errors
		//if(mStatusCode == 500)
		//{
		//	llwarns << "LLHTTPPipe::process_impl:500:Internal Server Error" 
		//			<< llendl;
		//}
	}

	PUMP_DEBUG;
	switch (mState)
	{
		case STATE_DELAYED:
			lockChain(pump);
			mState = STATE_LOCKED;
			return STATUS_BREAK;

		case STATE_LOCKED:
			// should never ever happen!
			return STATUS_ERROR;

		case STATE_GOOD_RESULT:
		{
			LLSD headers = mHeaders;
			headers["Content-Type"] = "application/llsd+xml";
			context[CONTEXT_RESPONSE][CONTEXT_HEADERS] = headers;
			LLBufferStream ostr(channels, buffer.get());
			LLSDSerialize::toXML(mGoodResult, ostr);

			return STATUS_DONE;
		}

		case STATE_STATUS_RESULT:
		{
			LLSD headers = mHeaders;
			headers["Content-Type"] = "text/plain";
			context[CONTEXT_RESPONSE][CONTEXT_HEADERS] = headers;
			context[CONTEXT_RESPONSE]["statusCode"] = mStatusCode;
			context[CONTEXT_RESPONSE]["statusMessage"] = mStatusMessage;
			LLBufferStream ostr(channels, buffer.get());
			ostr << mStatusMessage;

			return STATUS_DONE;
		}
		case STATE_EXTENDED_RESULT:
		{
			context[CONTEXT_RESPONSE][CONTEXT_HEADERS] = mHeaders;
			context[CONTEXT_RESPONSE]["statusCode"] = mStatusCode;
			LLBufferStream ostr(channels, buffer.get());
			ostr << mStatusMessage;

			return STATUS_DONE;
		}
		default:
			llwarns << "LLHTTPPipe::process_impl: unexpected state "
				<< mState << llendl;

			return STATUS_BREAK;
	}
// 	PUMP_DEBUG; // unreachable
}

LLPointer<LLHTTPPipe::Response> LLHTTPPipe::Response::create(LLHTTPPipe* pipe)
{
	LLPointer<Response> result = new Response();
	result->mPipe = pipe;
	return result;
}

// virtual
LLHTTPPipe::Response::~Response()
{
}

void LLHTTPPipe::Response::nullPipe()
{
	mPipe = NULL;
}

// virtual
void LLHTTPPipe::Response::result(const LLSD& r)
{
	if(! mPipe)
	{
		llwarns << "LLHTTPPipe::Response::result: NULL pipe" << llendl;
		return;
	}

	mPipe->mStatusCode = 200;
	mPipe->mStatusMessage = "OK";
	mPipe->mGoodResult = r;
	mPipe->mState = STATE_GOOD_RESULT;
	mPipe->mHeaders = mHeaders;
	mPipe->unlockChain();	
}

void LLHTTPPipe::Response::extendedResult(S32 code, const std::string& body, const LLSD& headers)
{
	if(! mPipe)
	{
		llwarns << "LLHTTPPipe::Response::status: NULL pipe" << llendl;
		return;
	}

	mPipe->mStatusCode = code;
	mPipe->mStatusMessage = body;
	mPipe->mHeaders = headers;
	mPipe->mState = STATE_EXTENDED_RESULT;
	mPipe->unlockChain();
}

// virtual
void LLHTTPPipe::Response::status(S32 code, const std::string& message)
{
	if(! mPipe)
	{
		llwarns << "LLHTTPPipe::Response::status: NULL pipe" << llendl;
		return;
	}

	mPipe->mStatusCode = code;
	mPipe->mStatusMessage = message;
	mPipe->mState = STATE_STATUS_RESULT;
	mPipe->mHeaders = mHeaders;
	mPipe->unlockChain();
}

void LLHTTPPipe::lockChain(LLPumpIO* pump)
{
	if (mChainLock != 0) { return; }

	mLockedPump = pump;
	mChainLock = pump->setLock();
}

void LLHTTPPipe::unlockChain()
{
	if (mChainLock == 0) { return; }

	mLockedPump->clearLock(mChainLock);
	mLockedPump = NULL;
	mChainLock = 0;
}



/** 
 * @class LLHTTPResponseHeader
 * @brief Class which correctly builds HTTP headers on a pipe
 * @see LLIOPipe
 *
 * An instance of this class can be placed in a chain where it will
 * wait for an end of stream. Once it gets that, it will count the
 * bytes on CHANNEL_OUT (or the size of the buffer in io pipe versions
 * prior to 2) prepend that data to the request in an HTTP format, and
 * supply all normal HTTP response headers.
 */
class LLHTTPResponseHeader : public LLIOPipe
{
public:
	LLHTTPResponseHeader() : mCode(0) {}
	virtual ~LLHTTPResponseHeader() {}

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	S32 mCode;
};


/**
 * LLHTTPResponseHeader
 */

static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_HEADER("HTTP Header");

// virtual
LLIOPipe::EStatus LLHTTPResponseHeader::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_HTTP_HEADER);
	PUMP_DEBUG;
	if(eos)
	{
		PUMP_DEBUG;
		//mGotEOS = true;
		std::ostringstream ostr;
		std::string message = context[CONTEXT_RESPONSE]["statusMessage"];
		
		int code = context[CONTEXT_RESPONSE]["statusCode"];
		if (code < 200)
		{
			code = 200;
			message = "OK";
		}
		
		ostr << HTTP_VERSION_STR << " " << code << " " << message << "\r\n";

		S32 content_length = buffer->countAfter(channels.in(), NULL);
		if(0 < content_length)
		{
			ostr << "Content-Length: " << content_length << "\r\n";
		}
		// *NOTE: This guard can go away once the LLSD static map
		// iterator is available. Phoenix. 2008-05-09
		LLSD headers = context[CONTEXT_RESPONSE][CONTEXT_HEADERS];
		if(headers.isDefined())
		{
			LLSD::map_iterator iter = headers.beginMap();
			LLSD::map_iterator end = headers.endMap();
			for(; iter != end; ++iter)
			{
				ostr << (*iter).first << ": " << (*iter).second.asString()
					<< "\r\n";
			}
		}
		ostr << "\r\n";

		LLChangeChannel change(channels.in(), channels.out());
		std::for_each(buffer->beginSegment(), buffer->endSegment(), change);
		std::string header = ostr.str();
		buffer->prepend(channels.out(), (U8*)header.c_str(), header.size());
		PUMP_DEBUG;
		return STATUS_DONE;
	}
	PUMP_DEBUG;
	return STATUS_OK;
}



/** 
 * @class LLHTTPResponder
 * @brief This class 
 * @see LLIOPipe
 *
 * <b>NOTE:</b> You should not need to create or use one of these, the
 * details are handled by the HTTPResponseFactory.
 */
class LLHTTPResponder : public LLIOPipe
{
public:
	LLHTTPResponder(const LLHTTPNode& tree, const LLSD& ctx);
	~LLHTTPResponder();

protected:
	/** 
	 * @brief Read data off of CHANNEL_IN keeping track of last read position.
	 *
	 * This is a quick little hack to read headers. It is not IO
	 * optimal, but it makes it easier for me to implement the header
	 * parsing. Plus, there should never be more than a few headers.
	 * This method will tend to read more than necessary, find the
	 * newline, make the front part of dest look like a c string, and
	 * move the read head back to where the newline was found. Thus,
	 * the next read will pick up on the next line.
	 * @param channel The channel to read in the buffer
	 * @param buffer The heap array of processed data
	 * @param dest Destination for the data to be read
	 * @param[in,out] len <b>in</b> The size of the buffer. <b>out</b> how 
	 * much was read. This value is not useful for determining where to 
	 * seek orfor string assignment.
	 * @returns Returns true if a line was found.
	 */
	bool readHeaderLine(
		const LLChannelDescriptors& channels,
		buffer_ptr_t buffer,
		U8* dest,
		S32& len);
	
	/** 
	 * @brief Mark the request as bad, and handle appropriately
	 *
	 * @param channels The channels to use in the buffer.
	 * @param buffer The heap array of processed data.
	 */
	void markBad(const LLChannelDescriptors& channels, buffer_ptr_t buffer);

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data in buffer
	 */
	EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	enum EState
	{
		STATE_NOTHING,
		STATE_READING_HEADERS,
		STATE_LOOKING_FOR_EOS,
		STATE_DONE,
		STATE_SHORT_CIRCUIT
	};

	LLSD mBuildContext;
	EState mState;
	U8* mLastRead;
	std::string mVerb;
	std::string mAbsPathAndQuery;
	std::string mPath;
	std::string mQuery;
	std::string mVersion;
	S32 mContentLength;
	LLSD mHeaders;

	// handle the urls
	const LLHTTPNode& mRootNode;
};

LLHTTPResponder::LLHTTPResponder(const LLHTTPNode& tree, const LLSD& ctx) :
	mBuildContext(ctx),
	mState(STATE_NOTHING),
	mLastRead(NULL),
	mContentLength(0),
	mRootNode(tree)
{
}

// virtual
LLHTTPResponder::~LLHTTPResponder()
{
	//lldebugs << "destroying LLHTTPResponder" << llendl;
}

bool LLHTTPResponder::readHeaderLine(
	const LLChannelDescriptors& channels,
	buffer_ptr_t buffer,
	U8* dest,
	S32& len)
{
	--len;
	U8* last = buffer->readAfter(channels.in(), mLastRead, dest, len);
	dest[len] = '\0';
	U8* newline = (U8*)strchr((char*)dest, '\n');
	if(!newline)
	{
		if(len)
		{
			lldebugs << "readLine failed - too long maybe?" << llendl;
			markBad(channels, buffer);
		}
		return false;
	}
	S32 offset = -((len - 1) - (newline - dest));
	++newline;
	*newline = '\0';
	mLastRead = buffer->seek(channels.in(), last, offset);
	return true;
}

void LLHTTPResponder::markBad(
	const LLChannelDescriptors& channels,
	buffer_ptr_t buffer)
{
	mState = STATE_SHORT_CIRCUIT;
	LLBufferStream out(channels, buffer.get());
	out << HTTP_VERSION_STR << " 400 Bad Request\r\n\r\n<html>\n"
		<< "<title>Bad Request</title>\n<body>\nBad Request.\n"
		<< "</body>\n</html>\n";
}

static LLFastTimer::DeclareTimer FTM_PROCESS_HTTP_RESPONDER("HTTP Responder");

// virtual
LLIOPipe::EStatus LLHTTPResponder::process_impl(
	const LLChannelDescriptors& channels,
	buffer_ptr_t& buffer,
	bool& eos,
	LLSD& context,
	LLPumpIO* pump)
{
	LLFastTimer t(FTM_PROCESS_HTTP_RESPONDER);
	PUMP_DEBUG;
	LLIOPipe::EStatus status = STATUS_OK;

	// parsing headers
	if((STATE_NOTHING == mState) || (STATE_READING_HEADERS == mState))
	{
		PUMP_DEBUG;
		status = STATUS_BREAK;
		mState = STATE_READING_HEADERS;
		const S32 HEADER_BUFFER_SIZE = 1024;
		char buf[HEADER_BUFFER_SIZE + 1];  /*Flawfinder: ignore*/
		S32 len = HEADER_BUFFER_SIZE;

#if 0
		if(true)
		{
		LLBufferArray::segment_iterator_t seg_iter = buffer->beginSegment();
		char buf[1024];	  /*Flawfinder: ignore*/
		while(seg_iter != buffer->endSegment())
		{
			memcpy(buf, (*seg_iter).data(), (*seg_iter).size());	  /*Flawfinder: ignore*/
			buf[(*seg_iter).size()] = '\0';
			llinfos << (*seg_iter).getChannel() << ": " << buf
					<< llendl;
			++seg_iter;
		}
		}
#endif
		
		PUMP_DEBUG;
		if(readHeaderLine(channels, buffer, (U8*)buf, len))
		{
			bool read_next_line = false;
			bool parse_all = true;
			if(mVerb.empty())
			{
				read_next_line = true;
				LLMemoryStream header((U8*)buf, len);
				header >> mVerb;

				if((HTTP_VERB_GET == mVerb)
				   || (HTTP_VERB_POST == mVerb)
				   || (HTTP_VERB_PUT == mVerb)
				   || (HTTP_VERB_DELETE == mVerb)
				   || (HTTP_VERB_OPTIONS == mVerb))
				{
					header >> mAbsPathAndQuery;
					header >> mVersion;

					lldebugs << "http request: "
							 << mVerb
							 << " " << mAbsPathAndQuery
							 << " " << mVersion << llendl;

					std::string::size_type delimiter
						= mAbsPathAndQuery.find('?');
					if (delimiter == std::string::npos)
					{
						mPath = mAbsPathAndQuery;
						mQuery = "";
					}
					else
					{
						mPath = mAbsPathAndQuery.substr(0, delimiter);
						mQuery = mAbsPathAndQuery.substr(delimiter+1);
					}

					if(!mAbsPathAndQuery.empty())
					{
						if(mVersion.empty())
						{
							// simple request.
							parse_all = false;
							mState = STATE_DONE;
							mVersion.assign("HTTP/1.0");
						}
					}
				}
				else
				{
					read_next_line = false;
					parse_all = false;
					lldebugs << "unknown http verb: " << mVerb << llendl;
					markBad(channels, buffer);
				}
			}
			if(parse_all)
			{
				bool keep_parsing = true;
				while(keep_parsing)
				{
					if(read_next_line)
					{
						len = HEADER_BUFFER_SIZE;	
						if (!readHeaderLine(channels, buffer, (U8*)buf, len))
						{
							// Failed to read the header line, probably too long.
							// readHeaderLine already marked the channel/buffer as bad.
							keep_parsing = false;
							break;
						}
					}
					if(0 == len)
					{
						return status;
					}
					if(buf[0] == '\r' && buf[1] == '\n')
					{
						// end-o-headers
						keep_parsing = false;
						mState = STATE_LOOKING_FOR_EOS;
						break;
					}
					char* pos_colon = strchr(buf, ':');
					if(NULL == pos_colon)
					{
						keep_parsing = false;
						lldebugs << "bad header: " << buf << llendl;
						markBad(channels, buffer);
						break;
					}
					// we've found a header
					read_next_line = true;
					std::string name(buf, pos_colon - buf);
					std::string value(pos_colon + 2);
					LLStringUtil::toLower(name);
					if("content-length" == name)
					{
						lldebugs << "Content-Length: " << value << llendl;
						mContentLength = atoi(value.c_str());
					}
					else
					{
						LLStringUtil::trimTail(value);
						mHeaders[name] = value;
					}
				}
			}
		}
	}

	PUMP_DEBUG;
	// look for the end of stream based on 
	if(STATE_LOOKING_FOR_EOS == mState)
	{
		if(0 == mContentLength)
		{
			mState = STATE_DONE;
		}
		else if(buffer->countAfter(channels.in(), mLastRead) >= mContentLength)
		{
			mState = STATE_DONE;
		}
		// else more bytes should be coming.
	}

	PUMP_DEBUG;
	if(STATE_DONE == mState)
	{
		// hey, hey, we should have everything now, so we pass it to
		// a content handler.
		context[CONTEXT_REQUEST][CONTEXT_VERB] = mVerb;
		const LLHTTPNode* node = mRootNode.traverse(mPath, context);
		if(node)
		{
 			//llinfos << "LLHTTPResponder::process_impl found node for "
			//	<< mAbsPathAndQuery << llendl;

  			// Copy everything after mLast read to the out.
			LLBufferArray::segment_iterator_t seg_iter;

			buffer->lock();
			seg_iter = buffer->splitAfter(mLastRead);
			if(seg_iter != buffer->endSegment())
			{
				LLChangeChannel change(channels.in(), channels.out());
				++seg_iter;
				std::for_each(seg_iter, buffer->endSegment(), change);

#if 0
				seg_iter = buffer->beginSegment();
				char buf[1024];	  /*Flawfinder: ignore*/
				while(seg_iter != buffer->endSegment())
				{
					memcpy(buf, (*seg_iter).data(), (*seg_iter).size());	  /*Flawfinder: ignore*/
					buf[(*seg_iter).size()] = '\0';
					llinfos << (*seg_iter).getChannel() << ": " << buf
							<< llendl;
					++seg_iter;
				}
#endif
			}
			buffer->unlock();
			//
			// *FIX: get rid of extra bytes off the end
			//

			// Set up a chain which will prepend a content length and
			// HTTP headers.
			LLPumpIO::chain_t chain;
			chain.push_back(LLIOPipe::ptr_t(new LLIOFlush));
			context[CONTEXT_REQUEST]["path"] = mPath;
			context[CONTEXT_REQUEST]["query-string"] = mQuery;
			context[CONTEXT_REQUEST]["remote-host"]
				= mBuildContext["remote-host"];
			context[CONTEXT_REQUEST]["remote-port"]
				= mBuildContext["remote-port"];
			context[CONTEXT_REQUEST][CONTEXT_HEADERS] = mHeaders;

			const LLChainIOFactory* protocolHandler
				= node->getProtocolHandler();
			if (protocolHandler)
			{
				lldebugs << "HTTP context: " << context << llendl;
				protocolHandler->build(chain, context);
			}
			else
			{
				// this is a simple LLHTTPNode, so use LLHTTPPipe
				chain.push_back(LLIOPipe::ptr_t(new LLHTTPPipe(*node)));
			}

			// Add the header - which needs to have the same
			// channel information as the link before it since it
			// is part of the response.
			LLIOPipe* header = new LLHTTPResponseHeader;
			chain.push_back(LLIOPipe::ptr_t(header));

			// We need to copy all of the pipes _after_ this so
			// that the response goes out correctly.
			LLPumpIO::links_t current_links;
			pump->copyCurrentLinkInfo(current_links);
			LLPumpIO::links_t::iterator link_iter = current_links.begin();
			LLPumpIO::links_t::iterator links_end = current_links.end();
			bool after_this = false;
			for(; link_iter < links_end; ++link_iter)
			{
				if(after_this)
				{
					chain.push_back((*link_iter).mPipe);
				}
				else if(this == (*link_iter).mPipe.get())
				{
					after_this = true;
				}
			}
			
			// Do the final build of the chain, and send it on
			// it's way.
			LLChannelDescriptors chnl = channels;
			LLPumpIO::LLLinkInfo link;
			LLPumpIO::links_t links;
			LLPumpIO::chain_t::iterator it = chain.begin();
			LLPumpIO::chain_t::iterator end = chain.end();
			while(it != end)
			{
				link.mPipe = *it;
				link.mChannels = chnl;
				links.push_back(link);
				chnl = LLBufferArray::makeChannelConsumer(chnl);
				++it;
			}
			pump->addChain(
				links,
				buffer,
				context,
				DEFAULT_CHAIN_EXPIRY_SECS);

			status = STATUS_STOP;
		}
		else
		{
			llwarns << "LLHTTPResponder::process_impl didn't find a node for "
				<< mAbsPathAndQuery << llendl;
			LLBufferStream str(channels, buffer.get());
			mState = STATE_SHORT_CIRCUIT;
			str << HTTP_VERSION_STR << " 404 Not Found\r\n\r\n<html>\n"
				<< "<title>Not Found</title>\n<body>\nNode '" << mAbsPathAndQuery
				<< "' not found.\n</body>\n</html>\n";
		}
	}

	if(STATE_SHORT_CIRCUIT == mState)
	{
		//status = mNext->process(buffer, true, pump, context);
		status = STATUS_DONE;
	}
	PUMP_DEBUG;
	return status;
}


// static 
void LLIOHTTPServer::createPipe(LLPumpIO::chain_t& chain, 
        const LLHTTPNode& root, const LLSD& ctx)
{
	chain.push_back(LLIOPipe::ptr_t(new LLHTTPResponder(root, ctx)));
}


class LLHTTPResponseFactory : public LLChainIOFactory
{
public:
	bool build(LLPumpIO::chain_t& chain, LLSD ctx) const
	{
		LLIOHTTPServer::createPipe(chain, mTree, ctx);
		return true;
	}

	LLHTTPNode& getRootNode() { return mTree; }

private:
	LLHTTPNode mTree;
};


// static
LLHTTPNode& LLIOHTTPServer::create(
	apr_pool_t* pool, LLPumpIO& pump, U16 port)
{
	LLSocket::ptr_t socket = LLSocket::create(
        pool,
        LLSocket::STREAM_TCP,
        port);
    if(!socket)
    {
        llerrs << "Unable to initialize socket" << llendl;
    }

    LLHTTPResponseFactory* factory = new LLHTTPResponseFactory;
	boost::shared_ptr<LLChainIOFactory> factory_ptr(factory);

    LLIOServerSocket* server = new LLIOServerSocket(pool, socket, factory_ptr);

	LLPumpIO::chain_t chain;
    chain.push_back(LLIOPipe::ptr_t(server));
    pump.addChain(chain, NEVER_CHAIN_EXPIRY_SECS);

	return factory->getRootNode();
}

// static
void LLIOHTTPServer::setTimingCallback(timing_callback_t callback,
									   void* data)
{
	sTimingCallback = callback;
	sTimingCallbackData = data;
}
