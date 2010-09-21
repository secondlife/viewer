/** 
 * @file llxmltree.cpp
 * @brief LLXmlTree implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llxmltree.h"
#include "v3color.h"
#include "v4color.h"
#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "v4math.h"
#include "llquaternion.h"
#include "lluuid.h"

//////////////////////////////////////////////////////////////
// LLXmlTree

// static
LLStdStringTable LLXmlTree::sAttributeKeys(1024);

LLXmlTree::LLXmlTree()
	: mRoot( NULL ),
	  mNodeNames(512)
{
}

LLXmlTree::~LLXmlTree()
{
	cleanup();
}

void LLXmlTree::cleanup()
{
	delete mRoot;
	mRoot = NULL;
	mNodeNames.cleanup();
}


BOOL LLXmlTree::parseFile(const std::string &path, BOOL keep_contents)
{
	delete mRoot;
	mRoot = NULL;

	LLXmlTreeParser parser(this);
	BOOL success = parser.parseFile( path, &mRoot, keep_contents );
	if( !success )
	{
		S32 line_number = parser.getCurrentLineNumber();
		const char* error =  parser.getErrorString();
		llwarns << "LLXmlTree parse failed.  Line " << line_number << ": " << error << llendl;
	}
	return success;
}

void LLXmlTree::dump()
{
	if( mRoot )
	{
		dumpNode( mRoot, "    " );
	}
}

void LLXmlTree::dumpNode( LLXmlTreeNode* node, const std::string& prefix )
{
	node->dump( prefix );

	std::string new_prefix = prefix + "    ";
	for( LLXmlTreeNode* child = node->getFirstChild(); child; child = node->getNextChild() )
	{
		dumpNode( child, new_prefix );
	}
}

//////////////////////////////////////////////////////////////
// LLXmlTreeNode

LLXmlTreeNode::LLXmlTreeNode( const std::string& name, LLXmlTreeNode* parent, LLXmlTree* tree )
	: mName(name),
	  mParent(parent),
	  mTree(tree)
{
}

LLXmlTreeNode::~LLXmlTreeNode()
{
	attribute_map_t::iterator iter;
	for (iter=mAttributes.begin(); iter != mAttributes.end(); iter++)
		delete iter->second;
	child_list_t::iterator child_iter;
	for (child_iter=mChildList.begin(); child_iter != mChildList.end(); child_iter++)
		delete *child_iter;
}
 
void LLXmlTreeNode::dump( const std::string& prefix )
{
	llinfos << prefix << mName ;
	if( !mContents.empty() )
	{
		llcont << " contents = \"" << mContents << "\"";
	}
	attribute_map_t::iterator iter;
	for (iter=mAttributes.begin(); iter != mAttributes.end(); iter++)
	{
		LLStdStringHandle key = iter->first;
		const std::string* value = iter->second;
		llcont << prefix << " " << key << "=" << (value->empty() ? "NULL" : *value);
	}
	llcont << llendl;
} 

BOOL LLXmlTreeNode::hasAttribute(const std::string& name)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	attribute_map_t::iterator iter = mAttributes.find(canonical_name);
	return (iter == mAttributes.end()) ? false : true;
}

void LLXmlTreeNode::addAttribute(const std::string& name, const std::string& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	const std::string *newstr = new std::string(value);
	mAttributes[canonical_name] = newstr; // insert + copy
}

LLXmlTreeNode*	LLXmlTreeNode::getFirstChild()
{
	mChildListIter = mChildList.begin();
	return getNextChild();
}
LLXmlTreeNode*	LLXmlTreeNode::getNextChild()
{
	if (mChildListIter == mChildList.end())
		return 0;
	else
		return *mChildListIter++;
}

LLXmlTreeNode* LLXmlTreeNode::getChildByName(const std::string& name)
{
	LLStdStringHandle tableptr = mTree->mNodeNames.checkString(name);
	mChildMapIter = mChildMap.lower_bound(tableptr);
	mChildMapEndIter = mChildMap.upper_bound(tableptr);
	return getNextNamedChild();
}

LLXmlTreeNode* LLXmlTreeNode::getNextNamedChild()
{
	if (mChildMapIter == mChildMapEndIter)
		return NULL;
	else
		return (mChildMapIter++)->second;
}

void LLXmlTreeNode::appendContents(const std::string& str)
{
	mContents.append( str );
}

void LLXmlTreeNode::addChild(LLXmlTreeNode* child)
{
	llassert( child );
	mChildList.push_back( child );

	// Add a name mapping to this node
	LLStdStringHandle tableptr = mTree->mNodeNames.insert(child->mName);
	mChildMap.insert( child_map_t::value_type(tableptr, child));
	
	child->mParent = this;
}

//////////////////////////////////////////////////////////////

// These functions assume that name is already in mAttritrubteKeys

BOOL LLXmlTreeNode::getFastAttributeBOOL(LLStdStringHandle canonical_name, BOOL& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToBOOL( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeU8(LLStdStringHandle canonical_name, U8& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToU8( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeS8(LLStdStringHandle canonical_name, S8& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToS8( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeS16(LLStdStringHandle canonical_name, S16& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToS16( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeU16(LLStdStringHandle canonical_name, U16& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToU16( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeU32(LLStdStringHandle canonical_name, U32& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToU32( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeS32(LLStdStringHandle canonical_name, S32& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToS32( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeF32(LLStdStringHandle canonical_name, F32& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToF32( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeF64(LLStdStringHandle canonical_name, F64& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s && LLStringUtil::convertToF64( *s, value );
}

BOOL LLXmlTreeNode::getFastAttributeColor(LLStdStringHandle canonical_name, LLColor4& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLColor4::parseColor(*s, &value) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeColor4(LLStdStringHandle canonical_name, LLColor4& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLColor4::parseColor4(*s, &value) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeColor4U(LLStdStringHandle canonical_name, LLColor4U& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLColor4U::parseColor4U(*s, &value ) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeVector3(LLStdStringHandle canonical_name, LLVector3& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLVector3::parseVector3(*s, &value ) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeVector3d(LLStdStringHandle canonical_name, LLVector3d& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLVector3d::parseVector3d(*s,  &value ) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeQuat(LLStdStringHandle canonical_name, LLQuaternion& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLQuaternion::parseQuat(*s, &value ) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeUUID(LLStdStringHandle canonical_name, LLUUID& value)
{
	const std::string *s = getAttribute( canonical_name );
	return s ? LLUUID::parseUUID(*s, &value ) : FALSE;
}

BOOL LLXmlTreeNode::getFastAttributeString(LLStdStringHandle canonical_name, std::string& value)
{
	const std::string *s = getAttribute( canonical_name );
	if( !s )
	{
		return FALSE;
	}

	value = *s;
	return TRUE;
}


//////////////////////////////////////////////////////////////

BOOL LLXmlTreeNode::getAttributeBOOL(const std::string& name, BOOL& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeBOOL(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeU8(const std::string& name, U8& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeU8(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeS8(const std::string& name, S8& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeS8(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeS16(const std::string& name, S16& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeS16(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeU16(const std::string& name, U16& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeU16(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeU32(const std::string& name, U32& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeU32(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeS32(const std::string& name, S32& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeS32(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeF32(const std::string& name, F32& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeF32(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeF64(const std::string& name, F64& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeF64(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeColor(const std::string& name, LLColor4& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeColor(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeColor4(const std::string& name, LLColor4& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeColor4(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeColor4U(const std::string& name, LLColor4U& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeColor4U(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeVector3(const std::string& name, LLVector3& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeVector3(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeVector3d(const std::string& name, LLVector3d& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeVector3d(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeQuat(const std::string& name, LLQuaternion& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeQuat(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeUUID(const std::string& name, LLUUID& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeUUID(canonical_name, value);
}

BOOL LLXmlTreeNode::getAttributeString(const std::string& name, std::string& value)
{
	LLStdStringHandle canonical_name = LLXmlTree::sAttributeKeys.addString( name );
	return getFastAttributeString(canonical_name, value);
}

/*
  The following xml <message> nodes will all return the string from getTextContents():
  "The quick brown fox\n  Jumps over the lazy dog"

  1. HTML paragraph format:
		<message>
		<p>The quick brown fox</p>
		<p>  Jumps over the lazy dog</p>
		</message>
  2. Each quoted section -> paragraph:
		<message>
		"The quick brown fox"
		"  Jumps over the lazy dog"
		</message>
  3. Literal text with beginning and trailing whitespace removed:
		<message>
The quick brown fox
  Jumps over the lazy dog
		</message>
  
*/

std::string LLXmlTreeNode::getTextContents()
{
	std::string msg;
	LLXmlTreeNode* p = getChildByName("p");
	if (p)
	{
		// Case 1: node has <p>text</p> tags
		while (p)
		{
			msg += p->getContents() + "\n";
			p = getNextNamedChild();
		}
	}
	else
	{
		std::string::size_type n = mContents.find_first_not_of(" \t\n");
		if (n != std::string::npos && mContents[n] == '\"')
		{
			// Case 2: node has quoted text
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
					m = mContents.find_first_of("\\\"", t); // find first \ or "
					if ((m == std::string::npos) || (mContents[m] == '\"'))
					{
						break;
					}
					mContents.erase(m,1);
					t = m+1;
				}
				if (m == std::string::npos)
				{
					break;
				}
				// mContents[m] == '"'
				num_lines++;
				msg += mContents.substr(n,m-n) + "\n";
				n = mContents.find_first_of("\"", m+1);
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
			// Case 3: node has embedded text (beginning and trailing whitespace trimmed)
			msg = mContents;
		}
	}
	return msg;
}
	

//////////////////////////////////////////////////////////////
// LLXmlTreeParser

LLXmlTreeParser::LLXmlTreeParser(LLXmlTree* tree) 
	: mTree(tree),
	  mRoot( NULL ),
	  mCurrent( NULL ),
	  mDump( FALSE ),
	  mKeepContents(FALSE)
{
}

LLXmlTreeParser::~LLXmlTreeParser() 
{
}

BOOL LLXmlTreeParser::parseFile(const std::string &path, LLXmlTreeNode** root, BOOL keep_contents)
{
	llassert( !mRoot );
	llassert( !mCurrent );

	mKeepContents = keep_contents;

	BOOL success = LLXmlParser::parseFile(path);

	*root = mRoot;
	mRoot = NULL;

	if( success )
	{
		llassert( !mCurrent );
	}
	mCurrent = NULL;
	
	return success;
}


const std::string& LLXmlTreeParser::tabs()
{
	static std::string s;
	s = "";
	S32 num_tabs = getDepth() - 1;
	for( S32 i = 0; i < num_tabs; i++)
	{
		s += "    ";
	}
	return s;
}

void LLXmlTreeParser::startElement(const char* name, const char **atts) 
{
	if( mDump )
	{
		llinfos << tabs() << "startElement " << name << llendl;
		
		S32 i = 0;
		while( atts[i] && atts[i+1] )
		{
			llinfos << tabs() << "attribute: " << atts[i] << "=" << atts[i+1] << llendl;
			i += 2;
		}
	}

	LLXmlTreeNode* child = CreateXmlTreeNode( std::string(name), mCurrent );

	S32 i = 0;
	while( atts[i] && atts[i+1] )
	{
		child->addAttribute( atts[i], atts[i+1] );
		i += 2;
	}

	if( mCurrent )
	{
		mCurrent->addChild( child );

	}
	else
	{
		llassert( !mRoot );
		mRoot = child;
	}
	mCurrent = child;
}

LLXmlTreeNode* LLXmlTreeParser::CreateXmlTreeNode(const std::string& name, LLXmlTreeNode* parent)
{
	return new LLXmlTreeNode(name, parent, mTree);
}


void LLXmlTreeParser::endElement(const char* name) 
{
	if( mDump )
	{
		llinfos << tabs() << "endElement " << name << llendl;
	}

	if( !mCurrent->mContents.empty() )
	{
		LLStringUtil::trim(mCurrent->mContents);
		LLStringUtil::removeCRLF(mCurrent->mContents);
	}

	mCurrent = mCurrent->getParent();
}

void LLXmlTreeParser::characterData(const char *s, int len) 
{
	std::string str;
	if (s) str = std::string(s, len);
	if( mDump )
	{
		llinfos << tabs() << "CharacterData " << str << llendl;
	}

	if (mKeepContents)
	{
		mCurrent->appendContents( str );
	}
}

void LLXmlTreeParser::processingInstruction(const char *target, const char *data)
{
	if( mDump )
	{
		llinfos << tabs() << "processingInstruction " << data << llendl;
	}
}

void LLXmlTreeParser::comment(const char *data)
{
	if( mDump )
	{
		llinfos << tabs() << "comment " << data << llendl;
	}
}

void LLXmlTreeParser::startCdataSection()
{
	if( mDump )
	{
		llinfos << tabs() << "startCdataSection" << llendl;
	}
}

void LLXmlTreeParser::endCdataSection()
{
	if( mDump )
	{
		llinfos << tabs() << "endCdataSection" << llendl;
	}
}

void LLXmlTreeParser::defaultData(const char *s, int len)
{
	if( mDump )
	{
		std::string str;
		if (s) str = std::string(s, len);
		llinfos << tabs() << "defaultData " << str << llendl;
	}
}

void LLXmlTreeParser::unparsedEntityDecl(
	const char* entity_name,
	const char* base,
	const char* system_id,
	const char* public_id,
	const char* notation_name)
{
	if( mDump )
	{
		llinfos << tabs() << "unparsed entity:"			<< llendl;
		llinfos << tabs() << "    entityName "			<< entity_name	<< llendl;
		llinfos << tabs() << "    base "				<< base			<< llendl;
		llinfos << tabs() << "    systemId "			<< system_id	<< llendl;
		llinfos << tabs() << "    publicId "			<< public_id	<< llendl;
		llinfos << tabs() << "    notationName "		<< notation_name<< llendl;
	}
}

void test_llxmltree()
{
	LLXmlTree tree;
	BOOL success = tree.parseFile( "test.xml" );
	if( success )
	{
		tree.dump();
	}
}

