/** 
 * @file llxmlnode.cpp
 * @author Tom Yedwab
 * @brief LLXMLNode implementation
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include <iostream>
#include <map>

#include "llxmlnode.h"

#include "v3color.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "llquaternion.h"
#include "llstring.h"
#include "lluuid.h"
#include "lldir.h"

const S32 MAX_COLUMN_WIDTH = 80;

// static
BOOL LLXMLNode::sStripEscapedStrings = TRUE;
BOOL LLXMLNode::sStripWhitespaceValues = FALSE;

LLXMLNode::LLXMLNode() : 
	mID(""),
	mParser(NULL),
	mIsAttribute(FALSE),
	mVersionMajor(0), 
	mVersionMinor(0), 
	mLength(0), 
	mPrecision(64),
	mType(TYPE_CONTAINER),
	mEncoding(ENCODING_DEFAULT),
	mLineNumber(-1),
	mParent(NULL),
	mChildren(NULL),
	mAttributes(),
	mPrev(NULL),
	mNext(NULL),
	mName(NULL), 
	mValue(""), 
	mDefault(NULL)
{
}

LLXMLNode::LLXMLNode(const char* name, BOOL is_attribute) : 
	mID(""),
	mParser(NULL),
	mIsAttribute(is_attribute),
	mVersionMajor(0), 
	mVersionMinor(0), 
	mLength(0), 
	mPrecision(64),
	mType(TYPE_CONTAINER), 
	mEncoding(ENCODING_DEFAULT),
	mLineNumber(-1),
	mParent(NULL),
	mChildren(NULL),
	mAttributes(),
	mPrev(NULL),
	mNext(NULL),
	mValue(""), 
	mDefault(NULL)
{
    mName = gStringTable.addStringEntry(name);
}

LLXMLNode::LLXMLNode(LLStringTableEntry* name, BOOL is_attribute) : 
	mID(""),
	mParser(NULL),
	mIsAttribute(is_attribute),
	mVersionMajor(0), 
	mVersionMinor(0), 
	mLength(0), 
	mPrecision(64),
	mType(TYPE_CONTAINER), 
	mEncoding(ENCODING_DEFAULT),
	mLineNumber(-1),
	mParent(NULL),
	mChildren(NULL),
	mAttributes(),
	mPrev(NULL),
	mNext(NULL),
	mName(name),
	mValue(""), 
	mDefault(NULL)
{
}

// copy constructor (except for the children)
LLXMLNode::LLXMLNode(const LLXMLNode& rhs) : 
	mID(rhs.mID),
	mIsAttribute(rhs.mIsAttribute),
	mVersionMajor(rhs.mVersionMajor), 
	mVersionMinor(rhs.mVersionMinor), 
	mLength(rhs.mLength), 
	mPrecision(rhs.mPrecision),
	mType(rhs.mType),
	mEncoding(rhs.mEncoding),
	mLineNumber(0),
	mParser(NULL),
	mParent(NULL),
	mChildren(NULL),
	mAttributes(),
	mPrev(NULL),
	mNext(NULL),
	mName(rhs.mName), 
	mValue(rhs.mValue), 
	mDefault(rhs.mDefault)
{
}

// returns a new copy of this node and all its children
LLXMLNodePtr LLXMLNode::deepCopy()
{
	LLXMLNodePtr newnode = LLXMLNodePtr(new LLXMLNode(*this));
	if (mChildren.notNull())
	{
		for (LLXMLChildList::iterator iter = mChildren->map.begin();
			 iter != mChildren->map.end(); ++iter)	
		{
			newnode->addChild(iter->second->deepCopy());
		}
	}
	for (LLXMLAttribList::iterator iter = mAttributes.begin();
		 iter != mAttributes.end(); ++iter)
	{
		newnode->addChild(iter->second->deepCopy());
	}

	return newnode;
}

// virtual
LLXMLNode::~LLXMLNode()
{
	// Strictly speaking none of this should be required execept 'delete mChildren'...
	// Sadly, that's only true if we hadn't had reference-counted smart pointers linked
	// in three different directions. This entire class is a frightening, hard-to-maintain
	// mess.
	if (mChildren.notNull())
	{
		for (LLXMLChildList::iterator iter = mChildren->map.begin();
			 iter != mChildren->map.end(); ++iter)
		{
			LLXMLNodePtr child = iter->second;
			child->mParent = NULL;
			child->mNext = NULL;
			child->mPrev = NULL;
		}
		mChildren->map.clear();
		mChildren->head = NULL;
		mChildren->tail = NULL;
		mChildren = NULL;
	}
	for (LLXMLAttribList::iterator iter = mAttributes.begin();
		 iter != mAttributes.end(); ++iter)
	{
		LLXMLNodePtr attr = iter->second;
		attr->mParent = NULL;
		attr->mNext = NULL;
		attr->mPrev = NULL;
	}
	llassert(mParent == NULL);
	mDefault = NULL;
}

BOOL LLXMLNode::isNull()
{	
	return (mName == NULL);
}

// protected
BOOL LLXMLNode::removeChild(LLXMLNode *target_child) 
{
	if (!target_child)
	{
		return FALSE;
	}
	if (target_child->mIsAttribute)
	{
		LLXMLAttribList::iterator children_itr = mAttributes.find(target_child->mName);
		if (children_itr != mAttributes.end())
		{
			target_child->mParent = NULL;
			mAttributes.erase(children_itr);
			return TRUE;
		}
	}
	else if (mChildren.notNull())
	{
		LLXMLChildList::iterator children_itr = mChildren->map.find(target_child->mName);
		while (children_itr != mChildren->map.end())
		{
			if (target_child == children_itr->second)
			{
				if (target_child == mChildren->head)
				{
					mChildren->head = target_child->mNext;
				}
				if (target_child == mChildren->tail)
				{
					mChildren->tail = target_child->mPrev;
				}

				LLXMLNodePtr prev = target_child->mPrev;
				LLXMLNodePtr next = target_child->mNext;
				if (prev.notNull()) prev->mNext = next;
				if (next.notNull()) next->mPrev = prev;

				target_child->mPrev = NULL;
				target_child->mNext = NULL;
				target_child->mParent = NULL;
				mChildren->map.erase(children_itr);
				if (mChildren->map.empty())
				{
					mChildren = NULL;
				}
				return TRUE;
			}
			else if (children_itr->first != target_child->mName)
			{
				break;
			}
			else
			{
				++children_itr;
			}
		}
	}
	return FALSE;
}

void LLXMLNode::addChild(LLXMLNodePtr new_child, LLXMLNodePtr after_child)
{
	if (new_child->mParent != NULL)
	{
		if (new_child->mParent == this)
		{
			return;
		}
		new_child->mParent->removeChild(new_child);
	}

	new_child->mParent = this;
	if (new_child->mIsAttribute)
	{
		mAttributes.insert(std::make_pair(new_child->mName, new_child));
	}
	else
	{
		if (mChildren.isNull())
		{
			mChildren = new LLXMLChildren();
			mChildren->head = new_child;
			mChildren->tail = new_child;
		}
		mChildren->map.insert(std::make_pair(new_child->mName, new_child));

		// if after_child is specified, it damn well better be in the list of children
		// for this node. I'm not going to assert that, because it would be expensive,
		// but don't specify that parameter if you didn't get the value for it from the
		// list of children of this node!
		if (after_child.isNull())
		{
			if (mChildren->tail != new_child)
			{
				mChildren->tail->mNext = new_child;
				new_child->mPrev = mChildren->tail;
				mChildren->tail = new_child;
			}
		}
		// if after_child == parent, then put new_child at beginning
		else if (after_child == this)
		{
			// add to front of list
			new_child->mNext = mChildren->head;
			if (mChildren->head)
			{
				mChildren->head->mPrev = new_child;
				mChildren->head = new_child;
			}
			else // no children
			{
				mChildren->head = new_child;
				mChildren->tail = new_child;
			}
		}
		else
		{
			if (after_child->mNext.notNull())
			{
				// if after_child was not the last item, fix up some pointers
				after_child->mNext->mPrev = new_child;
				new_child->mNext = after_child->mNext;
			}
			new_child->mPrev = after_child;
			after_child->mNext = new_child;
			if (mChildren->tail == after_child)
			{
				mChildren->tail = new_child;
			}
		}
	}

	new_child->updateDefault();
}

// virtual 
LLXMLNodePtr LLXMLNode::createChild(const char* name, BOOL is_attribute)
{
	return createChild(gStringTable.addStringEntry(name), is_attribute);
}

// virtual 
LLXMLNodePtr LLXMLNode::createChild(LLStringTableEntry* name, BOOL is_attribute)
{
	LLXMLNode* ret = new LLXMLNode(name, is_attribute);
	ret->mID.clear();
	addChild(ret);
	return ret;
}

BOOL LLXMLNode::deleteChild(LLXMLNode *child)
{
	if (removeChild(child))
	{
		return TRUE;
	}
	return FALSE;
}

void LLXMLNode::setParent(LLXMLNodePtr new_parent)
{
	if (new_parent.notNull())
	{
		new_parent->addChild(this);
	}
	else
	{
		if (mParent != NULL)
		{
		    LLXMLNodePtr old_parent = mParent;
			mParent = NULL;
			old_parent->removeChild(this);
		}
	}
}


void LLXMLNode::updateDefault()
{
	if (mParent != NULL && !mParent->mDefault.isNull())
	{
		mDefault = NULL;

		// Find default value in parent's default tree
		if (!mParent->mDefault.isNull())
		{
			findDefault(mParent->mDefault);
		}
	}

	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator children_itr;
		LLXMLChildList::const_iterator children_end = mChildren->map.end();
		for (children_itr = mChildren->map.begin(); children_itr != children_end; ++children_itr)
		{
			LLXMLNodePtr child = (*children_itr).second;
			child->updateDefault();
		}
	}
}

void XMLCALL StartXMLNode(void *userData,
                          const XML_Char *name,
                          const XML_Char **atts)
{
	// Create a new node
	LLXMLNode *new_node_ptr = new LLXMLNode(name, FALSE);

	LLXMLNodePtr new_node = new_node_ptr;
	new_node->mID.clear();
	LLXMLNodePtr ptr_new_node = new_node;

	// Set the parent-child relationship with the current active node
	LLXMLNode* parent = (LLXMLNode *)userData;

	if (NULL == parent)
	{
		llwarns << "parent (userData) is NULL; aborting function" << llendl;
		return;
	}

	new_node_ptr->mParser = parent->mParser;
	new_node_ptr->setLineNumber(XML_GetCurrentLineNumber(*new_node_ptr->mParser));
	
	// Set the current active node to the new node
	XML_Parser *parser = parent->mParser;
	XML_SetUserData(*parser, (void *)new_node_ptr);

	// Parse attributes
	U32 pos = 0;
	while (atts[pos] != NULL)
	{
		std::string attr_name = atts[pos];
		std::string attr_value = atts[pos+1];

		// Special cases
		if ('i' == attr_name[0] && "id" == attr_name)
		{
			new_node->mID = attr_value;
		}
		else if ('v' == attr_name[0] && "version" == attr_name)
		{
			U32 version_major = 0;
			U32 version_minor = 0;
			if (sscanf(attr_value.c_str(), "%d.%d", &version_major, &version_minor) > 0)
			{
				new_node->mVersionMajor = version_major;
				new_node->mVersionMinor = version_minor;
			}
		}
		else if (('s' == attr_name[0] && "size" == attr_name) || ('l' == attr_name[0] && "length" == attr_name))
		{
			U32 length;
			if (sscanf(attr_value.c_str(), "%d", &length) > 0)
			{
				new_node->mLength = length;
			}
		}
		else if ('p' == attr_name[0] && "precision" == attr_name)
		{
			U32 precision;
			if (sscanf(attr_value.c_str(), "%d", &precision) > 0)
			{
				new_node->mPrecision = precision;
			}
		}
		else if ('t' == attr_name[0] && "type" == attr_name)
		{
			if ("boolean" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_BOOLEAN;
			}
			else if ("integer" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_INTEGER;
			}
			else if ("float" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_FLOAT;
			}
			else if ("string" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_STRING;
			}
			else if ("uuid" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_UUID;
			}
			else if ("noderef" == attr_value)
			{
				new_node->mType = LLXMLNode::TYPE_NODEREF;
			}
		}
		else if ('e' == attr_name[0] && "encoding" == attr_name)
		{
			if ("decimal" == attr_value)
			{
				new_node->mEncoding = LLXMLNode::ENCODING_DECIMAL;
			}
			else if ("hex" == attr_value)
			{
				new_node->mEncoding = LLXMLNode::ENCODING_HEX;
			}
			/*else if (attr_value == "base32")
			{
				new_node->mEncoding = LLXMLNode::ENCODING_BASE32;
			}*/
		}

		// only one attribute child per description
		LLXMLNodePtr attr_node;
		if (!new_node->getAttribute(attr_name.c_str(), attr_node, FALSE))
		{
			attr_node = new LLXMLNode(attr_name.c_str(), TRUE);
			attr_node->setLineNumber(XML_GetCurrentLineNumber(*new_node_ptr->mParser));
		}
		attr_node->setValue(attr_value);
		new_node->addChild(attr_node);

		pos += 2;
	}

	if (parent)
	{
		parent->addChild(new_node);
	}
}

void XMLCALL EndXMLNode(void *userData,
                        const XML_Char *name)
{
	// [FUGLY] Set the current active node to the current node's parent
	LLXMLNode *node = (LLXMLNode *)userData;
	XML_Parser *parser = node->mParser;
	XML_SetUserData(*parser, (void *)node->mParent);
	// SJB: total hack:
	if (LLXMLNode::sStripWhitespaceValues)
	{
		std::string value = node->getValue();
		BOOL is_empty = TRUE;
		for (std::string::size_type s = 0; s < value.length(); s++)
		{
			char c = value[s];
			if (c != ' ' && c != '\t' && c != '\n')
			{
				is_empty = FALSE;
				break;
			}
		}
		if (is_empty)
		{
			value.clear();
			node->setValue(value);
		}
	}
}

void XMLCALL XMLData(void *userData,
                     const XML_Char *s,
                     int len)
{
	LLXMLNode* current_node = (LLXMLNode *)userData;
	std::string value = current_node->getValue();
	if (LLXMLNode::sStripEscapedStrings)
	{
		if (s[0] == '\"' && s[len-1] == '\"')
		{
			// Special-case: Escaped string.
			std::string unescaped_string;
			for (S32 pos=1; pos<len-1; ++pos)
			{
				if (s[pos] == '\\' && s[pos+1] == '\\')
				{
					unescaped_string.append("\\");
					++pos;
				}
				else if (s[pos] == '\\' && s[pos+1] == '\"')
				{
					unescaped_string.append("\"");
					++pos;
				}
				else
				{
					unescaped_string.append(&s[pos], 1);
				}
			}
			value.append(unescaped_string);
			current_node->setValue(value);
			return;
		}
	}
	value.append(std::string(s, len));
	current_node->setValue(value);
}



// static 
bool LLXMLNode::updateNode(
	LLXMLNodePtr& node,
	LLXMLNodePtr& update_node)
{

	if (!node || !update_node)
	{
		llwarns << "Node invalid" << llendl;
		return FALSE;
	}

	//update the node value
	node->mValue = update_node->mValue;

	//update all attribute values
	LLXMLAttribList::const_iterator itor;

	for(itor = update_node->mAttributes.begin(); itor != update_node->mAttributes.end(); ++itor)
	{
		const LLStringTableEntry* attribNameEntry = (*itor).first;
		LLXMLNodePtr updateAttribNode = (*itor).second;

		LLXMLNodePtr attribNode;

		node->getAttribute(attribNameEntry, attribNode, 0);

		if (attribNode)
		{
			attribNode->mValue = updateAttribNode->mValue;
		}
	}

	//update all of node's children with updateNodes children that match name
	LLXMLNodePtr child = node->getFirstChild();
	LLXMLNodePtr last_child = child;
	LLXMLNodePtr updateChild;
	
	for (updateChild = update_node->getFirstChild(); updateChild.notNull(); 
		 updateChild = updateChild->getNextSibling())
	{
		while(child.notNull())
		{
			std::string nodeName;
			std::string updateName;

			updateChild->getAttributeString("name", updateName);
			child->getAttributeString("name", nodeName);		


			//if it's a combobox there's no name, but there is a value
			if (updateName.empty())
			{
				updateChild->getAttributeString("value", updateName);
				child->getAttributeString("value", nodeName);
			}

			if ((nodeName != "") && (updateName == nodeName))
			{
				updateNode(child, updateChild);
				last_child = child;
				child = child->getNextSibling();
				if (child.isNull())
				{
					child = node->getFirstChild();
				}
				break;
			}
			
			child = child->getNextSibling();
			if (child.isNull())
			{
				child = node->getFirstChild();
			}
			if (child == last_child)
			{
				break;
			}
		}
	}

	return TRUE;
}


// static 
LLXMLNodePtr LLXMLNode::replaceNode(LLXMLNodePtr node, LLXMLNodePtr update_node)
{	
	if (!node || !update_node)
	{
		llwarns << "Node invalid" << llendl;
		return node;
	}
	
	LLXMLNodePtr cloned_node = update_node->deepCopy();
	node->mParent->addChild(cloned_node, node);	// add after node
	LLXMLNodePtr parent = node->mParent;
	parent->removeChild(node);
	parent->updateDefault();
	
	return cloned_node;
}



// static
bool LLXMLNode::parseFile(const std::string& filename, LLXMLNodePtr& node, LLXMLNode* defaults_tree)
{
	// Read file
	LL_DEBUGS("XMLNode") << "parsing XML file: " << filename << LL_ENDL;
	LLFILE* fp = LLFile::fopen(filename, "rb");		/* Flawfinder: ignore */
	if (fp == NULL)
	{
		node = NULL ;
		return false;
	}
	fseek(fp, 0, SEEK_END);
	U32 length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	U8* buffer = new U8[length+1];
	size_t nread = fread(buffer, 1, length, fp);
	buffer[nread] = 0;
	fclose(fp);

	bool rv = parseBuffer(buffer, nread, node, defaults_tree);
	delete [] buffer;
	return rv;
}

// static
bool LLXMLNode::parseBuffer(
	U8* buffer,
	U32 length,
	LLXMLNodePtr& node, 
	LLXMLNode* defaults)
{
	// Init
	XML_Parser my_parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(my_parser, StartXMLNode, EndXMLNode);
	XML_SetCharacterDataHandler(my_parser, XMLData);

	// Create a root node
	LLXMLNode *file_node_ptr = new LLXMLNode("XML", FALSE);
	LLXMLNodePtr file_node = file_node_ptr;

	file_node->mParser = &my_parser;

	XML_SetUserData(my_parser, (void *)file_node_ptr);

	// Do the parsing
	if (XML_Parse(my_parser, (const char *)buffer, length, TRUE) != XML_STATUS_OK)
	{
		llwarns << "Error parsing xml error code: "
				<< XML_ErrorString(XML_GetErrorCode(my_parser))
				<< " on line " << XML_GetCurrentLineNumber(my_parser)
				<< llendl;
	}

	// Deinit
	XML_ParserFree(my_parser);

	if (!file_node->mChildren || file_node->mChildren->map.size() != 1)
	{
		llwarns << "Parse failure - wrong number of top-level nodes xml."
				<< llendl;
		node = NULL ;
		return false;
	}

	LLXMLNode *return_node = file_node->mChildren->map.begin()->second;

	return_node->setDefault(defaults);
	return_node->updateDefault();

	node = return_node;
	return true;
}

// static
bool LLXMLNode::parseStream(
	std::istream& str,
	LLXMLNodePtr& node, 
	LLXMLNode* defaults)
{
	// Init
	XML_Parser my_parser = XML_ParserCreate(NULL);
	XML_SetElementHandler(my_parser, StartXMLNode, EndXMLNode);
	XML_SetCharacterDataHandler(my_parser, XMLData);

	// Create a root node
	LLXMLNode *file_node_ptr = new LLXMLNode("XML", FALSE);
	LLXMLNodePtr file_node = file_node_ptr;

	file_node->mParser = &my_parser;

	XML_SetUserData(my_parser, (void *)file_node_ptr);

	const int BUFSIZE = 1024; 
	U8* buffer = new U8[BUFSIZE];

	while(str.good())
	{
		str.read((char*)buffer, BUFSIZE);
		int count = (int)str.gcount();
		
		if (XML_Parse(my_parser, (const char *)buffer, count, !str.good()) != XML_STATUS_OK)
		{
			llwarns << "Error parsing xml error code: "
					<< XML_ErrorString(XML_GetErrorCode(my_parser))
					<< " on lne " << XML_GetCurrentLineNumber(my_parser)
					<< llendl;
			break;
		}
	}
	
	delete [] buffer;

	// Deinit
	XML_ParserFree(my_parser);

	if (!file_node->mChildren || file_node->mChildren->map.size() != 1)
	{
		llwarns << "Parse failure - wrong number of top-level nodes xml."
				<< llendl;
		node = NULL;
		return false;
	}

	LLXMLNode *return_node = file_node->mChildren->map.begin()->second;

	return_node->setDefault(defaults);
	return_node->updateDefault();

	node = return_node;
	return true;
}


BOOL LLXMLNode::isFullyDefault()
{
	if (mDefault.isNull())
	{
		return FALSE;
	}
	BOOL has_default_value = (mValue == mDefault->mValue);
	BOOL has_default_attribute = (mIsAttribute == mDefault->mIsAttribute);
	BOOL has_default_type = mIsAttribute || (mType == mDefault->mType);
	BOOL has_default_encoding = mIsAttribute || (mEncoding == mDefault->mEncoding);
	BOOL has_default_precision = mIsAttribute || (mPrecision == mDefault->mPrecision);
	BOOL has_default_length = mIsAttribute || (mLength == mDefault->mLength);

	if (has_default_value 
		&& has_default_type 
		&& has_default_encoding 
		&& has_default_precision
		&& has_default_length
		&& has_default_attribute)
	{
		if (mChildren.notNull())
		{
			LLXMLChildList::const_iterator children_itr;
			LLXMLChildList::const_iterator children_end = mChildren->map.end();
			for (children_itr = mChildren->map.begin(); children_itr != children_end; ++children_itr)
			{
				LLXMLNodePtr child = (*children_itr).second;
				if (!child->isFullyDefault())
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	return FALSE;
}

// static
bool LLXMLNode::getLayeredXMLNode(LLXMLNodePtr& root,
								  const std::vector<std::string>& paths)
{
	if (paths.empty()) return false;

	std::string filename = paths.front();
	if (filename.empty())
	{
		return false;
	}
	
	if (!LLXMLNode::parseFile(filename, root, NULL))
	{
		llwarns << "Problem reading UI description file: " << filename << llendl;
		return false;
	}

	LLXMLNodePtr updateRoot;

	std::vector<std::string>::const_iterator itor;

	// We've already dealt with the first item, skip that one
	for (itor = paths.begin() + 1; itor != paths.end(); ++itor)
	{
		std::string layer_filename = *itor;
		if(layer_filename.empty() || layer_filename == filename)
		{
			// no localized version of this file, that's ok, keep looking
			continue;
		}

		if (!LLXMLNode::parseFile(layer_filename, updateRoot, NULL))
		{
			llwarns << "Problem reading localized UI description file: " << layer_filename << llendl;
			return false;
		}

		std::string nodeName;
		std::string updateName;

		updateRoot->getAttributeString("name", updateName);
		root->getAttributeString("name", nodeName);

		if (updateName == nodeName)
		{
			LLXMLNode::updateNode(root, updateRoot);
		}
	}

	return true;
}

// static
void LLXMLNode::writeHeaderToFile(LLFILE *out_file)
{
	fprintf(out_file, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n");
}

void LLXMLNode::writeToFile(LLFILE *out_file, const std::string& indent, bool use_type_decorations)
{
	if (isFullyDefault())
	{
		// Don't write out nodes that are an exact match to defaults
		return;
	}

	std::ostringstream ostream;
	writeToOstream(ostream, indent, use_type_decorations);
	std::string outstring = ostream.str();
	size_t written = fwrite(outstring.c_str(), 1, outstring.length(), out_file);
	if (written != outstring.length())
	{
		llwarns << "Short write" << llendl;
	}
}

void LLXMLNode::writeToOstream(std::ostream& output_stream, const std::string& indent, bool use_type_decorations)
{
	if (isFullyDefault())
	{
		// Don't write out nodes that are an exact match to defaults
		return;
	}

	BOOL has_default_type = mDefault.isNull()?FALSE:(mType == mDefault->mType);
	BOOL has_default_encoding = mDefault.isNull()?FALSE:(mEncoding == mDefault->mEncoding);
	BOOL has_default_precision = mDefault.isNull()?FALSE:(mPrecision == mDefault->mPrecision);
	BOOL has_default_length = mDefault.isNull()?FALSE:(mLength == mDefault->mLength);

	// stream the name
	output_stream << indent << "<" << mName->mString << "\n";

	if (use_type_decorations)
	{
		// ID
		if (mID != "")
		{
			output_stream << indent << " id=\"" << mID << "\"\n";
		}

		// Type
		if (!has_default_type)
		{
			switch (mType)
			{
			case TYPE_BOOLEAN:
				output_stream << indent << " type=\"boolean\"\n";
				break;
			case TYPE_INTEGER:
				output_stream << indent << " type=\"integer\"\n";
				break;
			case TYPE_FLOAT:
				output_stream << indent << " type=\"float\"\n";
				break;
			case TYPE_STRING:
				output_stream << indent << " type=\"string\"\n";
				break;
			case TYPE_UUID:
				output_stream << indent << " type=\"uuid\"\n";
				break;
			case TYPE_NODEREF:
				output_stream << indent << " type=\"noderef\"\n";
				break;
			default:
				// default on switch(enum) eliminates a warning on linux
				break;
			};
		}

		// Encoding
		if (!has_default_encoding)
		{
			switch (mEncoding)
			{
			case ENCODING_DECIMAL:
				output_stream << indent << " encoding=\"decimal\"\n";
				break;
			case ENCODING_HEX:
				output_stream << indent << " encoding=\"hex\"\n";
				break;
			/*case ENCODING_BASE32:
				output_stream << indent << " encoding=\"base32\"\n";
				break;*/
			default:
				// default on switch(enum) eliminates a warning on linux
				break;
			};
		}

		// Precision
		if (!has_default_precision && (mType == TYPE_INTEGER || mType == TYPE_FLOAT))
		{
			output_stream << indent << " precision=\"" << mPrecision << "\"\n";
		}

		// Version
		if (mVersionMajor > 0 || mVersionMinor > 0)
		{
			output_stream << indent << " version=\"" << mVersionMajor << "." << mVersionMinor << "\"\n";
		}

		// Array length
		if (!has_default_length && mLength > 0)
		{
			output_stream << indent << " length=\"" << mLength << "\"\n";
		}
	}

	{
		// Write out attributes
		LLXMLAttribList::const_iterator attr_itr;
		LLXMLAttribList::const_iterator attr_end = mAttributes.end();
		for (attr_itr = mAttributes.begin(); attr_itr != attr_end; ++attr_itr)
		{
			LLXMLNodePtr child = (*attr_itr).second;
			if (child->mDefault.isNull() || child->mDefault->mValue != child->mValue)
			{
				std::string attr = child->mName->mString;
				if (use_type_decorations
					&& (attr == "id" ||
						attr == "type" ||
						attr == "encoding" ||
						attr == "precision" ||
						attr == "version" ||
						attr == "length"))
				{
					continue; // skip built-in attributes
				}
				
				std::string attr_str = llformat(" %s=\"%s\"",
											 attr.c_str(),
											 escapeXML(child->mValue).c_str());
				output_stream << indent << attr_str << "\n";
			}
		}
	}

	// erase last \n before attaching final > or />
	output_stream.seekp(-1, std::ios::cur);

	if (mChildren.isNull() && mValue == "")
	{
		output_stream << " />\n";
		return;
	}
	else
	{
		output_stream << ">\n";
		if (mChildren.notNull())
		{
			// stream non-attributes
			std::string next_indent = indent + "    ";
			for (LLXMLNode* child = getFirstChild(); child; child = child->getNextSibling())
			{
				child->writeToOstream(output_stream, next_indent, use_type_decorations);
			}
		}
		if (!mValue.empty())
		{
			std::string contents = getTextContents();
			output_stream << indent << "    " << escapeXML(contents) << "\n";
		}
		output_stream << indent << "</" << mName->mString << ">\n";
	}
}

void LLXMLNode::findName(const std::string& name, LLXMLNodeList &results)
{
    LLStringTableEntry* name_entry = gStringTable.checkStringEntry(name);
	if (name_entry == mName)
	{
		results.insert(std::make_pair(this->mName->mString, this));
		return;
	}
	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator children_itr;
		LLXMLChildList::const_iterator children_end = mChildren->map.end();
		for (children_itr = mChildren->map.begin(); children_itr != children_end; ++children_itr)
		{
			LLXMLNodePtr child = (*children_itr).second;
			child->findName(name_entry, results);
		}
	}
}

void LLXMLNode::findName(LLStringTableEntry* name, LLXMLNodeList &results)
{
	if (name == mName)
	{
		results.insert(std::make_pair(this->mName->mString, this));
		return;
	}
	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator children_itr;
		LLXMLChildList::const_iterator children_end = mChildren->map.end();
		for (children_itr = mChildren->map.begin(); children_itr != children_end; ++children_itr)
		{
			LLXMLNodePtr child = (*children_itr).second;
			child->findName(name, results);
		}
	}
}

void LLXMLNode::findID(const std::string& id, LLXMLNodeList &results)
{
	if (id == mID)
	{
		results.insert(std::make_pair(this->mName->mString, this));
		return;
	}
	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator children_itr;
		LLXMLChildList::const_iterator children_end = mChildren->map.end();
		for (children_itr = mChildren->map.begin(); children_itr != children_end; ++children_itr)
		{
			LLXMLNodePtr child = (*children_itr).second;
			child->findID(id, results);
		}
	}
}

void LLXMLNode::scrubToTree(LLXMLNode *tree)
{
	if (!tree || tree->mChildren.isNull())
	{
		return;
	}
	if (mChildren.notNull())
	{
		std::vector<LLXMLNodePtr> to_delete_list;
		LLXMLChildList::iterator itor = mChildren->map.begin();
		while (itor != mChildren->map.end())
		{
			LLXMLNodePtr child = itor->second;
			LLXMLNodePtr child_tree = NULL;
			// Look for this child in the default's children
			bool found = false;
			LLXMLChildList::iterator itor2 = tree->mChildren->map.begin();
			while (itor2 != tree->mChildren->map.end())
			{
				if (child->mName == itor2->second->mName)
				{
					child_tree = itor2->second;
					found = true;
				}
				++itor2;
			}
			if (!found)
			{
				to_delete_list.push_back(child);
			}
			else
			{
				child->scrubToTree(child_tree);
			}
			++itor;
		}
		std::vector<LLXMLNodePtr>::iterator itor3;
		for (itor3=to_delete_list.begin(); itor3!=to_delete_list.end(); ++itor3)
		{
			(*itor3)->setParent(NULL);
		}
	}
}

bool LLXMLNode::getChild(const char* name, LLXMLNodePtr& node, BOOL use_default_if_missing)
{
    return getChild(gStringTable.checkStringEntry(name), node, use_default_if_missing);
}

bool LLXMLNode::getChild(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing)
{
	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator child_itr = mChildren->map.find(name);
		if (child_itr != mChildren->map.end())
		{
			node = (*child_itr).second;
			return true;
		}
	}
	if (use_default_if_missing && !mDefault.isNull())
	{
		return mDefault->getChild(name, node, FALSE);
	}
	node = NULL;
	return false;
}

void LLXMLNode::getChildren(const char* name, LLXMLNodeList &children, BOOL use_default_if_missing) const
{
    getChildren(gStringTable.checkStringEntry(name), children, use_default_if_missing);
}

void LLXMLNode::getChildren(const LLStringTableEntry* name, LLXMLNodeList &children, BOOL use_default_if_missing) const
{
	if (mChildren.notNull())
	{
		LLXMLChildList::const_iterator child_itr = mChildren->map.find(name);
		if (child_itr != mChildren->map.end())
		{
			LLXMLChildList::const_iterator children_end = mChildren->map.end();
			while (child_itr != children_end)
			{
				LLXMLNodePtr child = (*child_itr).second;
				if (name != child->mName)
				{
					break;
				}
				children.insert(std::make_pair(child->mName->mString, child));
				child_itr++;
			}
		}
	}
	if (children.size() == 0 && use_default_if_missing && !mDefault.isNull())
	{
		mDefault->getChildren(name, children, FALSE);
	}
}

// recursively walks the tree and returns all children at all nesting levels matching the name
void LLXMLNode::getDescendants(const LLStringTableEntry* name, LLXMLNodeList &children) const
{
	if (mChildren.notNull())
	{
		for (LLXMLChildList::const_iterator child_itr = mChildren->map.begin();
			 child_itr != mChildren->map.end(); ++child_itr)
		{
			LLXMLNodePtr child = (*child_itr).second;
			if (name == child->mName)
			{
				children.insert(std::make_pair(child->mName->mString, child));
			}
			// and check each child as well
			child->getDescendants(name, children);
		}
	}
}

bool LLXMLNode::getAttribute(const char* name, LLXMLNodePtr& node, BOOL use_default_if_missing)
{
    return getAttribute(gStringTable.checkStringEntry(name), node, use_default_if_missing);
}

bool LLXMLNode::getAttribute(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing)
{
	LLXMLAttribList::const_iterator child_itr = mAttributes.find(name);
	if (child_itr != mAttributes.end())
	{
		node = (*child_itr).second;
		return true;
	}
	if (use_default_if_missing && !mDefault.isNull())
	{
		return mDefault->getAttribute(name, node, FALSE);
	}
	
	return false;
}

bool LLXMLNode::setAttributeString(const char* attr, const std::string& value)
{
	LLStringTableEntry* name = gStringTable.checkStringEntry(attr);
	LLXMLAttribList::const_iterator child_itr = mAttributes.find(name);
	if (child_itr != mAttributes.end())
	{
		LLXMLNodePtr node = (*child_itr).second;
		node->setValue(value);
		return true;
	}
	return false;
}

BOOL LLXMLNode::hasAttribute(const char* name )
{
	LLXMLNodePtr node;
	return getAttribute(name, node);
}

// the structure of these getAttribute_ functions is ugly, but it's because the
// underlying system is based on BOOL and LLString; if we change
// so that they're based on more generic mechanisms, these will be
// simplified.
bool LLXMLNode::getAttribute_bool(const char* name, bool& value )
{
	LLXMLNodePtr node;
    if (!getAttribute(name, node))
    {
        return false;
    }
    BOOL temp;
	bool retval = node->getBoolValue(1, &temp);
    value = temp;
    return retval;
}

BOOL LLXMLNode::getAttributeBOOL(const char* name, BOOL& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getBoolValue(1, &value));
}

BOOL LLXMLNode::getAttributeU8(const char* name, U8& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getByteValue(1, &value));
}

BOOL LLXMLNode::getAttributeS8(const char* name, S8& value )
{
	LLXMLNodePtr node;
	S32 val;
	if (!(getAttribute(name, node) && node->getIntValue(1, &val)))
	{
		return false;
	}
	value = val;
	return true;
}

BOOL LLXMLNode::getAttributeU16(const char* name, U16& value )
{
	LLXMLNodePtr node;
	U32 val;
	if (!(getAttribute(name, node) && node->getUnsignedValue(1, &val)))
	{
		return false;
	}
	value = val;
	return true;
}

BOOL LLXMLNode::getAttributeS16(const char* name, S16& value )
{
	LLXMLNodePtr node;
	S32 val;
	if (!(getAttribute(name, node) && node->getIntValue(1, &val)))
	{
		return false;
	}
	value = val;
	return true;
}

BOOL LLXMLNode::getAttributeU32(const char* name, U32& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getUnsignedValue(1, &value));
}

BOOL LLXMLNode::getAttributeS32(const char* name, S32& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getIntValue(1, &value));
}

BOOL LLXMLNode::getAttributeF32(const char* name, F32& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getFloatValue(1, &value));
}

BOOL LLXMLNode::getAttributeF64(const char* name, F64& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getDoubleValue(1, &value));
}

BOOL LLXMLNode::getAttributeColor(const char* name, LLColor4& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getFloatValue(4, value.mV));
}

BOOL LLXMLNode::getAttributeColor4(const char* name, LLColor4& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getFloatValue(4, value.mV));
}

BOOL LLXMLNode::getAttributeColor4U(const char* name, LLColor4U& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getByteValue(4, value.mV));
}

BOOL LLXMLNode::getAttributeVector3(const char* name, LLVector3& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getFloatValue(3, value.mV));
}

BOOL LLXMLNode::getAttributeVector3d(const char* name, LLVector3d& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getDoubleValue(3, value.mdV));
}

BOOL LLXMLNode::getAttributeQuat(const char* name, LLQuaternion& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getFloatValue(4, value.mQ));
}

BOOL LLXMLNode::getAttributeUUID(const char* name, LLUUID& value )
{
	LLXMLNodePtr node;
	return (getAttribute(name, node) && node->getUUIDValue(1, &value));
}

BOOL LLXMLNode::getAttributeString(const char* name, std::string& value )
{
	LLXMLNodePtr node;
	if (!getAttribute(name, node))
	{
		return false;
	}
	value = node->getValue();
	return true;
}

LLXMLNodePtr LLXMLNode::getRoot()
{
	if (mParent == NULL)
	{
		return this;
	}
	return mParent->getRoot();
}

/*static */
const char *LLXMLNode::skipWhitespace(const char *str)
{
	// skip whitespace characters
	while (str[0] == ' ' || str[0] == '\t' || str[0] == '\n') ++str;
	return str;
}

/*static */
const char *LLXMLNode::skipNonWhitespace(const char *str)
{
	// skip non-whitespace characters
	while (str[0] != ' ' && str[0] != '\t' && str[0] != '\n' && str[0] != 0) ++str;
	return str;
}

/*static */
const char *LLXMLNode::parseInteger(const char *str, U64 *dest, BOOL *is_negative, U32 precision, Encoding encoding)
{
	*dest = 0;
	*is_negative = FALSE;

	str = skipWhitespace(str);

	if (str[0] == 0) return NULL;

	if (encoding == ENCODING_DECIMAL || encoding == ENCODING_DEFAULT)
	{
		if (str[0] == '+')
		{
			++str;
		}
		if (str[0] == '-')
		{
			*is_negative = TRUE;
			++str;
		}

		str = skipWhitespace(str);

		U64 ret = 0;
		while (str[0] >= '0' && str[0] <= '9')
		{
			ret *= 10;
			ret += str[0] - '0';
			++str;
		}

		if (str[0] == '.')
		{
			// If there is a fractional part, skip it
			str = skipNonWhitespace(str);
		}

		*dest = ret;
		return str;
	}
	if (encoding == ENCODING_HEX)
	{
		U64 ret = 0;
		str = skipWhitespace(str);
		for (U32 pos=0; pos<(precision/4); ++pos)
		{
			ret <<= 4;
			str = skipWhitespace(str);
			if (str[0] >= '0' && str[0] <= '9')
			{
				ret += str[0] - '0';
			} 
			else if (str[0] >= 'a' && str[0] <= 'f')
			{
				ret += str[0] - 'a' + 10;
			}
			else if (str[0] >= 'A' && str[0] <= 'F')
			{
				ret += str[0] - 'A' + 10;
			}
			else
			{
				return NULL;
			}
			++str;
		}

		*dest = ret;
		return str;
	}
	return NULL;
}

// 25 elements - decimal expansions of 1/(2^n), multiplied by 10 each iteration
const U64 float_coeff_table[] = 
	{ 5, 25, 125, 625, 3125, 
	  15625, 78125, 390625, 1953125, 9765625,
	  48828125, 244140625, 1220703125, 6103515625LL, 30517578125LL,
	  152587890625LL, 762939453125LL, 3814697265625LL, 19073486328125LL, 95367431640625LL,
	  476837158203125LL, 2384185791015625LL, 11920928955078125LL, 59604644775390625LL, 298023223876953125LL };

// 36 elements - decimal expansions of 1/(2^n) after the last 28, truncated, no multiply each iteration
const U64 float_coeff_table_2[] =
	{  149011611938476562LL,74505805969238281LL,
	   37252902984619140LL, 18626451492309570LL, 9313225746154785LL, 4656612873077392LL,
	   2328306436538696LL,  1164153218269348LL,  582076609134674LL,  291038304567337LL,
	   145519152283668LL,   72759576141834LL,    36379788070917LL,   18189894035458LL,
	   9094947017729LL,     4547473508864LL,     2273736754432LL,    1136868377216LL,
	   568434188608LL,      284217094304LL,      142108547152LL,     71054273576LL,
	   35527136788LL,       17763568394LL,       8881784197LL,       4440892098LL,
	   2220446049LL,        1110223024LL,        555111512LL,        277555756LL,
	   138777878,         69388939,          34694469,         17347234,
	   8673617,           4336808,           2168404,          1084202,
	   542101,            271050,            135525,           67762,
	};

/*static */
const char *LLXMLNode::parseFloat(const char *str, F64 *dest, U32 precision, Encoding encoding)
{
	str = skipWhitespace(str);

	if (str[0] == 0) return NULL;

	if (encoding == ENCODING_DECIMAL || encoding == ENCODING_DEFAULT)
	{
		str = skipWhitespace(str);

		if (memcmp(str, "inf", 3) == 0)
		{
			*(U64 *)dest = 0x7FF0000000000000ll;
			return str + 3;
		}
		if (memcmp(str, "-inf", 4) == 0)
		{
			*(U64 *)dest = 0xFFF0000000000000ll;
			return str + 4;
		}
		if (memcmp(str, "1.#INF", 6) == 0)
		{
			*(U64 *)dest = 0x7FF0000000000000ll;
			return str + 6;
		}
		if (memcmp(str, "-1.#INF", 7) == 0)
		{
			*(U64 *)dest = 0xFFF0000000000000ll;
			return str + 7;
		}

		F64 negative = 1.0f;
		if (str[0] == '+')
		{
			++str;
		}
		if (str[0] == '-')
		{
			negative = -1.0f;
			++str;
		}

		const char* base_str = str;
		str = skipWhitespace(str);

		// Parse the integer part of the expression
		U64 int_part = 0;
		while (str[0] >= '0' && str[0] <= '9')
		{
			int_part *= 10;
			int_part += U64(str[0] - '0');
			++str;
		}

		U64 f_part = 0;//, f_decimal = 1;
		if (str[0] == '.')
		{
			++str;
			U64 remainder = 0;
			U32 pos = 0;
			// Parse the decimal part of the expression
			while (str[0] >= '0' && str[0] <= '9' && pos < 25)
			{
				remainder = (remainder*10) + U64(str[0] - '0');
				f_part <<= 1;
				//f_decimal <<= 1;
				// Check the n'th bit
				if (remainder >= float_coeff_table[pos])
				{
					remainder -= float_coeff_table[pos];
					f_part |= 1;
				}
				++pos;
				++str;
			}
			if (pos == 25)
			{
				// Drop any excessive digits
				while (str[0] >= '0' && str[0] <= '9')
				{
					++str;
				}
			}
			else
			{
				while (pos < 25)
				{
					remainder *= 10;
					f_part <<= 1;
					//f_decimal <<= 1;
					// Check the n'th bit
					if (remainder >= float_coeff_table[pos])
					{
						remainder -= float_coeff_table[pos];
						f_part |= 1;
					}
					++pos;
				}
			}
			pos = 0;
			while (pos < 36)
			{
				f_part <<= 1;
				//f_decimal <<= 1;
				if (remainder >= float_coeff_table_2[pos])
				{
					remainder -= float_coeff_table_2[pos];
					f_part |= 1;
				}
				++pos;
			}
		}
		
		F64 ret = F64(int_part) + (F64(f_part)/F64(1LL<<61));

		F64 exponent = 1.f;
		if (str[0] == 'e')
		{
			// Scientific notation!
			++str;
			U64 exp;
			BOOL is_negative;
			str = parseInteger(str, &exp, &is_negative, 64, ENCODING_DECIMAL);
			if (str == NULL)
			{
				exp = 1;
			}
			F64 exp_d = F64(exp) * (is_negative?-1:1);
			exponent = pow(10.0, exp_d);
		}

		if (str == base_str)
		{
			// no digits parsed
			return NULL;
		}
		else
		{
			*dest = ret*negative*exponent;
			return str;
		}
	}
	if (encoding == ENCODING_HEX)
	{
		U64 bytes_dest;
		BOOL is_negative;
		str = parseInteger(str, (U64 *)&bytes_dest, &is_negative, precision, ENCODING_HEX);
		// Upcast to F64
		switch (precision)
		{
		case 32:
			{
				U32 short_dest = (U32)bytes_dest;
				F32 ret_val = *(F32 *)&short_dest;
				*dest = ret_val;
			}
			break;
		case 64:
			*dest = *(F64 *)&bytes_dest;
			break;
		default:
			return NULL;
		}
		return str;
	}
	return NULL;
}

U32 LLXMLNode::getBoolValue(U32 expected_length, BOOL *array)
{
	llassert(array);

	// Check type - accept booleans or strings
	if (mType != TYPE_BOOLEAN && mType != TYPE_STRING && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	std::string *str_array = new std::string[expected_length];

	U32 length = getStringValue(expected_length, str_array);

	U32 ret_length = 0;
	for (U32 i=0; i<length; ++i)
	{
		LLStringUtil::toLower(str_array[i]);
		if (str_array[i] == "false")
		{
			array[ret_length++] = FALSE;
		}
		else if (str_array[i] == "true")
		{
			array[ret_length++] = TRUE;
		}
	}

	delete[] str_array;

#if LL_DEBUG
	if (ret_length != expected_length)
	{
		lldebugs << "LLXMLNode::getBoolValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << ret_length << llendl;
	}
#endif
	return ret_length;
}

U32 LLXMLNode::getByteValue(U32 expected_length, U8 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept bytes or integers (below 256 only)
	if (mType != TYPE_INTEGER 
		&& mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getByteValue asked for " << expected_length 
			<< " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i;
	for (i=0; i<expected_length; ++i)
	{
		U64 value;
		BOOL is_negative;
		value_string = parseInteger(value_string, &value, &is_negative, 8, encoding);
		if (value_string == NULL)
		{
			break;
		}
		if (value > 255 || is_negative)
		{
			llwarns << "getByteValue: Value outside of valid range." << llendl;
			break;
		}
		array[i] = U8(value);
	}
#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getByteValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif
	return i;
}

U32 LLXMLNode::getIntValue(U32 expected_length, S32 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept bytes or integers
	if (mType != TYPE_INTEGER && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getIntValue asked for " << expected_length 
			<< " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i = 0;
	for (i=0; i<expected_length; ++i)
	{
		U64 value;
		BOOL is_negative;
		value_string = parseInteger(value_string, &value, &is_negative, 32, encoding);
		if (value_string == NULL)
		{
			break;
		}
		if (value > 0x7fffffff)
		{
			llwarns << "getIntValue: Value outside of valid range." << llendl;
			break;
		}
		array[i] = S32(value) * (is_negative?-1:1);
	}

#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getIntValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif
	return i;
}

U32 LLXMLNode::getUnsignedValue(U32 expected_length, U32 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept bytes or integers
	if (mType != TYPE_INTEGER && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getUnsignedValue asked for " << expected_length 
			<< " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i = 0;
	// Int type
	for (i=0; i<expected_length; ++i)
	{
		U64 value;
		BOOL is_negative;
		value_string = parseInteger(value_string, &value, &is_negative, 32, encoding);
		if (value_string == NULL)
		{
			break;
		}
		if (is_negative || value > 0xffffffff)
		{
			llwarns << "getUnsignedValue: Value outside of valid range." << llendl;
			break;
		}
		array[i] = U32(value);
	}

#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getUnsignedValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif

	return i;
}

U32 LLXMLNode::getLongValue(U32 expected_length, U64 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept bytes or integers
	if (mType != TYPE_INTEGER && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getLongValue asked for " << expected_length << " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i = 0;
	// Int type
	for (i=0; i<expected_length; ++i)
	{
		U64 value;
		BOOL is_negative;
		value_string = parseInteger(value_string, &value, &is_negative, 64, encoding);
		if (value_string == NULL)
		{
			break;
		}
		if (is_negative)
		{
			llwarns << "getLongValue: Value outside of valid range." << llendl;
			break;
		}
		array[i] = value;
	}

#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getLongValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif

	return i;
}

U32 LLXMLNode::getFloatValue(U32 expected_length, F32 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept only floats or doubles
	if (mType != TYPE_FLOAT && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getFloatValue asked for " << expected_length << " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i;
	for (i=0; i<expected_length; ++i)
	{
		F64 value;
		value_string = parseFloat(value_string, &value, 32, encoding);
		if (value_string == NULL)
		{
			break;
		}
		array[i] = F32(value);
	}
#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getFloatValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif
	return i;
}

U32 LLXMLNode::getDoubleValue(U32 expected_length, F64 *array, Encoding encoding)
{
	llassert(array);

	// Check type - accept only floats or doubles
	if (mType != TYPE_FLOAT && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getDoubleValue asked for " << expected_length << " elements, while node has " << mLength << llendl;
		return 0;
	}

	if (encoding == ENCODING_DEFAULT)
	{
		encoding = mEncoding;
	}

	const char *value_string = mValue.c_str();

	U32 i;
	for (i=0; i<expected_length; ++i)
	{
		F64 value;
		value_string = parseFloat(value_string, &value, 64, encoding);
		if (value_string == NULL)
		{
			break;
		}
		array[i] = value;
	}
#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getDoubleValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif
	return i;
}

U32 LLXMLNode::getStringValue(U32 expected_length, std::string *array)
{
	llassert(array);

	// Can always return any value as a string

	if (mLength > 0 && mLength != expected_length)
	{
		llwarns << "XMLNode::getStringValue asked for " << expected_length << " elements, while node has " << mLength << llendl;
		return 0;
	}

	U32 num_returned_strings = 0;

	// Array of strings is whitespace-separated
	const std::string sep(" \n\t");
	
	std::string::size_type n = 0;
	std::string::size_type m = 0;
	while(1)
	{
		if (num_returned_strings >= expected_length)
		{
			break;
		}
		n = mValue.find_first_not_of(sep, m);
		m = mValue.find_first_of(sep, n);
		if (m == std::string::npos)
		{
			break;
		}
		array[num_returned_strings++] = mValue.substr(n,m-n);
	}
	if (n != std::string::npos && num_returned_strings < expected_length)
	{
		array[num_returned_strings++] = mValue.substr(n);
	}
#if LL_DEBUG
	if (num_returned_strings != expected_length)
	{
		lldebugs << "LLXMLNode::getStringValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << num_returned_strings << llendl;
	}
#endif

	return num_returned_strings;
}

U32 LLXMLNode::getUUIDValue(U32 expected_length, LLUUID *array)
{
	llassert(array);

	// Check type
	if (mType != TYPE_UUID && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	const char *value_string = mValue.c_str();

	U32 i;
	for (i=0; i<expected_length; ++i)
	{
		LLUUID uuid_value;
		value_string = skipWhitespace(value_string);

		if (strlen(value_string) < (UUID_STR_LENGTH-1))		/* Flawfinder: ignore */
		{
			break;
		}
		char uuid_string[UUID_STR_LENGTH];		/* Flawfinder: ignore */
		memcpy(uuid_string, value_string, (UUID_STR_LENGTH-1));		/* Flawfinder: ignore */
		uuid_string[(UUID_STR_LENGTH-1)] = 0;

		if (!LLUUID::parseUUID(std::string(uuid_string), &uuid_value))
		{
			break;
		}
		value_string = &value_string[(UUID_STR_LENGTH-1)];
		array[i] = uuid_value;
	}
#if LL_DEBUG
	if (i != expected_length)
	{
		lldebugs << "LLXMLNode::getUUIDValue() failed for node named '" 
			<< mName->mString << "' -- expected " << expected_length << " but "
			<< "only found " << i << llendl;
	}
#endif
	return i;
}

U32 LLXMLNode::getNodeRefValue(U32 expected_length, LLXMLNode **array)
{
	llassert(array);

	// Check type
	if (mType != TYPE_NODEREF && mType != TYPE_UNKNOWN)
	{
		return 0;
	}

	std::string *string_array = new std::string[expected_length];

	U32 num_strings = getStringValue(expected_length, string_array);

	U32 num_returned_refs = 0;

	LLXMLNodePtr root = getRoot();
	for (U32 strnum=0; strnum<num_strings; ++strnum)
	{
		LLXMLNodeList node_list;
		root->findID(string_array[strnum], node_list);
		if (node_list.empty())
		{
			llwarns << "XML: Could not find node ID: " << string_array[strnum] << llendl;
		}
		else if (node_list.size() > 1)
		{
			llwarns << "XML: Node ID not unique: " << string_array[strnum] << llendl;
		}
		else
		{
			LLXMLNodeList::const_iterator list_itr = node_list.begin(); 
			if (list_itr != node_list.end())
			{
				LLXMLNode* child = (*list_itr).second;

				array[num_returned_refs++] = child;
			}
		}
	}

	delete[] string_array;

	return num_returned_refs;
}

void LLXMLNode::setBoolValue(U32 length, const BOOL *array)
{
	if (length == 0) return;

	std::string new_value;
	for (U32 pos=0; pos<length; ++pos)
	{
		if (pos > 0)
		{
			new_value = llformat("%s %s", new_value.c_str(), array[pos]?"true":"false");
		}
		else
		{
			new_value = array[pos]?"true":"false";
		}
	}

	mValue = new_value;
	mEncoding = ENCODING_DEFAULT;
	mLength = length;
	mType = TYPE_BOOLEAN;
}

void LLXMLNode::setByteValue(U32 length, const U8* const array, Encoding encoding)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(llformat(" %u", array[pos]));
			}
			else
			{
				new_value = llformat("%u", array[pos]);
			}
		}
	}
	if (encoding == ENCODING_HEX)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0 && pos % 16 == 0)
			{
				new_value.append(llformat(" %02X", array[pos]));
			}
			else
			{
				new_value.append(llformat("%02X", array[pos]));
			}
		}
	}
	// TODO -- Handle Base32

	mValue = new_value;
	mEncoding = encoding;
	mLength = length;
	mType = TYPE_INTEGER;
	mPrecision = 8;
}


void LLXMLNode::setIntValue(U32 length, const S32 *array, Encoding encoding)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(llformat(" %d", array[pos]));
			}
			else
			{
				new_value = llformat("%d", array[pos]);
			}
		}
		mValue = new_value;
	}
	else if (encoding == ENCODING_HEX)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0 && pos % 16 == 0)
			{
				new_value.append(llformat(" %08X", ((U32 *)array)[pos]));
			}
			else
			{
				new_value.append(llformat("%08X", ((U32 *)array)[pos]));
			}
		}
		mValue = new_value;
	}
	else
	{
		mValue = new_value;
	}
	// TODO -- Handle Base32

	mEncoding = encoding;
	mLength = length;
	mType = TYPE_INTEGER;
	mPrecision = 32;
}

void LLXMLNode::setUnsignedValue(U32 length, const U32* array, Encoding encoding)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(llformat(" %u", array[pos]));
			}
			else
			{
				new_value = llformat("%u", array[pos]);
			}
		}
	}
	if (encoding == ENCODING_HEX)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0 && pos % 16 == 0)
			{
				new_value.append(llformat(" %08X", array[pos]));
			}
			else
			{
				new_value.append(llformat("%08X", array[pos]));
			}
		}
		mValue = new_value;
	}
	// TODO -- Handle Base32

	mValue = new_value;
	mEncoding = encoding;
	mLength = length;
	mType = TYPE_INTEGER;
	mPrecision = 32;
}

#if LL_WINDOWS
#define PU64 "I64u"
#else
#define PU64 "llu"
#endif

void LLXMLNode::setLongValue(U32 length, const U64* array, Encoding encoding)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(llformat(" %" PU64, array[pos]));
			}
			else
			{
				new_value = llformat("%" PU64, array[pos]);
			}
		}
		mValue = new_value;
	}
	if (encoding == ENCODING_HEX)
	{
		for (U32 pos=0; pos<length; ++pos)
		{
			U32 upper_32 = U32(array[pos]>>32);
			U32 lower_32 = U32(array[pos]&0xffffffff);
			if (pos > 0 && pos % 8 == 0)
			{
				new_value.append(llformat(" %08X%08X", upper_32, lower_32));
			}
			else
			{
				new_value.append(llformat("%08X%08X", upper_32, lower_32));
			}
		}
		mValue = new_value;
	}
	else
	{
		mValue = new_value;
	}
	// TODO -- Handle Base32

	mEncoding = encoding;
	mLength = length;
	mType = TYPE_INTEGER;
	mPrecision = 64;
}

void LLXMLNode::setFloatValue(U32 length, const F32 *array, Encoding encoding, U32 precision)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		std::string format_string;
		if (precision > 0)
		{
			if (precision > 25)
			{
				precision = 25;
			}
			format_string = llformat( "%%.%dg", precision);
		}
		else
		{
			format_string = llformat( "%%g");
		}

		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(" ");
				new_value.append(llformat(format_string.c_str(), array[pos]));
			}
			else
			{
				new_value.assign(llformat(format_string.c_str(), array[pos]));
			}
		}
		mValue = new_value;
	}
	else if (encoding == ENCODING_HEX)
	{
		U32 *byte_array = (U32 *)array;
		setUnsignedValue(length, byte_array, ENCODING_HEX);
	}
	else
	{
		mValue = new_value;
	}

	mEncoding = encoding;
	mLength = length;
	mType = TYPE_FLOAT;
	mPrecision = 32;
}

void LLXMLNode::setDoubleValue(U32 length, const F64 *array, Encoding encoding, U32 precision)
{
	if (length == 0) return;

	std::string new_value;
	if (encoding == ENCODING_DEFAULT || encoding == ENCODING_DECIMAL)
	{
		std::string format_string;
		if (precision > 0)
		{
			if (precision > 25)
			{
				precision = 25;
			}
			format_string = llformat( "%%.%dg", precision);
		}
		else
		{
			format_string = llformat( "%%g");
		}
		for (U32 pos=0; pos<length; ++pos)
		{
			if (pos > 0)
			{
				new_value.append(" ");
				new_value.append(llformat(format_string.c_str(), array[pos]));
			}
			else
			{
				new_value.assign(llformat(format_string.c_str(), array[pos]));
			}
		}
		mValue = new_value;
	}
	if (encoding == ENCODING_HEX)
	{
		U64 *byte_array = (U64 *)array;
		setLongValue(length, byte_array, ENCODING_HEX);
	}
	else
	{
		mValue = new_value;
	}
	// TODO -- Handle Base32

	mEncoding = encoding;
	mLength = length;
	mType = TYPE_FLOAT;
	mPrecision = 64;
}

// static
std::string LLXMLNode::escapeXML(const std::string& xml)
{
	std::string out;
	for (std::string::size_type i = 0; i < xml.size(); ++i)
	{
		char c = xml[i];
		switch(c)
		{
		case '"':	out.append("&quot;");	break;
		case '\'':	out.append("&apos;");	break;
		case '&':	out.append("&amp;");	break;
		case '<':	out.append("&lt;");		break;
		case '>':	out.append("&gt;");		break;
		default:	out.push_back(c);		break;
		}
	}
	return out;
}

void LLXMLNode::setStringValue(U32 length, const std::string *strings)
{
	if (length == 0) return;

	std::string new_value;
	for (U32 pos=0; pos<length; ++pos)
	{
		// *NOTE: Do not escape strings here - do it on output
		new_value.append( strings[pos] );
		if (pos < length-1) new_value.append(" ");
	}

	mValue = new_value;
	mEncoding = ENCODING_DEFAULT;
	mLength = length;
	mType = TYPE_STRING;
}

void LLXMLNode::setUUIDValue(U32 length, const LLUUID *array)
{
	if (length == 0) return;

	std::string new_value;
	for (U32 pos=0; pos<length; ++pos)
	{
		new_value.append(array[pos].asString());
		if (pos < length-1) new_value.append(" ");
	}

	mValue = new_value;
	mEncoding = ENCODING_DEFAULT;
	mLength = length;
	mType = TYPE_UUID;
}

void LLXMLNode::setNodeRefValue(U32 length, const LLXMLNode **array)
{
	if (length == 0) return;

	std::string new_value;
	for (U32 pos=0; pos<length; ++pos)
	{
		if (array[pos]->mID != "")
		{
			new_value.append(array[pos]->mID);
		}
		else
		{
			new_value.append("(null)");
		}
		if (pos < length-1) new_value.append(" ");
	}

	mValue = new_value;
	mEncoding = ENCODING_DEFAULT;
	mLength = length;
	mType = TYPE_NODEREF;
}

void LLXMLNode::setValue(const std::string& value)
{
	if (TYPE_CONTAINER == mType)
	{
		mType = TYPE_UNKNOWN;
	}
	mValue = value;
}

void LLXMLNode::setDefault(LLXMLNode *default_node)
{
	mDefault = default_node;
}

void LLXMLNode::findDefault(LLXMLNode *defaults_list)
{
	if (defaults_list)
	{
		LLXMLNodeList children;
		defaults_list->getChildren(mName->mString, children);

		LLXMLNodeList::const_iterator children_itr;
		LLXMLNodeList::const_iterator children_end = children.end();
		for (children_itr = children.begin(); children_itr != children_end; ++children_itr)
		{
			LLXMLNode* child = (*children_itr).second;
			if (child->mVersionMajor == mVersionMajor &&
				child->mVersionMinor == mVersionMinor)
			{
				mDefault = child;
				return;
			}
		}
	}
	mDefault = NULL;
}

BOOL LLXMLNode::deleteChildren(const std::string& name)
{
	U32 removed_count = 0;
	LLXMLNodeList node_list;
	findName(name, node_list);
	if (!node_list.empty())
	{
		// TODO -- use multimap::find()
		// TODO -- need to watch out for invalid iterators
		LLXMLNodeList::iterator children_itr;
		for (children_itr = node_list.begin(); children_itr != node_list.end(); ++children_itr)
		{ 
			LLXMLNode* child = (*children_itr).second;
			if (deleteChild(child))
			{
				removed_count++;
			}
		}
	}
	return removed_count > 0 ? TRUE : FALSE;
}

BOOL LLXMLNode::deleteChildren(LLStringTableEntry* name)
{
	U32 removed_count = 0;
	LLXMLNodeList node_list;
	findName(name, node_list);
	if (!node_list.empty())
	{
		// TODO -- use multimap::find()
		// TODO -- need to watch out for invalid iterators
		LLXMLNodeList::iterator children_itr;
		for (children_itr = node_list.begin(); children_itr != node_list.end(); ++children_itr)
		{ 
			LLXMLNode* child = (*children_itr).second;
			if (deleteChild(child))
			{
				removed_count++;
			}
		}
	}
	return removed_count > 0 ? TRUE : FALSE;
}

void LLXMLNode::setAttributes(LLXMLNode::ValueType type, U32 precision, LLXMLNode::Encoding encoding, U32 length)
{
	mType = type;
	mEncoding = encoding;
	mPrecision = precision;
	mLength = length;
}

void LLXMLNode::setName(const std::string& name)
{
	setName(gStringTable.addStringEntry(name));
}

void LLXMLNode::setName(LLStringTableEntry* name)
{
	LLXMLNode* old_parent = mParent;
	if (mParent)
	{
		// we need to remove and re-add to the parent so that
		// the multimap key agrees with this node's name
		mParent->removeChild(this);
	}
	mName = name;
	if (old_parent)
	{
		old_parent->addChild(this);
	}
}

// Unused
// void LLXMLNode::appendValue(const std::string& value)
// {
// 	mValue.append(value);
// }

U32 LLXMLNode::getChildCount() const 
{ 
	if (mChildren.notNull())
	{
		return mChildren->map.size(); 
	}
	return 0;
}

//***************************************************
//  UNIT TESTING 
//***************************************************

U32 get_rand(U32 max_value)
{
	U32 random_num = rand() + ((U32)rand() << 16);
	return (random_num % max_value);
}

LLXMLNode *get_rand_node(LLXMLNode *node)
{
	if (node->mChildren.notNull())
	{
		U32 num_children = node->mChildren->map.size();
		if (get_rand(2) == 0)
		{
			while (true)
			{
				S32 child_num = S32(get_rand(num_children*2)) - num_children;
				LLXMLChildList::iterator itor = node->mChildren->map.begin();
				while (child_num > 0)
				{
					--child_num;
					++itor;
				}
				if (!itor->second->mIsAttribute)
				{
					return get_rand_node(itor->second);
				}
			}
		}
	}
	return node;
}

void LLXMLNode::createUnitTest(S32 max_num_children)
{
	// Random ID
	std::string rand_id;
	U32 rand_id_len = get_rand(10)+5;
	for (U32 pos = 0; pos<rand_id_len; ++pos)
	{
		char c = 'a' + get_rand(26);
		rand_id.append(1, c);
	}
	mID = rand_id;

	if (max_num_children < 2)
	{
		setStringValue(1, &mID);
		return;
	}

	// Checksums
	U32 integer_checksum = 0;
	U64 long_checksum = 0;
	U32 bool_true_count = 0;
	LLUUID uuid_checksum;
	U32 noderef_checksum = 0;
	U32 float_checksum = 0;

	// Create a random number of children
	U32 num_children = get_rand(max_num_children)+1;
	for (U32 child_num=0; child_num<num_children; ++child_num)
	{
		// Random Name
		std::string child_name;
		U32 child_name_len = get_rand(10)+5;
		for (U32 pos = 0; pos<child_name_len; ++pos)
		{
			char c = 'a' + get_rand(26);
			child_name.append(1, c);
		}

		LLXMLNode *new_child = createChild(child_name.c_str(), FALSE);

		// Random ID
		std::string child_id;
		U32 child_id_len = get_rand(10)+5;
		for (U32 pos=0; pos<child_id_len; ++pos)
		{
			char c = 'a' + get_rand(26);
			child_id.append(1, c);
		}
		new_child->mID = child_id;

		// Random Length
		U32 array_size = get_rand(28)+1;

		// Random Encoding
		Encoding new_encoding = get_rand(2)?ENCODING_DECIMAL:ENCODING_HEX;

		// Random Type
		int type = get_rand(8);
		switch (type)
		{
		case 0: // TYPE_CONTAINER
			new_child->createUnitTest(max_num_children/2);
			break;
		case 1: // TYPE_BOOLEAN
			{
				BOOL random_bool_values[30];
				for (U32 value=0; value<array_size; ++value)
				{
					random_bool_values[value] = get_rand(2);
					if (random_bool_values[value])
					{
						++bool_true_count;
					}
				}
				new_child->setBoolValue(array_size, random_bool_values);
			}
			break;
		case 2: // TYPE_INTEGER (32-bit)
			{
				U32 random_int_values[30];
				for (U32 value=0; value<array_size; ++value)
				{
					random_int_values[value] = get_rand(0xffffffff);
					integer_checksum ^= random_int_values[value];
				}
				new_child->setUnsignedValue(array_size, random_int_values, new_encoding);
			}
			break;
		case 3: // TYPE_INTEGER (64-bit)
			{
				U64 random_int_values[30];
				for (U64 value=0; value<array_size; ++value)
				{
					random_int_values[value] = (U64(get_rand(0xffffffff)) << 32) + get_rand(0xffffffff);
					long_checksum ^= random_int_values[value];
				}
				new_child->setLongValue(array_size, random_int_values, new_encoding);
			}
			break;
		case 4: // TYPE_FLOAT (32-bit)
			{
				F32 random_float_values[30];
				for (U32 value=0; value<array_size; ++value)
				{
					S32 exponent = get_rand(256) - 128;
					S32 fractional_part = get_rand(0xffffffff);
					S32 sign = get_rand(2) * 2 - 1;
					random_float_values[value] = F32(fractional_part) / F32(0xffffffff) * exp(F32(exponent)) * F32(sign);

					U32 *float_bits = &((U32 *)random_float_values)[value];
					if (*float_bits == 0x80000000)
					{
						*float_bits = 0x00000000;
					}
					float_checksum ^= (*float_bits & 0xfffff000);
				}
				new_child->setFloatValue(array_size, random_float_values, new_encoding, 12);
			}
			break;
		case 5: // TYPE_FLOAT (64-bit)
			{
				F64 random_float_values[30];
				for (U32 value=0; value<array_size; ++value)
				{
					S32 exponent = get_rand(2048) - 1024;
					S32 fractional_part = get_rand(0xffffffff);
					S32 sign = get_rand(2) * 2 - 1;
					random_float_values[value] = F64(fractional_part) / F64(0xffffffff) * exp(F64(exponent)) * F64(sign);

					U64 *float_bits = &((U64 *)random_float_values)[value];
					if (*float_bits == 0x8000000000000000ll)
					{
						*float_bits = 0x0000000000000000ll;
					}
					float_checksum ^= ((*float_bits & 0xfffffff000000000ll) >> 32);
				}
				new_child->setDoubleValue(array_size, random_float_values, new_encoding, 12);
			}
			break;
		case 6: // TYPE_UUID
			{
				LLUUID random_uuid_values[30];
				for (U32 value=0; value<array_size; ++value)
				{
					random_uuid_values[value].generate();
					for (S32 byte=0; byte<UUID_BYTES; ++byte)
					{
						uuid_checksum.mData[byte] ^= random_uuid_values[value].mData[byte];
					}
				}
				new_child->setUUIDValue(array_size, random_uuid_values);
			}
			break;
		case 7: // TYPE_NODEREF
			{
				LLXMLNode *random_node_array[30];
				LLXMLNode *root = getRoot();
				for (U32 value=0; value<array_size; ++value)
				{
					random_node_array[value] = get_rand_node(root);
					const char *node_name = random_node_array[value]->mName->mString;
					for (U32 pos=0; pos<strlen(node_name); ++pos)		/* Flawfinder: ignore */
					{
						U32 hash_contrib = U32(node_name[pos]) << ((pos % 4) * 8);
						noderef_checksum ^= hash_contrib;
					}
				}
				new_child->setNodeRefValue(array_size, (const LLXMLNode **)random_node_array);
			}
			break;
		}
	}

	createChild("integer_checksum", TRUE)->setUnsignedValue(1, &integer_checksum, LLXMLNode::ENCODING_HEX);
	createChild("long_checksum", TRUE)->setLongValue(1, &long_checksum, LLXMLNode::ENCODING_HEX);
	createChild("bool_true_count", TRUE)->setUnsignedValue(1, &bool_true_count, LLXMLNode::ENCODING_HEX);
	createChild("uuid_checksum", TRUE)->setUUIDValue(1, &uuid_checksum);
	createChild("noderef_checksum", TRUE)->setUnsignedValue(1, &noderef_checksum, LLXMLNode::ENCODING_HEX);
	createChild("float_checksum", TRUE)->setUnsignedValue(1, &float_checksum, LLXMLNode::ENCODING_HEX);
}

BOOL LLXMLNode::performUnitTest(std::string &error_buffer)
{
	if (mChildren.isNull())
	{
		error_buffer.append(llformat("ERROR Node %s: No children found.\n", mName->mString));
		return FALSE;
	}

	// Checksums
	U32 integer_checksum = 0;
	U32 bool_true_count = 0;
	LLUUID uuid_checksum;
	U32 noderef_checksum = 0;
	U32 float_checksum = 0;
	U64 long_checksum = 0;

	LLXMLChildList::iterator itor;
	for (itor=mChildren->map.begin(); itor!=mChildren->map.end(); ++itor)
	{
		LLXMLNode *node = itor->second;
		if (node->mIsAttribute)
		{
			continue;
		}
		if (node->mType == TYPE_CONTAINER)
		{
			if (!node->performUnitTest(error_buffer))
			{
				error_buffer.append(llformat("Child test failed for %s.\n", mName->mString));
				//return FALSE;
			}
			continue;
		}
		if (node->mLength < 1 || node->mLength > 30)
		{
			error_buffer.append(llformat("ERROR Node %s: Invalid array length %d, child %s.\n", mName->mString, node->mLength, node->mName->mString));
			return FALSE;
		}
		switch (node->mType)
		{
		case TYPE_CONTAINER:
		case TYPE_UNKNOWN:
			break;
		case TYPE_BOOLEAN:
			{
				BOOL bool_array[30];
				if (node->getBoolValue(node->mLength, bool_array) < node->mLength)
				{
					error_buffer.append(llformat("ERROR Node %s: Could not read boolean array, child %s.\n", mName->mString, node->mName->mString));
					return FALSE;
				}
				for (U32 pos=0; pos<(U32)node->mLength; ++pos)
				{
					if (bool_array[pos])
					{
						++bool_true_count;
					}
				}
			}
			break;
		case TYPE_INTEGER:
			{
				if (node->mPrecision == 32)
				{
					U32 integer_array[30];
					if (node->getUnsignedValue(node->mLength, integer_array, node->mEncoding) < node->mLength)
					{
						error_buffer.append(llformat("ERROR Node %s: Could not read integer array, child %s.\n", mName->mString, node->mName->mString));
						return FALSE;
					}
					for (U32 pos=0; pos<(U32)node->mLength; ++pos)
					{
						integer_checksum ^= integer_array[pos];
					}
				}
				else
				{
					U64 integer_array[30];
					if (node->getLongValue(node->mLength, integer_array, node->mEncoding) < node->mLength)
					{
						error_buffer.append(llformat("ERROR Node %s: Could not read long integer array, child %s.\n", mName->mString, node->mName->mString));
						return FALSE;
					}
					for (U32 pos=0; pos<(U32)node->mLength; ++pos)
					{
						long_checksum ^= integer_array[pos];
					}
				}
			}
			break;
		case TYPE_FLOAT:
			{
				if (node->mPrecision == 32)
				{
					F32 float_array[30];
					if (node->getFloatValue(node->mLength, float_array, node->mEncoding) < node->mLength)
					{
						error_buffer.append(llformat("ERROR Node %s: Could not read float array, child %s.\n", mName->mString, node->mName->mString));
						return FALSE;
					}
					for (U32 pos=0; pos<(U32)node->mLength; ++pos)
					{
						U32 float_bits = ((U32 *)float_array)[pos];
						float_checksum ^= (float_bits & 0xfffff000);
					}
				}
				else
				{
					F64 float_array[30];
					if (node->getDoubleValue(node->mLength, float_array, node->mEncoding) < node->mLength)
					{
						error_buffer.append(llformat("ERROR Node %s: Could not read float array, child %s.\n", mName->mString, node->mName->mString));
						return FALSE;
					}
					for (U32 pos=0; pos<(U32)node->mLength; ++pos)
					{
						U64 float_bits = ((U64 *)float_array)[pos];
						float_checksum ^= ((float_bits & 0xfffffff000000000ll) >> 32);
					}
				}
			}
			break;
		case TYPE_STRING:
			break;
		case TYPE_UUID:
			{
				LLUUID uuid_array[30];
				if (node->getUUIDValue(node->mLength, uuid_array) < node->mLength)
				{
					error_buffer.append(llformat("ERROR Node %s: Could not read uuid array, child %s.\n", mName->mString, node->mName->mString));
					return FALSE;
				}
				for (U32 pos=0; pos<(U32)node->mLength; ++pos)
				{
					for (S32 byte=0; byte<UUID_BYTES; ++byte)
					{
						uuid_checksum.mData[byte] ^= uuid_array[pos].mData[byte];
					}
				}
			}
			break;
		case TYPE_NODEREF:
			{
				LLXMLNode *node_array[30];
				if (node->getNodeRefValue(node->mLength, node_array) < node->mLength)
				{
					error_buffer.append(llformat("ERROR Node %s: Could not read node ref array, child %s.\n", mName->mString, node->mName->mString));
					return FALSE;
				}
				for (U32 pos=0; pos<node->mLength; ++pos)
				{
					const char *node_name = node_array[pos]->mName->mString;
					for (U32 pos2=0; pos2<strlen(node_name); ++pos2)		/* Flawfinder: ignore */
					{
						U32 hash_contrib = U32(node_name[pos2]) << ((pos2 % 4) * 8);
						noderef_checksum ^= hash_contrib;
					}
				}
			}
			break;
		}
	}

	LLXMLNodePtr checksum_node;

	// Compare checksums
	{
		U32 node_integer_checksum = 0;
		if (!getAttribute("integer_checksum", checksum_node, FALSE) || 
			checksum_node->getUnsignedValue(1, &node_integer_checksum, ENCODING_HEX) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: Integer checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_integer_checksum != integer_checksum)
		{
			error_buffer.append(llformat("ERROR Node %s: Integer checksum mismatch: read %X / calc %X.\n", mName->mString, node_integer_checksum, integer_checksum));
			return FALSE;
		}
	}

	{
		U64 node_long_checksum = 0;
		if (!getAttribute("long_checksum", checksum_node, FALSE) || 
			checksum_node->getLongValue(1, &node_long_checksum, ENCODING_HEX) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: Long Integer checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_long_checksum != long_checksum)
		{
			U32 *pp1 = (U32 *)&node_long_checksum;
			U32 *pp2 = (U32 *)&long_checksum;
			error_buffer.append(llformat("ERROR Node %s: Long Integer checksum mismatch: read %08X%08X / calc %08X%08X.\n", mName->mString, pp1[1], pp1[0], pp2[1], pp2[0]));
			return FALSE;
		}
	}

	{
		U32 node_bool_true_count = 0;
		if (!getAttribute("bool_true_count", checksum_node, FALSE) || 
			checksum_node->getUnsignedValue(1, &node_bool_true_count, ENCODING_HEX) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: Boolean checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_bool_true_count != bool_true_count)
		{
			error_buffer.append(llformat("ERROR Node %s: Boolean checksum mismatch: read %X / calc %X.\n", mName->mString, node_bool_true_count, bool_true_count));
			return FALSE;
		}
	}

	{
		LLUUID node_uuid_checksum;
		if (!getAttribute("uuid_checksum", checksum_node, FALSE) || 
			checksum_node->getUUIDValue(1, &node_uuid_checksum) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: UUID checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_uuid_checksum != uuid_checksum)
		{
			error_buffer.append(llformat("ERROR Node %s: UUID checksum mismatch: read %s / calc %s.\n", mName->mString, node_uuid_checksum.asString().c_str(), uuid_checksum.asString().c_str()));
			return FALSE;
		}
	}

	{
		U32 node_noderef_checksum = 0;
		if (!getAttribute("noderef_checksum", checksum_node, FALSE) || 
			checksum_node->getUnsignedValue(1, &node_noderef_checksum, ENCODING_HEX) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: Node Ref checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_noderef_checksum != noderef_checksum)
		{
			error_buffer.append(llformat("ERROR Node %s: Node Ref checksum mismatch: read %X / calc %X.\n", mName->mString, node_noderef_checksum, noderef_checksum));
			return FALSE;
		}
	}

	{
		U32 node_float_checksum = 0;
		if (!getAttribute("float_checksum", checksum_node, FALSE) || 
			checksum_node->getUnsignedValue(1, &node_float_checksum, ENCODING_HEX) != 1)
		{
			error_buffer.append(llformat("ERROR Node %s: Float checksum missing.\n", mName->mString));
			return FALSE;
		}
		if (node_float_checksum != float_checksum)
		{
			error_buffer.append(llformat("ERROR Node %s: Float checksum mismatch: read %X / calc %X.\n", mName->mString, node_float_checksum, float_checksum));
			return FALSE;
		}
	}

	return TRUE;
}

LLXMLNodePtr LLXMLNode::getFirstChild() const
{
	if (mChildren.isNull()) return NULL;
	LLXMLNodePtr ret = mChildren->head;
	return ret;
}

LLXMLNodePtr LLXMLNode::getNextSibling() const
{
	LLXMLNodePtr ret = mNext;
	return ret;
}

std::string LLXMLNode::getSanitizedValue() const 
{ 
	if (mIsAttribute) 
	{
		return getValue() ;
	}
	else 
	{
		return getTextContents(); 
	}
}


std::string LLXMLNode::getTextContents() const
{
	std::string msg;
	std::string contents = mValue;
	std::string::size_type n = contents.find_first_not_of(" \t\n");
	if (n != std::string::npos && contents[n] == '\"')
	{
		// Case 1: node has quoted text
		S32 num_lines = 0;
		while(1)
		{
			// mContents[n] == '"'
			++n;
			std::string::size_type t = n;
			std::string::size_type m = 0;
			// fix-up escaped characters
			while(1)
			{
				m = contents.find_first_of("\\\"", t); // find first \ or "
				if ((m == std::string::npos) || (contents[m] == '\"'))
				{
					break;
				}
				contents.erase(m,1);
				t = m+1;
			}
			if (m == std::string::npos)
			{
				break;
			}
			// mContents[m] == '"'
			num_lines++;
			msg += contents.substr(n,m-n) + "\n";
			n = contents.find_first_of("\"", m+1);
			if (n == std::string::npos)
			{
				if (num_lines == 1)
				{
					msg.erase(msg.size()-1); // remove "\n" if only one line
				}
				break;
			}
		}
	}
	else
	{
		// Case 2: node has embedded text (beginning and trailing whitespace trimmed)
		std::string::size_type start = mValue.find_first_not_of(" \t\n");
		if (start != mValue.npos)
		{
			std::string::size_type end = mValue.find_last_not_of(" \t\n");
			if (end != mValue.npos)
			{
				msg = mValue.substr(start, end+1-start);
			}
			else
			{
				msg = mValue.substr(start);
			}
		}
		// Convert any internal CR to LF
		msg = utf8str_removeCRLF(msg);
	}
	return msg;
}

void LLXMLNode::setLineNumber(S32 line_number)
{
	mLineNumber = line_number;
}

S32 LLXMLNode::getLineNumber()
{
	return mLineNumber;
}
