/** 
 * @file message.h
 * @brief LLMessageSystem class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MESSAGE_H
#define LL_MESSAGE_H

#include <cstring>
#include <stdio.h>
#include <map>
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
#include "string_table.h"
#include "llcircuit.h"
#include "lltimer.h"
#include "llpacketring.h"
#include "llhost.h"
#include "llpacketack.h"
#include "message_prehash.h"
#include "llstl.h"

const U32 MESSAGE_MAX_STRINGS_LENGTH = 64;
const U32 MESSAGE_NUMBER_OF_HASH_BUCKETS = 8192;

const S32 MESSAGE_MAX_PER_FRAME = 400;

class LLMessageStringTable
{
public:
	LLMessageStringTable();
	~LLMessageStringTable();

	char *getString(const char *str);

	U32	 mUsed;
	BOOL mEmpty[MESSAGE_NUMBER_OF_HASH_BUCKETS];
	char mString[MESSAGE_NUMBER_OF_HASH_BUCKETS][MESSAGE_MAX_STRINGS_LENGTH];	/* Flawfinder: ignore */
};

extern LLMessageStringTable gMessageStringTable;

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

const S32	LL_MINIMUM_VALID_PACKET_SIZE = LL_PACKET_ID_SIZE + 1;		// 4 bytes id + 1 byte message name (high)

const S32 LL_DEFAULT_RELIABLE_RETRIES = 3;
const F32 LL_MINIMUM_RELIABLE_TIMEOUT_SECONDS = 1.f;
const F32 LL_MINIMUM_SEMIRELIABLE_TIMEOUT_SECONDS = 1.f;
const F32 LL_PING_BASED_TIMEOUT_DUMMY = 0.0f;

// *NOTE: Maybe these factors shouldn't include the msec to sec conversion
// implicitly.
// However, all units should be MKS.
const F32 LL_SEMIRELIABLE_TIMEOUT_FACTOR	= 5.f / 1000.f;		// factor * averaged ping
const F32 LL_RELIABLE_TIMEOUT_FACTOR		= 5.f / 1000.f;      // factor * averaged ping
const F32 LL_FILE_XFER_TIMEOUT_FACTOR		= 5.f / 1000.f;      // factor * averaged ping
const F32 LL_LOST_TIMEOUT_FACTOR			= 16.f / 1000.f;     // factor * averaged ping for marking packets "Lost"
const F32 LL_MAX_LOST_TIMEOUT				= 5.f;				// Maximum amount of time before considering something "lost"

const S32 MAX_MESSAGE_COUNT_NUM = 1024;

// Forward declarations
class LLCircuit;
class LLVector3;
class LLVector4;
class LLVector3d;
class LLQuaternion;
class LLSD;
class LLUUID;
class LLMessageSystem;

// message data pieces are used to collect the data called for by the message template

// iterator typedefs precede each class as needed
typedef enum e_message_variable_type
{
	MVT_NULL,
	MVT_FIXED,
	MVT_VARIABLE,
	MVT_U8,
	MVT_U16,
	MVT_U32,
	MVT_U64,
	MVT_S8,
	MVT_S16,
	MVT_S32,
	MVT_S64,
	MVT_F32,
	MVT_F64,
	MVT_LLVector3,
	MVT_LLVector3d,
	MVT_LLVector4,
	MVT_LLQuaternion,
	MVT_LLUUID,	
	MVT_BOOL,
	MVT_IP_ADDR,
	MVT_IP_PORT,
	MVT_U16Vec3,
	MVT_U16Quat,
	MVT_S16Array,
	MVT_EOL
} EMsgVariableType;

// message system exceptional condition handlers.
enum EMessageException
{
	MX_UNREGISTERED_MESSAGE, // message number not part of template
	MX_PACKET_TOO_SHORT, // invalid packet, shorter than minimum packet size
	MX_RAN_OFF_END_OF_PACKET, // ran off the end of the packet during decode
	MX_WROTE_PAST_BUFFER_SIZE // wrote past buffer size in zero code expand
};
typedef void (*msg_exception_callback)(LLMessageSystem*,void*,EMessageException);


class LLMsgData;
class LLMsgBlkData;
class LLMessageTemplate;

class LLMessagePollInfo;

class LLMessageSystem
{
	LOG_CLASS(LLMessageSystem);
	
public:
	U8										mSendBuffer[MAX_BUFFER_SIZE];
	// Encoded send buffer needs to be slightly larger since the zero
	// coding can potentially increase the size of the send data.
	U8										mEncodedSendBuffer[2 * MAX_BUFFER_SIZE];
	S32										mSendSize;
	S32										mCurrentSendTotal;

	LLPacketRing							mPacketRing;
	LLReliablePacketParams					mReliablePacketParams;

	//LLLinkedList<LLPacketAck>				mAckList;

	// Set this flag to TRUE when you want *very* verbose logs.
	BOOL mVerboseLog;

	U32                                     mMessageFileChecksum;	
	F32                                     mMessageFileVersionNumber;

	typedef std::map<const char *, LLMessageTemplate*> message_template_name_map_t;
	typedef std::map<U32, LLMessageTemplate*> message_template_number_map_t;

private:
	message_template_name_map_t				mMessageTemplates;
	message_template_number_map_t			mMessageNumbers;

public:
	S32										mSystemVersionMajor;
	S32										mSystemVersionMinor;
	S32										mSystemVersionPatch;
	S32										mSystemVersionServer;
	U32										mVersionFlags;


	BOOL									mbProtected;

	U32										mNumberHighFreqMessages;
	U32										mNumberMediumFreqMessages;
	U32										mNumberLowFreqMessages;
	S32										mPort;
	S32										mSocket;

	U32										mPacketsIn;			// total packets in, including compressed and uncompressed
	U32										mPacketsOut;		// total packets out, including compressed and uncompressed

	U64										mBytesIn;			// total bytes in, including compressed and uncompressed
	U64										mBytesOut;			// total bytes out, including compressed and uncompressed
	
	U32										mCompressedPacketsIn;		// total compressed packets in
	U32										mCompressedPacketsOut;	    // total compressed packets out

	U32										mReliablePacketsIn;		    // total reliable packets in
	U32										mReliablePacketsOut;	    // total reliable packets out

	U32                                     mDroppedPackets;            // total dropped packets in
	U32                                     mResentPackets;             // total resent packets out
	U32                                     mFailedResendPackets;       // total resend failure packets out
	U32                                     mOffCircuitPackets;         // total # of off-circuit packets rejected
	U32                                     mInvalidOnCircuitPackets;   // total # of on-circuit but invalid packets rejected

	S64										mUncompressedBytesIn;		// total uncompressed size of compressed packets in
	S64										mUncompressedBytesOut;		// total uncompressed size of compressed packets out
	S64										mCompressedBytesIn;			// total compressed size of compressed packets in
	S64										mCompressedBytesOut;		// total compressed size of compressed packets out
	S64										mTotalBytesIn;			    // total size of all uncompressed packets in
	S64										mTotalBytesOut;				// total size of all uncompressed packets out

	BOOL                                    mSendReliable;              // does the outgoing message require a pos ack?

	LLCircuit								mCircuitInfo;
	F64										mCircuitPrintTime;			// used to print circuit debug info every couple minutes
	F32										mCircuitPrintFreq;			// seconds

	std::map<U64, U32>						mIPPortToCircuitCode;
	std::map<U32, U64>						mCircuitCodeToIPPort;
	U32										mOurCircuitCode;
	S32										mSendPacketFailureCount;
	S32										mUnackedListDepth;
	S32										mUnackedListSize;
	S32										mDSMaxListDepth;

public:
	// Read file and build message templates
	LLMessageSystem(const char *filename, U32 port, S32 version_major,
		S32 version_minor, S32 version_patch);

public:
	// Subclass use.
	LLMessageSystem();

public:
	virtual ~LLMessageSystem();

	BOOL isOK() const { return !mbError; }
	S32 getErrorCode() const { return mErrorCode; }

	// Read file and build message templates filename must point to a
	// valid string which specifies the path of a valid linden
	// template.
	void loadTemplateFile(const char* filename);


	// methods for building, sending, receiving, and handling messages
	void	setHandlerFuncFast(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL);
	void	setHandlerFunc(const char *name, void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data = NULL)
	{
		setHandlerFuncFast(gMessageStringTable.getString(name), handler_func, user_data);
	}

	bool callHandler(const char *name, bool trustedSource,
							LLMessageSystem* msg);

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

	// This method returns true if the code is in the circuit codes map.
	BOOL isCircuitCodeKnown(U32 code) const;

	// usually called in response to an AddCircuitCode message, but
	// may also be called by the login process.
	bool addCircuitCode(U32 code, const LLUUID& session_id);

	BOOL	poll(F32 seconds); // Number of seconds that we want to block waiting for data, returns if data was received
	BOOL	checkMessages( S64 frame_count = 0 );
	void	processAcks();

	BOOL	isMessageFast(const char *msg);
	BOOL	isMessage(const char *msg)
	{
		return isMessageFast(gMessageStringTable.getString(msg));
	}

	void dumpPacketToLog();

	char	*getMessageName();

	const LLHost& getSender() const;
	U32		getSenderIP() const;			// getSender() is preferred
	U32		getSenderPort() const;		// getSender() is preferred

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

	virtual void newMessageFast(const char *name);
	void	newMessage(const char *name)
	{
		newMessageFast(gMessageStringTable.getString(name));
	}

	void	copyMessageRtoS();
	void	clearMessage();

	virtual void nextBlockFast(const char *blockname);
	void	nextBlock(const char *blockname)
	{
		nextBlockFast(gMessageStringTable.getString(blockname));
	}
private:
	void	addDataFast(const char *varname, const void *data, EMsgVariableType type, S32 size);	// Use only for types not in system already
	void	addData(const char *varname, const void *data, EMsgVariableType type, S32 size)
	{
		addDataFast(gMessageStringTable.getString(varname), data, type, size);
	}

	
	void	addDataFast(const char *varname, const void *data, EMsgVariableType type);				// DEPRECATED - not typed, doesn't check storage space
	void	addData(const char *varname, const void *data, EMsgVariableType type)
	{
		addDataFast(gMessageStringTable.getString(varname), data, type);
	}
public:
	void	addBinaryDataFast(const char *varname, const void *data, S32 size)
	{
		addDataFast(varname, data, MVT_FIXED, size);
	}
	void	addBinaryData(const char *varname, const void *data, S32 size)
	{
		addDataFast(gMessageStringTable.getString(varname), data, MVT_FIXED, size);
	}

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
	virtual void addU32Fast(	const char *varname, U32 u);						// typed, checks storage space
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
	virtual void addUUIDFast( const char *varname, const LLUUID& uuid);			// typed, checks storage space
	void	addUUID( const char *varname, const LLUUID& uuid);			// typed, checks storage space
	void	addIPAddrFast( const char *varname, const U32 ip);			// typed, checks storage space
	void	addIPAddr( const char *varname, const U32 ip);			// typed, checks storage space
	void	addIPPortFast( const char *varname, const U16 port);			// typed, checks storage space
	void	addIPPort( const char *varname, const U16 port);			// typed, checks storage space
	void	addStringFast( const char* varname, const char* s);				// typed, checks storage space
	void	addString( const char* varname, const char* s);				// typed, checks storage space
	void	addStringFast( const char* varname, const std::string& s);				// typed, checks storage space
	void	addString( const char* varname, const std::string& s);				// typed, checks storage space

	TPACKETID getCurrentRecvPacketID() { return mCurrentRecvPacketID; }
	S32 getCurrentSendTotal() const { return mCurrentSendTotal; }

	// This method checks for current send total and returns true if
	// you need to go to the next block type or need to start a new
	// message. Specify the current blockname to check block counts,
	// otherwise the method only checks against MTU.
	BOOL isSendFull(const char* blockname = NULL);
	BOOL isSendFullFast(const char* blockname = NULL);

	BOOL	removeLastBlock();

	void	buildMessage();

	S32     zeroCode(U8 **data, S32 *data_size);
	S32		zeroCodeExpand(U8 **data, S32 *data_size);
	S32		zeroCodeAdjustCurrentSendTotal();

	// Uses ping-based retry
	virtual S32 sendReliable(const LLHost &host);

	// Uses ping-based retry
	S32	sendReliable(const U32 circuit)			{ return sendReliable(findHost(circuit)); }

	// Use this one if you DON'T want automatic ping-based retry.
	S32	sendReliable(	const LLHost &host, 
							S32 retries, 
							BOOL ping_based_retries,
							F32 timeout, 
							void (*callback)(void **,S32), 
							void ** callback_data);

	S32 sendSemiReliable(	const LLHost &host, 
							void (*callback)(void **,S32), void ** callback_data);

	// flush sends a message only if data's been pushed on it.
	S32		flushSemiReliable(	const LLHost &host, 
								void (*callback)(void **,S32), void ** callback_data);

	S32		flushReliable(	const LLHost &host );

	void    forwardMessage(const LLHost &host);
	void    forwardReliable(const LLHost &host);
	void    forwardReliable(const U32 circuit_code);

	S32		sendMessage(const LLHost &host);
	S32		sendMessage(const U32 circuit);

	BOOL	decodeData(const U8 *buffer, const LLHost &host);

	// TODO: Consolide these functions
	// TODO: Make these private, force use of typed functions.
	// If size is not 0, an error is generated if size doesn't exactly match the size of the data.
	// At all times, the number if bytes written to *datap is <= max_size.
private:
	void	getDataFast(const char *blockname, const char *varname, void *datap, S32 size = 0, S32 blocknum = 0, S32 max_size = S32_MAX);
	void	getData(const char *blockname, const char *varname, void *datap, S32 size = 0, S32 blocknum = 0, S32 max_size = S32_MAX)
	{
		getDataFast(gMessageStringTable.getString(blockname), gMessageStringTable.getString(varname), datap, size, blocknum, max_size);
	}
public:
	void	getBinaryDataFast(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX)
	{
		getDataFast(blockname, varname, datap, size, blocknum, max_size);
	}
	void	getBinaryData(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum = 0, S32 max_size = S32_MAX)
	{
		getDataFast(gMessageStringTable.getString(blockname), gMessageStringTable.getString(varname), datap, size, blocknum, max_size);
	}

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
	virtual void getU32Fast(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	void	getU32(		const char *block, const char *var, U32 &data, S32 blocknum = 0);
	virtual void getU64Fast(		const char *block, const char *var, U64 &data, S32 blocknum = 0);
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
	virtual void getUUIDFast(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	void	getUUID(	const char *block, const char *var, LLUUID &uuid, S32 blocknum = 0);
	virtual void getIPAddrFast(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	void	getIPAddr(	const char *block, const char *var, U32 &ip, S32 blocknum = 0);
	virtual void getIPPortFast(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	void	getIPPort(	const char *block, const char *var, U16 &port, S32 blocknum = 0);
	virtual void getStringFast(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);
	void	getString(	const char *block, const char *var, S32 buffer_size, char *buffer, S32 blocknum = 0);


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
	LLString getCircuitInfoString();

	virtual U32 getOurCircuitCode();
	
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

	S32		getNumberOfBlocksFast(const char *blockname);
	S32		getNumberOfBlocks(const char *blockname)
	{
		return getNumberOfBlocksFast(gMessageStringTable.getString(blockname));
	}
	S32		getSizeFast(const char *blockname, const char *varname);
	S32		getSize(const char *blockname, const char *varname)
	{
		return getSizeFast(gMessageStringTable.getString(blockname), gMessageStringTable.getString(varname));
	}
	S32		getSizeFast(const char *blockname, S32 blocknum, const char *varname);		// size in bytes of variable length data
	S32		getSize(const char *blockname, S32 blocknum, const char *varname)
	{
		return getSizeFast(gMessageStringTable.getString(blockname), blocknum, gMessageStringTable.getString(varname));
	}

	void	resetReceiveCounts();				// resets receive counts for all message types to 0
	void	dumpReceiveCounts();				// dumps receive count for each message type to llinfos
	void	dumpCircuitInfo();					// Circuit information to llinfos

	BOOL	isClear() const;					// returns mbSClear;
	S32 	flush(const LLHost &host);

	U32		getListenPort( void ) const;

	void startLogging();					// start verbose  logging
	void stopLogging();						// flush and close file
	void summarizeLogs(std::ostream& str);	// log statistics

	S32		getReceiveSize() const				{ return mReceiveSize; }
	S32		getReceiveCompressedSize() const	{ return mIncomingCompressedSize; }
	S32		getReceiveBytes() const;

	S32		getUnackedListSize() const			{ return mUnackedListSize; }

	const char* getCurrentSMessageName() const { return mCurrentSMessageName; }
	const char* getCurrentSBlockName() const { return mCurrentSBlockName; }

	// friends
	friend std::ostream&	operator<<(std::ostream& s, LLMessageSystem &msg);

	void setMaxMessageTime(const F32 seconds);	// Max time to process messages before warning and dumping (neg to disable)
	void setMaxMessageCounts(const S32 num);	// Max number of messages before dumping (neg to disable)
	
	// statics
public:
	static U64 getMessageTimeUsecs(const BOOL update = FALSE);	// Get the current message system time in microseconds
	static F64 getMessageTimeSeconds(const BOOL update = FALSE); // Get the current message system time in seconds

	static void setTimeDecodes( BOOL b )		
		{ LLMessageSystem::mTimeDecodes = b; }

	static void setTimeDecodesSpamThreshold( F32 seconds ) 
		{ LLMessageSystem::mTimeDecodesSpamThreshold = seconds; }

	// message handlers internal to the message systesm
	//static void processAssignCircuitCode(LLMessageSystem* msg, void**);
	static void processAddCircuitCode(LLMessageSystem* msg, void**);
	static void processUseCircuitCode(LLMessageSystem* msg, void**);

	void setMessageBans(const LLSD& trusted, const LLSD& untrusted);
	
private:
	// data used in those internal handlers

	// The mCircuitCodes is a map from circuit codes to session
	// ids. This allows us to verify sessions on connect.
	typedef std::map<U32, LLUUID> code_session_map_t;
	code_session_map_t mCircuitCodes;

	// Viewers need to track a process session in order to make sure
	// that no one gives them a bad circuit code.
	LLUUID mSessionID;

private:
	void	addTemplate(LLMessageTemplate *templatep);
	void		clearReceiveState();
	BOOL		decodeTemplate( const U8* buffer, S32 buffer_size, LLMessageTemplate** msg_template );

	void		logMsgFromInvalidCircuit( const LLHost& sender, BOOL recv_reliable );
	void		logTrustedMsgFromUntrustedCircuit( const LLHost& sender );
	void		logValidMsg(LLCircuitData *cdp, const LLHost& sender, BOOL recv_reliable, BOOL recv_resent, BOOL recv_acks );
	void		logRanOffEndOfPacket( const LLHost& sender );

private:
	class LLMessageCountInfo
	{
	public:
		U32 mMessageNum;
		U32 mMessageBytes;
		BOOL mInvalid;
	};

	LLMessagePollInfo						*mPollInfop;

	U8										mEncodedRecvBuffer[MAX_BUFFER_SIZE];
	U8										mTrueReceiveBuffer[MAX_BUFFER_SIZE];
	S32										mTrueReceiveSize;

	// Must be valid during decode
	S32										mReceiveSize;
	TPACKETID                               mCurrentRecvPacketID;       // packet ID of current receive packet (for reporting)
	LLMessageTemplate						*mCurrentRMessageTemplate;
	LLMsgData								*mCurrentRMessageData;
	S32									    mIncomingCompressedSize;		// original size of compressed msg (0 if uncomp.)
	LLHost									mLastSender;

	// send message storage
	LLMsgData								*mCurrentSMessageData;
	LLMessageTemplate						*mCurrentSMessageTemplate;
	LLMsgBlkData							*mCurrentSDataBlock;
	char									*mCurrentSMessageName;
	char									*mCurrentSBlockName;

	BOOL									mbError;
	S32 mErrorCode;

	BOOL									mbSBuilt;	// is send message built?
	BOOL									mbSClear;	// is the send message clear?

	F64										mResendDumpTime; // The last time we dumped resends

	LLMessageCountInfo mMessageCountList[MAX_MESSAGE_COUNT_NUM];
	S32 mNumMessageCounts;
	F32 mReceiveTime;
	F32 mMaxMessageTime; // Max number of seconds for processing messages
	S32 mMaxMessageCounts; // Max number of messages to process before dumping.
	F64 mMessageCountTime;

	F64 mCurrentMessageTimeSeconds; // The current "message system time" (updated the first call to checkMessages after a resetReceiveCount

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
};


// external hook into messaging system
extern LLMessageSystem	*gMessageSystem;

// Must specific overall system version, which is used to determine
// if a patch is available in the message template checksum verification.
// Return TRUE if able to initialize system.
BOOL start_messaging_system(
	const std::string& template_name,
	U32 port, 
	S32 version_major,
	S32 version_minor,
	S32 version_patch,
	BOOL b_dump_prehash_file,
	const std::string& secret);

void end_messaging_system();

void null_message_callback(LLMessageSystem *msg, void **data);

//
// Inlines
//

static inline void *htonmemcpy(void *vs, const void *vct, EMsgVariableType type, size_t n)
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
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
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
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
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
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
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
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 8, ct + 8, MVT_F32, 4);
		htonmemcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htonmemcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_LLVector3d:
		if (n != 24)
		{
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 16, ct + 16, MVT_F64, 8);
		htonmemcpy(s + 8, ct + 8, MVT_F64, 8);
		return(htonmemcpy(s, ct, MVT_F64, 8));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_LLVector4:
		if (n != 16)
		{
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 12, ct + 12, MVT_F32, 4);
		htonmemcpy(s + 8, ct + 8, MVT_F32, 4);
		htonmemcpy(s + 4, ct + 4, MVT_F32, 4);
		return(htonmemcpy(s, ct, MVT_F32, 4));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U16Vec3:
		if (n != 6)
		{
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 4, ct + 4, MVT_U16, 2);
		htonmemcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htonmemcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_U16Quat:
		if (n != 8)
		{
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		htonmemcpy(s + 6, ct + 6, MVT_U16, 2);
		htonmemcpy(s + 4, ct + 4, MVT_U16, 2);
		htonmemcpy(s + 2, ct + 2, MVT_U16, 2);
		return(htonmemcpy(s, ct, MVT_U16, 2));
#else
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
#endif

	case MVT_S16Array:
		if (n % 2)
		{
			llerrs << "Size argument passed to htonmemcpy doesn't match swizzle type size" << llendl;
		}
#ifdef LL_BIG_ENDIAN
		length = n % 2;
		for (i = 1; i < length; i++)
		{
			htonmemcpy(s + i*2, ct + i*2, MVT_S16, 2);
		}
		return(htonmemcpy(s, ct, MVT_S16, 2));
#else
		return(memcpy(s,ct,n));
#endif

	default:
		return(memcpy(s,ct,n));	/* Flawfinder: ignore */ 
	}
}

inline void *ntohmemcpy(void *s, const void *ct, EMsgVariableType type, size_t n)
{
	return(htonmemcpy(s,ct,type, n));
}


inline const LLHost& LLMessageSystem::getSender() const
{
	return mLastSender;
}

inline U32 LLMessageSystem::getSenderIP() const
{
	return mLastSender.getAddress();
}

inline U32 LLMessageSystem::getSenderPort() const
{
	return mLastSender.getPort();
}

inline void LLMessageSystem::addS8Fast(const char *varname, S8 s)
{
	addDataFast(varname, &s, MVT_S8, sizeof(s));
}

inline void LLMessageSystem::addS8(const char *varname, S8 s)
{
	addDataFast(gMessageStringTable.getString(varname), &s, MVT_S8, sizeof(s));
}

inline void LLMessageSystem::addU8Fast(const char *varname, U8 u)
{
	addDataFast(varname, &u, MVT_U8, sizeof(u));
}

inline void LLMessageSystem::addU8(const char *varname, U8 u)
{
	addDataFast(gMessageStringTable.getString(varname), &u, MVT_U8, sizeof(u));
}

inline void LLMessageSystem::addS16Fast(const char *varname, S16 i)
{
	addDataFast(varname, &i, MVT_S16, sizeof(i));
}

inline void LLMessageSystem::addS16(const char *varname, S16 i)
{
	addDataFast(gMessageStringTable.getString(varname), &i, MVT_S16, sizeof(i));
}

inline void LLMessageSystem::addU16Fast(const char *varname, U16 i)
{
	addDataFast(varname, &i, MVT_U16, sizeof(i));
}

inline void LLMessageSystem::addU16(const char *varname, U16 i)
{
	addDataFast(gMessageStringTable.getString(varname), &i, MVT_U16, sizeof(i));
}

inline void LLMessageSystem::addF32Fast(const char *varname, F32 f)
{
	addDataFast(varname, &f, MVT_F32, sizeof(f));
}

inline void LLMessageSystem::addF32(const char *varname, F32 f)
{
	addDataFast(gMessageStringTable.getString(varname), &f, MVT_F32, sizeof(f));
}

inline void LLMessageSystem::addS32Fast(const char *varname, S32 s)
{
	addDataFast(varname, &s, MVT_S32, sizeof(s));
}

inline void LLMessageSystem::addS32(const char *varname, S32 s)
{
	addDataFast(gMessageStringTable.getString(varname), &s, MVT_S32, sizeof(s));
}

inline void LLMessageSystem::addU32Fast(const char *varname, U32 u)
{
	addDataFast(varname, &u, MVT_U32, sizeof(u));
}

inline void LLMessageSystem::addU32(const char *varname, U32 u)
{
	addDataFast(gMessageStringTable.getString(varname), &u, MVT_U32, sizeof(u));
}

inline void LLMessageSystem::addU64Fast(const char *varname, U64 lu)
{
	addDataFast(varname, &lu, MVT_U64, sizeof(lu));
}

inline void LLMessageSystem::addU64(const char *varname, U64 lu)
{
	addDataFast(gMessageStringTable.getString(varname), &lu, MVT_U64, sizeof(lu));
}

inline void LLMessageSystem::addF64Fast(const char *varname, F64 d)
{
	addDataFast(varname, &d, MVT_F64, sizeof(d));
}

inline void LLMessageSystem::addF64(const char *varname, F64 d)
{
	addDataFast(gMessageStringTable.getString(varname), &d, MVT_F64, sizeof(d));
}

inline void LLMessageSystem::addIPAddrFast(const char *varname, U32 u)
{
	addDataFast(varname, &u, MVT_IP_ADDR, sizeof(u));
}

inline void LLMessageSystem::addIPAddr(const char *varname, U32 u)
{
	addDataFast(gMessageStringTable.getString(varname), &u, MVT_IP_ADDR, sizeof(u));
}

inline void LLMessageSystem::addIPPortFast(const char *varname, U16 u)
{
	u = htons(u);
	addDataFast(varname, &u, MVT_IP_PORT, sizeof(u));
}

inline void LLMessageSystem::addIPPort(const char *varname, U16 u)
{
	u = htons(u);
	addDataFast(gMessageStringTable.getString(varname), &u, MVT_IP_PORT, sizeof(u));
}

inline void LLMessageSystem::addBOOLFast(const char* varname, BOOL b)
{
	// Can't just cast a BOOL (actually a U32) to a U8.
	// In some cases the low order bits will be zero.
	U8 temp = (b != 0);
	addDataFast(varname, &temp, MVT_BOOL, sizeof(temp));
}

inline void LLMessageSystem::addBOOL(const char* varname, BOOL b)
{
	// Can't just cast a BOOL (actually a U32) to a U8.
	// In some cases the low order bits will be zero.
	U8 temp = (b != 0);
	addDataFast(gMessageStringTable.getString(varname), &temp, MVT_BOOL, sizeof(temp));
}

inline void LLMessageSystem::addStringFast(const char* varname, const char* s)
{
	if (s)
		addDataFast( varname, (void *)s, MVT_VARIABLE, (S32)strlen(s) + 1);  /* Flawfinder: ignore */  
	else
		addDataFast( varname, NULL, MVT_VARIABLE, 0); 
}

inline void LLMessageSystem::addString(const char* varname, const char* s)
{
	if (s)
		addDataFast( gMessageStringTable.getString(varname), (void *)s, MVT_VARIABLE, (S32)strlen(s) + 1);  /* Flawfinder: ignore */ 
	else
		addDataFast( gMessageStringTable.getString(varname), NULL, MVT_VARIABLE, 0); 
}

inline void LLMessageSystem::addStringFast(const char* varname, const std::string& s)
{
	if (s.size())
		addDataFast( varname, (void *)s.c_str(), MVT_VARIABLE, (S32)(s.size()) + 1); 
	else
		addDataFast( varname, NULL, MVT_VARIABLE, 0); 
}

inline void LLMessageSystem::addString(const char* varname, const std::string& s)
{
	if (s.size())
		addDataFast( gMessageStringTable.getString(varname), (void *)s.c_str(), MVT_VARIABLE, (S32)(s.size()) + 1); 
	else
		addDataFast( gMessageStringTable.getString(varname), NULL, MVT_VARIABLE, 0); 
}


//-----------------------------------------------------------------------------
// Retrieval aliases
//-----------------------------------------------------------------------------
inline void LLMessageSystem::getS8Fast(const char *block, const char *var, S8 &u, S32 blocknum)
{
	getDataFast(block, var, &u, sizeof(S8), blocknum);
}

inline void LLMessageSystem::getS8(const char *block, const char *var, S8 &u, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &u, sizeof(S8), blocknum);
}

inline void LLMessageSystem::getU8Fast(const char *block, const char *var, U8 &u, S32 blocknum)
{
	getDataFast(block, var, &u, sizeof(U8), blocknum);
}

inline void LLMessageSystem::getU8(const char *block, const char *var, U8 &u, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &u, sizeof(U8), blocknum);
}

inline void LLMessageSystem::getBOOLFast(const char *block, const char *var, BOOL &b, S32 blocknum )
{
	U8 value;
	getDataFast(block, var, &value, sizeof(U8), blocknum);
	b = (BOOL) value;
}

inline void LLMessageSystem::getBOOL(const char *block, const char *var, BOOL &b, S32 blocknum )
{
	U8 value;
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &value, sizeof(U8), blocknum);
	b = (BOOL) value;
}

inline void LLMessageSystem::getS16Fast(const char *block, const char *var, S16 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(S16), blocknum);
}

inline void LLMessageSystem::getS16(const char *block, const char *var, S16 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(S16), blocknum);
}

inline void LLMessageSystem::getU16Fast(const char *block, const char *var, U16 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(U16), blocknum);
}

inline void LLMessageSystem::getU16(const char *block, const char *var, U16 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(U16), blocknum);
}

inline void LLMessageSystem::getS32Fast(const char *block, const char *var, S32 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(S32), blocknum);
}

inline void LLMessageSystem::getS32(const char *block, const char *var, S32 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(S32), blocknum);
}

inline void LLMessageSystem::getU32Fast(const char *block, const char *var, U32 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(U32), blocknum);
}

inline void LLMessageSystem::getU32(const char *block, const char *var, U32 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(U32), blocknum);
}

inline void LLMessageSystem::getU64Fast(const char *block, const char *var, U64 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(U64), blocknum);
}

inline void LLMessageSystem::getU64(const char *block, const char *var, U64 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(U64), blocknum);
}


inline void LLMessageSystem::getIPAddrFast(const char *block, const char *var, U32 &u, S32 blocknum)
{
	getDataFast(block, var, &u, sizeof(U32), blocknum);
}

inline void LLMessageSystem::getIPAddr(const char *block, const char *var, U32 &u, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &u, sizeof(U32), blocknum);
}

inline void LLMessageSystem::getIPPortFast(const char *block, const char *var, U16 &u, S32 blocknum)
{
	getDataFast(block, var, &u, sizeof(U16), blocknum);
	u = ntohs(u);
}

inline void LLMessageSystem::getIPPort(const char *block, const char *var, U16 &u, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &u, sizeof(U16), blocknum);
	u = ntohs(u);
}


inline void LLMessageSystem::getStringFast(const char *block, const char *var, S32 buffer_size, char *s, S32 blocknum )
{
	s[0] = '\0';
	getDataFast(block, var, s, 0, blocknum, buffer_size);
	s[buffer_size - 1] = '\0';
}

inline void LLMessageSystem::getString(const char *block, const char *var, S32 buffer_size, char *s, S32 blocknum )
{
	s[0] = '\0';
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), s, 0, blocknum, buffer_size);
	s[buffer_size - 1] = '\0';
}

inline S32 LLMessageSystem::sendMessage(const U32 circuit)
{
	return sendMessage(findHost(circuit));
}

#endif
