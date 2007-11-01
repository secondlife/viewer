/** 
 * @file llxmlnode.h
 * @brief LLXMLNode definition
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLXMLNODE_H
#define LL_LLXMLNODE_H

#define XML_STATIC
#ifdef LL_STANDALONE
#include <expat.h>
#else
#include "expat/expat.h"
#endif
#include <map>

#include "indra_constants.h"
#include "llmemory.h"
#include "llthread.h"
#include "llstring.h"
#include "llstringtable.h"



struct CompareAttributes
{
	bool operator()(const LLStringTableEntry* const lhs, const LLStringTableEntry* const rhs) const
	{	
		if (lhs == NULL)
			return TRUE;
		if (rhs == NULL)
			return FALSE;

		return strcmp(lhs->mString, rhs->mString) < 0;
	}
};


// Defines a simple node hierarchy for reading and writing task objects

class LLXMLNode;
typedef LLPointer<LLXMLNode> LLXMLNodePtr;
typedef std::multimap<LLString, LLXMLNodePtr > LLXMLNodeList;
typedef std::multimap<const LLStringTableEntry *, LLXMLNodePtr > LLXMLChildList;
typedef std::map<const LLStringTableEntry *, LLXMLNodePtr, CompareAttributes> LLXMLAttribList;

struct LLXMLChildren
{
	LLXMLChildList map;			// Map of children names->pointers
	LLXMLNodePtr head;			// Head of the double-linked list
	LLXMLNodePtr tail;			// Tail of the double-linked list
};

class LLXMLNode : public LLThreadSafeRefCount
{
public:
	enum ValueType
	{
		TYPE_CONTAINER,		// A node which contains nodes
		TYPE_UNKNOWN,		// A node loaded from file without a specified type
		TYPE_BOOLEAN,		// "true" or "false"
		TYPE_INTEGER,		// any integer type: U8, U32, S32, U64, etc.
		TYPE_FLOAT,			// any floating point type: F32, F64
		TYPE_STRING,		// a string
		TYPE_UUID,			// a UUID
		TYPE_NODEREF,		// the ID of another node in the hierarchy to reference
	};

	enum Encoding
	{
		ENCODING_DEFAULT = 0,
		ENCODING_DECIMAL,
		ENCODING_HEX,
		// ENCODING_BASE32, // Not implemented yet
	};

protected:
	~LLXMLNode();

public:
	LLXMLNode();
	LLXMLNode(const LLString& name, BOOL is_attribute);
	LLXMLNode(LLStringTableEntry* name, BOOL is_attribute);

	BOOL isNull();

	BOOL deleteChild(LLXMLNode* child);
    void addChild(LLXMLNodePtr new_parent); 
    void setParent(LLXMLNodePtr new_parent); // reparent if necessary

    // Serialization
	static bool parseFile(
		LLString filename,
		LLXMLNodePtr& node, 
		LLXMLNode* defaults_tree);
	static bool parseBuffer(
		U8* buffer,
		U32 length,
		LLXMLNodePtr& node, 
		LLXMLNode* defaults);
	static bool parseStream(
		std::istream& str,
		LLXMLNodePtr& node, 
		LLXMLNode* defaults);
	static bool updateNode(
	LLXMLNodePtr& node,
	LLXMLNodePtr& update_node);
	static void writeHeaderToFile(FILE *fOut);
    void writeToFile(FILE *fOut, LLString indent = LLString());
    void writeToOstream(std::ostream& output_stream, const LLString& indent = LLString());

    // Utility
    void findName(const LLString& name, LLXMLNodeList &results);
    void findName(LLStringTableEntry* name, LLXMLNodeList &results);
    void findID(const LLString& id, LLXMLNodeList &results);


    virtual LLXMLNodePtr createChild(const LLString& name, BOOL is_attribute);
    virtual LLXMLNodePtr createChild(LLStringTableEntry* name, BOOL is_attribute);


    // Getters
    U32 getBoolValue(U32 expected_length, BOOL *array);
    U32 getByteValue(U32 expected_length, U8 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getIntValue(U32 expected_length, S32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getUnsignedValue(U32 expected_length, U32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getLongValue(U32 expected_length, U64 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getFloatValue(U32 expected_length, F32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getDoubleValue(U32 expected_length, F64 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getStringValue(U32 expected_length, LLString *array);
    U32 getUUIDValue(U32 expected_length, LLUUID *array);
    U32 getNodeRefValue(U32 expected_length, LLXMLNode **array);

	BOOL hasAttribute(const LLString& name );

	BOOL getAttributeBOOL(const LLString& name, BOOL& value );
	BOOL getAttributeU8(const LLString& name, U8& value );
	BOOL getAttributeS8(const LLString& name, S8& value );
	BOOL getAttributeU16(const LLString& name, U16& value );
	BOOL getAttributeS16(const LLString& name, S16& value );
	BOOL getAttributeU32(const LLString& name, U32& value );
	BOOL getAttributeS32(const LLString& name, S32& value );
	BOOL getAttributeF32(const LLString& name, F32& value );
	BOOL getAttributeF64(const LLString& name, F64& value );
	BOOL getAttributeColor(const LLString& name, LLColor4& value );
	BOOL getAttributeColor4(const LLString& name, LLColor4& value );
	BOOL getAttributeColor4U(const LLString& name, LLColor4U& value );
	BOOL getAttributeVector3(const LLString& name, LLVector3& value );
	BOOL getAttributeVector3d(const LLString& name, LLVector3d& value );
	BOOL getAttributeQuat(const LLString& name, LLQuaternion& value );
	BOOL getAttributeUUID(const LLString& name, LLUUID& value );
	BOOL getAttributeString(const LLString& name, LLString& value );

    const ValueType& getType() const { return mType; }
    U32 getLength() const { return mLength; }
    U32 getPrecision() const { return mPrecision; }
    const LLString& getValue() const { return mValue; }
	LLString getTextContents() const;
    const LLStringTableEntry* getName() const { return mName; }
	BOOL hasName(const char* name) const { return mName == gStringTable.checkStringEntry(name); }
	BOOL hasName(LLString name) const { return mName == gStringTable.checkStringEntry(name.c_str()); }
    const LLString& getID() const { return mID; }

    U32 getChildCount() const;
    // getChild returns a Null LLXMLNode (not a NULL pointer) if there is no such child.
    // This child has no value so any getTYPEValue() calls on it will return 0.
    bool getChild(const LLString& name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
    bool getChild(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
    void getChildren(const LLString& name, LLXMLNodeList &children, BOOL use_default_if_missing = TRUE) const;
    void getChildren(const LLStringTableEntry* name, LLXMLNodeList &children, BOOL use_default_if_missing = TRUE) const;

	bool getAttribute(const LLString& name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
	bool getAttribute(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);

	// The following skip over attributes
	LLXMLNodePtr getFirstChild();
	LLXMLNodePtr getNextSibling();

    LLXMLNodePtr getRoot();

	// Setters

	bool setAttributeString(const LLString& attr, const LLString& value);
	
	void setBoolValue(const BOOL value)	{ setBoolValue(1, &value); }
	void setByteValue(const U8 value, Encoding encoding = ENCODING_DEFAULT) { setByteValue(1, &value, encoding); }
	void setIntValue(const S32 value, Encoding encoding = ENCODING_DEFAULT) { setIntValue(1, &value, encoding); }
	void setUnsignedValue(const U32 value, Encoding encoding = ENCODING_DEFAULT) { setUnsignedValue(1, &value, encoding); }
	void setLongValue(const U64 value, Encoding encoding = ENCODING_DEFAULT) { setLongValue(1, &value, encoding); }
	void setFloatValue(const F32 value, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0) { setFloatValue(1, &value, encoding); }
	void setDoubleValue(const F64 value, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0) { setDoubleValue(1, &value, encoding); }
	void setStringValue(const LLString value) { setStringValue(1, &value); }
	void setUUIDValue(const LLUUID value) { setUUIDValue(1, &value); }
	void setNodeRefValue(const LLXMLNode *value) { setNodeRefValue(1, &value); }

	void setBoolValue(U32 length, const BOOL *array);
	void setByteValue(U32 length, const U8 *array, Encoding encoding = ENCODING_DEFAULT);
	void setIntValue(U32 length, const S32 *array, Encoding encoding = ENCODING_DEFAULT);
	void setUnsignedValue(U32 length, const U32* array, Encoding encoding = ENCODING_DEFAULT);
	void setLongValue(U32 length, const U64 *array, Encoding encoding = ENCODING_DEFAULT);
	void setFloatValue(U32 length, const F32 *array, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0);
	void setDoubleValue(U32 length, const F64 *array, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0);
	void setStringValue(U32 length, const LLString *array);
	void setUUIDValue(U32 length, const LLUUID *array);
	void setNodeRefValue(U32 length, const LLXMLNode **array);
	void setValue(const LLString& value);
	void setName(const LLString& name);
	void setName(LLStringTableEntry* name);

	// Escapes " (quot) ' (apos) & (amp) < (lt) > (gt)
	// TomY TODO: Make this private
	static LLString escapeXML(const LLString& xml);

	// Set the default node corresponding to this default node
	void setDefault(LLXMLNode *default_node);

	// Find the node within defaults_list which corresponds to this node
	void findDefault(LLXMLNode *defaults_list);

	void updateDefault();

	// Delete any child nodes that aren't among the tree's children, recursive
	void scrubToTree(LLXMLNode *tree);

	BOOL deleteChildren(const LLString& name);
	BOOL deleteChildren(LLStringTableEntry* name);
	void setAttributes(ValueType type, U32 precision, Encoding encoding, U32 length);
	void appendValue(const LLString& value);

	// Unit Testing
	void createUnitTest(S32 max_num_children);
	BOOL performUnitTest(LLString &error_buffer);

protected:
	BOOL removeChild(LLXMLNode* child);

public:
	LLString mID;				// The ID attribute of this node

	XML_Parser *mParser;		// Temporary pointer while loading

	BOOL mIsAttribute;			// Flag is only used for output formatting
	U32 mVersionMajor;			// Version of this tag to use
	U32 mVersionMinor;
	U32 mLength;				// If the length is nonzero, then only return arrays of this length
	U32 mPrecision;				// The number of BITS per array item
	ValueType mType;			// The value type
	Encoding mEncoding;			// The value encoding

	LLXMLNode* mParent;				// The parent node
	LLXMLChildren* mChildren;		// The child nodes
	LLXMLAttribList mAttributes;		// The attribute nodes
	LLXMLNodePtr mPrev;				// Double-linked list previous node
	LLXMLNodePtr mNext;				// Double-linked list next node

	static BOOL sStripEscapedStrings;
	static BOOL sStripWhitespaceValues;
	
protected:
	LLStringTableEntry *mName;		// The name of this node
	LLString mValue;			// The value of this node (use getters/setters only)

	LLXMLNodePtr mDefault;		// Mirror node in the default tree

	static const char *skipWhitespace(const char *str);
	static const char *skipNonWhitespace(const char *str);
	static const char *parseInteger(const char *str, U64 *dest, BOOL *is_negative, U32 precision, Encoding encoding);
	static const char *parseFloat(const char *str, F64 *dest, U32 precision, Encoding encoding);

	BOOL isFullyDefault();
};

#endif // LL_LLXMLNODE
