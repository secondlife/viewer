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
#include "llfasttimer.h"
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



class LLXUIParser : public LLInitParam::Parser, public LLSingleton<LLXUIParser>
{
LOG_CLASS(LLXUIParser);

protected:
	LLXUIParser();
	friend class LLSingleton<LLXUIParser>;
public:
	typedef LLInitParam::Parser::name_stack_t name_stack_t;

	/*virtual*/ std::string getCurrentElementName();
	/*virtual*/ void parserWarning(const std::string& message);
	/*virtual*/ void parserError(const std::string& message);

	void readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, const std::string& filename = LLStringUtil::null, bool silent=false);
	void writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const LLInitParam::BaseBlock* diff_block = NULL);

private:
	typedef std::list<std::pair<std::string, bool> >	token_list_t;

	bool readXUIImpl(LLXMLNodePtr node, const std::string& scope, LLInitParam::BaseBlock& block);
	bool readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block);

	//reader helper functions
	bool readBoolValue(void* val_ptr);
	bool readStringValue(void* val_ptr);
	bool readU8Value(void* val_ptr);
	bool readS8Value(void* val_ptr);
	bool readU16Value(void* val_ptr);
	bool readS16Value(void* val_ptr);
	bool readU32Value(void* val_ptr);
	bool readS32Value(void* val_ptr);
	bool readF32Value(void* val_ptr);
	bool readF64Value(void* val_ptr);
	bool readColor4Value(void* val_ptr);
	bool readUIColorValue(void* val_ptr);
	bool readUUIDValue(void* val_ptr);
	bool readSDValue(void* val_ptr);

	//writer helper functions
	bool writeBoolValue(const void* val_ptr, const name_stack_t&);
	bool writeStringValue(const void* val_ptr, const name_stack_t&);
	bool writeU8Value(const void* val_ptr, const name_stack_t&);
	bool writeS8Value(const void* val_ptr, const name_stack_t&);
	bool writeU16Value(const void* val_ptr, const name_stack_t&);
	bool writeS16Value(const void* val_ptr, const name_stack_t&);
	bool writeU32Value(const void* val_ptr, const name_stack_t&);
	bool writeS32Value(const void* val_ptr, const name_stack_t&);
	bool writeF32Value(const void* val_ptr, const name_stack_t&);
	bool writeF64Value(const void* val_ptr, const name_stack_t&);
	bool writeColor4Value(const void* val_ptr, const name_stack_t&);
	bool writeUIColorValue(const void* val_ptr, const name_stack_t&);
	bool writeUUIDValue(const void* val_ptr, const name_stack_t&);
	bool writeSDValue(const void* val_ptr, const name_stack_t&);

	LLXMLNodePtr getNode(const name_stack_t& stack);

private:
	Parser::name_stack_t			mNameStack;
	LLXMLNodePtr					mCurReadNode;
	// Root of the widget XML sub-tree, for example, "line_editor"
	LLXMLNodePtr					mWriteRootNode;
	
	typedef std::map<S32, LLXMLNodePtr>	out_nodes_t;
	out_nodes_t						mOutNodes;
	S32								mLastWriteGeneration;
	LLXMLNodePtr					mLastWrittenChild;
	S32								mCurReadDepth;
	std::string						mCurFileName;
};


#endif //LLXUIPARSER_H
