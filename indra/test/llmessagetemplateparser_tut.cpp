/** 
 * @file llmessagetemplateparser_tut.cpp
 * @date April 2007
 * @brief LLMessageTemplateParser unit tests
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"
#include "llmessagetemplateparser.h"
#include "lltut.h"

namespace tut
{
	struct LLMessageTemplateParserTestData {
		LLMessageTemplateParserTestData() : mMessage("unset message")
		{
		}

		~LLMessageTemplateParserTestData()
		{
		}

		void ensure_next(LLTemplateTokenizer & tokens,
						 std::string value,
						 U32 line)
		{
			std::string next = tokens.next();
			ensure_equals(mMessage + " token matches", next, value);
			ensure_equals(mMessage + " line matches", tokens.line(), line);
		}

		char * prehash(const char * name)
		{
			return gMessageStringTable.getString(name);
		}

		void ensure_block_attributes(std::string identifier,
									 const LLMessageTemplate * message,
									 const char * name,
									 EMsgBlockType type,
									 S32 number,
									 S32 total_size)
		{
			const LLMessageBlock * block = message->getBlock(prehash(name));
			identifier = identifier + ":" + message->mName + ":" + name + " block";
			ensure(identifier + " exists", block != NULL);
			ensure_equals(identifier + " name", block->mName, prehash(name));
			ensure_equals(identifier + " type", block->mType, type);
			ensure_equals(identifier + " number", block->mNumber, number);
			ensure_equals(identifier + " total size", block->mTotalSize, total_size);
		}

		void ensure_variable_attributes(std::string identifier,
										const LLMessageBlock * block,
										const char * name,
										EMsgVariableType type,
										S32 size)
		{
			const LLMessageVariable * var = block->getVariable(prehash(name));
			identifier = identifier + ":" + block->mName + ":" + name + " variable";
			ensure(identifier + " exists", var != NULL);
			ensure_equals(
				identifier + " name", var->getName(), prehash(name));
			ensure_equals(
				identifier + " type", var->getType(), type);
			ensure_equals(identifier + " size", var->getSize(), size);
		}
		
		std::string mMessage;
			
	};
	
	typedef test_group<LLMessageTemplateParserTestData> LLMessageTemplateParserTestGroup;
	typedef LLMessageTemplateParserTestGroup::object LLMessageTemplateParserTestObject;
	LLMessageTemplateParserTestGroup llMessageTemplateParserTestGroup("LLMessageTemplateParser");
	
	template<> template<>
	void LLMessageTemplateParserTestObject::test<1>()
		// tests tokenizer constructor and next methods
	{
		mMessage = "test method 1 walkthrough";
		LLTemplateTokenizer tokens("first line\nnext\t line\n\nfourth");
		ensure_next(tokens, "first", 1);
		ensure_next(tokens, "line", 1);
		ensure_next(tokens, "next", 2);
		ensure_next(tokens, "line", 2);
		ensure_next(tokens, "fourth", 4);

		tokens = LLTemplateTokenizer("\n\t{ \t   Test1 Fixed \n 523 }\n\n");
		ensure(tokens.want("{"));
		ensure_next(tokens, "Test1", 2);
		ensure_next(tokens, "Fixed", 2);
		ensure_next(tokens, "523", 3);
		ensure(tokens.want("}"));

		tokens = LLTemplateTokenizer("first line\nnext\t line\n\nfourth");
		ensure(tokens.want("first"));
		ensure_next(tokens, "line", 1);
		ensure_next(tokens, "next", 2);
		ensure_next(tokens, "line", 2);
		ensure(tokens.want("fourth"));
	}

	template<> template<>
	void LLMessageTemplateParserTestObject::test<2>()
		// tests tokenizer want method
	{
		// *NOTE: order matters
		LLTemplateTokenizer tokens("first line\nnext\t line\n\nfourth");
		ensure_equals("wants first token", tokens.want("first"), true);
		ensure_equals("doesn't want blar token", tokens.want("blar"), false);
		ensure_equals("wants line token", tokens.want("line"), true);
	}

	template<> template<>
	void LLMessageTemplateParserTestObject::test<3>()
		// tests tokenizer eof methods
	{
		LLTemplateTokenizer tokens("single\n\n");
		ensure_equals("is not at eof at beginning", tokens.atEOF(), false);
		ensure_equals("doesn't want eof", tokens.wantEOF(), false);
		ensure_equals("wants the first token just to consume it",
					  tokens.want("single"), true);
		ensure_equals("is not at eof in middle", tokens.atEOF(), false);
		ensure_equals("wants eof", tokens.wantEOF(), true);
		ensure_equals("is at eof at end", tokens.atEOF(), true);
	}

	template<> template<>
	void LLMessageTemplateParserTestObject::test<4>()
		// tests variable parsing method
	{
		LLTemplateTokenizer tokens(std::string("{    Test0  \n\t\n   U32 \n\n }"));
		LLMessageVariable * var = LLTemplateParser::parseVariable(tokens);

		ensure("test0 var parsed", var != 0);
		ensure_equals("name of variable", std::string(var->getName()), std::string("Test0"));
		ensure_equals("type of variable is U32", var->getType(), MVT_U32);
		ensure_equals("size of variable", var->getSize(), 4);

		delete var;

		std::string message_string("\n\t{ \t   Test1 Fixed \n 523 }\n\n");
		tokens = LLTemplateTokenizer(message_string);
		var = LLTemplateParser::parseVariable(tokens);

		ensure("test1 var parsed", var != 0);
		ensure_equals("name of variable", std::string(var->getName()), std::string("Test1"));
		ensure_equals("type of variable is Fixed", var->getType(), MVT_FIXED);
		ensure_equals("size of variable", var->getSize(), 523);

		delete var;
	
		// *NOTE: the parsers call llerrs on invalid input, so we can't really
		// test that  :-(
	}

	template<> template<>
	void LLMessageTemplateParserTestObject::test<5>()
		// tests block parsing method
	{
		LLTemplateTokenizer tokens("{ BlockA Single { VarX F32 } }");
		LLMessageBlock * block = LLTemplateParser::parseBlock(tokens);

		ensure("blockA block parsed", block != 0);
		ensure_equals("name of block", std::string(block->mName), std::string("BlockA"));
		ensure_equals("type of block is Single", block->mType, MBT_SINGLE);
		ensure_equals("total size of block", block->mTotalSize, 4);
		ensure_equals("number of block defaults to 1", block->mNumber, 1);
		ensure_equals("variable type of VarX is F32",
					  block->getVariableType(prehash("VarX")), MVT_F32);
		ensure_equals("variable size of VarX",
					  block->getVariableSize(prehash("VarX")), 4);

		delete block;

		tokens = LLTemplateTokenizer("{ Stuff Variable { Id LLUUID } }");
		block = LLTemplateParser::parseBlock(tokens);

		ensure("stuff block parsed", block != 0);
		ensure_equals("name of block", std::string(block->mName), std::string("Stuff"));
		ensure_equals("type of block is Multiple", block->mType, MBT_VARIABLE);
		ensure_equals("total size of block", block->mTotalSize, 16);
		ensure_equals("number of block defaults to 1", block->mNumber, 1);
		ensure_equals("variable type of Id is LLUUID",
					  block->getVariableType(prehash("Id")), MVT_LLUUID);
		ensure_equals("variable size of Id",
					  block->getVariableSize(prehash("Id")), 16);

		delete block;

		tokens = LLTemplateTokenizer("{ Stuff2 Multiple 45 { Shid LLVector3d } }");
		block = LLTemplateParser::parseBlock(tokens);

		ensure("stuff2 block parsed", block != 0);
		ensure_equals("name of block", std::string(block->mName), std::string("Stuff2"));
		ensure_equals("type of block is Multiple", block->mType, MBT_MULTIPLE);
		ensure_equals("total size of block", block->mTotalSize, 24);
		ensure_equals("number of blocks", block->mNumber, 45);
		ensure_equals("variable type of Shid is Vector3d",
					  block->getVariableType(prehash("Shid")), MVT_LLVector3d);
		ensure_equals("variable size of Shid",
					  block->getVariableSize(prehash("Shid")), 24);

		delete block;		
	}

	template<> template<>
	void LLMessageTemplateParserTestObject::test<6>()
		// tests message parsing method on a simple message
	{
		std::string message_skel(
			"{\n"
			"TestMessage Low 1 NotTrusted Zerocoded\n"
			"// comment \n"
			"  {\n"
			"TestBlock1      Single\n"
			"      {   Test1       U32 }\n"
			"  }\n"
			"  {\n"
			"      NeighborBlock       Multiple        4\n"
			"      {   Test0       U32 }\n"
			"      {   Test1       U32 }\n"
			"      {   Test2       U32 }\n"
			"  }\n"
			"}");
		LLTemplateTokenizer tokens(message_skel);
		LLMessageTemplate * message = LLTemplateParser::parseMessage(tokens);

		ensure("simple message parsed", message != 0);
		ensure_equals("name of message", std::string(message->mName), std::string("TestMessage"));
		ensure_equals("frequency is Low", message->mFrequency, MFT_LOW);
		ensure_equals("trust is untrusted", message->mTrust, MT_NOTRUST);
		ensure_equals("message number", message->mMessageNumber, (U32)((255 << 24) | (255 << 16) | 1));
		ensure_equals("message encoding is zerocoded", message->mEncoding, ME_ZEROCODED);
		ensure_equals("message deprecation is notdeprecated", message->mDeprecation, MD_NOTDEPRECATED);

		LLMessageBlock * block = message->getBlock(prehash("NonexistantBlock"));
		ensure("Nonexistant block does not exist", block == 0);
		
		delete message;
	}

		template<> template<>
	void LLMessageTemplateParserTestObject::test<7>()
		// tests message parsing method on a deprecated message
	{
		std::string message_skel(
			"{\n"
			"TestMessageDeprecated High 34 Trusted Unencoded Deprecated\n"
			"  {\n"
			"TestBlock2      Single\n"
			"      {   Test2       S32 }\n"
			"  }\n"
			"}");
		LLTemplateTokenizer tokens(message_skel);
		LLMessageTemplate * message = LLTemplateParser::parseMessage(tokens);
		
		ensure("deprecated message parsed", message != 0);
		ensure_equals("name of message", std::string(message->mName), std::string("TestMessageDeprecated"));
		ensure_equals("frequency is High", message->mFrequency, MFT_HIGH);
		ensure_equals("trust is trusted", message->mTrust, MT_TRUST);
		ensure_equals("message number", message->mMessageNumber, (U32)34);
		ensure_equals("message encoding is unencoded", message->mEncoding, ME_UNENCODED);
		ensure_equals("message deprecation is deprecated", message->mDeprecation, MD_DEPRECATED);

		delete message;
	}

	void LLMessageTemplateParserTestObject::test<8>()
		// tests message parsing on RezMultipleAttachmentsFromInv, a possibly-faulty message
	{
		std::string message_skel(
			"{\n\
				RezMultipleAttachmentsFromInv Low 452 NotTrusted Zerocoded\n\
				{\n\
					AgentData			Single\n\
					{	AgentID			LLUUID	}\n\
					{	SessionID		LLUUID	}\n\
				}	\n\
				{\n\
					HeaderData			Single\n\
					{	CompoundMsgID			LLUUID  }	// All messages a single \"compound msg\" must have the same id\n\
					{	TotalObjects			U8	}\n\
					{	FirstDetachAll			BOOL	}\n\
				}\n\
				{\n\
					ObjectData			Variable		// 1 to 4 of these per packet\n\
					{	ItemID					LLUUID	}\n\
					{	OwnerID					LLUUID	}\n\
					{	AttachmentPt			U8	}	// 0 for default\n\
					{	ItemFlags				U32 }\n\
					{	GroupMask				U32 }\n\
					{	EveryoneMask			U32 }\n\
					{	NextOwnerMask			U32	}\n\
					{	Name					Variable	1	}\n\
					{	Description				Variable	1	}\n\
				}\n\
			}\n\
			");
		LLTemplateTokenizer tokens(message_skel);
		LLMessageTemplate * message = LLTemplateParser::parseMessage(tokens);

		ensure("RezMultipleAttachmentsFromInv message parsed", message != 0);
		ensure_equals("name of message", message->mName, prehash("RezMultipleAttachmentsFromInv"));
		ensure_equals("frequency is low", message->mFrequency, MFT_LOW);
		ensure_equals("trust is not trusted", message->mTrust, MT_NOTRUST);
		ensure_equals("message number", message->mMessageNumber, (U32)((255 << 24) | (255 << 16) | 452));
		ensure_equals("message encoding is zerocoded", message->mEncoding, ME_ZEROCODED);

		ensure_block_attributes(
			"RMAFI", message, "AgentData", MBT_SINGLE, 1, 16+16);
		LLMessageBlock * block = message->getBlock(prehash("AgentData"));
		ensure_variable_attributes("RMAFI",
								   block, "AgentID", MVT_LLUUID, 16);
		ensure_variable_attributes("RMAFI",
								   block, "SessionID", MVT_LLUUID, 16);
		
		ensure_block_attributes(
			"RMAFI", message, "HeaderData", MBT_SINGLE, 1, 16+1+1);
		block = message->getBlock(prehash("HeaderData"));
		ensure_variable_attributes(
			"RMAFI", block, "CompoundMsgID", MVT_LLUUID, 16);
		ensure_variable_attributes(
			"RMAFI", block, "TotalObjects", MVT_U8, 1);
		ensure_variable_attributes(
			"RMAFI", block, "FirstDetachAll", MVT_BOOL, 1);


		ensure_block_attributes(
			"RMAFI", message, "ObjectData", MBT_VARIABLE, 1, -1);
		block = message->getBlock(prehash("ObjectData"));
		ensure_variable_attributes("RMAFI", block, "ItemID", MVT_LLUUID, 16);
		ensure_variable_attributes("RMAFI", block, "OwnerID", MVT_LLUUID, 16);
		ensure_variable_attributes("RMAFI", block, "AttachmentPt", MVT_U8, 1);
		ensure_variable_attributes("RMAFI", block, "ItemFlags", MVT_U32, 4);
		ensure_variable_attributes("RMAFI", block, "GroupMask", MVT_U32, 4);
		ensure_variable_attributes("RMAFI", block, "EveryoneMask", MVT_U32, 4);
		ensure_variable_attributes("RMAFI", block, "NextOwnerMask", MVT_U32, 4);
		ensure_variable_attributes("RMAFI", block, "Name", MVT_VARIABLE, 1);
		ensure_variable_attributes("RMAFI", block, "Description", MVT_VARIABLE, 1);

		delete message;
	}

	

}
