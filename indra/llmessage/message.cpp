/** 
 * @file message.cpp
 * @brief LLMessageSystem class implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "message.h"

// system library includes
#if !LL_WINDOWS
// following header files required for inet_addr()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <time.h>
#include <iomanip>
#include <iterator>
#include <sstream>

#include "llapr.h"
#include "apr-1/apr_portable.h"
#include "apr-1/apr_network_io.h"
#include "apr-1/apr_poll.h"

// linden library headers
#include "indra_constants.h"
#include "lldarray.h"
#include "lldir.h"
#include "llerror.h"
#include "llerrorlegacy.h"
#include "llfasttimer.h"
#include "llhttpclient.h"
#include "llhttpsender.h"
#include "llmd5.h"
#include "llmessagebuilder.h"
#include "llmessageconfig.h"
#include "llpumpio.h"
#include "lltemplatemessagebuilder.h"
#include "lltemplatemessagereader.h"
#include "llmessagetemplate.h"
#include "llmessagetemplateparser.h"
#include "llsd.h"
#include "llsdmessagebuilder.h"
#include "llsdmessagereader.h"
#include "llsdserialize.h"
#include "llstring.h"
#include "lltransfermanager.h"
#include "lluuid.h"
#include "llxfermanager.h"
#include "timing.h"
#include "llquaternion.h"
#include "u64.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4math.h"
#include "lltransfertargetvfile.h"

// Constants
//const char* MESSAGE_LOG_FILENAME = "message.log";
static const F32 CIRCUIT_DUMP_TIMEOUT = 30.f;
static const S32 TRUST_TIME_WINDOW = 3;

// *NOTE: This needs to be moved into a seperate file so that it never gets
// included in the viewer.  30 Sep 2002 mark
// *NOTE: I don't think it's important that the messgage system tracks
// this since it must get set externally. 2004.08.25 Phoenix.
static std::string g_shared_secret;
std::string get_shared_secret();

class LLMessagePollInfo
{
public:
	apr_socket_t *mAPRSocketp;
	apr_pollfd_t mPollFD;
};

namespace
{
	class LLFnPtrResponder : public LLHTTPClient::Responder
	{
		LOG_CLASS(LLFnPtrResponder);
	public:
		LLFnPtrResponder(void (*callback)(void **,S32), void **callbackData, const std::string& name) :
			mCallback(callback),
			mCallbackData(callbackData),
			mMessageName(name)
		{
		}

		virtual void error(U32 status, const std::string& reason)
		{
			// don't spam when agent communication disconnected already
			if (status != 410)
			{
				llwarns << "error status " << status
						<< " for message " << mMessageName
						<< " reason " << reason << llendl;
			}
			// TODO: Map status in to useful error code.
			if(NULL != mCallback) mCallback(mCallbackData, LL_ERR_TCP_TIMEOUT);
		}
		
		virtual void result(const LLSD& content)
		{
			if(NULL != mCallback) mCallback(mCallbackData, LL_ERR_NOERR);
		}

	private:

		void (*mCallback)(void **,S32);    
		void **mCallbackData;
		std::string mMessageName;
	};
}


class LLTrustedMessageService : public LLHTTPNode
{
	virtual bool validate(const std::string& name, LLSD& context) const
		{ return true; }

	virtual void post(LLHTTPNode::ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const;
};

//virtual
void LLTrustedMessageService::post(LLHTTPNode::ResponsePtr response,
								   const LLSD& context,
								   const LLSD& input) const
{
	std::string name = context["request"]["wildcard"]["message-name"];
	std::string senderIP = context["request"]["remote-host"];
	std::string senderPort = context["request"]["headers"]
		["x-secondlife-udp-listen-port"];

	LLSD message_data;
	std::string sender = senderIP + ":" + senderPort;
	message_data["sender"] = sender;
	message_data["body"] = input;
	
	// untrusted senders should not have access to the trusted message
	// service, but this can happen in development, so check and warn
	LLMessageConfig::SenderTrust trust =
		LLMessageConfig::getSenderTrustedness(name);
	if ((trust == LLMessageConfig::TRUSTED ||
		 (trust == LLMessageConfig::NOT_SET &&
		  gMessageSystem->isTrustedMessage(name)))
		 && !gMessageSystem->isTrustedSender(LLHost(sender)))
	{
		llwarns << "trusted message POST to /trusted-message/" 
				<< name << " from unknown or untrusted sender "
				<< sender << llendl;
		response->status(403, "Unknown or untrusted sender");
	}
	else
	{
		LLMessageSystem::dispatch(name, message_data, response);
	}
}

class LLMessageHandlerBridge : public LLHTTPNode
{
	virtual bool validate(const std::string& name, LLSD& context) const
		{ return true; }

	virtual void post(LLHTTPNode::ResponsePtr response, const LLSD& context, 
					  const LLSD& input) const;
};

//virtual 
void LLMessageHandlerBridge::post(LLHTTPNode::ResponsePtr response, 
							const LLSD& context, const LLSD& input) const
{
	std::string name = context["request"]["wildcard"]["message-name"];
	char* namePtr = gMessageStringTable.getString(name.c_str());
	
	lldebugs << "Setting mLastSender " << input["sender"].asString() << llendl;
	gMessageSystem->mLastSender = LLHost(input["sender"].asString());
	gMessageSystem->mPacketsIn += 1;
	gMessageSystem->mLLSDMessageReader->setMessage(namePtr, input["body"]);
	gMessageSystem->mMessageReader = gMessageSystem->mLLSDMessageReader;
	
	if(gMessageSystem->callHandler(namePtr, false, gMessageSystem))
	{
		response->result(LLSD());
	}
	else
	{
		response->notFound();
	}
}

LLHTTPRegistration<LLMessageHandlerBridge>
	gHTTPRegistrationMessageWildcard("/message/<message-name>");

LLHTTPRegistration<LLTrustedMessageService>
	gHTTPRegistrationTrustedMessageWildcard("/trusted-message/<message-name>");

//virtual
LLUseCircuitCodeResponder::~LLUseCircuitCodeResponder()
{
	// even abstract base classes need a concrete destructor
}

static const char* nullToEmpty(const char* s)
{
	static char emptyString[] = "";
	return s? s : emptyString;
}

void LLMessageSystem::init()
{
	// initialize member variables
	mVerboseLog = FALSE;

	mbError = FALSE;
	mErrorCode = 0;
	mSendReliable = FALSE;

	mUnackedListDepth = 0;
	mUnackedListSize = 0;
	mDSMaxListDepth = 0;

	mNumberHighFreqMessages = 0;
	mNumberMediumFreqMessages = 0;
	mNumberLowFreqMessages = 0;
	mPacketsIn = mPacketsOut = 0;
	mBytesIn = mBytesOut = 0;
	mCompressedPacketsIn = mCompressedPacketsOut = 0;
	mReliablePacketsIn = mReliablePacketsOut = 0;

	mCompressedBytesIn = 0;
	mCompressedBytesOut = 0;
	mUncompressedBytesIn = 0;
	mUncompressedBytesOut = 0;
	mTotalBytesIn = 0;
	mTotalBytesOut = 0;

    mDroppedPackets = 0;            // total dropped packets in
    mResentPackets = 0;             // total resent packets out
    mFailedResendPackets = 0;       // total resend failure packets out
    mOffCircuitPackets = 0;         // total # of off-circuit packets rejected
    mInvalidOnCircuitPackets = 0;   // total # of on-circuit packets rejected

	mOurCircuitCode = 0;

	mIncomingCompressedSize = 0;
	mCurrentRecvPacketID = 0;

	mMessageFileVersionNumber = 0.f;

	mTimingCallback = NULL;
	mTimingCallbackData = NULL;

	mMessageBuilder = NULL;
	mMessageReader = NULL;
}

// Read file and build message templates
LLMessageSystem::LLMessageSystem(const char *filename, U32 port, 
								 S32 version_major,
								 S32 version_minor,
								 S32 version_patch)
{
	init();

	mSystemVersionMajor = version_major;
	mSystemVersionMinor = version_minor;
	mSystemVersionPatch = version_patch;
	mSystemVersionServer = 0;
	mVersionFlags = 0x0;

	// default to not accepting packets from not alive circuits
	mbProtected = TRUE;

	mSendPacketFailureCount = 0;

	mCircuitPrintFreq = 60.f;		// seconds

	loadTemplateFile(filename);

	mTemplateMessageBuilder = new LLTemplateMessageBuilder(mMessageTemplates);
	mLLSDMessageBuilder = new LLSDMessageBuilder();
	mMessageBuilder = NULL;

	mTemplateMessageReader = new LLTemplateMessageReader(mMessageNumbers);
	mLLSDMessageReader = new LLSDMessageReader();
	mMessageReader = NULL;

	// initialize various bits of net info
	mSocket = 0;
	mPort = port;

	S32 error = start_net(mSocket, mPort);
	if (error != 0)
	{
		mbError = TRUE;
		mErrorCode = error;
	}
	//llinfos <<  << "*** port: " << mPort << llendl;

	//
	// Create the data structure that we can poll on
	//
	if (!gAPRPoolp)
	{
		llerrs << "No APR pool before message system initialization!" << llendl;
		ll_init_apr();
	}
	apr_socket_t *aprSocketp = NULL;
	apr_os_sock_put(&aprSocketp, (apr_os_sock_t*)&mSocket, gAPRPoolp);

	mPollInfop = new LLMessagePollInfo;
	mPollInfop->mAPRSocketp = aprSocketp;
	mPollInfop->mPollFD.p = gAPRPoolp;
	mPollInfop->mPollFD.desc_type = APR_POLL_SOCKET;
	mPollInfop->mPollFD.reqevents = APR_POLLIN;
	mPollInfop->mPollFD.rtnevents = 0;
	mPollInfop->mPollFD.desc.s = aprSocketp;
	mPollInfop->mPollFD.client_data = NULL;

	F64 mt_sec = getMessageTimeSeconds();
	mResendDumpTime = mt_sec;
	mMessageCountTime = mt_sec;
	mCircuitPrintTime = mt_sec;
	mCurrentMessageTimeSeconds = mt_sec;

	// Constants for dumping output based on message processing time/count
	mNumMessageCounts = 0;
	mMaxMessageCounts = 200; // >= 0 means dump warnings
	mMaxMessageTime   = 1.f;

	mTrueReceiveSize = 0;
}



// Read file and build message templates
void LLMessageSystem::loadTemplateFile(const char* filename)
{
	if(!filename)
	{
		llerrs << "No template filename specified" << llendl;
		mbError = TRUE;
		return;
	}

	LLString template_body;
	if(!LLString::read(template_body, filename))
	{
		llwarns << "Failed to open template: " << filename << llendl;
		mbError = TRUE;
		return;
	}
	
	LLTemplateTokenizer tokens(template_body);
	LLTemplateParser parsed(tokens);
	mMessageFileVersionNumber = parsed.getVersion();
	for(LLTemplateParser::message_iterator iter = parsed.getMessagesBegin();
		iter != parsed.getMessagesEnd();
		iter++)
	{
		addTemplate(*iter);
	}
}


LLMessageSystem::~LLMessageSystem()
{
	mMessageTemplates.clear(); // don't delete templates.
	for_each(mMessageNumbers.begin(), mMessageNumbers.end(), DeletePairedPointer());
	mMessageNumbers.clear();
	
	if (!mbError)
	{
		end_net(mSocket);
	}
	
	delete mTemplateMessageReader;
	mTemplateMessageReader = NULL;
	mMessageReader = NULL;

	delete mTemplateMessageBuilder;
	mTemplateMessageBuilder = NULL;
	mMessageBuilder = NULL;

	delete mLLSDMessageReader;
	mLLSDMessageReader = NULL;

	delete mLLSDMessageBuilder;
	mLLSDMessageBuilder = NULL;

	delete mPollInfop;
	mPollInfop = NULL;

	mIncomingCompressedSize = 0;
	mCurrentRecvPacketID = 0;
}

void LLMessageSystem::clearReceiveState()
{
	mCurrentRecvPacketID = 0;
	mIncomingCompressedSize = 0;
	mLastSender.invalidate();
	mMessageReader->clearMessage();
}


BOOL LLMessageSystem::poll(F32 seconds)
{
	S32 num_socks;
	apr_status_t status;
	status = apr_poll(&(mPollInfop->mPollFD), 1, &num_socks,(U64)(seconds*1000000.f));
	if (status != APR_TIMEUP)
	{
		ll_apr_warn_status(status);
	}
	if (num_socks)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool LLMessageSystem::isTrustedSender(const LLHost& host) const
{
	LLCircuitData* cdp = mCircuitInfo.findCircuit(host);
	if(NULL == cdp)
	{
		return false;
	}
	return cdp->getTrusted();
}

static LLMessageSystem::message_template_name_map_t::const_iterator 
findTemplate(const LLMessageSystem::message_template_name_map_t& templates, 
			 std::string name)
{
	const char* namePrehash = gMessageStringTable.getString(name.c_str());
	if(NULL == namePrehash) {return templates.end();}
	return templates.find(namePrehash);
}

bool LLMessageSystem::isTrustedMessage(const std::string& name) const
{
	message_template_name_map_t::const_iterator iter = 
		findTemplate(mMessageTemplates, name);
	if(iter == mMessageTemplates.end()) {return false;}
	return iter->second->getTrust() == MT_TRUST;
}

bool LLMessageSystem::isUntrustedMessage(const std::string& name) const
{
	message_template_name_map_t::const_iterator iter = 
		findTemplate(mMessageTemplates, name);
	if(iter == mMessageTemplates.end()) {return false;}
	return iter->second->getTrust() == MT_NOTRUST;
}

LLCircuitData* LLMessageSystem::findCircuit(const LLHost& host,
											bool resetPacketId)
{
	LLCircuitData* cdp = mCircuitInfo.findCircuit(host);
	if (!cdp)
	{
		// This packet comes from a circuit we don't know about.
		
		// Are we rejecting off-circuit packets?
		if (mbProtected)
		{
			// cdp is already NULL, so we don't need to unset it.
		}
		else
		{
			// nope, open the new circuit
			cdp = mCircuitInfo.addCircuitData(host, mCurrentRecvPacketID);

			if(resetPacketId)
			{
				// I added this - I think it's correct - DJS
				// reset packet in ID
				cdp->setPacketInID(mCurrentRecvPacketID);
			}
			// And claim the packet is on the circuit we just added.
		}
	}
	else
	{
		// this is an old circuit. . . is it still alive?
		if (!cdp->isAlive())
		{
			// nope. don't accept if we're protected
			if (mbProtected)
			{
				// don't accept packets from unexpected sources
				cdp = NULL;
			}
			else
			{
				// wake up the circuit
				cdp->setAlive(TRUE);
				
				if(resetPacketId)
				{
					// reset packet in ID
					cdp->setPacketInID(mCurrentRecvPacketID);
				}
			}
		}
	}
	return cdp;
}

// Returns TRUE if a valid, on-circuit message has been received.
BOOL LLMessageSystem::checkMessages( S64 frame_count )
{
	// Pump 
	BOOL	valid_packet = FALSE;
	mMessageReader = mTemplateMessageReader;

	LLTransferTargetVFile::updateQueue();
	
	if (!mNumMessageCounts)
	{
		// This is the first message being handled after a resetReceiveCounts,
		// we must be starting the message processing loop.  Reset the timers.
		mCurrentMessageTimeSeconds = totalTime() * SEC_PER_USEC;
		mMessageCountTime = getMessageTimeSeconds();
	}

	// loop until either no packets or a valid packet
	// i.e., burn through packets from unregistered circuits
	S32 receive_size = 0;
	do
	{
		clearReceiveState();
		
		BOOL recv_reliable = FALSE;
		BOOL recv_resent = FALSE;
		S32 acks = 0;
		S32 true_rcv_size = 0;

		U8* buffer = mTrueReceiveBuffer;
		
		mTrueReceiveSize = mPacketRing.receivePacket(mSocket, (char *)mTrueReceiveBuffer);
		// If you want to dump all received packets into SecondLife.log, uncomment this
		//dumpPacketToLog();
		
		receive_size = mTrueReceiveSize;
		mLastSender = mPacketRing.getLastSender();
		
		if (receive_size < (S32) LL_MINIMUM_VALID_PACKET_SIZE)
		{
			// A receive size of zero is OK, that means that there are no more packets available.
			// Ones that are non-zero but below the minimum packet size are worrisome.
			if (receive_size > 0)
			{
				llwarns << "Invalid (too short) packet discarded " << receive_size << llendl;
				callExceptionFunc(MX_PACKET_TOO_SHORT);
			}
			// no data in packet receive buffer
			valid_packet = FALSE;
		}
		else
		{
			LLHost host;
			LLCircuitData* cdp;
			
			// note if packet acks are appended.
			if(buffer[0] & LL_ACK_FLAG)
			{
				acks += buffer[--receive_size];
				true_rcv_size = receive_size;
				if(receive_size >= ((S32)(acks * sizeof(TPACKETID) + LL_MINIMUM_VALID_PACKET_SIZE)))
				{
					receive_size -= acks * sizeof(TPACKETID);
				}
				else
				{
					// mal-formed packet. ignore it and continue with
					// the next one
					llwarns << "Malformed packet received. Packet size "
						<< receive_size << " with invalid no. of acks " << acks
						<< llendl;
					valid_packet = FALSE;
					continue;
				}
			}

			// process the message as normal
			mIncomingCompressedSize = zeroCodeExpand(&buffer, &receive_size);
			mCurrentRecvPacketID = ntohl(*((U32*)(&buffer[1])));
			host = getSender();

			const bool resetPacketId = true;
			cdp = findCircuit(host, resetPacketId);

			// At this point, cdp is now a pointer to the circuit that
			// this message came in on if it's valid, and NULL if the
			// circuit was bogus.

			if(cdp && (acks > 0) && ((S32)(acks * sizeof(TPACKETID)) < (true_rcv_size)))
			{
				TPACKETID packet_id;
				U32 mem_id=0;
				for(S32 i = 0; i < acks; ++i)
				{
					true_rcv_size -= sizeof(TPACKETID);
					memcpy(&mem_id, &mTrueReceiveBuffer[true_rcv_size], /* Flawfinder: ignore*/
					     sizeof(TPACKETID));
					packet_id = ntohl(mem_id);
					//llinfos << "got ack: " << packet_id << llendl;
					cdp->ackReliablePacket(packet_id);
				}
				if (!cdp->getUnackedPacketCount())
				{
					// Remove this circuit from the list of circuits with unacked packets
					mCircuitInfo.mUnackedCircuitMap.erase(cdp->mHost);
				}
			}

			if (buffer[0] & LL_RELIABLE_FLAG)
			{
				recv_reliable = TRUE;
			}
			if (buffer[0] & LL_RESENT_FLAG)
			{
				recv_resent = TRUE;
				if (cdp && cdp->isDuplicateResend(mCurrentRecvPacketID))
				{
					// We need to ACK here to suppress
					// further resends of packets we've
					// already seen.
					if (recv_reliable)
					{
						//mAckList.addData(new LLPacketAck(host, mCurrentRecvPacketID));
						// ***************************************
						// TESTING CODE
						//if(mCircuitInfo.mCurrentCircuit->mHost != host)
						//{
						//	llwarns << "DISCARDED PACKET HOST MISMATCH! HOST: "
						//			<< host << " CIRCUIT: "
						//			<< mCircuitInfo.mCurrentCircuit->mHost
						//			<< llendl;
						//}
						// ***************************************
						//mCircuitInfo.mCurrentCircuit->mAcks.put(mCurrentRecvPacketID);
						cdp->collectRAck(mCurrentRecvPacketID);
					}
								 
					//llinfos << "Discarding duplicate resend from " << host << llendl;
					if(mVerboseLog)
					{
						std::ostringstream str;
						str << "MSG: <- " << host;
						char buffer[MAX_STRING]; /* Flawfinder: ignore*/
						snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", receive_size, (mIncomingCompressedSize ? mIncomingCompressedSize : receive_size), mCurrentRecvPacketID);	/* Flawfinder: ignore */
						str << buffer << "(unknown)"
							<< (recv_reliable ? " reliable" : "")
							<< " resent "
							<< ((acks > 0) ? "acks" : "")
							<< " DISCARD DUPLICATE";
						llinfos << str.str() << llendl;
					}
					mPacketsIn++;
					valid_packet = FALSE;
					continue;
				}
			}

			// UseCircuitCode can be a valid, off-circuit packet.
			// But we don't want to acknowledge UseCircuitCode until the circuit is
			// available, which is why the acknowledgement test is done above.  JC

			valid_packet = mTemplateMessageReader->validateMessage(buffer, 
														   receive_size,
														   host);

			// UseCircuitCode is allowed in even from an invalid circuit, so that
			// we can toss circuits around.
			if(valid_packet && !cdp && 
			   (mTemplateMessageReader->getMessageName() != _PREHASH_UseCircuitCode))
			{
				logMsgFromInvalidCircuit( host, recv_reliable );
				clearReceiveState();
				valid_packet = FALSE;
			}

			if(valid_packet && cdp && !cdp->getTrusted() && 
				mTemplateMessageReader->isTrusted())
			{
				logTrustedMsgFromUntrustedCircuit( host );
				clearReceiveState();

				sendDenyTrustedCircuit(host);
				valid_packet = FALSE;
			}

			if (valid_packet
			&& mTemplateMessageReader->isBanned(cdp && cdp->getTrusted()))
			{
				llwarns << "LLMessageSystem::checkMessages "
					<< "received banned message "
					<< mTemplateMessageReader->getMessageName()
					<< " from "
					<< ((cdp && cdp->getTrusted()) ? "trusted " : "untrusted ")
					<< host << llendl;
				clearReceiveState();
				valid_packet = FALSE;
			}
			
			if( valid_packet )
			{
				logValidMsg(cdp, host, recv_reliable, recv_resent, (BOOL)(acks>0) );

				valid_packet = mTemplateMessageReader->readMessage(buffer, host);
			}

			// It's possible that the circuit went away, because ANY message can disable the circuit
			// (for example, UseCircuit, CloseCircuit, DisableSimulator).  Find it again.
			cdp = mCircuitInfo.findCircuit(host);

			if (valid_packet)
			{
				// enable this for output of message names
				//llinfos << "< \"" << mTemplateMessageReader->getMessageName()
						//<< "\"" << llendl;

				/* Code for dumping the complete contents of a message.  Keep for future use in optimizing messages.
				if( 1 )
				{
					static char* object_update = gMessageStringTable.getString("ObjectUpdate");
					if(object_update == mTemplateMessageReader->getMessageName() )
					{
						llinfos << "ObjectUpdate:" << llendl;
						U32 i;
						llinfos << "    Zero Encoded: " << zero_unexpanded_size << llendl;
						for( i = 0; i<zero_unexpanded_size; i++ )
						{
							llinfos << "     " << i << ": " << (U32) zero_unexpanded_buffer[i] << llendl;
						}
						llinfos << "" << llendl;

						llinfos << "    Zero Unencoded: " << receive_size << llendl;
						for( i = 0; i<receive_size; i++ )
						{
							llinfos << "     " << i << ": " << (U32) buffer[i] << llendl;
						}
						llinfos << "" << llendl;

						llinfos << "    Blocks and variables: " << llendl;
						S32 byte_count = 0;
						for (LLMessageTemplate::message_block_map_t::iterator
								 iter = mCurrentRMessageTemplate->mMemberBlocks.begin(),
								 end = mCurrentRMessageTemplate->mMemberBlocks.end();
							 iter != end; iter++)
						{
							LLMessageBlock* block = iter->second;
							const char* block_name = block->mName;
							for (LLMsgBlkData::msg_var_data_map_t::iterator
									 iter = block->mMemberVariables.begin(),
									 end = block->mMemberVariables.end();
								 iter != end; iter++)
							{
								const char* var_name = iter->first;
								
								if( getNumberOfBlocksFast( block_name ) < 1 )
								{
									llinfos << var_name << " has no blocks" << llendl;
								}
								for( S32 blocknum = 0; blocknum < getNumberOfBlocksFast( block_name ); blocknum++ )
								{
									char *bnamep = (char *)block_name + blocknum; // this works because it's just a hash.  The bnamep is never derefference
									char *vnamep = (char *)var_name; 

									LLMsgBlkData *msg_block_data = mCurrentRMessageData->mMemberBlocks[bnamep];

									char errmsg[1024];
									if (!msg_block_data)
									{
										sprintf(errmsg, "Block %s #%d not in message %s", block_name, blocknum, mCurrentRMessageData->mName);
										llerrs << errmsg << llendl;
									}

									LLMsgVarData vardata = msg_block_data->mMemberVarData[vnamep];

									if (!vardata.getName())
									{
										sprintf(errmsg, "Variable %s not in message %s block %s", vnamep, mCurrentRMessageData->mName, bnamep);
										llerrs << errmsg << llendl;
									}

									const S32 vardata_size = vardata.getSize();
									if( vardata_size )
									{
										for( i = 0; i < vardata_size; i++ )
										{
											byte_count++;
											llinfos << block_name << " " << var_name << " [" << blocknum << "][" << i << "]= " << (U32)(((U8*)vardata.getData())[i]) << llendl;
										}
									}
									else
									{
										llinfos << block_name << " " << var_name << " [" << blocknum << "] 0 bytes" << llendl;
									}
								}
							}
						}
						llinfos << "Byte count =" << byte_count << llendl;
					}
				}
				*/

				mPacketsIn++;
				mBytesIn += mTrueReceiveSize;
				
				// ACK here for	valid packets that we've seen
				// for the first time.
				if (cdp && recv_reliable)
				{
					// Add to the recently received list for duplicate suppression
					cdp->mRecentlyReceivedReliablePackets[mCurrentRecvPacketID] = getMessageTimeUsecs();

					// Put it onto the list of packets to be acked
					cdp->collectRAck(mCurrentRecvPacketID);
					mReliablePacketsIn++;
				}
			}
			else
			{
				if (mbProtected  && (!cdp))
				{
					llwarns << "Invalid Packet from invalid circuit " << host << llendl;
					mOffCircuitPackets++;
				}
				else
				{
					mInvalidOnCircuitPackets++;
				}
			}

			// Code for dumping the complete contents of a message 
			// delete [] zero_unexpanded_buffer;
		}
	} while (!valid_packet && receive_size > 0);

	F64 mt_sec = getMessageTimeSeconds();
	// Check to see if we need to print debug info
	if ((mt_sec - mCircuitPrintTime) > mCircuitPrintFreq)
	{
		dumpCircuitInfo();
		mCircuitPrintTime = mt_sec;
	}

	if( !valid_packet )
	{
		clearReceiveState();
	}

	return valid_packet;
}

S32	LLMessageSystem::getReceiveBytes() const
{
	if (getReceiveCompressedSize())
	{
		return getReceiveCompressedSize() * 8;
	}
	else
	{
		return getReceiveSize() * 8;
	}
}


void LLMessageSystem::processAcks()
{
	F64 mt_sec = getMessageTimeSeconds();
	{
		gTransferManager.updateTransfers();

		if (gXferManager)
		{
			gXferManager->retransmitUnackedPackets();
		}

		if (gAssetStorage)
		{
			gAssetStorage->checkForTimeouts();
		}
	}

	BOOL dump = FALSE;
	{
		// Check the status of circuits
		mCircuitInfo.updateWatchDogTimers(this);

		//resend any necessary packets
		mCircuitInfo.resendUnackedPackets(mUnackedListDepth, mUnackedListSize);

		//cycle through ack list for each host we need to send acks to
		mCircuitInfo.sendAcks();

		if (!mDenyTrustedCircuitSet.empty())
		{
			llinfos << "Sending queued DenyTrustedCircuit messages." << llendl;
			for (host_set_t::iterator hostit = mDenyTrustedCircuitSet.begin(); hostit != mDenyTrustedCircuitSet.end(); ++hostit)
			{
				reallySendDenyTrustedCircuit(*hostit);
			}
			mDenyTrustedCircuitSet.clear();
		}

		if (mMaxMessageCounts >= 0)
		{
			if (mNumMessageCounts >= mMaxMessageCounts)
			{
				dump = TRUE;
			}
		}

		if (mMaxMessageTime >= 0.f)
		{
			// This is one of the only places where we're required to get REAL message system time.
			mReceiveTime = (F32)(getMessageTimeSeconds(TRUE) - mMessageCountTime);
			if (mReceiveTime > mMaxMessageTime)
			{
				dump = TRUE;
			}
		}
	}

	if (dump)
	{
		dumpReceiveCounts();
	}
	resetReceiveCounts();

	if ((mt_sec - mResendDumpTime) > CIRCUIT_DUMP_TIMEOUT)
	{
		mResendDumpTime = mt_sec;
		mCircuitInfo.dumpResends();
	}
}

void LLMessageSystem::copyMessageRtoS()
{
	// NOTE: babbage: switch builder to match reader to avoid
	// converting message format
	if(mMessageReader == mTemplateMessageReader)
	{
		mMessageBuilder = mTemplateMessageBuilder;
	}
	else
	{
		mMessageBuilder = mLLSDMessageBuilder;
	}
	mSendReliable = FALSE;
	mMessageBuilder->newMessage(mMessageReader->getMessageName());
	mMessageReader->copyToBuilder(*mMessageBuilder);
}

void LLMessageSystem::clearMessage()
{
	mSendReliable = FALSE;
	mMessageBuilder->clearMessage();
}

// set block to add data to within current message
void LLMessageSystem::nextBlockFast(const char *blockname)
{
	mMessageBuilder->nextBlock(blockname);
}

BOOL LLMessageSystem::isSendFull(const char* blockname)
{
	char* stringTableName = NULL;
	if(NULL != blockname)
	{
		stringTableName = gMessageStringTable.getString(blockname);
	}
	return isSendFullFast(stringTableName);
}

BOOL LLMessageSystem::isSendFullFast(const char* blockname)
{
	return mMessageBuilder->isMessageFull(blockname);
}


// blow away the last block of a message, return FALSE if that leaves no blocks or there wasn't a block to remove
// TODO: Babbage: Remove this horror.
BOOL LLMessageSystem::removeLastBlock()
{
	return mMessageBuilder->removeLastBlock();
}

S32 LLMessageSystem::sendReliable(const LLHost &host)
{
	return sendReliable(host, LL_DEFAULT_RELIABLE_RETRIES, TRUE, LL_PING_BASED_TIMEOUT_DUMMY, NULL, NULL);
}


S32 LLMessageSystem::sendSemiReliable(const LLHost &host, void (*callback)(void **,S32), void ** callback_data)
{
	F32 timeout;

	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		timeout = llmax(LL_MINIMUM_SEMIRELIABLE_TIMEOUT_SECONDS,
						LL_SEMIRELIABLE_TIMEOUT_FACTOR * cdp->getPingDelayAveraged());
	}
	else
	{
		timeout = LL_SEMIRELIABLE_TIMEOUT_FACTOR * LL_AVERAGED_PING_MAX;
	}

	const S32 retries = 0;
	const BOOL ping_based_timeout = FALSE;
	return sendReliable(host, retries, ping_based_timeout, timeout, callback, callback_data);
}

// send the message via a UDP packet
S32 LLMessageSystem::sendReliable(	const LLHost &host, 
									S32 retries, 
									BOOL ping_based_timeout,
									F32 timeout, 
									void (*callback)(void **,S32), 
									void ** callback_data)
{
	if (ping_based_timeout)
	{
	    LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	    if (cdp)
	    {
		    timeout = llmax(LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS, LL_RELIABLE_TIMEOUT_FACTOR * cdp->getPingDelayAveraged());
	    }
	    else
	    {
		    timeout = llmax(LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS, LL_RELIABLE_TIMEOUT_FACTOR * LL_AVERAGED_PING_MAX);
	    }
	}

	mSendReliable = TRUE;
	mReliablePacketParams.set(host, retries, ping_based_timeout, timeout, 
		callback, callback_data, 
		const_cast<char*>(mMessageBuilder->getMessageName()));
	return sendMessage(host);
}

void LLMessageSystem::forwardMessage(const LLHost &host)
{
	copyMessageRtoS();
	sendMessage(host);
}

void LLMessageSystem::forwardReliable(const LLHost &host)
{
	copyMessageRtoS();
	sendReliable(host);
}

void LLMessageSystem::forwardReliable(const U32 circuit_code)
{
	copyMessageRtoS();
	sendReliable(findHost(circuit_code));
}

S32 LLMessageSystem::flushSemiReliable(const LLHost &host, void (*callback)(void **,S32), void ** callback_data)
{
	F32 timeout; 

	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		timeout = llmax(LL_MINIMUM_SEMIRELIABLE_TIMEOUT_SECONDS,
						LL_SEMIRELIABLE_TIMEOUT_FACTOR * cdp->getPingDelayAveraged());
	}
	else
	{
		timeout = LL_SEMIRELIABLE_TIMEOUT_FACTOR * LL_AVERAGED_PING_MAX;
	}

	S32 send_bytes = 0;
	if (mMessageBuilder->getMessageSize())
	{
		mSendReliable = TRUE;
		// No need for ping-based retry as not going to retry
		mReliablePacketParams.set(host, 0, FALSE, timeout, callback, 
								  callback_data, 
								  const_cast<char*>(mMessageBuilder->getMessageName()));
		send_bytes = sendMessage(host);
		clearMessage();
	}
	else
	{
		delete callback_data;
	}
	return send_bytes;
}

S32 LLMessageSystem::flushReliable(const LLHost &host)
{
	S32 send_bytes = 0;
	if (mMessageBuilder->getMessageSize())
	{
		send_bytes = sendReliable(host);
	}
	clearMessage();
	return send_bytes;
}

LLHTTPClient::ResponderPtr LLMessageSystem::createResponder(const std::string& name)
{
	if(mSendReliable)
	{
		return new LLFnPtrResponder(mReliablePacketParams.mCallback,
									mReliablePacketParams.mCallbackData,
									name);
	}
	else
	{
		llwarns << "LLMessageSystem::sendMessage: Sending unreliable "
				<< mMessageBuilder->getMessageName() << " message via HTTP"
				<< llendl;
		return new LLFnPtrResponder(NULL, NULL,
									mMessageBuilder->getMessageName());
	}
}

// This can be called from signal handlers,
// so should should not use llinfos.
S32 LLMessageSystem::sendMessage(const LLHost &host)
{
	if (! mMessageBuilder->isBuilt())
	{
		mSendSize = mMessageBuilder->buildMessage(
			mSendBuffer,
			MAX_BUFFER_SIZE,
			0);
	}

	if (!(host.isOk()))    // if port and ip are zero, don't bother trying to send the message
	{
		return 0;
	}

	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (!cdp)
	{
		// this is a new circuit!
		// are we protected?
		if (mbProtected)
		{
			// yup! don't send packets to an unknown circuit
			if(mVerboseLog)
			{
				llinfos << "MSG: -> " << host << "\tUNKNOWN CIRCUIT:\t"
						<< mMessageBuilder->getMessageName() << llendl;
			}
			llwarns << "sendMessage - Trying to send "
					<< mMessageBuilder->getMessageName() << " on unknown circuit "
					<< host << llendl;
			return 0;
		}
		else
		{
			// nope, open the new circuit

			cdp = mCircuitInfo.addCircuitData(host, 0);
		}
	}
	else
	{
		// this is an old circuit. . . is it still alive?
		if (!cdp->isAlive())
		{
			// nope. don't send to dead circuits
			if(mVerboseLog)
			{
				llinfos << "MSG: -> " << host << "\tDEAD CIRCUIT\t\t"
						<< mMessageBuilder->getMessageName() << llendl;
			}
			llwarns << "sendMessage - Trying to send message "
					<< mMessageBuilder->getMessageName() << " to dead circuit "
					<< host << llendl;
			return 0;
		}
	}

	// NOTE: babbage: LLSD message -> HTTP, template message -> UDP
	if(mMessageBuilder == mLLSDMessageBuilder)
	{
		LLSD message = mLLSDMessageBuilder->getMessage();
		
		const LLHTTPSender& sender = LLHTTPSender::getSender(host);
		sender.send(host, mLLSDMessageBuilder->getMessageName(),
					message, createResponder(mLLSDMessageBuilder->getMessageName()));

		mSendReliable = FALSE;
		mReliablePacketParams.clear();
		return 1;
	}

	// zero out the flags and packetid. Subtract 1 here so that we do
	// not overwrite the offset if it was set set in buildMessage().
	memset(mSendBuffer, 0, LL_PACKET_ID_SIZE - 1); 

	// add the send id to the front of the message
	cdp->nextPacketOutID();

	// Packet ID size is always 4
	*((S32*)&mSendBuffer[PHL_PACKET_ID]) = htonl(cdp->getPacketOutID());

	// Compress the message, which will usually reduce its size.
	U8 * buf_ptr = (U8 *)mSendBuffer;
	U32 buffer_length = mSendSize;
	mMessageBuilder->compressMessage(buf_ptr, buffer_length);

	if (buffer_length > 1500)
	{
		if((mMessageBuilder->getMessageName() != _PREHASH_ChildAgentUpdate)
		   && (mMessageBuilder->getMessageName() != _PREHASH_SendXferPacket))
		{
			llwarns << "sendMessage - Trying to send "
					<< ((buffer_length > 4000) ? "EXTRA " : "")
					<< "BIG message " << mMessageBuilder->getMessageName() << " - "
					<< buffer_length << llendl;
		}
	}
	if (mSendReliable)
	{
		buf_ptr[0] |= LL_RELIABLE_FLAG;

		if (!cdp->getUnackedPacketCount())
		{
			// We are adding the first packed onto the unacked packet list(s)
			// Add this circuit to the list of circuits with unacked packets
			mCircuitInfo.mUnackedCircuitMap[cdp->mHost] = cdp;
		}

		cdp->addReliablePacket(mSocket,buf_ptr,buffer_length, &mReliablePacketParams);
		mReliablePacketsOut++;
	}

	// tack packet acks onto the end of this message
	S32 space_left = (MTUBYTES - buffer_length) / sizeof(TPACKETID); // space left for packet ids
	S32 ack_count = (S32)cdp->mAcks.size();
	BOOL is_ack_appended = FALSE;
	std::vector<TPACKETID> acks;
	if((space_left > 0) && (ack_count > 0) && 
	   (mMessageBuilder->getMessageName() != _PREHASH_PacketAck))
	{
		buf_ptr[0] |= LL_ACK_FLAG;
		S32 append_ack_count = llmin(space_left, ack_count);
		const S32 MAX_ACKS = 250;
		append_ack_count = llmin(append_ack_count, MAX_ACKS);
		std::vector<TPACKETID>::iterator iter = cdp->mAcks.begin();
		std::vector<TPACKETID>::iterator last = cdp->mAcks.begin();
		last += append_ack_count;
		TPACKETID packet_id;
		for( ; iter != last ; ++iter)
		{
			// grab the next packet id.
			packet_id = (*iter);
			if(mVerboseLog)
			{
				acks.push_back(packet_id);
			}

			// put it on the end of the buffer
			packet_id = htonl(packet_id);

			if((S32)(buffer_length + sizeof(TPACKETID)) < MAX_BUFFER_SIZE)
			{
			    memcpy(&buf_ptr[buffer_length], &packet_id, sizeof(TPACKETID));	/* Flawfinder: ignore */
			    // Do the accounting
			    buffer_length += sizeof(TPACKETID);
			}
			else
			{
			    // Just reporting error is likely not enough.  Need to
			    // check how to abort or error out gracefully from
			    // this function. XXXTBD
				// *NOTE: Actually hitting this error would indicate
				// the calculation above for space_left, ack_count,
				// append_acout_count is incorrect or that
				// MAX_BUFFER_SIZE has fallen below MTU which is bad
				// and probably programmer error.
			    llerrs << "Buffer packing failed due to size.." << llendl;
			}
		}

		// clean up the source
		cdp->mAcks.erase(cdp->mAcks.begin(), last);

		// tack the count in the final byte
		U8 count = (U8)append_ack_count;
		buf_ptr[buffer_length++] = count;
		is_ack_appended = TRUE;
	}

	BOOL success;
	success = mPacketRing.sendPacket(mSocket, (char *)buf_ptr, buffer_length, host);

	if (!success)
	{
		mSendPacketFailureCount++;
	}
	else
	{
		// mCircuitInfo already points to the correct circuit data
		cdp->addBytesOut( buffer_length );
	}

	if(mVerboseLog)
	{
		std::ostringstream str;
		str << "MSG: -> " << host;
		char buffer[MAX_STRING];			/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mSendSize, buffer_length, cdp->getPacketOutID());		/* Flawfinder: ignore */
		str << buffer
			<< mMessageBuilder->getMessageName()
			<< (mSendReliable ? " reliable " : "");
		if(is_ack_appended)
		{
			str << "\tACKS:\t";
			std::ostream_iterator<TPACKETID> append(str, " ");
			std::copy(acks.begin(), acks.end(), append);
		}
		llinfos << str.str() << llendl;
	}

	/*lldebugst(LLERR_MESSAGE) << "MessageSent at: " << (S32)totalTime() 
							 << "," << mMessageBuilder->getMessageName()
							 << " to " << host 
							 << llendl;*/

	mPacketsOut++;
	mBytesOut += buffer_length;
	
	mSendReliable = FALSE;
	mReliablePacketParams.clear();
	return buffer_length;
}

void LLMessageSystem::logMsgFromInvalidCircuit( const LLHost& host, BOOL recv_reliable )
{
	if(mVerboseLog)
	{
		std::ostringstream str;
		str << "MSG: <- " << host;
		char buffer[MAX_STRING];			/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mMessageReader->getMessageSize(), (mIncomingCompressedSize ? mIncomingCompressedSize: mMessageReader->getMessageSize()), mCurrentRecvPacketID);		/* Flawfinder: ignore */
		str << buffer
			<< nullToEmpty(mMessageReader->getMessageName())
			<< (recv_reliable ? " reliable" : "")
 			<< " REJECTED";
		llinfos << str.str() << llendl;
	}
	// nope!
	// cout << "Rejecting unexpected message " << mCurrentMessageTemplate->mName << " from " << hex << ip << " , " << dec << port << endl;

	// Keep track of rejected messages as well
	if (mNumMessageCounts >= MAX_MESSAGE_COUNT_NUM)
	{
		llwarns << "Got more than " << MAX_MESSAGE_COUNT_NUM << " packets without clearing counts" << llendl;
	}
	else
	{
		// TODO: babbage: work out if we need these
		// mMessageCountList[mNumMessageCounts].mMessageNum = mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes = mMessageReader->getMessageSize();
		mMessageCountList[mNumMessageCounts].mInvalid = TRUE;
		mNumMessageCounts++;
	}
}

S32 LLMessageSystem::sendMessage(const LLHost &host, const char* name,
								  const LLSD& message)
{
	if (!(host.isOk()))
	{
		llwarns << "trying to send message to invalid host"	<< llendl;
		return 0;
	}
	newMessage(name);	
	if (mMessageBuilder != mLLSDMessageBuilder)
	{
		llwarns << "trying to send llsd message when builder is not LLSD!"
				<< llendl;
		return 0;
	}

	const LLHTTPSender& sender = LLHTTPSender::getSender(host);
	sender.send(host, name, message, createResponder(name));
	return 1;
}

void LLMessageSystem::logTrustedMsgFromUntrustedCircuit( const LLHost& host )
{
	// RequestTrustedCircuit is how we establish trust, so don't spam
	// if it's received on a trusted circuit. JC
	if (strcmp(mMessageReader->getMessageName(), "RequestTrustedCircuit"))
	{
		llwarns << "Received trusted message on untrusted circuit. "
				<< "Will reply with deny. "
				<< "Message: " << nullToEmpty(mMessageReader->getMessageName())
				<< " Host: " << host << llendl;
	}

	if (mNumMessageCounts >= MAX_MESSAGE_COUNT_NUM)
	{
		llwarns << "got more than " << MAX_MESSAGE_COUNT_NUM
			<< " packets without clearing counts"
			<< llendl;
	}
	else
	{
		// TODO: babbage: work out if we need these
		//mMessageCountList[mNumMessageCounts].mMessageNum
		//	= mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes
			= mMessageReader->getMessageSize();
		mMessageCountList[mNumMessageCounts].mInvalid = TRUE;
		mNumMessageCounts++;
	}
}

void LLMessageSystem::logValidMsg(LLCircuitData *cdp, const LLHost& host, BOOL recv_reliable, BOOL recv_resent, BOOL recv_acks )
{
	if (mNumMessageCounts >= MAX_MESSAGE_COUNT_NUM)
	{
		llwarns << "Got more than " << MAX_MESSAGE_COUNT_NUM << " packets without clearing counts" << llendl;
	}
	else
	{
		// TODO: babbage: work out if we need these
		//mMessageCountList[mNumMessageCounts].mMessageNum = mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes = mMessageReader->getMessageSize();
		mMessageCountList[mNumMessageCounts].mInvalid = FALSE;
		mNumMessageCounts++;
	}

	if (cdp)
	{
		// update circuit packet ID tracking (missing/out of order packets)
		cdp->checkPacketInID( mCurrentRecvPacketID, recv_resent );
		cdp->addBytesIn( mTrueReceiveSize );
	}

	if(mVerboseLog)
	{
		std::ostringstream str;
		str << "MSG: <- " << host;
		char buffer[MAX_STRING];			/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mMessageReader->getMessageSize(), (mIncomingCompressedSize ? mIncomingCompressedSize : mMessageReader->getMessageSize()), mCurrentRecvPacketID);		/* Flawfinder: ignore */
		str << buffer
			<< nullToEmpty(mMessageReader->getMessageName())
			<< (recv_reliable ? " reliable" : "")
			<< (recv_resent ? " resent" : "")
			<< (recv_acks ? " acks" : "");
		llinfos << str.str() << llendl;
	}
}

void LLMessageSystem::sanityCheck()
{
// TODO: babbage: reinstate

//	if (!mCurrentRMessageData)
//	{
//		llerrs << "mCurrentRMessageData is NULL" << llendl;
//	}

//	if (!mCurrentRMessageTemplate)
//	{
//		llerrs << "mCurrentRMessageTemplate is NULL" << llendl;
//	}

//	if (!mCurrentRTemplateBlock)
//	{
//		llerrs << "mCurrentRTemplateBlock is NULL" << llendl;
//	}

//	if (!mCurrentRDataBlock)
//	{
//		llerrs << "mCurrentRDataBlock is NULL" << llendl;
//	}

//	if (!mCurrentSMessageData)
//	{
//		llerrs << "mCurrentSMessageData is NULL" << llendl;
//	}

//	if (!mCurrentSMessageTemplate)
//	{
//		llerrs << "mCurrentSMessageTemplate is NULL" << llendl;
//	}

//	if (!mCurrentSTemplateBlock)
//	{
//		llerrs << "mCurrentSTemplateBlock is NULL" << llendl;
//	}

//	if (!mCurrentSDataBlock)
//	{
//		llerrs << "mCurrentSDataBlock is NULL" << llendl;
//	}
}

void LLMessageSystem::showCircuitInfo()
{
	llinfos << mCircuitInfo << llendl;
}


void LLMessageSystem::dumpCircuitInfo()
{
	lldebugst(LLERR_CIRCUIT_INFO) << mCircuitInfo << llendl;
}

/* virtual */
U32 LLMessageSystem::getOurCircuitCode()
{
	return mOurCircuitCode;
}

LLString LLMessageSystem::getCircuitInfoString()
{
	LLString info_string;

	info_string += mCircuitInfo.getInfoString();
	return info_string;
}

// returns whether the given host is on a trusted circuit
BOOL    LLMessageSystem::getCircuitTrust(const LLHost &host)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		return cdp->getTrusted();
	}

	return FALSE;
}

// Activate a circuit, and set its trust level (TRUE if trusted,
// FALSE if not).
void LLMessageSystem::enableCircuit(const LLHost &host, BOOL trusted)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (!cdp)
	{
		cdp = mCircuitInfo.addCircuitData(host, 0);
	}
	else
	{
		cdp->setAlive(TRUE);
	}
	cdp->setTrusted(trusted);
}

void LLMessageSystem::disableCircuit(const LLHost &host)
{
	llinfos << "LLMessageSystem::disableCircuit for " << host << llendl;
	U32 code = gMessageSystem->findCircuitCode( host );

	// Don't need to do this, as we're removing the circuit info anyway - djs 01/28/03

	// don't clean up 0 circuit code entries
	// because many hosts (neighbor sims, etc) can have the 0 circuit
	if (code)
	{
		//if (mCircuitCodes.checkKey(code))
		code_session_map_t::iterator it = mCircuitCodes.find(code);
		if(it != mCircuitCodes.end())
		{
			llinfos << "Circuit " << code << " removed from list" << llendl;
			//mCircuitCodes.removeData(code);
			mCircuitCodes.erase(it);
		}

		U64 ip_port = 0;
		std::map<U32, U64>::iterator iter = gMessageSystem->mCircuitCodeToIPPort.find(code);
		if (iter != gMessageSystem->mCircuitCodeToIPPort.end())
		{
			ip_port = iter->second;

			gMessageSystem->mCircuitCodeToIPPort.erase(iter);

			U32 old_port = (U32)(ip_port & (U64)0xFFFFFFFF);
			U32 old_ip = (U32)(ip_port >> 32);

			llinfos << "Host " << LLHost(old_ip, old_port) << " circuit " << code << " removed from lookup table" << llendl;
			gMessageSystem->mIPPortToCircuitCode.erase(ip_port);
		}
		mCircuitInfo.removeCircuitData(host);
	}
	else
	{
		// Sigh, since we can open circuits which don't have circuit
		// codes, it's possible for this to happen...
		
		llwarns << "Couldn't find circuit code for " << host << llendl;
	}

}


void LLMessageSystem::setCircuitAllowTimeout(const LLHost &host, BOOL allow)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		cdp->setAllowTimeout(allow);
	}
}

void LLMessageSystem::setCircuitTimeoutCallback(const LLHost &host, void (*callback_func)(const LLHost & host, void *user_data), void *user_data)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		cdp->setTimeoutCallback(callback_func, user_data);
	}
}


BOOL LLMessageSystem::checkCircuitBlocked(const U32 circuit)
{
	LLHost host = findHost(circuit);

	if (!host.isOk())
	{
		//llinfos << "checkCircuitBlocked: Unknown circuit " << circuit << llendl;
		return TRUE;
	}

	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		return cdp->isBlocked();
	}
	else
	{
		llinfos << "checkCircuitBlocked(circuit): Unknown host - " << host << llendl;
		return FALSE;
	}
}

BOOL LLMessageSystem::checkCircuitAlive(const U32 circuit)
{
	LLHost host = findHost(circuit);

	if (!host.isOk())
	{
		//llinfos << "checkCircuitAlive: Unknown circuit " << circuit << llendl;
		return FALSE;
	}

	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		return cdp->isAlive();
	}
	else
	{
		llinfos << "checkCircuitAlive(circuit): Unknown host - " << host << llendl;
		return FALSE;
	}
}

BOOL LLMessageSystem::checkCircuitAlive(const LLHost &host)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (cdp)
	{
		return cdp->isAlive();
	}
	else
	{
		//llinfos << "checkCircuitAlive(host): Unknown host - " << host << llendl;
		return FALSE;
	}
}


void LLMessageSystem::setCircuitProtection(BOOL b_protect)
{
	mbProtected = b_protect;
}


U32 LLMessageSystem::findCircuitCode(const LLHost &host)
{
	U64 ip64 = (U64) host.getAddress();
	U64 port64 = (U64) host.getPort();
	U64 ip_port = (ip64 << 32) | port64;

	return get_if_there(mIPPortToCircuitCode, ip_port, U32(0));
}

LLHost LLMessageSystem::findHost(const U32 circuit_code)
{
	if (mCircuitCodeToIPPort.count(circuit_code) > 0)
	{
		return LLHost(mCircuitCodeToIPPort[circuit_code]);
	}
	else
	{
		return LLHost::invalid;
	}
}

void LLMessageSystem::setMaxMessageTime(const F32 seconds)
{
	mMaxMessageTime = seconds;
}

void LLMessageSystem::setMaxMessageCounts(const S32 num)
{
	mMaxMessageCounts = num;
}


std::ostream& operator<<(std::ostream& s, LLMessageSystem &msg)
{
	U32 i;
	if (msg.mbError)
	{
		s << "Message system not correctly initialized";
	}
	else
	{
		s << "Message system open on port " << msg.mPort << " and socket " << msg.mSocket << "\n";
//		s << "Message template file " << msg.mName << " loaded\n";
		
		s << "\nHigh frequency messages:\n";

		for (i = 1; msg.mMessageNumbers[i] && (i < 255); i++)
		{
			s << *(msg.mMessageNumbers[i]);
		}
		
		s << "\nMedium frequency messages:\n";

		for (i = (255 << 8) + 1; msg.mMessageNumbers[i] && (i < (255 << 8) + 255); i++)
		{
			s << *msg.mMessageNumbers[i];
		}
		
		s << "\nLow frequency messages:\n";

		for (i = (0xFFFF0000) + 1; msg.mMessageNumbers[i] && (i < 0xFFFFFFFF); i++)
		{
			s << *msg.mMessageNumbers[i];
		}
	}
	return s;
}

LLMessageSystem	*gMessageSystem = NULL;

// update appropriate ping info
void	process_complete_ping_check(LLMessageSystem *msgsystem, void** /*user_data*/)
{
	U8 ping_id;
	msgsystem->getU8Fast(_PREHASH_PingID, _PREHASH_PingID, ping_id);

	LLCircuitData *cdp;
	cdp = msgsystem->mCircuitInfo.findCircuit(msgsystem->getSender());

	// stop the appropriate timer
	if (cdp)
	{
		cdp->pingTimerStop(ping_id);
	}
}

void	process_start_ping_check(LLMessageSystem *msgsystem, void** /*user_data*/)
{
	U8 ping_id;
	msgsystem->getU8Fast(_PREHASH_PingID, _PREHASH_PingID, ping_id);

	LLCircuitData *cdp;
	cdp = msgsystem->mCircuitInfo.findCircuit(msgsystem->getSender());
	if (cdp)
	{
		// Grab the packet id of the oldest unacked packet
		U32 packet_id;
		msgsystem->getU32Fast(_PREHASH_PingID, _PREHASH_OldestUnacked, packet_id);
		cdp->clearDuplicateList(packet_id);
	}

	// Send off the response
	msgsystem->newMessageFast(_PREHASH_CompletePingCheck);
	msgsystem->nextBlockFast(_PREHASH_PingID);
	msgsystem->addU8(_PREHASH_PingID, ping_id);
	msgsystem->sendMessage(msgsystem->getSender());
}



// Note: this is currently unused. --mark
void	open_circuit(LLMessageSystem *msgsystem, void** /*user_data*/)
{
	U32  ip;
	U16	 port;

	msgsystem->getIPAddrFast(_PREHASH_CircuitInfo, _PREHASH_IP, ip);
	msgsystem->getIPPortFast(_PREHASH_CircuitInfo, _PREHASH_Port, port);

	// By default, OpenCircuit's are untrusted
	msgsystem->enableCircuit(LLHost(ip, port), FALSE);
}

void	close_circuit(LLMessageSystem *msgsystem, void** /*user_data*/)
{
	msgsystem->disableCircuit(msgsystem->getSender());
}

// static
/*
void LLMessageSystem::processAssignCircuitCode(LLMessageSystem* msg, void**)
{
	// if we already have a circuit code, we can bail
	if(msg->mOurCircuitCode) return;
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_CircuitCode, _PREHASH_SessionID, session_id);
	if(session_id != msg->getMySessionID())
	{
		llwarns << "AssignCircuitCode, bad session id. Expecting "
				<< msg->getMySessionID() << " but got " << session_id
				<< llendl;
		return;
	}
	U32 code;
	msg->getU32Fast(_PREHASH_CircuitCode, _PREHASH_Code, code);
	if (!code)
	{
		llerrs << "Assigning circuit code of zero!" << llendl;
	}
	
	msg->mOurCircuitCode = code;
	llinfos << "Circuit code " << code << " assigned." << llendl;
}
*/

// static
void LLMessageSystem::processAddCircuitCode(LLMessageSystem* msg, void**)
{
	U32 code;
	msg->getU32Fast(_PREHASH_CircuitCode, _PREHASH_Code, code);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_CircuitCode, _PREHASH_SessionID, session_id);
	(void)msg->addCircuitCode(code, session_id);

	// Send the ack back
	//msg->newMessageFast(_PREHASH_AckAddCircuitCode);
	//msg->nextBlockFast(_PREHASH_CircuitCode);
	//msg->addU32Fast(_PREHASH_Code, code);
	//msg->sendMessage(msg->getSender());
}

bool LLMessageSystem::addCircuitCode(U32 code, const LLUUID& session_id)
{
	if(!code)
	{
		llwarns << "addCircuitCode: zero circuit code" << llendl;
		return false;
	}
	code_session_map_t::iterator it = mCircuitCodes.find(code);
	if(it == mCircuitCodes.end())
	{
		llinfos << "New circuit code " << code << " added" << llendl;
		//msg->mCircuitCodes[circuit_code] = circuit_code;
		
		mCircuitCodes.insert(code_session_map_t::value_type(code, session_id));
	}
	else
	{
		llinfos << "Duplicate circuit code " << code << " added" << llendl;
	}
	return true;
}

//void ack_add_circuit_code(LLMessageSystem *msgsystem, void** /*user_data*/)
//{
	// By default, we do nothing.  This particular message is only handled by the spaceserver
//}

// static
void LLMessageSystem::processUseCircuitCode(LLMessageSystem* msg,
											void** user)
{
	U32 circuit_code_in;
	msg->getU32Fast(_PREHASH_CircuitCode, _PREHASH_Code, circuit_code_in);

	U32 ip = msg->getSenderIP();
	U32 port = msg->getSenderPort();

	U64 ip64 = ip;
	U64 port64 = port;
	U64 ip_port_in = (ip64 << 32) | port64;

	if (circuit_code_in)
	{
		//if (!msg->mCircuitCodes.checkKey(circuit_code_in))
		code_session_map_t::iterator it;
		it = msg->mCircuitCodes.find(circuit_code_in);
		if(it == msg->mCircuitCodes.end())
		{
			// Whoah, abort!  We don't know anything about this circuit code.
			llwarns << "UseCircuitCode for " << circuit_code_in
					<< " received without AddCircuitCode message - aborting"
					<< llendl;
			return;
		}

		LLUUID id;
		msg->getUUIDFast(_PREHASH_CircuitCode, _PREHASH_ID, id);
		LLUUID session_id;
		msg->getUUIDFast(_PREHASH_CircuitCode, _PREHASH_SessionID, session_id);
		if(session_id != (*it).second)
		{
			llwarns << "UseCircuitCode unmatched session id. Got "
					<< session_id << " but expected " << (*it).second
					<< llendl;
			return;
		}

		// Clean up previous references to this ip/port or circuit
		U64 ip_port_old = get_if_there(msg->mCircuitCodeToIPPort, circuit_code_in, U64(0));
		U32 circuit_code_old = get_if_there(msg->mIPPortToCircuitCode, ip_port_in, U32(0));

		if (ip_port_old)
		{
			if ((ip_port_old == ip_port_in) && (circuit_code_old == circuit_code_in))
			{
				// Current information is the same as incoming info, ignore
				llinfos << "Got duplicate UseCircuitCode for circuit " << circuit_code_in << " to " << msg->getSender() << llendl;
				return;
			}

			// Hmm, got a different IP and port for the same circuit code.
			U32 circut_code_old_ip_port = get_if_there(msg->mIPPortToCircuitCode, ip_port_old, U32(0));
			msg->mCircuitCodeToIPPort.erase(circut_code_old_ip_port);
			msg->mIPPortToCircuitCode.erase(ip_port_old);
			U32 old_port = (U32)(ip_port_old & (U64)0xFFFFFFFF);
			U32 old_ip = (U32)(ip_port_old >> 32);
			llinfos << "Removing derelict lookup entry for circuit " << circuit_code_old << " to " << LLHost(old_ip, old_port) << llendl;
		}

		if (circuit_code_old)
		{
			LLHost cur_host(ip, port);

			llwarns << "Disabling existing circuit for " << cur_host << llendl;
			msg->disableCircuit(cur_host);
			if (circuit_code_old == circuit_code_in)
			{
				llwarns << "Asymmetrical circuit to ip/port lookup!" << llendl;
				llwarns << "Multiple circuit codes for " << cur_host << " probably!" << llendl;
				llwarns << "Permanently disabling circuit" << llendl;
				return;
			}
			else
			{
				llwarns << "Circuit code changed for " << msg->getSender()
						<< " from " << circuit_code_old << " to "
						<< circuit_code_in << llendl;
			}
		}

		// Since this comes from the viewer, it's untrusted, but it
		// passed the circuit code and session id check, so we will go
		// ahead and persist the ID associated.
		LLCircuitData *cdp = msg->mCircuitInfo.findCircuit(msg->getSender());
		BOOL had_circuit_already = cdp ? TRUE : FALSE;

		msg->enableCircuit(msg->getSender(), FALSE);
		cdp = msg->mCircuitInfo.findCircuit(msg->getSender());
		if(cdp)
		{
			cdp->setRemoteID(id);
			cdp->setRemoteSessionID(session_id);
		}

		if (!had_circuit_already)
		{
			//
			// HACK HACK HACK HACK HACK!
			//
			// This would NORMALLY happen inside logValidMsg, but at the point that this happens
			// inside logValidMsg, there's no circuit for this message yet.  So the awful thing that
			// we do here is do it inside this message handler immediately AFTER the message is
			// handled.
			//
			// We COULD not do this, but then what happens is that some of the circuit bookkeeping
			// gets broken, especially the packets in count.  That causes some later packets to flush
			// the RecentlyReceivedReliable list, resulting in an error in which UseCircuitCode
			// doesn't get properly duplicate suppressed.  Not a BIG deal, but it's somewhat confusing
			// (and bad from a state point of view).  DJS 9/23/04
			//
			cdp->checkPacketInID(gMessageSystem->mCurrentRecvPacketID, FALSE ); // Since this is the first message on the circuit, by definition it's not resent.
		}

		msg->mIPPortToCircuitCode[ip_port_in] = circuit_code_in;
		msg->mCircuitCodeToIPPort[circuit_code_in] = ip_port_in;

		llinfos << "Circuit code " << circuit_code_in << " from "
				<< msg->getSender() << " for agent " << id << " in session "
				<< session_id << llendl;

		const LLUseCircuitCodeResponder* responder =
			(const LLUseCircuitCodeResponder*) user;
		if(responder)
		{
			responder->complete(msg->getSender(), id);
		}
	}
	else
	{
		llwarns << "Got zero circuit code in use_circuit_code" << llendl;
	}
}

// static
void LLMessageSystem::processError(LLMessageSystem* msg, void**)
{
	char buffer[MTUBYTES];
	S32 error_code = 0;
	msg->getS32("Data", "Code", error_code);
	std::string error_token;
	msg->getString("Data", "Token", MTUBYTES, buffer);
	error_token.assign(buffer);
	LLUUID error_id;
	msg->getUUID("Data", "ID", error_id);
	std::string error_system;
	msg->getString("Data", "System", MTUBYTES, buffer);
	error_system.assign(buffer);
	std::string error_message;
	msg->getString("Data", "Message", MTUBYTES, buffer);
	error_message.assign(buffer);

	llwarns << "Message error from " << msg->getSender() << " - "
		<< error_code << " " << error_token << " " << error_id << " \""
		<< error_system << "\" \"" << error_message << "\"" << llendl;
}


static LLHTTPNode& messageRootNode()
{
	static LLHTTPNode root_node;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		LLHTTPRegistrar::buildAllServices(root_node);
	}

	return root_node;
}

//static
void LLMessageSystem::dispatch(
	const std::string& msg_name,
	const LLSD& message)
{
	LLPointer<LLSimpleResponse>	responsep =	LLSimpleResponse::create();
	dispatch(msg_name, message, responsep);
}

//static
void LLMessageSystem::dispatch(
	const std::string& msg_name,
	const LLSD& message,
	LLHTTPNode::ResponsePtr responsep)
{
	if ((gMessageSystem->mMessageTemplates.find
			(gMessageStringTable.getString(msg_name.c_str())) ==
				gMessageSystem->mMessageTemplates.end()) &&
		!LLMessageConfig::isValidMessage(msg_name))
	{
		llwarns << "Ignoring unknown message " << msg_name << llendl;
		responsep->notFound("Invalid message name");
		return;
	}
	
	std::string	path = "/message/" + msg_name;
	LLSD context;
	const LLHTTPNode* handler =	messageRootNode().traverse(path, context);
	if (!handler)
	{
		llwarns	<< "LLMessageService::dispatch > no handler for "
				<< path << llendl;
		return;
	}
	// enable this for output of message names
	//llinfos << "< \"" << msg_name << "\"" << llendl;
	//lldebugs << "data: " << LLSDXMLStreamer(message) << llendl;	   

	handler->post(responsep, context, message);
}

static void check_for_unrecognized_messages(
		const char* type,
		const LLSD& map,
		LLMessageSystem::message_template_name_map_t& templates)
{
	for (LLSD::map_const_iterator iter = map.beginMap(),
			end = map.endMap();
		 iter != end; ++iter)
	{
		const char* name = gMessageStringTable.getString(iter->first.c_str());

		if (templates.find(name) == templates.end())
		{
			llinfos << "    " << type
				<< " ban list contains unrecognized message "
				<< name << llendl;
		}
	}
}

void LLMessageSystem::setMessageBans(
		const LLSD& trusted, const LLSD& untrusted)
{
	llinfos << "LLMessageSystem::setMessageBans:" << llendl;
	bool any_set = false;

	for (message_template_name_map_t::iterator iter = mMessageTemplates.begin(),
			 end = mMessageTemplates.end();
		 iter != end; ++iter)
	{
		LLMessageTemplate* mt = iter->second;

		std::string name(mt->mName);
		bool ban_from_trusted
			= trusted.has(name) && trusted.get(name).asBoolean();
		bool ban_from_untrusted
			= untrusted.has(name) && untrusted.get(name).asBoolean();

		mt->mBanFromTrusted = ban_from_trusted;
		mt->mBanFromUntrusted = ban_from_untrusted;

		if (ban_from_trusted  ||  ban_from_untrusted)
		{
			llinfos << "    " << name << " banned from "
				<< (ban_from_trusted ? "TRUSTED " : " ")
				<< (ban_from_untrusted ? "UNTRUSTED " : " ")
				<< llendl;
			any_set = true;
		}
	}

	if (!any_set) 
	{
		llinfos << "    no messages banned" << llendl;
	}

	check_for_unrecognized_messages("trusted", trusted, mMessageTemplates);
	check_for_unrecognized_messages("untrusted", untrusted, mMessageTemplates);
}

S32 LLMessageSystem::sendError(
	const LLHost& host,
	const LLUUID& agent_id,
	S32 code,
	const std::string& token,
	const LLUUID& id,
	const std::string& system,
	const std::string& message,
	const LLSD& data)
{
	newMessage("Error");
	nextBlockFast(_PREHASH_AgentData);
	addUUIDFast(_PREHASH_AgentID, agent_id);
	nextBlockFast(_PREHASH_Data);
	addS32("Code", code);
	addString("Token", token);
	addUUID("ID", id);
	addString("System", system);
	std::string temp;
	temp = message;
	if(temp.size() > (size_t)MTUBYTES) temp.resize((size_t)MTUBYTES);
	addString("Message", message);
	LLPointer<LLSDBinaryFormatter> formatter = new LLSDBinaryFormatter;
	std::ostringstream ostr;
	formatter->format(data, ostr);
	temp = ostr.str();
	bool pack_data = true;
	static const std::string ERROR_MESSAGE_NAME("Error");
	if (LLMessageConfig::getMessageFlavor(ERROR_MESSAGE_NAME) ==
		LLMessageConfig::TEMPLATE_FLAVOR)
	{
		S32 msg_size = temp.size() + mMessageBuilder->getMessageSize();
		if(msg_size >= ETHERNET_MTU_BYTES)
		{
			pack_data = false;
		}
	}
	if(pack_data)
	{
		addBinaryData("Data", (void*)temp.c_str(), temp.size());
	}
	else
	{
		llwarns << "Data and message were too large -- data removed."
			<< llendl;
		addBinaryData("Data", NULL, 0);
	}
	return sendReliable(host);
}

void	process_packet_ack(LLMessageSystem *msgsystem, void** /*user_data*/)
{
	TPACKETID packet_id;

	LLHost host = msgsystem->getSender();
	LLCircuitData *cdp = msgsystem->mCircuitInfo.findCircuit(host);
	if (cdp)
	{
	
		S32 ack_count = msgsystem->getNumberOfBlocksFast(_PREHASH_Packets);

		for (S32 i = 0; i < ack_count; i++)
		{
			msgsystem->getU32Fast(_PREHASH_Packets, _PREHASH_ID, packet_id, i);
//			llinfos << "ack recvd' from " << host << " for packet " << (TPACKETID)packet_id << llendl;
			cdp->ackReliablePacket(packet_id);
		}
		if (!cdp->getUnackedPacketCount())
		{
			// Remove this circuit from the list of circuits with unacked packets
			gMessageSystem->mCircuitInfo.mUnackedCircuitMap.erase(host);
		}
	}
}


/*
void process_log_messages(LLMessageSystem* msg, void**)
{
	U8 log_message;

	msg->getU8Fast(_PREHASH_Options, _PREHASH_Enable, log_message);
	
	if (log_message)
	{
		llinfos << "Starting logging via message" << llendl;
		msg->startLogging();
	}
	else
	{
		llinfos << "Stopping logging via message" << llendl;
		msg->stopLogging();
	}
}*/

// Make circuit trusted if the MD5 Digest matches, otherwise
// notify remote end that they are not trusted.
void process_create_trusted_circuit(LLMessageSystem *msg, void **)
{
	// don't try to create trust on machines with no shared secret
	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty()) return;

	LLUUID remote_id;
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_EndPointID, remote_id);

	LLCircuitData *cdp = msg->mCircuitInfo.findCircuit(msg->getSender());
	if (!cdp)
	{
		llwarns << "Attempt to create trusted circuit without circuit data: "
				<< msg->getSender() << llendl;
		return;
	}

	LLUUID local_id;
	local_id = cdp->getLocalEndPointID();
	if (remote_id == local_id)
	{
		//	Don't respond to requests that use the same end point ID
		return;
	}

	char their_digest[MD5HEX_STR_SIZE];	/* Flawfinder: ignore */
	S32 size = msg->getSizeFast(_PREHASH_DataBlock, _PREHASH_Digest);
	if(size != MD5HEX_STR_BYTES)
	{
		// ignore requests which pack the wrong amount of data.
		return;
	}
	msg->getBinaryDataFast(_PREHASH_DataBlock, _PREHASH_Digest, their_digest, MD5HEX_STR_BYTES);
	their_digest[MD5HEX_STR_SIZE - 1] = '\0';
	if(msg->isMatchingDigestForWindowAndUUIDs(their_digest, TRUST_TIME_WINDOW, local_id, remote_id))
	{
		cdp->setTrusted(TRUE);
		llinfos << "Trusted digest from " << msg->getSender() << llendl;
		return;
	}
	else if (cdp->getTrusted())
	{
		// The digest is bad, but this circuit is already trusted.
		// This means that this could just be the result of a stale deny sent from a while back, and
		// the message system is being slow.  Don't bother sending the deny, as it may continually
		// ping-pong back and forth on a very hosed circuit.
		llwarns << "Ignoring bad digest from known trusted circuit: " << their_digest
			<< " host: " << msg->getSender() << llendl;
		return;
	}
	else
	{
		llwarns << "Bad digest from known circuit: " << their_digest
				<< " host: " << msg->getSender() << llendl;
		msg->sendDenyTrustedCircuit(msg->getSender());
		return;
	}
}		   

void process_deny_trusted_circuit(LLMessageSystem *msg, void **)
{
	// don't try to create trust on machines with no shared secret
	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty()) return;

	LLUUID remote_id;
	msg->getUUIDFast(_PREHASH_DataBlock, _PREHASH_EndPointID, remote_id);

	LLCircuitData *cdp = msg->mCircuitInfo.findCircuit(msg->getSender());
	if (!cdp)
	{
		return;
	}

	LLUUID local_id;
	local_id = cdp->getLocalEndPointID();
	if (remote_id == local_id)
	{
		//	Don't respond to requests that use the same end point ID
		return;
	}

	// Assume that we require trust to proceed, so resend.
	// This catches the case where a circuit that was trusted
	// times out, and allows us to re-establish it, but does
	// mean that if our shared_secret or clock is wrong, we'll
	// spin.
	// *TODO: probably should keep a count of number of resends
	// per circuit, and stop resending after a while.
	llinfos << "Got DenyTrustedCircuit. Sending CreateTrustedCircuit to "
			<< msg->getSender() << llendl;
	msg->sendCreateTrustedCircuit(msg->getSender(), local_id, remote_id);
}


void dump_prehash_files()
{
	U32 i;
	FILE* fp = LLFile::fopen("../../indra/llmessage/message_prehash.h", "w");	/* Flawfinder: ignore */
	if (fp)
	{
		fprintf(
			fp,
			"/**\n"
			" * @file message_prehash.h\n"
			" * @brief header file of externs of prehashed variables plus defines.\n"
			" *\n"
			" * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.\n"
			" * $License$\n"
			" */\n\n"
			"#ifndef LL_MESSAGE_PREHASH_H\n#define LL_MESSAGE_PREHASH_H\n\n");
		fprintf(
			fp,
			"/**\n"
			" * Generated from message template version number %.3f\n"
			" */\n",
			gMessageSystem->mMessageFileVersionNumber);
		fprintf(fp, "\n\nextern F32 gPrehashVersionNumber;\n\n");
		for (i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
		{
			if (!gMessageStringTable.mEmpty[i] && gMessageStringTable.mString[i][0] != '.')
			{
				fprintf(fp, "extern char * _PREHASH_%s;\n", gMessageStringTable.mString[i]);
			}
		}
		fprintf(fp, "\n\nvoid init_prehash_data();\n\n");
		fprintf(fp, "\n\n");
		fprintf(fp, "\n\n#endif\n");
		fclose(fp);
	}
	fp = LLFile::fopen("../../indra/llmessage/message_prehash.cpp", "w");	/* Flawfinder: ignore */
	if (fp)
	{
		fprintf(
			fp,
			"/**\n"
			" * @file message_prehash.cpp\n"
			" * @brief file of prehashed variables\n"
			" *\n"
			" * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.\n"
			" * $License$\n"
			" */\n\n"
			"/**\n"
			" * Generated from message template version number %.3f\n"
			" */\n",
			gMessageSystem->mMessageFileVersionNumber);
		fprintf(fp, "#include \"linden_common.h\"\n");
		fprintf(fp, "#include \"message.h\"\n\n");
		fprintf(fp, "\n\nF32 gPrehashVersionNumber = %.3ff;\n\n", gMessageSystem->mMessageFileVersionNumber);
		for (i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
		{
			if (!gMessageStringTable.mEmpty[i] && gMessageStringTable.mString[i][0] != '.')
			{
				fprintf(fp, "char * _PREHASH_%s;\n", gMessageStringTable.mString[i]);
			}
		}
		fprintf(fp, "\nvoid init_prehash_data()\n");
		fprintf(fp, "{\n");
		for (i = 0; i < MESSAGE_NUMBER_OF_HASH_BUCKETS; i++)
		{
			if (!gMessageStringTable.mEmpty[i] && gMessageStringTable.mString[i][0] != '.')
			{
				fprintf(fp, "\t_PREHASH_%s = gMessageStringTable.getString(\"%s\");\n", gMessageStringTable.mString[i], gMessageStringTable.mString[i]);
			}
		}
		fprintf(fp, "}\n");
		fclose(fp);
	}
}

BOOL start_messaging_system(
	const std::string& template_name,
	U32 port,
	S32 version_major,
	S32 version_minor,
	S32 version_patch,
	BOOL b_dump_prehash_file,
	const std::string& secret,
	const LLUseCircuitCodeResponder* responder)
{
	gMessageSystem = new LLMessageSystem(
		template_name.c_str(),
		port, 
		version_major, 
		version_minor, 
		version_patch);
	g_shared_secret.assign(secret);

	if (!gMessageSystem)
	{
		llerrs << "Messaging system initialization failed." << llendl;
		return FALSE;
	}

	// bail if system encountered an error.
	if(!gMessageSystem->isOK())
	{
		return FALSE;
	}

	if (b_dump_prehash_file)
	{
		dump_prehash_files();
		exit(0);
	}
	else
	{
		init_prehash_data();
		if (gMessageSystem->mMessageFileVersionNumber != gPrehashVersionNumber)
		{
			llinfos << "Message template version does not match prehash version number" << llendl;
			llinfos << "Run simulator with -prehash command line option to rebuild prehash data" << llendl;
		}
		else
		{
			llinfos << "Message template version matches prehash version number" << llendl;
		}
	}

	gMessageSystem->setHandlerFuncFast(_PREHASH_StartPingCheck,			process_start_ping_check,		NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_CompletePingCheck,		process_complete_ping_check,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_OpenCircuit,			open_circuit,			NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_CloseCircuit,			close_circuit,			NULL);

	//gMessageSystem->setHandlerFuncFast(_PREHASH_AssignCircuitCode, LLMessageSystem::processAssignCircuitCode);	   
	gMessageSystem->setHandlerFuncFast(_PREHASH_AddCircuitCode, LLMessageSystem::processAddCircuitCode);
	//gMessageSystem->setHandlerFuncFast(_PREHASH_AckAddCircuitCode,		ack_add_circuit_code,		NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_UseCircuitCode, LLMessageSystem::processUseCircuitCode, (void**)responder);
	gMessageSystem->setHandlerFuncFast(_PREHASH_PacketAck,             process_packet_ack,	    NULL);
	//gMessageSystem->setHandlerFuncFast(_PREHASH_LogMessages,			process_log_messages,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_CreateTrustedCircuit,
				       process_create_trusted_circuit,
				       NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_DenyTrustedCircuit,
				       process_deny_trusted_circuit,
				       NULL);
	gMessageSystem->setHandlerFunc("Error", LLMessageSystem::processError);

	// We can hand this to the null_message_callback since it is a
	// trusted message, so it will automatically be denied if it isn't
	// trusted and ignored if it is -- exactly what we want.
	gMessageSystem->setHandlerFunc(
		"RequestTrustedCircuit",
		null_message_callback,
		NULL);

	// Initialize the transfer manager
	gTransferManager.init();

	return TRUE;
}

void LLMessageSystem::startLogging()
{
	mVerboseLog = TRUE;
	std::ostringstream str;
	str << "START MESSAGE LOG" << std::endl;
	str << "Legend:" << std::endl;
	str << "\t<-\tincoming message" <<std::endl;
	str << "\t->\toutgoing message" << std::endl;
	str << "     <>        host           size    zero      id name";
	llinfos << str.str() << llendl;
}

void LLMessageSystem::stopLogging()
{
	if(mVerboseLog)
	{
		mVerboseLog = FALSE;
		llinfos << "END MESSAGE LOG" << llendl;
	}
}

void LLMessageSystem::summarizeLogs(std::ostream& str)
{
	char buffer[MAX_STRING];  	/* Flawfinder: ignore */ 
	char tmp_str[MAX_STRING];	 /* Flawfinder: ignore */ 
	F32 run_time = mMessageSystemTimer.getElapsedTimeF32();
	str << "START MESSAGE LOG SUMMARY" << std::endl;
	snprintf(buffer, MAX_STRING, "Run time: %12.3f seconds", run_time);	/* Flawfinder: ignore */

	// Incoming
	str << buffer << std::endl << "Incoming:" << std::endl;
	U64_to_str(mTotalBytesIn, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total bytes received:      %20s (%5.2f kbits per second)", tmp_str, ((F32)mTotalBytesIn * 0.008f) / run_time);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mPacketsIn, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total packets received:    %20s (%5.2f packets per second)", tmp_str, ((F32) mPacketsIn / run_time));	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Average packet size:       %20.0f bytes", (F32)mTotalBytesIn / (F32)mPacketsIn);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mReliablePacketsIn, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total reliable packets:    %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mReliablePacketsIn)/((F32) mPacketsIn + 1));		/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mCompressedPacketsIn, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total compressed packets:  %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mCompressedPacketsIn)/((F32) mPacketsIn + 1));	/* Flawfinder: ignore */
	str << buffer << std::endl;
	S64 savings = mUncompressedBytesIn - mCompressedBytesIn;
	U64_to_str(savings, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total compression savings: %20s bytes", tmp_str);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(savings/(mCompressedPacketsIn +1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg comp packet savings:   %20s (%5.2f : 1)", tmp_str, ((F32) mUncompressedBytesIn)/((F32) mCompressedBytesIn+1));	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(savings/(mPacketsIn+1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg overall comp savings:  %20s (%5.2f : 1)", tmp_str, ((F32) mTotalBytesIn + (F32) savings)/((F32) mTotalBytesIn + 1.f));	/* Flawfinder: ignore */

	// Outgoing
	str << buffer << std::endl << std::endl << "Outgoing:" << std::endl;
	U64_to_str(mTotalBytesOut, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total bytes sent:          %20s (%5.2f kbits per second)", tmp_str, ((F32)mTotalBytesOut * 0.008f) / run_time );	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mPacketsOut, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total packets sent:        %20s (%5.2f packets per second)", tmp_str, ((F32)mPacketsOut / run_time));	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Average packet size:       %20.0f bytes", (F32)mTotalBytesOut / (F32)mPacketsOut);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mReliablePacketsOut, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total reliable packets:    %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mReliablePacketsOut)/((F32) mPacketsOut + 1));		/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(mCompressedPacketsOut, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total compressed packets:  %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mCompressedPacketsOut)/((F32) mPacketsOut + 1));		/* Flawfinder: ignore */
	str << buffer << std::endl;
	savings = mUncompressedBytesOut - mCompressedBytesOut;
	U64_to_str(savings, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total compression savings: %20s bytes", tmp_str);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(savings/(mCompressedPacketsOut +1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg comp packet savings:   %20s (%5.2f : 1)", tmp_str, ((F32) mUncompressedBytesOut)/((F32) mCompressedBytesOut+1));		/* Flawfinder: ignore */
	str << buffer << std::endl;
	U64_to_str(savings/(mPacketsOut+1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg overall comp savings:  %20s (%5.2f : 1)", tmp_str, ((F32) mTotalBytesOut + (F32) savings)/((F32) mTotalBytesOut + 1.f));		/* Flawfinder: ignore */
	str << buffer << std::endl << std::endl;
	snprintf(buffer, MAX_STRING, "SendPacket failures:       %20d", mSendPacketFailureCount);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Dropped packets:           %20d", mDroppedPackets);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Resent packets:            %20d", mResentPackets);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Failed reliable resends:   %20d", mFailedResendPackets);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "Off-circuit rejected packets: %17d", mOffCircuitPackets);	/* Flawfinder: ignore */
	str << buffer << std::endl;
	snprintf(buffer, MAX_STRING, "On-circuit invalid packets:   %17d", mInvalidOnCircuitPackets);		/* Flawfinder: ignore */
	str << buffer << std::endl << std::endl;

	str << "Decoding: " << std::endl;
	snprintf(buffer, MAX_STRING, "%35s%10s%10s%10s%10s", "Message", "Count", "Time", "Max", "Avg");	/* Flawfinder: ignore */
	str << buffer << std:: endl;	
	F32 avg;
	for (message_template_name_map_t::const_iterator iter = mMessageTemplates.begin(),
			 end = mMessageTemplates.end();
		 iter != end; iter++)
	{
		const LLMessageTemplate* mt = iter->second;
		if(mt->mTotalDecoded > 0)
		{
			avg = mt->mTotalDecodeTime / (F32)mt->mTotalDecoded;
			snprintf(buffer, MAX_STRING, "%35s%10u%10f%10f%10f", mt->mName, mt->mTotalDecoded, mt->mTotalDecodeTime, mt->mMaxDecodeTimePerMsg, avg);	/* Flawfinder: ignore */
			str << buffer << std::endl;
		}
	}
	str << "END MESSAGE LOG SUMMARY" << std::endl;
}

void end_messaging_system()
{
	gTransferManager.cleanup();
	LLTransferTargetVFile::updateQueue(true); // shutdown LLTransferTargetVFile
	if (gMessageSystem)
	{
		gMessageSystem->stopLogging();

		std::ostringstream str;
		gMessageSystem->summarizeLogs(str);
		llinfos << str.str().c_str() << llendl;

		delete gMessageSystem;
		gMessageSystem = NULL;
	}
}

void LLMessageSystem::resetReceiveCounts()
{
	mNumMessageCounts = 0;

	for (message_template_name_map_t::iterator iter = mMessageTemplates.begin(),
			 end = mMessageTemplates.end();
		 iter != end; iter++)
	{
		LLMessageTemplate* mt = iter->second;
		mt->mDecodeTimeThisFrame = 0.f;
	}
}


void LLMessageSystem::dumpReceiveCounts()
{
	LLMessageTemplate		*mt;

	for (message_template_name_map_t::iterator iter = mMessageTemplates.begin(),
			 end = mMessageTemplates.end();
		 iter != end; iter++)
	{
		LLMessageTemplate* mt = iter->second;
		mt->mReceiveCount = 0;
		mt->mReceiveBytes = 0;
		mt->mReceiveInvalid = 0;
	}

	S32 i;
	for (i = 0; i < mNumMessageCounts; i++)
	{
		mt = get_ptr_in_map(mMessageNumbers,mMessageCountList[i].mMessageNum);
		if (mt)
		{
			mt->mReceiveCount++;
			mt->mReceiveBytes += mMessageCountList[i].mMessageBytes;
			if (mMessageCountList[i].mInvalid)
			{
				mt->mReceiveInvalid++;
			}
		}
	}

	if(mNumMessageCounts > 0)
	{
		llinfos << "Dump: " << mNumMessageCounts << " messages processed in " << mReceiveTime << " seconds" << llendl;
		for (message_template_name_map_t::const_iterator iter = mMessageTemplates.begin(),
				 end = mMessageTemplates.end();
			 iter != end; iter++)
		{
			const LLMessageTemplate* mt = iter->second;
			if (mt->mReceiveCount > 0)
			{
				llinfos << "Num: " << std::setw(3) << mt->mReceiveCount << " Bytes: " << std::setw(6) << mt->mReceiveBytes
						<< " Invalid: " << std::setw(3) << mt->mReceiveInvalid << " " << mt->mName << " " << llround(100 * mt->mDecodeTimeThisFrame / mReceiveTime) << "%" << llendl;
			}
		}
	}
}



BOOL LLMessageSystem::isClear() const
{
	return mMessageBuilder->isClear();
}


S32 LLMessageSystem::flush(const LLHost &host)
{
	if (mMessageBuilder->getMessageSize())
	{
		S32 sentbytes = sendMessage(host);
		clearMessage();
		return sentbytes;
	}
	else
	{
		return 0;
	}
}

U32 LLMessageSystem::getListenPort( void ) const
{
	return mPort;
}

// TODO: babbage: remove this horror!
S32 LLMessageSystem::zeroCodeAdjustCurrentSendTotal()
{
	if(mMessageBuilder == mLLSDMessageBuilder)
	{
		// babbage: don't compress LLSD messages, so delta is 0
		return 0;
	}
	
	if (! mMessageBuilder->isBuilt())
	{
		mSendSize = mMessageBuilder->buildMessage(
			mSendBuffer,
			MAX_BUFFER_SIZE,
			0);
	}
	// TODO: babbage: remove this horror
	mMessageBuilder->setBuilt(FALSE);

	S32 count = mSendSize;
	
	S32 net_gain = 0;
	U8 num_zeroes = 0;
	
	U8 *inptr = (U8 *)mSendBuffer;

// skip the packet id field

	for (U32 ii = 0; ii < LL_PACKET_ID_SIZE; ++ii)
	{
		count--;
		inptr++;
	}

// don't actually build, just test

// sequential zero bytes are encoded as 0 [U8 count] 
// with 0 0 [count] representing wrap (>256 zeroes)

	while (count--)
	{
		if (!(*inptr))   // in a zero count
		{
			if (num_zeroes)
			{
				if (++num_zeroes > 254)
				{
					num_zeroes = 0;
				}
				net_gain--;   // subseqent zeroes save one
			}
			else
			{
				net_gain++;  // starting a zero count adds one
				num_zeroes = 1;
			}
			inptr++;
		}
		else
		{
			if (num_zeroes)
			{
				num_zeroes = 0;
			}
			inptr++;
		}
	}
	if (net_gain < 0)
	{
		return net_gain;
	}
	else
	{
		return 0;
	}
}



S32 LLMessageSystem::zeroCodeExpand(U8** data, S32* data_size)
{
	if ((*data_size ) < LL_MINIMUM_VALID_PACKET_SIZE)
	{
		llwarns << "zeroCodeExpand() called with data_size of " << *data_size
			<< llendl;
	}

	mTotalBytesIn += *data_size;

	// if we're not zero-coded, simply return.
	if (!(*data[0] & LL_ZERO_CODE_FLAG))
	{
		return 0;
	}

	S32 in_size = *data_size;
	mCompressedPacketsIn++;
	mCompressedBytesIn += *data_size;
	
	*data[0] &= (~LL_ZERO_CODE_FLAG);

	S32 count = (*data_size);  
	
	U8 *inptr = (U8 *)*data;
	U8 *outptr = (U8 *)mEncodedRecvBuffer;

// skip the packet id field

	for (U32 ii = 0; ii < LL_PACKET_ID_SIZE; ++ii)
	{
		count--;
		*outptr++ = *inptr++;
	}

// reconstruct encoded packet, keeping track of net size gain

// sequential zero bytes are encoded as 0 [U8 count] 
// with 0 0 [count] representing wrap (>256 zeroes)

	while (count--)
	{
		if (outptr > (&mEncodedRecvBuffer[MAX_BUFFER_SIZE-1]))
		{
			llwarns << "attempt to write past reasonable encoded buffer size 1" << llendl;
			callExceptionFunc(MX_WROTE_PAST_BUFFER_SIZE);
			outptr = mEncodedRecvBuffer;					
			break;
		}
		if (!((*outptr++ = *inptr++)))
		{
			while (((count--)) && (!(*inptr)))
			{
				*outptr++ = *inptr++;
  				if (outptr > (&mEncodedRecvBuffer[MAX_BUFFER_SIZE-256]))
  				{
  					llwarns << "attempt to write past reasonable encoded buffer size 2" << llendl;
					callExceptionFunc(MX_WROTE_PAST_BUFFER_SIZE);
					outptr = mEncodedRecvBuffer;
					count = -1;
					break;
  				}
				memset(outptr,0,255);
				outptr += 255;
			}
			
			if (count < 0)
			{
				break;
			}

			else
			{
  				if (outptr > (&mEncodedRecvBuffer[MAX_BUFFER_SIZE-(*inptr)]))
				{
  					llwarns << "attempt to write past reasonable encoded buffer size 3" << llendl;
					callExceptionFunc(MX_WROTE_PAST_BUFFER_SIZE);
					outptr = mEncodedRecvBuffer;					
				}
				memset(outptr,0,(*inptr) - 1);
				outptr += ((*inptr) - 1);
				inptr++;
			}
		}		
	}
	
	*data = mEncodedRecvBuffer;
	*data_size = (S32)(outptr - mEncodedRecvBuffer);
	mUncompressedBytesIn += *data_size;

	return(in_size);
}


void LLMessageSystem::addTemplate(LLMessageTemplate *templatep)
{
	if (mMessageTemplates.count(templatep->mName) > 0)
	{	
		llerrs << templatep->mName << " already  used as a template name!"
			<< llendl;
	}
	mMessageTemplates[templatep->mName] = templatep;
	mMessageNumbers[templatep->mMessageNumber] = templatep;
}


void LLMessageSystem::setHandlerFuncFast(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data)
{
	LLMessageTemplate* msgtemplate = get_ptr_in_map(mMessageTemplates, name);
	if (msgtemplate)
	{
		msgtemplate->setHandlerFunc(handler_func, user_data);
	}
	else
	{
		llerrs << name << " is not a known message name!" << llendl;
	}
}

bool LLMessageSystem::callHandler(const char *name,
		bool trustedSource, LLMessageSystem* msg)
{
	name = gMessageStringTable.getString(name);
	message_template_name_map_t::const_iterator iter;
	iter = mMessageTemplates.find(name);
	if(iter == mMessageTemplates.end())
	{
		llwarns << "LLMessageSystem::callHandler: unknown message " 
			<< name << llendl;
		return false;
	}

	const LLMessageTemplate* msg_template = iter->second;
	if (msg_template->isBanned(trustedSource))
	{
		llwarns << "LLMessageSystem::callHandler: banned message " 
			<< name 
			<< " from "
			<< (trustedSource ? "trusted " : "untrusted ")
			<< "source" << llendl;
		return false;
	}

	return msg_template->callHandlerFunc(msg);
}


void LLMessageSystem::setExceptionFunc(EMessageException e,
									   msg_exception_callback func,
									   void* data)
{
	callbacks_t::iterator it = mExceptionCallbacks.find(e);
	if(it != mExceptionCallbacks.end())
	{
		mExceptionCallbacks.erase(it);
	}
	if(func)
	{
		mExceptionCallbacks.insert(callbacks_t::value_type(e, exception_t(func, data)));
	}
}

BOOL LLMessageSystem::callExceptionFunc(EMessageException exception)
{
	callbacks_t::iterator it = mExceptionCallbacks.find(exception);
	if(it != mExceptionCallbacks.end())
	{
		((*it).second.first)(this, (*it).second.second,exception);
		return TRUE;
	}
	return FALSE;
}

void LLMessageSystem::setTimingFunc(msg_timing_callback func, void* data)
{
	mTimingCallback = func;
	mTimingCallbackData = data;
}

BOOL LLMessageSystem::isCircuitCodeKnown(U32 code) const
{
	if(mCircuitCodes.find(code) == mCircuitCodes.end())
		return FALSE;
	return TRUE;
}

BOOL LLMessageSystem::isMessageFast(const char *msg)
{
	return msg == mMessageReader->getMessageName();
}


char* LLMessageSystem::getMessageName()
{
	return const_cast<char*>(mMessageReader->getMessageName());
}

const LLUUID& LLMessageSystem::getSenderID() const
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(mLastSender);
	if (cdp)
	{
		return (cdp->mRemoteID);
	}

	return LLUUID::null;
}

const LLUUID& LLMessageSystem::getSenderSessionID() const
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(mLastSender);
	if (cdp)
	{
		return (cdp->mRemoteSessionID);
	}
	return LLUUID::null;
}

bool LLMessageSystem::generateDigestForNumberAndUUIDs(
	char* digest,
	const U32 number,
	const LLUUID& id1,
	const LLUUID& id2) const
{
	// *NOTE: This method is needlessly inefficient. Instead of
	// calling LLUUID::asString, it should just call
	// LLUUID::toString().

	const char *colon = ":";
	char tbuf[16];	/* Flawfinder: ignore */ 
	LLMD5 d;
	std::string id1string = id1.asString();
	std::string id2string = id2.asString();
	std::string shared_secret = get_shared_secret();
	unsigned char * secret = (unsigned char*)shared_secret.c_str();
	unsigned char * id1str = (unsigned char*)id1string.c_str();
	unsigned char * id2str = (unsigned char*)id2string.c_str();

	memset(digest, 0, MD5HEX_STR_SIZE);
	
	if( secret != NULL)
	{
		d.update(secret, (U32)strlen((char *) secret));	/* Flawfinder: ignore */
	}

	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	
	snprintf(tbuf, sizeof(tbuf),"%i", number);		/* Flawfinder: ignore */
	d.update((unsigned char *) tbuf, (U32)strlen(tbuf));	/* Flawfinder: ignore */ 
	
	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	if( (char*) id1str != NULL)
	{
		d.update(id1str, (U32)strlen((char *) id1str));	/* Flawfinder: ignore */	 
	}
	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	
	if( (char*) id2str != NULL)
	{
		d.update(id2str, (U32)strlen((char *) id2str));	/* Flawfinder: ignore */	
	}
	
	d.finalize();
	d.hex_digest(digest);
	digest[MD5HEX_STR_SIZE - 1] = '\0';

	return true;
}

bool LLMessageSystem::generateDigestForWindowAndUUIDs(char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const
{
	if(0 == window) return false;
	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty())
	{
		llerrs << "Trying to generate complex digest on a machine without a shared secret!" << llendl;
	}

	U32 now = time(NULL);

	now /= window;

	bool result = generateDigestForNumberAndUUIDs(digest, now, id1, id2);

	return result;
}

bool LLMessageSystem::isMatchingDigestForWindowAndUUIDs(const char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const
{
	if(0 == window) return false;

	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty())
	{
		llerrs << "Trying to compare complex digests on a machine without a shared secret!" << llendl;
	}
	
	char our_digest[MD5HEX_STR_SIZE];	/* Flawfinder: ignore */
	U32 now = time(NULL);

	now /= window;

	// Check 1 window ago, now, and one window from now to catch edge
	// conditions. Process them as current window, one window ago, and
	// one window in the future to catch the edges.
	const S32 WINDOW_BIN_COUNT = 3;
	U32 window_bin[WINDOW_BIN_COUNT];
	window_bin[0] = now;
	window_bin[1] = now - 1;
	window_bin[2] = now + 1;
	for(S32 i = 0; i < WINDOW_BIN_COUNT; ++i)
	{
		generateDigestForNumberAndUUIDs(our_digest, window_bin[i], id2, id1);
		if(0 == strncmp(digest, our_digest, MD5HEX_STR_BYTES))
		{
			return true;
		}
	}
	return false;
}

bool LLMessageSystem::generateDigestForNumber(char* digest, const U32 number) const
{
	memset(digest, 0, MD5HEX_STR_SIZE);

	LLMD5 d;
	std::string shared_secret = get_shared_secret();
	d = LLMD5((const unsigned char *)shared_secret.c_str(), number);
	d.hex_digest(digest);
	digest[MD5HEX_STR_SIZE - 1] = '\0';

	return true;
}

bool LLMessageSystem::generateDigestForWindow(char* digest, const S32 window) const
{
	if(0 == window) return false;

	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty())
	{
		llerrs << "Trying to generate simple digest on a machine without a shared secret!" << llendl;
	}

	U32 now = time(NULL);

	now /= window;

	bool result = generateDigestForNumber(digest, now);

	return result;
}

bool LLMessageSystem::isMatchingDigestForWindow(const char* digest, S32 const window) const
{
	if(0 == window) return false;

	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty())
	{
		llerrs << "Trying to compare simple digests on a machine without a shared secret!" << llendl;
	}
	
	char our_digest[MD5HEX_STR_SIZE];	/* Flawfinder: ignore */
	U32 now = (S32)time(NULL);

	now /= window;

	// Check 1 window ago, now, and one window from now to catch edge
	// conditions. Process them as current window, one window ago, and
	// one window in the future to catch the edges.
	const S32 WINDOW_BIN_COUNT = 3;
	U32 window_bin[WINDOW_BIN_COUNT];
	window_bin[0] = now;
	window_bin[1] = now - 1;
	window_bin[2] = now + 1;
	for(S32 i = 0; i < WINDOW_BIN_COUNT; ++i)
	{
		generateDigestForNumber(our_digest, window_bin[i]);
		if(0 == strncmp(digest, our_digest, MD5HEX_STR_BYTES))
		{
			return true;
		}
	}
	return false;
}

void LLMessageSystem::sendCreateTrustedCircuit(const LLHost &host, const LLUUID & id1, const LLUUID & id2)
{
	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty()) return;
	char digest[MD5HEX_STR_SIZE];	/* Flawfinder: ignore */
	if (id1.isNull())
	{
		llwarns << "Can't send CreateTrustedCircuit to " << host << " because we don't have the local end point ID" << llendl;
		return;
	}
	if (id2.isNull())
	{
		llwarns << "Can't send CreateTrustedCircuit to " << host << " because we don't have the remote end point ID" << llendl;
		return;
	}
	generateDigestForWindowAndUUIDs(digest, TRUST_TIME_WINDOW, id1, id2);
	newMessageFast(_PREHASH_CreateTrustedCircuit);
	nextBlockFast(_PREHASH_DataBlock);
	addUUIDFast(_PREHASH_EndPointID, id1);
	addBinaryDataFast(_PREHASH_Digest, digest, MD5HEX_STR_BYTES);
	llinfos << "xmitting digest: " << digest << " Host: " << host << llendl;
	sendMessage(host);
}

void LLMessageSystem::sendDenyTrustedCircuit(const LLHost &host)
{
	mDenyTrustedCircuitSet.insert(host);
}

void LLMessageSystem::reallySendDenyTrustedCircuit(const LLHost &host)
{
	LLCircuitData *cdp = mCircuitInfo.findCircuit(host);
	if (!cdp)
	{
		llwarns << "Not sending DenyTrustedCircuit to host without a circuit." << llendl;
		return;
	}
	llinfos << "Sending DenyTrustedCircuit to " << host << llendl;
	newMessageFast(_PREHASH_DenyTrustedCircuit);
	nextBlockFast(_PREHASH_DataBlock);
	addUUIDFast(_PREHASH_EndPointID, cdp->getLocalEndPointID());
	sendMessage(host);
}

void null_message_callback(LLMessageSystem *msg, void **data)
{
	// Nothing should ever go here, but we use this to register messages
	// that we are expecting to see (and spinning on) at startup.
	return;
}

// Try to establish a bidirectional trust metric by pinging a host until it's
// up, and then sending auth messages.
void LLMessageSystem::establishBidirectionalTrust(const LLHost &host, S64 frame_count )
{
	std::string shared_secret = get_shared_secret();
	if(shared_secret.empty())
	{
		llerrs << "Trying to establish bidirectional trust on a machine without a shared secret!" << llendl;
	}
	LLTimer timeout;

	timeout.setTimerExpirySec(20.0);
	setHandlerFuncFast(_PREHASH_StartPingCheck, null_message_callback, NULL);
	setHandlerFuncFast(_PREHASH_CompletePingCheck, null_message_callback,
		       NULL);

	while (! timeout.hasExpired())
	{
		newMessageFast(_PREHASH_StartPingCheck);
		nextBlockFast(_PREHASH_PingID);
		addU8Fast(_PREHASH_PingID, 0);
		addU32Fast(_PREHASH_OldestUnacked, 0);
		sendMessage(host);
		if (checkMessages( frame_count ))
		{
			if (isMessageFast(_PREHASH_CompletePingCheck) &&
			    (getSender() == host))
			{
				break;
			}
		}
		processAcks();
		ms_sleep(1);
	}

	// Send a request, a deny, and give the host 2 seconds to complete
	// the trust handshake.
	newMessage("RequestTrustedCircuit");
	sendMessage(host);
	reallySendDenyTrustedCircuit(host);
	setHandlerFuncFast(_PREHASH_StartPingCheck, process_start_ping_check, NULL);
	setHandlerFuncFast(_PREHASH_CompletePingCheck, process_complete_ping_check, NULL);

	timeout.setTimerExpirySec(2.0);
	LLCircuitData* cdp = NULL;
	while(!timeout.hasExpired())
	{
		cdp = mCircuitInfo.findCircuit(host);
		if(!cdp) break; // no circuit anymore, no point continuing.
		if(cdp->getTrusted()) break; // circuit is trusted.
		checkMessages(frame_count);
		processAcks();
		ms_sleep(1);
	}
}


void LLMessageSystem::dumpPacketToLog()
{
	llwarns << "Packet Dump from:" << mPacketRing.getLastSender() << llendl;
	llwarns << "Packet Size:" << mTrueReceiveSize << llendl;
	char line_buffer[256];		/* Flawfinder: ignore */
	S32 i;
	S32 cur_line_pos = 0;
	S32 cur_line = 0;

	for (i = 0; i < mTrueReceiveSize; i++)
	{
		S32 offset = cur_line_pos * 3;
		snprintf(line_buffer + offset, sizeof(line_buffer) - offset,
				 "%02x ", mTrueReceiveBuffer[i]);	/* Flawfinder: ignore */
		cur_line_pos++;
		if (cur_line_pos >= 16)
		{
			cur_line_pos = 0;
			llwarns << "PD:" << cur_line << "PD:" << line_buffer << llendl;
			cur_line++;
		}
	}
	if (cur_line_pos)
	{
		llwarns << "PD:" << cur_line << "PD:" << line_buffer << llendl;
	}
}


//static
U64 LLMessageSystem::getMessageTimeUsecs(const BOOL update)
{
	if (gMessageSystem)
	{
		if (update)
		{
			gMessageSystem->mCurrentMessageTimeSeconds = totalTime()*SEC_PER_USEC;
		}
		return (U64)(gMessageSystem->mCurrentMessageTimeSeconds * USEC_PER_SEC);
	}
	else
	{
		return totalTime();
	}
}

//static
F64 LLMessageSystem::getMessageTimeSeconds(const BOOL update)
{
	if (gMessageSystem)
	{
		if (update)
		{
			gMessageSystem->mCurrentMessageTimeSeconds = totalTime()*SEC_PER_USEC;
		}
		return gMessageSystem->mCurrentMessageTimeSeconds;
	}
	else
	{
		return totalTime()*SEC_PER_USEC;
	}
}

std::string get_shared_secret()
{
	static const std::string SHARED_SECRET_KEY("shared_secret");
	if(g_shared_secret.empty())
	{
		LLApp* app = LLApp::instance();
		if(app) return app->getOption(SHARED_SECRET_KEY);
	}
	return g_shared_secret;
}

typedef std::map<const char*, LLMessageBuilder*> BuilderMap;

void LLMessageSystem::newMessageFast(const char *name)
{
	LLMessageConfig::Flavor message_flavor =
		LLMessageConfig::getMessageFlavor(name);
	LLMessageConfig::Flavor server_flavor =
		LLMessageConfig::getServerDefaultFlavor();

	if(message_flavor == LLMessageConfig::TEMPLATE_FLAVOR)
	{
		mMessageBuilder = mTemplateMessageBuilder;
	}
	else if (message_flavor == LLMessageConfig::LLSD_FLAVOR)
	{
		mMessageBuilder = mLLSDMessageBuilder;
	}
	// NO_FLAVOR
	else
	{
		if (server_flavor == LLMessageConfig::LLSD_FLAVOR)
		{
			mMessageBuilder = mLLSDMessageBuilder;
		}
		// TEMPLATE_FLAVOR or NO_FLAVOR
		else
		{
			mMessageBuilder = mTemplateMessageBuilder;
		}
	}
	mSendReliable = FALSE;
	mMessageBuilder->newMessage(name);
}
	
void LLMessageSystem::newMessage(const char *name)
{
	newMessageFast(gMessageStringTable.getString(name));
}

void LLMessageSystem::addBinaryDataFast(const char *varname, const void *data, S32 size)
{
	mMessageBuilder->addBinaryData(varname, data, size);
}

void LLMessageSystem::addBinaryData(const char *varname, const void *data, S32 size)
{
	mMessageBuilder->addBinaryData(gMessageStringTable.getString(varname),data, size);
}

void LLMessageSystem::addS8Fast(const char *varname, S8 v)
{
	mMessageBuilder->addS8(varname, v);
}

void LLMessageSystem::addS8(const char *varname, S8 v)
{
	mMessageBuilder->addS8(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addU8Fast(const char *varname, U8 v)
{
	mMessageBuilder->addU8(varname, v);
}

void LLMessageSystem::addU8(const char *varname, U8 v)
{
	mMessageBuilder->addU8(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addS16Fast(const char *varname, S16 v)
{
	mMessageBuilder->addS16(varname, v);
}

void LLMessageSystem::addS16(const char *varname, S16 v)
{
	mMessageBuilder->addS16(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addU16Fast(const char *varname, U16 v)
{
	mMessageBuilder->addU16(varname, v);
}

void LLMessageSystem::addU16(const char *varname, U16 v)
{
	mMessageBuilder->addU16(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addF32Fast(const char *varname, F32 v)
{
	mMessageBuilder->addF32(varname, v);
}

void LLMessageSystem::addF32(const char *varname, F32 v)
{
	mMessageBuilder->addF32(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addS32Fast(const char *varname, S32 v)
{
	mMessageBuilder->addS32(varname, v);
}

void LLMessageSystem::addS32(const char *varname, S32 v)
{
	mMessageBuilder->addS32(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addU32Fast(const char *varname, U32 v)
{
	mMessageBuilder->addU32(varname, v);
}

void LLMessageSystem::addU32(const char *varname, U32 v)
{
	mMessageBuilder->addU32(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addU64Fast(const char *varname, U64 v)
{
	mMessageBuilder->addU64(varname, v);
}

void LLMessageSystem::addU64(const char *varname, U64 v)
{
	mMessageBuilder->addU64(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addF64Fast(const char *varname, F64 v)
{
	mMessageBuilder->addF64(varname, v);
}

void LLMessageSystem::addF64(const char *varname, F64 v)
{
	mMessageBuilder->addF64(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addIPAddrFast(const char *varname, U32 v)
{
	mMessageBuilder->addIPAddr(varname, v);
}

void LLMessageSystem::addIPAddr(const char *varname, U32 v)
{
	mMessageBuilder->addIPAddr(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addIPPortFast(const char *varname, U16 v)
{
	mMessageBuilder->addIPPort(varname, v);
}

void LLMessageSystem::addIPPort(const char *varname, U16 v)
{
	mMessageBuilder->addIPPort(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addBOOLFast(const char* varname, BOOL v)
{
	mMessageBuilder->addBOOL(varname, v);
}

void LLMessageSystem::addBOOL(const char* varname, BOOL v)
{
	mMessageBuilder->addBOOL(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addStringFast(const char* varname, const char* v)
{
	mMessageBuilder->addString(varname, v);
}

void LLMessageSystem::addString(const char* varname, const char* v)
{
	mMessageBuilder->addString(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addStringFast(const char* varname, const std::string& v)
{
	mMessageBuilder->addString(varname, v);
}

void LLMessageSystem::addString(const char* varname, const std::string& v)
{
	mMessageBuilder->addString(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addVector3Fast(const char *varname, const LLVector3& v)
{
	mMessageBuilder->addVector3(varname, v);
}

void LLMessageSystem::addVector3(const char *varname, const LLVector3& v)
{
	mMessageBuilder->addVector3(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addVector4Fast(const char *varname, const LLVector4& v)
{
	mMessageBuilder->addVector4(varname, v);
}

void LLMessageSystem::addVector4(const char *varname, const LLVector4& v)
{
	mMessageBuilder->addVector4(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addVector3dFast(const char *varname, const LLVector3d& v)
{
	mMessageBuilder->addVector3d(varname, v);
}

void LLMessageSystem::addVector3d(const char *varname, const LLVector3d& v)
{
	mMessageBuilder->addVector3d(gMessageStringTable.getString(varname), v);
}

void LLMessageSystem::addQuatFast(const char *varname, const LLQuaternion& v)
{
	mMessageBuilder->addQuat(varname, v);
}

void LLMessageSystem::addQuat(const char *varname, const LLQuaternion& v)
{
	mMessageBuilder->addQuat(gMessageStringTable.getString(varname), v);
}


void LLMessageSystem::addUUIDFast(const char *varname, const LLUUID& v)
{
	mMessageBuilder->addUUID(varname, v);
}

void LLMessageSystem::addUUID(const char *varname, const LLUUID& v)
{
	mMessageBuilder->addUUID(gMessageStringTable.getString(varname), v);
}

S32 LLMessageSystem::getCurrentSendTotal() const
{
	return mMessageBuilder->getMessageSize();
}

void LLMessageSystem::getS8Fast(const char *block, const char *var, S8 &u, 
								S32 blocknum)
{
	mMessageReader->getS8(block, var, u, blocknum);
}

void LLMessageSystem::getS8(const char *block, const char *var, S8 &u, 
							S32 blocknum)
{
	getS8Fast(gMessageStringTable.getString(block), 
			  gMessageStringTable.getString(var), u, blocknum);
}

void LLMessageSystem::getU8Fast(const char *block, const char *var, U8 &u, 
								S32 blocknum)
{
	mMessageReader->getU8(block, var, u, blocknum);
}

void LLMessageSystem::getU8(const char *block, const char *var, U8 &u, 
							S32 blocknum)
{
	getU8Fast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), u, blocknum);
}

void LLMessageSystem::getBOOLFast(const char *block, const char *var, BOOL &b,
								  S32 blocknum)
{
	mMessageReader->getBOOL(block, var, b, blocknum);
}

void LLMessageSystem::getBOOL(const char *block, const char *var, BOOL &b, 
							  S32 blocknum)
{
	getBOOLFast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), b, blocknum);
}

void LLMessageSystem::getS16Fast(const char *block, const char *var, S16 &d, 
								 S32 blocknum)
{
	mMessageReader->getS16(block, var, d, blocknum);
}

void LLMessageSystem::getS16(const char *block, const char *var, S16 &d, 
							 S32 blocknum)
{
	getS16Fast(gMessageStringTable.getString(block), 
			   gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getU16Fast(const char *block, const char *var, U16 &d, 
								 S32 blocknum)
{
	mMessageReader->getU16(block, var, d, blocknum);
}

void LLMessageSystem::getU16(const char *block, const char *var, U16 &d, 
							 S32 blocknum)
{
	getU16Fast(gMessageStringTable.getString(block), 
			   gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getS32Fast(const char *block, const char *var, S32 &d, 
								 S32 blocknum)
{
	mMessageReader->getS32(block, var, d, blocknum);
}

void LLMessageSystem::getS32(const char *block, const char *var, S32 &d, 
							 S32 blocknum)
{
	getS32Fast(gMessageStringTable.getString(block), 
			   gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getU32Fast(const char *block, const char *var, U32 &d, 
								 S32 blocknum)
{
	mMessageReader->getU32(block, var, d, blocknum);
}

void LLMessageSystem::getU32(const char *block, const char *var, U32 &d, 
							 S32 blocknum)
{
	getU32Fast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getU64Fast(const char *block, const char *var, U64 &d, 
								 S32 blocknum)
{
	mMessageReader->getU64(block, var, d, blocknum);
}

void LLMessageSystem::getU64(const char *block, const char *var, U64 &d, 
							 S32 blocknum)
{
	
	getU64Fast(gMessageStringTable.getString(block), 
			   gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getBinaryDataFast(const char *blockname, 
										const char *varname, 
										void *datap, S32 size, 
										S32 blocknum, S32 max_size)
{
	mMessageReader->getBinaryData(blockname, varname, datap, size, blocknum, 
								  max_size);
}

void LLMessageSystem::getBinaryData(const char *blockname, 
									const char *varname, 
									void *datap, S32 size, 
									S32 blocknum, S32 max_size)
{
	getBinaryDataFast(gMessageStringTable.getString(blockname), 
					  gMessageStringTable.getString(varname), 
					  datap, size, blocknum, max_size);
}

void LLMessageSystem::getF32Fast(const char *block, const char *var, F32 &d, 
								 S32 blocknum)
{
	mMessageReader->getF32(block, var, d, blocknum);
}

void LLMessageSystem::getF32(const char *block, const char *var, F32 &d, 
							 S32 blocknum)
{
	getF32Fast(gMessageStringTable.getString(block), 
			   gMessageStringTable.getString(var), d, blocknum);
}

void LLMessageSystem::getF64Fast(const char *block, const char *var, F64 &d, 
								 S32 blocknum)
{
	mMessageReader->getF64(block, var, d, blocknum);
}

void LLMessageSystem::getF64(const char *block, const char *var, F64 &d, 
							 S32 blocknum)
{
	getF64Fast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), d, blocknum);
}


void LLMessageSystem::getVector3Fast(const char *block, const char *var, 
									 LLVector3 &v, S32 blocknum )
{
	mMessageReader->getVector3(block, var, v, blocknum);
}

void LLMessageSystem::getVector3(const char *block, const char *var, 
								 LLVector3 &v, S32 blocknum )
{
	getVector3Fast(gMessageStringTable.getString(block), 
				   gMessageStringTable.getString(var), v, blocknum);
}

void LLMessageSystem::getVector4Fast(const char *block, const char *var, 
									 LLVector4 &v, S32 blocknum )
{
	mMessageReader->getVector4(block, var, v, blocknum);
}

void LLMessageSystem::getVector4(const char *block, const char *var, 
								 LLVector4 &v, S32 blocknum )
{
	getVector4Fast(gMessageStringTable.getString(block), 
				   gMessageStringTable.getString(var), v, blocknum);
}

void LLMessageSystem::getVector3dFast(const char *block, const char *var, 
									  LLVector3d &v, S32 blocknum )
{
	mMessageReader->getVector3d(block, var, v, blocknum);
}

void LLMessageSystem::getVector3d(const char *block, const char *var, 
								  LLVector3d &v, S32 blocknum )
{
	getVector3dFast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), v, blocknum);
}

void LLMessageSystem::getQuatFast(const char *block, const char *var, 
								  LLQuaternion &q, S32 blocknum )
{
	mMessageReader->getQuat(block, var, q, blocknum);
}

void LLMessageSystem::getQuat(const char *block, const char *var, 
							  LLQuaternion &q, S32 blocknum)
{
	getQuatFast(gMessageStringTable.getString(block), 
			gMessageStringTable.getString(var), q, blocknum);
}

void LLMessageSystem::getUUIDFast(const char *block, const char *var, 
								  LLUUID &u, S32 blocknum )
{
	mMessageReader->getUUID(block, var, u, blocknum);
}

void LLMessageSystem::getUUID(const char *block, const char *var, LLUUID &u, 
							  S32 blocknum )
{
	getUUIDFast(gMessageStringTable.getString(block), 
				gMessageStringTable.getString(var), u, blocknum);
}

void LLMessageSystem::getIPAddrFast(const char *block, const char *var, 
									U32 &u, S32 blocknum)
{
	mMessageReader->getIPAddr(block, var, u, blocknum);
}

void LLMessageSystem::getIPAddr(const char *block, const char *var, U32 &u, 
								S32 blocknum)
{
	getIPAddrFast(gMessageStringTable.getString(block), 
				  gMessageStringTable.getString(var), u, blocknum);
}

void LLMessageSystem::getIPPortFast(const char *block, const char *var, 
									U16 &u, S32 blocknum)
{
	mMessageReader->getIPPort(block, var, u, blocknum);
}

void LLMessageSystem::getIPPort(const char *block, const char *var, U16 &u, 
								S32 blocknum)
{
	getIPPortFast(gMessageStringTable.getString(block), 
				  gMessageStringTable.getString(var), u, 
				  blocknum);
}


void LLMessageSystem::getStringFast(const char *block, const char *var, 
									S32 buffer_size, char *s, S32 blocknum)
{
	if(buffer_size <= 0)
	{
		llwarns << "buffer_size <= 0" << llendl;
	}
	mMessageReader->getString(block, var, buffer_size, s, blocknum);
}

void LLMessageSystem::getString(const char *block, const char *var, 
								S32 buffer_size, char *s, S32 blocknum )
{
	getStringFast(gMessageStringTable.getString(block), 
				  gMessageStringTable.getString(var), buffer_size, s, 
				  blocknum);
}

S32	LLMessageSystem::getNumberOfBlocksFast(const char *blockname)
{
	return mMessageReader->getNumberOfBlocks(blockname);
}

S32	LLMessageSystem::getNumberOfBlocks(const char *blockname)
{
	return getNumberOfBlocksFast(gMessageStringTable.getString(blockname));
}
	
S32	LLMessageSystem::getSizeFast(const char *blockname, const char *varname)
{
	return mMessageReader->getSize(blockname, varname);
}

S32	LLMessageSystem::getSize(const char *blockname, const char *varname)
{
	return getSizeFast(gMessageStringTable.getString(blockname), 
					   gMessageStringTable.getString(varname));
}
	
// size in bytes of variable length data
S32	LLMessageSystem::getSizeFast(const char *blockname, S32 blocknum, 
								 const char *varname)
{
	return mMessageReader->getSize(blockname, blocknum, varname);
}
		
S32	LLMessageSystem::getSize(const char *blockname, S32 blocknum, 
							 const char *varname)
{
	return getSizeFast(gMessageStringTable.getString(blockname), blocknum, 
					   gMessageStringTable.getString(varname));
}

S32 LLMessageSystem::getReceiveSize() const
{
	return mMessageReader->getMessageSize();
}

//static 
void LLMessageSystem::setTimeDecodes( BOOL b )
{
	LLMessageReader::setTimeDecodes(b);
}
		
//static 
void LLMessageSystem::setTimeDecodesSpamThreshold( F32 seconds )
{ 
	LLMessageReader::setTimeDecodesSpamThreshold(seconds);
}

// HACK! babbage: return true if message rxed via either UDP or HTTP
// TODO: babbage: move gServicePump in to LLMessageSystem?
bool LLMessageSystem::checkAllMessages(S64 frame_count, LLPumpIO* http_pump)
{
	if(checkMessages(frame_count))
	{
		return true;
	}
	U32 packetsIn = mPacketsIn;
	http_pump->pump();
	http_pump->callback();
	return (mPacketsIn - packetsIn) > 0;
}
