/** 
 * @file llxuiparser.h
 * @brief Utility functions for handling XUI structures in XML
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LLXUIPARSER_H
#define LLXUIPARSER_H

#include "llinitparam.h"
#include "llregistry.h"
#include "llpointer.h"

#include <boost/function.hpp>
#include <iosfwd>
#include <stack>
#include <set>



class LLView;


typedef LLPointer<class LLXMLNode> LLXMLNodePtr;


// lookup widget type by name
class LLWidgetTypeRegistry
:	public LLRegistrySingleton<std::string, const std::type_info*, LLWidgetTypeRegistry>
{};


// global static instance for registering all widget types
typedef boost::function<LLView* (LLXMLNodePtr node, LLView *parent, LLXMLNodePtr output_node)> LLWidgetCreatorFunc;

typedef LLRegistry<std::string, LLWidgetCreatorFunc> widget_registry_t;

class LLChildRegistryRegistry
: public LLRegistrySingleton<const std::type_info*, widget_registry_t, LLChildRegistryRegistry>
{};



class LLXSDWriter : public LLInitParam::Parser
{
	LOG_CLASS(LLXSDWriter);
public:
	void writeXSD(const std::string& name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace);

	/*virtual*/ std::string getCurrentElementName() { return LLStringUtil::null; }

	LLXSDWriter();

protected:
	void writeAttribute(const std::string& type, const Parser::name_stack_t&, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values);
	void addAttributeToSchema(LLXMLNodePtr nodep, const std::string& attribute_name, const std::string& type, bool mandatory, const std::vector<std::string>* possible_values);
	LLXMLNodePtr mAttributeNode;
	LLXMLNodePtr mElementNode;
	LLXMLNodePtr mSchemaNode;

	typedef std::set<std::string> string_set_t;
	typedef std::map<LLXMLNodePtr, string_set_t> attributes_map_t;
	attributes_map_t	mAttributesWritten;
};



// NOTE: DOES NOT WORK YET
// should support child widgets for XUI
class LLXUIXSDWriter : public LLXSDWriter
{
public:
	void writeXSD(const std::string& name, const std::string& path, const LLInitParam::BaseBlock& block);
};


class LLXUIParserImpl;

class LLXUIParser : public LLInitParam::Parser
{
LOG_CLASS(LLXUIParser);

public:
	LLXUIParser();
	typedef LLInitParam::Parser::name_stack_t name_stack_t;

	/*virtual*/ std::string getCurrentElementName();
	/*virtual*/ void parserWarning(const std::string& message);
	/*virtual*/ void parserError(const std::string& message);

	void readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, const std::string& filename = LLStringUtil::null, bool silent=false);
	void writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const LLInitParam::BaseBlock* diff_block = NULL);

private:
	bool readXUIImpl(LLXMLNodePtr node, LLInitParam::BaseBlock& block);
	bool readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block);

	//reader helper functions
	static bool readFlag(Parser& parser, void* val_ptr);
	static bool readBoolValue(Parser& parser, void* val_ptr);
	static bool readStringValue(Parser& parser, void* val_ptr);
	static bool readU8Value(Parser& parser, void* val_ptr);
	static bool readS8Value(Parser& parser, void* val_ptr);
	static bool readU16Value(Parser& parser, void* val_ptr);
	static bool readS16Value(Parser& parser, void* val_ptr);
	static bool readU32Value(Parser& parser, void* val_ptr);
	static bool readS32Value(Parser& parser, void* val_ptr);
	static bool readF32Value(Parser& parser, void* val_ptr);
	static bool readF64Value(Parser& parser, void* val_ptr);
	static bool readColor4Value(Parser& parser, void* val_ptr);
	static bool readUIColorValue(Parser& parser, void* val_ptr);
	static bool readUUIDValue(Parser& parser, void* val_ptr);
	static bool readSDValue(Parser& parser, void* val_ptr);

	//writer helper functions
	static bool writeFlag(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeBoolValue(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeStringValue(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeU8Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeS8Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeU16Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeS16Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeU32Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeS32Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeF32Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeF64Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeColor4Value(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeUIColorValue(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeUUIDValue(Parser& parser, const void* val_ptr, name_stack_t&);
	static bool writeSDValue(Parser& parser, const void* val_ptr, name_stack_t&);

	LLXMLNodePtr getNode(name_stack_t& stack);

private:
	Parser::name_stack_t			mNameStack;
	LLXMLNodePtr					mCurReadNode;
	// Root of the widget XML sub-tree, for example, "line_editor"
	LLXMLNodePtr					mWriteRootNode;
	
	typedef std::map<std::string, LLXMLNodePtr>	out_nodes_t;
	out_nodes_t						mOutNodes;
	LLXMLNodePtr					mLastWrittenChild;
	S32								mCurReadDepth;
	std::string						mCurFileName;
	std::string						mRootNodeName;
};

// LLSimpleXUIParser is a streamlined SAX-based XUI parser that does not support localization 
// or parsing of a tree of independent param blocks, such as child widgets.
// Use this for reading non-localized files that only need a single param block as a result.
//
// NOTE: In order to support nested block parsing, we need callbacks for start element that
// push new blocks contexts on the mScope stack.
// NOTE: To support localization without building a DOM, we need to enforce consistent 
// ordering of child elements from base file to localized diff file.  Then we can use a pair
// of coroutines to perform matching of xml nodes during parsing.  Not sure if the overhead
// of coroutines would offset the gain from SAX parsing
class LLSimpleXUIParserImpl;

class LLSimpleXUIParser : public LLInitParam::Parser
{
LOG_CLASS(LLSimpleXUIParser);
public:
	typedef LLInitParam::Parser::name_stack_t name_stack_t;
	typedef LLInitParam::BaseBlock* (*element_start_callback_t)(LLSimpleXUIParser&, const char* block_name);

	LLSimpleXUIParser(element_start_callback_t element_cb = NULL);
	virtual ~LLSimpleXUIParser();

	/*virtual*/ std::string getCurrentElementName();
	/*virtual*/ void parserWarning(const std::string& message);
	/*virtual*/ void parserError(const std::string& message);

	bool readXUI(const std::string& filename, LLInitParam::BaseBlock& block, bool silent=false);


private:
	//reader helper functions
	static bool readFlag(Parser&, void* val_ptr);
	static bool readBoolValue(Parser&, void* val_ptr);
	static bool readStringValue(Parser&, void* val_ptr);
	static bool readU8Value(Parser&, void* val_ptr);
	static bool readS8Value(Parser&, void* val_ptr);
	static bool readU16Value(Parser&, void* val_ptr);
	static bool readS16Value(Parser&, void* val_ptr);
	static bool readU32Value(Parser&, void* val_ptr);
	static bool readS32Value(Parser&, void* val_ptr);
	static bool readF32Value(Parser&, void* val_ptr);
	static bool readF64Value(Parser&, void* val_ptr);
	static bool readColor4Value(Parser&, void* val_ptr);
	static bool readUIColorValue(Parser&, void* val_ptr);
	static bool readUUIDValue(Parser&, void* val_ptr);
	static bool readSDValue(Parser&, void* val_ptr);

private:
	static void startElementHandler(void *userData, const char *name, const char **atts);
	static void endElementHandler(void *userData, const char *name);
	static void characterDataHandler(void *userData, const char *s, int len);

	void startElement(const char *name, const char **atts);
	void endElement(const char *name);
	void characterData(const char *s, int len);
	bool readAttributes(const char **atts);
	bool processText();

	Parser::name_stack_t			mNameStack;
	struct XML_ParserStruct*		mParser;
	LLXMLNodePtr					mLastWrittenChild;
	S32								mCurReadDepth;
	std::string						mCurFileName;
	std::string						mTextContents;
	const char*						mCurAttributeValueBegin;
	std::vector<S32>				mTokenSizeStack;
	std::vector<std::string>		mScope;
	std::vector<bool>				mEmptyLeafNode;
	element_start_callback_t		mElementCB;

	std::vector<std::pair<LLInitParam::BaseBlock*, S32> > mOutputStack;
};


#endif //LLXUIPARSER_H
