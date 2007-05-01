#ifndef LL_LLTEMPLATEMESSAGEREADER_H
#define LL_LLTEMPLATEMESSAGEREADER_H

#include "llmessagereader.h"

#include <map>

class LLMessageTemplate;
class LLMsgData;

class LLTemplateMessageReader : public LLMessageReader
{
public:

	typedef std::map<U32, LLMessageTemplate*> message_template_number_map_t;

	LLTemplateMessageReader(message_template_number_map_t&);
	virtual ~LLTemplateMessageReader();

	/** All get* methods expect pointers to canonical strings. */
	virtual void getBinaryData(const char *blockname, const char *varname, 
							   void *datap, S32 size, S32 blocknum = 0, 
							   S32 max_size = S32_MAX);
	virtual void getBOOL(const char *block, const char *var, BOOL &data, 
						 S32 blocknum = 0);
	virtual void getS8(const char *block, const char *var, S8 &data, 
					   S32 blocknum = 0);
	virtual void getU8(const char *block, const char *var, U8 &data, 
					   S32 blocknum = 0);
	virtual void getS16(const char *block, const char *var, S16 &data, 
						S32 blocknum = 0);
	virtual void getU16(const char *block, const char *var, U16 &data, 
						S32 blocknum = 0);
	virtual void getS32(const char *block, const char *var, S32 &data, 
						S32 blocknum = 0);
	virtual void getF32(const char *block, const char *var, F32 &data, 
						S32 blocknum = 0);
	virtual void getU32(const char *block, const char *var, U32 &data, 
						S32 blocknum = 0);
	virtual void getU64(const char *block, const char *var, U64 &data, 
						S32 blocknum = 0);
	virtual void getF64(const char *block, const char *var, F64 &data, 
						S32 blocknum = 0);
	virtual void getVector3(const char *block, const char *var, 
							LLVector3 &vec, S32 blocknum = 0);
	virtual void getVector4(const char *block, const char *var, 
							LLVector4 &vec, S32 blocknum = 0);
	virtual void getVector3d(const char *block, const char *var, 
							 LLVector3d &vec, S32 blocknum = 0);
	virtual void getQuat(const char *block, const char *var, LLQuaternion &q, 
						 S32 blocknum = 0);
	virtual void getUUID(const char *block, const char *var, LLUUID &uuid, 
						 S32 blocknum = 0);
	virtual void getIPAddr(const char *block, const char *var, U32 &ip, 
						   S32 blocknum = 0);
	virtual void getIPPort(const char *block, const char *var, U16 &port, 
						   S32 blocknum = 0);
	virtual void getString(const char *block, const char *var, 
						   S32 buffer_size, char *buffer, S32 blocknum = 0);

	virtual S32	getNumberOfBlocks(const char *blockname);
	virtual S32	getSize(const char *blockname, const char *varname);
	virtual S32	getSize(const char *blockname, S32 blocknum, 
						const char *varname);

	virtual void clearMessage();

	virtual const char* getMessageName() const;
	virtual S32 getMessageSize() const;

	virtual void copyToBuilder(LLMessageBuilder&) const;

	BOOL validateMessage(const U8* buffer, S32 buffer_size, 
						 const LLHost& sender);
	BOOL readMessage(const U8* buffer, const LLHost& sender);

	bool isTrusted() const;
	bool isBanned(bool trusted_source) const;

private:

	void getData(const char *blockname, const char *varname, void *datap, 
				 S32 size = 0, S32 blocknum = 0, S32 max_size = S32_MAX);

	BOOL decodeTemplate(const U8* buffer, S32 buffer_size,  // inputs
						LLMessageTemplate** msg_template ); // outputs

	void logRanOffEndOfPacket( const LLHost& host );

	BOOL decodeData(const U8* buffer, const LLHost& sender );

	S32	mReceiveSize;
	LLMessageTemplate* mCurrentRMessageTemplate;
	LLMsgData* mCurrentRMessageData;
	message_template_number_map_t& mMessageNumbers;
};

#endif // LL_LLTEMPLATEMESSAGEREADER_H
