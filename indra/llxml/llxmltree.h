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
#include "llstringtable.h"

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

    virtual bool    parseFile(const std::string &path, bool keep_contents = true);

    LLXmlTreeNode*  getRoot() { return mRoot; }

    void            dump();
    void            dumpNode( LLXmlTreeNode* node, const std::string& prefix );

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

    const std::string&  getName()
    {
        return mName;
    }
    bool hasName( const std::string& name )
    {
        return mName == name;
    }

    bool hasAttribute( const std::string& name );

    // Fast versions use cannonical_name handlee to entru in LLXmlTree::sAttributeKeys string table
    bool            getFastAttributeBOOL(       LLStdStringHandle cannonical_name, bool& value );
    bool            getFastAttributeU8(         LLStdStringHandle cannonical_name, U8& value );
    bool            getFastAttributeS8(         LLStdStringHandle cannonical_name, S8& value );
    bool            getFastAttributeU16(        LLStdStringHandle cannonical_name, U16& value );
    bool            getFastAttributeS16(        LLStdStringHandle cannonical_name, S16& value );
    bool            getFastAttributeU32(        LLStdStringHandle cannonical_name, U32& value );
    bool            getFastAttributeS32(        LLStdStringHandle cannonical_name, S32& value );
    bool            getFastAttributeF32(        LLStdStringHandle cannonical_name, F32& value );
    bool            getFastAttributeF64(        LLStdStringHandle cannonical_name, F64& value );
    bool            getFastAttributeColor(      LLStdStringHandle cannonical_name, LLColor4& value );
    bool            getFastAttributeColor4(     LLStdStringHandle cannonical_name, LLColor4& value );
    bool            getFastAttributeColor4U(    LLStdStringHandle cannonical_name, LLColor4U& value );
    bool            getFastAttributeVector3(    LLStdStringHandle cannonical_name, LLVector3& value );
    bool            getFastAttributeVector3d(   LLStdStringHandle cannonical_name, LLVector3d& value );
    bool            getFastAttributeQuat(       LLStdStringHandle cannonical_name, LLQuaternion& value );
    bool            getFastAttributeUUID(       LLStdStringHandle cannonical_name, LLUUID& value );
    bool            getFastAttributeString(     LLStdStringHandle cannonical_name, std::string& value );

    // Normal versions find 'name' in LLXmlTree::sAttributeKeys then call fast versions
    virtual bool        getAttributeBOOL(       const std::string& name, bool& value );
    virtual bool        getAttributeU8(         const std::string& name, U8& value );
    virtual bool        getAttributeS8(         const std::string& name, S8& value );
    virtual bool        getAttributeU16(        const std::string& name, U16& value );
    virtual bool        getAttributeS16(        const std::string& name, S16& value );
    virtual bool        getAttributeU32(        const std::string& name, U32& value );
    virtual bool        getAttributeS32(        const std::string& name, S32& value );
    virtual bool        getAttributeF32(        const std::string& name, F32& value );
    virtual bool        getAttributeF64(        const std::string& name, F64& value );
    virtual bool        getAttributeColor(      const std::string& name, LLColor4& value );
    virtual bool        getAttributeColor4(     const std::string& name, LLColor4& value );
    virtual bool        getAttributeColor4U(    const std::string& name, LLColor4U& value );
    virtual bool        getAttributeVector3(    const std::string& name, LLVector3& value );
    virtual bool        getAttributeVector3d(   const std::string& name, LLVector3d& value );
    virtual bool        getAttributeQuat(       const std::string& name, LLQuaternion& value );
    virtual bool        getAttributeUUID(       const std::string& name, LLUUID& value );
    virtual bool        getAttributeString(     const std::string& name, std::string& value );

    const std::string& getContents()
    {
        return mContents;
    }
    std::string getTextContents();

    LLXmlTreeNode*  getParent()                         { return mParent; }
    LLXmlTreeNode*  getFirstChild();
    LLXmlTreeNode*  getNextChild();
    S32             getChildCount()                     { return (S32)mChildren.size(); }
    LLXmlTreeNode*  getChildByName( const std::string& name );  // returns first child with name, NULL if none
    LLXmlTreeNode*  getNextNamedChild();                // returns next child with name, NULL if none

protected:
    const std::string* getAttribute( LLStdStringHandle name)
    {
        attribute_map_t::iterator iter = mAttributes.find(name);
        return (iter == mAttributes.end()) ? 0 : iter->second;
    }

private:
    void            addAttribute( const std::string& name, const std::string& value );
    void            appendContents( const std::string& str );
    void            addChild( LLXmlTreeNode* child );

    void            dump( const std::string& prefix );

protected:
    typedef std::map<LLStdStringHandle, const std::string*> attribute_map_t;
    attribute_map_t                     mAttributes;

private:
    std::string                         mName;
    std::string                         mContents;

    typedef std::vector<class LLXmlTreeNode *> children_t;
    children_t                          mChildren;
    children_t::iterator                mChildrenIter;

    typedef std::multimap<LLStdStringHandle, LLXmlTreeNode *> child_map_t;
    child_map_t                         mChildMap;      // for fast name lookups
    child_map_t::iterator               mChildMapIter;
    child_map_t::iterator               mChildMapEndIter;

    LLXmlTreeNode*                      mParent;
    LLXmlTree*                          mTree;
};

//////////////////////////////////////////////////////////////
// LLXmlTreeParser

class LLXmlTreeParser : public LLXmlParser
{
public:
    LLXmlTreeParser(LLXmlTree* tree);
    virtual ~LLXmlTreeParser();

    bool parseFile(const std::string &path, LLXmlTreeNode** root, bool keep_contents );

protected:
    const std::string& tabs();

    // Overrides from LLXmlParser
    virtual void    startElement(const char *name, const char **attributes);
    virtual void    endElement(const char *name);
    virtual void    characterData(const char *s, int len);
    virtual void    processingInstruction(const char *target, const char *data);
    virtual void    comment(const char *data);
    virtual void    startCdataSection();
    virtual void    endCdataSection();
    virtual void    defaultData(const char *s, int len);
    virtual void    unparsedEntityDecl(
        const char* entity_name,
        const char* base,
        const char* system_id,
        const char* public_id,
        const char* notation_name);

    //template method pattern
    virtual LLXmlTreeNode* CreateXmlTreeNode(const std::string& name, LLXmlTreeNode* parent);

protected:
    LLXmlTree*      mTree;
    LLXmlTreeNode*  mRoot;
    LLXmlTreeNode*  mCurrent;
    bool            mDump;  // Dump parse tree to LL_INFOS() as it is read.
    bool            mKeepContents;
};

#endif  // LL_LLXMLTREE_H
