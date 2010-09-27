/** 
 * @file lscript_tree.h
 * @brief provides the classes required to build lscript's abstract syntax tree and symbol table
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

#ifndef LL_LSCRIPT_TREE_H
#define LL_LSCRIPT_TREE_H

#include "v3math.h"
#include "llquaternion.h"
#include "linked_lists.h"
#include "lscript_error.h"
#include "lscript_typecheck.h"
#include "lscript_byteformat.h"


// Nota Bene:  Class destructors don't delete pointed to classes because it isn't guaranteed that lex/yacc will build
//				complete data structures.  Instead various chunks that are allocated are stored and deleted by allocation lists

class LLScriptType : public LLScriptFilePosition
{
public:
	LLScriptType(S32 line, S32 col, LSCRIPTType type)
		: LLScriptFilePosition(line, col), mType(type)
	{
	}

	~LLScriptType() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTType	mType;
};

// contains a literal or constant value
class LLScriptConstant : public LLScriptFilePosition
{
public:
	LLScriptConstant(S32 line, S32 col, LSCRIPTType type)
		: LLScriptFilePosition(line, col), mType(type)
	{
	}

	virtual ~LLScriptConstant() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTType mType;
};

class LLScriptConstantInteger : public LLScriptConstant
{
public:
	LLScriptConstantInteger(S32 line, S32 col, S32 value)
		: LLScriptConstant(line, col, LST_INTEGER), mValue(value)
	{
	}

	~LLScriptConstantInteger() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	S32 mValue;
};

class LLScriptConstantFloat : public LLScriptConstant
{
public:
	LLScriptConstantFloat(S32 line, S32 col, F32 value)
		: LLScriptConstant(line, col, LST_FLOATINGPOINT), mValue(value)
	{
	}

	~LLScriptConstantFloat() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	F32 mValue;
};

class LLScriptConstantString : public LLScriptConstant
{
public:
	LLScriptConstantString(S32 line, S32 col, char *value)
		: LLScriptConstant(line, col, LST_STRING), mValue(value)
	{
	}

	~LLScriptConstantString() 
	{
		delete [] mValue;
		mValue = NULL;
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	char *mValue;
};

// container for individual identifiers
class LLScriptIdentifier : public LLScriptFilePosition
{
public:
	LLScriptIdentifier(S32 line, S32 col, char *name, LLScriptType *type = NULL)
		: LLScriptFilePosition(line, col), mName(name), mScopeEntry(NULL), mType(type)
	{
	}

	~LLScriptIdentifier() 
	{
		delete [] mName;
		mName = NULL;
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	char					*mName;
	LLScriptScopeEntry		*mScopeEntry;
	LLScriptType			*mType;
};

typedef enum e_lscript_simple_assignable_type
{
	LSSAT_NULL,
	LSSAT_IDENTIFIER,
	LSSAT_CONSTANT,
	LSSAT_VECTOR_CONSTANT,
	LSSAT_QUATERNION_CONSTANT,
	LSSAT_LIST_CONSTANT,
	LSSAT_EOF
} LSCRIPTSimpleAssignableType;

class LLScriptSimpleAssignable : public LLScriptFilePosition
{
public:
	LLScriptSimpleAssignable(S32 line, S32 col, LSCRIPTSimpleAssignableType type)
		: LLScriptFilePosition(line, col), mType(type), mNextp(NULL)
	{
	}

	void addAssignable(LLScriptSimpleAssignable *assign);

	virtual ~LLScriptSimpleAssignable() 
	{
		// don't delete next pointer because we're going to store allocation lists and delete from those
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTSimpleAssignableType		mType;
	LLScriptSimpleAssignable		*mNextp;
};

class LLScriptSAIdentifier : public LLScriptSimpleAssignable
{
public:
	LLScriptSAIdentifier(S32 line, S32 col, LLScriptIdentifier *identifier)
		: LLScriptSimpleAssignable(line, col, LSSAT_IDENTIFIER), mIdentifier(identifier)
	{
	}

	~LLScriptSAIdentifier() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier *mIdentifier;
};

class LLScriptSAConstant : public LLScriptSimpleAssignable
{
public:
	LLScriptSAConstant(S32 line, S32 col, LLScriptConstant *constant)
		: LLScriptSimpleAssignable(line, col, LSSAT_CONSTANT), mConstant(constant)
	{
	}

	~LLScriptSAConstant() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptConstant *mConstant;
};

class LLScriptSAVector : public LLScriptSimpleAssignable
{
public:
	LLScriptSAVector(S32 line, S32 col, LLScriptSimpleAssignable *e1, 
										LLScriptSimpleAssignable *e2, 
										LLScriptSimpleAssignable *e3)
		: LLScriptSimpleAssignable(line, col, LSSAT_VECTOR_CONSTANT), 
			mEntry1(e1), mEntry2(e2), mEntry3(e3)
	{
	}

	~LLScriptSAVector() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptSimpleAssignable *mEntry1;
	LLScriptSimpleAssignable *mEntry2;
	LLScriptSimpleAssignable *mEntry3;
};

class LLScriptSAQuaternion : public LLScriptSimpleAssignable
{
public:
	LLScriptSAQuaternion(S32 line, S32 col, LLScriptSimpleAssignable *e1, 
											LLScriptSimpleAssignable *e2, 
											LLScriptSimpleAssignable *e3, 
											LLScriptSimpleAssignable *e4)
		: LLScriptSimpleAssignable(line, col, LSSAT_QUATERNION_CONSTANT), 
			mEntry1(e1), mEntry2(e2), mEntry3(e3), mEntry4(e4)
	{
	}

	~LLScriptSAQuaternion() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptSimpleAssignable *mEntry1;
	LLScriptSimpleAssignable *mEntry2;
	LLScriptSimpleAssignable *mEntry3;
	LLScriptSimpleAssignable *mEntry4;
};

class LLScriptSAList : public LLScriptSimpleAssignable
{
public:
	LLScriptSAList(S32 line, S32 col, LLScriptSimpleAssignable *elist)
		: LLScriptSimpleAssignable(line, col, LSSAT_QUATERNION_CONSTANT), mEntryList(elist)
	{
	}

	~LLScriptSAList() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptSimpleAssignable	*mEntryList;
};

// global variables
class LLScriptGlobalVariable : public LLScriptFilePosition
{
public:
	LLScriptGlobalVariable(S32 line, S32 col, LLScriptType *type,
											  LLScriptIdentifier *identifier,
											  LLScriptSimpleAssignable *assignable)
		: LLScriptFilePosition(line, col), mType(type), mIdentifier(identifier), mAssignable(assignable), mNextp(NULL), mAssignableType(LST_NULL)
	{
	}

	void addGlobal(LLScriptGlobalVariable *global);

	~LLScriptGlobalVariable() 
	{
	}
	
	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptType				*mType;
	LLScriptIdentifier			*mIdentifier;
	LLScriptSimpleAssignable	*mAssignable;
	LLScriptGlobalVariable		*mNextp;
	LSCRIPTType					mAssignableType;
};

// events

class LLScriptEvent : public LLScriptFilePosition
{
public:
	LLScriptEvent(S32 line, S32 col, LSCRIPTStateEventType type)
		: LLScriptFilePosition(line, col), mType(type)
	{
	}

	virtual ~LLScriptEvent()
	{
		// don't delete next pointer because we're going to store allocation lists and delete from those
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTStateEventType	mType;
};

class LLScriptStateEntryEvent : public LLScriptEvent
{
public:
	LLScriptStateEntryEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_STATE_ENTRY)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptStateEntryEvent() {}
};

class LLScriptStateExitEvent : public LLScriptEvent
{
public:
	LLScriptStateExitEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_STATE_EXIT)
	{
	}

	~LLScriptStateExitEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();
};

class LLScriptTouchStartEvent : public LLScriptEvent
{
public:
	LLScriptTouchStartEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_TOUCH_START), mCount(count)
	{
	}

	~LLScriptTouchStartEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptTouchEvent : public LLScriptEvent
{
public:
	LLScriptTouchEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_TOUCH), mCount(count)
	{
	}

	~LLScriptTouchEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptTouchEndEvent : public LLScriptEvent
{
public:
	LLScriptTouchEndEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_TOUCH_END), mCount(count)
	{
	}

	~LLScriptTouchEndEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptCollisionStartEvent : public LLScriptEvent
{
public:
	LLScriptCollisionStartEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_COLLISION_START), mCount(count)
	{
	}

	~LLScriptCollisionStartEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptCollisionEvent : public LLScriptEvent
{
public:
	LLScriptCollisionEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_COLLISION), mCount(count)
	{
	}

	~LLScriptCollisionEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptCollisionEndEvent : public LLScriptEvent
{
public:
	LLScriptCollisionEndEvent(S32 line, S32 col, LLScriptIdentifier *count)
		: LLScriptEvent(line, col, LSTT_COLLISION_END), mCount(count)
	{
	}

	~LLScriptCollisionEndEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mCount;
};

class LLScriptLandCollisionStartEvent : public LLScriptEvent
{
public:
	LLScriptLandCollisionStartEvent(S32 line, S32 col, LLScriptIdentifier *pos)
		: LLScriptEvent(line, col, LSTT_LAND_COLLISION_START), mPosition(pos)
	{
	}

	~LLScriptLandCollisionStartEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mPosition;
};

class LLScriptLandCollisionEvent : public LLScriptEvent
{
public:
	LLScriptLandCollisionEvent(S32 line, S32 col, LLScriptIdentifier *pos)
		: LLScriptEvent(line, col, LSTT_LAND_COLLISION), mPosition(pos)
	{
	}

	~LLScriptLandCollisionEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mPosition;
};

class LLScriptLandCollisionEndEvent : public LLScriptEvent
{
public:
	LLScriptLandCollisionEndEvent(S32 line, S32 col, LLScriptIdentifier *pos)
		: LLScriptEvent(line, col, LSTT_LAND_COLLISION_END), mPosition(pos)
	{
	}

	~LLScriptLandCollisionEndEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mPosition;
};

class LLScriptInventoryEvent : public LLScriptEvent
{
public:
	LLScriptInventoryEvent(S32 line, S32 col, LLScriptIdentifier *change)
		: LLScriptEvent(line, col, LSTT_INVENTORY), mChange(change)
	{
	}

	~LLScriptInventoryEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mChange;
};

class LLScriptAttachEvent : public LLScriptEvent
{
public:
	LLScriptAttachEvent(S32 line, S32 col, LLScriptIdentifier *attach)
		: LLScriptEvent(line, col, LSTT_ATTACH), mAttach(attach)
	{
	}

	~LLScriptAttachEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mAttach;
};

class LLScriptDataserverEvent : public LLScriptEvent
{
public:
	LLScriptDataserverEvent(S32 line, S32 col, LLScriptIdentifier *id, LLScriptIdentifier *data)
		: LLScriptEvent(line, col, LSTT_DATASERVER), mID(id), mData(data)
	{
	}

	~LLScriptDataserverEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mID;
	LLScriptIdentifier	*mData;
};

class LLScriptTimerEvent : public LLScriptEvent
{
public:
	LLScriptTimerEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_TIMER)
	{
	}

	~LLScriptTimerEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();
};

class LLScriptMovingStartEvent : public LLScriptEvent
{
public:
	LLScriptMovingStartEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_MOVING_START)
	{
	}

	~LLScriptMovingStartEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();
};

class LLScriptMovingEndEvent : public LLScriptEvent
{
public:
	LLScriptMovingEndEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_MOVING_END)
	{
	}

	~LLScriptMovingEndEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();
};

class LLScriptRTPEvent : public LLScriptEvent
{
public:
	LLScriptRTPEvent(S32 line, S32 col, LLScriptIdentifier *rtperm)
		: LLScriptEvent(line, col, LSTT_RTPERMISSIONS), mRTPermissions(rtperm) 
	{
	}

	~LLScriptRTPEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mRTPermissions;
};

class LLScriptChatEvent : public LLScriptEvent
{
public:
	LLScriptChatEvent(S32 line, S32 col, LLScriptIdentifier *channel, LLScriptIdentifier *name, LLScriptIdentifier *id, LLScriptIdentifier *message)
		: LLScriptEvent(line, col, LSTT_CHAT), mChannel(channel), mName(name), mID(id), mMessage(message)
	{
	}

	~LLScriptChatEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mChannel;
	LLScriptIdentifier	*mName;
	LLScriptIdentifier	*mID;
	LLScriptIdentifier	*mMessage;
};

class LLScriptObjectRezEvent : public LLScriptEvent
{
public:
	LLScriptObjectRezEvent(S32 line, S32 col, LLScriptIdentifier *id)
		: LLScriptEvent(line, col, LSTT_OBJECT_REZ), mID(id)
	{
	}

	~LLScriptObjectRezEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mID;
};

class LLScriptSensorEvent : public LLScriptEvent
{
public:
	LLScriptSensorEvent(S32 line, S32 col, LLScriptIdentifier *number)
		: LLScriptEvent(line, col, LSTT_SENSOR), mNumber(number)
	{
	}

	~LLScriptSensorEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mNumber;
};

class LLScriptControlEvent : public LLScriptEvent
{
public:
	LLScriptControlEvent(S32 line, S32 col, LLScriptIdentifier *name, LLScriptIdentifier *levels, LLScriptIdentifier *edges)
		: LLScriptEvent(line, col, LSTT_CONTROL), mName(name), mLevels(levels), mEdges(edges)
	{
	}

	~LLScriptControlEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mName;
	LLScriptIdentifier	*mLevels;
	LLScriptIdentifier	*mEdges;
};

class LLScriptLinkMessageEvent : public LLScriptEvent
{
public:
	LLScriptLinkMessageEvent(S32 line, S32 col, LLScriptIdentifier *sender, LLScriptIdentifier *num, LLScriptIdentifier *str, LLScriptIdentifier *id)
		: LLScriptEvent(line, col, LSTT_LINK_MESSAGE), mSender(sender), mNum(num), mStr(str), mID(id)
	{
	}

	~LLScriptLinkMessageEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mSender;
	LLScriptIdentifier	*mNum;
	LLScriptIdentifier	*mStr;
	LLScriptIdentifier	*mID;
};

class LLScriptRemoteEvent : public LLScriptEvent
{
public:
	LLScriptRemoteEvent(S32 line, S32 col, LLScriptIdentifier *type, LLScriptIdentifier *channel, LLScriptIdentifier *message_id, LLScriptIdentifier *sender, LLScriptIdentifier *int_val, LLScriptIdentifier *str_val)
		: LLScriptEvent(line, col, LSTT_REMOTE_DATA), mType(type), mChannel(channel), mMessageID(message_id), mSender(sender), mIntVal(int_val), mStrVal(str_val)
	{
	}

	~LLScriptRemoteEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mType;
	LLScriptIdentifier	*mChannel;
	LLScriptIdentifier	*mMessageID;
	LLScriptIdentifier	*mSender;
	LLScriptIdentifier	*mIntVal;
	LLScriptIdentifier	*mStrVal;
};

class LLScriptHTTPResponseEvent : public LLScriptEvent
{
public:
	LLScriptHTTPResponseEvent(S32 line, S32 col,
		LLScriptIdentifier *request_id,
		LLScriptIdentifier *status,
		LLScriptIdentifier *metadata,
		LLScriptIdentifier *body)
		: LLScriptEvent(line, col, LSTT_HTTP_RESPONSE),
		 mRequestId(request_id), mStatus(status), mMetadata(metadata), mBody(body)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass,
		LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope,
		LSCRIPTType &type, LSCRIPTType basetype, U64 &count,
		LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap,
		S32 stacksize, LLScriptScopeEntry *entry,
		S32 entrycount, LLScriptLibData **ldata);
		
	S32 getSize();

	LLScriptIdentifier	*mRequestId;
	LLScriptIdentifier	*mStatus;
	LLScriptIdentifier	*mMetadata;
	LLScriptIdentifier	*mBody;
};

class LLScriptHTTPRequestEvent : public LLScriptEvent
{
public:
	LLScriptHTTPRequestEvent(S32 line, S32 col,
		LLScriptIdentifier *request_id,
		LLScriptIdentifier *method,
		LLScriptIdentifier *body)
		: LLScriptEvent(line, col, LSTT_HTTP_REQUEST),
		 mRequestId(request_id), mMethod(method), mBody(body)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass,
		LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope,
		LSCRIPTType &type, LSCRIPTType basetype, U64 &count,
		LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap,
		S32 stacksize, LLScriptScopeEntry *entry,
		S32 entrycount, LLScriptLibData **ldata);
		
	S32 getSize();

	LLScriptIdentifier	*mRequestId;
	LLScriptIdentifier	*mMethod;
	LLScriptIdentifier	*mBody;
};

class LLScriptRezEvent : public LLScriptEvent
{
public:
	LLScriptRezEvent(S32 line, S32 col, LLScriptIdentifier *start_param)
		: LLScriptEvent(line, col, LSTT_REZ), mStartParam(start_param)
	{
	}
	~LLScriptRezEvent() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mStartParam;
};

class LLScriptNoSensorEvent : public LLScriptEvent
{
public:
	LLScriptNoSensorEvent(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_NO_SENSOR)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptNoSensorEvent() {}
};

class LLScriptAtTarget : public LLScriptEvent
{
public:
	LLScriptAtTarget(S32 line, S32 col, LLScriptIdentifier *tnumber, LLScriptIdentifier *tpos, LLScriptIdentifier *ourpos)
		: LLScriptEvent(line, col, LSTT_AT_TARGET), mTargetNumber(tnumber), mTargetPosition(tpos), mOurPosition(ourpos)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptAtTarget() {}

	LLScriptIdentifier	*mTargetNumber;
	LLScriptIdentifier	*mTargetPosition;
	LLScriptIdentifier	*mOurPosition;
};

class LLScriptNotAtTarget : public LLScriptEvent
{
public:
	LLScriptNotAtTarget(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_NOT_AT_TARGET)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptNotAtTarget() {}
};

class LLScriptAtRotTarget : public LLScriptEvent
{
public:
	LLScriptAtRotTarget(S32 line, S32 col, LLScriptIdentifier *tnumber, LLScriptIdentifier *trot, LLScriptIdentifier *ourrot)
		: LLScriptEvent(line, col, LSTT_AT_ROT_TARGET), mTargetNumber(tnumber), mTargetRotation(trot), mOurRotation(ourrot)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptAtRotTarget() {}

	LLScriptIdentifier	*mTargetNumber;
	LLScriptIdentifier	*mTargetRotation;
	LLScriptIdentifier	*mOurRotation;
};

class LLScriptNotAtRotTarget : public LLScriptEvent
{
public:
	LLScriptNotAtRotTarget(S32 line, S32 col)
		: LLScriptEvent(line, col, LSTT_NOT_AT_ROT_TARGET)
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	~LLScriptNotAtRotTarget() {}
};

class LLScriptMoneyEvent : public LLScriptEvent
{
public:
	LLScriptMoneyEvent(S32 line, S32 col, LLScriptIdentifier *name, LLScriptIdentifier *amount)
		: LLScriptEvent(line, col, LSTT_MONEY), mName(name), mAmount(amount)
	{
	}

	~LLScriptMoneyEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mName;
	LLScriptIdentifier	*mAmount;
};

class LLScriptEmailEvent : public LLScriptEvent
{
public:
	LLScriptEmailEvent(S32 line, S32 col, LLScriptIdentifier *time, LLScriptIdentifier *address, LLScriptIdentifier *subject, LLScriptIdentifier *body, LLScriptIdentifier *number)
		: LLScriptEvent(line, col, LSTT_EMAIL), mTime(time), mAddress(address), mSubject(subject), mBody(body), mNumber(number)
	{
	}

	~LLScriptEmailEvent() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mTime;
	LLScriptIdentifier	*mAddress;
	LLScriptIdentifier	*mSubject;
	LLScriptIdentifier	*mBody;
	LLScriptIdentifier	*mNumber;
};


class LLScriptExpression : public LLScriptFilePosition
{
public:
	LLScriptExpression(S32 line, S32 col, LSCRIPTExpressionType type)
		: LLScriptFilePosition(line, col), mType(type), mNextp(NULL), mLeftType(LST_NULL), mRightType(LST_NULL), mReturnType(LST_NULL)
	{
	}

	void addExpression(LLScriptExpression *expression);

	virtual ~LLScriptExpression() 
	{
		// don't delete next pointer because we're going to store allocation lists and delete from those
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTExpressionType	mType;
	LLScriptExpression		*mNextp;
	LSCRIPTType				mLeftType, mRightType, mReturnType;

};

class LLScriptForExpressionList : public LLScriptExpression
{
public:
	LLScriptForExpressionList(S32 line, S32 col, LLScriptExpression *first, LLScriptExpression *second)
		: LLScriptExpression(line, col, LET_FOR_EXPRESSION_LIST), mFirstp(first), mSecondp(second)
	{
	}

	~LLScriptForExpressionList() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mFirstp;
	LLScriptExpression	*mSecondp;
};

class LLScriptFuncExpressionList : public LLScriptExpression
{
public:
	LLScriptFuncExpressionList(S32 line, S32 col, LLScriptExpression *first, LLScriptExpression *second)
		: LLScriptExpression(line, col, LET_FUNC_EXPRESSION_LIST), mFirstp(first), mSecondp(second)
	{
	}

	~LLScriptFuncExpressionList() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mFirstp;
	LLScriptExpression	*mSecondp;
};

class LLScriptListExpressionList : public LLScriptExpression
{
public:
	LLScriptListExpressionList(S32 line, S32 col, LLScriptExpression *first, LLScriptExpression *second)
		: LLScriptExpression(line, col, LET_LIST_EXPRESSION_LIST), mFirstp(first), mSecondp(second)
	{
	}

	~LLScriptListExpressionList() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mFirstp;
	LLScriptExpression	*mSecondp;
};

class LLScriptLValue : public LLScriptExpression
{
public:
	LLScriptLValue(S32 line, S32 col, LLScriptIdentifier *identifier, LLScriptIdentifier *accessor)
		: LLScriptExpression(line, col, LET_LVALUE), mOffset(0), mIdentifier(identifier), mAccessor(accessor)
	{
	}

	~LLScriptLValue() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	S32					mOffset;
	LLScriptIdentifier	*mIdentifier;
	LLScriptIdentifier	*mAccessor;
};

class LLScriptAssignment : public LLScriptExpression
{
public:
	LLScriptAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_ASSIGNMENT), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptAddAssignment : public LLScriptExpression
{
public:
	LLScriptAddAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_ADD_ASSIGN), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptAddAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptSubAssignment : public LLScriptExpression
{
public:
	LLScriptSubAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_SUB_ASSIGN), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptSubAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptMulAssignment : public LLScriptExpression
{
public:
	LLScriptMulAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_MUL_ASSIGN), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptMulAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptDivAssignment : public LLScriptExpression
{
public:
	LLScriptDivAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_DIV_ASSIGN), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptDivAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptModAssignment : public LLScriptExpression
{
public:
	LLScriptModAssignment(S32 line, S32 col, LLScriptExpression *lvalue, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_MOD_ASSIGN), mLValue(lvalue), mRightSide(rightside)
	{
	}

	~LLScriptModAssignment() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLValue;
	LLScriptExpression	*mRightSide;
};

class LLScriptEquality : public LLScriptExpression
{
public:
	LLScriptEquality(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_EQUALITY), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptEquality() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptNotEquals : public LLScriptExpression
{
public:
	LLScriptNotEquals(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_NOT_EQUALS), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptNotEquals() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptLessEquals : public LLScriptExpression
{
public:
	LLScriptLessEquals(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_LESS_EQUALS), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptLessEquals() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptGreaterEquals : public LLScriptExpression
{
public:
	LLScriptGreaterEquals(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_GREATER_EQUALS), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptGreaterEquals() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptLessThan : public LLScriptExpression
{
public:
	LLScriptLessThan(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_LESS_THAN), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptLessThan() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptGreaterThan : public LLScriptExpression
{
public:
	LLScriptGreaterThan(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_GREATER_THAN), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptGreaterThan() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptPlus : public LLScriptExpression
{
public:
	LLScriptPlus(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_PLUS), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptPlus() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptMinus : public LLScriptExpression
{
public:
	LLScriptMinus(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_MINUS), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptMinus() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptTimes : public LLScriptExpression
{
public:
	LLScriptTimes(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_TIMES), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptTimes() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptDivide : public LLScriptExpression
{
public:
	LLScriptDivide(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_DIVIDE), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptDivide() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptMod : public LLScriptExpression
{
public:
	LLScriptMod(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_MOD), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptMod() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptBitAnd : public LLScriptExpression
{
public:
	LLScriptBitAnd(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_BIT_AND), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptBitAnd() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptBitOr : public LLScriptExpression
{
public:
	LLScriptBitOr(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_BIT_OR), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptBitOr() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptBitXor : public LLScriptExpression
{
public:
	LLScriptBitXor(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_BIT_XOR), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptBitXor() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptBooleanAnd : public LLScriptExpression
{
public:
	LLScriptBooleanAnd(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_BOOLEAN_AND), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptBooleanAnd() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptBooleanOr : public LLScriptExpression
{
public:
	LLScriptBooleanOr(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_BOOLEAN_OR), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptBooleanOr() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptShiftLeft : public LLScriptExpression
{
public:
	LLScriptShiftLeft(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_SHIFT_LEFT), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptShiftLeft() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptShiftRight : public LLScriptExpression
{
public:
	LLScriptShiftRight(S32 line, S32 col, LLScriptExpression *leftside, LLScriptExpression *rightside)
		: LLScriptExpression(line, col, LET_SHIFT_RIGHT), mLeftSide(leftside), mRightSide(rightside)
	{
	}

	~LLScriptShiftRight() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mLeftSide;
	LLScriptExpression	*mRightSide;
};

class LLScriptParenthesis : public LLScriptExpression
{
public:
	LLScriptParenthesis(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_PARENTHESIS), mExpression(expression)
	{
	}

	~LLScriptParenthesis() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptUnaryMinus : public LLScriptExpression
{
public:
	LLScriptUnaryMinus(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_UNARY_MINUS), mExpression(expression)
	{
	}

	~LLScriptUnaryMinus() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptBooleanNot : public LLScriptExpression
{
public:
	LLScriptBooleanNot(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_BOOLEAN_NOT), mExpression(expression)
	{
	}

	~LLScriptBooleanNot() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptBitNot : public LLScriptExpression
{
public:
	LLScriptBitNot(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_BIT_NOT), mExpression(expression)
	{
	}

	~LLScriptBitNot() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptPreIncrement : public LLScriptExpression
{
public:
	LLScriptPreIncrement(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_PRE_INCREMENT), mExpression(expression)
	{
	}

	~LLScriptPreIncrement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptPreDecrement : public LLScriptExpression
{
public:
	LLScriptPreDecrement(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_PRE_DECREMENT), mExpression(expression)
	{
	}

	~LLScriptPreDecrement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptTypeCast : public LLScriptExpression
{
public:
	LLScriptTypeCast(S32 line, S32 col, LLScriptType *type, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_CAST), mType(type), mExpression(expression)
	{
	}

	~LLScriptTypeCast() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptType		*mType;
	LLScriptExpression	*mExpression;
};

class LLScriptVectorInitializer : public LLScriptExpression
{
public:
	LLScriptVectorInitializer(S32 line, S32 col, LLScriptExpression *expression1, 
												 LLScriptExpression *expression2, 
												 LLScriptExpression *expression3)
		: LLScriptExpression(line, col, LET_VECTOR_INITIALIZER), 
			mExpression1(expression1),
			mExpression2(expression2),
			mExpression3(expression3)
	{
	}

	~LLScriptVectorInitializer() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression1;
	LLScriptExpression	*mExpression2;
	LLScriptExpression	*mExpression3;
};

class LLScriptQuaternionInitializer : public LLScriptExpression
{
public:
	LLScriptQuaternionInitializer(S32 line, S32 col, LLScriptExpression *expression1, 
													 LLScriptExpression *expression2, 
													 LLScriptExpression *expression3,
													 LLScriptExpression *expression4)
		: LLScriptExpression(line, col, LET_VECTOR_INITIALIZER), 
			mExpression1(expression1),
			mExpression2(expression2),
			mExpression3(expression3),
			mExpression4(expression4)
	{
	}

	~LLScriptQuaternionInitializer() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression1;
	LLScriptExpression	*mExpression2;
	LLScriptExpression	*mExpression3;
	LLScriptExpression	*mExpression4;
};

class LLScriptListInitializer : public LLScriptExpression
{
public:
	LLScriptListInitializer(S32 line, S32 col, LLScriptExpression *expressionlist)
		: LLScriptExpression(line, col, LET_LIST_INITIALIZER), mExpressionList(expressionlist)
	{
	}

	~LLScriptListInitializer() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpressionList;
};

class LLScriptPostIncrement : public LLScriptExpression
{
public:
	LLScriptPostIncrement(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_POST_INCREMENT), mExpression(expression)
	{
	}

	~LLScriptPostIncrement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptPostDecrement : public LLScriptExpression
{
public:
	LLScriptPostDecrement(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_POST_DECREMENT), mExpression(expression)
	{
	}

	~LLScriptPostDecrement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptFunctionCall : public LLScriptExpression
{
public:
	LLScriptFunctionCall(S32 line, S32 col, LLScriptIdentifier *identifier, LLScriptExpression *expressionlist)
		: LLScriptExpression(line, col, LET_FUNCTION_CALL), mIdentifier(identifier), mExpressionList(expressionlist)
	{
	}

	~LLScriptFunctionCall() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier	*mIdentifier;
	LLScriptExpression	*mExpressionList;
};

class LLScriptPrint : public LLScriptExpression
{
public:
	LLScriptPrint(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptExpression(line, col, LET_PRINT), mExpression(expression)
	{
	}

	~LLScriptPrint() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptConstantExpression : public LLScriptExpression
{
public:
	LLScriptConstantExpression(S32 line, S32 col, LLScriptConstant *constant)
		: LLScriptExpression(line, col, LET_CONSTANT), mConstant(constant)
	{
	}

	~LLScriptConstantExpression() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptConstant	*mConstant;
};

// statement
typedef enum e_lscript_statement_types
{
	LSSMT_NULL,
	LSSMT_SEQUENCE,
	LSSMT_NOOP,
	LSSMT_STATE_CHANGE,
	LSSMT_JUMP,
	LSSMT_LABEL,
	LSSMT_RETURN,
	LSSMT_EXPRESSION,
	LSSMT_IF,
	LSSMT_IF_ELSE,
	LSSMT_FOR,
	LSSMT_DO_WHILE,
	LSSMT_WHILE,
	LSSMT_DECLARATION,
	LSSMT_COMPOUND_STATEMENT,
	LSSMT_EOF
} LSCRIPTStatementType;

class LLScriptStatement : public LLScriptFilePosition
{
public:
	LLScriptStatement(S32 line, S32 col, LSCRIPTStatementType type)
		: LLScriptFilePosition(line, col), mType(type), mNextp(NULL), mStatementScope(NULL), mAllowDeclarations(TRUE)
	{
	}

	virtual ~LLScriptStatement() 
	{
		delete mStatementScope;
	}

	void addStatement(LLScriptStatement *event);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTStatementType	mType;
	LLScriptStatement		*mNextp;
	LLScriptScope			*mStatementScope;
	BOOL					mAllowDeclarations;
};

class LLScriptStatementSequence : public LLScriptStatement
{
public:
	LLScriptStatementSequence(S32 line, S32 col, LLScriptStatement *first, LLScriptStatement *second)
		: LLScriptStatement(line, col, LSSMT_SEQUENCE), mFirstp(first), mSecondp(second)
	{
	}

	~LLScriptStatementSequence() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptStatement *mFirstp;
	LLScriptStatement *mSecondp;
};

class LLScriptNOOP : public LLScriptStatement
{
public:
	LLScriptNOOP(S32 line, S32 col)
		: LLScriptStatement(line, col, LSSMT_NOOP)
	{
	}

	~LLScriptNOOP() {}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();
};

class LLScriptStateChange : public LLScriptStatement
{
public:
	LLScriptStateChange(S32 line, S32 col, LLScriptIdentifier *identifier)
		: LLScriptStatement(line, col, LSSMT_STATE_CHANGE), mIdentifier(identifier), mReturnType(LST_NULL)
	{
	}

	~LLScriptStateChange() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier *mIdentifier;
	LSCRIPTType mReturnType;
};

class LLScriptJump : public LLScriptStatement
{
public:
	LLScriptJump(S32 line, S32 col, LLScriptIdentifier *identifier)
		: LLScriptStatement(line, col, LSSMT_JUMP), mIdentifier(identifier)
	{
	}

	~LLScriptJump() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier *mIdentifier;
};

class LLScriptLabel : public LLScriptStatement
{
public:
	LLScriptLabel(S32 line, S32 col, LLScriptIdentifier *identifier)
		: LLScriptStatement(line, col, LSSMT_LABEL), mIdentifier(identifier)
	{
	}

	~LLScriptLabel()
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptIdentifier *mIdentifier;
};

class LLScriptReturn : public LLScriptStatement
{
public:
	LLScriptReturn(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptStatement(line, col, LSSMT_RETURN), mExpression(expression), mType(LST_NULL)
	{
	}

	~LLScriptReturn() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
	LSCRIPTType			mType;
};

class LLScriptExpressionStatement : public LLScriptStatement
{
public:
	LLScriptExpressionStatement(S32 line, S32 col, LLScriptExpression *expression)
		: LLScriptStatement(line, col, LSSMT_EXPRESSION), mExpression(expression)
	{
	}

	~LLScriptExpressionStatement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression	*mExpression;
};

class LLScriptIf : public LLScriptStatement
{
public:
	LLScriptIf(S32 line, S32 col, LLScriptExpression *expression, LLScriptStatement *statement)
		: LLScriptStatement(line, col, LSSMT_IF), mType(LST_NULL), mExpression(expression), mStatement(statement)
	{
	}

	~LLScriptIf() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTType				mType;
	LLScriptExpression		*mExpression;
	LLScriptStatement		*mStatement;
};

class LLScriptIfElse : public LLScriptStatement
{
public:
	LLScriptIfElse(S32 line, S32 col, LLScriptExpression *expression, LLScriptStatement *statement1, LLScriptStatement *statement2)
		: LLScriptStatement(line, col, LSSMT_IF_ELSE), mExpression(expression), mStatement1(statement1), mStatement2(statement2), mType(LST_NULL)
	{
	}

	~LLScriptIfElse() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression		*mExpression;
	LLScriptStatement		*mStatement1;
	LLScriptStatement		*mStatement2;
	LSCRIPTType				mType;
};

class LLScriptFor : public LLScriptStatement
{
public:
	LLScriptFor(S32 line, S32 col, LLScriptExpression *sequence, LLScriptExpression *expression, LLScriptExpression *expressionlist, LLScriptStatement *statement)
		: LLScriptStatement(line, col, LSSMT_FOR), mSequence(sequence), mExpression(expression), mExpressionList(expressionlist), mStatement(statement), mType(LST_NULL)
	{
	}

	~LLScriptFor() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression		*mSequence;
	LLScriptExpression		*mExpression;
	LLScriptExpression		*mExpressionList;
	LLScriptStatement		*mStatement;
	LSCRIPTType				mType;
};

class LLScriptDoWhile : public LLScriptStatement
{
public:
	LLScriptDoWhile(S32 line, S32 col, LLScriptStatement *statement, LLScriptExpression *expression)
		: LLScriptStatement(line, col, LSSMT_DO_WHILE), mStatement(statement), mExpression(expression), mType(LST_NULL)
	{
	}

	~LLScriptDoWhile() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptStatement		*mStatement;
	LLScriptExpression		*mExpression;
	LSCRIPTType				mType;
};

class LLScriptWhile : public LLScriptStatement
{
public:
	LLScriptWhile(S32 line, S32 col, LLScriptExpression *expression, LLScriptStatement *statement)
		: LLScriptStatement(line, col, LSSMT_WHILE), mExpression(expression), mStatement(statement), mType(LST_NULL)
	{
	}

	~LLScriptWhile() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptExpression			*mExpression;
	LLScriptStatement			*mStatement;
	LSCRIPTType					mType;
};

// local variables
class LLScriptDeclaration : public LLScriptStatement
{
public:
	LLScriptDeclaration(S32 line, S32 col, LLScriptType *type, LLScriptIdentifier *identifier, LLScriptExpression *expression)
		: LLScriptStatement(line, col, LSSMT_DECLARATION), mType(type), mIdentifier(identifier), mExpression(expression)
	{
	}

	~LLScriptDeclaration() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptType		*mType;
	LLScriptIdentifier	*mIdentifier;
	LLScriptExpression	*mExpression;
};

class LLScriptCompoundStatement : public LLScriptStatement
{
public:
	LLScriptCompoundStatement(S32 line, S32 col, LLScriptStatement *statement)
		: LLScriptStatement(line, col, LSSMT_COMPOUND_STATEMENT),  mStatement(statement)
	{
	}

	~LLScriptCompoundStatement() 
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptStatement	*mStatement;
};

class LLScriptEventHandler : public LLScriptFilePosition
{
public:
	LLScriptEventHandler(S32 line, S32 col, LLScriptEvent *event, LLScriptStatement *statement)
		: LLScriptFilePosition(line, col), mEventp(event), mStatement(statement), mNextp(NULL), mEventScope(NULL), mbNeedTrailingReturn(FALSE), mScopeEntry(NULL), mStackSpace(0)
	{
	}

	~LLScriptEventHandler()
	{
		delete mEventScope;
		delete mScopeEntry;
	}

	void addEvent(LLScriptEventHandler *event);

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptEvent			*mEventp;
	LLScriptStatement		*mStatement;
	LLScriptEventHandler	*mNextp;
	LLScriptScope			*mEventScope;
	BOOL					mbNeedTrailingReturn;
	LLScriptScopeEntry		*mScopeEntry;

	S32						mStackSpace;

};


// global functions
class LLScriptFunctionDec : public LLScriptFilePosition
{
public:
	LLScriptFunctionDec(S32 line, S32 col, LLScriptType *type, LLScriptIdentifier *identifier)
		: LLScriptFilePosition(line, col), mType(type), mIdentifier(identifier), mNextp(NULL)
	{
	}

	~LLScriptFunctionDec() 
	{
	}

	void addFunctionParameter(LLScriptFunctionDec *dec);

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptType		*mType;
	LLScriptIdentifier	*mIdentifier;
	LLScriptFunctionDec	*mNextp;
};

class LLScriptGlobalFunctions : public LLScriptFilePosition
{
public:
	LLScriptGlobalFunctions(S32 line, S32 col, LLScriptType *type, 
											   LLScriptIdentifier *identifier, 
											   LLScriptFunctionDec *parameters, 
											   LLScriptStatement *statements)
		: LLScriptFilePosition(line, col), mType(type), mIdentifier(identifier), mParameters(parameters), mStatements(statements), mNextp(NULL), mFunctionScope(NULL), mbNeedTrailingReturn(FALSE)
	{
	}

	void addGlobalFunction(LLScriptGlobalFunctions *global);

	~LLScriptGlobalFunctions()
	{
		delete mFunctionScope;
	}

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LLScriptType				*mType;
	LLScriptIdentifier			*mIdentifier;
	LLScriptFunctionDec			*mParameters;
	LLScriptStatement			*mStatements;
	LLScriptGlobalFunctions		*mNextp;
	LLScriptScope				*mFunctionScope;
	BOOL						mbNeedTrailingReturn;

};

typedef enum e_lscript_state_type
{
	LSSTYPE_NULL,
	LSSTYPE_DEFAULT,
	LSSTYPE_USER,
	LSSTYPE_EOF
} LSCRIPTStateType;

// info on state
class LLScriptState : public LLScriptFilePosition
{
public:
	LLScriptState(S32 line, S32 col, LSCRIPTStateType type, LLScriptIdentifier *identifier, LLScriptEventHandler *event)
		: LLScriptFilePosition(line, col), mType(type), mIdentifier(identifier), mEvent(event), mNextp(NULL), mStateScope(NULL)
	{
	}

	void addState(LLScriptState *state);

	~LLScriptState() 
	{
	}

	void gonext(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	LSCRIPTStateType mType;
	LLScriptIdentifier *mIdentifier;
	LLScriptEventHandler *mEvent;
	LLScriptState *mNextp;
	LLScriptScope *mStateScope;
};

class LLScritpGlobalStorage : public LLScriptFilePosition
{
public:

	LLScritpGlobalStorage(LLScriptGlobalVariable *var)
		: LLScriptFilePosition(0, 0), mGlobal(var), mbGlobalFunction(FALSE), mNextp(NULL)
	{
	}

	LLScritpGlobalStorage(LLScriptGlobalFunctions *func)
		: LLScriptFilePosition(0, 0), mGlobal(func), mbGlobalFunction(TRUE), mNextp(NULL)
	{
	}

	~LLScritpGlobalStorage()
	{
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
	{
	}
	
	S32 getSize()
	{
		return 0;
	}

	void addGlobal(LLScritpGlobalStorage *global)
	{
		if (mNextp)
		{
			global->mNextp = mNextp;
		}
		mNextp = global;
	}

	LLScriptFilePosition	*mGlobal;
	BOOL					mbGlobalFunction;
	LLScritpGlobalStorage	*mNextp;
};

// top level container for entire script
class LLScriptScript : public LLScriptFilePosition
{
public:
	LLScriptScript(LLScritpGlobalStorage *globals, 
				   LLScriptState *states);

	~LLScriptScript()
	{
		delete mGlobalScope;
	}

	void recurse(LLFILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata);
	S32 getSize();

	void setBytecodeDest(const char* dst_filename);

	void setClassName(const char* class_name);
	const char* getClassName() {return mClassName;}

	LLScriptState			*mStates;
	LLScriptScope			*mGlobalScope;
	LLScriptGlobalVariable	*mGlobals;
	LLScriptGlobalFunctions	*mGlobalFunctions;
	BOOL					mGodLike;

private:
	std::string mBytecodeDest;
	char mClassName[MAX_STRING];
};

class LLScriptAllocationManager
{
public:
	LLScriptAllocationManager() {}
	~LLScriptAllocationManager() 
	{
		mAllocationList.deleteAllData();
	}

	void addAllocation(LLScriptFilePosition *ptr)
	{
		mAllocationList.addData(ptr);
	}

	void deleteAllocations()
	{
		mAllocationList.deleteAllData();
	}

	LLLinkedList<LLScriptFilePosition> mAllocationList;
};

extern LLScriptAllocationManager *gAllocationManager;
extern LLScriptScript			 *gScriptp;

#endif
