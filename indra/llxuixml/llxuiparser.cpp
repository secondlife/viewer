/** 
 * @file llxuiparser.cpp
 * @brief Utility functions for handling XUI structures in XML
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llxuiparser.h"

#include "llxmlnode.h"
#include <fstream>
#include <boost/tokenizer.hpp>

#include "lluicolor.h"

const S32 MAX_STRING_ATTRIBUTE_SIZE = 40;

//
// LLXSDWriter
//
LLXSDWriter::LLXSDWriter()
{
	registerInspectFunc<bool>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:boolean", _1, _2, _3, _4));
	registerInspectFunc<std::string>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<U8>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedByte", _1, _2, _3, _4));
	registerInspectFunc<S8>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:signedByte", _1, _2, _3, _4));
	registerInspectFunc<U16>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedShort", _1, _2, _3, _4));
	registerInspectFunc<S16>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:signedShort", _1, _2, _3, _4));
	registerInspectFunc<U32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedInt", _1, _2, _3, _4));
	registerInspectFunc<S32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:integer", _1, _2, _3, _4));
	registerInspectFunc<F32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:float", _1, _2, _3, _4));
	registerInspectFunc<F64>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:double", _1, _2, _3, _4));
	registerInspectFunc<LLColor4>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLUIColor>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLUUID>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLSD>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
}

void LLXSDWriter::writeXSD(const std::string& type_name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace)
{
	mSchemaNode = node;
	node->setName("xs:schema");
	node->createChild("attributeFormDefault", true)->setStringValue("unqualified");
	node->createChild("elementFormDefault", true)->setStringValue("qualified");
	node->createChild("targetNamespace", true)->setStringValue(xml_namespace);
	node->createChild("xmlns:xs", true)->setStringValue("http://www.w3.org/2001/XMLSchema");
	node->createChild("xmlns", true)->setStringValue(xml_namespace);

	node = node->createChild("xs:complexType", false);
	node->createChild("name", true)->setStringValue(type_name);
	node->createChild("mixed", true)->setStringValue("true");

	mAttributeNode = node;
	mElementNode = node->createChild("xs:choice", false);
	mElementNode->createChild("minOccurs", true)->setStringValue("0");
	mElementNode->createChild("maxOccurs", true)->setStringValue("unbounded");
	block.inspectBlock(*this);

	// duplicate element choices
	LLXMLNodeList children;
	mElementNode->getChildren("xs:element", children, FALSE);
	for (LLXMLNodeList::iterator child_it = children.begin(); child_it != children.end(); ++child_it)
	{
		LLXMLNodePtr child_copy = child_it->second->deepCopy();
		std::string child_name;
		child_copy->getAttributeString("name", child_name);
		child_copy->setAttributeString("name", type_name + "." + child_name);
		mElementNode->addChild(child_copy);
	}

	LLXMLNodePtr element_declaration_node = mSchemaNode->createChild("xs:element", false);
	element_declaration_node->createChild("name", true)->setStringValue(type_name);
	element_declaration_node->createChild("type", true)->setStringValue(type_name);
}

void LLXSDWriter::writeAttribute(const std::string& type, const Parser::name_stack_t& stack, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values)
{
	name_stack_t non_empty_names;
	std::string attribute_name;
	for (name_stack_t::const_iterator it = stack.begin();
		it != stack.end();
		++it)
	{
		const std::string& name = it->first;
		if (!name.empty())
		{
			non_empty_names.push_back(*it);
		}
	}

	for (name_stack_t::const_iterator it = non_empty_names.begin();
		it != non_empty_names.end();
		++it)
	{
		if (!attribute_name.empty())
		{
			attribute_name += ".";
		}
		attribute_name += it->first;
	}

	// only flag non-nested attributes as mandatory, nested attributes have variant syntax
	// that can't be properly constrained in XSD
	// e.g. <foo mandatory.value="bar"/> vs <foo><mandatory value="bar"/></foo>
	bool attribute_mandatory = min_count == 1 && max_count == 1 && non_empty_names.size() == 1;

	// don't bother supporting "Multiple" params as xml attributes
	if (max_count <= 1)
	{
		// add compound attribute to root node
		addAttributeToSchema(mAttributeNode, attribute_name, type, attribute_mandatory, possible_values);
	}

	// now generated nested elements for compound attributes
	if (non_empty_names.size() > 1 && !attribute_mandatory)
	{
		std::string element_name;

		// traverse all but last element, leaving that as an attribute name
		name_stack_t::const_iterator end_it = non_empty_names.end();
		end_it--;

		for (name_stack_t::const_iterator it = non_empty_names.begin();
			it != end_it;
			++it)
		{
			if (it != non_empty_names.begin())
			{
				element_name += ".";
			}
			element_name += it->first;
		}

		std::string short_attribute_name = non_empty_names.back().first;

		LLXMLNodePtr complex_type_node;

		// find existing element node here, starting at tail of child list
		if (mElementNode->mChildren.notNull())
		{
			for(LLXMLNodePtr element = mElementNode->mChildren->tail;
				element.notNull(); 
				element = element->mPrev)
			{
				std::string name;
				if(element->getAttributeString("name", name) && name == element_name)
				{
					complex_type_node = element->mChildren->head;
					break;
				}
			}
		}
		//create complex_type node
		//
		//<xs:element
        //    maxOccurs="1"
        //    minOccurs="0"
        //    name="name">
        //       <xs:complexType>
        //       </xs:complexType>
        //</xs:element>
		if(complex_type_node.isNull())
		{
			complex_type_node = mElementNode->createChild("xs:element", false);

			complex_type_node->createChild("minOccurs", true)->setIntValue(min_count);
			complex_type_node->createChild("maxOccurs", true)->setIntValue(max_count);
			complex_type_node->createChild("name",		true)->setStringValue(element_name);
			complex_type_node = complex_type_node->createChild("xs:complexType", false);
		}

		addAttributeToSchema(complex_type_node, short_attribute_name, type, false, possible_values);
	}
}

void LLXSDWriter::addAttributeToSchema(LLXMLNodePtr type_declaration_node, const std::string& attribute_name, const std::string& type, bool mandatory, const std::vector<std::string>* possible_values)
{
	if (!attribute_name.empty())
	{
		LLXMLNodePtr new_enum_type_node;
		if (possible_values != NULL)
		{
			// custom attribute type, for example
			//<xs:simpleType>
			 // <xs:restriction
			 //    base="xs:string">
			 //     <xs:enumeration
			 //      value="a" />
			 //     <xs:enumeration
			 //      value="b" />
			 //   </xs:restriction>
			 // </xs:simpleType>
			new_enum_type_node = new LLXMLNode("xs:simpleType", false);

			LLXMLNodePtr restriction_node = new_enum_type_node->createChild("xs:restriction", false);
			restriction_node->createChild("base", true)->setStringValue("xs:string");

			for (std::vector<std::string>::const_iterator it = possible_values->begin();
				it != possible_values->end();
				++it)
			{
				LLXMLNodePtr enum_node = restriction_node->createChild("xs:enumeration", false);
				enum_node->createChild("value", true)->setStringValue(*it);
			}
		}

		string_set_t& attributes_written = mAttributesWritten[type_declaration_node];

		string_set_t::iterator found_it = attributes_written.lower_bound(attribute_name);

		// attribute not yet declared
		if (found_it == attributes_written.end() || attributes_written.key_comp()(attribute_name, *found_it))
		{
			attributes_written.insert(found_it, attribute_name);

			LLXMLNodePtr attribute_node = type_declaration_node->createChild("xs:attribute", false);

			// attribute name
			attribute_node->createChild("name", true)->setStringValue(attribute_name);

			if (new_enum_type_node.notNull())
			{
				attribute_node->addChild(new_enum_type_node);
			}
			else
			{
				// simple attribute type
				attribute_node->createChild("type", true)->setStringValue(type);
			}

			// required or optional
			attribute_node->createChild("use", true)->setStringValue(mandatory ? "required" : "optional");
		}
		 // attribute exists...handle collision of same name attributes with potentially different types
		else
		{
			LLXMLNodePtr attribute_declaration;
			if (type_declaration_node.notNull())
			{
				for(LLXMLNodePtr node = type_declaration_node->mChildren->tail; 
					node.notNull(); 
					node = node->mPrev)
				{
					std::string name;
					if (node->getAttributeString("name", name) && name == attribute_name)
					{
						attribute_declaration = node;
						break;
					}
				}
			}

			bool new_type_is_enum = new_enum_type_node.notNull();
			bool existing_type_is_enum = !attribute_declaration->hasAttribute("type");

			// either type is enum, revert to string in collision
			// don't bother to check for enum equivalence
			if (new_type_is_enum || existing_type_is_enum)
			{
				if (attribute_declaration->hasAttribute("type"))
				{
					attribute_declaration->setAttributeString("type", "xs:string");
				}
				else
				{
					attribute_declaration->createChild("type", true)->setStringValue("xs:string");
				}
				attribute_declaration->deleteChildren("xs:simpleType");
			}
			else 
			{
				// check for collision of different standard types
				std::string existing_type;
				attribute_declaration->getAttributeString("type", existing_type);
				// if current type is not the same as the new type, revert to strnig
				if (existing_type != type)
				{
					// ...than use most general type, string
					attribute_declaration->setAttributeString("type", "string");
				}
			}
		}
	}
}

//
// LLXUIXSDWriter
//
void LLXUIXSDWriter::writeXSD(const std::string& type_name, const std::string& path, const LLInitParam::BaseBlock& block)
{
	std::string file_name(path);
	file_name += type_name + ".xsd";
	LLXMLNodePtr root_nodep = new LLXMLNode();

	LLXSDWriter::writeXSD(type_name, root_nodep, block, "http://www.lindenlab.com/xui");

	// add includes for all possible children
	const std::type_info* type = *LLWidgetTypeRegistry::instance().getValue(type_name);
	const widget_registry_t* widget_registryp = LLChildRegistryRegistry::instance().getValue(type);

	// add choices for valid children
	if (widget_registryp)
	{
		// add include declarations for all valid children
		for (widget_registry_t::Registrar::registry_map_t::const_iterator it = widget_registryp->currentRegistrar().beginItems();
		     it != widget_registryp->currentRegistrar().endItems();
		     ++it)
		{
			std::string widget_name = it->first;
			if (widget_name == type_name)
			{
				continue;
			}
			LLXMLNodePtr nodep = new LLXMLNode("xs:include", false);
			nodep->createChild("schemaLocation", true)->setStringValue(widget_name + ".xsd");
			
			// add to front of schema
			mSchemaNode->addChild(nodep, mSchemaNode);
		}

		for (widget_registry_t::Registrar::registry_map_t::const_iterator it = widget_registryp->currentRegistrar().beginItems();
			it != widget_registryp->currentRegistrar().endItems();
			++it)
		{
			std::string widget_name = it->first;
			//<xs:element name="widget_name" type="widget_name">
			LLXMLNodePtr widget_node = mElementNode->createChild("xs:element", false);
			widget_node->createChild("name", true)->setStringValue(widget_name);
			widget_node->createChild("type", true)->setStringValue(widget_name);
		}
	}

	LLFILE* xsd_file = LLFile::fopen(file_name.c_str(), "w");
	LLXMLNode::writeHeaderToFile(xsd_file);
	root_nodep->writeToFile(xsd_file);
	fclose(xsd_file);
}

//
// LLXUIParser
//
LLXUIParser::LLXUIParser()
:	mLastWriteGeneration(-1),
	mCurReadDepth(0)
{
	registerParserFuncs<bool>(boost::bind(&LLXUIParser::readBoolValue, this, _1),
								boost::bind(&LLXUIParser::writeBoolValue, this, _1, _2));
	registerParserFuncs<std::string>(boost::bind(&LLXUIParser::readStringValue, this, _1),
								boost::bind(&LLXUIParser::writeStringValue, this, _1, _2));
	registerParserFuncs<U8>(boost::bind(&LLXUIParser::readU8Value, this, _1),
								boost::bind(&LLXUIParser::writeU8Value, this, _1, _2));
	registerParserFuncs<S8>(boost::bind(&LLXUIParser::readS8Value, this, _1),
								boost::bind(&LLXUIParser::writeS8Value, this, _1, _2));
	registerParserFuncs<U16>(boost::bind(&LLXUIParser::readU16Value, this, _1),
								boost::bind(&LLXUIParser::writeU16Value, this, _1, _2));
	registerParserFuncs<S16>(boost::bind(&LLXUIParser::readS16Value, this, _1),
								boost::bind(&LLXUIParser::writeS16Value, this, _1, _2));
	registerParserFuncs<U32>(boost::bind(&LLXUIParser::readU32Value, this, _1),
								boost::bind(&LLXUIParser::writeU32Value, this, _1, _2));
	registerParserFuncs<S32>(boost::bind(&LLXUIParser::readS32Value, this, _1),
								boost::bind(&LLXUIParser::writeS32Value, this, _1, _2));
	registerParserFuncs<F32>(boost::bind(&LLXUIParser::readF32Value, this, _1),
								boost::bind(&LLXUIParser::writeF32Value, this, _1, _2));
	registerParserFuncs<F64>(boost::bind(&LLXUIParser::readF64Value, this, _1),
								boost::bind(&LLXUIParser::writeF64Value, this, _1, _2));
	registerParserFuncs<LLColor4>(boost::bind(&LLXUIParser::readColor4Value, this, _1),
								boost::bind(&LLXUIParser::writeColor4Value, this, _1, _2));
	registerParserFuncs<LLUIColor>(boost::bind(&LLXUIParser::readUIColorValue, this, _1),
								boost::bind(&LLXUIParser::writeUIColorValue, this, _1, _2));
	registerParserFuncs<LLUUID>(boost::bind(&LLXUIParser::readUUIDValue, this, _1),
								boost::bind(&LLXUIParser::writeUUIDValue, this, _1, _2));
	registerParserFuncs<LLSD>(boost::bind(&LLXUIParser::readSDValue, this, _1),
								boost::bind(&LLXUIParser::writeSDValue, this, _1, _2));
}

static LLFastTimer::DeclareTimer FTM_PARSE_XUI("XUI Parsing");

void LLXUIParser::readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, const std::string& filename, bool silent)
{
	LLFastTimer timer(FTM_PARSE_XUI);
	mNameStack.clear();
	mCurFileName = filename;
	mCurReadDepth = 0;
	setParseSilently(silent);

	if (node.isNull())
	{
		parserWarning("Invalid node");
	}
	else
	{
		readXUIImpl(node, std::string(node->getName()->mString), block);
	}
}

bool LLXUIParser::readXUIImpl(LLXMLNodePtr nodep, const std::string& scope, LLInitParam::BaseBlock& block)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool values_parsed = false;

	// submit attributes for current node
	values_parsed |= readAttributes(nodep, block);

	// treat text contents of xml node as "value" parameter
	std::string text_contents = nodep->getSanitizedValue();
	if (!text_contents.empty())
	{
		mCurReadNode = nodep;
		mNameStack.push_back(std::make_pair(std::string("value"), newParseGeneration()));
		// child nodes are not necessarily valid parameters (could be a child widget)
		// so don't complain once we've recursed
		bool silent = mCurReadDepth > 0;
		if (!block.submitValue(mNameStack, *this, true))
		{
			mNameStack.pop_back();
			block.submitValue(mNameStack, *this, silent);
		}
		else
		{
			mNameStack.pop_back();
		}
	}

	// then traverse children
	// child node must start with last name of parent node (our "scope")
	// for example: "<button><button.param nested_param1="foo"><param.nested_param2 nested_param3="bar"/></button.param></button>"
	// which equates to the following nesting:
	// button
	//     param
	//         nested_param1
	//         nested_param2
	//             nested_param3	
	mCurReadDepth++;
	for(LLXMLNodePtr childp = nodep->getFirstChild(); childp.notNull();)
	{
		std::string child_name(childp->getName()->mString);
		S32 num_tokens_pushed = 0;

		// for non "dotted" child nodes	check to see if child node maps to another widget type
		// and if not, treat as a child element of the current node
		// e.g. <button><rect left="10"/></button> will interpret <rect> as "button.rect"
		// since there is no widget named "rect"
		if (child_name.find(".") == std::string::npos) 
		{
			mNameStack.push_back(std::make_pair(child_name, newParseGeneration()));
			num_tokens_pushed++;
		}
		else
		{
			// parse out "dotted" name into individual tokens
			tokenizer name_tokens(child_name, sep);

			tokenizer::iterator name_token_it = name_tokens.begin();
			if(name_token_it == name_tokens.end()) 
			{
				childp = childp->getNextSibling();
				continue;
			}

			// check for proper nesting
			if(!scope.empty() && *name_token_it != scope)
			{
				childp = childp->getNextSibling();
				continue;
			}

			// now ignore first token
			++name_token_it; 

			// copy remaining tokens on to our running token list
			for(tokenizer::iterator token_to_push = name_token_it; token_to_push != name_tokens.end(); ++token_to_push)
			{
				mNameStack.push_back(std::make_pair(*token_to_push, newParseGeneration()));
				num_tokens_pushed++;
			}
		}

		// recurse and visit children XML nodes
		if(readXUIImpl(childp, mNameStack.empty() ? scope : mNameStack.back().first, block))
		{
			// child node successfully parsed, remove from DOM

			values_parsed = true;
			LLXMLNodePtr node_to_remove = childp;
			childp = childp->getNextSibling();

			nodep->deleteChild(node_to_remove);
		}
		else
		{
			childp = childp->getNextSibling();
		}

		while(num_tokens_pushed-- > 0)
		{
			mNameStack.pop_back();
		}
	}
	mCurReadDepth--;
	return values_parsed;
}

bool LLXUIParser::readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool any_parsed = false;

	for(LLXMLAttribList::const_iterator attribute_it = nodep->mAttributes.begin(); 
		attribute_it != nodep->mAttributes.end(); 
		++attribute_it)
	{
		S32 num_tokens_pushed = 0;
		std::string attribute_name(attribute_it->first->mString);
		mCurReadNode = attribute_it->second;

		tokenizer name_tokens(attribute_name, sep);
		// copy remaining tokens on to our running token list
		for(tokenizer::iterator token_to_push = name_tokens.begin(); token_to_push != name_tokens.end(); ++token_to_push)
		{
			mNameStack.push_back(std::make_pair(*token_to_push, newParseGeneration()));
			num_tokens_pushed++;
		}

		// child nodes are not necessarily valid attributes, so don't complain once we've recursed
		bool silent = mCurReadDepth > 0;
		any_parsed |= block.submitValue(mNameStack, *this, silent);
		
		while(num_tokens_pushed-- > 0)
		{
			mNameStack.pop_back();
		}
	}

	return any_parsed;
}

void LLXUIParser::writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock &block, const LLInitParam::BaseBlock* diff_block)
{
	mWriteRootNode = node;
	block.serializeBlock(*this, Parser::name_stack_t(), diff_block);
	mOutNodes.clear();
}

// go from a stack of names to a specific XML node
LLXMLNodePtr LLXUIParser::getNode(const name_stack_t& stack)
{
	name_stack_t name_stack;
	for (name_stack_t::const_iterator it = stack.begin();
		it != stack.end();
		++it)
	{
		if (!it->first.empty())
		{
			name_stack.push_back(*it);
		}
	}

	LLXMLNodePtr out_node = mWriteRootNode;

	name_stack_t::const_iterator next_it = name_stack.begin();
	for (name_stack_t::const_iterator it = name_stack.begin();
		it != name_stack.end();
		it = next_it)
	{
		++next_it;
		if (it->first.empty())
		{
			continue;
		}

		out_nodes_t::iterator found_it = mOutNodes.lower_bound(it->second);

		// node with this name not yet written
		if (found_it == mOutNodes.end() || mOutNodes.key_comp()(found_it->first, it->second))
		{
			// make an attribute if we are the last element on the name stack
			bool is_attribute = next_it == name_stack.end();
			LLXMLNodePtr new_node = new LLXMLNode(it->first.c_str(), is_attribute);
			out_node->addChild(new_node);
			mOutNodes.insert(found_it, std::make_pair(it->second, new_node));
			out_node = new_node;
		}
		else
		{
			out_node = found_it->second;
		}
	}

	return (out_node == mWriteRootNode ? LLXMLNodePtr(NULL) : out_node);
}


bool LLXUIParser::readBoolValue(void* val_ptr)
{
	S32 value;
	bool success = mCurReadNode->getBoolValue(1, &value);
	*((bool*)val_ptr) = (value != FALSE);
	return success;
}

bool LLXUIParser::writeBoolValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setBoolValue(*((bool*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readStringValue(void* val_ptr)
{
	*((std::string*)val_ptr) = mCurReadNode->getSanitizedValue();
	return true;
}

bool LLXUIParser::writeStringValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		const std::string* string_val = reinterpret_cast<const std::string*>(val_ptr);
		if (string_val->find('\n') != std::string::npos 
			|| string_val->size() > MAX_STRING_ATTRIBUTE_SIZE)
		{
			// don't write strings with newlines into attributes
			std::string attribute_name = node->getName()->mString;
			LLXMLNodePtr parent_node = node->mParent;
			parent_node->deleteChild(node);
			// write results in text contents of node
			if (attribute_name == "value")
			{
				// "value" is implicit, just write to parent
				node = parent_node;
			}
			else
			{
				// create a child that is not an attribute, but with same name
				node = parent_node->createChild(attribute_name.c_str(), false);
			}
		}
		node->setStringValue(*string_val);
		return true;
	}
	return false;
}

bool LLXUIParser::readU8Value(void* val_ptr)
{
	return mCurReadNode->getByteValue(1, (U8*)val_ptr);
}

bool LLXUIParser::writeU8Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS8Value(void* val_ptr)
{
	S32 value;
	if(mCurReadNode->getIntValue(1, &value))
	{
		*((S8*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS8Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU16Value(void* val_ptr)
{
	U32 value;
	if(mCurReadNode->getUnsignedValue(1, &value))
	{
		*((U16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeU16Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS16Value(void* val_ptr)
{
	S32 value;
	if(mCurReadNode->getIntValue(1, &value))
	{
		*((S16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS16Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU32Value(void* val_ptr)
{
	return mCurReadNode->getUnsignedValue(1, (U32*)val_ptr);
}

bool LLXUIParser::writeU32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS32Value(void* val_ptr)
{
	return mCurReadNode->getIntValue(1, (S32*)val_ptr);
}

bool LLXUIParser::writeS32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF32Value(void* val_ptr)
{
	return mCurReadNode->getFloatValue(1, (F32*)val_ptr);
}

bool LLXUIParser::writeF32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setFloatValue(*((F32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF64Value(void* val_ptr)
{
	return mCurReadNode->getDoubleValue(1, (F64*)val_ptr);
}

bool LLXUIParser::writeF64Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setDoubleValue(*((F64*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readColor4Value(void* val_ptr)
{
	LLColor4* colorp = (LLColor4*)val_ptr;
	if(mCurReadNode->getFloatValue(4, colorp->mV) >= 3)
	{
		return true;
	}

	return false;
}

bool LLXUIParser::writeColor4Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		LLColor4 color = *((LLColor4*)val_ptr);
		node->setFloatValue(4, color.mV);
		return true;
	}
	return false;
}

bool LLXUIParser::readUIColorValue(void* val_ptr)
{
	LLUIColor* param = (LLUIColor*)val_ptr;
	LLColor4 color;
	bool success =  mCurReadNode->getFloatValue(4, color.mV) >= 3;
	if (success)
	{
		param->set(color);
		return true;
	}
	return false;
}

bool LLXUIParser::writeUIColorValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		LLUIColor color = *((LLUIColor*)val_ptr);
		//RN: don't write out the color that is represented by a function
		// rely on param block exporting to get the reference to the color settings
		if (color.isReference()) return false;
		node->setFloatValue(4, color.get().mV);
		return true;
	}
	return false;
}

bool LLXUIParser::readUUIDValue(void* val_ptr)
{
	LLUUID temp_id;
	// LLUUID::set is destructive, so use temporary value
	if (temp_id.set(mCurReadNode->getSanitizedValue()))
	{
		*(LLUUID*)(val_ptr) = temp_id;
		return true;
	}
	return false;
}

bool LLXUIParser::writeUUIDValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setStringValue(((LLUUID*)val_ptr)->asString());
		return true;
	}
	return false;
}

bool LLXUIParser::readSDValue(void* val_ptr)
{
	*((LLSD*)val_ptr) = LLSD(mCurReadNode->getSanitizedValue());
	return true;
}

bool LLXUIParser::writeSDValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		std::string string_val = ((LLSD*)val_ptr)->asString();
		if (string_val.find('\n') != std::string::npos || string_val.size() > MAX_STRING_ATTRIBUTE_SIZE)
		{
			// don't write strings with newlines into attributes
			std::string attribute_name = node->getName()->mString;
			LLXMLNodePtr parent_node = node->mParent;
			parent_node->deleteChild(node);
			// write results in text contents of node
			if (attribute_name == "value")
			{
				// "value" is implicit, just write to parent
				node = parent_node;
			}
			else
			{
				node = parent_node->createChild(attribute_name.c_str(), false);
			}
		}

		node->setStringValue(string_val);
		return true;
	}
	return false;
}

/*virtual*/ std::string LLXUIParser::getCurrentElementName()
{
	std::string full_name;
	for (name_stack_t::iterator it = mNameStack.begin();	
		it != mNameStack.end();
		++it)
	{
		full_name += it->first + "."; // build up dotted names: "button.param.nestedparam."
	}

	return full_name;
}

void LLXUIParser::parserWarning(const std::string& message)
{
#ifdef LL_WINDOWS
	// use Visual Studo friendly formatting of output message for easy access to originating xml
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", mCurFileName.c_str(), mCurReadNode->getLineNumber(), message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserWarning(message);
#endif
}

void LLXUIParser::parserError(const std::string& message)
{
#ifdef LL_WINDOWS
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", mCurFileName.c_str(), mCurReadNode->getLineNumber(), message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserError(message);
#endif
}
