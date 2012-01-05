/** 
 * @file llxuiparser.cpp
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

#include "linden_common.h"

#include "llxuiparser.h"

#include "llxmlnode.h"

#ifdef LL_STANDALONE
#include <expat.h>
#else
#include "expat/expat.h"
#endif

#include <fstream>
#include <boost/tokenizer.hpp>
//#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic_core.hpp>

#include "lluicolor.h"

using namespace BOOST_SPIRIT_CLASSIC_NS;

const S32 MAX_STRING_ATTRIBUTE_SIZE = 40;

static 	LLInitParam::Parser::parser_read_func_map_t sXSDReadFuncs;
static 	LLInitParam::Parser::parser_write_func_map_t sXSDWriteFuncs;
static 	LLInitParam::Parser::parser_inspect_func_map_t sXSDInspectFuncs;

static 	LLInitParam::Parser::parser_read_func_map_t sSimpleXUIReadFuncs;
static 	LLInitParam::Parser::parser_write_func_map_t sSimpleXUIWriteFuncs;
static 	LLInitParam::Parser::parser_inspect_func_map_t sSimpleXUIInspectFuncs;

const char* NO_VALUE_MARKER = "no_value";

const S32 LINE_NUMBER_HERE = 0;

struct MaxOccursValues : public LLInitParam::TypeValuesHelper<U32, MaxOccursValues>
{
	static void declareValues()
	{
		declare("unbounded", U32_MAX);
	}
};

struct Occurs : public LLInitParam::Block<Occurs>
{
	Optional<U32>					minOccurs;
	Optional<U32, MaxOccursValues>	maxOccurs;

	Occurs()
	:	minOccurs("minOccurs", 0),
		maxOccurs("maxOccurs", U32_MAX)

	{}
};


typedef enum
{
	USE_REQUIRED,
	USE_OPTIONAL
} EUse;

namespace LLInitParam
{
	template<>
	struct TypeValues<EUse> : public TypeValuesHelper<EUse>
	{
		static void declareValues()
		{
			declare("required", USE_REQUIRED);
			declare("optional", USE_OPTIONAL);
		}
	};
}

struct Element;
struct Group;
struct Choice;
struct Sequence;
struct Any;

struct Attribute : public LLInitParam::Block<Attribute>
{
	Mandatory<std::string>	name;
	Mandatory<std::string>	type;
	Mandatory<EUse>			use;
	
	Attribute()
	:	name("name"),
		type("type"),
		use("use")
	{}
};

struct Any : public LLInitParam::Block<Any, Occurs>
{
	Optional<std::string> _namespace;

	Any()
	:	_namespace("namespace")
	{}
};

struct All : public LLInitParam::Block<All, Occurs>
{
	Multiple< Lazy<Element> > elements;

	All()
	:	elements("element")
	{
		maxOccurs = 1;
	}
};

struct Choice : public LLInitParam::ChoiceBlock<Choice, Occurs>
{
	Alternative< Lazy<Element> >	element;
	Alternative< Lazy<Group> >		group;
	Alternative< Lazy<Choice> >		choice;
	Alternative< Lazy<Sequence> >	sequence;
	Alternative< Lazy<Any> >		any;

	Choice()
	:	element("element"),
		group("group"),
		choice("choice"),
		sequence("sequence"),
		any("any")
	{}

};

struct Sequence : public LLInitParam::ChoiceBlock<Sequence, Occurs>
{
	Alternative< Lazy<Element> >	element;
	Alternative< Lazy<Group> >		group;
	Alternative< Lazy<Choice> >		choice;
	Alternative< Lazy<Sequence> >	sequence;
	Alternative< Lazy<Any> >		any;
};

struct GroupContents : public LLInitParam::ChoiceBlock<GroupContents, Occurs>
{
	Alternative<All>		all;
	Alternative<Choice>		choice;
	Alternative<Sequence>	sequence;

	GroupContents()
	:	all("all"),
		choice("choice"),
		sequence("sequence")
	{}
};

struct Group : public LLInitParam::Block<Group, GroupContents>
{
	Optional<std::string>	name,
							ref;

	Group()
	:	name("name"),
		ref("ref")
	{}
};

struct Restriction : public LLInitParam::Block<Restriction>
{
};

struct Extension : public LLInitParam::Block<Extension>
{
};

struct SimpleContent : public LLInitParam::ChoiceBlock<SimpleContent>
{
	Alternative<Restriction> restriction;
	Alternative<Extension> extension;

	SimpleContent()
	:	restriction("restriction"),
		extension("extension")
	{}
};

struct SimpleType : public LLInitParam::Block<SimpleType>
{
	// TODO
};

struct ComplexContent : public LLInitParam::Block<ComplexContent, SimpleContent>
{
	Optional<bool> mixed;

	ComplexContent()
	:	mixed("mixed", true)
	{}
};

struct ComplexTypeContents : public LLInitParam::ChoiceBlock<ComplexTypeContents>
{
	Alternative<SimpleContent>	simple_content;
	Alternative<ComplexContent> complex_content;
	Alternative<Group>			group;
	Alternative<All>			all;
	Alternative<Choice>			choice;
	Alternative<Sequence>		sequence;

	ComplexTypeContents()
	:	simple_content("simpleContent"),
		complex_content("complexContent"),
		group("group"),
		all("all"),
		choice("choice"),
		sequence("sequence")
	{}
};

struct ComplexType : public LLInitParam::Block<ComplexType, ComplexTypeContents>
{
	Optional<std::string>			name;
	Optional<bool>					mixed;

	Multiple<Attribute>				attribute;
	Multiple< Lazy<Element> >			elements;

	ComplexType()
	:	name("name"),
		attribute("xs:attribute"),
		elements("xs:element"),
		mixed("mixed")
	{
	}
};

struct ElementContents : public LLInitParam::ChoiceBlock<ElementContents, Occurs>
{
	Alternative<SimpleType>		simpleType;
	Alternative<ComplexType>	complexType;

	ElementContents()
	:	simpleType("simpleType"),
		complexType("complexType")
	{}
};

struct Element : public LLInitParam::Block<Element, ElementContents>
{
	Optional<std::string>	name,
							ref,
							type;

	Element()
	:	name("xs:name"),
		ref("xs:ref"),
		type("xs:type")
	{}
};

struct Schema : public LLInitParam::Block<Schema>
{
private:
	Mandatory<std::string>	targetNamespace,
							xmlns,
							xs;

public:
	Optional<std::string>	attributeFormDefault,
							elementFormDefault;

	Mandatory<Element>		root_element;
	
	void setNameSpace(const std::string& ns) {targetNamespace = ns; xmlns = ns;}

	Schema(const std::string& ns = LLStringUtil::null)
	:	attributeFormDefault("attributeFormDefault"),
		elementFormDefault("elementFormDefault"),
		xs("xmlns:xs"),
		targetNamespace("targetNamespace"),
		xmlns("xmlns"),
		root_element("xs:element")
	{
		attributeFormDefault = "unqualified";
		elementFormDefault = "qualified";
		xs = "http://www.w3.org/2001/XMLSchema";
		if (!ns.empty())
		{
			setNameSpace(ns);
		};
	}

};

//
// LLXSDWriter
//
LLXSDWriter::LLXSDWriter()
: Parser(sXSDReadFuncs, sXSDWriteFuncs, sXSDInspectFuncs)
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
	Schema schema(xml_namespace);

	schema.root_element.name = type_name;
	Choice& choice = schema.root_element.complexType.choice;

	choice.minOccurs = 0;
	choice.maxOccurs = "unbounded";

	mSchemaNode = node;
	//node->setName("xs:schema");
	//node->createChild("attributeFormDefault", true)->setStringValue("unqualified");
	//node->createChild("elementFormDefault", true)->setStringValue("qualified");
	//node->createChild("targetNamespace", true)->setStringValue(xml_namespace);
	//node->createChild("xmlns:xs", true)->setStringValue("http://www.w3.org/2001/XMLSchema");
	//node->createChild("xmlns", true)->setStringValue(xml_namespace);

	//node = node->createChild("xs:complexType", false);
	//node->createChild("name", true)->setStringValue(type_name);
	//node->createChild("mixed", true)->setStringValue("true");

	//mAttributeNode = node;
	//mElementNode = node->createChild("xs:choice", false);
	//mElementNode->createChild("minOccurs", true)->setStringValue("0");
	//mElementNode->createChild("maxOccurs", true)->setStringValue("unbounded");
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

static 	LLInitParam::Parser::parser_read_func_map_t sXUIReadFuncs;
static 	LLInitParam::Parser::parser_write_func_map_t sXUIWriteFuncs;
static 	LLInitParam::Parser::parser_inspect_func_map_t sXUIInspectFuncs;

//
// LLXUIParser
//
LLXUIParser::LLXUIParser()
:	Parser(sXUIReadFuncs, sXUIWriteFuncs, sXUIInspectFuncs),
	mCurReadDepth(0)
{
	if (sXUIReadFuncs.empty())
	{
		registerParserFuncs<LLInitParam::Flag>(readFlag, writeFlag);
		registerParserFuncs<bool>(readBoolValue, writeBoolValue);
		registerParserFuncs<std::string>(readStringValue, writeStringValue);
		registerParserFuncs<U8>(readU8Value, writeU8Value);
		registerParserFuncs<S8>(readS8Value, writeS8Value);
		registerParserFuncs<U16>(readU16Value, writeU16Value);
		registerParserFuncs<S16>(readS16Value, writeS16Value);
		registerParserFuncs<U32>(readU32Value, writeU32Value);
		registerParserFuncs<S32>(readS32Value, writeS32Value);
		registerParserFuncs<F32>(readF32Value, writeF32Value);
		registerParserFuncs<F64>(readF64Value, writeF64Value);
		registerParserFuncs<LLColor4>(readColor4Value, writeColor4Value);
		registerParserFuncs<LLUIColor>(readUIColorValue, writeUIColorValue);
		registerParserFuncs<LLUUID>(readUUIDValue, writeUUIDValue);
		registerParserFuncs<LLSD>(readSDValue, writeSDValue);
	}
}

static LLFastTimer::DeclareTimer FTM_PARSE_XUI("XUI Parsing");
const LLXMLNodePtr DUMMY_NODE = new LLXMLNode();

void LLXUIParser::readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, const std::string& filename, bool silent)
{
	LLFastTimer timer(FTM_PARSE_XUI);
	mNameStack.clear();
	mRootNodeName = node->getName()->mString;
	mCurFileName = filename;
	mCurReadDepth = 0;
	setParseSilently(silent);

	if (node.isNull())
	{
		parserWarning("Invalid node");
	}
	else
	{
		readXUIImpl(node, block);
	}
}

bool LLXUIParser::readXUIImpl(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool values_parsed = false;
	bool silent = mCurReadDepth > 0;

	if (nodep->getFirstChild().isNull() 
		&& nodep->mAttributes.empty() 
		&& nodep->getSanitizedValue().empty())
	{
		// empty node, just parse as flag
		mCurReadNode = DUMMY_NODE;
		return block.submitValue(mNameStack, *this, silent);
	}

	// submit attributes for current node
	values_parsed |= readAttributes(nodep, block);

	// treat text contents of xml node as "value" parameter
	std::string text_contents = nodep->getSanitizedValue();
	if (!text_contents.empty())
	{
		mCurReadNode = nodep;
		mNameStack.push_back(std::make_pair(std::string("value"), true));
		// child nodes are not necessarily valid parameters (could be a child widget)
		// so don't complain once we've recursed
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
			mNameStack.push_back(std::make_pair(child_name, true));
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
			if (mNameStack.empty())
			{
				if (*name_token_it != mRootNodeName)
				{
					childp = childp->getNextSibling();
					continue;
				}
			}
			else if(mNameStack.back().first != *name_token_it)
			{
				childp = childp->getNextSibling();
				continue;
			}

			// now ignore first token
			++name_token_it; 

			// copy remaining tokens on to our running token list
			for(tokenizer::iterator token_to_push = name_token_it; token_to_push != name_tokens.end(); ++token_to_push)
			{
				mNameStack.push_back(std::make_pair(*token_to_push, true));
				num_tokens_pushed++;
			}
		}

		// recurse and visit children XML nodes
		if(readXUIImpl(childp, block))
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
	bool silent = mCurReadDepth > 0;

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
			mNameStack.push_back(std::make_pair(*token_to_push, true));
			num_tokens_pushed++;
		}

		// child nodes are not necessarily valid attributes, so don't complain once we've recursed
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
	name_stack_t name_stack = Parser::name_stack_t();
	block.serializeBlock(*this, name_stack, diff_block);
	mOutNodes.clear();
}

// go from a stack of names to a specific XML node
LLXMLNodePtr LLXUIParser::getNode(name_stack_t& stack)
{
	LLXMLNodePtr out_node = mWriteRootNode;

	name_stack_t::iterator next_it = stack.begin();
	for (name_stack_t::iterator it = stack.begin();
		it != stack.end();
		it = next_it)
	{
		++next_it;
		if (it->first.empty())
		{
			it->second = false;
			continue;
		}

		out_nodes_t::iterator found_it = mOutNodes.find(it->first);

		// node with this name not yet written
		if (found_it == mOutNodes.end() || it->second)
		{
			// make an attribute if we are the last element on the name stack
			bool is_attribute = next_it == stack.end();
			LLXMLNodePtr new_node = new LLXMLNode(it->first.c_str(), is_attribute);
			out_node->addChild(new_node);
			mOutNodes[it->first] = new_node;
			out_node = new_node;
			it->second = false;
		}
		else
		{
			out_node = found_it->second;
		}
	}

	return (out_node == mWriteRootNode ? LLXMLNodePtr(NULL) : out_node);
}

bool LLXUIParser::readFlag(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode == DUMMY_NODE;
}

bool LLXUIParser::writeFlag(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	// just create node
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	return node.notNull();
}

bool LLXUIParser::readBoolValue(Parser& parser, void* val_ptr)
{
	S32 value;
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	bool success = self.mCurReadNode->getBoolValue(1, &value);
	*((bool*)val_ptr) = (value != FALSE);
	return success;
}

bool LLXUIParser::writeBoolValue(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setBoolValue(*((bool*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readStringValue(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	*((std::string*)val_ptr) = self.mCurReadNode->getSanitizedValue();
	return true;
}

bool LLXUIParser::writeStringValue(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
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

bool LLXUIParser::readU8Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode->getByteValue(1, (U8*)val_ptr);
}

bool LLXUIParser::writeU8Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS8Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	S32 value;
	if(self.mCurReadNode->getIntValue(1, &value))
	{
		*((S8*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS8Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU16Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	U32 value;
	if(self.mCurReadNode->getUnsignedValue(1, &value))
	{
		*((U16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeU16Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS16Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	S32 value;
	if(self.mCurReadNode->getIntValue(1, &value))
	{
		*((S16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS16Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU32Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode->getUnsignedValue(1, (U32*)val_ptr);
}

bool LLXUIParser::writeU32Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS32Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode->getIntValue(1, (S32*)val_ptr);
}

bool LLXUIParser::writeS32Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF32Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode->getFloatValue(1, (F32*)val_ptr);
}

bool LLXUIParser::writeF32Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setFloatValue(*((F32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF64Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	return self.mCurReadNode->getDoubleValue(1, (F64*)val_ptr);
}

bool LLXUIParser::writeF64Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setDoubleValue(*((F64*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readColor4Value(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLColor4* colorp = (LLColor4*)val_ptr;
	if(self.mCurReadNode->getFloatValue(4, colorp->mV) >= 3)
	{
		return true;
	}

	return false;
}

bool LLXUIParser::writeColor4Value(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		LLColor4 color = *((LLColor4*)val_ptr);
		node->setFloatValue(4, color.mV);
		return true;
	}
	return false;
}

bool LLXUIParser::readUIColorValue(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLUIColor* param = (LLUIColor*)val_ptr;
	LLColor4 color;
	bool success =  self.mCurReadNode->getFloatValue(4, color.mV) >= 3;
	if (success)
	{
		param->set(color);
		return true;
	}
	return false;
}

bool LLXUIParser::writeUIColorValue(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
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

bool LLXUIParser::readUUIDValue(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLUUID temp_id;
	// LLUUID::set is destructive, so use temporary value
	if (temp_id.set(self.mCurReadNode->getSanitizedValue()))
	{
		*(LLUUID*)(val_ptr) = temp_id;
		return true;
	}
	return false;
}

bool LLXUIParser::writeUUIDValue(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	LLXMLNodePtr node = self.getNode(stack);
	if (node.notNull())
	{
		node->setStringValue(((LLUUID*)val_ptr)->asString());
		return true;
	}
	return false;
}

bool LLXUIParser::readSDValue(Parser& parser, void* val_ptr)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);
	*((LLSD*)val_ptr) = LLSD(self.mCurReadNode->getSanitizedValue());
	return true;
}

bool LLXUIParser::writeSDValue(Parser& parser, const void* val_ptr, name_stack_t& stack)
{
	LLXUIParser& self = static_cast<LLXUIParser&>(parser);

	LLXMLNodePtr node = self.getNode(stack);
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


//
// LLSimpleXUIParser
//

struct ScopedFile
{
	ScopedFile( const std::string& filename, const char* accessmode )
	{
		mFile = LLFile::fopen(filename, accessmode);
	}

	~ScopedFile()
	{
		fclose(mFile);
		mFile = NULL;
	}

	S32 getRemainingBytes()
	{
		if (!isOpen()) return 0;

		S32 cur_pos = ftell(mFile);
		fseek(mFile, 0L, SEEK_END);
		S32 file_size = ftell(mFile);
		fseek(mFile, cur_pos, SEEK_SET);
		return file_size - cur_pos;
	}

	bool isOpen() { return mFile != NULL; }

	LLFILE* mFile;
};
LLSimpleXUIParser::LLSimpleXUIParser(LLSimpleXUIParser::element_start_callback_t element_cb)
:	Parser(sSimpleXUIReadFuncs, sSimpleXUIWriteFuncs, sSimpleXUIInspectFuncs),
	mCurReadDepth(0),
	mElementCB(element_cb)
{
	if (sSimpleXUIReadFuncs.empty())
	{
		registerParserFuncs<LLInitParam::Flag>(readFlag);
		registerParserFuncs<bool>(readBoolValue);
		registerParserFuncs<std::string>(readStringValue);
		registerParserFuncs<U8>(readU8Value);
		registerParserFuncs<S8>(readS8Value);
		registerParserFuncs<U16>(readU16Value);
		registerParserFuncs<S16>(readS16Value);
		registerParserFuncs<U32>(readU32Value);
		registerParserFuncs<S32>(readS32Value);
		registerParserFuncs<F32>(readF32Value);
		registerParserFuncs<F64>(readF64Value);
		registerParserFuncs<LLColor4>(readColor4Value);
		registerParserFuncs<LLUIColor>(readUIColorValue);
		registerParserFuncs<LLUUID>(readUUIDValue);
		registerParserFuncs<LLSD>(readSDValue);
	}
}

LLSimpleXUIParser::~LLSimpleXUIParser()
{
}


bool LLSimpleXUIParser::readXUI(const std::string& filename, LLInitParam::BaseBlock& block, bool silent)
{
	LLFastTimer timer(FTM_PARSE_XUI);

	mParser = XML_ParserCreate(NULL);
	XML_SetUserData(mParser, this);
	XML_SetElementHandler(			mParser,	startElementHandler, endElementHandler);
	XML_SetCharacterDataHandler(	mParser,	characterDataHandler);

	mOutputStack.push_back(std::make_pair(&block, 0));
	mNameStack.clear();
	mCurFileName = filename;
	mCurReadDepth = 0;
	setParseSilently(silent);

	ScopedFile file(filename, "rb");
	if( !file.isOpen() )
	{
		LL_WARNS("ReadXUI") << "Unable to open file " << filename << LL_ENDL;
		XML_ParserFree( mParser );
		return false;
	}

	S32 bytes_read = 0;
	
	S32 buffer_size = file.getRemainingBytes();
	void* buffer = XML_GetBuffer(mParser, buffer_size);
	if( !buffer ) 
	{
		LL_WARNS("ReadXUI") << "Unable to allocate XML buffer while reading file " << filename << LL_ENDL;
		XML_ParserFree( mParser );
		return false;
	}

	bytes_read = (S32)fread(buffer, 1, buffer_size, file.mFile);
	if( bytes_read <= 0 )
	{
		LL_WARNS("ReadXUI") << "Error while reading file  " << filename << LL_ENDL;
		XML_ParserFree( mParser );
		return false;
	}
	
	mEmptyLeafNode.push_back(false);

	if( !XML_ParseBuffer(mParser, bytes_read, TRUE ) )
	{
		LL_WARNS("ReadXUI") << "Error while parsing file  " << filename << LL_ENDL;
		XML_ParserFree( mParser );
		return false;
	}

	mEmptyLeafNode.pop_back();

	XML_ParserFree( mParser );
	return true;
}

void LLSimpleXUIParser::startElementHandler(void *userData, const char *name, const char **atts)
{
	LLSimpleXUIParser* self = reinterpret_cast<LLSimpleXUIParser*>(userData);
	self->startElement(name, atts);
}

void LLSimpleXUIParser::endElementHandler(void *userData, const char *name)
{
	LLSimpleXUIParser* self = reinterpret_cast<LLSimpleXUIParser*>(userData);
	self->endElement(name);
}

void LLSimpleXUIParser::characterDataHandler(void *userData, const char *s, int len)
{
	LLSimpleXUIParser* self = reinterpret_cast<LLSimpleXUIParser*>(userData);
	self->characterData(s, len);
}

void LLSimpleXUIParser::characterData(const char *s, int len)
{
	mTextContents += std::string(s, len);
}

void LLSimpleXUIParser::startElement(const char *name, const char **atts)
{
	processText();

	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	if (mElementCB) 
	{
		LLInitParam::BaseBlock* blockp = mElementCB(*this, name);
		if (blockp)
		{
			mOutputStack.push_back(std::make_pair(blockp, 0));
		}
	}

	mOutputStack.back().second++;
	S32 num_tokens_pushed = 0;
	std::string child_name(name);

	if (mOutputStack.back().second == 1)
	{	// root node for this block
		mScope.push_back(child_name);
	}
	else
	{	// compound attribute
		if (child_name.find(".") == std::string::npos) 
		{
			mNameStack.push_back(std::make_pair(child_name, true));
			num_tokens_pushed++;
			mScope.push_back(child_name);
		}
		else
		{
			// parse out "dotted" name into individual tokens
			tokenizer name_tokens(child_name, sep);

			tokenizer::iterator name_token_it = name_tokens.begin();
			if(name_token_it == name_tokens.end()) 
			{
				return;
			}

			// check for proper nesting
			if(!mScope.empty() && *name_token_it != mScope.back())
			{
				return;
			}

			// now ignore first token
			++name_token_it; 

			// copy remaining tokens on to our running token list
			for(tokenizer::iterator token_to_push = name_token_it; token_to_push != name_tokens.end(); ++token_to_push)
			{
				mNameStack.push_back(std::make_pair(*token_to_push, true));
				num_tokens_pushed++;
			}
			mScope.push_back(mNameStack.back().first);
		}
	}

	// parent node is not empty
	mEmptyLeafNode.back() = false;
	// we are empty if we have no attributes
	mEmptyLeafNode.push_back(atts[0] == NULL);

	mTokenSizeStack.push_back(num_tokens_pushed);
	readAttributes(atts);

}

void LLSimpleXUIParser::endElement(const char *name)
{
	bool has_text = processText();

	// no text, attributes, or children
	if (!has_text && mEmptyLeafNode.back())
	{
		// submit this as a valueless name (even though there might be text contents we haven't seen yet)
		mCurAttributeValueBegin = NO_VALUE_MARKER;
		mOutputStack.back().first->submitValue(mNameStack, *this, mParseSilently);
	}

	if (--mOutputStack.back().second == 0)
	{
		if (mOutputStack.empty())
		{
			LL_ERRS("ReadXUI") << "Parameter block output stack popped while empty." << LL_ENDL;
		}
		mOutputStack.pop_back();
	}

	S32 num_tokens_to_pop = mTokenSizeStack.back();
	mTokenSizeStack.pop_back();
	while(num_tokens_to_pop-- > 0)
	{
		mNameStack.pop_back();
	}
	mScope.pop_back();
	mEmptyLeafNode.pop_back();
}

bool LLSimpleXUIParser::readAttributes(const char **atts)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool any_parsed = false;
	for(S32 i = 0; atts[i] && atts[i+1]; i += 2 )
	{
		std::string attribute_name(atts[i]);
		mCurAttributeValueBegin = atts[i+1];
		
		S32 num_tokens_pushed = 0;
		tokenizer name_tokens(attribute_name, sep);
		// copy remaining tokens on to our running token list
		for(tokenizer::iterator token_to_push = name_tokens.begin(); token_to_push != name_tokens.end(); ++token_to_push)
		{
			mNameStack.push_back(std::make_pair(*token_to_push, true));
			num_tokens_pushed++;
		}

		// child nodes are not necessarily valid attributes, so don't complain once we've recursed
		any_parsed |= mOutputStack.back().first->submitValue(mNameStack, *this, mParseSilently);
		
		while(num_tokens_pushed-- > 0)
		{
			mNameStack.pop_back();
		}
	}
	return any_parsed;
}

bool LLSimpleXUIParser::processText()
{
	if (!mTextContents.empty())
	{
		LLStringUtil::trim(mTextContents);
		if (!mTextContents.empty())
		{
			mNameStack.push_back(std::make_pair(std::string("value"), true));
			mCurAttributeValueBegin = mTextContents.c_str();
			mOutputStack.back().first->submitValue(mNameStack, *this, mParseSilently);
			mNameStack.pop_back();
		}
		mTextContents.clear();
		return true;
	}
	return false;
}

/*virtual*/ std::string LLSimpleXUIParser::getCurrentElementName()
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

void LLSimpleXUIParser::parserWarning(const std::string& message)
{
#ifdef LL_WINDOWS
	// use Visual Studo friendly formatting of output message for easy access to originating xml
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", mCurFileName.c_str(), LINE_NUMBER_HERE, message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserWarning(message);
#endif
}

void LLSimpleXUIParser::parserError(const std::string& message)
{
#ifdef LL_WINDOWS
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", mCurFileName.c_str(), LINE_NUMBER_HERE, message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserError(message);
#endif
}

bool LLSimpleXUIParser::readFlag(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return self.mCurAttributeValueBegin == NO_VALUE_MARKER;
}

bool LLSimpleXUIParser::readBoolValue(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	if (!strcmp(self.mCurAttributeValueBegin, "true")) 
	{
		*((bool*)val_ptr) = true;
		return true;
	}
	else if (!strcmp(self.mCurAttributeValueBegin, "false"))
	{
		*((bool*)val_ptr) = false;
		return true;
	}

	return false;
}

bool LLSimpleXUIParser::readStringValue(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	*((std::string*)val_ptr) = self.mCurAttributeValueBegin;
	return true;
}

bool LLSimpleXUIParser::readU8Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, uint_p[assign_a(*(U8*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readS8Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, int_p[assign_a(*(S8*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readU16Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, uint_p[assign_a(*(U16*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readS16Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, int_p[assign_a(*(S16*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readU32Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, uint_p[assign_a(*(U32*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readS32Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, int_p[assign_a(*(S32*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readF32Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, real_p[assign_a(*(F32*)val_ptr)]).full;
}

bool LLSimpleXUIParser::readF64Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	return parse(self.mCurAttributeValueBegin, real_p[assign_a(*(F64*)val_ptr)]).full;
}
	
bool LLSimpleXUIParser::readColor4Value(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	LLColor4 value;

	if (parse(self.mCurAttributeValueBegin, real_p[assign_a(value.mV[0])] >> real_p[assign_a(value.mV[1])] >> real_p[assign_a(value.mV[2])] >> real_p[assign_a(value.mV[3])], space_p).full)
	{
		*(LLColor4*)(val_ptr) = value;
		return true;
	}
	return false;
}

bool LLSimpleXUIParser::readUIColorValue(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	LLColor4 value;
	LLUIColor* colorp = (LLUIColor*)val_ptr;

	if (parse(self.mCurAttributeValueBegin, real_p[assign_a(value.mV[0])] >> real_p[assign_a(value.mV[1])] >> real_p[assign_a(value.mV[2])] >> real_p[assign_a(value.mV[3])], space_p).full)
	{
		colorp->set(value);
		return true;
	}
	return false;
}

bool LLSimpleXUIParser::readUUIDValue(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	LLUUID temp_id;
	// LLUUID::set is destructive, so use temporary value
	if (temp_id.set(std::string(self.mCurAttributeValueBegin)))
	{
		*(LLUUID*)(val_ptr) = temp_id;
		return true;
	}
	return false;
}

bool LLSimpleXUIParser::readSDValue(Parser& parser, void* val_ptr)
{
	LLSimpleXUIParser& self = static_cast<LLSimpleXUIParser&>(parser);
	*((LLSD*)val_ptr) = LLSD(self.mCurAttributeValueBegin);
	return true;
}
