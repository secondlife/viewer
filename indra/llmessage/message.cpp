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
#include "lldir.h"
#include "llerror.h"
#include "llfasttimer.h"
#include "llmd5.h"
#include "llsd.h"
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

class LLMsgVarData
{
public:
	LLMsgVarData() : mName(NULL), mSize(-1), mDataSize(-1), mData(NULL), mType(MVT_U8)
	{
	}

	LLMsgVarData(const char *name, EMsgVariableType type) : mSize(-1), mDataSize(-1), mData(NULL), mType(type)
	{
		mName = (char *)name; 
	}

	~LLMsgVarData() 
	{
		// copy constructor just copies the mData pointer, so only delete mData explicitly
	}
	
	void deleteData() 
	{
		delete[] mData;
		mData = NULL;
	}
	
	void addData(const void *indata, S32 size, EMsgVariableType type, S32 data_size = -1);

	char *getName() const	{ return mName; }
	S32 getSize() const		{ return mSize; }
	void *getData()			{ return (void*)mData; }
	S32 getDataSize() const	{ return mDataSize; }
	EMsgVariableType getType() const	{ return mType; }

protected:
	char				*mName;
	S32					mSize;
	S32					mDataSize;

	U8					*mData;
	EMsgVariableType	mType;
};


class LLMsgBlkData
{
public:
	LLMsgBlkData(const char *name, S32 blocknum) : mOffset(-1), mBlockNumber(blocknum), mTotalSize(-1) 
	{ 
		mName = (char *)name; 
	}

	~LLMsgBlkData()
	{
		for (msg_var_data_map_t::iterator iter = mMemberVarData.begin();
			 iter != mMemberVarData.end(); iter++)
		{
			iter->deleteData();
		}
	}

	void addVariable(const char *name, EMsgVariableType type)
	{
		LLMsgVarData tmp(name,type);
		mMemberVarData[name] = tmp;
	}

	void addData(char *name, const void *data, S32 size, EMsgVariableType type, S32 data_size = -1)
	{
		LLMsgVarData* temp = &mMemberVarData[name]; // creates a new entry if one doesn't exist
		temp->addData(data, size, type, data_size);
	}

	S32									mOffset;
	S32									mBlockNumber;
	typedef LLDynamicArrayIndexed<LLMsgVarData, const char *, 8> msg_var_data_map_t;
	msg_var_data_map_t					mMemberVarData;
	char								*mName;
	S32									mTotalSize;
};


class LLMsgData
{
public:
	LLMsgData(const char *name) : mTotalSize(-1) 
	{ 
		mName = (char *)name; 
	}
	~LLMsgData()
	{
		for_each(mMemberBlocks.begin(), mMemberBlocks.end(), DeletePairedPointer());
	}

	void addBlock(LLMsgBlkData *blockp)
	{
		mMemberBlocks[blockp->mName] = blockp;
	}

	void addDataFast(char *blockname, char *varname, const void *data, S32 size, EMsgVariableType type, S32 data_size = -1);

public:
	S32									mOffset;
	typedef std::map<char*, LLMsgBlkData*> msg_blk_data_map_t;
	msg_blk_data_map_t					mMemberBlocks;
	char								*mName;
	S32									mTotalSize;
};

inline void LLMsgVarData::addData(const void *data, S32 size, EMsgVariableType type, S32 data_size)
{
	mSize = size;
	mDataSize = data_size;
	if ( (type != MVT_VARIABLE) && (type != MVT_FIXED) 
		 && (mType != MVT_VARIABLE) && (mType != MVT_FIXED))
	{
		if (mType != type)
		{
			llwarns << "Type mismatch in addData for " << mName
					<< " message: " << gMessageSystem->getCurrentSMessageName()
					<< " block: " << gMessageSystem->getCurrentSBlockName()
					<< llendl;
		}
	}
	if(size)
	{
		delete mData; // Delete it if it already exists
		mData = new U8[size];
		htonmemcpy(mData, data, mType, size);
	}
}



inline void LLMsgData::addDataFast(char *blockname, char *varname, const void *data, S32 size, EMsgVariableType type, S32 data_size)
{
	// remember that if the blocknumber is > 0 then the number is appended to the name
	char *namep = (char *)blockname;
	LLMsgBlkData* block_data = mMemberBlocks[namep];
	if (block_data->mBlockNumber)
	{
		namep += block_data->mBlockNumber;
		block_data->addData(varname, data, size, type, data_size);
	}
	else
	{
		block_data->addData(varname, data, size, type, data_size);
	}
}

// LLMessage* classes store the template of messages


class LLMessageVariable
{
public:
	LLMessageVariable() : mName(NULL), mType(MVT_NULL), mSize(-1)
	{
	}

	LLMessageVariable(char *name) : mType(MVT_NULL), mSize(-1)
	{
		mName = name;
	}

	LLMessageVariable(char *name, const EMsgVariableType type, const S32 size) : mType(type), mSize(size) 
	{
		mName = gMessageStringTable.getString(name); 
	}
	
	~LLMessageVariable() {}

	friend std::ostream&	 operator<<(std::ostream& s, LLMessageVariable &msg);

	EMsgVariableType getType() const				{ return mType; }
	S32	getSize() const								{ return mSize; }
	char *getName() const							{ return mName; }
protected:
	char				*mName;
	EMsgVariableType	mType;
	S32					mSize;
};


typedef enum e_message_block_type
{
	MBT_NULL,
	MBT_SINGLE,
	MBT_MULTIPLE,
	MBT_VARIABLE,
	MBT_EOF
} EMsgBlockType;

class LLMessageBlock
{
public:
	LLMessageBlock(char *name, EMsgBlockType type, S32 number = 1) : mType(type), mNumber(number), mTotalSize(0) 
	{ 
		mName = gMessageStringTable.getString(name);
	}

	~LLMessageBlock()
	{
		for_each(mMemberVariables.begin(), mMemberVariables.end(), DeletePairedPointer());
	}

	void addVariable(char *name, const EMsgVariableType type, const S32 size)
	{
		LLMessageVariable** varp = &mMemberVariables[name];
		if (*varp != NULL)
		{
			llerrs << name << " has already been used as a variable name!" << llendl;
		}
		*varp = new LLMessageVariable(name, type, size);
		if (((*varp)->getType() != MVT_VARIABLE)
			&&(mTotalSize != -1))
		{
			mTotalSize += (*varp)->getSize();
		}
		else
		{
			mTotalSize = -1;
		}
	}

	EMsgVariableType getVariableType(char *name)
	{
		return (mMemberVariables[name])->getType();
	}

	S32 getVariableSize(char *name)
	{
		return (mMemberVariables[name])->getSize();
	}

	friend std::ostream&	 operator<<(std::ostream& s, LLMessageBlock &msg);

	typedef std::map<const char *, LLMessageVariable*> message_variable_map_t;
	message_variable_map_t 					mMemberVariables;
	char									*mName;
	EMsgBlockType							mType;
	S32										mNumber;
	S32										mTotalSize;
};


enum EMsgFrequency
{
	MFT_NULL	= 0,  // value is size of message number in bytes
	MFT_HIGH	= 1,
	MFT_MEDIUM	= 2,
	MFT_LOW		= 4
};

typedef enum e_message_trust
{
	MT_TRUST,
	MT_NOTRUST
} EMsgTrust;

enum EMsgEncoding
{
	ME_UNENCODED,
	ME_ZEROCODED
};

class LLMessageTemplate
{
public:
	LLMessageTemplate(const char *name, U32 message_number, EMsgFrequency freq)
		: 
		//mMemberBlocks(),
		mName(NULL),
		mFrequency(freq),
		mTrust(MT_NOTRUST),
		mEncoding(ME_ZEROCODED),
		mMessageNumber(message_number), 
		mTotalSize(0), 
		mReceiveCount(0),
		mReceiveBytes(0),
		mReceiveInvalid(0),
		mDecodeTimeThisFrame(0.f),
		mTotalDecoded(0),
		mTotalDecodeTime(0.f),
		mMaxDecodeTimePerMsg(0.f),
		mBanFromTrusted(false),
		mBanFromUntrusted(false),
		mHandlerFunc(NULL), 
		mUserData(NULL)
	{ 
		mName = gMessageStringTable.getString(name);
	}

	~LLMessageTemplate()
	{
		for_each(mMemberBlocks.begin(), mMemberBlocks.end(), DeletePairedPointer());
	}

	void addBlock(LLMessageBlock *blockp)
	{
		LLMessageBlock** member_blockp = &mMemberBlocks[blockp->mName];
		if (*member_blockp != NULL)
		{
			llerrs << "Block " << blockp->mName
				<< "has already been used as a block name!" << llendl;
		}
		*member_blockp = blockp;
		if (  (mTotalSize != -1)
			&&(blockp->mTotalSize != -1)
			&&(  (blockp->mType == MBT_SINGLE)
			   ||(blockp->mType == MBT_MULTIPLE)))
		{
			mTotalSize += blockp->mNumber*blockp->mTotalSize;
		}
		else
		{
			mTotalSize = -1;
		}
	}

	LLMessageBlock *getBlock(char *name)
	{
		return mMemberBlocks[name];
	}

	// Trusted messages can only be recieved on trusted circuits.
	void setTrust(EMsgTrust t)
	{
		mTrust = t;
	}

	EMsgTrust getTrust(void)
	{
		return mTrust;
	}

	// controls for how the message should be encoded
	void setEncoding(EMsgEncoding e)
	{
		mEncoding = e;
	}
	EMsgEncoding getEncoding()
	{
		return mEncoding;
	}

	void setHandlerFunc(void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data)
	{
		mHandlerFunc = handler_func;
		mUserData = user_data;
	}

	BOOL callHandlerFunc(LLMessageSystem *msgsystem)
	{
		if (mHandlerFunc)
		{
			mHandlerFunc(msgsystem, mUserData);
			return TRUE;
		}
		return FALSE;
	}

	bool isBanned(bool trustedSource)
	{
		return trustedSource ? mBanFromTrusted : mBanFromUntrusted;
	}

	friend std::ostream&	 operator<<(std::ostream& s, LLMessageTemplate &msg);

public:
	typedef std::map<char*, LLMessageBlock*> message_block_map_t;
	message_block_map_t						mMemberBlocks;
	char									*mName;
	EMsgFrequency							mFrequency;
	EMsgTrust								mTrust;
	EMsgEncoding							mEncoding;
	U32										mMessageNumber;
	S32										mTotalSize;
	U32										mReceiveCount;		// how many of this template have been received since last reset
	U32										mReceiveBytes;		// How many bytes received
	U32										mReceiveInvalid;	// How many "invalid" packets
	F32										mDecodeTimeThisFrame;	// Total seconds spent decoding this frame
	U32										mTotalDecoded;		// Total messages successfully decoded
	F32										mTotalDecodeTime;	// Total time successfully decoding messages
	F32										mMaxDecodeTimePerMsg;

	bool									mBanFromTrusted;
	bool									mBanFromUntrusted;

private:
	// message handler function (this is set by each application)
	void									(*mHandlerFunc)(LLMessageSystem *msgsystem, void **user_data);
	void									**mUserData;
};



// static
BOOL LLMessageSystem::mTimeDecodes = FALSE;

// static, 50ms per message decode
F32  LLMessageSystem::mTimeDecodesSpamThreshold = 0.05f;

// FIXME: This needs to be moved into a seperate file so that it never gets
// included in the viewer.  30 Sep 2002 mark
// NOTE: I don't think it's important that the messgage system tracks
// this since it must get set externally. 2004.08.25 Phoenix.
static std::string g_shared_secret;
std::string get_shared_secret();

class LLMessagePollInfo
{
public:
	apr_socket_t *mAPRSocketp;
	apr_pollfd_t mPollFD;
};


// LLMessageVariable functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageVariable &msg)
{
	s << "\t\t" << msg.mName << " (";
	switch (msg.mType)
	{
	case MVT_FIXED:
		s << "Fixed, " << msg.mSize << " bytes total)\n";
		break;
	case MVT_VARIABLE:
		s << "Variable, " << msg.mSize << " bytes of size info)\n";
		break;
	default:
		s << "Unknown\n";
		break;
	}
	return s;
}

// LLMessageBlock functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageBlock &msg)
{
	s << "\t" << msg.mName << " (";
	switch (msg.mType)
	{
	case MBT_SINGLE:
		s << "Fixed";
		break;
	case MBT_MULTIPLE:
		s << "Multiple - " << msg.mNumber << " copies";
		break;
	case MBT_VARIABLE:
		s << "Variable";
		break;
	default:
		s << "Unknown";
		break;
	}
	if (msg.mTotalSize != -1)
	{
		s << ", " << msg.mTotalSize << " bytes each, " << msg.mNumber*msg.mTotalSize << " bytes total)\n";
	}
	else
	{
		s << ")\n";
	}


	for (LLMessageBlock::message_variable_map_t::iterator iter = msg.mMemberVariables.begin();
		 iter != msg.mMemberVariables.end(); iter++)
	{
		LLMessageVariable& ci = *(iter->second);
		s << ci;
	}

	return s;
}

// LLMessageTemplate functions and friends

std::ostream& operator<<(std::ostream& s, LLMessageTemplate &msg)
{
	switch (msg.mFrequency)
	{
	case MFT_HIGH:
		s << "========================================\n" << "Message #" << msg.mMessageNumber << "\n" << msg.mName << " (";
		s << "High";
		break;
	case MFT_MEDIUM:
		s << "========================================\n" << "Message #";
		s << (msg.mMessageNumber & 0xFF) << "\n" << msg.mName << " (";
		s << "Medium";
		break;
	case MFT_LOW:
		s << "========================================\n" << "Message #";
		s << (msg.mMessageNumber & 0xFFFF) << "\n" << msg.mName << " (";
		s << "Low";
		break;
	default:
		s << "Unknown";
		break;
	}

	if (msg.mTotalSize != -1)
	{
		s << ", " << msg.mTotalSize << " bytes total)\n";
	}
	else
	{
		s << ")\n";
	}
	
	for (LLMessageTemplate::message_block_map_t::iterator iter = msg.mMemberBlocks.begin();
		 iter != msg.mMemberBlocks.end(); iter++)
	{
		LLMessageBlock* ci = iter->second;
		s << *ci;
	}

	return s;
}

// LLMessageList functions and friends

// Lets support a small subset of regular expressions here
// Syntax is a string made up of:
//	a	- checks against alphanumeric				([A-Za-z0-9])
//	c	- checks against character					([A-Za-z])
//	f	- checks against first variable character	([A-Za-z_])
//	v	- checks against variable					([A-Za-z0-9_])
//	s	- checks against sign of integer			([-0-9])
//  d	- checks against integer digit				([0-9])
//  *	- repeat last check

// checks 'a'
BOOL	b_return_alphanumeric_ok(char c)
{
	if (  (  (c < 'A')
		   ||(c > 'Z'))
		&&(  (c < 'a')
		   ||(c > 'z'))
		&&(  (c < '0')
		   ||(c > '9')))
	{
		return FALSE;
	}
	return TRUE;
}

// checks 'c'
BOOL	b_return_character_ok(char c)
{
	if (  (  (c < 'A')
		   ||(c > 'Z'))
		&&(  (c < 'a')
		   ||(c > 'z')))
	{
		return FALSE;
	}
	return TRUE;
}

// checks 'f'
BOOL	b_return_first_variable_ok(char c)
{
	if (  (  (c < 'A')
		   ||(c > 'Z'))
		&&(  (c < 'a')
		   ||(c > 'z'))
		&&(c != '_'))
	{
		return FALSE;
	}
	return TRUE;
}

// checks 'v'
BOOL	b_return_variable_ok(char c)
{
	if (  (  (c < 'A')
		   ||(c > 'Z'))
		&&(  (c < 'a')
		   ||(c > 'z'))
		&&(  (c < '0')
		   ||(c > '9'))
		&&(c != '_'))
	{
		return FALSE;
	}
	return TRUE;
}

// checks 's'
BOOL	b_return_signed_integer_ok(char c)
{
	if (  (  (c < '0')
		   ||(c > '9'))
		&&(c != '-'))
	{
		return FALSE;
	}
	return TRUE;
}

// checks 'd'
BOOL	b_return_integer_ok(char c)
{
	if (  (c < '0')
		||(c > '9'))
	{
		return FALSE;
	}
	return TRUE;
}

BOOL	(*gParseCheckCharacters[])(char c) =
{
	b_return_alphanumeric_ok,
	b_return_character_ok,
	b_return_first_variable_ok,
	b_return_variable_ok,
	b_return_signed_integer_ok,
	b_return_integer_ok
};

S32 get_checker_number(char checker)
{
	switch(checker)
	{
	case 'a':
		return 0;
	case 'c':
		return 1;
	case 'f':
		return 2;
	case 'v':
		return 3;
	case 's':
		return 4;
	case 'd':
		return 5;
	case '*':
		return 9999;
	default:
		return -1;
	}
}

// check token based on passed simplified regular expression
BOOL	b_check_token(char *token, char *regexp)
{
	S32 tptr, rptr = 0;
	S32 current_checker, next_checker = 0;

	current_checker = get_checker_number(regexp[rptr++]);

	if (current_checker == -1)
	{
		llerrs << "Invalid regular expression value!" << llendl;
		return FALSE;
	}

	if (current_checker == 9999)
	{
		llerrs << "Regular expression can't start with *!" << llendl;
		return FALSE;
	}

	for (tptr = 0; token[tptr]; tptr++)
	{
		if (current_checker == -1)
		{
			llerrs << "Input exceeds regular expression!\nDid you forget a *?" << llendl;
			return FALSE;
		}

		if (!gParseCheckCharacters[current_checker](token[tptr]))
		{
			return FALSE;
		}
		if (next_checker != 9999)
		{
			next_checker = get_checker_number(regexp[rptr++]);
			if (next_checker != 9999)
			{
				current_checker = next_checker;
			}
		}
	}
	return TRUE;
}

// C variable can be made up of upper or lower case letters, underscores, or numbers, but can't start with a number
BOOL	b_variable_ok(char *token)
{
	if (!b_check_token(token, "fv*"))
	{
		llerrs << "Token '" << token << "' isn't a variable!" << llendl;
		return FALSE;
	}
	return TRUE;
}

// An integer is made up of the digits 0-9 and may be preceded by a '-'
BOOL	b_integer_ok(char *token)
{
	if (!b_check_token(token, "sd*"))
	{
		llerrs << "Token isn't an integer!" << llendl;
		return FALSE;
	}
	return TRUE;
}

// An integer is made up of the digits 0-9
BOOL	b_positive_integer_ok(char *token)
{
	if (!b_check_token(token, "d*"))
	{
		llerrs << "Token isn't an integer!" << llendl;
		return FALSE;
	}
	return TRUE;
}

void LLMessageSystem::init()
{
	// initialize member variables
	mVerboseLog = FALSE;

	mbError = FALSE;
	mErrorCode = 0;
	mIncomingCompressedSize = 0;
	mSendReliable = FALSE;

	mbSBuilt = FALSE;
	mbSClear = TRUE;

	mUnackedListDepth = 0;
	mUnackedListSize = 0;
	mDSMaxListDepth = 0;

	mCurrentRMessageData = NULL;
	mCurrentRMessageTemplate = NULL;

	mCurrentSMessageData = NULL;
	mCurrentSMessageTemplate = NULL;
	mCurrentSMessageName = NULL;

	mCurrentRecvPacketID = 0;

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

	mMessageFileChecksum = 0;
	mMessageFileVersionNumber = 0.f;
}

LLMessageSystem::LLMessageSystem()
{
	init();

	mSystemVersionMajor = 0;
	mSystemVersionMinor = 0;
	mSystemVersionPatch = 0;
	mSystemVersionServer = 0;
	mVersionFlags = 0x0;

	// default to not accepting packets from not alive circuits
	mbProtected = TRUE;

	mSendPacketFailureCount = 0;
	mCircuitPrintFreq = 0.f;		// seconds

	// initialize various bits of net info
	mSocket = 0;
	mPort = 0;

	mPollInfop = NULL;

	mResendDumpTime = 0;
	mMessageCountTime = 0;
	mCircuitPrintTime = 0;
	mCurrentMessageTimeSeconds = 0;

	// Constants for dumping output based on message processing time/count
	mNumMessageCounts = 0;
	mMaxMessageCounts = 0; // >= 0 means dump warnings
	mMaxMessageTime   = 0.f;

	mTrueReceiveSize = 0;

	// Error if checking this state, subclass methods which aren't implemented are delegated
	// to properly constructed message system.
	mbError = TRUE;
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
	}

	char token[MAX_MESSAGE_INTERNAL_NAME_SIZE]; /* Flawfinder: ignore */ 

	// state variables
	BOOL				b_template_start = TRUE;
	BOOL				b_template_end = FALSE;
	BOOL				b_template = FALSE;
	BOOL				b_block_start = FALSE;
	BOOL				b_block_end = FALSE;
	BOOL				b_block = FALSE;
	BOOL				b_variable_start = FALSE;
	BOOL				b_variable_end = FALSE;
	BOOL				b_variable = FALSE;
	//BOOL				b_in_comment_block = FALSE;		// not yet used

	// working temp variables
	LLMessageTemplate	*templatep = NULL;
	char				template_name[MAX_MESSAGE_INTERNAL_NAME_SIZE];		/* Flawfinder: ignore */ 

	LLMessageBlock		*blockp = NULL;
	char				block_name[MAX_MESSAGE_INTERNAL_NAME_SIZE];		/* Flawfinder: ignore */ 

	LLMessageVariable	var;
	char				var_name[MAX_MESSAGE_INTERNAL_NAME_SIZE];		/* Flawfinder: ignore */ 
	char				formatString[MAX_MESSAGE_INTERNAL_NAME_SIZE];

	FILE* messagefilep = NULL;
	mMessageFileChecksum = 0;
	mMessageFileVersionNumber = 0.f;
	S32 checksum_offset = 0;
	char* checkp = NULL;

	snprintf(formatString, sizeof(formatString), "%%%ds", MAX_MESSAGE_INTERNAL_NAME_SIZE);
	messagefilep = LLFile::fopen(filename, "r");
	if (messagefilep)
	{
//		mName = gMessageStringTable.getString(filename); 

		fseek(messagefilep, 0L, SEEK_SET );
		while(fscanf(messagefilep, formatString, token) != EOF)
		{
			// skip comments
			if (token[0] == '/')
			{
				// skip to end of line
				while (token[0] != 10)
					fscanf(messagefilep, "%c", token);
				continue;
			}
	
			checkp = token;

			while (*checkp)
			{
				mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
				checksum_offset = (checksum_offset + 8) % 32;
			}

			// what are we looking for
			if (!strcmp(token, "{"))
			{
				// is that a legit option?
				if (b_template_start)
				{
					// yup!
					b_template_start = FALSE;

					// remember that it could be only a signal message, so name is all that it contains
					b_template_end = TRUE;

					// start working on it!
					b_template = TRUE;
				}
				else if (b_block_start)
				{
					// yup!
					b_block_start = FALSE;
					b_template_end = FALSE;

					// start working on it!
					b_block = TRUE;
				}
				else if (b_variable_start)
				{
					// yup!
					b_variable_start = FALSE;
					b_block_end = FALSE;

					// start working on it!
					b_variable = TRUE;
				}
				else
				{
					llerrs << "Detcted unexpected token '" << token 
						<< "' while parsing template." << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
			}

			if (!strcmp(token, "}"))
			{
				// is that a legit option?
				if (b_template_end)
				{
					// yup!
					b_template_end = FALSE;
					b_template = FALSE;
					b_block_start = FALSE;

					// add data!
					// we've gotten a complete variable! hooray!
					// add it!
					addTemplate(templatep);

					//llinfos << "Read template: "templatep->mNametemp_str
					//	<< llendl;

					// look for next one!
					b_template_start = TRUE;
				}
				else if (b_block_end)
				{
					// yup!
					b_block_end = FALSE;
					b_variable_start = FALSE;

					// add data!
					// we've gotten a complete variable! hooray!
					// add it to template

					templatep->addBlock(blockp);

					// start working on it!
					b_template_end = TRUE;
					b_block_start = TRUE;
				}
				else if (b_variable_end)
				{
					// yup!
					b_variable_end = FALSE;

					// add data!
					// we've gotten a complete variable! hooray!
					// add it to block
					blockp->addVariable(var.getName(), var.getType(), var.getSize());

					// start working on it!
					b_variable_start = TRUE;
					b_block_end = TRUE;
				}
				else
				{
					llerrs << "Detcted unexpected token '" << token 
						<< "' while parsing template." << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
			}

			// now, are we looking to start a template?
			if (b_template)
			{

				b_template = FALSE;

				// name first
				if (fscanf(messagefilep, formatString, template_name) == EOF)
				{
					// oops, file ended
					llerrs << "Expected message template name, but file ended"
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				// debugging to help figure out busted templates
				//llinfos << template_name << llendl;

				// is name a legit C variable name
				if (!b_variable_ok(template_name))
				{
					// nope!
					llerrs << "Not legal message template name: "
						<< template_name << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = template_name;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				// ok, now get Frequency ("High", "Medium", or "Low")
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// oops, file ended
					llerrs << "Expected message template frequency, found EOF."
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = token;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				// which one is it?
				if (!strcmp(token, "High"))
				{
					if (++mNumberHighFreqMessages == 255)
					{
						// oops, too many High Frequency messages!!
						llerrs << "Message " << template_name
							<< " exceeded 254 High frequency messages!"
							<< llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a template!
					// message number is just mNumberHighFreqMessages
					templatep = new LLMessageTemplate(template_name, mNumberHighFreqMessages, MFT_HIGH);
					//lldebugs << "Template " << template_name << " # "
					//		 << std::hex << mNumberHighFreqMessages
					//		 << std::dec << " high"
					//		 << llendl;
				}
				else if (!strcmp(token, "Medium"))
				{
					if (++mNumberMediumFreqMessages == 255)
					{
						// oops, too many Medium Frequency messages!!
						llerrs << "Message " << template_name
							<< " exceeded 254 Medium frequency messages!"
							<< llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a template!
					// message number is ((255 << 8) | mNumberMediumFreqMessages)
					templatep = new LLMessageTemplate(template_name, (255 << 8) | mNumberMediumFreqMessages, MFT_MEDIUM);
					//lldebugs << "Template " << template_name << " # "
					//		 << std::hex <<  mNumberMediumFreqMessages
					//		 << std::dec << " medium"
					//		 << llendl;
				}
				else if (!strcmp(token, "Low"))
				{
					if (++mNumberLowFreqMessages == 65535)
					{
						// oops, too many High Frequency messages!!
						llerrs << "Message " << template_name
							<< " exceeded 65534 Low frequency messages!"
							<< llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a template!
					// message number is ((255 << 24) | (255 << 16) | mNumberLowFreqMessages)
					templatep = new LLMessageTemplate(template_name, (255 << 24) | (255 << 16) | mNumberLowFreqMessages, MFT_LOW);
					//lldebugs << "Template " << template_name << " # "
					//		 << std::hex << mNumberLowFreqMessages
					//		 << std::dec << " low"
					//		 << llendl;
				}
				else if (!strcmp(token, "Fixed"))
				{
					U32 message_num = 0;
					if (fscanf(messagefilep, formatString, token) == EOF)
					{
						// oops, file ended
						llerrs << "Expected message template number (fixed),"
							<< " found EOF." << llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}

					checkp = token;
					while (*checkp)
					{
						mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
						checksum_offset = (checksum_offset + 8) % 32;
					}
					
					message_num = strtoul(token,NULL,0);

					// ok, we can create a template!
					// message number is ((255 << 24) | (255 << 16) | mNumberLowFreqMessages)
					templatep = new LLMessageTemplate(template_name, message_num, MFT_LOW);
				}
				else
				{
					// oops, bad frequency line
					llerrs << "Bad frequency! " << token
					   << " isn't High, Medium, or Low" << llendl
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				// Now get trust ("Trusted", "NotTrusted")
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// File ended
					llerrs << "Expected message template "
						"trust, but file ended."
					       << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				checkp = token;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32) *checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				if (strcmp(token, "Trusted") == 0)
				{
					templatep->setTrust(MT_TRUST);
				}
				else if (strcmp(token, "NotTrusted") == 0)
				{
					templatep->setTrust(MT_NOTRUST);
				}
				else
				{
					// bad trust token
					llerrs << "bad trust: " << token
					       << " isn't Trusted or NotTrusted"
					       << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				// get encoding
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// File ended
					llerrs << "Expected message encoding, but file ended."
					       << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				checkp = token;
				while(*checkp)
				{
					mMessageFileChecksum += ((U32) *checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				if(0 == strcmp(token, "Unencoded"))
				{
					templatep->setEncoding(ME_UNENCODED);
				}
				else if(0 == strcmp(token, "Zerocoded"))
				{
					templatep->setEncoding(ME_ZEROCODED);
				}
				else
				{
					// bad trust token
					llerrs << "bad encoding: " << token
						   << " isn't Unencoded or Zerocoded" << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				// ok, now we need to look for a block
				b_block_start = TRUE;
				continue;
			}

			// now, are we looking to start a template?
			if (b_block)
			{
				b_block = FALSE;
				// ok, need to pull header info

				// name first
				if (fscanf(messagefilep, formatString, block_name) == EOF)
				{
					// oops, file ended
					llerrs << "Expected block name, but file ended" << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = block_name;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				// is name a legit C variable name
				if (!b_variable_ok(block_name))
				{
					// nope!
					llerrs << block_name << "is not a legal block name"
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				
				// now, block type ("Single", "Multiple", or "Variable")
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// oops, file ended
					llerrs << "Expected block type, but file ended." << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = token;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				// which one is it?
				if (!strcmp(token, "Single"))
				{
					// ok, we can create a block
					blockp = new LLMessageBlock(block_name, MBT_SINGLE);
				}
				else if (!strcmp(token, "Multiple"))
				{
					// need to get the number of repeats
					if (fscanf(messagefilep, formatString, token) == EOF)
					{
						// oops, file ended
						llerrs << "Expected block multiple count,"
							" but file ended." << llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}

					checkp = token;
					while (*checkp)
					{
						mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
						checksum_offset = (checksum_offset + 8) % 32;
					}

					// is it a legal integer
					if (!b_positive_integer_ok(token))
					{
						// nope!
						llerrs << token << "is not a legal integer for"
							" block multiple count" << llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a block
					blockp = new LLMessageBlock(block_name, MBT_MULTIPLE, atoi(token));
				}
				else if (!strcmp(token, "Variable"))
				{
					// ok, we can create a block
					blockp = new LLMessageBlock(block_name, MBT_VARIABLE);
				}
				else
				{
					// oops, bad block type
					llerrs << "Bad block type! " << token
					   << " isn't Single, Multiple, or Variable" << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				// ok, now we need to look for a variable
				b_variable_start = TRUE;
				continue;
			}

			// now, are we looking to start a template?
			if (b_variable)
			{
				b_variable = FALSE;
				// ok, need to pull header info

				// name first
				if (fscanf(messagefilep, formatString, var_name) == EOF)
				{
					// oops, file ended
					llerrs << "Expected variable name, but file ended."
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = var_name;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				// is name a legit C variable name
				if (!b_variable_ok(var_name))
				{
					// nope!
					llerrs << var_name << " is not a legal variable name"
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				
				// now, variable type ("Fixed" or "Variable")
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// oops, file ended
					llerrs << "Expected variable type, but file ended"
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				checkp = token;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}


				// which one is it?
				if (!strcmp(token, "U8"))
				{
					var = LLMessageVariable(var_name, MVT_U8, 1);					
				}
				else if (!strcmp(token, "U16"))
				{
					var = LLMessageVariable(var_name, MVT_U16, 2);					
				}
				else if (!strcmp(token, "U32"))
				{
					var = LLMessageVariable(var_name, MVT_U32, 4);					
				}
				else if (!strcmp(token, "U64"))
				{
					var = LLMessageVariable(var_name, MVT_U64, 8);					
				}
				else if (!strcmp(token, "S8"))
				{
					var = LLMessageVariable(var_name, MVT_S8, 1);					
				}
				else if (!strcmp(token, "S16"))
				{
					var = LLMessageVariable(var_name, MVT_S16, 2);					
				}
				else if (!strcmp(token, "S32"))
				{
					var = LLMessageVariable(var_name, MVT_S32, 4);					
				}
				else if (!strcmp(token, "S64"))
				{
					var = LLMessageVariable(var_name, MVT_S64, 8);					
				}
				else if (!strcmp(token, "F32"))
				{
					var = LLMessageVariable(var_name, MVT_F32, 4);					
				}
				else if (!strcmp(token, "F64"))
				{
					var =  LLMessageVariable(var_name, MVT_F64, 8);					
				}
				else if (!strcmp(token, "LLVector3"))
				{
					var = LLMessageVariable(var_name, MVT_LLVector3, 12);					
				}
				else if (!strcmp(token, "LLVector3d"))
				{
					var = LLMessageVariable(var_name, MVT_LLVector3d, 24);
				}
				else if (!strcmp(token, "LLVector4"))
				{
					var = LLMessageVariable(var_name, MVT_LLVector4, 16);					
				}
				else if (!strcmp(token, "LLQuaternion"))
				{
					var = LLMessageVariable(var_name, MVT_LLQuaternion, 12);
				}
				else if (!strcmp(token, "LLUUID"))
				{
					var = LLMessageVariable(var_name, MVT_LLUUID, 16);					
				}
				else if (!strcmp(token, "BOOL"))
				{
					var = LLMessageVariable(var_name, MVT_BOOL, 1);					
				}
				else if (!strcmp(token, "IPADDR"))
				{
					var = LLMessageVariable(var_name, MVT_IP_ADDR, 4);					
				}
				else if (!strcmp(token, "IPPORT"))
				{
					var = LLMessageVariable(var_name, MVT_IP_PORT, 2);					
				}
				else if (!strcmp(token, "Fixed"))
				{
					// need to get the variable size
					if (fscanf(messagefilep, formatString, token) == EOF)
					{
						// oops, file ended
						llerrs << "Expected variable size, but file ended"
							<< llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}

					checkp = token;
					while (*checkp)
					{
						mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
						checksum_offset = (checksum_offset + 8) % 32;
					}

					// is it a legal integer
					if (!b_positive_integer_ok(token))
					{
						// nope!
						llerrs << token << " is not a legal integer for"
							" variable size" << llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a block
					var = LLMessageVariable(var_name, MVT_FIXED, atoi(token));
				}
				else if (!strcmp(token, "Variable"))
				{
					// need to get the variable size
					if (fscanf(messagefilep, formatString, token) == EOF)
					{
						// oops, file ended
						llerrs << "Expected variable size, but file ended"
							<< llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}

					checkp = token;
					while (*checkp)
					{
						mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
						checksum_offset = (checksum_offset + 8) % 32;
					}

					// is it a legal integer
					if (!b_positive_integer_ok(token))
					{
						// nope!
						llerrs << token << "is not a legal integer"
							" for variable size" << llendl;
						mbError = TRUE;
						fclose(messagefilep);
						return;
					}
					// ok, we can create a block
					var = LLMessageVariable(var_name, MVT_VARIABLE, atoi(token));
				}
				else
				{
					// oops, bad variable type
					llerrs << "Bad variable type! " << token
						<< " isn't Fixed or Variable" << llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}

				// we got us a variable!
				b_variable_end = TRUE;
				continue;
			}

			// do we have a version number stuck in the file?
			if (!strcmp(token, "version"))
			{
				// version number 
				if (fscanf(messagefilep, formatString, token) == EOF)
				{
					// oops, file ended
					llerrs << "Expected version number, but file ended" 
						<< llendl;
					mbError = TRUE;
					fclose(messagefilep);
					return;
				}
				
				checkp = token;
				while (*checkp)
				{
					mMessageFileChecksum += ((U32)*checkp++) << checksum_offset;
					checksum_offset = (checksum_offset + 8) % 32;
				}

				mMessageFileVersionNumber = (F32)atof(token);
				
//				llinfos << "### Message template version " << mMessageFileVersionNumber << " ###" << llendl;
				continue;
   			}
		}

	    llinfos << "Message template checksum = " << std::hex << mMessageFileChecksum << std::dec << llendl;
	}
	else
	{
		llwarns << "Failed to open template: " << filename << llendl;
		mbError = TRUE;
		return;
	}
	fclose(messagefilep);
}


LLMessageSystem::~LLMessageSystem()
{
	mMessageTemplates.clear(); // don't delete templates.
	for_each(mMessageNumbers.begin(), mMessageNumbers.end(), DeletePairedPointer());
	mMessageNumbers.clear();
	
	if (!mbError)
	{
		end_net();
	}
	
	delete mCurrentRMessageData;
	mCurrentRMessageData = NULL;

	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;

	delete mPollInfop;
	mPollInfop = NULL;
}

void LLMessageSystem::clearReceiveState()
{
	mReceiveSize = -1;
	mCurrentRecvPacketID = 0;
	mCurrentRMessageTemplate = NULL;
	delete mCurrentRMessageData;
	mCurrentRMessageData = NULL;
	mIncomingCompressedSize = 0;
	mLastSender.invalidate();
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


// Returns TRUE if a valid, on-circuit message has been received.
BOOL LLMessageSystem::checkMessages( S64 frame_count )
{
	BOOL	valid_packet = FALSE;

	LLTransferTargetVFile::updateQueue();
	
	if (!mNumMessageCounts)
	{
		// This is the first message being handled after a resetReceiveCounts, we must be starting
		// the message processing loop.  Reset the timers.
		mCurrentMessageTimeSeconds = totalTime() * SEC_PER_USEC;
		mMessageCountTime = getMessageTimeSeconds();
	}

	// loop until either no packets or a valid packet
	// i.e., burn through packets from unregistered circuits
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
		
		mReceiveSize = mTrueReceiveSize;
		mLastSender = mPacketRing.getLastSender();
		
		if (mReceiveSize < (S32) LL_MINIMUM_VALID_PACKET_SIZE)
		{
			// A receive size of zero is OK, that means that there are no more packets available.
			// Ones that are non-zero but below the minimum packet size are worrisome.
			if (mReceiveSize > 0)
			{
				llwarns << "Invalid (too short) packet discarded " << mReceiveSize << llendl;
				callExceptionFunc(MX_PACKET_TOO_SHORT);
			}
			// no data in packet receive buffer
			valid_packet = FALSE;
		}
		else
		{
			LLHost host;
			LLCircuitData *cdp;
			
			// note if packet acks are appended.
			if(buffer[0] & LL_ACK_FLAG)
			{
				acks += buffer[--mReceiveSize];
				true_rcv_size = mReceiveSize;
				mReceiveSize -= acks * sizeof(TPACKETID);
			}

			// process the message as normal

			mIncomingCompressedSize = zeroCodeExpand(&buffer,&mReceiveSize);
			mCurrentRecvPacketID = buffer[1] + ((buffer[0] & 0x0f ) * 256);
			if (sizeof(TPACKETID) == 4)
			{
				mCurrentRecvPacketID *= 256;
				mCurrentRecvPacketID += buffer[2];
				mCurrentRecvPacketID *= 256;
				mCurrentRecvPacketID += buffer[3];
			}

			host = getSender();
			//llinfos << host << ":" << mCurrentRecvPacketID << llendl;

			// For testing the weird case we're having in the office where the first few packets
			// on a connection get dropped
			//if ((mCurrentRecvPacketID < 8) && !(buffer[0] & LL_RESENT_FLAG))
			//{
			//	llinfos << "Evil!  Dropping " << mCurrentRecvPacketID << " from " << host << " for fun!" << llendl;
			//	continue;
			//}

			cdp = mCircuitInfo.findCircuit(host);
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

					// I added this - I think it's correct - DJS
					// reset packet in ID
					cdp->setPacketInID(mCurrentRecvPacketID);

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

						// reset packet in ID
						cdp->setPacketInID(mCurrentRecvPacketID);
					}
				}
			}

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
						snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mReceiveSize, (mIncomingCompressedSize ? mIncomingCompressedSize : mReceiveSize), mCurrentRecvPacketID);/* Flawfinder: ignore*/
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

			valid_packet = decodeTemplate( buffer, mReceiveSize, &mCurrentRMessageTemplate );
			if( valid_packet )
			{
				mCurrentRMessageTemplate->mReceiveCount++;
				lldebugst(LLERR_MESSAGE) << "MessageRecvd:" << mCurrentRMessageTemplate->mName << " from " << host << llendl;
			}

			// UseCircuitCode is allowed in even from an invalid circuit, so that
			// we can toss circuits around.
			if (valid_packet && !cdp && (mCurrentRMessageTemplate->mName != _PREHASH_UseCircuitCode) )
			{
				logMsgFromInvalidCircuit( host, recv_reliable );
				clearReceiveState();
				valid_packet = FALSE;
			}

			if (valid_packet && cdp && !cdp->getTrusted() && (mCurrentRMessageTemplate->getTrust() == MT_TRUST) )
			{
				logTrustedMsgFromUntrustedCircuit( host );
				clearReceiveState();

				sendDenyTrustedCircuit(host);
				valid_packet = FALSE;
			}

			if (valid_packet
			&& mCurrentRMessageTemplate->isBanned(cdp && cdp->getTrusted()))
			{
				llwarns << "LLMessageSystem::checkMessages "
					<< "received banned message "
					<< mCurrentRMessageTemplate->mName
					<< " from "
					<< ((cdp && cdp->getTrusted()) ? "trusted " : "untrusted ")
					<< host << llendl;
				clearReceiveState();
				valid_packet = FALSE;
			}
			
			if( valid_packet )
			{
				logValidMsg(cdp, host, recv_reliable, recv_resent, (BOOL)(acks>0) );

				valid_packet = decodeData( buffer, host );
			}

			// It's possible that the circuit went away, because ANY message can disable the circuit
			// (for example, UseCircuit, CloseCircuit, DisableSimulator).  Find it again.
			cdp = mCircuitInfo.findCircuit(host);

			if (valid_packet)
			{
				/* Code for dumping the complete contents of a message.  Keep for future use in optimizing messages.
				if( 1 )
				{
					static char* object_update = gMessageStringTable.getString("ObjectUpdate");
					if(object_update == mCurrentRMessageTemplate->mName )
					{
						llinfos << "ObjectUpdate:" << llendl;
						U32 i;
						llinfos << "    Zero Encoded: " << zero_unexpanded_size << llendl;
						for( i = 0; i<zero_unexpanded_size; i++ )
						{
							llinfos << "     " << i << ": " << (U32) zero_unexpanded_buffer[i] << llendl;
						}
						llinfos << "" << llendl;

						llinfos << "    Zero Unencoded: " << mReceiveSize << llendl;
						for( i = 0; i<mReceiveSize; i++ )
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
					llwarns << "Packet "
							<< (mCurrentRMessageTemplate ? mCurrentRMessageTemplate->mName : "")
							<< " from invalid circuit " << host << llendl;
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
	} while (!valid_packet && mReceiveSize > 0);

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


void LLMessageSystem::newMessageFast(const char *name)
{
	mbSBuilt = FALSE;
	mbSClear = FALSE;

	mCurrentSendTotal = 0;
	mSendReliable = FALSE;

	char *namep = (char *)name; 

	if (mMessageTemplates.count(namep) > 0)
	{
		mCurrentSMessageTemplate = mMessageTemplates[namep];
		if (mCurrentSMessageData)
		{
			delete mCurrentSMessageData;
		}
		mCurrentSMessageData = new LLMsgData(namep);
		mCurrentSMessageName = namep;
		mCurrentSDataBlock = NULL;
		mCurrentSBlockName = NULL;

		// add at one of each block
		LLMessageTemplate* msg_template = mMessageTemplates[namep];
		for (LLMessageTemplate::message_block_map_t::iterator iter = msg_template->mMemberBlocks.begin();
			 iter != msg_template->mMemberBlocks.end(); iter++)
		{
			LLMessageBlock* ci = iter->second;
			LLMsgBlkData	*tblockp;
			tblockp = new LLMsgBlkData(ci->mName, 0);
			mCurrentSMessageData->addBlock(tblockp);
		}
	}
	else
	{
		llerrs << "newMessage - Message " << name << " not registered" << llendl;
	}
}

void LLMessageSystem::copyMessageRtoS()
{
	if (!mCurrentRMessageTemplate)
	{
		return;
	}
	newMessageFast(mCurrentRMessageTemplate->mName);

	// copy the blocks
	// counting variables used to encode multiple block info
	S32 block_count = 0;
    char *block_name = NULL;

	// loop through msg blocks to loop through variables, totalling up size data and filling the new (send) message
	LLMsgData::msg_blk_data_map_t::iterator iter = mCurrentRMessageData->mMemberBlocks.begin();
	LLMsgData::msg_blk_data_map_t::iterator end = mCurrentRMessageData->mMemberBlocks.end();
	for(; iter != end; ++iter)
	{
		LLMsgBlkData* mbci = iter->second;
		if(!mbci) continue;

		// do we need to encode a block code?
		if (block_count == 0)
		{
			block_count = mbci->mBlockNumber;
			block_name = (char *)mbci->mName;
		}

		// counting down mutliple blocks
		block_count--;

		nextBlockFast(block_name);

		// now loop through the variables
		LLMsgBlkData::msg_var_data_map_t::iterator dit = mbci->mMemberVarData.begin();
		LLMsgBlkData::msg_var_data_map_t::iterator dend = mbci->mMemberVarData.end();
		
		for(; dit != dend; ++dit)
		{
			LLMsgVarData& mvci = *dit;
			addDataFast(mvci.getName(), mvci.getData(), mvci.getType(), mvci.getSize());
		}
	}
}

void LLMessageSystem::clearMessage()
{
	mbSBuilt = FALSE;
	mbSClear = TRUE;

	mCurrentSendTotal = 0;
	mSendReliable = FALSE;

	mCurrentSMessageTemplate = NULL;

	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;

	mCurrentSMessageName = NULL;
	mCurrentSDataBlock = NULL;
	mCurrentSBlockName = NULL;
}


// set block to add data to within current message
void LLMessageSystem::nextBlockFast(const char *blockname)
{
	char *bnamep = (char *)blockname; 

	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to setBlock" << llendl;
		return;
	}

	// now, does this block exist?
	LLMessageTemplate::message_block_map_t::iterator temp_iter = mCurrentSMessageTemplate->mMemberBlocks.find(bnamep);
	if (temp_iter == mCurrentSMessageTemplate->mMemberBlocks.end())
	{
		llerrs << "LLMessageSystem::nextBlockFast " << bnamep
			<< " not a block in " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}
	
	LLMessageBlock* template_data = temp_iter->second;
	
	// ok, have we already set this block?
	LLMsgBlkData* block_data = mCurrentSMessageData->mMemberBlocks[bnamep];
	if (block_data->mBlockNumber == 0)
	{
		// nope! set this as the current block
		block_data->mBlockNumber = 1;
		mCurrentSDataBlock = block_data;
		mCurrentSBlockName = bnamep;

		// add placeholders for each of the variables
		for (LLMessageBlock::message_variable_map_t::iterator iter = template_data->mMemberVariables.begin();
			 iter != template_data->mMemberVariables.end(); iter++)
		{
			LLMessageVariable& ci = *(iter->second);
			mCurrentSDataBlock->addVariable(ci.getName(), ci.getType());
		}
		return;
	}
	else
	{
		// already have this block. . . 
		// are we supposed to have a new one?

		// if the block is type MBT_SINGLE this is bad!
		if (template_data->mType == MBT_SINGLE)
		{
			llerrs << "LLMessageSystem::nextBlockFast called multiple times"
				<< " for " << bnamep << " but is type MBT_SINGLE" << llendl;
			return;
		}


		// if the block is type MBT_MULTIPLE then we need a known number, make sure that we're not exceeding it
		if (  (template_data->mType == MBT_MULTIPLE)
			&&(mCurrentSDataBlock->mBlockNumber == template_data->mNumber))
		{
			llerrs << "LLMessageSystem::nextBlockFast called "
				<< mCurrentSDataBlock->mBlockNumber << " times for " << bnamep
				<< " exceeding " << template_data->mNumber
				<< " specified in type MBT_MULTIPLE." << llendl;
			return;
		}

		// ok, we can make a new one
		// modify the name to avoid name collision by adding number to end
		S32  count = block_data->mBlockNumber;

		// incrememt base name's count
		block_data->mBlockNumber++;

		if (block_data->mBlockNumber > MAX_BLOCKS)
		{
			llerrs << "Trying to pack too many blocks into MBT_VARIABLE type (limited to " << MAX_BLOCKS << ")" << llendl;
		}

		// create new name
		// Nota Bene: if things are working correctly, mCurrentMessageData->mMemberBlocks[blockname]->mBlockNumber == mCurrentDataBlock->mBlockNumber + 1

		char *nbnamep = bnamep + count;
	
		mCurrentSDataBlock = new LLMsgBlkData(bnamep, count);
		mCurrentSDataBlock->mName = nbnamep;
		mCurrentSMessageData->mMemberBlocks[nbnamep] = mCurrentSDataBlock;

		// add placeholders for each of the variables
		for (LLMessageBlock::message_variable_map_t::iterator
				 iter = template_data->mMemberVariables.begin(),
				 end = template_data->mMemberVariables.end();
			 iter != end; iter++)
		{
			LLMessageVariable& ci = *(iter->second);
			mCurrentSDataBlock->addVariable(ci.getName(), ci.getType());
		}
		return;
	}
}

// add data to variable in current block
void LLMessageSystem::addDataFast(const char *varname, const void *data, EMsgVariableType type, S32 size)
{
	char *vnamep = (char *)varname; 

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to addData" << llendl;
		return;
	}

	// do we have a current block?
	if (!mCurrentSDataBlock)
	{
		llerrs << "setBlock not called prior to addData" << llendl;
		return;
	}

	// kewl, add the data if it exists
	LLMessageVariable* var_data = mCurrentSMessageTemplate->mMemberBlocks[mCurrentSBlockName]->mMemberVariables[vnamep];
	if (!var_data || !var_data->getName())
	{
		llerrs << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}

	// ok, it seems ok. . . are we the correct size?
	if (var_data->getType() == MVT_VARIABLE)
	{
		// Variable 1 can only store 255 bytes, make sure our data is smaller
		if ((var_data->getSize() == 1) &&
			(size > 255))
		{
			llwarns << "Field " << varname << " is a Variable 1 but program "
			       << "attempted to stuff more than 255 bytes in "
			       << "(" << size << ").  Clamping size and truncating data." << llendl;
			size = 255;
			char *truncate = (char *)data;
			truncate[255] = 0;
		}

		// no correct size for MVT_VARIABLE, instead we need to tell how many bytes the size will be encoded as
		mCurrentSDataBlock->addData(vnamep, data, size, type, var_data->getSize());
		mCurrentSendTotal += size;
	}
	else
	{
		if (size != var_data->getSize())
		{
			llerrs << varname << " is type MVT_FIXED but request size " << size << " doesn't match template size "
				   << var_data->getSize() << llendl;
			return;
		}
		// alright, smash it in
		mCurrentSDataBlock->addData(vnamep, data, size, type);
		mCurrentSendTotal += size;
	}
}

// add data to variable in current block - fails if variable isn't MVT_FIXED
void LLMessageSystem::addDataFast(const char *varname, const void *data, EMsgVariableType type)
{
	char *vnamep = (char *)varname; 

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to addData" << llendl;
		return;
	}

	// do we have a current block?
	if (!mCurrentSDataBlock)
	{
		llerrs << "setBlock not called prior to addData" << llendl;
		return;
	}

	// kewl, add the data if it exists
	LLMessageVariable* var_data = mCurrentSMessageTemplate->mMemberBlocks[mCurrentSBlockName]->mMemberVariables[vnamep];
	if (!var_data->getName())
	{
		llerrs << vnamep << " not a variable in block " << mCurrentSBlockName << " of " << mCurrentSMessageTemplate->mName << llendl;
		return;
	}

	// ok, it seems ok. . . are we MVT_VARIABLE?
	if (var_data->getType() == MVT_VARIABLE)
	{
		// nope
		llerrs << vnamep << " is type MVT_VARIABLE. Call using addData(name, data, size)" << llendl;
		return;
	}
	else
	{
		mCurrentSDataBlock->addData(vnamep, data, var_data->getSize(), type);
		mCurrentSendTotal += var_data->getSize();
	}
}

BOOL LLMessageSystem::isSendFull(const char* blockname)
{
	if(!blockname)
	{
		return (mCurrentSendTotal > MTUBYTES);
	}
	return isSendFullFast(gMessageStringTable.getString(blockname));
}

BOOL LLMessageSystem::isSendFullFast(const char* blockname)
{
	if(mCurrentSendTotal > MTUBYTES)
	{
		return TRUE;
	}
	if(!blockname)
	{
		return FALSE;
	}
	char* bnamep = (char*)blockname;
	S32 max;

	LLMessageBlock* template_data = mCurrentSMessageTemplate->mMemberBlocks[bnamep];
	
	switch(template_data->mType)
	{
	case MBT_SINGLE:
		max = 1;
		break;
	case MBT_MULTIPLE:
		max = template_data->mNumber;
		break;
	case MBT_VARIABLE:
	default:
		max = MAX_BLOCKS;
		break;
	}
	if(mCurrentSMessageData->mMemberBlocks[bnamep]->mBlockNumber >= max)
	{
		return TRUE;
	}
	return FALSE;
}


// blow away the last block of a message, return FALSE if that leaves no blocks or there wasn't a block to remove
BOOL  LLMessageSystem::removeLastBlock()
{
	if (mCurrentSBlockName)
	{
		if (  (mCurrentSMessageData)
			&&(mCurrentSMessageTemplate))
		{
			if (mCurrentSMessageData->mMemberBlocks[mCurrentSBlockName]->mBlockNumber >= 1)
			{
				// At least one block for the current block name.

				// Store the current block name for future reference.
				char *block_name = mCurrentSBlockName;

				// Decrement the sent total by the size of the
				// data in the message block that we're currently building.

				LLMessageBlock* template_data = mCurrentSMessageTemplate->mMemberBlocks[mCurrentSBlockName];
				
				for (LLMessageBlock::message_variable_map_t::iterator iter = template_data->mMemberVariables.begin();
					 iter != template_data->mMemberVariables.end(); iter++)
				{
					LLMessageVariable& ci = *(iter->second);
					mCurrentSendTotal -= ci.getSize();
				}


				// Now we want to find the block that we're blowing away.

				// Get the number of blocks.
				LLMsgBlkData* block_data = mCurrentSMessageData->mMemberBlocks[block_name];
				S32 num_blocks = block_data->mBlockNumber;

				// Use the same (suspect?) algorithm that's used to generate
				// the names in the nextBlock method to find it.
				char *block_getting_whacked = block_name + num_blocks - 1;
				LLMsgBlkData* whacked_data = mCurrentSMessageData->mMemberBlocks[block_getting_whacked];
				delete whacked_data;
				mCurrentSMessageData->mMemberBlocks.erase(block_getting_whacked);

				if (num_blocks <= 1)
				{
					// we just blew away the last one, so return FALSE
					return FALSE;
				}
				else
				{
					// Decrement the counter.
					block_data->mBlockNumber--;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

// make sure that all the desired data is in place and then copy the data into mSendBuffer
void LLMessageSystem::buildMessage()
{
	// basic algorithm is to loop through the various pieces, building
	// size and offset info if we encounter a -1 for mSize at any
	// point that variable wasn't given data

	// do we have a current message?
	if (!mCurrentSMessageTemplate)
	{
		llerrs << "newMessage not called prior to buildMessage" << llendl;
		return;
	}

	// zero out some useful values

	// leave room for circuit counter
	mSendSize = LL_PACKET_ID_SIZE;

	// encode message number and adjust total_offset
	if (mCurrentSMessageTemplate->mFrequency == MFT_HIGH)
	{
// old, endian-dependant way
//		memcpy(&mSendBuffer[mSendSize], &mCurrentMessageTemplate->mMessageNumber, sizeof(U8));

// new, independant way
		mSendBuffer[mSendSize] = (U8)mCurrentSMessageTemplate->mMessageNumber;
		mSendSize += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_MEDIUM)
	{
		U8 temp = 255;
		memcpy(&mSendBuffer[mSendSize], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		mSendSize += sizeof(U8);

		// mask off unsightly bits
		temp = mCurrentSMessageTemplate->mMessageNumber & 255;
		memcpy(&mSendBuffer[mSendSize], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		mSendSize += sizeof(U8);
	}
	else if (mCurrentSMessageTemplate->mFrequency == MFT_LOW)
	{
		U8 temp = 255;
		U16  message_num;
		memcpy(&mSendBuffer[mSendSize], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		mSendSize += sizeof(U8);
		memcpy(&mSendBuffer[mSendSize], &temp, sizeof(U8));  /*Flawfinder: ignore*/
		mSendSize += sizeof(U8);

		// mask off unsightly bits
		message_num = mCurrentSMessageTemplate->mMessageNumber & 0xFFFF;

	    // convert to network byte order
		message_num = htons(message_num);
		memcpy(&mSendBuffer[mSendSize], &message_num, sizeof(U16)); /*Flawfinder: ignore*/
		mSendSize += sizeof(U16);
	}
	else
	{
		llerrs << "unexpected message frequency in buildMessage" << llendl;
		return;
	}

	// counting variables used to encode multiple block info
	S32 block_count = 0;
	U8  temp_block_number;

	// loop through msg blocks to loop through variables, totalling up size data and copying into mSendBuffer
	for (LLMsgData::msg_blk_data_map_t::iterator
			 iter = mCurrentSMessageData->mMemberBlocks.begin(),
			 end = mCurrentSMessageData->mMemberBlocks.end();
		 iter != end; iter++)
	{
		LLMsgBlkData* mbci = iter->second;
		// do we need to encode a block code?
		if (block_count == 0)
		{
			block_count = mbci->mBlockNumber;

			LLMessageBlock* template_data = mCurrentSMessageTemplate->mMemberBlocks[mbci->mName];
			
			// ok, if this is the first block of a repeating pack, set block_count and, if it's type MBT_VARIABLE encode a byte for how many there are
			if (template_data->mType == MBT_VARIABLE)
			{
				// remember that mBlockNumber is a S32
				temp_block_number = (U8)mbci->mBlockNumber;
				if ((S32)(mSendSize + sizeof(U8)) < MAX_BUFFER_SIZE)
				{
				    memcpy(&mSendBuffer[mSendSize], &temp_block_number, sizeof(U8));
				    mSendSize += sizeof(U8);
				}
				else
				{
				    // Just reporting error is likely not enough. Need
				    // to check how to abort or error out gracefully
				    // from this function. XXXTBD
				    llerrs << "buildMessage failed. Message excedding"
						" sendBuffersize." << llendl;
				}
			}
			else if (template_data->mType == MBT_MULTIPLE)
			{
				if (block_count != template_data->mNumber)
				{
					// nope!  need to fill it in all the way!
					llerrs << "Block " << mbci->mName
						<< " is type MBT_MULTIPLE but only has data for "
						<< block_count << " out of its "
						<< template_data->mNumber << " blocks" << llendl;
				}
			}
		}

		// counting down multiple blocks
		block_count--;

		// now loop through the variables
		for (LLMsgBlkData::msg_var_data_map_t::iterator iter = mbci->mMemberVarData.begin();
			 iter != mbci->mMemberVarData.end(); iter++)
		{
			LLMsgVarData& mvci = *iter;
			if (mvci.getSize() == -1)
			{
				// oops, this variable wasn't ever set!
				llerrs << "The variable " << mvci.getName() << " in block "
					<< mbci->mName << " of message "
					<< mCurrentSMessageData->mName
					<< " wasn't set prior to buildMessage call" << llendl;
			}
			else
			{
				S32 data_size = mvci.getDataSize();
				if(data_size > 0)
				{
					// The type is MVT_VARIABLE, which means that we
					// need to encode a size argument. Otherwise,
					// there is no need.
					S32 size = mvci.getSize();
					U8 sizeb;
					U16 sizeh;
					switch(data_size)
					{
					case 1:
						sizeb = size;
						htonmemcpy(&mSendBuffer[mSendSize], &sizeb, MVT_U8, 1);
						break;
					case 2:
						sizeh = size;
						htonmemcpy(&mSendBuffer[mSendSize], &sizeh, MVT_U16, 2);
						break;
					case 4:
						htonmemcpy(&mSendBuffer[mSendSize], &size, MVT_S32, 4);
						break;
					default:
						llerrs << "Attempting to build variable field with unknown size of " << size << llendl;
						break;
					}
					mSendSize += mvci.getDataSize();
				}

				// if there is any data to pack, pack it
				if((mvci.getData() != NULL) && mvci.getSize())
				{
					if(mSendSize + mvci.getSize() < (S32)sizeof(mSendBuffer))
					{
					    memcpy(
							&mSendBuffer[mSendSize],
							mvci.getData(),
							mvci.getSize());
					    mSendSize += mvci.getSize();
					}
					else
					{
					    // Just reporting error is likely not
					    // enough. Need to check how to abort or error
					    // out gracefully from this function. XXXTBD
						llerrs << "LLMessageSystem::buildMessage failed. "
							<< "Attempted to pack "
							<< mSendSize + mvci.getSize()
							<< " bytes into a buffer with size "
							<< mSendBuffer << "." << llendl
					}						
				}
			}
		}
	}
	mbSBuilt = TRUE;
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

	return sendReliable(host, 0, FALSE, timeout, callback, callback_data);
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
		callback, callback_data, mCurrentSMessageName);
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
	if (mCurrentSendTotal)
	{
		mSendReliable = TRUE;
		// No need for ping-based retry as not going to retry
		mReliablePacketParams.set(host, 0, FALSE, timeout, callback, callback_data, mCurrentSMessageName);
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
	if (mCurrentSendTotal)
	{
		send_bytes = sendReliable(host);
	}
	clearMessage();
	return send_bytes;
}

		
// This can be called from signal handlers,
// so should should not use llinfos.
S32 LLMessageSystem::sendMessage(const LLHost &host)
{
	if (!mbSBuilt)
	{
		buildMessage();
	}

	mCurrentSendTotal = 0;

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
						<< mCurrentSMessageName << llendl;
			}
			llwarns << "sendMessage - Trying to send "
					<< mCurrentSMessageName << " on unknown circuit "
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
						<< mCurrentSMessageName << llendl;
			}
			llwarns << "sendMessage - Trying to send message "
					<< mCurrentSMessageName << " to dead circuit "
					<< host << llendl;
			return 0;
		}
	}

	memset(mSendBuffer,0,LL_PACKET_ID_SIZE); // zero out the packet ID field

	// add the send id to the front of the message
	cdp->nextPacketOutID();

	// Packet ID size is always 4
	*((S32*)&mSendBuffer[0]) = htonl(cdp->getPacketOutID());

	// Compress the message, which will usually reduce its size.
	U8 * buf_ptr = (U8 *)mSendBuffer;
	S32 buffer_length = mSendSize;
	if(ME_ZEROCODED == mCurrentSMessageTemplate->getEncoding())
	{
		zeroCode(&buf_ptr, &buffer_length);
	}

	if (buffer_length > 1500)
	{
		if((mCurrentSMessageName != _PREHASH_ChildAgentUpdate)
		   && (mCurrentSMessageName != _PREHASH_SendXferPacket))
		{
			llwarns << "sendMessage - Trying to send "
					<< ((buffer_length > 4000) ? "EXTRA " : "")
					<< "BIG message " << mCurrentSMessageName << " - "
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
	   (mCurrentSMessageName != _PREHASH_PacketAck))
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
			    memcpy(&buf_ptr[buffer_length], &packet_id, sizeof(TPACKETID));
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
			<< mCurrentSMessageTemplate->mName
			<< (mSendReliable ? " reliable " : "");
		if(is_ack_appended)
		{
			str << "\tACKS:\t";
			std::ostream_iterator<TPACKETID> append(str, " ");
			std::copy(acks.begin(), acks.end(), append);
		}
		llinfos << str.str() << llendl;
	}

	lldebugst(LLERR_MESSAGE) << "MessageSent at: " << (S32)totalTime() 
		<< ", " << mCurrentSMessageTemplate->mName
		<< " to " << host 
		<< llendl;

	// ok, clean up temp data
	delete mCurrentSMessageData;
	mCurrentSMessageData = NULL;

	mPacketsOut++;
	mBytesOut += buffer_length;
	
	return buffer_length;
}


// Returns template for the message contained in buffer
BOOL LLMessageSystem::decodeTemplate(  
		const U8* buffer, S32 buffer_size,  // inputs
		LLMessageTemplate** msg_template ) // outputs
{
	const U8* header = buffer + LL_PACKET_ID_SIZE;

	// is there a message ready to go?
	if (buffer_size <= 0)
	{
		llwarns << "No message waiting for decode!" << llendl;
		return(FALSE);
	}

	U32 num = 0;

	if (header[0] != 255)
	{
		// high frequency message
		num = header[0];
	}
	else if ((buffer_size >= ((S32) LL_MINIMUM_VALID_PACKET_SIZE + 1)) && (header[1] != 255))
	{
		// medium frequency message
		num = (255 << 8) | header[1];
	}
	else if ((buffer_size >= ((S32) LL_MINIMUM_VALID_PACKET_SIZE + 3)) && (header[1] == 255))
	{
		// low frequency message
		U16	message_id_U16 = 0;
		// I think this check busts the message system.
		// it appears that if there is a NULL in the message #, it won't copy it....
		// what was the goal?
		//if(header[2])
		memcpy(&message_id_U16, &header[2], 2);

		// dependant on endian-ness:
		//		U32	temp = (255 << 24) | (255 << 16) | header[2];

		// independant of endian-ness:
		message_id_U16 = ntohs(message_id_U16);
		num = 0xFFFF0000 | message_id_U16;
	}
	else // bogus packet received (too short)
	{
		llwarns << "Packet with unusable length received (too short): "
				<< buffer_size << llendl;
		return(FALSE);
	}

	LLMessageTemplate* temp = get_ptr_in_map(mMessageNumbers,num);
	if (temp)
	{
		*msg_template = temp;
	}
	else
	{
		llwarns << "Message #" << std::hex << num << std::dec
			<< " received but not registered!" << llendl;
		callExceptionFunc(MX_UNREGISTERED_MESSAGE);
		return(FALSE);
	}

	return(TRUE);
}


void LLMessageSystem::logMsgFromInvalidCircuit( const LLHost& host, BOOL recv_reliable )
{
	if(mVerboseLog)
	{
		std::ostringstream str;
		str << "MSG: <- " << host;
		char buffer[MAX_STRING];			/* Flawfinder: ignore */
		snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mReceiveSize, (mIncomingCompressedSize ? mIncomingCompressedSize: mReceiveSize), mCurrentRecvPacketID);		/* Flawfinder: ignore */
		str << buffer
			<< mCurrentRMessageTemplate->mName
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
		mMessageCountList[mNumMessageCounts].mMessageNum = mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes = mReceiveSize;
		mMessageCountList[mNumMessageCounts].mInvalid = TRUE;
		mNumMessageCounts++;
	}
}

void LLMessageSystem::logTrustedMsgFromUntrustedCircuit( const LLHost& host )
{
	llwarns << "Recieved trusted message on untrusted circuit. "
	   	<< "Will reply with deny. "
		<< "Message: " << mCurrentRMessageTemplate->mName
		<< " Host: " << host << llendl;
	if (mNumMessageCounts >= MAX_MESSAGE_COUNT_NUM)
	{
		llwarns << "got more than " << MAX_MESSAGE_COUNT_NUM
			<< " packets without clearing counts"
			<< llendl;
	}
	else
	{
		mMessageCountList[mNumMessageCounts].mMessageNum
			= mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes
			= mReceiveSize;
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
		mMessageCountList[mNumMessageCounts].mMessageNum = mCurrentRMessageTemplate->mMessageNumber;
		mMessageCountList[mNumMessageCounts].mMessageBytes = mReceiveSize;
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
		snprintf(buffer, MAX_STRING, "\t%6d\t%6d\t%6d ", mReceiveSize, (mIncomingCompressedSize ? mIncomingCompressedSize : mReceiveSize), mCurrentRecvPacketID);		/* Flawfinder: ignore */
		str << buffer
			<< mCurrentRMessageTemplate->mName
			<< (recv_reliable ? " reliable" : "")
			<< (recv_resent ? " resent" : "")
			<< (recv_acks ? " acks" : "");
		llinfos << str.str() << llendl;
	}
}


void LLMessageSystem::logRanOffEndOfPacket( const LLHost& host )
{
	// we've run off the end of the packet!
	llwarns << "Ran off end of packet " << mCurrentRMessageTemplate->mName
			<< " with id " << mCurrentRecvPacketID << " from " << host
			<< llendl;
	if(mVerboseLog)
	{
		llinfos << "MSG: -> " << host << "\tREAD PAST END:\t"
				<< mCurrentRecvPacketID << " "
				<< mCurrentSMessageTemplate->mName << llendl;
	}
	callExceptionFunc(MX_RAN_OFF_END_OF_PACKET);
}


// decode a given message
BOOL LLMessageSystem::decodeData(const U8* buffer, const LLHost& sender )
{
	llassert( mReceiveSize >= 0 );
	llassert( mCurrentRMessageTemplate);
	llassert( !mCurrentRMessageData );
	delete mCurrentRMessageData; // just to make sure

	S32 decode_pos = LL_PACKET_ID_SIZE + (S32)(mCurrentRMessageTemplate->mFrequency);

	// create base working data set
	mCurrentRMessageData = new LLMsgData(mCurrentRMessageTemplate->mName);

	// loop through the template building the data structure as we go
	for (LLMessageTemplate::message_block_map_t::iterator iter = mCurrentRMessageTemplate->mMemberBlocks.begin();
		 iter != mCurrentRMessageTemplate->mMemberBlocks.end(); iter++)
	{
		LLMessageBlock* mbci = iter->second;
		U8	repeat_number;
		S32	i;

		// how many of this block?

		if (mbci->mType == MBT_SINGLE)
		{
			// just one
			repeat_number = 1;
		}
		else if (mbci->mType == MBT_MULTIPLE)
		{
			// a known number
			repeat_number = mbci->mNumber;
		}
		else if (mbci->mType == MBT_VARIABLE)
		{
			// need to read the number from the message
			// repeat number is a single byte
			if (decode_pos >= mReceiveSize)
			{
				logRanOffEndOfPacket( sender );
				return FALSE;
			}
			repeat_number = buffer[decode_pos];
			decode_pos++;
		}
		else
		{
			llerrs << "Unknown block type" << llendl;
			return FALSE;
		}

		LLMsgBlkData* cur_data_block = NULL;

		// now loop through the block
		for (i = 0; i < repeat_number; i++)
		{
			if (i)
			{
				// build new name to prevent collisions
				// TODO: This should really change to a vector
				cur_data_block = new LLMsgBlkData(mbci->mName, repeat_number);
				cur_data_block->mName = mbci->mName + i;
			}
			else
			{
				cur_data_block = new LLMsgBlkData(mbci->mName, repeat_number);
			}

			// add the block to the message
			mCurrentRMessageData->addBlock(cur_data_block);

			// now read the variables
			for (LLMessageBlock::message_variable_map_t::iterator iter = mbci->mMemberVariables.begin();
				 iter != mbci->mMemberVariables.end(); iter++)
			{
				LLMessageVariable& mvci = *(iter->second);
				// ok, build out the variables
				// add variable block
				cur_data_block->addVariable(mvci.getName(), mvci.getType());

				// what type of variable?
				if (mvci.getType() == MVT_VARIABLE)
				{
					// variable, get the number of bytes to read from the template
					S32 data_size = mvci.getSize();
					U8 tsizeb = 0;
					U16 tsizeh = 0;
					U32 tsize = 0;

					if ((decode_pos + data_size) > mReceiveSize)
					{
						logRanOffEndOfPacket( sender );
						return FALSE;
					}
					switch(data_size)
					{
					case 1:
						htonmemcpy(&tsizeb, &buffer[decode_pos], MVT_U8, 1);
						tsize = tsizeb;
						break;
					case 2:
						htonmemcpy(&tsizeh, &buffer[decode_pos], MVT_U16, 2);
						tsize = tsizeh;
						break;
					case 4:
						htonmemcpy(&tsizeb, &buffer[decode_pos], MVT_U32, 4);
						break;
					default:
						llerrs << "Attempting to read variable field with unknown size of " << data_size << llendl;
						break;
						
					}
					decode_pos += data_size;

					if ((decode_pos + (S32)tsize) > mReceiveSize)
					{
						logRanOffEndOfPacket( sender );
						return FALSE;
					}
					cur_data_block->addData(mvci.getName(), &buffer[decode_pos], tsize, mvci.getType());
					decode_pos += tsize;
				}
				else
				{
					// fixed!
					// so, copy data pointer and set data size to fixed size

					if ((decode_pos + mvci.getSize()) > mReceiveSize)
					{
						logRanOffEndOfPacket( sender );
						return FALSE;
					}

					cur_data_block->addData(mvci.getName(), &buffer[decode_pos], mvci.getSize(), mvci.getType());
					decode_pos += mvci.getSize();
				}
			}
		}
	}

	if (mCurrentRMessageData->mMemberBlocks.empty()
		&& !mCurrentRMessageTemplate->mMemberBlocks.empty())
	{
		lldebugs << "Empty message '" << mCurrentRMessageTemplate->mName << "' (no blocks)" << llendl;
		return FALSE;
	}

	{
		static LLTimer decode_timer;

		if( mTimeDecodes )
		{
			decode_timer.reset();
		}

		//	if( mCurrentRMessageTemplate->mName == _PREHASH_AgentToNewRegion )
		//	{
		//		VTResume();  // VTune
		//	}

		{
			LLFastTimer t(LLFastTimer::FTM_PROCESS_MESSAGES);
			if( !mCurrentRMessageTemplate->callHandlerFunc(this) )
			{
				llwarns << "Message from " << sender << " with no handler function received: " << mCurrentRMessageTemplate->mName << llendl;
			}
		}

		//	if( mCurrentRMessageTemplate->mName == _PREHASH_AgentToNewRegion )
		//	{
		//		VTPause();	// VTune
		//	}

		if( mTimeDecodes )
		{
			F32 decode_time = decode_timer.getElapsedTimeF32();
			mCurrentRMessageTemplate->mDecodeTimeThisFrame += decode_time;

			mCurrentRMessageTemplate->mTotalDecoded++;
			mCurrentRMessageTemplate->mTotalDecodeTime += decode_time;

			if( mCurrentRMessageTemplate->mMaxDecodeTimePerMsg < decode_time )
			{
				mCurrentRMessageTemplate->mMaxDecodeTimePerMsg = decode_time;
			}


			if( decode_time > mTimeDecodesSpamThreshold )
			{
				lldebugs << "--------- Message " << mCurrentRMessageTemplate->mName << " decode took " << decode_time << " seconds. (" <<
					mCurrentRMessageTemplate->mMaxDecodeTimePerMsg << " max, " <<
					(mCurrentRMessageTemplate->mTotalDecodeTime / mCurrentRMessageTemplate->mTotalDecoded) << " avg)" << llendl;
			}
		}
	}
	return TRUE;
}

void LLMessageSystem::getDataFast(const char *blockname, const char *varname, void *datap, S32 size, S32 blocknum, S32 max_size)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 2!" << llendl;
		return;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentMessageData in getData!" << llendl;
		return;
	}

	char *bnamep = (char *)blockname + blocknum; // this works because it's just a hash.  The bnamep is never derefference
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);

	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		llerrs << "Block " << blockname << " #" << blocknum
			<< " not in message " << mCurrentRMessageData->mName << llendl;
		return;
	}

	LLMsgBlkData *msg_block_data = iter->second;
	LLMsgVarData& vardata = msg_block_data->mMemberVarData[vnamep];

	if (!vardata.getName())
	{
		llerrs << "Variable "<< vnamep << " not in message "
			<< mCurrentRMessageData->mName<< " block " << bnamep << llendl;
		return;
	}

	if (size && size != vardata.getSize())
	{
		llerrs << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but copying into buffer of size " << size
			<< llendl;
		return;
	}


	const S32 vardata_size = vardata.getSize();
	if( max_size >= vardata_size )
	{   
		switch( vardata_size )
		{ 
		case 1:
			*((U8*)datap) = *((U8*)vardata.getData());
			break;
		case 2:
			*((U16*)datap) = *((U16*)vardata.getData());
			break;
		case 4:
			*((U32*)datap) = *((U32*)vardata.getData());
			break;
		case 8:
			((U32*)datap)[0] = ((U32*)vardata.getData())[0];
			((U32*)datap)[1] = ((U32*)vardata.getData())[1];
			break;
		default:
			memcpy(datap, vardata.getData(), vardata_size);
			break;
		}
	}
	else
	{
		llwarns << "Msg " << mCurrentRMessageData->mName 
			<< " variable " << vnamep
			<< " is size " << vardata.getSize()
			<< " but truncated to max size of " << max_size
			<< llendl;

		memcpy(datap, vardata.getData(), max_size);
	}
}

S32 LLMessageSystem::getNumberOfBlocksFast(const char *blockname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 3!" << llendl;
		return -1;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return -1;
	}

	char *bnamep = (char *)blockname; 

	LLMsgData::msg_blk_data_map_t::iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
//		sprintf(errmsg, "Block %s not in message %s", bnamep, mCurrentRMessageData->mName);
//		llerrs << errmsg << llendl;
//		return -1;
		return 0;
	}

	return (iter->second)->mBlockNumber;
}

S32 LLMessageSystem::getSizeFast(const char *blockname, const char *varname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 4!" << llendl;
		return -1;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return -1;
	}

	char *bnamep = (char *)blockname; 

	LLMsgData::msg_blk_data_map_t::iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		llerrs << "Block " << bnamep << " not in message "
			<< mCurrentRMessageData->mName << llendl;
		return -1;
	}

	char *vnamep = (char *)varname; 

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{
		llerrs << "Variable " << varname << " not in message "
			<< mCurrentRMessageData->mName << " block " << bnamep << llendl;
		return -1;
	}

	if (mCurrentRMessageTemplate->mMemberBlocks[bnamep]->mType != MBT_SINGLE)
	{
		llerrs << "Block " << bnamep << " isn't type MBT_SINGLE,"
			" use getSize with blocknum argument!" << llendl;
		return -1;
	}

	return vardata.getSize();
}


S32 LLMessageSystem::getSizeFast(const char *blockname, S32 blocknum, const char *varname)
{
	// is there a message ready to go?
	if (mReceiveSize == -1)
	{
		llerrs << "No message waiting for decode 5!" << llendl;
		return -1;
	}

	if (!mCurrentRMessageData)
	{
		llerrs << "Invalid mCurrentRMessageData in getData!" << llendl;
		return -1;
	}

	char *bnamep = (char *)blockname + blocknum; 
	char *vnamep = (char *)varname; 

	LLMsgData::msg_blk_data_map_t::iterator iter = mCurrentRMessageData->mMemberBlocks.find(bnamep);
	
	if (iter == mCurrentRMessageData->mMemberBlocks.end())
	{
		llerrs << "Block " << bnamep << " not in message "
			<< mCurrentRMessageData->mName << llendl;
		return -1;
	}

	LLMsgBlkData* msg_data = iter->second;
	LLMsgVarData& vardata = msg_data->mMemberVarData[vnamep];
	
	if (!vardata.getName())
	{
		llerrs << "Variable " << vnamep << " not in message "
			<<  mCurrentRMessageData->mName << " block " << bnamep << llendl;
		return -1;
	}

	return vardata.getSize();
}


void LLMessageSystem::sanityCheck()
{
	if (!mCurrentRMessageData)
	{
		llerrs << "mCurrentRMessageData is NULL" << llendl;
	}

	if (!mCurrentRMessageTemplate)
	{
		llerrs << "mCurrentRMessageTemplate is NULL" << llendl;
	}

//	if (!mCurrentRTemplateBlock)
//	{
//		llerrs << "mCurrentRTemplateBlock is NULL" << llendl;
//	}

//	if (!mCurrentRDataBlock)
//	{
//		llerrs << "mCurrentRDataBlock is NULL" << llendl;
//	}

	if (!mCurrentSMessageData)
	{
		llerrs << "mCurrentSMessageData is NULL" << llendl;
	}

	if (!mCurrentSMessageTemplate)
	{
		llerrs << "mCurrentSMessageTemplate is NULL" << llendl;
	}

//	if (!mCurrentSTemplateBlock)
//	{
//		llerrs << "mCurrentSTemplateBlock is NULL" << llendl;
//	}

	if (!mCurrentSDataBlock)
	{
		llerrs << "mCurrentSDataBlock is NULL" << llendl;
	}
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
	}
	else
	{
		// Sigh, since we can open circuits which don't have circuit
		// codes, it's possible for this to happen...
		
		//llwarns << "Couldn't find circuit code for " << host << llendl;
	}

	mCircuitInfo.removeCircuitData(host);
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
void LLMessageSystem::processUseCircuitCode(LLMessageSystem* msg, void**)
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
	}
	else
	{
		llwarns << "Got zero circuit code in use_circuit_code" << llendl;
	}
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

void send_template_reply(LLMessageSystem* msg, const LLUUID& token)
{
	msg->newMessageFast(_PREHASH_TemplateChecksumReply);
	msg->nextBlockFast(_PREHASH_DataBlock);
	msg->addU32Fast(_PREHASH_Checksum, msg->mMessageFileChecksum);
	msg->addU8Fast(_PREHASH_MajorVersion,  U8(msg->mSystemVersionMajor) );
	msg->addU8Fast(_PREHASH_MinorVersion,  U8(msg->mSystemVersionMinor) );
	msg->addU8Fast(_PREHASH_PatchVersion,  U8(msg->mSystemVersionPatch) );
	msg->addU8Fast(_PREHASH_ServerVersion, U8(msg->mSystemVersionServer) );
	msg->addU32Fast(_PREHASH_Flags, msg->mVersionFlags);
	msg->nextBlockFast(_PREHASH_TokenBlock);
	msg->addUUIDFast(_PREHASH_Token, token);
	msg->sendMessage(msg->getSender());
}

void process_template_checksum_request(LLMessageSystem* msg, void**)
{
	llinfos << "Message template checksum request received from "
			<< msg->getSender() << llendl;
	send_template_reply(msg, LLUUID::null);
}

void process_secured_template_checksum_request(LLMessageSystem* msg, void**)
{
	llinfos << "Secured message template checksum request received from "
			<< msg->getSender() << llendl;
	LLUUID token;
	msg->getUUIDFast(_PREHASH_TokenBlock, _PREHASH_Token, token);
	send_template_reply(msg, token);
}

void process_log_control(LLMessageSystem* msg, void**)
{
	U8 level;
	U32 mask;
	BOOL time;
	BOOL location;
	BOOL remote_infos;

	msg->getU8Fast(_PREHASH_Options, _PREHASH_Level, level);
	msg->getU32Fast(_PREHASH_Options, _PREHASH_Mask, mask);
	msg->getBOOLFast(_PREHASH_Options, _PREHASH_Time, time);
	msg->getBOOLFast(_PREHASH_Options, _PREHASH_Location, location);
	msg->getBOOLFast(_PREHASH_Options, _PREHASH_RemoteInfos, remote_infos);

	gErrorStream.setLevel(LLErrorStream::ELevel(level));
	gErrorStream.setDebugMask(mask);
	gErrorStream.setTime(time);
	gErrorStream.setPrintLocation(location);
	gErrorStream.setElevatedRemote(remote_infos);

	llinfos << "Logging set to level " << gErrorStream.getLevel() 
		<< " mask " << std::hex << gErrorStream.getDebugMask() << std::dec
		<< " time " << gErrorStream.getTime()
		<< " loc " << gErrorStream.getPrintLocation()
		<< llendl;
}

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
}

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

	char their_digest[MD5HEX_STR_SIZE];
	msg->getBinaryDataFast(_PREHASH_DataBlock, _PREHASH_Digest, their_digest, 32);
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
	// FIXME: probably should keep a count of number of resends
	// per circuit, and stop resending after a while.
	llinfos << "Got DenyTrustedCircuit. Sending CreateTrustedCircuit to "
			<< msg->getSender() << llendl;
	msg->sendCreateTrustedCircuit(msg->getSender(), local_id, remote_id);
}

#define LL_ENCRYPT_BUF_LENGTH	16384

void encrypt_template(const char *src_name, const char *dest_name)
{
	// encrypt and decrypt are symmetric
	decrypt_template(src_name, dest_name);
}

BOOL decrypt_template(const char *src_name, const char *dest_name)
{
	S32 buf_length = LL_ENCRYPT_BUF_LENGTH;
	char buf[LL_ENCRYPT_BUF_LENGTH];
	
	FILE* infp = NULL;
	FILE* outfp = NULL;
	BOOL success = FALSE;
	char* bufp = NULL;
	U32 key = 0;
	S32 more_data = 0;

	if(src_name==NULL)
	{
		 llwarns << "Input src_name is NULL!!" << llendl;
		 goto exit;
	}

	infp = LLFile::fopen(src_name,"rb");
	if (!infp)
	{
		llwarns << "could not open " << src_name << " for reading" << llendl;
		goto exit;
	}

	if(dest_name==NULL)
	{
		 llwarns << "Output dest_name is NULL!!" << llendl;
		 goto exit;
	}

	outfp = LLFile::fopen(dest_name,"w+b");
	if (!outfp)
	{
		llwarns << "could not open " << src_name << " for writing" << llendl;
		goto exit;
	}

	while ((buf_length = (S32)fread(buf,1,LL_ENCRYPT_BUF_LENGTH,infp)))
	{
		// unscrozzle bits here
		bufp = buf;
		more_data = buf_length;
		while (more_data--)
		{
			*bufp = *bufp ^ ((key * 43) % 256);
			key++;
			bufp++;
		}

		if(buf_length != (S32)fwrite(buf,1,buf_length,outfp))
		{
			goto exit;
		}
	}
	success = TRUE;

 exit:
	if(infp) fclose(infp);
	if(outfp) fclose(outfp);
	return success;
}

void dump_prehash_files()
{
	U32 i;
	FILE *fp = LLFile::fopen("../../indra/llmessage/message_prehash.h", "w");
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
	fp = LLFile::fopen("../../indra/llmessage/message_prehash.cpp", "w");
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
	const std::string& secret)
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
	gMessageSystem->setHandlerFuncFast(_PREHASH_UseCircuitCode, LLMessageSystem::processUseCircuitCode);
	gMessageSystem->setHandlerFuncFast(_PREHASH_PacketAck,             process_packet_ack,	    NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_TemplateChecksumRequest,  process_template_checksum_request,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_SecuredTemplateChecksumRequest,  process_secured_template_checksum_request,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_LogControl,			process_log_control,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_LogMessages,			process_log_messages,	NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_CreateTrustedCircuit,
				       process_create_trusted_circuit,
				       NULL);
	gMessageSystem->setHandlerFuncFast(_PREHASH_DenyTrustedCircuit,
				       process_deny_trusted_circuit,
				       NULL);

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
	snprintf(buffer, MAX_STRING, "Total reliable packets:    %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mReliablePacketsIn)/((F32) mPacketsIn + 1));	/* Flawfinder: ignore */ 
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
	snprintf(buffer, MAX_STRING, "Total compressed packets:  %20s (%5.2f%%)", tmp_str, 100.f * ((F32) mCompressedPacketsOut)/((F32) mPacketsOut + 1));	/* Flawfinder: ignore */ 
	str << buffer << std::endl;
	savings = mUncompressedBytesOut - mCompressedBytesOut;
	U64_to_str(savings, tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Total compression savings: %20s bytes", tmp_str);	/* Flawfinder: ignore */ 
	str << buffer << std::endl;
	U64_to_str(savings/(mCompressedPacketsOut +1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg comp packet savings:   %20s (%5.2f : 1)", tmp_str, ((F32) mUncompressedBytesOut)/((F32) mCompressedBytesOut+1));	/* Flawfinder: ignore */ 
	str << buffer << std::endl;
	U64_to_str(savings/(mPacketsOut+1), tmp_str, sizeof(tmp_str));
	snprintf(buffer, MAX_STRING, "Avg overall comp savings:  %20s (%5.2f : 1)", tmp_str, ((F32) mTotalBytesOut + (F32) savings)/((F32) mTotalBytesOut + 1.f));	/* Flawfinder: ignore */ 
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
	snprintf(buffer, MAX_STRING, "On-circuit invalid packets:   %17d", mInvalidOnCircuitPackets);	/* Flawfinder: ignore */ 
	str << buffer << std::endl << std::endl;

	str << "Decoding: " << std::endl;
	snprintf(buffer, MAX_STRING, "%35s%10s%10s%10s%10s", "Message", "Count", "Time", "Max", "Avg");	/* Flawfinder: ignore */ 
	str << buffer << std:: endl;	
	F32 avg;
	for (message_template_name_map_t::iterator iter = mMessageTemplates.begin(),
			 end = mMessageTemplates.end();
		 iter != end; iter++)
	{
		LLMessageTemplate* mt = iter->second;
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
		for (message_template_name_map_t::iterator iter = mMessageTemplates.begin(),
				 end = mMessageTemplates.end();
			 iter != end; iter++)
		{
			LLMessageTemplate* mt = iter->second;
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
	return mbSClear;
}


S32 LLMessageSystem::flush(const LLHost &host)
{
	if (mCurrentSendTotal)
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


S32 LLMessageSystem::zeroCode(U8 **data, S32 *data_size)
{
	S32 count = *data_size;
	
	S32 net_gain = 0;
	U8 num_zeroes = 0;
	
	U8 *inptr = (U8 *)*data;
	U8 *outptr = (U8 *)mEncodedSendBuffer;

// skip the packet id field

	for (U32 i=0;i<LL_PACKET_ID_SIZE;i++)
	{
		count--;
		*outptr++ = *inptr++;
	}

// build encoded packet, keeping track of net size gain

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
					*outptr++ = num_zeroes;
					num_zeroes = 0;
				}
				net_gain--;   // subseqent zeroes save one
			}
			else
			{
				*outptr++ = 0;
				net_gain++;  // starting a zero count adds one
				num_zeroes = 1;
			}
			inptr++;
		}
		else
		{
			if (num_zeroes)
			{
				*outptr++ = num_zeroes;
				num_zeroes = 0;
			}
			*outptr++ = *inptr++;
		}
	}

	if (num_zeroes)
	{
		*outptr++ = num_zeroes;
	}

	if (net_gain < 0)
	{
		mCompressedPacketsOut++;
		mUncompressedBytesOut += *data_size;

		*data = mEncodedSendBuffer;
		*data_size += net_gain;
		mEncodedSendBuffer[0] |= LL_ZERO_CODE_FLAG;          // set the head bit to indicate zero coding

		mCompressedBytesOut += *data_size;

	}
	mTotalBytesOut += *data_size;

	return(net_gain);
}

S32 LLMessageSystem::zeroCodeAdjustCurrentSendTotal()
{
	if (!mbSBuilt)
	{
		buildMessage();
	}
	mbSBuilt = FALSE;

	S32 count = mSendSize;
	
	S32 net_gain = 0;
	U8 num_zeroes = 0;
	
	U8 *inptr = (U8 *)mSendBuffer;

// skip the packet id field

	for (U32 i=0;i<LL_PACKET_ID_SIZE;i++)
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



S32 LLMessageSystem::zeroCodeExpand(U8 **data, S32 *data_size)
{

	if ((*data_size ) < LL_PACKET_ID_SIZE)
	{
		llwarns << "zeroCodeExpand() called with data_size of " << *data_size << llendl;
	}
	
	mTotalBytesIn += *data_size;

	if (!(*data[0] & LL_ZERO_CODE_FLAG)) // if we're not zero-coded, just go 'way
	{
		return(0);
	}

	S32 in_size = *data_size;
	mCompressedPacketsIn++;
	mCompressedBytesIn += *data_size;
	
	*data[0] &= (~LL_ZERO_CODE_FLAG);

	S32 count = (*data_size);  
	
	U8 *inptr = (U8 *)*data;
	U8 *outptr = (U8 *)mEncodedRecvBuffer;

// skip the packet id field

	for (U32 i=0;i<LL_PACKET_ID_SIZE;i++)
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
	LLMessageTemplate* msg_template = mMessageTemplates[(char*)name];
	if (!msg_template)
	{
		llwarns << "LLMessageSystem::callHandler: unknown message " 
			<< name << llendl;
		return false;
	}
	
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

BOOL LLMessageSystem::isCircuitCodeKnown(U32 code) const
{
	if(mCircuitCodes.find(code) == mCircuitCodes.end())
		return FALSE;
	return TRUE;
}

BOOL LLMessageSystem::isMessageFast(const char *msg)
{
	if (mCurrentRMessageTemplate)
	{
		return(msg == mCurrentRMessageTemplate->mName);
	}
	else
	{
		return FALSE;
	}
}


char* LLMessageSystem::getMessageName()
{
	if (mCurrentRMessageTemplate)
	{
		return mCurrentRMessageTemplate->mName;
	}
	else
	{
		return NULL;
	}
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

void LLMessageSystem::addVector3Fast(const char *varname, const LLVector3& vec)
{
	addDataFast(varname, vec.mV, MVT_LLVector3, sizeof(vec.mV));
}

void LLMessageSystem::addVector3(const char *varname, const LLVector3& vec)
{
	addDataFast(gMessageStringTable.getString(varname), vec.mV, MVT_LLVector3, sizeof(vec.mV));
}

void LLMessageSystem::addVector4Fast(const char *varname, const LLVector4& vec)
{
	addDataFast(varname, vec.mV, MVT_LLVector4, sizeof(vec.mV));
}

void LLMessageSystem::addVector4(const char *varname, const LLVector4& vec)
{
	addDataFast(gMessageStringTable.getString(varname), vec.mV, MVT_LLVector4, sizeof(vec.mV));
}


void LLMessageSystem::addVector3dFast(const char *varname, const LLVector3d& vec)
{
	addDataFast(varname, vec.mdV, MVT_LLVector3d, sizeof(vec.mdV));
}

void LLMessageSystem::addVector3d(const char *varname, const LLVector3d& vec)
{
	addDataFast(gMessageStringTable.getString(varname), vec.mdV, MVT_LLVector3d, sizeof(vec.mdV));
}


void LLMessageSystem::addQuatFast(const char *varname, const LLQuaternion& quat)
{
	addDataFast(varname, quat.packToVector3().mV, MVT_LLQuaternion, sizeof(LLVector3));
}

void LLMessageSystem::addQuat(const char *varname, const LLQuaternion& quat)
{
	addDataFast(gMessageStringTable.getString(varname), quat.packToVector3().mV, MVT_LLQuaternion, sizeof(LLVector3));
}


void LLMessageSystem::addUUIDFast(const char *varname, const LLUUID& uuid)
{
	addDataFast(varname, uuid.mData, MVT_LLUUID, sizeof(uuid.mData));
}

void LLMessageSystem::addUUID(const char *varname, const LLUUID& uuid)
{
	addDataFast(gMessageStringTable.getString(varname), uuid.mData, MVT_LLUUID, sizeof(uuid.mData));
}

void LLMessageSystem::getF32Fast(const char *block, const char *var, F32 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(F32), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF32Fast " << block << " " << var << llendl;
		d = 0;
	}
}

void LLMessageSystem::getF32(const char *block, const char *var, F32 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(F32), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF32 " << block << " " << var << llendl;
		d = 0;
	}
}

void LLMessageSystem::getF64Fast(const char *block, const char *var, F64 &d, S32 blocknum)
{
	getDataFast(block, var, &d, sizeof(F64), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF64Fast " << block << " " << var << llendl;
		d = 0;
	}
}

void LLMessageSystem::getF64(const char *block, const char *var, F64 &d, S32 blocknum)
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), &d, sizeof(F64), blocknum);

	if( !llfinite( d ) )
	{
		llwarns << "non-finite in getF64 " << block << " " << var << llendl;
		d = 0;
	}
}


void LLMessageSystem::getVector3Fast(const char *block, const char *var, LLVector3 &v, S32 blocknum )
{
	getDataFast(block, var, v.mV, sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3Fast " << block << " " << var << llendl;
		v.zeroVec();
	}
}

void LLMessageSystem::getVector3(const char *block, const char *var, LLVector3 &v, S32 blocknum )
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), v.mV, sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector4 " << block << " " << var << llendl;
		v.zeroVec();
	}
}

void LLMessageSystem::getVector4Fast(const char *block, const char *var, LLVector4 &v, S32 blocknum )
{
	getDataFast(block, var, v.mV, sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector4Fast " << block << " " << var << llendl;
		v.zeroVec();
	}
}

void LLMessageSystem::getVector4(const char *block, const char *var, LLVector4 &v, S32 blocknum )
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), v.mV, sizeof(v.mV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3 " << block << " " << var << llendl;
		v.zeroVec();
	}
}

void LLMessageSystem::getVector3dFast(const char *block, const char *var, LLVector3d &v, S32 blocknum )
{
	getDataFast(block, var, v.mdV, sizeof(v.mdV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3dFast " << block << " " << var << llendl;
		v.zeroVec();
	}

}

void LLMessageSystem::getVector3d(const char *block, const char *var, LLVector3d &v, S32 blocknum )
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), v.mdV, sizeof(v.mdV), blocknum);

	if( !v.isFinite() )
	{
		llwarns << "non-finite in getVector3d " << block << " " << var << llendl;
		v.zeroVec();
	}
}

void LLMessageSystem::getQuatFast(const char *block, const char *var, LLQuaternion &q, S32 blocknum )
{
	LLVector3 vec;
	getDataFast(block, var, vec.mV, sizeof(vec.mV), blocknum);
	if( vec.isFinite() )
	{
		q.unpackFromVector3( vec );
	}
	else
	{
		llwarns << "non-finite in getQuatFast " << block << " " << var << llendl;
		q.loadIdentity();
	}
}

void LLMessageSystem::getQuat(const char *block, const char *var, LLQuaternion &q, S32 blocknum )
{
	LLVector3 vec;
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), vec.mV, sizeof(vec.mV), blocknum);
	if( vec.isFinite() )
	{
		q.unpackFromVector3( vec );
	}
	else
	{
		llwarns << "non-finite in getQuat " << block << " " << var << llendl;
		q.loadIdentity();
	}
}

void LLMessageSystem::getUUIDFast(const char *block, const char *var, LLUUID &u, S32 blocknum )
{
	getDataFast(block, var, u.mData, sizeof(u.mData), blocknum);
}

void LLMessageSystem::getUUID(const char *block, const char *var, LLUUID &u, S32 blocknum )
{
	getDataFast(gMessageStringTable.getString(block), gMessageStringTable.getString(var), u.mData, sizeof(u.mData), blocknum);
}

bool LLMessageSystem::generateDigestForNumberAndUUIDs(char* digest, const U32 number, const LLUUID &id1, const LLUUID &id2) const
{
	const char *colon = ":";
	char tbuf[16];	/* Flawfinder: ignore */ 
	LLMD5 d;
	LLString id1string = id1.getString();
	LLString id2string = id2.getString();
	std::string shared_secret = get_shared_secret();
	unsigned char * secret = (unsigned char*)shared_secret.c_str();
	unsigned char * id1str = (unsigned char*)id1string.c_str();
	unsigned char * id2str = (unsigned char*)id2string.c_str();

	memset(digest, 0, MD5HEX_STR_SIZE);
	
	if( secret != NULL)
	{
		d.update(secret, (U32)strlen((char *) secret));
	}

	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	
	snprintf(tbuf, sizeof(tbuf),"%i", number);	/* Flawfinder: ignore */ 
	d.update((unsigned char *) tbuf, (U32)strlen(tbuf));	/* Flawfinder: ignore */ 
	
	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	if( (char*) id1str != NULL)
	{
		d.update(id1str, (U32)strlen((char *) id1str));	 
	}
	d.update((const unsigned char *) colon, (U32)strlen(colon));	/* Flawfinder: ignore */ 
	
	if( (char*) id2str != NULL)
	{
		d.update(id2str, (U32)strlen((char *) id2str));	
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
		snprintf(line_buffer + cur_line_pos*3, sizeof(line_buffer),"%02x ", mTrueReceiveBuffer[i]);	/* Flawfinder: ignore */ 
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

