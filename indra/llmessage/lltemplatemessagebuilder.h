/** 
 * @file lltemplatemessagebuilder.h
 * @brief Declaration of LLTemplateMessageBuilder class.
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

#ifndef LL_LLTEMPLATEMESSAGEBUILDER_H
#define LL_LLTEMPLATEMESSAGEBUILDER_H

#include <map>

#include "llmessagebuilder.h"
#include "llmsgvariabletype.h"

class LLMsgData;
class LLMessageTemplate;
class LLMsgBlkData;
class LLMessageTemplate;

class LLTemplateMessageBuilder : public LLMessageBuilder
{
public:
	
	typedef std::map<const char* , LLMessageTemplate*> message_template_name_map_t;

	LLTemplateMessageBuilder(const message_template_name_map_t&);
	virtual ~LLTemplateMessageBuilder();

	virtual void newMessage(const char* name);

	virtual void nextBlock(const char* blockname);
	virtual BOOL removeLastBlock(); // TODO: babbage: remove this horror...

	/** All add* methods expect pointers to canonical varname strings. */
	virtual void addBinaryData(const char *varname, const void *data, 
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
        /**< Return built message size */
	
	virtual void clearMessage();

	// TODO: babbage: remove this horror.
	virtual void setBuilt(BOOL b);

	virtual S32 getMessageSize();
	virtual const char* getMessageName() const;

	virtual void copyFromMessageData(const LLMsgData& data);
	virtual void copyFromLLSD(const LLSD&);

	LLMsgData* getCurrentMessage() const { return mCurrentSMessageData; }
private:
	void addData(const char* varname, const void* data, 
					 EMsgVariableType type, S32 size);
	
	void addData(const char* varname, const void* data, 
						EMsgVariableType type);

	LLMsgData* mCurrentSMessageData;
	const LLMessageTemplate* mCurrentSMessageTemplate;
	LLMsgBlkData* mCurrentSDataBlock;
	char* mCurrentSMessageName;
	char* mCurrentSBlockName;
	BOOL mbSBuilt;
	BOOL mbSClear;
	S32	 mCurrentSendTotal;
	const message_template_name_map_t& mMessageTemplates;
};

#endif // LL_LLTEMPLATEMESSAGEBUILDER_H
