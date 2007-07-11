/** 
 * @file llsdmessagebuilder.h
 * @brief Declaration of LLSDMessageBuilder class.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSDMESSAGEBUILDER_H
#define LL_LLSDMESSAGEBUILDER_H

#include <map>

#include "llmessagebuilder.h"
#include "llmsgvariabletype.h"
#include "llsd.h"

class LLMessageTemplate;
class LLMsgData;

class LLSDMessageBuilder : public LLMessageBuilder
{
public:

	//CLASS_LOG_TYPE(LLSDMessageBuilder);
	
	LLSDMessageBuilder();
	virtual ~LLSDMessageBuilder();

	virtual void newMessage(const char* name);

	virtual void nextBlock(const char* blockname);
	virtual BOOL removeLastBlock(); // TODO: babbage: remove this horror...

	/** All add* methods expect pointers to canonical varname strings. */
	virtual void addBinaryData(
		const char* varname,
		const void* data, 
		S32 size);
	virtual void addBOOL(const char* varname, BOOL b);
	virtual void addS8(const char* varname, S8 s);
	virtual void addU8(const char* varname, U8 u);
	virtual void addS16(const char* varname, S16 i);
	virtual void addU16(const char* varname, U16 i);
	virtual void addF32(const char* varname, F32 f);
	virtual void addS32(const char* varname, S32 s);
	virtual void addU32(const char* varname, U32 u);
	virtual void addU64(const char* varname, U64 lu);
	virtual void addF64(const char* varname, F64 d);
	virtual void addVector3(const char* varname, const LLVector3& vec);
	virtual void addVector4(const char* varname, const LLVector4& vec);
	virtual void addVector3d(const char* varname, const LLVector3d& vec);
	virtual void addQuat(const char* varname, const LLQuaternion& quat);
	virtual void addUUID(const char* varname, const LLUUID& uuid);
	virtual void addIPAddr(const char* varname, const U32 ip);
	virtual void addIPPort(const char* varname, const U16 port);
	virtual void addString(const char* varname, const char* s);
	virtual void addString(const char* varname, const std::string& s);

	virtual BOOL isMessageFull(const char* blockname) const;
	virtual void compressMessage(U8*& buf_ptr, U32& buffer_length);

	virtual BOOL isBuilt() const;
	virtual BOOL isClear() const;
	virtual U32 buildMessage(U8* buffer, U32 buffer_size, U8 offset_to_data);
        /**< Null implementation which returns 0. */
	
	virtual void clearMessage();

	// TODO: babbage: remove this horror.
	virtual void setBuilt(BOOL b);

	virtual S32 getMessageSize();
	virtual const char* getMessageName() const;

	virtual void copyFromMessageData(const LLMsgData& data);

	virtual void copyFromLLSD(const LLSD& msg);

	const LLSD& getMessage() const;
private:

	/* mCurrentMessage is of the following format:
		mCurrentMessage = { 'block_name1' : [ { 'block1_field1' : 'b1f1_data',
												'block1_field2' : 'b1f2_data',
												...
												'block1_fieldn' : 'b1fn_data'},
											{ 'block2_field1' : 'b2f1_data',
												'block2_field2' : 'b2f2_data',
												...
												'block2_fieldn' : 'b2fn_data'},
											...											
											{ 'blockm_field1' : 'bmf1_data',
												'blockm_field2' : 'bmf2_data',
												...
												'blockm_fieldn' : 'bmfn_data'} ],
							'block_name2' : ...,
							...
							'block_namem' } */
	LLSD mCurrentMessage;
	LLSD* mCurrentBlock;
	std::string mCurrentMessageName;
	std::string mCurrentBlockName;
	BOOL mbSBuilt;
	BOOL mbSClear;
};

#endif // LL_LLSDMESSAGEBUILDER_H
