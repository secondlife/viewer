/** 
 * @file llxmlnode.h
 * @brief LLXMLNode definition
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLXMLNODE_H
#define LL_LLXMLNODE_H

#ifndef XML_STATIC
#define XML_STATIC
#endif
#ifdef LL_STANDALONE
#include <expat.h>
#else
#include "expat/expat.h"
#endif
#include <map>

#include "indra_constants.h"
#include "llpointer.h"
#include "llthread.h"		// LLThreadSafeRefCount
#include "llstring.h"
#include "llstringtable.h"
#include "llfile.h"


class LLVector3;
class LLVector3d;
class LLQuaternion;
class LLUUID;
class LLColor4;
class LLColor4U;


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
typedef std::multimap<std::string, LLXMLNodePtr > LLXMLNodeList;
typedef std::multimap<const LLStringTableEntry *, LLXMLNodePtr > LLXMLChildList;
typedef std::map<const LLStringTableEntry *, LLXMLNodePtr, CompareAttributes> LLXMLAttribList;

class LLColor4;
class LLColor4U;
class LLQuaternion;
class LLVector3;
class LLVector3d;
class LLVector4;
class LLVector4U;

struct LLXMLChildren : public LLThreadSafeRefCount
{
	LLXMLChildList map;			// Map of children names->pointers
	LLXMLNodePtr head;			// Head of the double-linked list
	LLXMLNodePtr tail;			// Tail of the double-linked list
};
typedef LLPointer<LLXMLChildren> LLXMLChildrenPtr;

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
	LLXMLNode(const char* name, BOOL is_attribute);
	LLXMLNode(LLStringTableEntry* name, BOOL is_attribute);
	LLXMLNode(const LLXMLNode& rhs);
	LLXMLNodePtr deepCopy();

	BOOL isNull();

	BOOL deleteChild(LLXMLNode* child);
    void addChild(LLXMLNodePtr new_child, LLXMLNodePtr after_child = LLXMLNodePtr(NULL)); 
    void setParent(LLXMLNodePtr new_parent); // reparent if necessary

    // Serialization
	static bool parseFile(
		const std::string& filename,
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
	static LLXMLNodePtr replaceNode(LLXMLNodePtr node, LLXMLNodePtr replacement_node);
	
	static bool getLayeredXMLNode(LLXMLNodePtr& root, const std::vector<std::string>& paths);
	
	
	// Write standard XML file header:
	// <?xml version="1.0" encoding="utf-8" standalone="yes" ?>
	static void writeHeaderToFile(LLFILE *out_file);
	
	// Write XML to file with one attribute per line.
	// XML escapes values as they are written.
    void writeToFile(LLFILE *out_file, const std::string& indent = std::string(), bool use_type_decorations=true);
    void writeToOstream(std::ostream& output_stream, const std::string& indent = std::string(), bool use_type_decorations=true);

    // Utility
    void findName(const std::string& name, LLXMLNodeList &results);
    void findName(LLStringTableEntry* name, LLXMLNodeList &results);
    void findID(const std::string& id, LLXMLNodeList &results);


    virtual LLXMLNodePtr createChild(const char* name, BOOL is_attribute);
    virtual LLXMLNodePtr createChild(LLStringTableEntry* name, BOOL is_attribute);


    // Getters
    U32 getBoolValue(U32 expected_length, BOOL *array);
    U32 getByteValue(U32 expected_length, U8 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getIntValue(U32 expected_length, S32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getUnsignedValue(U32 expected_length, U32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getLongValue(U32 expected_length, U64 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getFloatValue(U32 expected_length, F32 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getDoubleValue(U32 expected_length, F64 *array, Encoding encoding = ENCODING_DEFAULT);
    U32 getStringValue(U32 expected_length, std::string *array);
    U32 getUUIDValue(U32 expected_length, LLUUID *array);
    U32 getNodeRefValue(U32 expected_length, LLXMLNode **array);

	BOOL hasAttribute(const char* name );

        // these are designed to be more generic versions of the functions
    // rather than relying on LL-types
    bool getAttribute_bool(const char* name, bool& value ); 

	BOOL getAttributeBOOL(const char* name, BOOL& value );
	BOOL getAttributeU8(const char* name, U8& value );
	BOOL getAttributeS8(const char* name, S8& value );
	BOOL getAttributeU16(const char* name, U16& value );
	BOOL getAttributeS16(const char* name, S16& value );
	BOOL getAttributeU32(const char* name, U32& value );
	BOOL getAttributeS32(const char* name, S32& value );
	BOOL getAttributeF32(const char* name, F32& value );
	BOOL getAttributeF64(const char* name, F64& value );
	BOOL getAttributeColor(const char* name, LLColor4& value );
	BOOL getAttributeColor4(const char* name, LLColor4& value );
	BOOL getAttributeColor4U(const char* name, LLColor4U& value );
	BOOL getAttributeVector3(const char* name, LLVector3& value );
	BOOL getAttributeVector3d(const char* name, LLVector3d& value );
	BOOL getAttributeQuat(const char* name, LLQuaternion& value );
	BOOL getAttributeUUID(const char* name, LLUUID& value );
	BOOL getAttributeString(const char* name, std::string& value );

    const ValueType& getType() const { return mType; }
    U32 getLength() const { return mLength; }
    U32 getPrecision() const { return mPrecision; }
    const std::string& getValue() const { return mValue; }
	std::string getSanitizedValue() const;
	std::string getTextContents() const;
    const LLStringTableEntry* getName() const { return mName; }
	BOOL hasName(const char* name) const { return mName == gStringTable.checkStringEntry(name); }
	BOOL hasName(const std::string& name) const { return mName == gStringTable.checkStringEntry(name.c_str()); }
    const std::string& getID() const { return mID; }

    U32 getChildCount() const;
    // getChild returns a Null LLXMLNode (not a NULL pointer) if there is no such child.
    // This child has no value so any getTYPEValue() calls on it will return 0.
    bool getChild(const char* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
    bool getChild(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
    void getChildren(const char* name, LLXMLNodeList &children, BOOL use_default_if_missing = TRUE) const;
    void getChildren(const LLStringTableEntry* name, LLXMLNodeList &children, BOOL use_default_if_missing = TRUE) const;
	
	// recursively finds all children at any level matching name
	void getDescendants(const LLStringTableEntry* name, LLXMLNodeList &children) const;

	bool getAttribute(const char* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);
	bool getAttribute(const LLStringTableEntry* name, LLXMLNodePtr& node, BOOL use_default_if_missing = TRUE);

	S32 getLineNumber();

	// The following skip over attributes
	LLXMLNodePtr getFirstChild() const;
	LLXMLNodePtr getNextSibling() const;

    LLXMLNodePtr getRoot();

	// Setters

	bool setAttributeString(const char* attr, const std::string& value);
	
	void setBoolValue(const BOOL value)	{ setBoolValue(1, &value); }
	void setByteValue(const U8 value, Encoding encoding = ENCODING_DEFAULT) { setByteValue(1, &value, encoding); }
	void setIntValue(const S32 value, Encoding encoding = ENCODING_DEFAULT) { setIntValue(1, &value, encoding); }
	void setUnsignedValue(const U32 value, Encoding encoding = ENCODING_DEFAULT) { setUnsignedValue(1, &value, encoding); }
	void setLongValue(const U64 value, Encoding encoding = ENCODING_DEFAULT) { setLongValue(1, &value, encoding); }
	void setFloatValue(const F32 value, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0) { setFloatValue(1, &value, encoding); }
	void setDoubleValue(const F64 value, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0) { setDoubleValue(1, &value, encoding); }
	void setStringValue(const std::string& value) { setStringValue(1, &value); }
	void setUUIDValue(const LLUUID value) { setUUIDValue(1, &value); }
	void setNodeRefValue(const LLXMLNode *value) { setNodeRefValue(1, &value); }

	void setBoolValue(U32 length, const BOOL *array);
	void setByteValue(U32 length, const U8 *array, Encoding encoding = ENCODING_DEFAULT);
	void setIntValue(U32 length, const S32 *array, Encoding encoding = ENCODING_DEFAULT);
	void setUnsignedValue(U32 length, const U32* array, Encoding encoding = ENCODING_DEFAULT);
	void setLongValue(U32 length, const U64 *array, Encoding encoding = ENCODING_DEFAULT);
	void setFloatValue(U32 length, const F32 *array, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0);
	void setDoubleValue(U32 length, const F64 *array, Encoding encoding = ENCODING_DEFAULT, U32 precision = 0);
	void setStringValue(U32 length, const std::string *array);
	void setUUIDValue(U32 length, const LLUUID *array);
	void setNodeRefValue(U32 length, const LLXMLNode **array);
	void setValue(const std::string& value);
	void setName(const std::string& name);
	void setName(LLStringTableEntry* name);

	void setLineNumber(S32 line_number);

	// Escapes " (quot) ' (apos) & (amp) < (lt) > (gt)
	static std::string escapeXML(const std::string& xml);

	// Set the default node corresponding to this default node
	void setDefault(LLXMLNode *default_node);

	// Find the node within defaults_list which corresponds to this node
	void findDefault(LLXMLNode *defaults_list);

	void updateDefault();

	// Delete any child nodes that aren't among the tree's children, recursive
	void scrubToTree(LLXMLNode *tree);

	BOOL deleteChildren(const std::string& name);
	BOOL deleteChildren(LLStringTableEntry* name);
	void setAttributes(ValueType type, U32 precision, Encoding encoding, U32 length);
// 	void appendValue(const std::string& value); // Unused

	// Unit Testing
	void createUnitTest(S32 max_num_children);
	BOOL performUnitTest(std::string &error_buffer);

protected:
	BOOL removeChild(LLXMLNode* child);

public:
	std::string mID;				// The ID attribute of this node

	XML_Parser *mParser;		// Temporary pointer while loading

	BOOL mIsAttribute;			// Flag is only used for output formatting
	U32 mVersionMajor;			// Version of this tag to use
	U32 mVersionMinor;
	U32 mLength;				// If the length is nonzero, then only return arrays of this length
	U32 mPrecision;				// The number of BITS per array item
	ValueType mType;			// The value type
	Encoding mEncoding;			// The value encoding
	S32 mLineNumber;			// line number in source file, if applicable

	LLXMLNode* mParent;				// The parent node
	LLXMLChildrenPtr mChildren;		// The child nodes
	LLXMLAttribList mAttributes;		// The attribute nodes
	LLXMLNodePtr mPrev;				// Double-linked list previous node
	LLXMLNodePtr mNext;				// Double-linked list next node

	static BOOL sStripEscapedStrings;
	static BOOL sStripWhitespaceValues;
	
protected:
	LLStringTableEntry *mName;		// The name of this node

	// The value of this node (use getters/setters only)
	// Values are not XML-escaped in memory
	// They may contain " (quot) ' (apos) & (amp) < (lt) > (gt)
	std::string mValue;

	LLXMLNodePtr mDefault;		// Mirror node in the default tree

	static const char *skipWhitespace(const char *str);
	static const char *skipNonWhitespace(const char *str);
	static const char *parseInteger(const char *str, U64 *dest, BOOL *is_negative, U32 precision, Encoding encoding);
	static const char *parseFloat(const char *str, F64 *dest, U32 precision, Encoding encoding);

	BOOL isFullyDefault();
};

#endif // LL_LLXMLNODE
