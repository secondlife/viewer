/** 
 * @file llmessagetemplate.h
 * @brief Declaration of the message template classes.
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLMESSAGETEMPLATE_H
#define LL_LLMESSAGETEMPLATE_H

#include "lldarray.h"
#include "message.h" // TODO: babbage: Remove...
#include "llstat.h"
#include "llstl.h"

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
	const void *getData() const { return (const void*)mData; }
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
        LLMsgBlkData(const char *name, S32 blocknum) : mBlockNumber(blocknum), mTotalSize(-1) 
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
	typedef std::map<char*, LLMsgBlkData*> msg_blk_data_map_t;
	msg_blk_data_map_t					mMemberBlocks;
	char								*mName;
	S32									mTotalSize;
};

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

	LLMessageVariable(const char *name, const EMsgVariableType type, const S32 size) : mType(type), mSize(size) 
	{
		mName = LLMessageStringTable::getInstance()->getString(name); 
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
	LLMessageBlock(const char *name, EMsgBlockType type, S32 number = 1) : mType(type), mNumber(number), mTotalSize(0) 
	{ 
		mName = LLMessageStringTable::getInstance()->getString(name);
	}

	~LLMessageBlock()
	{
		for_each(mMemberVariables.begin(), mMemberVariables.end(), DeletePointer());
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

	const LLMessageVariable* getVariable(char* name) const
	{
		message_variable_map_t::const_iterator iter = mMemberVariables.find(name);
		return iter != mMemberVariables.end()? *iter : NULL;
	}

	friend std::ostream&	 operator<<(std::ostream& s, LLMessageBlock &msg);

	typedef LLDynamicArrayIndexed<LLMessageVariable*, const char *, 8> message_variable_map_t;
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

enum EMsgDeprecation
{
	MD_NOTDEPRECATED,
	MD_UDPDEPRECATED,
	MD_UDPBLACKLISTED,
	MD_DEPRECATED
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
		mDeprecation(MD_NOTDEPRECATED),
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
		mName = LLMessageStringTable::getInstance()->getString(name);
	}

	~LLMessageTemplate()
	{
		for_each(mMemberBlocks.begin(), mMemberBlocks.end(), DeletePointer());
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

	EMsgTrust getTrust(void) const
	{
		return mTrust;
	}

	// controls for how the message should be encoded
	void setEncoding(EMsgEncoding e)
	{
		mEncoding = e;
	}
	EMsgEncoding getEncoding() const
	{
		return mEncoding;
	}

	void setDeprecation(EMsgDeprecation d)
	{
		mDeprecation = d;
	}

	EMsgDeprecation getDeprecation() const
	{
		return mDeprecation;
	}
	
	void setHandlerFunc(void (*handler_func)(LLMessageSystem *msgsystem, void **user_data), void **user_data)
	{
		mHandlerFunc = handler_func;
		mUserData = user_data;
	}

	BOOL callHandlerFunc(LLMessageSystem *msgsystem) const
	{
		if (mHandlerFunc)
		{
            LLPerfBlock msg_cb_time("msg_cb", mName);
			mHandlerFunc(msgsystem, mUserData);
			return TRUE;
		}
		return FALSE;
	}

	bool isUdpBanned() const
	{
		return mDeprecation == MD_UDPBLACKLISTED;
	}

	void banUdp();

	bool isBanned(bool trustedSource) const
	{
		return trustedSource ? mBanFromTrusted : mBanFromUntrusted;
	}

	friend std::ostream&	 operator<<(std::ostream& s, LLMessageTemplate &msg);

	const LLMessageBlock* getBlock(char* name) const
	{
		message_block_map_t::const_iterator iter = mMemberBlocks.find(name);
		return iter != mMemberBlocks.end()? *iter : NULL;
	}

public:
	typedef LLDynamicArrayIndexed<LLMessageBlock*, char*, 8> message_block_map_t;
	message_block_map_t						mMemberBlocks;
	char									*mName;
	EMsgFrequency							mFrequency;
	EMsgTrust								mTrust;
	EMsgEncoding							mEncoding;
	EMsgDeprecation							mDeprecation;
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

#endif // LL_LLMESSAGETEMPLATE_H
