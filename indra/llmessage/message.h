/** 
 * @file message.h
 * @brief LLMessageSystem class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_MESSAGE_H
#define LL_MESSAGE_H

#include <cstring>
#include <set>

#if LL_LINUX
#include <endian.h>
#include <netinet/in.h>
#endif

#if LL_WINDOWS
#include "winsock2.h" // htons etc.
#endif

#include "llerror.h"
#include "net.h"
#include "llstringtable.h"
#include "llcircuit.h"
#include "lltimer.h"
#include "llpacketring.h"
#include "llhost.h"
#include "llhttpnode.h"
//#include "llpacketack.h"
#include "llsingleton.h"
#include "message_prehash.h"
#include "llstl.h"
#include "llmsgvariabletype.h"
#include "llmessagesenderinterface.h"

#include "llstoredmessage.h"
#include "boost/function.hpp"
#include "llpounceable.h"
#include "llcoros.h"
#include LLCOROS_MUTEX_HEADER

const U32 MESSAGE_MAX_STRINGS_LENGTH = 64;
const U32 MESSAGE_NUMBER_OF_HASH_BUCKETS = 8192;

const S32 MESSAGE_MAX_PER_FRAME = 400;

class LLMessageStringTable : public LLSingleton<LLMessageStringTable>
{
	LLSINGLETON(LLMessageStringTable);
	~LLMessageStringTable();

public:
	char *getString(const char *str);

	U32	 mUsed;
	BOOL mEmpty[MESSAGE_NUMBER_OF_HASH_BUCKETS];
	char mString[MESSAGE_NUMBER_OF_HASH_BUCKETS][MESSAGE_MAX_STRINGS_LENGTH];	/* Flawfinder: ignore */
};


// Individual Messages are described with the following format
// Note that to ease parsing, keywords are used
//
//	// Comment 					(Comment like a C++ single line comment)
//  							Comments can only be placed between Messages
// {
// MessageName					(same naming restrictions as C variable)
// Frequency					("High", "Medium", or "Low" - determines whether message ID is 8, 16, or 32-bits -- 
//								 there can 254 messages in the first 2 groups, 32K in the last group)
//								(A message can be made up only of the Name if it is only a signal)
// Trust						("Trusted", "NotTrusted" - determines if a message will be accepted
//								 on a circuit.  "Trusted" messages are not accepted from NotTrusted circuits
//								 while NotTrusted messages are accepted on any circuit.  An example of a
//								 NotTrusted circuit is any circuit from the viewer.)
// Encoding						("Zerocoded", "Unencoded" - zerocoded messages attempt to compress sequences of 
//								 zeros, but if there is no space win, it discards the compression and goes unencoded)
//		{
//		Block Name				(same naming restrictions as C variable)
//		Block Type				("Single", "Multiple", or "Variable" - determines if the block is coded once, 
//								 a known number of times, or has a 8 bit argument encoded to tell the decoder 
//								 how many times the group is repeated)
//		Block Repeat Number		(Optional - used only with the "Multiple" type - tells how many times the field is repeated
//			{
//			Variable 1 Name		(same naming restrictions as C variable)
//			Variable Type		("Fixed" or "Variable" - determines if the variable is of fixed size or needs to 
//								 encode an argument describing the size in bytes)
//			Variable Size		(In bytes, either of the "Fixed" variable itself or of the size argument)
//
//			repeat variables
//
//			}
//
//			Repeat for number of variables in block		
//		}
//
//		Repeat for number of blocks in message
// }
// Repeat for number of messages in file
//

// Constants
const S32 MAX_MESSAGE_INTERNAL_NAME_SIZE = 255;
const S32 MAX_BUFFER_SIZE = NET_BUFFER_SIZE;
const S32 MAX_BLOCKS = 255;

const U8 LL_ZERO_CODE_FLAG = 0x80;
const U8 LL_RELIABLE_FLAG = 0x40;
const U8 LL_RESENT_FLAG = 0x20;
const U8 LL_ACK_FLAG = 0x10;

// 1 byte flags, 4 bytes sequence, 1 byte offset + 1 byte message name (high)
const S32 LL_MINIMUM_VALID_PACKET_SIZE = LL_PACKET_ID_SIZE + 1;
enum EPacketHeaderLayout
{
	PHL_FLAGS = 0,
	PHL_PACKET_ID = 1,
	PHL_OFFSET = 5,
	PHL_NAME = 6
};


const S32 LL_DEFAULT_RELIABLE_RETRIES = 3;
const F32Seconds LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS(1.f);
const F32Seconds LL_MINIMUM_SEMIRELIABLE_TIMEOUT_SECONDS(1.f);
const F32Seconds LL_PING_BASED_TIMEOUT_DUMMY(0.0f);

const F32 LL_SEMIRELIABLE_TIMEOUT_FACTOR	= 5.f;		// averaged ping
const F32 LL_RELIABLE_TIMEOUT_FACTOR		= 5.f;		// averaged ping
const F32 LL_LOST_TIMEOUT_FACTOR			= 16.f;     // averaged ping for marking packets "Lost"
const F32Seconds LL_MAX_LOST_TIMEOUT(5.f);				// Maximum amount of time before considering something "lost"

const S32 MAX_MESSAGE_COUNT_NUM = 1024;

// Forward declarations
class LLVector3;
class LLVector4;
class LLVector3d;
class LLQuaternion;
class LLSD;
class LLUUID;
class LLMessageSystem;
class LLPumpIO;

// message system exceptional condition handlers.
enum EMessageException
{
	MX_UNREGISTERED_MESSAGE, // message number not part of template
	MX_PACKET_TOO_SHORT, // invalid packet, shorter than minimum packet size
	MX_RAN_OFF_END_OF_PACKET, // ran off the end of the packet during decode
	MX_WROTE_PAST_BUFFER_SIZE // wrote past buffer size in zero code expand
};
typedef void (*msg_exception_callback)(LLMessageSystem*,void*,EMessageException);


// message data pieces are used to collect the data called for by the message template
class LLMsgData;
class LLMsgBlkData;
class LLMessageTemplate;

class LLMessagePollInfo;
class LLMessageBuilder;
class LLTemplateMessageBuilder;
class LLSDMessageBuilder;
class LLMessageReader;
class LLTemplateMessageReader;
class LLSDMessageReader;



class LLUseCircuitCodeResponder
{
	LOG_CLASS(LLMessageSystem);
	
public:
	virtual ~LLUseCircuitCodeResponder();
	virtual void complete(const LLHost& host, const LLUUID& agent) const = 0;
};

/**
 * SL-12204: We've observed crashes when consumer code sets
 * LLMessageSystem::mMessageReader, assuming that all subsequent processing of
 * the current message will use the same mMessageReader value -- only to have
 * a different coroutine sneak in and replace mMessageReader before
 * completion. This is a limitation of sharing a stateful global resource for
 * message parsing; instead code receiving a new message should instantiate a
 * (trivially constructed) local message parser and use that.
 *
 * Until then, when one coroutine sets a particular LLMessageReader subclass
 * as the current message reader, ensure that no other coroutine can replace
 * it until the first coroutine has finished with its message.
 *
 * This is achieved with two helper classes. LLMessageSystem::mMessageReader
 * is now an LLMessageReaderPointer instance, which can efficiently compare or
 * dereference its contained LLMessageReader* but which cannot be directly
 * assigned. To change the value of LLMessageReaderPointer, you must
 * instantiate LockMessageReader with the LLMessageReader* you wish to make
 * current. mMessageReader will have that value for the lifetime of the
 * LockMessageReader instance, then revert to nullptr. Moreover, as its name
 * implies, LockMessageReader locks the mutex in LLMessageReaderPointer so
 * that any other coroutine instantiating LockMessageReader will block until
 * the first coroutine has destroyed its instance.
 */
class LLMessageReaderPointer
{
public:
    LLMessageReaderPointer(): mPtr(nullptr) {}
    // It is essential that comparison and dereferencing must be fast, which
    // is why we don't check for nullptr when dereferencing.
    LLMessageReader* operator->() const { return mPtr; }
    bool operator==(const LLMessageReader* other) const { return mPtr == other; }
    bool operator!=(const LLMessageReader* other) const { return ! (*this == other); }
private:
    // Only LockMessageReader can set mPtr.
    friend class LockMessageReader;
    LLMessageReader* mPtr;
    LLCoros::Mutex mMutex;
};

/**
 * To set mMessageReader to nullptr:
 *
 * @code
 * // use an anonymous instance that is destroyed immediately
 * LockMessageReader(gMessageSystem->mMessageReader, nullptr);
 * @endcode
 *
 * Why do we still require going through LockMessageReader at all? Because it
 * would be Bad if any coroutine set mMessageReader to nullptr while another
 * coroutine was still parsing a message.
 */
class LockMessageReader
{
public:
    LockMessageReader(LLMessageReaderPointer& var, LLMessageReader* instance):
        mVar(var.mPtr),
        mLock(var.mMutex)
    {
        mVar = instance;
    }
    // Some compilers reportedly fail to suppress generating implicit copy
    // operations even though we have a move-only LockType data member.
    LockMessageReader(const LockMessageReader&) = delete;
    LockMessageReader& operator=(const LockMessageReader&) = delete;
    ~LockMessageReader()
    {
        mVar = nullptr;
    }
private:
    // capture a reference to LLMessageReaderPointer::mPtr
    decltype(LLMessageReaderPointer::mPtr)& mVar;
    // while holding a lock on LLMessageReaderPointer::mMutex
    LLCoros::LockType mLock;
};

/**
 * LockMessageReader is great as long as you only need mMessageReader locked
 * during a single LLMessageSystem function call. However, empirically the
 * sequence from checkAllMessages() through processAcks() need mMessageReader
 * locked to LLTemplateMessageReader. Enforce that by making them require an
 * instance of LockMessageChecker.
 */
class LockMessageChecker;

class LLMessageSystem : public LLMessageSenderInterface
{
 private:
	U8					mSendBuffer[MAX_BUFFER_SIZE];
	S32					mSendSize;

	bool				mBlockUntrustedInterface;
	LLHost				mUntrustedInterface;

 public:
	LLPacketRing				mPacketRing;
	LLReliablePacketParams		mReliablePacketParams;

	// Set this flag to TRUE when you want *very* verbose logs.
	BOOL						mVerboseLog;

	F32                         mMessageFileVersionNumber;

	typedef std::map<const char *, LLMessageTemplate*> message_template_name_map_t;
	typedef std::map<U32, LLMessageTemplate*> message_template_number_map_t;

private:
	message_template_name_map_t		mMessageTemplates;
	message_template_number_map_t	mMessageNumbers;

public:
	S32					mSystemVersionMajor;
	S32					mSystemVersionMinor;
	S32					mSystemVersionPatch;
	S32					mSystemVersionServer;
	U32					mVersionFlags;

	BOOL				mbProtected;

	U32					mNumberHighFreqMessages;
	U32					mNumberMediumFreqMessages;
	U32					mNumberLowFreqMessages;
	S32					mPort;
	S32					mSocket;

	U32					mPacketsIn;		    // total packets in, including compressed and uncompressed
	U32					mPacketsOut;		    // total packets out, including compressed and uncompressed

	U64					mBytesIn;		    // total bytes in, including compressed and uncompressed
	U64					mBytesOut;		    // total bytes out, including compressed and uncompressed
	
	U32					mCompressedPacketsIn;	    // total compressed packets in
	U32					mCompressedPacketsOut;	    // total compressed packets out

	U32					mReliablePacketsIn;	    // total reliable packets in
	U32					mReliablePacketsOut;	    // total reliable packets out

	U32                 mDroppedPackets;            // total dropped packets in
	U32                 mResentPackets;             // total resent packets out
	U32                 mFailedResendPackets;       // total resend failure packets out
	U32                 mOffCircuitPackets;         // total # of off-circuit packets rejected
	U32                 mInvalidOnCircuitPackets;   // total # of on-circuit but invalid packets rejected

	S64					mUncompressedBytesIn;	    // total uncompressed size of compressed packets in
	S64					mUncompressedBytesOut;	    // total uncompressed size of compressed packets out
	S64					mCompressedBytesIn;	    // total compressed size of compressed packets in
	S64					mCompressedBytesOut;	    // total compressed size of compressed packets out
	S64					mTotalBytesIn;		    // total size of all uncompressed packets in
	S64					mTotalBytesOut;		    // total size of all uncompressed packets out

	BOOL                mSendReliable;              // does the outgoing message require a pos ack?

	LLCircuit 	 		mCircuitInfo;
	F64Seconds			mCircuitPrintTime;	    // used to print circuit debug info every couple minutes
	F32Seconds			mCircuitPrintFreq;	    

	std::map<U64, U32>	mIPPortToCircuitCode;
	std::map<U32, U64>	mCircuitCodeToIPPort;
	U32					mOurCircuitCode;
	S32					mSendPacketFailureCount;
	S32					mUnackedListDepth;
	S32					mUnackedListSize;
	S32					mDSMaxListDepth;

public:
	// Read file and build message templates
	LLMessageSystem(const std::string& filename, U32 port, S32 version_major,
					S32 version_minor, S32 version_patch,
					bool failure_is_fatal,
					const F32 circuit_heartbeat_interval, const F32 circuit_timeout);

	~LLMessageSystem();

	BOOL isOK() const { return !mbError; }
	S32 getErrorCode() const { return mErrorCode; }

	// Read file and build message templates filename must point to a
	// valid string which specifies the path of a valid linden
	// template.
	void loadTemplateFile(const std::string& filename, bool failure_is_fatal);


	// methods for building, sending, receiving, and handling messages
	void	setHandlerFuncFast(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL);
	void	setHandlerFunc(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL)
	{
		setHandlerFuncFast(LLMessageStringTable::getInstance()->getString(name), handler_func, user_data);
	}

	// Set a callback function for a message system exception.
	void setExceptionFunc(EMessageException exception, msg_exception_callback func, void* data = NULL);
	// Call the specified exception func, and return TRUE if a
	// function was found and called. Otherwise return FALSE.
	BOOL callExceptionFunc(EMessageException exception);

	// Set a function that will be called once per packet processed with the 
	// hashed message name and the time spent in the processing handler function
	// measured in seconds.  JC
	typedef void (*msg_timing_callback)(const char* hashed_name, F32 time, void* data);
	void setTimingFunc(msg_timing_callback func, void* data = NULL);
	msg_timing_callback getTimingCallback() 
	{ 
		return mTimingCallback; 
	}
	void* getTimingCallbackData() 
	{
		return mTimingCallbackData;
	}

	// This method returns true if the code is in the circuit codes map.
	BOOL isCircuitCodeKnown(U32 code) const;

	// usually called in response to an AddCircuitCode message, but
	// may also be called by the login process.
	bool addCircuitCode(U32 code, const LLUUID& session_id);

	BOOL	poll(F32 seconds); // Number of seconds that we want to block waiting for data, returns if data was received
	BOOL	checkMessages(LockMessageChecker&, S64 frame_count = 0 );
	void	processAcks(LockMessageChecker&, F32 collect_time = 0.f);

	BOOL	isMessageFast(const char *msg);
	BOOL	isMessage(const char *msg)
	{
		return isMessageFast(LLMessageStringTable::getInstance()->getString(msg));
	}

	void dumpPacketToLog();

	char	*getMessageName();

	const LLHost& getSender() const;
	U32		getSenderIP() const;			// getSender() is preferred
	U32		getSenderPort() const;		// getSender() is preferred

	const LLHost& getReceivingInterface() const;

	// This method returns the uuid associated with the sender. The
	// UUID will be null if it is not yet known or is a server
	// circuit.
	const LLUUID& getSenderID() const;

	// This method returns the session id associated with the last
	// sender.
	const LLUUID& getSenderSessionID() const;

	// set & get the session id (useful for viewers for now.)
	void setMySessionID(const LLUUID& session_id) { mSessionID = session_id; }
	const LLUUID& getMySessionID() { return mSessionID; }

	void newMessageFast(const char *name);
	void newMessage(const char *name);


public:
	LLStoredMessagePtr getReceivedMessage() const; 
	LLStoredMessagePtr getBuiltMessage() const;
	S32 sendMessage(const LLHost &host, LLStoredMessagePtr message);

private:
	LLSD getReceivedMessageLLSD() const;
	LLSD getBuiltMessageLLSD() const;

	// NOTE: babbage: Only use to support legacy misuse of the
	// LLMessageSystem API where values are dangerously written
	// as one type and read as another. LLSD does not support
	// dangerous conversions and so converting the message to an
	// LLSD would result in the reads failing. All code which
	// misuses the message system in this way should be made safe
	// but while the unsafe code is run in old processes, this
	// method should be used to forward unsafe messages.
	LLSD wrapReceivedTemplateData() const;
	LLSD wrapBuiltTemplateData() const;

public:

	void copyMessageReceivedToSend();
	void clearMessage();

	void nextBlockFast(const char *blockname);
	void nextBlock(const char *blockname);

public:
	void addBinaryDataFast(const char *varname, const void *data, S32 size);
	void addBinaryData(const char *varname, const void *data, S32 size);

	void	addBOOLFast( const char* varname, BOOL b);						// typed, checks storage space
	void	addBOOL( const char* varname, BOOL b);						// typed, checks storage space
	void	addS8Fast(	const char *varname, S8 s);							// typed, checks storage space
	void	addS8(	const char *varname, S8 s);							// typed, checks storage space
	void	addU8Fast(	const char *varname, U8 u);							// typed, checks storage space
	void	addU8(	const char *varname, U8 u);							// typed, checks storage space
	void	addS16Fast(	const char *varname, S16 i);						// typed, checks storage space
	void	addS16(	const char *varname, S16 i);						// typed, checks storage space
	void	addU16Fast(	const char *varname, U16 i);						// typed, checks storage space
	void	addU16(	const char *varname, U16 i);						// typed, checks storage space
	void	addF32Fast(	const char *varname, F32 f);						// typed, checks storage space
	void	addF32(	const char *varname, F32 f);						// typed, checks storage space
	void	addS32Fast(	const char *varname, S32 s);						// typed, checks storage space
	void	addS32(	const char *varname, S32 s);						// typed, checks storage space
	void addU32Fast(	const char *varname, U32 u);						// typed, checks storage space
	void	addU32(	const char *varname, U32 u);						// typed, checks storage space
	void	addU64Fast(	const char *varname, U64 lu);						// typed, checks storage space
	void	addU64(	const char *varname, U64 lu);						// typed, checks storage space
	void	addF64Fast(	const char *varname, F64 d);						// typed, checks storage space
	void	addF64(	const char *varname, F64 d);						// typed, checks storage space
	void	addVector3Fast(	const char *varname, const LLVector3& vec);		// typed, checks storage space
	void	addVector3(	const char *varname, const LLVector3& vec);		// typed, checks storage space
	void	addVector4Fast(	const char *varname, const LLVector4& vec);		// typed, checks storage space
	void	addVector4(	const char *varname, const LLVector4& vec);		// typed, checks storage space
	void	addVector3dFast( const char *varname, const LLVector3d& vec);	// typed, checks storage space
	void	addVector3d( const char *varname, const LLVector3d& vec);	// typed, checks storage space
	void	addQuatFast( const char *varname, const LLQuaternion& quat);	// typed, checks storage space
	void	addQuat( const char *varname, const LLQuaternion& quat);	// typed, checks storage space
	void addUUIDFast( const char *varname, const LLUUID& uuid);			// typed, checks storage space
	void	addUUID( const char *varname, const LLUUID& uuid);			// typed, checks storage space
	void	addIPAddrFast( const char *varname, const U32 ip);			// typed, checks storage space
	void	addIPAddr( const char *varname, const U32 ip);			// typed, checks storage space
	void	addIPPortFast( const char *varname, const U16 port);			// typed, checks storage space
	void	addIPPort( const char *varname, const U16 port);			// typed, checks storage space
	void	addStringFast( const char* varname, const char* s);				// typed, checks storage space
	void	addString( const char* varname, const char* s);				// typed, checks storage space
	void	addStringFast( const char* varname, const std::string& s);				// typed, checks storage space
	void	addString( const char* varname, const std::string& s);				// typed, checks storage space

	S32 getCurrentSendTotal() const;
	TPACKETID getCurrentRecvPacketID() { return mCurrentRecvPacketID; }

	// This method checks for current send total and returns true if
	// you need to go to the next block type or need to start a new
	// message. Specify the current blockname to check block counts,
	// otherwise the method only checks against MTU.
	BOOL isSendFull(const char* blockname = NULL);
	BOOL isSendFullFast(const char* blockname = NULL);

	BOOL removeLastBlock();

	//void	buildMessage();

	S32     zeroCode(U8 **data, S32 *data_size);
	S32		zeroCodeExpand(U8 **data, S32 *data_size);
	S32		zeroCodeAdjustCurrentSendTotal();

	// Uses ping-based retry
	S32 sendReliable(const LLHost &host);

	// Uses ping-based retry
	S32	sendReliable(const U32 circuit)			{ return sendReliable(findHost(circuit)); }

	// Use this one if you DON'T want automatic ping-based retry.
	S32	sendReliable(	const LLHost &host, 
							S32 retries, 
							BOOL ping_based_retries,
							F32Seconds timeout, 
							void (*callback)(void **,S32), 
							void ** callback_data);

	S32 sendSemiReliable(	const LLHost &host, 
							void (*callback)(void **,S32), void ** callback_data);

	// flush sends a message only if data's been pushed on it.
	S32	 flushSemiReliable(	const LLHost &host, 
								void (*callback)(void **,S32), void ** callback_data);

	S32	flushReliable(	const LLHost &host );

	void forwardMessage(const LLHost &host);
	void forwardReliable(const LLHost &host);
	void forwardReliable(const U32 circuit_code);
	S32 forwardReliable(
		const LLHost &host, 
		S32 retries, 
		BOOL ping_based_timeout,
		F32Seconds timeout, 
		void (*callback)(void **,S32), 
		void ** callback_data);

	S32		sendMessage(const LLHost &host);
	S32		sendMessage(const U32 circuit);
private:
	S32		sendMessage(const LLHost &host, const char* name,
						const LLSD& message);
public:
	// BOOL	decodeData(const U8 *buffer, const LLHost &host);

	/**
	gets binary data from the current message.
	
	@param blockname the name of the block in the message (from the message template)

	@param varname 

	@param datap
	
	@param size expected size - set to zero to get any amount of data up to max_size.
	Make sure max_size is set in that case!

	@param blocknum

	@param max_size the max number of bytes to read
	*/
	void	getBinaryDataFast(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX);
	void	getBinaryData(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX);
	void	getBOOLFast(	const char *block, const char *var, BOOL &data, S32 blocknum = 0);
	void	getBOOL(	const char *block, const char *var, BOOL &data, S32 blocknum = 0);
	void	getS8Fast(		const char *block, const char *var, S8 &data, S32 blocknum = 0);
	void	getS8(		const char *block, const char *var, S8 &data, S32 blocknum = 0);
	void	getU8Fast(		const char *block, const char *var, U8 &data, S32 blocknum = 0);
	void	getU8(		const char *block, const char *var, U8 &data, S32 blocknum = 0);
	void	getS16Fast(		const char *block, const char *var, S16 &data, S32 blocknum = 0);
	void	getS16(		const char *block, const char *var, S16 &data, S32 blocknum = 0);
	void	getU16Fast(		const char *block, const char *var, U16 &data, S32 blocknum = 0);
	void	getU16(		const char *block, const char *var, U16 &data, S32 blocknum = 0);
	void	getS32Fast(		const char *block, const char *var, S32 &data, S32 blocknum = 0);
	void	getS32(		const char *block, const char *var, S32 &data, S32 blocknum = 0);
	void	getF32Fast(		const char *block, const char *var, F32 &data, S32 blocknum = 0);
	void	getF32(		const char *block, const char *var, F32 &data, S32 blocknum = 0);
	void getU32Fast(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	void	getU32(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	void getU64Fast(		const char *block, const char *var, U64 &data, S32 blocknum = 0);
	void	getU64(		const char *block, const char *var, U64 &data, S32 blocknum = 0);
	void	getF64Fast(		const char *block, const char *var, F64 &data, S32 blocknum = 0);
	void	getF64(		const char *block, const char *var, F64 &data, S32 blocknum = 0);
	void	getVector3Fast(	const char *block, const char *var, LLVector3 &vec, S32 blocknum = 0);
	void	getVector3(	const char *block, const char *var, LLVector3 &vec, S32 blocknum = 0);
	void	getVector4Fast(	const char *block, const char *var, LLVector4 &vec, S32 blocknum = 0);
	void	getVector4(	const char *block, const char *var, LLVector4 &vec, S32 blocknum = 0);
	void	getVector3dFast(const char *block, const char *var, LLVector3d &vec, S32 blocknum = 0);
	void	getVector3d(const char *block, const char *var, LLVector3d &vec, S32 blocknum = 0);
	void	getQuatFast(	const char *block, const char *var, LLQuaternion &q, S32 blocknum = 0);
	void	getQuat(	const char *block, const char *var, LLQuaternion &q, S32 blocknum = 0);
	void getUUIDFast(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	void	getUUID(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	void getIPAddrFast(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	void	getIPAddr(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	void getIPPortFast(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	void	getIPPort(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	void getStringFast(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);
	void	getString(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);
	void getStringFast(	const char *block, const char *var, std::string& outstr, S32 blocknum = 0);
	void	getString(	const char *block, const char *var, std::string& outstr, S32 blocknum = 0);


	// Utility functions to generate a replay-resistant digest check
	// against the shared secret. The window specifies how much of a
	// time window is allowed - 1 second is good for tight
	// connections, but multi-process windows might want to be upwards
	// of 5 seconds. For generateDigest, you want to pass in a
	// character array of at least MD5HEX_STR_SIZE so that the hex
	// digest and null termination will fit.
	bool generateDigestForNumberAndUUIDs(char* digest, const U32 number, const LLUUID &id1, const LLUUID &id2) const;
	bool generateDigestForWindowAndUUIDs(char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const;
	bool isMatchingDigestForWindowAndUUIDs(const char* digest, const S32 window, const LLUUID &id1, const LLUUID &id2) const;

	bool generateDigestForNumber(char* digest, const U32 number) const;
	bool generateDigestForWindow(char* digest, const S32 window) const;
	bool isMatchingDigestForWindow(const char* digest, const S32 window) const;

	void	showCircuitInfo();
	void getCircuitInfo(LLSD& info) const;

	U32 getOurCircuitCode();
	
	void	enableCircuit(const LLHost &host, BOOL trusted);
	void	disableCircuit(const LLHost &host);
	
	// Use this to establish trust on startup and in response to
	// DenyTrustedCircuit.
	void sendCreateTrustedCircuit(const LLHost& host, const LLUUID & id1, const LLUUID & id2);

	// Use this to inform a peer that they aren't currently trusted...
	// This now enqueues the request so that we can ensure that we only send
	// one deny per circuit per message loop so that this doesn't become a DoS.
	// The actual sending is done by reallySendDenyTrustedCircuit()
	void	sendDenyTrustedCircuit(const LLHost &host);

	/** Return false if host is unknown or untrusted */
	// Note:DaveH/Babbage some trusted messages can be received without a circuit
	bool isTrustedSender(const LLHost& host) const;

	/** Return true if current message is from trusted source */
	bool isTrustedSender() const;

	/** Return false true if name is unknown or untrusted */
	bool isTrustedMessage(const std::string& name) const;

	/** Return false true if name is unknown or trusted */
	bool isUntrustedMessage(const std::string& name) const;

	// Mark an interface ineligible for trust
	void setUntrustedInterface( const LLHost host ) { mUntrustedInterface = host; }
	LLHost getUntrustedInterface() const { return mUntrustedInterface; }
	void setBlockUntrustedInterface( bool block ) { mBlockUntrustedInterface = block; } // Throw a switch to allow, sending warnings only
	bool getBlockUntrustedInterface() const { return mBlockUntrustedInterface; }

	// Change this message to be UDP black listed.
	void banUdpMessage(const std::string& name);


private:
	// A list of the circuits that need to be sent DenyTrustedCircuit messages.
	typedef std::set<LLHost> host_set_t;
	host_set_t mDenyTrustedCircuitSet;

	// Really sends the DenyTrustedCircuit message to a given host
	// related to sendDenyTrustedCircuit()
	void	reallySendDenyTrustedCircuit(const LLHost &host);

public:
	// Use this to establish trust to and from a host.  This blocks
	// until trust has been established, and probably should only be
	// used on startup.
	void	establishBidirectionalTrust(const LLHost &host, S64 frame_count = 0);

	// returns whether the given host is on a trusted circuit
	// Note:DaveH/Babbage some trusted messages can be received without a circuit
	BOOL    getCircuitTrust(const LLHost &host);
	
	void	setCircuitAllowTimeout(const LLHost &host, BOOL allow);
	void	setCircuitTimeoutCallback(const LLHost &host, void (*callback_func)(const LLHost &host, void *user_data), void *user_data);

	BOOL	checkCircuitBlocked(const U32 circuit);
	BOOL	checkCircuitAlive(const U32 circuit);
	BOOL	checkCircuitAlive(const LLHost &host);
	void	setCircuitProtection(BOOL b_protect);
	U32		findCircuitCode(const LLHost &host);
	LLHost	findHost(const U32 circuit_code);
	void	sanityCheck();

	BOOL	has(const char *blockname) const;
	S32		getNumberOfBlocksFast(const char *blockname) const;
	S32		getNumberOfBlocks(const char *blockname) const;
	S32		getSizeFast(const char *blockname, const char *varname) const;
	S32		getSize(const char *blockname, const char *varname) const;
	S32		getSizeFast(const char *blockname, S32 blocknum, 
						const char *varname) const; // size in bytes of data
	S32		getSize(const char *blockname, S32 blocknum, const char *varname) const;

	void	resetReceiveCounts();				// resets receive counts for all message types to 0
	void	dumpReceiveCounts();				// dumps receive count for each message type to LL_INFOS()
	void	dumpCircuitInfo();					// Circuit information to LL_INFOS()

	BOOL	isClear() const;					// returns mbSClear;
	S32 	flush(const LLHost &host);

	U32		getListenPort( void ) const;

	void startLogging();					// start verbose  logging
	void stopLogging();						// flush and close file
	void summarizeLogs(std::ostream& str);	// log statistics

	S32		getReceiveSize() const;
	S32		getReceiveCompressedSize() const { return mIncomingCompressedSize; }
	S32		getReceiveBytes() const;

	S32		getUnackedListSize() const			{ return mUnackedListSize; }

	//const char* getCurrentSMessageName() const { return mCurrentSMessageName; }
	//const char* getCurrentSBlockName() const { return mCurrentSBlockName; }

	// friends
	friend std::ostream&	operator<<(std::ostream& s, LLMessageSystem &msg);

	void setMaxMessageTime(const F32 seconds);	// Max time to process messages before warning and dumping (neg to disable)
	void setMaxMessageCounts(const S32 num);	// Max number of messages before dumping (neg to disable)
	
	static U64Microseconds getMessageTimeUsecs(const BOOL update = FALSE);	// Get the current message system time in microseconds
	static F64Seconds getMessageTimeSeconds(const BOOL update = FALSE); // Get the current message system time in seconds

	static void setTimeDecodes(BOOL b);
	static void setTimeDecodesSpamThreshold(F32 seconds); 

	// message handlers internal to the message systesm
	//static void processAssignCircuitCode(LLMessageSystem* msg, void**);
	static void processAddCircuitCode(LLMessageSystem* msg, void**);
	static void processUseCircuitCode(LLMessageSystem* msg, void**);
	static void processError(LLMessageSystem* msg, void**);

	// dispatch llsd message to http node tree
	static void dispatch(const std::string& msg_name,
						 const LLSD& message);
	static void dispatch(const std::string& msg_name,
						 const LLSD& message,
						 LLHTTPNode::ResponsePtr responsep);

	// this is added to support specific legacy messages and is
	// ***not intended for general use*** Si, Gabriel, 2009
	static void dispatchTemplate(const std::string& msg_name,
						 const LLSD& message,
						 LLHTTPNode::ResponsePtr responsep);

	void setMessageBans(const LLSD& trusted, const LLSD& untrusted);

	/**
	 * @brief send an error message to the host. This is a helper method.
	 *
	 * @param host Destination host.
	 * @param agent_id Destination agent id (may be null)
	 * @param code An HTTP status compatible error code.
	 * @param token A specific short string based message
	 * @param id The transactionid/uniqueid/sessionid whatever.
	 * @param system The hierarchical path to the system (255 bytes)
	 * @param message Human readable message (1200 bytes) 
	 * @param data Extra info.
	 * @return Returns value returned from sendReliable().
	 */
	S32 sendError(
		const LLHost& host,
		const LLUUID& agent_id,
		S32 code,
		const std::string& token,
		const LLUUID& id,
		const std::string& system,
		const std::string& message,
		const LLSD& data);

	// Check UDP messages and pump http_pump to receive HTTP messages.
	bool checkAllMessages(LockMessageChecker&, S64 frame_count, LLPumpIO* http_pump);

	// Moved to allow access from LLTemplateMessageDispatcher
	void clearReceiveState();

	// This will cause all trust queries to return true until the next message
	// is read: use with caution!
	void receivedMessageFromTrustedSender();
	
private:
    typedef boost::function<void(S32)>  UntrustedCallback_t;
    void sendUntrustedSimulatorMessageCoro(std::string url, std::string message, LLSD body, UntrustedCallback_t callback);


	bool mLastMessageFromTrustedMessageService;
	
	// The mCircuitCodes is a map from circuit codes to session
	// ids. This allows us to verify sessions on connect.
	typedef std::map<U32, LLUUID> code_session_map_t;
	code_session_map_t mCircuitCodes;

	// Viewers need to track a process session in order to make sure
	// that no one gives them a bad circuit code.
	LLUUID mSessionID;
	
	void	addTemplate(LLMessageTemplate *templatep);
	BOOL		decodeTemplate( const U8* buffer, S32 buffer_size, LLMessageTemplate** msg_template );

	void		logMsgFromInvalidCircuit( const LLHost& sender, BOOL recv_reliable );
	void		logTrustedMsgFromUntrustedCircuit( const LLHost& sender );
	void		logValidMsg(LLCircuitData *cdp, const LLHost& sender, BOOL recv_reliable, BOOL recv_resent, BOOL recv_acks );
	void		logRanOffEndOfPacket( const LLHost& sender );

	class LLMessageCountInfo
	{
	public:
		U32 mMessageNum;
		U32 mMessageBytes;
		BOOL mInvalid;
	};

	LLMessagePollInfo						*mPollInfop;

	U8	mEncodedRecvBuffer[MAX_BUFFER_SIZE];
	U8	mTrueReceiveBuffer[MAX_BUFFER_SIZE];
	S32	mTrueReceiveSize;

	// Must be valid during decode
	
	BOOL	mbError;
	S32	mErrorCode;

	F64Seconds										mResendDumpTime; // The last time we dumped resends

	LLMessageCountInfo mMessageCountList[MAX_MESSAGE_COUNT_NUM];
	S32 mNumMessageCounts;
	F32Seconds mReceiveTime;
	F32Seconds mMaxMessageTime; // Max number of seconds for processing messages
	S32 mMaxMessageCounts; // Max number of messages to process before dumping.
	F64Seconds mMessageCountTime;

	F64Seconds mCurrentMessageTime; // The current "message system time" (updated the first call to checkMessages after a resetReceiveCount

	// message system exceptions
	typedef std::pair<msg_exception_callback, void*> exception_t;
	typedef std::map<EMessageException, exception_t> callbacks_t;
	callbacks_t mExceptionCallbacks;

	// stuff for logging
	LLTimer mMessageSystemTimer;

	static F32 mTimeDecodesSpamThreshold;  // If mTimeDecodes is on, all this many seconds for each msg decode before spamming
	static BOOL mTimeDecodes;  // Measure time for all message decodes if TRUE;

	msg_timing_callback mTimingCallback;
	void* mTimingCallbackData;

	void init(); // ctor shared initialisation.

	LLHost mLastSender;
	LLHost mLastReceivingIF;
	S32 mIncomingCompressedSize;		// original size of compressed msg (0 if uncomp.)
	TPACKETID mCurrentRecvPacketID;       // packet ID of current receive packet (for reporting)

	LLMessageBuilder* mMessageBuilder;
	LLTemplateMessageBuilder* mTemplateMessageBuilder;
	LLSDMessageBuilder* mLLSDMessageBuilder;
	LLMessageReaderPointer mMessageReader;
	LLTemplateMessageReader* mTemplateMessageReader;
	LLSDMessageReader* mLLSDMessageReader;

	friend class LLMessageHandlerBridge;
	friend class LockMessageChecker;

	bool callHandler(const char *name, bool trustedSource,
					 LLMessageSystem* msg);


	/** Find, create or revive circuit for host as needed */
	LLCircuitData* findCircuit(const LLHost& host, bool resetPacketId);
};


// external hook into messaging system
extern LLPounceable<LLMessageSystem*, LLPounceableStatic> gMessageSystem;

// Implementation of LockMessageChecker depends on definition of
// LLMessageSystem, hence must follow it.
class LockMessageChecker: public LockMessageReader
{
public:
    LockMessageChecker(LLMessageSystem* msgsystem);

    // For convenience, provide forwarding wrappers so you can call (e.g.)
    // checkAllMessages() on your LockMessageChecker instance instead of
    // passing the instance to LLMessageSystem::checkAllMessages(). Use
    // perfect forwarding to avoid having to maintain these wrappers in sync
    // with the target methods.
    template <typename... ARGS>
    bool checkAllMessages(ARGS&&... args)
    {
        return mMessageSystem->checkAllMessages(*this, std::forward<ARGS>(args)...);
    }

    template <typename... ARGS>
    bool checkMessages(ARGS&&... args)
    {
        return mMessageSystem->checkMessages(*this, std::forward<ARGS>(args)...);
    }

    template <typename... ARGS>
    void processAcks(ARGS&&... args)
    {
        return mMessageSystem->processAcks(*this, std::forward<ARGS>(args)...);
    }

private:
    LLMessageSystem* mMessageSystem;
};

// Must specific overall system version, which is used to determine
// if a patch is available in the message template checksum verification.
// Return true if able to initialize system.
bool start_messaging_system(
	const std::string& template_name,
	U32 port, 
	S32 version_major,
	S32 version_minor,
	S32 version_patch,
	bool b_dump_prehash_file,
	const std::string& secret,
	const LLUseCircuitCodeResponder* responder,
	bool failure_is_fatal,
	const F32 circuit_heartbeat_interval, 
	const F32 circuit_timeout);

void end_messaging_system(bool print_summary = true);

void null_message_callback(LLMessageSystem *msg, void **data);

//
// Inlines
//

#if !defined( LL_BIG_ENDIAN ) && !defined( LL_LITTLE_ENDIAN )
#error Unknown endianness for htolememcpy. Did you miss a common include?
#endif

static inline void *htolememcpy(void *vs, const void *vct, EMsgVariableType type, size_t n)
{
	char *s = (char *)vs;
	const char *ct = (const char *)vct;
#ifdef LL_BIG_ENDIAN
	S32 i, length;
#endif
	switch(type)
	{
	case MVT_FIXED:
	case MVT_VARIABLE:
	case MVT_U8:
	case MVT_S8:
	case MVT_BOOL:
	case MVT_LLUUID:
	case MVT_IP_ADDR:	// these two are swizzled in the getters and setters
	case MVT_IP_PORT:	// these two are swizzled in the getters and setters
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 

	case MVT_U16:
	case MVT_S16:
		if (n != 2)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 1) = *(ct);
		*(s) = *(ct + 1);
		return(vs);
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U32:
	case MVT_S32:
	case MVT_F32:
		if (n != 4)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 3) = *(ct);
		*(s + 2) = *(ct + 1);
		*(s + 1) = *(ct + 2);
		*(s) = *(ct + 3);
		return(vs);
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U64:
	case MVT_S64:
	case MVT_F64:
		if (n != 8)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		*(s + 7) = *(ct);
		*(s + 6) = *(ct + 1);
		*(s + 5) = *(ct + 2);
		*(s + 4) = *(ct + 3);
		*(s + 3) = *(ct + 4);
		*(s + 2) = *(ct + 5);
		*(s + 1) = *(ct + 6);
		*(s) = *(ct + 7);
		return(vs);
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_LLVector3:
	case MVT_LLQuaternion:  // We only send x, y, z and infer w (we set x, y, z to ensure that w >= 0)
		if (n != 12)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htolememcpy(s + 8, ct + 8, MVT_F32, 4);
		htolememcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htolememcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_LLVector3d:
		if (n != 24)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htolememcpy(s + 16, ct + 16, MVT_F64, 8);
		htolememcpy(s + 8, ct + 8, MVT_F64, 8);
		return(htolememcpy(s, ct, MVT_F64, 8));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_LLVector4:
		if (n != 16)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htolememcpy(s + 12, ct + 12, MVT_F32, 4);
		htolememcpy(s + 8, ct + 8, MVT_F32, 4);
		htolememcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htolememcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U16Vec3:
		if (n != 6)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htolememcpy(s + 4, ct + 4, MVT_U16, 2);
		htolememcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htolememcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U16Quat:
		if (n != 8)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		htolememcpy(s + 6, ct + 6, MVT_U16, 2);
		htolememcpy(s + 4, ct + 4, MVT_U16, 2);
		htolememcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htolememcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_S16Array:
		if (n % 2)
		{
			LL_ERRS() << "Size argument passed to htolememcpy doesn't match swizzle type size" << LL_ENDL;
		}
#ifdef LL_BIG_ENDIAN
		length = n % 2;
		for (i = 1; i < length; i++)
		{
			htolememcpy(s + i*2, ct + i*2, MVT_S16, 2);
		}
		return(htolememcpy(s, ct, MVT_S16, 2));
#else
		return(memcpy(s,ct,n));
#endif

	default:
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
	}
}

inline void *ntohmemcpy(void *s, const void *ct, EMsgVariableType type, size_t n)
{
	return(htolememcpy(s,ct,type, n));
}

inline const LLHost& LLMessageSystem::getReceivingInterface() const {return mLastReceivingIF;}

inline U32 LLMessageSystem::getSenderIP() const 
{
	return mLastSender.getAddress();
}

inline U32 LLMessageSystem::getSenderPort() const
{
	return mLastSender.getPort();
}


//-----------------------------------------------------------------------------
// Transmission aliases
//-----------------------------------------------------------------------------

inline S32 LLMessageSystem::sendMessage(const U32 circuit)
{
	return sendMessage(findHost(circuit));
}

#endif
