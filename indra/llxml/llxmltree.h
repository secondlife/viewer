/** 
 * @file llxmltree.h
 * @author Aaron Yonas, Richard Nelson
 * @brief LLXmlTree class definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLXMLTREE_H
#define LL_LLXMLTREE_H

#include <map>
#include <list>
#include "llstring.h"
#include "llxmlparser.h"
#include "string_table.h"

class LLColor4;
class LLColor4U;
class LLQuaternion;
class LLUUID;
class LLVector3;
class LLVector3d;
class LLXmlTreeNode;
class LLXmlTreeParser;

//////////////////////////////////////////////////////////////
// LLXmlTree

class LLXmlTree
{
	friend class LLXmlTreeNode;
	
public:
	LLXmlTree();
	virtual ~LLXmlTree();
	void cleanup();

	virtual BOOL	parseFile(const std::string &path, BOOL keep_contents = TRUE);

	LLXmlTreeNode*	getRoot() { return mRoot; }

	void			dump();
	void			dumpNode( LLXmlTreeNode* node, const std::string& prefix );

	static LLStdStringHandle addAttributeString( const std::string& name)
	{
		return sAttributeKeys.addString( name );
	}
	
public:
	// global
	static LLStdStringTable sAttributeKeys;
	
protected:
	LLXmlTreeNode* mRoot;

	// local
	LLStdStringTable mNodeNames;	
};

//////////////////////////////////////////////////////////////
// LLXmlTreeNode

class LLXmlTreeNode
{
	friend class LLXmlTree;
	friend class LLXmlTreeParser;

protected:
	// Protected since nodes are only created and destroyed by friend classes and other LLXmlTreeNodes
	LLXmlTreeNode( const std::string& name, LLXmlTreeNode* parent, LLXmlTree* tree );
	
public:
	virtual ~LLXmlTreeNode();

	const std::string&	getName()
	{
		return mName;
	}
	BOOL hasName( const std::string& name )
	{
		return mName == name;
	}

	BOOL hasAttribute( const std::string& name );

	// Fast versions use cannonical_name handlee to entru in LLXmlTree::sAttributeKeys string table
	BOOL			getFastAttributeBOOL(		LLStdStringHandle cannonical_name, BOOL& value );
	BOOL			getFastAttributeU8(			LLStdStringHandle cannonical_name, U8& value );
	BOOL			getFastAttributeS8(			LLStdStringHandle cannonical_name, S8& value );
	BOOL			getFastAttributeU16(		LLStdStringHandle cannonical_name, U16& value );
	BOOL			getFastAttributeS16(		LLStdStringHandle cannonical_name, S16& value );
	BOOL			getFastAttributeU32(		LLStdStringHandle cannonical_name, U32& value );
	BOOL			getFastAttributeS32(		LLStdStringHandle cannonical_name, S32& value );
	BOOL			getFastAttributeF32(		LLStdStringHandle cannonical_name, F32& value );
	BOOL			getFastAttributeF64(		LLStdStringHandle cannonical_name, F64& value );
	BOOL			getFastAttributeColor(		LLStdStringHandle cannonical_name, LLColor4& value );
	BOOL			getFastAttributeColor4(		LLStdStringHandle cannonical_name, LLColor4& value );
	BOOL			getFastAttributeColor4U(	LLStdStringHandle cannonical_name, LLColor4U& value );
	BOOL			getFastAttributeVector3(	LLStdStringHandle cannonical_name, LLVector3& value );
	BOOL			getFastAttributeVector3d(	LLStdStringHandle cannonical_name, LLVector3d& value );
	BOOL			getFastAttributeQuat(		LLStdStringHandle cannonical_name, LLQuaternion& value );
	BOOL			getFastAttributeUUID(		LLStdStringHandle cannonical_name, LLUUID& value );
	BOOL			getFastAttributeString(		LLStdStringHandle cannonical_name, std::string& value );

	// Normal versions find 'name' in LLXmlTree::sAttributeKeys then call fast versions
	virtual BOOL		getAttributeBOOL(		const std::string& name, BOOL& value );
	virtual BOOL		getAttributeU8(			const std::string& name, U8& value );
	virtual BOOL		getAttributeS8(			const std::string& name, S8& value );
	virtual BOOL		getAttributeU16(		const std::string& name, U16& value );
	virtual BOOL		getAttributeS16(		const std::string& name, S16& value );
	virtual BOOL		getAttributeU32(		const std::string& name, U32& value );
	virtual BOOL		getAttributeS32(		const std::string& name, S32& value );
	virtual BOOL		getAttributeF32(		const std::string& name, F32& value );
	virtual BOOL		getAttributeF64(		const std::string& name, F64& value );
	virtual BOOL		getAttributeColor(		const std::string& name, LLColor4& value );
	virtual BOOL		getAttributeColor4(		const std::string& name, LLColor4& value );
	virtual BOOL		getAttributeColor4U(	const std::string& name, LLColor4U& value );
	virtual BOOL		getAttributeVector3(	const std::string& name, LLVector3& value );
	virtual BOOL		getAttributeVector3d(	const std::string& name, LLVector3d& value );
	virtual BOOL		getAttributeQuat(		const std::string& name, LLQuaternion& value );
	virtual BOOL		getAttributeUUID(		const std::string& name, LLUUID& value );
	virtual BOOL		getAttributeString(		const std::string& name, std::string& value );

	const std::string& getContents()
	{
		return mContents;
	}
	std::string getTextContents();

	LLXmlTreeNode*	getParent()							{ return mParent; }
	LLXmlTreeNode*	getFirstChild();
	LLXmlTreeNode*	getNextChild();
	S32				getChildCount()						{ return (S32)mChildList.size(); }
	LLXmlTreeNode*  getChildByName( const std::string& name );	// returns first child with name, NULL if none
	LLXmlTreeNode*  getNextNamedChild();				// returns next child with name, NULL if none

protected:
	const std::string* getAttribute( LLStdStringHandle name)
	{
		attribute_map_t::iterator iter = mAttributes.find(name);
		return (iter == mAttributes.end()) ? 0 : iter->second;
	}

private:
	void			addAttribute( const std::string& name, const std::string& value );
	void			appendContents( const std::string& str );
	void			addChild( LLXmlTreeNode* child );

	void			dump( const std::string& prefix );

protected:
	typedef std::map<LLStdStringHandle, const std::string*> attribute_map_t;
	attribute_map_t						mAttributes;

private:
	std::string							mName;
	std::string							mContents;
	
	typedef std::list<class LLXmlTreeNode *> child_list_t;
	child_list_t						mChildList;
	child_list_t::iterator				mChildListIter;
	
	typedef std::multimap<LLStdStringHandle, LLXmlTreeNode *> child_map_t;
	child_map_t							mChildMap;		// for fast name lookups
	child_map_t::iterator				mChildMapIter;
	child_map_t::iterator				mChildMapEndIter;

	LLXmlTreeNode*						mParent;
	LLXmlTree*							mTree;
};

//////////////////////////////////////////////////////////////
// LLXmlTreeParser

class LLXmlTreeParser : public LLXmlParser
{
public:
	LLXmlTreeParser(LLXmlTree* tree);
	virtual ~LLXmlTreeParser();

	BOOL parseFile(const std::string &path, LLXmlTreeNode** root, BOOL keep_contents );

protected:
	const std::string& tabs();

	// Overrides from LLXmlParser
	virtual void	startElement(const char *name, const char **attributes); 
	virtual void	endElement(const char *name);
	virtual void	characterData(const char *s, int len);
	virtual void	processingInstruction(const char *target, const char *data);
	virtual void	comment(const char *data);
	virtual void	startCdataSection();
	virtual void	endCdataSection();
	virtual void	defaultData(const char *s, int len);
	virtual void	unparsedEntityDecl(
		const char* entity_name,
		const char* base,
		const char* system_id,
		const char* public_id,
		const char* notation_name);

	//template method pattern
	virtual LLXmlTreeNode* CreateXmlTreeNode(const std::string& name, LLXmlTreeNode* parent);

protected:
	LLXmlTree*		mTree;
	LLXmlTreeNode*	mRoot;
	LLXmlTreeNode*  mCurrent;
	BOOL			mDump;	// Dump parse tree to llinfos as it is read.
	BOOL			mKeepContents;
};

#endif  // LL_LLXMLTREE_H
