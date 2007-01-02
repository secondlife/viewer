/** 
 * @file lscript_tree.cpp
 * @brief implements methods for lscript_tree.h classes
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// TO DO: Move print functionality from .h file to here

#include "linden_common.h"

#include "lscript_tree.h"
#include "lscript_typecheck.h"
#include "lscript_resource.h"
#include "lscript_bytecode.h"
#include "lscript_heap.h"
#include "lscript_library.h"
#include "lscript_alloc.h"

//#define LSL_INCLUDE_DEBUG_INFO

void print_cil_box(FILE* fp, LSCRIPTType type)
{
	switch(type)
	{
	case LST_INTEGER:
		fprintf(fp, "box [mscorlib]System.Int32\n");
		break;
	case LST_FLOATINGPOINT:
		fprintf(fp, "box [mscorlib]System.Double\n");
		break;
	case LST_STRING:
	case LST_KEY:
		fprintf(fp, "box [mscorlib]System.String\n");
		break;
	case LST_VECTOR:
		fprintf(fp, "box [LScriptLibrary]LLVector\n");
		break;
	case LST_QUATERNION:
		fprintf(fp, "box [LScriptLibrary]LLQuaternion\n");
		break;
	default:
		break;
	}
}

void print_cil_type(FILE* fp, LSCRIPTType type)
{
	switch(type)
	{
	case LST_INTEGER:
		fprintf(fp, "int32");
		break;
	case LST_FLOATINGPOINT:
		fprintf(fp, "float32");
		break;
	case LST_STRING:
	case LST_KEY:
		fprintf(fp, "string");
		break;
	case LST_VECTOR:
		fprintf(fp, "valuetype [LScriptLibrary]LLVector");
		break;
	case LST_QUATERNION:
		fprintf(fp, "valuetype [LScriptLibrary]LLQuaternion");
		break;
	case LST_LIST:
		fprintf(fp, "class [mscorlib]System.Collections.ArrayList");
		break;
	case LST_NULL:
		fprintf(fp, "void");
		break;
	default:
		break;
	}
}

void LLScriptType::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp,"%s",LSCRIPTTypeNames[mType]);
		break;
	case LSCP_TYPE:
		type = mType;
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		print_cil_type(fp, mType);
		break;
	default:
		break;
	}
}

S32 LLScriptType::getSize()
{
	return LSCRIPTDataSize[mType];
}

void LLScriptConstant::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp,"Script Constant Base class -- should never get here!\n");
		break;
	default:
		break;
	}
}

S32 LLScriptConstant::getSize()
{
	printf("Script Constant Base class -- should never get here!\n");
	return 0;
}



void LLScriptConstantInteger::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "%d", mValue);
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "PUSHARGI %d\n", mValue);
		break;
	case LSCP_TYPE:
		type = mType;
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			chunk->addInteger(mValue);
			type = mType;
		}
		break;
	case LSCP_TO_STACK:
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
			chunk->addInteger(mValue);
			type = mType;
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			*ldata = new LLScriptLibData(mValue);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "ldc.i4 %d\n", mValue);
		break;
	default:
		break;
	}
}

S32 LLScriptConstantInteger::getSize()
{
	return LSCRIPTDataSize[LST_INTEGER];
}

void LLScriptConstantFloat::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "%5.5f", mValue);
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "PUSHARGF %5.5f\n", mValue);
		break;
	case LSCP_TYPE:
		type = mType;
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			chunk->addFloat(mValue);
			type = mType;
		}
		break;
	case LSCP_TO_STACK:
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGF]);
			chunk->addFloat(mValue);
			type = mType;
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			*ldata = new LLScriptLibData(mValue);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "ldc.r8 %5.5f\n", mValue); // NOTE: Precision?
	default:
		break;
	}
}

S32 LLScriptConstantFloat::getSize()
{
	return LSCRIPTDataSize[LST_FLOATINGPOINT];
}

void print_escape_quotes(FILE* fp, const char* str)
{
  putc('"', fp);
  for(const char* c = str; *c != '\0'; ++c)
  {
	  if(*c == '"')
	  {
		  putc('\\', fp);
	  }
	  putc(*c, fp);
  }
  putc('"', fp);
}

void LLScriptConstantString::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "\"%s\"", mValue);
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "PUSHARGS \"%s\"\n", mValue);
		fprintf(fp, "STACKTOS %lu\n", strlen(mValue) + 1);
		break;
	case LSCP_TYPE:
		type = mType;
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			chunk->addInteger(heap->mCurrentOffset + 1);
			LLScriptLibData *data = new LLScriptLibData(mValue);
			U8 *temp;
			S32 size = lsa_create_data_block(&temp, data, heap->mCurrentOffset);

			heap->addBytes(temp, size);
			delete [] temp;
			delete data;
		}
		break;
	case LSCP_TO_STACK:
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGS]);
			chunk->addBytes(mValue, (S32)strlen(mValue) + 1);
			type = mType;
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			*ldata = new LLScriptLibData(mValue);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "ldstr ");
		print_escape_quotes(fp, mValue);
		fprintf(fp, "\n");
	default:
		break;
	}
}

S32 LLScriptConstantString::getSize()
{
	return (S32)strlen(mValue) + 1;
}


void LLScriptIdentifier::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "%s", mName);
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mScopeEntry)
		{
			if (mScopeEntry->mIDType == LIT_VARIABLE)
			{
				fprintf(fp, "$BP + %d [%s]", mScopeEntry->mOffset, mName);
			}
			else if (mScopeEntry->mIDType == LIT_GLOBAL)
			{
				fprintf(fp, "$GVR + %d [%s]", mScopeEntry->mOffset, mName);
			}
			else
			{
				fprintf(fp, "%s", mName);
			}
		}
		break;
	case LSCP_TYPE:
		if (mScopeEntry)
			type = mScopeEntry->mType;
		else
			type = LST_NULL;
		break;
	case LSCP_RESOURCE:
		if (mScopeEntry)
		{
			if (mScopeEntry->mIDType == LIT_VARIABLE)
			{
//				fprintf(fp, "LOCAL : %d : %d : %s\n", mScopeEntry->mOffset, mScopeEntry->mSize, mName);
			}
			else if (mScopeEntry->mIDType == LIT_GLOBAL)
			{
//				fprintf(fp, "GLOBAL: %d : %d : %s\n", mScopeEntry->mOffset, mScopeEntry->mSize, mName);
			}
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			if (mScopeEntry)
			{
				if (mScopeEntry->mType == LST_LIST)
				{
					gErrorToText.writeError(fp, this, LSERROR_NO_LISTS_IN_LISTS);
				}
				else if (mScopeEntry->mAssignable)
				{
					mScopeEntry->mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, ldata);
				}
				else
				{
					gErrorToText.writeError(fp, this, LSERROR_NO_UNITIALIZED_VARIABLES_IN_LISTS);
				}
			}
			else
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "%s", mName);
		break;
	default:
		break;
	}
}

S32 LLScriptIdentifier::getSize()
{
	
	return 0;
}



void LLScriptSimpleAssignable::addAssignable(LLScriptSimpleAssignable *assign)
{
	if (mNextp)
	{
		assign->mNextp = mNextp;
	}
	mNextp = assign;
}

void LLScriptSimpleAssignable::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	fprintf(fp, "Simple Assignable Base Class -- should never get here!\n");
}

S32 LLScriptSimpleAssignable::getSize()
{
	
	printf("Simple Assignable Base Class -- should never get here!\n");
	return 0;
}

void LLScriptSAIdentifier::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_SCOPE_PASS1:
		{ 
			LLScriptScopeEntry *entry = scope->findEntry(mIdentifier->mName);
			if (!entry)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
			else
			{
				// if we did find it, make sure this identifier is associated with the correct scope entry
				mIdentifier->mScopeEntry = entry;
			}
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			if (mIdentifier->mScopeEntry)
			{
				if(mIdentifier->mScopeEntry->mAssignable)
				{
					mIdentifier->mScopeEntry->mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				}
				else
				{
					// Babbage: 29/8/06: If the scope entry has no mAssignable,
					// set the default type and add the default 0 value to the 
					// chunk. Without this SAVectors and SAQuaternions will 
					// assume the arbitrary current type is the assignable type 
					// and may attempt to access a null chunk. (SL-20156)
					type = mIdentifier->mScopeEntry->mType;
					chunk->addBytes(LSCRIPTDataSize[type]);
				}
			}
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			}
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, ldata);
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, &(*ldata)->mListp);
			}
		}
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptSAIdentifier::getSize()
{
	return mIdentifier->getSize();
}

void LLScriptSAConstant::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, ldata);
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, &(*ldata)->mListp);
			}
		}
		break;
	default:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptSAConstant::getSize()
{
	return mConstant->getSize();
}

void print_cil_cast(FILE* fp, LSCRIPTType srcType, LSCRIPTType targetType)
{
	switch(srcType)
	{
	case LST_INTEGER:
		switch(targetType)
		{
		case LST_FLOATINGPOINT:
			fprintf(fp, "conv.r8\n");
			break;
		case LST_STRING:
			fprintf(fp, "call string class [mscorlib]System.Convert::ToString(int32)\n");
			break;
		case LST_LIST:
			fprintf(fp, "box [mscorlib]System.Int32\n");
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList class [LScriptLibrary]LScriptInternal::CreateList()\n");
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList class [LScriptLibrary]LScriptInternal::AddReturnList(object, class [mscorlib]System.Collections.ArrayList)\n");
			break;
		default:
			break;
		}
		break;
	case LST_FLOATINGPOINT:
		switch(targetType)
		{
		case LST_INTEGER:
			fprintf(fp, "conv.i4\n");
			break;
		case LST_STRING:
			fprintf(fp, "call string class [mscorlib]System.Convert::ToString(float32)\n");
			break;
		case LST_LIST:
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList [LScriptLibrary]LScriptInternal::CreateList(object)\n");
			break;
		default:
			break;
		}
		break;
	case LST_STRING:
		switch(targetType)
		{
		case LST_INTEGER:
			fprintf(fp, "call int32 valuetype [mscorlib]System.Int32::Parse(string)\n");
			break;
		case LST_FLOATINGPOINT:
			fprintf(fp, "call float64 valuetype [mscorlib]System.Double::Parse(string)\n");
			break;
		case LST_LIST:
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList [LScriptLibrary]LScriptInternal::CreateList(object)\n");
			break;
		case LST_VECTOR:
			fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'Parse'(string)\n");
			break;
		case LST_QUATERNION:
			fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'Parse'(string)\n");
			break;
		default:
			break;
		}
		break;
	case LST_KEY:
		switch(targetType)
		{
		case LST_KEY:
			break;
		case LST_STRING:
			break;
		case LST_LIST:
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList [LScriptLibrary]LScriptInternal::CreateList(object)\n");
			break;
		default:
			break;
		}
		break;
	case LST_VECTOR:
		switch(targetType)
		{
		case LST_VECTOR:
			break;
		case LST_STRING:
			fprintf(fp, "call string valuetype [LScriptLibrary]LLVector::'ToString'(valuetype [LScriptLibrary]LLVector)\n");
			break;
		case LST_LIST:
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList [LScriptLibrary]LScriptInternal::CreateList(object)\n");
			break;
		default:
			break;
		}
		break;
	case LST_QUATERNION:
		switch(targetType)
		{
		case LST_QUATERNION:
			break;
		case LST_STRING:
			fprintf(fp, "call string valuetype [LScriptLibrary]LLQuaternion::'ToString'(valuetype [LScriptLibrary]LLQuaternion)\n");
			break;
		case LST_LIST:
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList [LScriptLibrary]LScriptInternal::CreateList(object)\n");
			break;
		default:
			break;
		}
		break;
	case LST_LIST:
		switch(targetType)
		{
		case LST_LIST:
			break;
		case LST_STRING:
			fprintf(fp, "call string [LScriptLibrary]LScriptInternal::ListToString(class [mscorlib]System.Collections.ArrayList)\n");
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

bool is_SA_constant_integer(LLScriptSimpleAssignable* sa)
{
	// HACK: Downcast based on type.
	return (sa->mType == LSSAT_CONSTANT && ((LLScriptSAConstant*) sa)->mConstant->mType == LST_INTEGER);
}

void LLScriptSAVector::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "< ");
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >");
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		// vector's take floats
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		type = LST_VECTOR;
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			LLScriptByteCodeChunk	*list = new LLScriptByteCodeChunk(FALSE);
			mEntry3->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			mEntry2->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			mEntry1->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			LLVector3 vec;
			S32 offset = 0;
			bytestream2vector(vec, list->mCodeChunk, offset);
			*ldata = new LLScriptLibData(vec);
			delete list;
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, &(*ldata)->mListp);
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Load arguments.
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry1))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry3))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry3))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		
		// Call named ctor, which leaves new Vector on stack, so it can be saved in to local or argument just like a primitive type.
		fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'create'(float32, float32, float32)\n");

		// Next.
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptSAVector::getSize()
{
	return mEntry1->getSize() + mEntry2->getSize() + mEntry3->getSize();
}

void LLScriptSAQuaternion::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "< ");
		mEntry4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >");
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		// vector's take floats
		mEntry4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		type = LST_QUATERNION;
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		mEntry4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (type == LST_INTEGER)
		{
			S32 offset = chunk->mCurrentOffset - 4;
			bytestream_int2float(chunk->mCodeChunk, offset);
		}
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_LIST_BUILD_SIMPLE:
		{
			LLScriptByteCodeChunk	*list = new LLScriptByteCodeChunk(FALSE);
			mEntry4->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			mEntry3->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			mEntry2->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			mEntry1->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, list, heap, stacksize, entry, entrycount, NULL);
			if (type == LST_INTEGER)
			{
				S32 offset = list->mCurrentOffset - 4;
				bytestream_int2float(list->mCodeChunk, offset);
			}
			LLQuaternion quat;
			S32 offset = 0;
			bytestream2quaternion(quat, list->mCodeChunk, offset);
			*ldata = new LLScriptLibData(quat);
			delete list;
			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, &(*ldata)->mListp);
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Load arguments.
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry1))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry2))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry3))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		mEntry4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(is_SA_constant_integer(mEntry4))
		{
			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
		}
		
		// Call named ctor, which leaves new Vector on stack, so it can be saved in to local or argument just like a primitive type.
		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'create'(float32, float32, float32, float32)\n");

		// Next.
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		mEntry4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEntry3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEntry2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEntry1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptSAQuaternion::getSize()
{
	return mEntry1->getSize() + mEntry2->getSize() + mEntry3->getSize() + mEntry4->getSize();
}

void LLScriptSAList::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "[ ");
		if (mEntryList)
			mEntryList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ]");
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		if (mEntryList)
			mEntryList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		type = LST_LIST;
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			LLScriptLibData *list_data = new LLScriptLibData;

			list_data->mType = LST_LIST;
			if (mEntryList)
				mEntryList->recurse(fp, tabs, tabsize, LSCP_LIST_BUILD_SIMPLE, ptype, prunearg, scope, type, basetype, count, chunk, NULL, stacksize, entry, entrycount, &(list_data->mListp));

			U8 *temp;
			chunk->addInteger(heap->mCurrentOffset + 1);
			S32 size = lsa_create_data_block(&temp, list_data, heap->mCurrentOffset);
			heap->addBytes(temp, size);
			delete list_data;
			delete [] temp;

			if (mNextp)
			{
				mNextp->recurse(fp, tabs, tabsize, LSCP_EMIT_BYTE_CODE, ptype, prunearg, scope, type, basetype, count, chunk, NULL, stacksize, entry, entrycount, NULL);
			}
		}
		break;
	default:
		if (mEntryList)
			mEntryList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, ldata);
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, ldata);
		}
		break;
	}
}

S32 LLScriptSAList::getSize()
{
	return mEntryList->getSize();
}

void LLScriptGlobalVariable::addGlobal(LLScriptGlobalVariable *global)
{
	if (mNextp)
	{
		global->mNextp = mNextp;
	}
	mNextp = global;
}

void LLScriptGlobalVariable::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

void LLScriptGlobalVariable::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp,"\t");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mAssignable)
		{
			fprintf(fp, " = ");
			mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		fprintf(fp, ";\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp,"\t");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mAssignable)
		{
			fprintf(fp, " = ");
			mAssignable->recurse(fp, tabs, tabsize, LSCP_PRETTY_PRINT, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
			fprintf(fp, "Offset: %d Type: %d\n", mIdentifier->mScopeEntry->mOffset, (S32)LSCRIPTTypeByte[mType->mType]);
		}
		else
		{
			fprintf(fp, "\n");
			fprintf(fp, "Offset: %d Type: %d\n", mIdentifier->mScopeEntry->mOffset, (S32)LSCRIPTTypeByte[mType->mType]);
		}
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			if (mAssignable)
			{
				mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			}			
			// this needs to go after expression decent to make sure that we don't add ourselves or something silly
			mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_GLOBAL, mType->mType);
			if (mIdentifier->mScopeEntry && mAssignable)
					mIdentifier->mScopeEntry->mAssignable = mAssignable;
		}
		break;
	case LSCP_TYPE:
		// if the variable has an assignable, it must assignable to the variable's type
		if (mAssignable)
		{
			mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mAssignableType = type;
			if (!legal_assignment(mType->mType, mAssignableType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			// it also includes the name of the variable as well as the type
			// plus 4 bytes of offset from it's apparent address to the actual data
#ifdef LSL_INCLUDE_DEBUG_INFO
			count += strlen(mIdentifier->mName) + 1 + 1 + 4;
#else
			count += 1 + 1 + 4;
#endif
			mIdentifier->mScopeEntry->mOffset = (S32)count;
			mIdentifier->mScopeEntry->mSize = mType->getSize();
			count += mIdentifier->mScopeEntry->mSize;
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// order for global variables
			// 0 - 4: offset to actual data
			S32 offsetoffset = chunk->mCurrentOffset;
			S32 offsetdelta = 0;
			chunk->addBytes(4);
			// type
			char vtype;
			vtype = LSCRIPTTypeByte[mType->mType]; 
			chunk->addBytes(&vtype, 1);
			// null terminated name
#ifdef LSL_INCLUDE_DEBUG_INFO
			chunk->addBytes(mIdentifier->mName, strlen(mIdentifier->mName) + 1);
#else
			chunk->addBytes(1);
#endif
			// put correct offset delta in
			offsetdelta = chunk->mCurrentOffset - offsetoffset;
			integer2bytestream(chunk->mCodeChunk, offsetoffset, offsetdelta);

			// now we need space for the variable itself
			LLScriptByteCodeChunk	*value = new LLScriptByteCodeChunk(FALSE);
			if (mAssignable)
			{
				mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, value, heap, stacksize, entry, entrycount, NULL);
				// need to put sneaky type conversion here
				if (mAssignableType != mType->mType)
				{
					// the only legal case that is a problem is int->float
					if (mType->mType == LST_FLOATINGPOINT && mAssignableType == LST_INTEGER)
					{
						S32 offset = value->mCurrentOffset - 4;
						bytestream_int2float(value->mCodeChunk, offset);
					}
				}
			}
			else
			{
				if (  (mType->mType == LST_STRING)
					||(mType->mType == LST_KEY))
				{
					// string and keys (even empty ones) need heap entries
					chunk->addInteger(heap->mCurrentOffset + 1);
					LLScriptLibData *data = new LLScriptLibData("");
					U8 *temp;
					S32 size = lsa_create_data_block(&temp, data, heap->mCurrentOffset);

					heap->addBytes(temp, size);
					delete [] temp;
					delete data;
				}
				else if (mType->mType == LST_LIST)
				{
					chunk->addInteger(heap->mCurrentOffset + 1);
					LLScriptLibData *data = new LLScriptLibData;
					data->mType = LST_LIST;
					U8 *temp;
					S32 size = lsa_create_data_block(&temp, data, heap->mCurrentOffset);

					heap->addBytes(temp, size);
					delete [] temp;
					delete data;
				}
				else if (mType->mType == LST_QUATERNION)
				{
					chunk->addFloat(1.f);
					chunk->addFloat(0.f);
					chunk->addFloat(0.f);
					chunk->addFloat(0.f);
				}
				else
				{
					value->addBytes(LSCRIPTDataSize[mType->mType]);
				}
			}
			chunk->addBytes(value->mCodeChunk, value->mCurrentOffset);
			delete value;
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Initialisation inside ctor.
		if (mAssignable)
		{
			fprintf(fp, "ldarg.0\n");
			mAssignable->recurse(fp, tabs, tabsize, LSCP_EMIT_CIL_ASSEMBLY, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "stfld ");
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp," LSL::");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}
		break;
	default:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mAssignable)
		{
			mAssignable->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptGlobalVariable::getSize()
{
	S32 return_size;

	return_size = mType->getSize();
	return return_size;
}

void LLScriptEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	fprintf(fp, "Event Base Class -- should never get here!\n");
}

S32 LLScriptEvent::getSize()
{
	printf("Event Base Class -- should never get here!\n");
	return 0;
}

void LLScriptStateEntryEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "state_entry()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "state_entry()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "state_entry";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "state_entry()");
		break;
	default:
		break;
	}
}

S32 LLScriptStateEntryEvent::getSize()
{
	return 0;
}

void LLScriptStateExitEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "state_exit()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "state_exit()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "state_exit";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "state_exit()");
		break;
	default:
		break;
	}
}

S32 LLScriptStateExitEvent::getSize()
{
	return 0;
}

void LLScriptTouchStartEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "touch_start( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "touch_start";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mCount->mName, strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptTouchStartEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptTouchEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "touch( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "touch";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mCount->mName, strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptTouchEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptTouchEndEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "touch_end( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "touch_end";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mCount->mName, strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptTouchEndEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptCollisionStartEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "collision_start( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "collision_start";
			chunk->addBytes(name, (S32)strlen(name) + 1);
			chunk->addBytes(mCount->mName, (S32)strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptCollisionStartEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptCollisionEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "collision( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "collision";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mCount->mName, strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptCollisionEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptCollisionEndEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "collision_end( integer ");
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mCount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mCount->mScopeEntry = scope->addEntry(mCount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mCount->mScopeEntry)
			{
				mCount->mScopeEntry->mOffset = (S32)count;
				mCount->mScopeEntry->mSize = 4;
				count += mCount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "collision_end";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mCount->mName, strlen(mCount->mName) + 1);
#endif
		}
		break;
	default:
		mCount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptCollisionEndEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptLandCollisionStartEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "land_collision_start( vector ");
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mPosition->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mPosition->mScopeEntry = scope->addEntry(mPosition->mName, LIT_VARIABLE, LST_VECTOR);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mPosition->mScopeEntry)
			{
				mPosition->mScopeEntry->mOffset = (S32)count;
				mPosition->mScopeEntry->mSize = 12;
				count += mPosition->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "land_collision_start";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mPosition->mName, strlen(mPosition->mName) + 1);
#endif
		}
		break;
	default:
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptLandCollisionStartEvent::getSize()
{
	// vector = 12
	return 12;
}



void LLScriptLandCollisionEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "land_collision( vector ");
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mPosition->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mPosition->mScopeEntry = scope->addEntry(mPosition->mName, LIT_VARIABLE, LST_VECTOR);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mPosition->mScopeEntry)
			{
				mPosition->mScopeEntry->mOffset = (S32)count;
				mPosition->mScopeEntry->mSize = 12;
				count += mPosition->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "land_collision";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mPosition->mName, strlen(mPosition->mName) + 1);
#endif
		}
		break;
	default:
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptLandCollisionEvent::getSize()
{
	// vector = 12
	return 12;
}


void LLScriptLandCollisionEndEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "land_collision_end( vector ");
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mPosition->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mPosition->mScopeEntry = scope->addEntry(mPosition->mName, LIT_VARIABLE, LST_VECTOR);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mPosition->mScopeEntry)
			{
				mPosition->mScopeEntry->mOffset = (S32)count;
				mPosition->mScopeEntry->mSize = 12;
				count += mPosition->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "land_collision_end";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mPosition->mName, strlen(mPosition->mName) + 1);
#endif
		}
		break;
	default:
		mPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptLandCollisionEndEvent::getSize()
{
	// vector = 12
	return 12;
}


void LLScriptInventoryEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "changed( integer ");
		mChange->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mChange->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mChange->mScopeEntry = scope->addEntry(mChange->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mChange->mScopeEntry)
			{
				mChange->mScopeEntry->mOffset = (S32)count;
				mChange->mScopeEntry->mSize = 4;
				count += mChange->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "changed";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mChange->mName, strlen(mChange->mName) + 1);
#endif
		}
		break;
	default:
		mChange->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptInventoryEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptAttachEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "attach( key ");
		mAttach->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mAttach->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mAttach->mScopeEntry = scope->addEntry(mAttach->mName, LIT_VARIABLE, LST_KEY);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mAttach->mScopeEntry)
			{
				mAttach->mScopeEntry->mOffset = (S32)count;
				mAttach->mScopeEntry->mSize = 4;
				count += mAttach->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "attach";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mAttach->mName, strlen(mAttach->mName) + 1);
#endif
		}
		break;
	default:
		mAttach->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptAttachEvent::getSize()
{
	// key = 4
	return 4;
}

void LLScriptDataserverEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "dataserver( key ");
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mData->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mID->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mID->mScopeEntry = scope->addEntry(mID->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mData->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mData->mScopeEntry = scope->addEntry(mData->mName, LIT_VARIABLE, LST_STRING);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mID->mScopeEntry)
			{
				mID->mScopeEntry->mOffset = (S32)count;
				mID->mScopeEntry->mSize = 4;
				count += mID->mScopeEntry->mSize;
				mData->mScopeEntry->mOffset = (S32)count;
				mData->mScopeEntry->mSize = 4;
				count += mData->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "dataserver";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mID->mName, strlen(mID->mName) + 1);
			chunk->addBytes(mData->mName, strlen(mData->mName) + 1);
#endif
		}
		break;
	default:
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mData->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptDataserverEvent::getSize()
{
	// key + string = 8
	return 8;
}

void LLScriptTimerEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "timer()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "timer()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "timer";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptTimerEvent::getSize()
{
	return 0;
}

void LLScriptMovingStartEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "moving_start()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "moving_start";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptMovingStartEvent::getSize()
{
	return 0;
}

void LLScriptMovingEndEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "moving_end()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "moving_end";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptMovingEndEvent::getSize()
{
	return 0;
}

void LLScriptRTPEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "chat( integer ");
		mRTPermissions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mRTPermissions->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mRTPermissions->mScopeEntry = scope->addEntry(mRTPermissions->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mRTPermissions->mScopeEntry)
			{
				mRTPermissions->mScopeEntry->mOffset = (S32)count;
				mRTPermissions->mScopeEntry->mSize = 4;
				count += mRTPermissions->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "chat";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mRTPermissions->mName, strlen(mRTPermissions->mName) + 1);
#endif
		}
		break;
	default:
		mRTPermissions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptRTPEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptChatEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "chat( integer ");
		mChannel->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", key ");
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mMessage->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mChannel->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mChannel->mScopeEntry = scope->addEntry(mChannel->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mName->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mName->mScopeEntry = scope->addEntry(mName->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mID->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mID->mScopeEntry = scope->addEntry(mID->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mMessage->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mMessage->mScopeEntry = scope->addEntry(mMessage->mName, LIT_VARIABLE, LST_STRING);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mName->mScopeEntry)
			{
				mChannel->mScopeEntry->mOffset = (S32)count;
				mChannel->mScopeEntry->mSize = 4;
				count += mChannel->mScopeEntry->mSize;
				mName->mScopeEntry->mOffset = (S32)count;
				mName->mScopeEntry->mSize = 4;
				count += mName->mScopeEntry->mSize;
				mID->mScopeEntry->mOffset = (S32)count;
				mID->mScopeEntry->mSize = 4;
				count += mID->mScopeEntry->mSize;
				mMessage->mScopeEntry->mOffset = (S32)count;
				mMessage->mScopeEntry->mSize = 4;
				count += mMessage->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "chat";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mChannel->mName, strlen(mChannel->mName) + 1);
			chunk->addBytes(mName->mName, strlen(mName->mName) + 1);
			chunk->addBytes(mID->mName, strlen(mID->mName) + 1);
			chunk->addBytes(mMessage->mName, strlen(mMessage->mName) + 1);
#endif
		}
		break;
	default:
		mChannel->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mMessage->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptChatEvent::getSize()
{
	// integer + key + string + string = 16
	return 16;
}

void LLScriptSensorEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "sensor( integer ");
		mNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mNumber->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mNumber->mScopeEntry = scope->addEntry(mNumber->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mNumber->mScopeEntry)
			{
				mNumber->mScopeEntry->mOffset = (S32)count;
				mNumber->mScopeEntry->mSize = 4;
				count += mNumber->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "sensor";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mNumber->mName, strlen(mNumber->mName) + 1);
#endif
		}
		break;
	default:
		mNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptSensorEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptObjectRezEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "object_rez( key ");
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mID->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mID->mScopeEntry = scope->addEntry(mID->mName, LIT_VARIABLE, LST_KEY);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mID->mScopeEntry)
			{
				mID->mScopeEntry->mOffset = (S32)count;
				mID->mScopeEntry->mSize = 4;
				count += mID->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "sensor";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mID->mName, strlen(mID->mName) + 1);
#endif
		}
		break;
	default:
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptObjectRezEvent::getSize()
{
	// key = 4
	return 4;
}

void LLScriptControlEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "control( key ");
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mLevels->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mEdges->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mName->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mName->mScopeEntry = scope->addEntry(mName->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mLevels->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mLevels->mScopeEntry = scope->addEntry(mLevels->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mEdges->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mEdges->mScopeEntry = scope->addEntry(mEdges->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mName->mScopeEntry)
			{
				mName->mScopeEntry->mOffset = (S32)count;
				mName->mScopeEntry->mSize = 4;
				count += mName->mScopeEntry->mSize;
				mLevels->mScopeEntry->mOffset = (S32)count;
				mLevels->mScopeEntry->mSize = 4;
				count += mLevels->mScopeEntry->mSize;
				mEdges->mScopeEntry->mOffset = (S32)count;
				mEdges->mScopeEntry->mSize = 4;
				count += mEdges->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "control";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mName->mName, strlen(mName->mName) + 1);
			chunk->addBytes(mLevels->mName, strlen(mLevels->mName) + 1);
			chunk->addBytes(mEdges->mName, strlen(mEdges->mName) + 1);
#endif
		}
		break;
	default:
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLevels->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mEdges->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptControlEvent::getSize()
{
	// key + integer + integer = 12
	return 12;
}

void LLScriptLinkMessageEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "link_message( integer ");
		mSender->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mNum->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mStr->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", key ");
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mSender->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mSender->mScopeEntry = scope->addEntry(mSender->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mNum->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mNum->mScopeEntry = scope->addEntry(mNum->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mStr->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mStr->mScopeEntry = scope->addEntry(mStr->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mID->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mID->mScopeEntry = scope->addEntry(mID->mName, LIT_VARIABLE, LST_KEY);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mSender->mScopeEntry)
			{
				mSender->mScopeEntry->mOffset = (S32)count;
				mSender->mScopeEntry->mSize = 4;
				count += mSender->mScopeEntry->mSize;
				mNum->mScopeEntry->mOffset = (S32)count;
				mNum->mScopeEntry->mSize = 4;
				count += mNum->mScopeEntry->mSize;
				mStr->mScopeEntry->mOffset = (S32)count;
				mStr->mScopeEntry->mSize = 4;
				count += mStr->mScopeEntry->mSize;
				mID->mScopeEntry->mOffset = (S32)count;
				mID->mScopeEntry->mSize = 4;
				count += mID->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "link_message";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mSender->mName, strlen(mSender->mName) + 1);
			chunk->addBytes(mNum->mName, strlen(mNum->mName) + 1);
			chunk->addBytes(mStr->mName, strlen(mStr->mName) + 1);
			chunk->addBytes(mID->mName, strlen(mID->mName) + 1);
#endif
		}
		break;
	default:
		mSender->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mNum->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStr->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptLinkMessageEvent::getSize()
{
	// integer + key + integer + string = 16
	return 16;
}

void LLScriptRemoteEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "remote_event( integer ");
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", key ");
		mChannel->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", key ");
		mMessageID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mSender->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mIntVal->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mStrVal->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mType->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mType->mScopeEntry = scope->addEntry(mType->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mChannel->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mChannel->mScopeEntry = scope->addEntry(mChannel->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mMessageID->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mMessageID->mScopeEntry = scope->addEntry(mMessageID->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mSender->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mSender->mScopeEntry = scope->addEntry(mSender->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mIntVal->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mIntVal->mScopeEntry = scope->addEntry(mIntVal->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mStrVal->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mStrVal->mScopeEntry = scope->addEntry(mStrVal->mName, LIT_VARIABLE, LST_STRING);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mType->mScopeEntry)
			{
				mType->mScopeEntry->mOffset = (S32)count;
				mType->mScopeEntry->mSize = 4;
				count += mType->mScopeEntry->mSize;
				mChannel->mScopeEntry->mOffset = (S32)count;
				mChannel->mScopeEntry->mSize = 4;
				count += mChannel->mScopeEntry->mSize;
				mMessageID->mScopeEntry->mOffset = (S32)count;
				mMessageID->mScopeEntry->mSize = 4;
				count += mMessageID->mScopeEntry->mSize;
				mSender->mScopeEntry->mOffset = (S32)count;
				mSender->mScopeEntry->mSize = 4;
				count += mSender->mScopeEntry->mSize;
				mIntVal->mScopeEntry->mOffset = (S32)count;
				mIntVal->mScopeEntry->mSize = 4;
				count += mIntVal->mScopeEntry->mSize;
				mStrVal->mScopeEntry->mOffset = (S32)count;
				mStrVal->mScopeEntry->mSize = 4;
				count += mStrVal->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "remote_event";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mType->mName, strlen(mType->mName) + 1);
			chunk->addBytes(mChannel->mName, strlen(mChannel->mName) + 1);
			chunk->addBytes(mMessageID->mName, strlen(mMessageID->mName) + 1);
			chunk->addBytes(mSender->mName, strlen(mSender->mName) + 1);
			chunk->addBytes(mIntVal->mName, strlen(mIntVal->mName) + 1);
			chunk->addBytes(mStrVal->mName, strlen(mStrVal->mName) + 1);
#endif
		}
		break;
	default:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mChannel->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mMessageID->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mSender->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mIntVal->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStrVal->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptRemoteEvent::getSize()
{
	// integer + key + key + string + integer + string = 24
	return 24;
}

void LLScriptHTTPResponseEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "http_response( key ");
		mRequestId->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mStatus->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", list ");
		mMetadata->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mBody->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
		
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mRequestId->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mRequestId->mScopeEntry = scope->addEntry(mRequestId->mName, LIT_VARIABLE, LST_KEY);
		}
		
		if (scope->checkEntry(mStatus->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mStatus->mScopeEntry = scope->addEntry(mStatus->mName, LIT_VARIABLE, LST_INTEGER);
		}
		
		if (scope->checkEntry(mMetadata->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mMetadata->mScopeEntry = scope->addEntry(mMetadata->mName, LIT_VARIABLE, LST_LIST);
		}
		
		if (scope->checkEntry(mBody->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mBody->mScopeEntry = scope->addEntry(mBody->mName, LIT_VARIABLE, LST_STRING);
		}
		break;
		
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mRequestId->mScopeEntry)
			{
				mRequestId->mScopeEntry->mOffset = (S32)count;
				mRequestId->mScopeEntry->mSize = 4;
				count += mRequestId->mScopeEntry->mSize;

				mStatus->mScopeEntry->mOffset = (S32)count;
				mStatus->mScopeEntry->mSize = 4;
				count += mStatus->mScopeEntry->mSize;

				mMetadata->mScopeEntry->mOffset = (S32)count;
				mMetadata->mScopeEntry->mSize = 4;
				count += mMetadata->mScopeEntry->mSize;

				mBody->mScopeEntry->mOffset = (S32)count;
				mBody->mScopeEntry->mSize = 4;
				count += mBody->mScopeEntry->mSize;
			}
		}
		break;
		
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "http_response";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mRequestId->mName, strlen(mRequestId->mName) + 1);
			chunk->addBytes(mStatus->mName, strlen(mStatus->mName) + 1);
			chunk->addBytes(mMetadata->mName, strlen(mMetadata->mName) + 1);
			chunk->addBytes(mBody->mName, strlen(mBody->mName) + 1);
#endif
		}
		break;
		
	default:
		mRequestId->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatus->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mMetadata->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mBody->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptHTTPResponseEvent::getSize()
{
	// key + integer + list + string = 16
	return 16;
}


void LLScriptMoneyEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "money( key ");
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mAmount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mName->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mName->mScopeEntry = scope->addEntry(mName->mName, LIT_VARIABLE, LST_KEY);
		}
		if (scope->checkEntry(mAmount->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mAmount->mScopeEntry = scope->addEntry(mAmount->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mName->mScopeEntry)
			{
				mName->mScopeEntry->mOffset = (S32)count;
				mName->mScopeEntry->mSize = 4;
				count += mName->mScopeEntry->mSize;
				mAmount->mScopeEntry->mOffset = (S32)count;
				mAmount->mScopeEntry->mSize = 4;
				count += mAmount->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "money";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mName->mName, strlen(mName->mName) + 1);
			chunk->addBytes(mAmount->mName, strlen(mAmount->mName) + 1);
#endif
		}
		break;
	default:
		mName->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mAmount->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptMoneyEvent::getSize()
{
	// key + integer = 8
	return 8;
}

void LLScriptEmailEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "email( string ");
		mTime->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mAddress->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mSubject->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", string ");
		mBody->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", integer ");
		mNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mTime->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mTime->mScopeEntry = scope->addEntry(mTime->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mAddress->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mAddress->mScopeEntry = scope->addEntry(mAddress->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mSubject->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mSubject->mScopeEntry = scope->addEntry(mSubject->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mBody->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mBody->mScopeEntry = scope->addEntry(mBody->mName, LIT_VARIABLE, LST_STRING);
		}
		if (scope->checkEntry(mNumber->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mNumber->mScopeEntry = scope->addEntry(mNumber->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mAddress->mScopeEntry)
			{
				mTime->mScopeEntry->mOffset = (S32)count;
				mTime->mScopeEntry->mSize = 4;
				count += mTime->mScopeEntry->mSize;
				mAddress->mScopeEntry->mOffset = (S32)count;
				mAddress->mScopeEntry->mSize = 4;
				count += mAddress->mScopeEntry->mSize;
				mSubject->mScopeEntry->mOffset = (S32)count;
				mSubject->mScopeEntry->mSize = 4;
				count += mSubject->mScopeEntry->mSize;
				mBody->mScopeEntry->mOffset = (S32)count;
				mBody->mScopeEntry->mSize = 4;
				count += mBody->mScopeEntry->mSize;
				mNumber->mScopeEntry->mOffset = (S32)count;
				mNumber->mScopeEntry->mSize = 4;
				count += mNumber->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "email";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mTime->mName, strlen(mTime->mName) + 1);
			chunk->addBytes(mAddress->mName, strlen(mAddress->mName) + 1);
			chunk->addBytes(mSubject->mName, strlen(mSubject->mName) + 1);
			chunk->addBytes(mBody->mName, strlen(mBody->mName) + 1);
			chunk->addBytes(mNumber->mName, strlen(mNumber->mName) + 1);
#endif
		}
		break;
	default:
		mTime->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mAddress->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mSubject->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mBody->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptEmailEvent::getSize()
{
	// string + string + string + string + integer = 16
	return 20;
}

void LLScriptRezEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "rez( integer ");
		mStartParam->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mStartParam->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mStartParam->mScopeEntry = scope->addEntry(mStartParam->mName, LIT_VARIABLE, LST_INTEGER);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mStartParam->mScopeEntry)
			{
				mStartParam->mScopeEntry->mOffset = (S32)count;
				mStartParam->mScopeEntry->mSize = 4;
				count += mStartParam->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "rez";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mStartParam->mName, strlen(mStartParam->mName) + 1);
#endif
		}
		break;
	default:
		mStartParam->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptRezEvent::getSize()
{
	// integer = 4
	return 4;
}

void LLScriptNoSensorEvent::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "no_sensor()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "no_sensor()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "no_sensor";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptNoSensorEvent::getSize()
{
	return 0;
}

void LLScriptAtTarget::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "at_target( integer ");
		mTargetNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", vector ");
		mTargetPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", vector ");
		mOurPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mTargetNumber->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mTargetNumber->mScopeEntry = scope->addEntry(mTargetNumber->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mTargetPosition->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mTargetPosition->mScopeEntry = scope->addEntry(mTargetPosition->mName, LIT_VARIABLE, LST_VECTOR);
		}
		if (scope->checkEntry(mOurPosition->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mOurPosition->mScopeEntry = scope->addEntry(mOurPosition->mName, LIT_VARIABLE, LST_VECTOR);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mTargetNumber->mScopeEntry)
			{
				mTargetNumber->mScopeEntry->mOffset = (S32)count;
				mTargetNumber->mScopeEntry->mSize = 4;
				count += mTargetNumber->mScopeEntry->mSize;
				mTargetPosition->mScopeEntry->mOffset = (S32)count;
				mTargetPosition->mScopeEntry->mSize = 12;
				count += mTargetPosition->mScopeEntry->mSize;
				mOurPosition->mScopeEntry->mOffset = (S32)count;
				mOurPosition->mScopeEntry->mSize = 12;
				count += mOurPosition->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "at_target";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mTargetNumber->mName, strlen(mTargetNumber->mName) + 1);
			chunk->addBytes(mTargetPosition->mName, strlen(mTargetPosition->mName) + 1);
			chunk->addBytes(mOurPosition->mName, strlen(mOurPosition->mName) + 1);
#endif
		}
		break;
	default:
		mTargetNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mTargetPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mOurPosition->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptAtTarget::getSize()
{
	// integer + vector + vector = 28
	return 28;
}



void LLScriptNotAtTarget::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "not_at_target()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "not_at_target()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "not_at_target";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptNotAtTarget::getSize()
{
	return 0;
}

void LLScriptAtRotTarget::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
	case LSCP_EMIT_ASSEMBLY:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "at_target( integer ");
		mTargetNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", quaternion ");
		mTargetRotation->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", quaternion ");
		mOurRotation->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (scope->checkEntry(mTargetNumber->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mTargetNumber->mScopeEntry = scope->addEntry(mTargetNumber->mName, LIT_VARIABLE, LST_INTEGER);
		}
		if (scope->checkEntry(mTargetRotation->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mTargetRotation->mScopeEntry = scope->addEntry(mTargetRotation->mName, LIT_VARIABLE, LST_QUATERNION);
		}
		if (scope->checkEntry(mOurRotation->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mOurRotation->mScopeEntry = scope->addEntry(mOurRotation->mName, LIT_VARIABLE, LST_QUATERNION);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			if (mTargetNumber->mScopeEntry)
			{
				mTargetNumber->mScopeEntry->mOffset = (S32)count;
				mTargetNumber->mScopeEntry->mSize = 4;
				count += mTargetNumber->mScopeEntry->mSize;
				mTargetRotation->mScopeEntry->mOffset = (S32)count;
				mTargetRotation->mScopeEntry->mSize = 16;
				count += mTargetRotation->mScopeEntry->mSize;
				mOurRotation->mScopeEntry->mOffset = (S32)count;
				mOurRotation->mScopeEntry->mSize = 16;
				count += mOurRotation->mScopeEntry->mSize;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "at_rot_target";
			chunk->addBytes(name, strlen(name) + 1);
			chunk->addBytes(mTargetNumber->mName, strlen(mTargetNumber->mName) + 1);
			chunk->addBytes(mTargetRotation->mName, strlen(mTargetRotation->mName) + 1);
			chunk->addBytes(mOurRotation->mName, strlen(mOurRotation->mName) + 1);
#endif
		}
		break;
	default:
		mTargetNumber->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mTargetRotation->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mOurRotation->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}

S32 LLScriptAtRotTarget::getSize()
{
	// integer + quaternion + quaternion = 36
	return 36;
}



void LLScriptNotAtRotTarget::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "not_at_rot_target()\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "not_at_rot_target()\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
#ifdef LSL_INCLUDE_DEBUG_INFO
			char name[] = "not_at_rot_target";
			chunk->addBytes(name, strlen(name) + 1);
#endif
		}
		break;
	default:
		break;
	}
}

S32 LLScriptNotAtRotTarget::getSize()
{
	return 0;
}



void LLScriptExpression::addExpression(LLScriptExpression *expression)
{
	if (mNextp)
	{
		expression->mNextp = mNextp;
	}
	mNextp = expression;
}

void LLScriptExpression::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	fprintf(fp, "Expression Base Class -- should never get here!\n");
}

S32 LLScriptExpression::getSize()
{
	printf("Expression Base Class -- should never get here!\n");
	return 0;
}

void LLScriptExpression::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

void LLScriptForExpressionList::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			fprintf(fp, ", ");
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mFirstp->mReturnType)
		{
			fprintf(fp, "%s\n", LSCRIPTTypePop[mFirstp->mReturnType]);
		}
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mSecondp->mReturnType)
			{
				fprintf(fp, "%s\n", LSCRIPTTypePop[mSecondp->mReturnType]);
			}
		}
		break;
	case LSCP_TO_STACK:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		switch(mFirstp->mReturnType)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
			break;
		case LST_STRING:
		case LST_KEY:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
			break;
		case LST_LIST:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
			break;
		case LST_VECTOR:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
			break;
		case LST_QUATERNION:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
			break;
		default:
			break;
		}
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			switch(mSecondp->mReturnType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_STRING:
			case LST_KEY:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
				break;
			case LST_LIST:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
				break;
			default:
				break;
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mFirstp->mReturnType)
		{
			fprintf(fp, "pop\n");
		}
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mSecondp->mReturnType)
			{
				fprintf(fp, "pop\n");
			}
		}
		break;
	default:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptForExpressionList::getSize()
{
	return 0;
}

void LLScriptFuncExpressionList::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			fprintf(fp, ", ");
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		{
			mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!entry->mFunctionArgs.getType(entrycount))
			{
				gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
			}
			if (!legal_assignment(entry->mFunctionArgs.getType(entrycount), mFirstp->mReturnType))
			{
				gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
			}
			count++;
			entrycount++;
			if (mSecondp)
			{
				mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				if (mSecondp->mReturnType)
				{
					count++;
					if (!entry->mFunctionArgs.getType(entrycount))
					{
						gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
					}
					if (!legal_assignment(entry->mFunctionArgs.getType(entrycount), mSecondp->mReturnType))
					{
						gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
					}
				}
			}
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			LSCRIPTType argtype = entry->mFunctionArgs.getType(entrycount);
			if (argtype != mFirstp->mReturnType)
			{
				fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mFirstp->mReturnType], LSCRIPTTypeNames[argtype]);
			}
			entrycount++;
			if (mSecondp)
			{
				mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				if (mSecondp->mReturnType)
				{
					argtype = entry->mFunctionArgs.getType(entrycount);
					if (argtype != mSecondp->mReturnType)
					{
						fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mSecondp->mReturnType], LSCRIPTTypeNames[argtype]);
					}
				}
			}
		}
		break;
	case LSCP_TO_STACK:
		{
			mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			LSCRIPTType argtype = entry->mFunctionArgs.getType(entrycount);
			if (argtype != mFirstp->mReturnType)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
				U8 castbyte = LSCRIPTTypeByte[argtype] | LSCRIPTTypeHi4Bits[mFirstp->mReturnType];
				chunk->addByte(castbyte);
			}
			entrycount++;
			if (mSecondp)
			{
				mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				if (mSecondp->mReturnType)
				{
					argtype = entry->mFunctionArgs.getType(entrycount);
					if (argtype != mSecondp->mReturnType)
					{
						chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
						U8 castbyte = LSCRIPTTypeByte[argtype] | LSCRIPTTypeHi4Bits[mSecondp->mReturnType];
						chunk->addByte(castbyte);
					}
				}
			}
		}
		break;
	/* TODO: Fix conflict between global/local variable determination needing caller scope and cast determination here needs callee scope...
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			LSCRIPTType argtype = entry->mFunctionArgs.getType(entrycount);
			if (argtype != mFirstp->mReturnType)
			{
				print_cil_cast(fp, mFirstp->mReturnType, argtype);
			}
			entrycount++;
			if (mSecondp)
			{
				mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				if (mSecondp->mReturnType)
				{
					argtype = entry->mFunctionArgs.getType(entrycount);
					if (argtype != mSecondp->mReturnType)
					{
						print_cil_cast(fp, mFirstp->mReturnType, argtype);
					}
				}
			}
		}
		break;
		*/
	default:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptFuncExpressionList::getSize()
{
	return 0;
}

void LLScriptListExpressionList::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			fprintf(fp, ", ");
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mFirstp->mType != LET_LIST_EXPRESSION_LIST)
		{
			fprintf(fp, "%s\n", LSCRIPTListDescription[mFirstp->mReturnType]);
			count++;
		}
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mSecondp->mType != LET_LIST_EXPRESSION_LIST)
			{
				fprintf(fp, "%s\n", LSCRIPTListDescription[mSecondp->mReturnType]);
				count++;
			}
		}
		break;
	case LSCP_TO_STACK:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mFirstp->mType != LET_LIST_EXPRESSION_LIST)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGB]);
			chunk->addByte(LSCRIPTTypeByte[mFirstp->mReturnType]);
			count++;
		}
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mSecondp->mType != LET_LIST_EXPRESSION_LIST)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGB]);
				chunk->addByte(LSCRIPTTypeByte[mSecondp->mReturnType]);
				count++;
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		// Evaluate expressions in reverse order so first expression is on top of stack.
		// Results can then be popped and appended to list to result in list with correct order.
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mSecondp->mType != LET_LIST_EXPRESSION_LIST)
			{
				// Box value.
				print_cil_box(fp, mSecondp->mReturnType);

				++count;
			}
		}
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mFirstp->mType != LET_LIST_EXPRESSION_LIST)
		{
			// Box value.
			print_cil_box(fp, mFirstp->mReturnType);

			++count;
		}
		break;
	default:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mSecondp)
		{
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptListExpressionList::getSize()
{
	return 0;
}

// Returns true if identifier is a parameter and false if identifier is a local variable within function_scope.
bool is_parameter(LLScriptIdentifier* identifier, LLScriptScopeEntry* function_scope)
{
	// Function offset stores offset of first local.
	// Compare variable offset with function offset to
	// determine whether variable is local or parameter.
	return (identifier->mScopeEntry->mOffset < function_scope->mOffset);
}

// If assignment is to global variable, pushes this pointer on to stack.
void print_cil_load_address(FILE* fp, LLScriptExpression* exp, LLScriptScopeEntry* function_scope)
{
	LLScriptLValue *lvalue = (LLScriptLValue *) exp;
	LLScriptIdentifier *ident = lvalue->mIdentifier;

	// If global (member), load this pointer.
	if(ident->mScopeEntry->mIDType == LIT_GLOBAL)
	{
		fprintf(fp, "ldarg.0\n");
	}

	// If accessor, load address of object.
	if(lvalue->mAccessor)
	{
		if(ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			if(is_parameter(ident, function_scope))
			{
				// Parameter, load by name.
				fprintf(fp, "ldarga.s %s\n", ident->mScopeEntry->mIdentifier);
			}
			else
			{
				// Local, load by index.
				fprintf(fp, "ldloca.s %d\n", ident->mScopeEntry->mCount);
			}
		}
		else if (ident->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			fprintf(fp, "ldflda ");
			print_cil_type(fp, ident->mScopeEntry->mType);
			fprintf(fp, " LSL::%s\n", ident->mScopeEntry->mIdentifier);
		}
	}
}

void print_cil_accessor(FILE* fp, LLScriptLValue *lvalue)
{
	LLScriptIdentifier *ident = lvalue->mIdentifier;
	print_cil_type(fp, lvalue->mReturnType);
	fprintf(fp, " ");
	print_cil_type(fp, ident->mScopeEntry->mType);
	fprintf(fp, "::%s\n", lvalue->mAccessor->mName);
}

void print_cil_member(FILE* fp, LLScriptIdentifier *ident)
{
	print_cil_type(fp, ident->mScopeEntry->mType);
	fprintf(fp, " LSL::%s\n", ident->mScopeEntry->mIdentifier);
}

void LLScriptLValue::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mAccessor)
		{
			fprintf(fp, ".");
			mAccessor->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			if (mAccessor)
			{
				fprintf(fp, "%s%d [%s.%s]\n", LSCRIPTTypeLocalPush[mReturnType], mIdentifier->mScopeEntry->mOffset + mOffset, mIdentifier->mName, mAccessor->mName);
			}
			else
			{
				fprintf(fp, "%s%d [%s]\n", LSCRIPTTypeLocalPush[mIdentifier->mScopeEntry->mType], mIdentifier->mScopeEntry->mOffset, mIdentifier->mName);
			}
		}
		else if (mIdentifier->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			if (mAccessor)
			{
				fprintf(fp, "%s%d [%s.%s]\n", LSCRIPTTypeGlobalPush[mReturnType], mIdentifier->mScopeEntry->mOffset + mOffset, mIdentifier->mName, mAccessor->mName);
			}
			else
			{
				fprintf(fp, "%s%d [%s]\n", LSCRIPTTypeGlobalPush[mIdentifier->mScopeEntry->mType], mIdentifier->mScopeEntry->mOffset, mIdentifier->mName);
			}
		}
		else
		{
			fprintf(fp, "Unexpected LValue!\n");
		}
		break;
	case LSCP_SCOPE_PASS1:
		{
			LLScriptScopeEntry *entry = scope->findEntry(mIdentifier->mName);
			if (!entry || (  (entry->mIDType != LIT_GLOBAL) && (entry->mIDType != LIT_VARIABLE)))
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
			else
			{
				// if we did find it, make sure this identifier is associated with the correct scope entry
				mIdentifier->mScopeEntry = entry;
			}
		}
		break;
	case LSCP_TYPE:
		// if we have an accessor, we need to change what type our identifier returns and set our offset value
		if (mIdentifier->mScopeEntry)
		{
			if (mAccessor)
			{
				BOOL b_ok = FALSE;
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					if (mIdentifier->mScopeEntry->mType == LST_VECTOR)
					{
						if (!strcmp("x", mAccessor->mName))
						{
							mOffset = 0;
							b_ok = TRUE;
						}
						else if (!strcmp("y", mAccessor->mName))
						{
							mOffset = 4;
							b_ok = TRUE;
						}
						else if (!strcmp("z", mAccessor->mName))
						{
							mOffset = 8;
							b_ok = TRUE;
						}
					}
					else if (mIdentifier->mScopeEntry->mType == LST_QUATERNION)
					{
						if (!strcmp("x", mAccessor->mName))
						{
							mOffset = 0;
							b_ok = TRUE;
						}
						else if (!strcmp("y", mAccessor->mName))
						{
							mOffset = 4;
							b_ok = TRUE;
						}
						else if (!strcmp("z", mAccessor->mName))
						{
							mOffset = 8;
							b_ok = TRUE;
						}
						else if (!strcmp("s", mAccessor->mName))
						{
							mOffset = 12;
							b_ok = TRUE;
						}
					}
				}
				else
				{
					if (mIdentifier->mScopeEntry->mType == LST_VECTOR)
					{
						if (!strcmp("x", mAccessor->mName))
						{
							mOffset = 8;
							b_ok = TRUE;
						}
						else if (!strcmp("y", mAccessor->mName))
						{
							mOffset = 4;
							b_ok = TRUE;
						}
						else if (!strcmp("z", mAccessor->mName))
						{
							mOffset = 0;
							b_ok = TRUE;
						}
					}
					else if (mIdentifier->mScopeEntry->mType == LST_QUATERNION)
					{
						if (!strcmp("x", mAccessor->mName))
						{
							mOffset = 12;
							b_ok = TRUE;
						}
						else if (!strcmp("y", mAccessor->mName))
						{
							mOffset = 8;
							b_ok = TRUE;
						}
						else if (!strcmp("z", mAccessor->mName))
						{
							mOffset = 4;
							b_ok = TRUE;
						}
						else if (!strcmp("s", mAccessor->mName))
						{
							mOffset = 0;
							b_ok = TRUE;
						}
					}
				}
				if (b_ok)
				{
					mReturnType = type =  LST_FLOATINGPOINT;
				}
				else
				{
					gErrorToText.writeError(fp, this, LSERROR_VECTOR_METHOD_ERROR);
				}
			}
			else
			{
				mReturnType = type = mIdentifier->mScopeEntry->mType;
			}
		}
		else
		{
			mReturnType = type = LST_UNDEFINED;
		}
		break;
	case LSCP_TO_STACK:
		{
			switch(mReturnType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSH]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHG]);
				}
				break;
			case LST_KEY:
			case LST_STRING:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHS]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHGS]);
				}
				break;
			case LST_LIST:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHL]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHGL]);
				}
				break;
			case LST_VECTOR:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHV]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHGV]);
				}
				break;
			case LST_QUATERNION:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHQ]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHGQ]);
				}
				break;
			default:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSH]);
				}
				else
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHG]);
				}
				break;
			}
			S32 address = mIdentifier->mScopeEntry->mOffset + mOffset;
			chunk->addInteger(address);
		}	
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		print_cil_load_address(fp, this, entry);
		if(mAccessor)
		{
			fprintf(fp, "ldfld ");
			print_cil_accessor(fp, this);
		}
		else if(mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			if(is_parameter(mIdentifier, entry))
			{
				// Parameter, load by name.
				fprintf(fp, "ldarg.s %s\n", mIdentifier->mScopeEntry->mIdentifier);
			}
			else
			{
				// Local, load by index.
				fprintf(fp, "ldloc.s %d\n", mIdentifier->mScopeEntry->mCount);
			}
		}
		else if (mIdentifier->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			fprintf(fp, "ldfld ");
			print_cil_member(fp, mIdentifier);
		}
		else
		{
			fprintf(fp, "Unexpected LValue!\n");
		}
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptLValue::getSize()
{
	return 0;
}

void print_asignment(FILE *fp, LLScriptExpression *exp)
{
	LLScriptLValue *lvalue = (LLScriptLValue *)exp;
	LLScriptIdentifier *ident = lvalue->mIdentifier;
	if (lvalue->mAccessor)
	{
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			fprintf(fp, "%s%d [%s.%s]\n", LSCRIPTTypeLocalStore[ident->mScopeEntry->mType], ident->mScopeEntry->mOffset + lvalue->mOffset, ident->mName, lvalue->mAccessor->mName);
		}
		else if (ident->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			fprintf(fp, "%s%d [%s.%s]\n", LSCRIPTTypeGlobalStore[ident->mScopeEntry->mType], ident->mScopeEntry->mOffset + lvalue->mOffset, ident->mName, lvalue->mAccessor->mName);
		}
	}
	else
	{
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			fprintf(fp, "%s%d [%s]\n", LSCRIPTTypeLocalStore[ident->mScopeEntry->mType], ident->mScopeEntry->mOffset, ident->mName);
		}
		else if (ident->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			fprintf(fp, "%s%d [%s]\n", LSCRIPTTypeGlobalStore[ident->mScopeEntry->mType], ident->mScopeEntry->mOffset, ident->mName);
		}
	}
}

void print_cil_asignment(FILE *fp, LLScriptExpression *exp, LLScriptScopeEntry* function_scope)
{
	LLScriptLValue *lvalue = (LLScriptLValue *) exp;
	LLScriptIdentifier *ident = lvalue->mIdentifier;
	if (lvalue->mAccessor)
	{
		// Object address loaded, store in to field.
		fprintf(fp, "stfld ");
		print_cil_accessor(fp, lvalue);

		// Load object address.
		print_cil_load_address(fp, exp, function_scope);

		// Load field.
		fprintf(fp, "ldfld ");
		print_cil_accessor(fp, lvalue);
	}
	else
	{
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			// Language semantics require value of assignment to be left on stack. 
			// TODO: Optimise away redundant dup/pop pairs.
			fprintf(fp, "dup\n"); 
			if(is_parameter(ident, function_scope))
			{
				// Parameter, store by name.
				fprintf(fp, "starg.s %s\n", ident->mScopeEntry->mIdentifier);
			}
			else
			{
				// Local, store by index.
				fprintf(fp, "stloc.s %d\n", ident->mScopeEntry->mCount);
			}
		}
		else if (ident->mScopeEntry->mIDType == LIT_GLOBAL)
		{
			// Object address loaded, store in to field.
			fprintf(fp, "stfld ");
			print_cil_member(fp, ident);

			// Load object address.
			print_cil_load_address(fp, exp, function_scope);

			// Load field.
			fprintf(fp, "ldfld ");
			print_cil_member(fp, ident);
		}
	}
}

void print_cast(FILE *fp, LSCRIPTType ret_type, LSCRIPTType right_type)
{
	if (right_type != ret_type)
	{
		fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[right_type], LSCRIPTTypeNames[ret_type]);
	}
}

void cast2stack(LLScriptByteCodeChunk *chunk, LSCRIPTType ret_type, LSCRIPTType right_type)
{
	if (right_type != ret_type)
	{
		chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
		U8 castbyte = LSCRIPTTypeByte[right_type] | LSCRIPTTypeHi4Bits[ret_type];
		chunk->addByte(castbyte);
	}
}

void operation2stack(LLScriptByteCodeChunk *chunk, LSCRIPTType ret_type, LSCRIPTType right_type)
{
	U8 typebyte = LSCRIPTTypeByte[right_type] | LSCRIPTTypeHi4Bits[ret_type];
	chunk->addByte(typebyte);
}

void store2stack(LLScriptExpression *exp, LLScriptExpression *lv, LLScriptByteCodeChunk *chunk, LSCRIPTType right_type)
{
	LLScriptLValue *lvalue = (LLScriptLValue *)lv;
	LLScriptIdentifier *ident = lvalue->mIdentifier;
	LSCRIPTType rettype = exp->mReturnType;

	if (exp->mRightType != LST_NULL)
	{
		if (legal_binary_expression(rettype, exp->mLeftType, exp->mRightType, exp->mType)) 
			cast2stack(chunk, right_type, exp->mReturnType);
	}
	switch(exp->mReturnType)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STORE]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREG]);
		}
		break;
	case LST_KEY:
	case LST_STRING:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STORES]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREGS]);
		}
		break;
	case LST_LIST:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREL]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREGL]);
		}
		break;
	case LST_VECTOR:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREV]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREGV]);
		}
		break;
	case LST_QUATERNION:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREQ]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREGQ]);
		}
		break;
	default:
		if (ident->mScopeEntry->mIDType == LIT_VARIABLE)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STORE]);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STOREG]);
		}
		break;
	}
	S32 address = ident->mScopeEntry->mOffset + lvalue->mOffset;
	chunk->addInteger(address);
}

void print_cil_numeric_cast(FILE* fp, LSCRIPTType currentArg, LSCRIPTType otherArg)
{
	if((currentArg == LST_INTEGER) && (otherArg == LST_FLOATINGPOINT))
	{
		print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);
	}
}

void LLScriptAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " = ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cast(fp, mReturnType, mRightType);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_assignment(mLeftType, mRightType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType = mLeftType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			store2stack(this, mLValue, chunk, mRightType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mRightType, mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptAssignment::getSize()
{
	return 0;
}

void print_cil_add(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:

		// Numeric addition.
		fprintf(fp, "add\n");
		break;

	case LST_STRING:
	case LST_KEY:

		// String concatenation.
		fprintf(fp, "call string valuetype [mscorlib]System.String::Concat(string, string)");
		break;
	
	case LST_VECTOR:

		// Vector addition.
		// TODO: Inline (requires temporary variables, which must be identified in earlier pass).
		fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'add_vec'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLVector)\n");
		break;

	case LST_QUATERNION:

		// Rotation addition.
		// TODO: Inline (requires temporary variables, which must be identified in earlier pass).
		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'add_quat'(valuetype [LScriptLibrary]LLQuaternion, valuetype [LScriptLibrary]LLQuaternion)\n");
		break;

	case LST_LIST:
		print_cil_box(fp, right_type);
		fprintf(fp, "call class [mscorlib]System.Collections.ArrayList class [LScriptLibrary]LScriptInternal::AddReturnList(class [mscorlib]System.Collections.ArrayList, object)\n");
		break;

	default:
		break;
	}
}

void LLScriptAddAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " += ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "ADD %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
			operation2stack(chunk, mReturnType, mRightType);
			store2stack(this, mLValue, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mLValue->mReturnType, mRightSide->mReturnType);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mRightSide->mReturnType, mLValue->mReturnType);
			print_cil_add(fp, mLValue->mReturnType, mRightSide->mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptAddAssignment::getSize()
{
	return 0;
}

void print_cil_sub(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:

		// Numeric subtraction.
		fprintf(fp, "sub\n");
		break;
	
	case LST_VECTOR:

		// Vector subtraction.
		// TODO: Inline (requires temporary variables, which must be identified in earlier pass).
		fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'subtract_vec'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLVector)\n");
		break;

	case LST_QUATERNION:

		// Rotation subtraction.
		// TODO: Inline (requires temporary variables, which must be identified in earlier pass).
		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'subtract_quat'(valuetype [LScriptLibrary]LLQuaternion, valuetype [LScriptLibrary]LLQuaternion)\n");
		break;

	default:

		// Error.
		break;
	}
}

void LLScriptSubAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " -= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "SUB %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
			operation2stack(chunk, mReturnType, mRightType);
			store2stack(this, mLValue, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mLValue->mReturnType, mRightSide->mReturnType);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mRightSide->mReturnType, mLValue->mReturnType);
			print_cil_sub(fp, mLValue->mReturnType, mRightSide->mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptSubAssignment::getSize()
{
	return 0;
}

void print_cil_mul(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:

		// Numeric multiplication.
		fprintf(fp, "mul\n");
		break;
	
	case LST_VECTOR:

		switch(right_type)
		{
		case LST_INTEGER:

			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);

		case LST_FLOATINGPOINT:

			// Vector scaling.
			fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'multiply_float'(valuetype [LScriptLibrary]LLVector, float32)\n");
			break;

		case LST_VECTOR:

			// Dot product.
			fprintf(fp, "call float32 valuetype [LScriptLibrary]LLVector::'multiply_vec'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLVector)\n");
			break;

		case LST_QUATERNION:

			// Vector rotation.
			fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'multiply_quat'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLQuaternion)\n");
			break;

		default:
			break;
		}
		break;

	case LST_QUATERNION:

		// Rotation multiplication.
		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'multiply_quat'(valuetype [LScriptLibrary]LLQuaternion, valuetype [LScriptLibrary]LLQuaternion)\n");
		break;

	default:

		// Error.
		break;
	}
}

void LLScriptMulAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " *= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "MUL %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_MUL]);
			operation2stack(chunk, mReturnType, mRightType);
			store2stack(this, mLValue, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mLValue->mReturnType, mRightSide->mReturnType);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mRightSide->mReturnType, mLValue->mReturnType);
			print_cil_mul(fp, mLValue->mReturnType, mRightSide->mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptMulAssignment::getSize()
{
	return 0;
}

void print_cil_div(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:

		// Numeric addition.
		fprintf(fp, "div\n");
		break;
	
	case LST_VECTOR:

		switch(right_type)
		{
		case LST_INTEGER:

			print_cil_cast(fp, LST_INTEGER, LST_FLOATINGPOINT);

		case LST_FLOATINGPOINT:

			// Scale.
			fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'divide_float'(valuetype [LScriptLibrary]LLVector, float32)\n");
			break;

		case LST_QUATERNION:

			// Inverse rotation.
			fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'divide_quat'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLQuaternion)\n");
			break;

		default:
			break;
		}
		break;

	case LST_QUATERNION:

		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'divide_quat'(valuetype [LScriptLibrary]LLQuaternion, valuetype [LScriptLibrary]LLQuaternion)\n");		
		break;

	default:

		// Error.
		break;
	}
}

void LLScriptDivAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " /= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "DIV %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_DIV]);
			operation2stack(chunk, mReturnType, mRightType);
			store2stack(this, mLValue, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mLValue->mReturnType, mRightSide->mReturnType);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_numeric_cast(fp, mRightSide->mReturnType, mLValue->mReturnType);
			print_cil_div(fp, mLValue->mReturnType, mRightSide->mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptDivAssignment::getSize()
{
	return 0;
}

void print_cil_mod(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:

		// Numeric remainder.
		fprintf(fp, "rem\n");
		break;
	
	case LST_VECTOR:

		// Vector cross product.
		fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'mod_vec'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLVector)\n");
		break;

	default:

		// Error.
		break;
	}
}

void LLScriptModAssignment::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " %%= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "MOD %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
			print_asignment(fp, mLValue);
		}
		break;
	case LSCP_TYPE:
		{
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_MOD]);
			operation2stack(chunk, mReturnType, mRightType);
			store2stack(this, mLValue, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mLValue, entry);
			mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			print_cil_mod(fp, mLValue->mReturnType, mRightSide->mReturnType);
			print_cil_asignment(fp, mLValue, entry);
		}
		break;
	default:
		mLValue->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptModAssignment::getSize()
{
	return 0;
}

void print_cil_eq(FILE* fp, LSCRIPTType left_type, LSCRIPTType right_type)
{
	switch(left_type)
	{
	case LST_INTEGER:
	case LST_FLOATINGPOINT:

		// Numeric equality.
		fprintf(fp, "ceq\n");
		break;

	case LST_STRING:
	case LST_KEY:

		// String equality.
		fprintf(fp, "call bool valuetype [mscorlib]System.String::op_Equality(string, string)\n");
		break;
	
	case LST_VECTOR:

		// Vector equality.
		fprintf(fp, "call bool [LScriptLibrary]LLVector::'equals_vec'(valuetype [LScriptLibrary]LLVector, valuetype [LScriptLibrary]LLVector)\n");
		break;

	case LST_QUATERNION:

		// Rotation equality.
		fprintf(fp, "call bool [LScriptLibrary]LLQuaternion::'equals_quat'(valuetype [LScriptLibrary]LLQuaternion, valuetype [LScriptLibrary]LLQuaternion)\n");
		break;

	case LST_LIST:
		fprintf(fp, "call bool [LScriptLibrary]LScriptInternal::EqualsList(class [mscorlib]System.Collections.ArrayList, class [mscorlib]System.Collections.ArrayList)\n");
		break;

	default:

		// Error.
		break;
	}
}

void LLScriptEquality::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " == ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "EQ %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_EQ]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mRightSide->mReturnType, mLeftSide->mReturnType);
		print_cil_eq(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptEquality::getSize()
{
	return 0;
}

void LLScriptNotEquals::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " != ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "NEQ %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_NEQ]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "ceq\n");
		fprintf(fp, "ldc.i4.0\n");
		fprintf(fp, "ceq\n"); // Compare result of first compare equal with 0 to get compare not equal.
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptNotEquals::getSize()
{
	return 0;
}

void LLScriptLessEquals::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " <= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "LEQ %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_LEQ]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "cgt\n"); // Test greater than.
		fprintf(fp, "ldc.i4.0\n"); // Use (b == 0) implementation of boolean not.
		fprintf(fp, "ceq\n"); // Apply boolean not to greater than. If not greater than, then less or equal.
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptLessEquals::getSize()
{
	return 0;
}

void LLScriptGreaterEquals::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >= ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "GEQ %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_GEQ]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "clt\n"); // Test less than.
		fprintf(fp, "ldc.i4.0\n"); // Use (b == 0) implementation of boolean not.
		fprintf(fp, "ceq\n"); // Apply boolean not to less than. If not less than, then greater or equal.
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptGreaterEquals::getSize()
{
	return 0;
}

void LLScriptLessThan::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " < ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "LESS %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_LESS]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "clt\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptLessThan::getSize()
{
	return 0;
}

void LLScriptGreaterThan::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " > ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "GREATER %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_GREATER]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "cgt\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptGreaterThan::getSize()
{
	return 0;
}

void LLScriptPlus::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " + ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "ADD %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mRightSide->mReturnType, mLeftSide->mReturnType);
		print_cil_add(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPlus::getSize()
{
	return 0;
}

void LLScriptMinus::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " - ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "SUB %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mRightSide->mReturnType, mLeftSide->mReturnType);
		print_cil_sub(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptMinus::getSize()
{
	return 0;
}

void LLScriptTimes::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " * ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "MUL %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_MUL]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mRightSide->mReturnType, mLeftSide->mReturnType);
		print_cil_mul(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptTimes::getSize()
{
	return 0;
}

void LLScriptDivide::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " / ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "DIV %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_DIV]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_numeric_cast(fp, mRightSide->mReturnType, mLeftSide->mReturnType);
		print_cil_div(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptDivide::getSize()
{
	return 0;
}

void LLScriptMod::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " %% ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "MOD %s, %s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mRightType] | LSCRIPTTypeHi4Bits[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_MOD]);
			chunk->addByte(typebyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_mod(fp, mLeftSide->mReturnType, mRightSide->mReturnType);
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptMod::getSize()
{
	return 0;
}

void LLScriptBitAnd::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " & ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BITAND\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BITAND]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "and\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBitAnd::getSize()
{
	return 0;
}

void LLScriptBitOr::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " | ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BITOR\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BITOR]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "or\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBitOr::getSize()
{
	return 0;
}

void LLScriptBitXor::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ^ ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BITXOR\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BITXOR]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "xor\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBitXor::getSize()
{
	return 0;
}

void LLScriptBooleanAnd::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " && ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BOOLAND\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BOOLAND]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "and\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBooleanAnd::getSize()
{
	return 0;
}

void LLScriptBooleanOr::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " || ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BOOLOR\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BOOLOR]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "or\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBooleanOr::getSize()
{
	return 0;
}

void LLScriptShiftLeft::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " << ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "SHL\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_SHL]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "shl\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptShiftLeft::getSize()
{
	return 0;
}


void LLScriptShiftRight::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >> ");
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "SHR\n");
		break;
	case LSCP_TYPE:
		{
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftType = type;
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mRightType = type;
			if (!legal_binary_expression(mReturnType, mLeftType, mRightType, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			type = mReturnType;
		}
		break;
	case LSCP_TO_STACK:
		{
			mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_SHR]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "shr\n");
		break;
	default:
		mLeftSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightSide->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptShiftRight::getSize()
{
	return 0;
}

void LLScriptParenthesis::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )");
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mReturnType = mLeftType = type;
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mReturnType = mLeftType = type;
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptParenthesis::getSize()
{
	return 0;
}

void LLScriptUnaryMinus::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "-");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "NEG %s\n", LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_unary_expression(type, type, mType))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		else
		{
			mReturnType = mLeftType = type;
		}
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			U8 typebyte = LSCRIPTTypeByte[mLeftType];
			chunk->addByte(LSCRIPTOpCodes[LOPC_NEG]);
			chunk->addByte(typebyte);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptUnaryMinus::getSize()
{
	return 0;
}

void LLScriptBooleanNot::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "!");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BOOLNOT\n");
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_unary_expression(type, type, mType))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		else
		{
			mReturnType = mLeftType = type;
		}
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BOOLNOT]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "ldc.i4.0\n");
		fprintf(fp, "ceq\n"); // If f(e) is (e == 0), f(e) returns 1 if e is 0 and 0 otherwise, therefore f(e) implements boolean not.
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBooleanNot::getSize()
{
	return 0;
}

void LLScriptBitNot::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "~");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "BITNOT\n");
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_unary_expression(type, type, mType))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		else
		{
			mReturnType = mLeftType = type;
		}
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_BITNOT]);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "not\n");
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptBitNot::getSize()
{
	return 0;
}

void LLScriptPreIncrement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "++");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "PUSHARGI 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "\n");
				fprintf(fp, "ADD integer, integer\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "PUSHARGF 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "\n");
				fprintf(fp, "ADD float, float\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_asignment(fp, mExpression);
		}
		break;
	case LSCP_TYPE:
		if (mExpression->mType != LET_LVALUE)
		{
			gErrorToText.writeError(fp, this, LSERROR_EXPRESSION_ON_LVALUE);
		}
		else
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_unary_expression(type, type, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			else
			{
				mReturnType = mLeftType = type;
			}
		}
		break;
	case LSCP_TO_STACK:
		{
			if (mReturnType == LST_INTEGER)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
				chunk->addInteger(1);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
				chunk->addByte(LSCRIPTTypeByte[LST_INTEGER] | LSCRIPTTypeHi4Bits[LST_INTEGER]);
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGF]);
				chunk->addFloat(1.f);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
				chunk->addByte(LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[LST_FLOATINGPOINT]);
			}
			store2stack(this, mExpression, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mExpression, entry);
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "ldc.i4.1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "add\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "ldc.r8.1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "add\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_cil_asignment(fp, mExpression, entry);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPreIncrement::getSize()
{
	return 0;
}

void LLScriptPreDecrement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "--");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "PUSHARGI 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "\n");
				fprintf(fp, "SUB integer, integer\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "PUSHARGF 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "\n");
				fprintf(fp, "SUB float, float\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_asignment(fp, mExpression);
		}
		break;
	case LSCP_TYPE:
		if (mExpression->mType != LET_LVALUE)
		{
			gErrorToText.writeError(fp, this, LSERROR_EXPRESSION_ON_LVALUE);
		}
		else
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_unary_expression(type, type, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			else
			{
				mReturnType = mLeftType = type;
			}
		}
		break;
	case LSCP_TO_STACK:
		{
			if (mReturnType == LST_INTEGER)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
				chunk->addInteger(1);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
				chunk->addByte(LSCRIPTTypeByte[LST_INTEGER] | LSCRIPTTypeHi4Bits[LST_INTEGER]);
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGF]);
				chunk->addFloat(1.f);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
				chunk->addByte(LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[LST_FLOATINGPOINT]);
			}
			store2stack(this, mExpression, chunk, mReturnType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mExpression, entry);
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "ldc.i4.1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "sub\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "ldc.r8.1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "sub\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_cil_asignment(fp, mExpression, entry);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPreDecrement::getSize()
{
	return 0;
}

void LLScriptTypeCast::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "( ");
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ") ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mRightType], LSCRIPTTypeNames[mType->mType]);
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mRightType = type;
		if (!legal_casts(mType->mType, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		type = mType->mType;
		mReturnType = mLeftType = type;
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[mType->mType] | LSCRIPTTypeHi4Bits[mRightType];
			chunk->addByte(castbyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		print_cil_cast(fp, mRightType, mType->mType);
		break;
	default:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptTypeCast::getSize()
{
	return 0;
}

void LLScriptVectorInitializer::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "< ");
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression1->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression2->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression3->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		break;
	case LSCP_TYPE:
		// vector's take floats
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mReturnType = type = LST_VECTOR;
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TO_STACK:
		pass = LSCP_TO_STACK;
		mExpression1->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression1->mReturnType];
			chunk->addByte(castbyte);
		}
		mExpression2->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression2->mReturnType];
			chunk->addByte(castbyte);
		}
		mExpression3->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression3->mReturnType];
			chunk->addByte(castbyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Load arguments.
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression1->mReturnType, LST_FLOATINGPOINT);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression2->mReturnType, LST_FLOATINGPOINT);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression3->mReturnType, LST_FLOATINGPOINT);
		}
		// Call named ctor, which leaves new Vector on stack, so it can be saved in to local or argument just like a primitive type.
		fprintf(fp, "call valuetype [LScriptLibrary]LLVector valuetype [LScriptLibrary]LLVector::'create'(float32, float32, float32)\n");
		break;
	default:
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptVectorInitializer::getSize()
{
	return 0;
}

void LLScriptQuaternionInitializer::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "< ");
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ", ");
		mExpression4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " >");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression1->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression2->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression3->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		mExpression4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression4->mReturnType != LST_FLOATINGPOINT)
		{
			fprintf(fp, "CAST %s->%s\n", LSCRIPTTypeNames[mExpression4->mReturnType], LSCRIPTTypeNames[LST_FLOATINGPOINT]);
		}
		break;
	case LSCP_TYPE:
		// vector's take floats
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mExpression4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!legal_assignment(LST_FLOATINGPOINT, type))
		{
			gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
		}
		mReturnType = type = LST_QUATERNION;
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TO_STACK:
		pass = LSCP_TO_STACK;
		mExpression1->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression1->mReturnType];
			chunk->addByte(castbyte);
		}
		mExpression2->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression2->mReturnType];
			chunk->addByte(castbyte);
		}
		mExpression3->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression3->mReturnType];
			chunk->addByte(castbyte);
		}
		mExpression4->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression4->mReturnType != LST_FLOATINGPOINT)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CAST]);
			U8 castbyte = LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[mExpression4->mReturnType];
			chunk->addByte(castbyte);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Load arguments.
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression1->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression1->mReturnType, LST_FLOATINGPOINT);
		}
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression2->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression2->mReturnType, LST_FLOATINGPOINT);
		}
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression3->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression3->mReturnType, LST_FLOATINGPOINT);
		}
		mExpression4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression4->mReturnType != LST_FLOATINGPOINT)
		{
			print_cil_cast(fp, mExpression4->mReturnType, LST_FLOATINGPOINT);
		}

		// Call named ctor, which leaves new Vector on stack, so it can be saved in to local or argument just like a primitive type.
		fprintf(fp, "call valuetype [LScriptLibrary]LLQuaternion valuetype [LScriptLibrary]LLQuaternion::'create'(float32, float32, float32, float32)\n");
		break;
	default:
		mExpression1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression3->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression4->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptQuaternionInitializer::getSize()
{
	return 0;
}

void LLScriptListInitializer::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, "[ ");
		mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ]");
		break;
	case LSCP_EMIT_ASSEMBLY:
		count = 0;
		if (mExpressionList)
		{
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "STACKTOL %llu\n", count);
		}
		break;
	case LSCP_TYPE:
		if (mExpressionList)
		{
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mReturnType = type = LST_LIST;
		}
		mReturnType = type = LST_LIST;
		break;
	case LSCP_TO_STACK:
		if (mExpressionList)
		{
			pass = LSCP_TO_STACK;
			count = 0;
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_STACKTOL]);
			chunk->addInteger((S32)count);
			count = 0;
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_STACKTOL]);
			chunk->addInteger(0);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Push boxed elements on stack.
		count = 0;
		if (mExpressionList)
		{
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}

		// Create list on stack, consuming first boxed element.
		fprintf(fp, "call class [mscorlib]System.Collections.ArrayList class [LScriptLibrary]LScriptInternal::CreateList()\n");

		// Call AddReturnList to add remaining boxed expressions.
		for(U64 i = 0; i < count; i++)
		{
			fprintf(fp, "call class [mscorlib]System.Collections.ArrayList class [LScriptLibrary]LScriptInternal::AddReturnList(object, class [mscorlib]System.Collections.ArrayList)\n");
		}
		count = 0;
		
		break;
	default:
		if (mExpressionList)
		{
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptListInitializer::getSize()
{
	return 0;
}

void LLScriptPostIncrement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "++");
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "PUSHARGI 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "ADD integer, integer\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "PUSHARGF 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "ADD float, float\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_asignment(fp, mExpression);
			fprintf(fp, "%s\n", LSCRIPTTypePop[mReturnType]);
		}
		break;
	case LSCP_TYPE:
		if (mExpression->mType != LET_LVALUE)
		{
			gErrorToText.writeError(fp, this, LSERROR_EXPRESSION_ON_LVALUE);
		}
		else
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_unary_expression(type, type, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			else
			{
				mReturnType = mLeftType = type;
			}
		}
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mReturnType == LST_INTEGER)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
				chunk->addInteger(1);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
				chunk->addByte(LSCRIPTTypeByte[LST_INTEGER] | LSCRIPTTypeHi4Bits[LST_INTEGER]);
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGF]);
				chunk->addFloat(1.f);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
				chunk->addByte(LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[LST_FLOATINGPOINT]);
			}
			store2stack(this, mExpression, chunk, mReturnType);
			switch(mReturnType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_KEY:
			case LST_STRING:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
				break;
			case LST_LIST:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
				break;
			default:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mExpression, entry);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp,"dup\n"); // Copy expression result to use as increment operand.
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "ldc.i4.1\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "ldc.r8.1\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			fprintf(fp, "add\n");
			print_cil_asignment(fp, mExpression, entry);
			fprintf(fp, "pop\n"); // Pop assignment result to leave original expression result on stack. TODO: Optimise away redundant pop/dup pairs.
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPostIncrement::getSize()
{
	return 0;
}

void LLScriptPostDecrement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "--");
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "PUSHARGI 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "SUB integer, integer\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "PUSHARGF 1\n");
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, "SUB float, float\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			print_asignment(fp, mExpression);
			fprintf(fp, "%s\n", LSCRIPTTypePop[mReturnType]);
		}
		break;
	case LSCP_TYPE:
		if (mExpression->mType != LET_LVALUE)
		{
			gErrorToText.writeError(fp, this, LSERROR_EXPRESSION_ON_LVALUE);
		}
		else
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_unary_expression(type, type, mType))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			else
			{
				mReturnType = mLeftType = type;
			}
		}
		break;
	case LSCP_TO_STACK:
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mReturnType == LST_INTEGER)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
				chunk->addInteger(1);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
				chunk->addByte(LSCRIPTTypeByte[LST_INTEGER] | LSCRIPTTypeHi4Bits[LST_INTEGER]);
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGF]);
				chunk->addFloat(1.f);
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				chunk->addByte(LSCRIPTOpCodes[LOPC_SUB]);
				chunk->addByte(LSCRIPTTypeByte[LST_FLOATINGPOINT] | LSCRIPTTypeHi4Bits[LST_FLOATINGPOINT]);
			}
			store2stack(this, mExpression, chunk, mReturnType);
			switch(mReturnType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_KEY:
			case LST_STRING:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
				break;
			case LST_LIST:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
				break;
			default:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			print_cil_load_address(fp, mExpression, entry);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp,"dup\n"); // Copy expression result to use as decrement operand.
			if (mReturnType == LST_INTEGER)
			{
				fprintf(fp, "ldc.i4.1\n");
			}
			else if (mReturnType == LST_FLOATINGPOINT)
			{
				fprintf(fp, "ldc.r8.1\n");
			}
			else
			{
				fprintf(fp, "Unexpected Type\n");
			}
			fprintf(fp, "sub\n");
			print_cil_asignment(fp, mExpression, entry);
			fprintf(fp, "pop\n"); // Pop assignment result to leave original expression result on stack. TODO: Optimise away redundant pop/dup pairs.
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPostDecrement::getSize()
{
	return 0;
}

// Generate arg list.
void print_cil_arg_list(FILE *fp, LLScriptFuncExpressionList* exp_list)
{
	// Print first argument.
	print_cil_type(fp, exp_list->mFirstp->mReturnType);

	// Recursively print next arguments.
	if(exp_list->mSecondp != NULL)
	{
		fprintf(fp, ", ");
		print_cil_arg_list(fp, (LLScriptFuncExpressionList*) exp_list->mSecondp);
	}
}

void LLScriptFunctionCall::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "( ");
		if (mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )");
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mIdentifier->mScopeEntry->mType)
			fprintf(fp, "%s\n", LSCRIPTTypePush[mIdentifier->mScopeEntry->mType]);
		fprintf(fp,"PUSHE\n");
		fprintf(fp, "PUSHBP\n");
		if (mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, 0, NULL);
		fprintf(fp, "PUSHARGE %d\n", mIdentifier->mScopeEntry->mSize - mIdentifier->mScopeEntry->mOffset);
		fprintf(fp, "PUSHSP\n");
		fprintf(fp, "PUSHARGI %d\n", mIdentifier->mScopeEntry->mSize);
		fprintf(fp, "ADD integer, integer\n");
		fprintf(fp, "POPBP\n");
		if (mIdentifier->mScopeEntry->mIDType != LIT_LIBRARY_FUNCTION)
		{
			fprintf(fp, "CALL ");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		else
		{
			fprintf(fp, "CALLLID ");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, ", %d", (U32)mIdentifier->mScopeEntry->mLibraryNumber);
		}
		fprintf(fp, "\n");
		fprintf(fp, "POPBP\n");
		break;
	case LSCP_SCOPE_PASS1:
		if (mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_SCOPE_PASS2:
		{
			LLScriptScopeEntry *entry = scope->findEntryTyped(mIdentifier->mName, LIT_FUNCTION);
			if (!entry)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
			else
			{
				// if we did find it, make sure this identifier is associated with the correct scope entry
				mIdentifier->mScopeEntry = entry;
			}
			if (mExpressionList)
				mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		if (mIdentifier->mScopeEntry)
		{
			U64 argcount = 0;
			if (mExpressionList)
				mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, argcount, chunk, heap, stacksize, mIdentifier->mScopeEntry, 0, NULL);

			if (!mIdentifier->mScopeEntry->mFunctionArgs.mString)
			{
				if (argcount)
				{
					gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
				}
			}
			else if (argcount != strlen(mIdentifier->mScopeEntry->mFunctionArgs.mString))
			{
				gErrorToText.writeError(fp, this, LSERROR_FUNCTION_TYPE_ERROR);
			}
		}

		if (mIdentifier->mScopeEntry)
			type = mIdentifier->mScopeEntry->mType;
		else
			type = LST_NULL;
		mReturnType = type;
		break;
	case LSCP_TO_STACK:
		switch(mIdentifier->mScopeEntry->mType)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
		case LST_STRING:
		case LST_KEY:
		case LST_LIST:
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHE]);
			break;
		case LST_VECTOR:
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHEV]);
			break;
		case LST_QUATERNION:
			chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHEQ]);
			break;
		default:
			break;
		}
		chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHE]);
		chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHBP]);
		if (mExpressionList)
		{
			// Don't let this change the count.
			U64 dummy_count = 0;
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, dummy_count, chunk, heap, stacksize, mIdentifier->mScopeEntry, 0, NULL);
			//mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, 0, NULL);
		}
		chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGE]);
		chunk->addInteger(mIdentifier->mScopeEntry->mSize - mIdentifier->mScopeEntry->mOffset);
		chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHSP]);
		chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
		chunk->addInteger(mIdentifier->mScopeEntry->mSize);
		chunk->addByte(LSCRIPTOpCodes[LOPC_ADD]);
		chunk->addByte(LSCRIPTTypeByte[LST_INTEGER] | LSCRIPTTypeHi4Bits[LST_INTEGER]);
		chunk->addByte(LSCRIPTOpCodes[LOPC_POPBP]);
		if (mIdentifier->mScopeEntry->mIDType != LIT_LIBRARY_FUNCTION)
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CALL]);
			chunk->addInteger(mIdentifier->mScopeEntry->mCount);
		}
		else
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_CALLLIB_TWO_BYTE]);
			chunk->addU16(mIdentifier->mScopeEntry->mLibraryNumber);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			bool library_call = (mIdentifier->mScopeEntry->mIDType == LIT_LIBRARY_FUNCTION);
			if(! library_call)
			{
				// Load this pointer.
				fprintf(fp, "ldarg.0\n");
			}

			// Load args on to stack.
			if (mExpressionList)
			{
				mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry /* Needed for is_parameter calls */, 0, NULL);
			}

			// Make call.
			if (! library_call)
			{
				fprintf(fp, "callvirt instance ");
			}
			else
			{
				fprintf(fp, "call ");
			}
			print_cil_type(fp, mIdentifier->mScopeEntry->mType);
			fprintf(fp, " class ");
			if (library_call)
			{
				fprintf(fp, "[LScriptLibrary]LScriptLibrary");
			}
			else
			{
				fprintf(fp, "LSL");
			}
			fprintf(fp, "::");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "(");
			if (mExpressionList) {print_cil_arg_list(fp, (LLScriptFuncExpressionList*) mExpressionList);}
			fprintf(fp, ")\n");
		}
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptFunctionCall::getSize()
{
	return 0;
}

void LLScriptPrint::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fprintf(fp, " PRINT ( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "PRINT %s\n", LSCRIPTTypeNames[mLeftType]);
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mLeftType = type;
		mReturnType = LST_NULL;
		break;
	case LSCP_TO_STACK:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		chunk->addByte(LSCRIPTOpCodes[LOPC_PRINT]);
		chunk->addByte(LSCRIPTTypeByte[mLeftType]);
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptPrint::getSize()
{
	return 0;
}

void LLScriptConstantExpression::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_TYPE:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mReturnType = type;
		break;
	case LSCP_TO_STACK:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	default:
		mConstant->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptConstantExpression::getSize()
{
	return 0;
}

void LLScriptStatement::addStatement(LLScriptStatement *event)
{
	if (mNextp)
	{
		event->mNextp = mNextp;
	}
	mNextp = event;
}

void LLScriptStatement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	fprintf(fp, "Statement Base Class -- should never get here!\n");
}

S32 LLScriptStatement::getSize()
{
	printf("Statement Base Class -- should never get here!\n");
	return 0;
}

void LLScriptStatement::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptStatementSequence::getSize()
{
	return 0;
}

void LLScriptStatementSequence::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_PRUNE:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (prunearg)
		{
			ptype = LSPRUNE_DEAD_CODE;
			gErrorToText.writeWarning(fp, this, LSWARN_DEAD_CODE);
		}
		mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_TYPE:
		// pass the return type into all statements so we can check returns
		{
			LSCRIPTType	return_type = type;
			mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, return_type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			return_type = type;
			mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, return_type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		mFirstp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mSecondp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptNOOP::getSize()
{
	return 0;
}

void LLScriptNOOP::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, ";\n");
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	default:
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void add_exit_pops(LLScriptByteCodeChunk *chunk, LLScriptScopeEntry *entry)
{
	// remember that we need to pop in reverse order
	S32 number, i;

	if (entry->mLocals.mString)
	{
		number = (S32)strlen(entry->mLocals.mString);
		for (i = number - 1; i >= 0; i--)
		{
			switch(entry->mLocals.getType(i))
			{
			case LST_INTEGER:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_STRING:
			case LST_KEY:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
				break;
			case LST_LIST:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
				break;

			default:
				break;
			}
		}
	}

	if (entry->mFunctionArgs.mString)
	{
		number = (S32)strlen(entry->mFunctionArgs.mString);
		for (i = number - 1; i >= 0; i--)
		{
			switch(entry->mFunctionArgs.getType(i))
			{
			case LST_INTEGER:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
				break;
			case LST_STRING:
			case LST_KEY:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
				break;
			case LST_LIST:
				chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
				break;

			default:
				break;
			}
		}
	}
}

void print_exit_pops(FILE *fp, LLScriptScopeEntry *entry)
{
	// remember that we need to pop in reverse order
	S32 number, i;

	if (entry->mLocals.mString)
	{
		number = (S32)strlen(entry->mLocals.mString);
		for (i = number - 1; i >= 0; i--)
		{
			fprintf(fp, "%s", LSCRIPTTypePop[entry->mLocals.getType(i)]);
		}
	}

	if (entry->mFunctionArgs.mString)
	{
		number = (S32)strlen(entry->mFunctionArgs.mString);
		for (i = number - 1; i >= 0; i--)
		{
			fprintf(fp, "%s", LSCRIPTTypePop[entry->mFunctionArgs.getType(i)]);
		}
	}
}


S32 LLScriptStateChange::getSize()
{
	return 0;
}

void LLScriptStateChange::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "state ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ";\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		print_exit_pops(fp, entry);
		fprintf(fp, "STATE ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");
		break;
	case LSCP_PRUNE:
		if (  (ptype == LSPRUNE_GLOBAL_VOIDS)
			||(ptype == LSPRUNE_GLOBAL_NON_VOIDS))
		{
			gErrorToText.writeError(fp, this, LSERROR_STATE_CHANGE_IN_GLOBAL);
		}
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_SCOPE_PASS2:
		{
			LLScriptScopeEntry *entry = scope->findEntryTyped(mIdentifier->mName, LIT_STATE);
			if (!entry)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
			else
			{
				// if we did find it, make sure this identifier is associated with the correct scope entry
				mIdentifier->mScopeEntry = entry;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			add_exit_pops(chunk, entry);
			chunk->addByte(LSCRIPTOpCodes[LOPC_STATE]);
			chunk->addInteger(mIdentifier->mScopeEntry->mCount);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "ldstr \"");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\"\n");
		fprintf(fp, "call void class [LScriptLibrary]LScriptInternal::change_state(string)\n");
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptJump::getSize()
{
	return 0;
}

void LLScriptJump::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "jump ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ";\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "JUMP ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_SCOPE_PASS2:
		{
			LLScriptScopeEntry *entry = scope->findEntryTyped(mIdentifier->mName, LIT_LABEL);
			if (!entry)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
			else
			{
				// if we did find it, make sure this identifier is associated with the correct scope entry
				mIdentifier->mScopeEntry = entry;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMP]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(mIdentifier->mName);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		fprintf(fp, "br ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptLabel::getSize()
{
	return 0;
}

void LLScriptLabel::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "@");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ";\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		fprintf(fp, "LABEL ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");
		break;
	case LSCP_PRUNE:
		// Always clear this flag, to stop pruning after return statements.  A jump
		// might start up code at this label, so we need to stop pruning.
		prunearg = FALSE;
		break;
	case LSCP_SCOPE_PASS1:
		// add labels to scope
		if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_LABEL, LST_NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			chunk->addLabel(mIdentifier->mName);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ":\n");
		break;
	default:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void add_return(LLScriptByteCodeChunk *chunk, LLScriptScopeEntry *entry)
{
	add_exit_pops(chunk, entry);
	chunk->addByte(LSCRIPTOpCodes[LOPC_RETURN]);
}

void print_return(FILE *fp, LLScriptScopeEntry *entry)
{
	print_exit_pops(fp, entry);
	fprintf(fp, "RETURN\n");
}


S32 LLScriptReturn::getSize()
{
	return 0;
}

void LLScriptReturn::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mExpression)
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "return ");
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, ";\n");
		}
		else
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "return;\n");
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "%s\n", LSCRIPTTypeReturn[mType]);
		}
		print_return(fp, entry);
		break;
	case LSCP_PRUNE:
		if (  (ptype == LSPRUNE_GLOBAL_VOIDS)
			||(ptype == LSPRUNE_EVENTS))
		{
			if (mExpression)
			{
				gErrorToText.writeError(fp, this, LSERROR_INVALID_RETURN);
			}
		}
		else if (ptype == LSPRUNE_GLOBAL_NON_VOIDS)
		{
			if (!mExpression)
			{
				gErrorToText.writeError(fp, this, LSERROR_INVALID_VOID_RETURN);
			}
		}
		prunearg = TRUE;
	case LSCP_TYPE:
		// if there is a return expression, it must be promotable to the return type of the function
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_assignment(basetype, type))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
			else
			{
				mType = basetype;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			switch(mType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				chunk->addInteger(-12);
				break;
			case LST_STRING:
			case LST_KEY:
				// use normal store for reference counted types
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADSP]);
				chunk->addInteger(-12);
				break;
			case LST_LIST:
				// use normal store for reference counted types
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADLP]);
				chunk->addInteger(-12);
				break;
			case LST_VECTOR:
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADVP]);
				chunk->addInteger(-20);
				break;
			case LST_QUATERNION:
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADQP]);
				chunk->addInteger(-24);
				break;
			default:
				chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				chunk->addInteger(-12);
				break;
			}
		}
		add_return(chunk, entry);
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		fprintf(fp, "ret\n");
		break;
	default:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptExpressionStatement::getSize()
{
	return 0;
}

void LLScriptExpressionStatement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ";\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mExpression->mReturnType)
		{
			fprintf(fp, "%s\n", LSCRIPTTypePop[mExpression->mReturnType]);
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_EMIT_BYTE_CODE:
		mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		switch(mExpression->mReturnType)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POP]);
			break;
		case LST_STRING:
		case LST_KEY:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPS]);
			break;
		case LST_LIST:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPL]);
			break;
		case LST_VECTOR:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPV]);
			break;
		case LST_QUATERNION:
			chunk->addByte(LSCRIPTOpCodes[LOPC_POPQ]);
			break;
		default:
			break;
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(mExpression->mReturnType)
		{
			fprintf(fp, "pop\n");
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptIf::getSize()
{
	return 0;
}

void LLScriptIf::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "if ( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			S32 tjump =  gTempJumpCount++;
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMPNIF ##Temp Jump %d##\n", tjump);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump);
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mType = type;
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			char jumpname[32];
			sprintf(jumpname, "##Temp Jump %d##", gTempJumpCount++);

			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMPNIF]);
			chunk->addByte(LSCRIPTTypeByte[mType]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addLabel(jumpname);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			S32 tjump = gTempJumpCount++;
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "brfalse LabelTempJump%d\n", tjump);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LabelTempJump%d:\n", tjump);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptIfElse::getSize()
{
	return 0;
}

void LLScriptIfElse::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "if ( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "else\n");
		mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMPNIF ##Temp Jump %d##\n", tjump1);
			mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMP ##Temp Jump %d##\n", tjump2);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump1);
			mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump2);
		}
		break;
	case LSCP_PRUNE:
		{
			BOOL arg1 = TRUE, arg2 = TRUE;
			mStatement1->recurse(fp, tabs, tabsize, pass, ptype, arg1, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mStatement2->recurse(fp, tabs, tabsize, pass, ptype, arg2, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			prunearg = arg1 && arg2;
		}
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mType = type;
		mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			char jumpname1[32];
			sprintf(jumpname1, "##Temp Jump %d##", gTempJumpCount++);
			char jumpname2[32];
			sprintf(jumpname2, "##Temp Jump %d##", gTempJumpCount++);

			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMPNIF]);
			chunk->addByte(LSCRIPTTypeByte[mType]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname1);
			mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMP]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname2);
			chunk->addLabel(jumpname1);
			mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addLabel(jumpname2);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "brfalse LabelTempJump%d\n", tjump1);
			mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "br LabelTempJump%d\n", tjump2);
			fprintf(fp, "LabelTempJump%d:\n", tjump1);
			mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LabelTempJump%d:\n", tjump2);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatement1->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatement2->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	};
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptFor::getSize()
{
	return 0;
}

void LLScriptFor::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "for ( ");
		if(mSequence)
			mSequence->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ; ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ; ");
		if(mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		if(mStatement)
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			if(mSequence)
				mSequence->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump1);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMPNIF ##Temp Jump %d##\n", tjump2);
			if(mStatement)
				mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if(mExpressionList)
				mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMP ##Temp Jump %d##\n", tjump1);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump2);
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_TYPE:
		if(mSequence)
			mSequence->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mType = type;
		if(mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(mStatement)
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			char jumpname1[32];
			sprintf(jumpname1, "##Temp Jump %d##", gTempJumpCount++);
			char jumpname2[32];
			sprintf(jumpname2, "##Temp Jump %d##", gTempJumpCount++);

			if(mSequence)
				mSequence->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addLabel(jumpname1);
			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMPNIF]);
			chunk->addByte(LSCRIPTTypeByte[mType]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname2);
			if(mStatement)
				mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if(mExpressionList)
				mExpressionList->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMP]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname1);
			chunk->addLabel(jumpname2);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			if(mSequence)
				mSequence->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "LabelTempJump%d:\n", tjump1);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "brfalse LabelTempJump%d\n", tjump2);
			if(mStatement)
				mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if(mExpressionList)
				mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "br LabelTempJump%d\n", tjump1);
			fprintf(fp, "LabelTempJump%d:\n", tjump2);
		}
		break;
	default:
		if(mSequence)
			mSequence->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(mExpressionList)
			mExpressionList->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if(mStatement)
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptDoWhile::getSize()
{
	return 0;
}

void LLScriptDoWhile::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "do\n");
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "while( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " );\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump1);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMPIF ##Temp Jump %d##\n", tjump1);
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_TYPE:
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mType = type;
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			char jumpname1[32];
			sprintf(jumpname1, "##Temp Jump %d##", gTempJumpCount++);

			chunk->addLabel(jumpname1);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMPIF]);
			chunk->addByte(LSCRIPTTypeByte[mType]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname1);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			fprintf(fp, "LabelTempJump%d:\n", tjump1);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "brtrue LabelTempJump%d\n", tjump1);
		}
		break;
	default:
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptWhile::getSize()
{
	return 0;
}

void LLScriptWhile::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "while( ");
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " )\n");
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump1);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMPNIF ##Temp Jump %d##\n", tjump2);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "JUMP ##Temp Jump %d##\n", tjump1);
			fprintf(fp, "LABEL ##Temp Jump %d##\n", tjump2);
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_TYPE:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mType = type;
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			char jumpname1[32];
			sprintf(jumpname1, "##Temp Jump %d##", gTempJumpCount++);
			char jumpname2[32];
			sprintf(jumpname2, "##Temp Jump %d##", gTempJumpCount++);

			chunk->addLabel(jumpname1);
			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMPNIF]);
			chunk->addByte(LSCRIPTTypeByte[mType]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname2);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			chunk->addByte(LSCRIPTOpCodes[LOPC_JUMP]);
			chunk->addBytes(LSCRIPTDataSize[LST_INTEGER]);
			chunk->addJump(jumpname1);
			chunk->addLabel(jumpname2);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			S32 tjump1 =  gTempJumpCount++;
			S32 tjump2 =  gTempJumpCount++;
			fprintf(fp, "LabelTempJump%d:\n", tjump1);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "brfalse LabelTempJump%d\n", tjump2);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "br LabelTempJump%d\n", tjump1);
			fprintf(fp, "LabelTempJump%d:\n", tjump2);
		}
		break;
	default:
		mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptDeclaration::getSize()
{
	return mType->getSize();
}

void LLScriptDeclaration::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mExpression)
		{
			fdotabs(fp, tabs, tabsize);
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\t");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, " = ");
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, ";\n");
		}
		else
		{
			fdotabs(fp, tabs, tabsize);
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\t");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, ";\n");
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
			{
				fprintf(fp, "%s%d [%s]\n", LSCRIPTTypeLocalDeclaration[mIdentifier->mScopeEntry->mType], mIdentifier->mScopeEntry->mOffset, mIdentifier->mName);
			}
			else if (mIdentifier->mScopeEntry->mIDType == LIT_GLOBAL)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
		}
		break;
	case LSCP_PRUNE:
		if (ptype == LSPRUNE_DEAD_CODE)
			prunearg = TRUE;
		else
			prunearg = FALSE;
		break;
	case LSCP_SCOPE_PASS1:
		// Check to see if a declaration is valid here.
		if (!mAllowDeclarations)
		{
			gErrorToText.writeError(fp, this, LSERROR_NEED_NEW_SCOPE);
		}
		// add labels to scope
		else if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			if (mExpression)
			{
				mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			}
			// this needs to go after expression decent to make sure that we don't add ourselves or something silly
			// check expression if it exists
			mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_VARIABLE, mType->mType);
		}
		break;
	case LSCP_TYPE:
		// if there is an expression, it must be promotable to variable type
		if (mExpression && mIdentifier->mScopeEntry)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!legal_assignment(mIdentifier->mScopeEntry->mType, type))
			{
				gErrorToText.writeError(fp, this, LSERROR_TYPE_MISMATCH);
			}
		}
		break;
	case LSCP_RESOURCE:
		{
			mIdentifier->mScopeEntry->mOffset = (S32)count;
			mIdentifier->mScopeEntry->mSize = mType->getSize();
			count += mIdentifier->mScopeEntry->mSize;
			// Index into locals is current number of locals. Stored in mCount member of mScopeEntry.
			mIdentifier->mScopeEntry->mCount = entry->mLocals.getNumber();
			entry->mLocals.addType(mType->mType);
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, LSCP_TO_STACK, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mExpression->mReturnType != mIdentifier->mScopeEntry->mType)
			{
				cast2stack(chunk, mExpression->mReturnType, mIdentifier->mScopeEntry->mType);
			}
			switch(mExpression->mReturnType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				}
				break;
			case LST_STRING:
			case LST_KEY:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADSP]);
				}
				break;
			case LST_LIST:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADLP]);
				}
				break;
			case LST_VECTOR:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADVP]);
				}
				break;
			case LST_QUATERNION:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADQP]);
				}
				break;
			default:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				}
				break;
			}
			if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
			{
				S32 address = mIdentifier->mScopeEntry->mOffset;
				chunk->addInteger(address);
			}
		}
		else
		{
			switch(mIdentifier->mScopeEntry->mType)
			{
			case LST_INTEGER:
			case LST_FLOATINGPOINT:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
					chunk->addInteger(0);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				}
				break;
			case LST_STRING:
			case LST_KEY:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGS]);
					chunk->addBytes("", 1);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADSP]);
				}
				break;
			case LST_LIST:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_STACKTOL]);
					chunk->addInteger(0);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADLP]);
				}
				break;
			case LST_VECTOR:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGV]);
					chunk->addFloat(0);
					chunk->addFloat(0);
					chunk->addFloat(0);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADVP]);
				}
				break;
			case LST_QUATERNION:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGQ]);
					chunk->addFloat(1);
					chunk->addFloat(0);
					chunk->addFloat(0);
					chunk->addFloat(0);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADQP]);
				}
				break;
			default:
				if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
				{
					chunk->addByte(LSCRIPTOpCodes[LOPC_PUSHARGI]);
					chunk->addInteger(0);
					chunk->addByte(LSCRIPTOpCodes[LOPC_LOADP]);
				}
				break;
			}
			if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
			{
				S32 address = mIdentifier->mScopeEntry->mOffset;
				chunk->addInteger(address);
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		if (mExpression)
		{
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mIdentifier->mScopeEntry->mIDType == LIT_VARIABLE)
			{
				if(is_parameter(mIdentifier, entry))
				{
					// Parameter, store by name.
					fprintf(fp, "starg.s %s\n", mIdentifier->mScopeEntry->mIdentifier);
				}
				else
				{
					// Local, store by index.
					fprintf(fp, "stloc.s %d\n", mIdentifier->mScopeEntry->mCount);
				}
			}
			else if (mIdentifier->mScopeEntry->mIDType == LIT_GLOBAL)
			{
				gErrorToText.writeError(fp, this, LSERROR_UNDEFINED_NAME);
			}
		}
		break;
	default:
		if (mExpression)
		{
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mExpression->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		else
		{
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptCompoundStatement::getSize()
{
	return 0;
}

void LLScriptCompoundStatement::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mStatement)
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
			mStatement->recurse(fp, tabs + 1, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "}\n");
		}
		else
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "}\n");
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mStatement)
		{
			mStatement->recurse(fp, tabs + 1, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_PRUNE:
		if (mStatement)
		{
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		else
		{
			prunearg = FALSE;
		}
		break;
	case LSCP_SCOPE_PASS1:
		// compound statements create a new scope
		if (mStatement)
		{
			mStatementScope = new LLScriptScope(gScopeStringTable);
			mStatementScope->addParentScope(scope);
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mStatementScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_SCOPE_PASS2:
		// compound statements create a new scope
		if (mStatement)
		{
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mStatementScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mStatement)
		{
			mStatement->recurse(fp, tabs + 1, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void LLScriptEventHandler::addEvent(LLScriptEventHandler *event)
{
	if (mNextp)
	{
		event->mNextp = mNextp;
	}
	mNextp = event;
}

void LLScriptEventHandler::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptEventHandler::getSize()
{
	return mStackSpace;
}

U64 gCurrentHandler = 0;

void print_cil_local_init(FILE* fp, LLScriptScopeEntry* scopeEntry)
{
	if(scopeEntry->mLocals.getNumber() > 0)
	{
		fprintf(fp, ".locals init (");
		for(int local = 0; local < scopeEntry->mLocals.getNumber(); ++local)
		{
			if(local > 0)
			{
				fprintf(fp, ", ");
			}
			print_cil_type(fp, scopeEntry->mLocals.getType(local));
		}
		fprintf(fp, ")\n");
	}
}

void LLScriptEventHandler::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mStatement)
		{
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		else
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "}\n");
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mStatement)
		{
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, getSize(), mScopeEntry, entrycount, NULL);
		}
		if (mbNeedTrailingReturn)
		{
			print_return(fp, mScopeEntry);
		}
		fprintf(fp, "\n");
		break;
	case LSCP_PRUNE:
		mbNeedTrailingReturn = FALSE;
		prunearg = TRUE;
		mStatement->recurse(fp, tabs, tabsize, pass, LSPRUNE_EVENTS, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (!prunearg)
		{
			// this means that we didn't end with a return statement, need to add one
			mbNeedTrailingReturn = TRUE;
		}
		break;
	case LSCP_SCOPE_PASS1:
		// create event level scope
		mEventScope = new LLScriptScope(gScopeStringTable);
		mEventScope->addParentScope(scope);

		// add event parameters
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mEventScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);

		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mEventScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_SCOPE_PASS2:
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mEventScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_TYPE:
		mScopeEntry = new LLScriptScopeEntry("Event", LIT_HANDLER, LST_NULL);
		switch(mEventp->mType)
		{
		case LSTT_STATE_ENTRY:
			break;
		case LSTT_STATE_EXIT:
			break;
		case LSTT_TOUCH_START:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_TOUCH:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_TOUCH_END:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_COLLISION_START:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_COLLISION:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_COLLISION_END:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_LAND_COLLISION_START:
			mScopeEntry->mFunctionArgs.addType(LST_VECTOR);
			break;
		case LSTT_LAND_COLLISION:
			mScopeEntry->mFunctionArgs.addType(LST_VECTOR);
			break;
		case LSTT_LAND_COLLISION_END:
			mScopeEntry->mFunctionArgs.addType(LST_VECTOR);
			break;
		case LSTT_INVENTORY:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_ATTACH:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			break;
		case LSTT_DATASERVER:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			break;
		case LSTT_TIMER:
			break;
		case LSTT_MOVING_START:
			break;
		case LSTT_MOVING_END:
			break;
		case LSTT_OBJECT_REZ:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			break;
		case LSTT_REMOTE_DATA:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			break;
		case LSTT_CHAT:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			break;
		case LSTT_SENSOR:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_CONTROL:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_LINK_MESSAGE:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			break;
		case LSTT_MONEY:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_EMAIL:
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_REZ:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_NO_SENSOR:
			break;
		case LSTT_AT_TARGET:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_VECTOR);
			mScopeEntry->mFunctionArgs.addType(LST_VECTOR);
			break;
		case LSTT_NOT_AT_TARGET:
			break;
		case LSTT_AT_ROT_TARGET:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_QUATERNION);
			mScopeEntry->mFunctionArgs.addType(LST_QUATERNION);
			break;
		case LSTT_NOT_AT_ROT_TARGET:
			break;
		case LSTT_RTPERMISSIONS:
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			break;
		case LSTT_HTTP_RESPONSE:
			mScopeEntry->mFunctionArgs.addType(LST_KEY);
			mScopeEntry->mFunctionArgs.addType(LST_INTEGER);
			mScopeEntry->mFunctionArgs.addType(LST_LIST);
			mScopeEntry->mFunctionArgs.addType(LST_STRING);
			break;

		default:
			break;
		}
		mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_RESOURCE:
		// first determine resource counts for globals
		count = 0;
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mStatement)
		{
			entrycount = 0;
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mScopeEntry, entrycount, NULL);
			fprintf(fp, "Function Args: %s\n", mScopeEntry->mFunctionArgs.mString);
			fprintf(fp, "Local List: %s\n", mScopeEntry->mLocals.mString);
		}
		mStackSpace = (S32)count;
		break;
	case LSCP_DETERMINE_HANDLERS:
		count |= LSCRIPTStateBitField[mEventp->mType];
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// order for event handler
			// set jump table value
			S32 jumpoffset;
			jumpoffset = LSCRIPTDataSize[LST_INTEGER]*get_event_handler_jump_position(gCurrentHandler, mEventp->mType)*2;

			integer2bytestream(chunk->mCodeChunk, jumpoffset, chunk->mCurrentOffset);

			// 0 - 3: offset to actual data
			S32 offsetoffset = chunk->mCurrentOffset;
			S32 offsetdelta = 0;
			chunk->addBytes(4);

			// null terminated event name and null terminated parameters
			if (mEventp)
			{
				LLScriptByteCodeChunk	*event = new LLScriptByteCodeChunk(FALSE);
				mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, event, heap, stacksize, entry, entrycount, NULL);
				chunk->addBytes(event->mCodeChunk, event->mCurrentOffset);
				delete event;
			}
			chunk->addBytes(1);

			// now we're at the first opcode
			offsetdelta = chunk->mCurrentOffset - offsetoffset;
			integer2bytestream(chunk->mCodeChunk, offsetoffset, offsetdelta);

			// get ready to compute the number of bytes of opcode
			offsetdelta = chunk->mCurrentOffset;

			if (mStatement)
			{
				LLScriptByteCodeChunk	*statements = new LLScriptByteCodeChunk(TRUE);
				mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, statements, heap, getSize(), mScopeEntry, entrycount, NULL);
				statements->connectJumps();
				chunk->addBytes(statements->mCodeChunk, statements->mCurrentOffset);
				delete statements;
			}
			if (mbNeedTrailingReturn)
			{
				add_return(chunk, mScopeEntry);
			}
			// now stuff in the number of bytes of stack space that this routine needs
			integer2bytestream(chunk->mCodeChunk, jumpoffset, getSize());
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Method signature prefix.
		fprintf(fp, ".method public hidebysig instance default void ");

		// Mangle event handler name by prefixing it with state name. Allows state changing by finding handlers prefixed with new state name.
		fprintf(fp, entry->mIdentifier);

		// Handler name and arguments.
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);

		// Method signature postfix.
		fprintf(fp, " cil managed\n");

		// Function header.
		fprintf(fp,"{\n");
		fprintf(fp, ".maxstack 500\n"); // TODO: Calculated stack size...

		// Allocate space for locals.
		print_cil_local_init(fp, mScopeEntry);

		if (mStatement)
		{
			// Pass scope so identifiers can determine parameter or local.
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mScopeEntry, entrycount, NULL);
		}

		// Function footer.
		fprintf(fp, "\nret\n"); // TODO: Check whether return needed?
		fprintf(fp, "}\n");

		break;
	default:
		mEventp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mStatement)
		{
			mStatement->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void LLScriptFunctionDec::addFunctionParameter(LLScriptFunctionDec *dec)
{
	if (mNextp)
	{
		dec->mNextp = mNextp;
	}
	mNextp = dec;
}

void LLScriptFunctionDec::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		if (mNextp)
		{
			fprintf(fp, ", ");
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}

}

S32 LLScriptFunctionDec::getSize()
{
	return 0;
}

void LLScriptFunctionDec::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_SCOPE_PASS1:
		// add function names into global scope
		if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_VARIABLE, mType->mType);
		}
		break;
	case LSCP_RESOURCE:
		{
			// we're just tryng to determine how much space the variable needs
			mIdentifier->mScopeEntry->mOffset = (S32)count;
			mIdentifier->mScopeEntry->mSize = mType->getSize();
			count += mIdentifier->mScopeEntry->mSize;
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// return type
			char typereturn;
			if (mType)
			{
				typereturn = LSCRIPTTypeByte[mType->mType];
			}
			else
			{
				typereturn = LSCRIPTTypeByte[LST_NULL]; 
			}
			chunk->addBytes(&typereturn, 1);
			// name
#ifdef LSL_INCLUDE_DEBUG_INFO
			chunk->addBytes(mIdentifier->mName, strlen(mIdentifier->mName) + 1);
#else
			chunk->addBytes(1);
#endif
		}
		break;
	case LSCP_BUILD_FUNCTION_ARGS:
		{
			entry->mFunctionArgs.addType(mType->mType);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, " ");
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	default:
		mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void LLScriptGlobalFunctions::addGlobalFunction(LLScriptGlobalFunctions *global)
{
	if (mNextp)
	{
		global->mNextp = mNextp;
	}
	mNextp = global;
}

void LLScriptGlobalFunctions::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptGlobalFunctions::getSize()
{
	return 0;
}

void LLScriptGlobalFunctions::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		if (mType)
		{
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\t");
		}
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mParameters)
		{
			fprintf(fp, "( ");
			mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, " )\n");
		}
		else
		{
			fprintf(fp, "()\n");
		}
		if (mStatements)
		{
			fdotabs(fp, tabs, tabsize);
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, entrycount, NULL);
		}
		else
		{
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "}\n");
		}
		break;
	case LSCP_EMIT_ASSEMBLY:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mParameters)
		{
			fprintf(fp, "( ");
			mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, " )\n");
		}
		else
		{
			fprintf(fp, "()\n");
		}
		if (mStatements)
		{
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, mIdentifier->mScopeEntry->mSize, mIdentifier->mScopeEntry, entrycount, NULL);
		}
		if (mbNeedTrailingReturn)
		{
			print_return(fp, mIdentifier->mScopeEntry);
		}
		fprintf(fp, "\n");
		break;
	case LSCP_PRUNE:
		mbNeedTrailingReturn = FALSE;
		if (mType)
		{
			prunearg = TRUE;
			mStatements->recurse(fp, tabs, tabsize, pass, LSPRUNE_GLOBAL_NON_VOIDS, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!prunearg)
			{
				gErrorToText.writeError(fp, this, LSERROR_NO_RETURN);
			}
		}
		else
		{
			prunearg = TRUE;
			mStatements->recurse(fp, tabs, tabsize, pass, LSPRUNE_GLOBAL_VOIDS, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (!prunearg)
			{
				// this means that we didn't end with a return statement, need to add one
				mbNeedTrailingReturn = TRUE;
			}
		}
		break;
	case LSCP_SCOPE_PASS1:
		// add function names into global scope
		if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			if (mType)
			{
				mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_FUNCTION, mType->mType);
			}
			else
			{
				mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_FUNCTION, LST_NULL);
			}
		}

		// create function level scope
		mFunctionScope = new LLScriptScope(gScopeStringTable);
		mFunctionScope->addParentScope(scope);

		// function parameters
		if (mParameters)
		{
			mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mFunctionScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}

		mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mFunctionScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_SCOPE_PASS2:
		mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mFunctionScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		
		if (mParameters)
		{
			if (mIdentifier->mScopeEntry)
			{
				mParameters->recurse(fp, tabs, tabsize, LSCP_BUILD_FUNCTION_ARGS, ptype, prunearg, mFunctionScope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, 0, NULL);
			}
		}
		break;
	case LSCP_TYPE:
		if (mType)
		{
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, mType->mType, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		else
		{
			type = LST_NULL;
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_RESOURCE:
		// first determine resource counts for globals
		count = 0;

		if (mParameters)
		{
			mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}

		if (mIdentifier->mScopeEntry)
		{
			// this isn't a bug . . . Offset is used to determine how much is params vs locals
			mIdentifier->mScopeEntry->mOffset = (S32)count;
		}

		if (mStatements)
		{
			entrycount = 0;
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, entrycount, NULL);
			fprintf(fp, "Function Args: %s\n", mIdentifier->mScopeEntry->mFunctionArgs.mString);
			fprintf(fp, "Local List: %s\n", mIdentifier->mScopeEntry->mLocals.mString);
			if (mIdentifier->mScopeEntry)
			{
				mIdentifier->mScopeEntry->mSize = (S32)count;
			}
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// order for global functions
			// set jump table value
			S32 jumpoffset = LSCRIPTDataSize[LST_INTEGER]*mIdentifier->mScopeEntry->mCount + LSCRIPTDataSize[LST_INTEGER];
			integer2bytestream(chunk->mCodeChunk, jumpoffset, chunk->mCurrentOffset);

			// 0 - 3: offset to actual data
			S32 offsetoffset = chunk->mCurrentOffset;
			S32 offsetdelta = 0;
			chunk->addBytes(4);

			// null terminated function name
#ifdef LSL_INCLUDE_DEBUG_INFO
			chunk->addBytes(mIdentifier->mName, strlen(mIdentifier->mName) + 1);
#else
			chunk->addBytes(1);
#endif
			// return type
			char typereturn;
			if (mType)
			{
				typereturn = LSCRIPTTypeByte[mType->mType];
			}
			else
			{
				typereturn = LSCRIPTTypeByte[LST_NULL]; 
			}
			chunk->addBytes(&typereturn, 1);

			// null terminated parameters, followed by type
			if (mParameters)
			{
				LLScriptByteCodeChunk	*params = new LLScriptByteCodeChunk(FALSE);
				mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, params, heap, stacksize, entry, entrycount, NULL);
				chunk->addBytes(params->mCodeChunk, params->mCurrentOffset);
				delete params;
			}
			chunk->addBytes(1);

			// now we're at the first opcode
			offsetdelta = chunk->mCurrentOffset - offsetoffset;
			integer2bytestream(chunk->mCodeChunk, offsetoffset, offsetdelta);

			if (mStatements)
			{
				LLScriptByteCodeChunk	*statements = new LLScriptByteCodeChunk(TRUE);
				mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, statements, heap, mIdentifier->mScopeEntry->mSize, mIdentifier->mScopeEntry, entrycount, NULL);
				statements->connectJumps();
				chunk->addBytes(statements->mCodeChunk, statements->mCurrentOffset);
				delete statements;
			}
			if (mbNeedTrailingReturn)
			{
				add_return(chunk, mIdentifier->mScopeEntry);
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		{
			// Function header.
			fprintf(fp, ".method public hidebysig instance default ");
			print_cil_type(fp, mType ? mType->mType : LST_NULL);
			fprintf(fp, " ");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			if (mParameters)
			{
				fprintf(fp, "( ");
				mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				fprintf(fp, " )");
			}
			else
			{
				fprintf(fp, "()");
			}
			fprintf(fp, " cil managed\n{\n");
			fprintf(fp, ".maxstack 500\n"); // TODO: Calculated stack size...

			// Allocate space for locals.
			print_cil_local_init(fp, mIdentifier->mScopeEntry);

			if (mStatements)
			{
				mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, mIdentifier->mScopeEntry->mSize, mIdentifier->mScopeEntry, entrycount, NULL);
			}

			// Function footer.
			if (mbNeedTrailingReturn)
			{
				fprintf(fp, "ret\n");
			}
			fprintf(fp, "}\n");
			fprintf(fp, "\n");
		}
		break;
	default:
		if (mType)
		{
			mType->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mParameters)
		{
			mParameters->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		if (mStatements)
		{
			mStatements->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

void LLScriptState::addState(LLScriptState *state)
{
	if (mNextp)
	{
		state->mNextp = mNextp;
	}
	mNextp = state;
}

void LLScriptState::gonext(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	default:
		if (mNextp)
		{
			mNextp->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
}

S32 LLScriptState::getSize()
{
	return 0;
}

void LLScriptState::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		fdotabs(fp, tabs, tabsize);
		if (mType == LSSTYPE_DEFAULT)
		{
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
		}
		else
		{
			fprintf(fp, "state ");
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
			fdotabs(fp, tabs, tabsize);
			fprintf(fp, "{\n");
		}
		if (mEvent)
		{
			mEvent->recurse(fp, tabs + 1, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		fdotabs(fp, tabs, tabsize);
		fprintf(fp, "}\n");
		break;
	case LSCP_EMIT_ASSEMBLY:
		mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, ":\n");
		if (mEvent)
		{
			fprintf(fp, "EVENTS\n");
			mEvent->recurse(fp, tabs + 1, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}
		break;
	case LSCP_SCOPE_PASS1:
		// add state name
		if (scope->checkEntry(mIdentifier->mName))
		{
			gErrorToText.writeError(fp, this, LSERROR_DUPLICATE_NAME);
		}
		else
		{
			mIdentifier->mScopeEntry = scope->addEntry(mIdentifier->mName, LIT_STATE, LST_NULL);
		}
		// now do the events
		if (mEvent)
		{
			mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_SCOPE_PASS2:
		if (mEvent)
		{
			mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_TYPE:
		if (mEvent)
		{
			mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// order for states
			// set jump table value
			
			S32 jumpoffset;
			if (LSL2_CURRENT_MAJOR_VERSION == LSL2_MAJOR_VERSION_TWO)
			{
				jumpoffset = LSCRIPTDataSize[LST_INTEGER]*3*mIdentifier->mScopeEntry->mCount + LSCRIPTDataSize[LST_INTEGER];
			}
			else
			{
				jumpoffset = LSCRIPTDataSize[LST_INTEGER]*2*mIdentifier->mScopeEntry->mCount + LSCRIPTDataSize[LST_INTEGER];
			}
			integer2bytestream(chunk->mCodeChunk, jumpoffset, chunk->mCurrentOffset);

			// need to figure out what handlers this state has registered
			// we'll use to count to find it
			count = 0;

			if (mEvent)
			{
				mEvent->recurse(fp, tabs, tabsize, LSCP_DETERMINE_HANDLERS, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
				gCurrentHandler = count;
			}

			// add description word into chunk
			if (LSL2_CURRENT_MAJOR_VERSION == LSL2_MAJOR_VERSION_TWO)
			{
				u642bytestream(chunk->mCodeChunk, jumpoffset, gCurrentHandler);
			}
			else
			{
				integer2bytestream(chunk->mCodeChunk, jumpoffset, (S32)gCurrentHandler);
			}


			// 0 - 3: offset to event jump table
			S32 offsetoffset = chunk->mCurrentOffset;
			S32 offsetdelta = 0;
			chunk->addBytes(4);

			// null terminated state name
#ifdef LSL_INCLUDE_DEBUG_INFO
			chunk->addBytes(mIdentifier->mName, strlen(mIdentifier->mName) + 1);
#else
			chunk->addBytes(1);
#endif
			// now we're at the jump table
			offsetdelta = chunk->mCurrentOffset - offsetoffset;
			integer2bytestream(chunk->mCodeChunk, offsetoffset, offsetdelta);

			// add the events themselves
			if (mEvent)
			{
				LLScriptByteCodeChunk	*events = new LLScriptByteCodeChunk(FALSE);
				// make space for event jump table
				events->addBytes(LSCRIPTDataSize[LST_INTEGER]*get_number_of_event_handlers(gCurrentHandler)*2);
				mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, events, heap, stacksize, entry, entrycount, NULL);
				chunk->addBytes(events->mCodeChunk, events->mCurrentOffset);
				delete events;
			}
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:
		if (mEvent)
		{
			// Entry not used at this level, so pass state scope as entry parameter, to allow event handlers to do name mangling.
			mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, mIdentifier->mScopeEntry, entrycount, NULL);
		}
		break;
	default:
		if (mType == LSSTYPE_DEFAULT)
		{
		}
		else
		{
			mIdentifier->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		if (mEvent)
		{
			mEvent->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		break;
	}
	gonext(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
}

S32 LLScriptScript::getSize()
{
	return 0;
}

LLScriptScript::LLScriptScript(LLScritpGlobalStorage *globals, 
							   LLScriptState *states) :
    LLScriptFilePosition(0, 0),
	mStates(states), mGlobalScope(NULL), mGlobals(NULL), mGlobalFunctions(NULL), mGodLike(FALSE)
{
	const char DEFAULT_BYTECODE_FILENAME[] = "lscript.lso";
	strcpy(mBytecodeDest, DEFAULT_BYTECODE_FILENAME);

	LLScriptGlobalVariable	*tvar;
	LLScriptGlobalFunctions	*tfunc;
	LLScritpGlobalStorage *temp;

	temp = globals;
	while(temp)
	{
		if (temp->mbGlobalFunction)
		{
			if (!mGlobalFunctions)
			{
				mGlobalFunctions = (LLScriptGlobalFunctions *)temp->mGlobal;
			}
			else
			{
				tfunc = mGlobalFunctions;
				while(tfunc->mNextp)
				{
					tfunc = tfunc->mNextp;
				}
				tfunc->mNextp = (LLScriptGlobalFunctions *)temp->mGlobal;
			}
		}
		else
		{
			if (!mGlobals)
			{
				mGlobals = (LLScriptGlobalVariable *)temp->mGlobal;
			}
			else
			{
				tvar = mGlobals;
				while(tvar->mNextp)
				{
					tvar = tvar->mNextp;
				}
				tvar->mNextp = (LLScriptGlobalVariable *)temp->mGlobal;
			}
		}
		temp = temp->mNextp;
	}
}

void LLScriptScript::setBytecodeDest(const char* dst_filename)
{
	strncpy(mBytecodeDest, dst_filename, MAX_STRING);
	mBytecodeDest[MAX_STRING-1] = '\0';
}

void print_cil_globals(FILE* fp, LLScriptGlobalVariable* global)
{
	fprintf(fp, ".field private ");
	print_cil_type(fp, global->mType->mType);
	fprintf(fp, " ");
	fprintf(fp, global->mIdentifier->mName);
	fprintf(fp, "\n");
	if(NULL != global->mNextp)
	{
		print_cil_globals(fp, global->mNextp);
	}
}

void LLScriptScript::recurse(FILE *fp, S32 tabs, S32 tabsize, LSCRIPTCompilePass pass, LSCRIPTPruneType ptype, BOOL &prunearg, LLScriptScope *scope, LSCRIPTType &type, LSCRIPTType basetype, U64 &count, LLScriptByteCodeChunk *chunk, LLScriptByteCodeChunk *heap, S32 stacksize, LLScriptScopeEntry *entry, S32 entrycount, LLScriptLibData **ldata)
{
	if (gErrorToText.getErrors())
	{
		return;
	}
	switch(pass)
	{
	case LSCP_PRETTY_PRINT:
		if (mGlobals)
		{
			fdotabs(fp, tabs, tabsize);
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}

		if (mGlobalFunctions)
		{
			fdotabs(fp, tabs, tabsize);
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}

		fdotabs(fp, tabs, tabsize);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_PRUNE:
		if (mGlobalFunctions)
		{
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		}
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_SCOPE_PASS1:
		{
			mGlobalScope = new LLScriptScope(gScopeStringTable);
			// zeroth, add library functions to global scope
			S32 i;
			char *arg;
			LLScriptScopeEntry *sentry;
			for (i = 0; i < gScriptLibrary.mNextNumber; i++)
			{
				// First, check to make sure this isn't a god only function, or that the viewer's agent is a god.
				if (!gScriptLibrary.mFunctions[i]->mGodOnly || mGodLike)
				{
					if (gScriptLibrary.mFunctions[i]->mReturnType)
						sentry = mGlobalScope->addEntry(gScriptLibrary.mFunctions[i]->mName, LIT_LIBRARY_FUNCTION, char2type(*gScriptLibrary.mFunctions[i]->mReturnType));
					else
						sentry = mGlobalScope->addEntry(gScriptLibrary.mFunctions[i]->mName, LIT_LIBRARY_FUNCTION, LST_NULL);
					sentry->mLibraryNumber = i;
					arg = gScriptLibrary.mFunctions[i]->mArgs;
					if (arg)
					{
						while (*arg)
						{
							sentry->mFunctionArgs.addType(char2type(*arg));
							sentry->mSize += LSCRIPTDataSize[char2type(*arg)];
							sentry->mOffset += LSCRIPTDataSize[char2type(*arg)];
							arg++;
						}
					}
				}
			}
			// first go and collect all the global variables
			if (mGlobals)
				mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mGlobalScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			// second, do the global functions
			if (mGlobalFunctions)
				mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mGlobalScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			// now do states
			mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mGlobalScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			break;
		}
	case LSCP_SCOPE_PASS2:
		// now we're checking jumps, function calls, and state transitions
		if (mGlobalFunctions)
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mGlobalScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, mGlobalScope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_TYPE:
		// first we need to check global variables
		if (mGlobals)
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		// now do global functions and states
		if (mGlobalFunctions)
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_RESOURCE:
		// first determine resource counts for globals
		count = 0;
		if (mGlobals)
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		// now do locals
		if (mGlobalFunctions)
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	case LSCP_EMIT_ASSEMBLY:

		if (mGlobals)
		{
			fprintf(fp, "GLOBALS\n");
			fdotabs(fp, tabs, tabsize);
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}

		if (mGlobalFunctions)
		{
			fprintf(fp, "GLOBAL FUNCTIONS\n");
			fdotabs(fp, tabs, tabsize);
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}

		fprintf(fp, "STATES\n");
		fdotabs(fp, tabs, tabsize);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");
		break;
	case LSCP_EMIT_BYTE_CODE:
		{
			// first, create data structure to hold the whole shebang
			LLScriptScriptCodeChunk	*code = new LLScriptScriptCodeChunk(TOP_OF_MEMORY);

			// ok, let's add the registers, all zeroes for now
			S32 i;
			S32 nooffset = 0;

			for (i = LREG_IP; i < LREG_EOF; i++)
			{
				if (i < LREG_NCE)
					code->mRegisters->addBytes(4);
				else if (LSL2_CURRENT_MAJOR_VERSION == LSL2_MAJOR_VERSION_TWO)
					code->mRegisters->addBytes(8);
			}
			// global variables
			if (mGlobals)
				mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, code->mGlobalVariables, code->mHeap, stacksize, entry, entrycount, NULL);

			// put the ending heap block onto the heap
			U8 *temp;
			S32 size = lsa_create_data_block(&temp, NULL, 0);
			code->mHeap->addBytes(temp, size);
			delete [] temp;

			// global functions
			// make space for global function jump table
			if (mGlobalFunctions)
			{
				code->mGlobalFunctions->addBytes(LSCRIPTDataSize[LST_INTEGER]*mGlobalScope->mFunctionCount + LSCRIPTDataSize[LST_INTEGER]);
				integer2bytestream(code->mGlobalFunctions->mCodeChunk, nooffset, mGlobalScope->mFunctionCount);
				mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, code->mGlobalFunctions, NULL, stacksize, entry, entrycount, NULL);
			}


			nooffset = 0;
			// states
			// make space for state jump/info table
			if (LSL2_CURRENT_MAJOR_VERSION == LSL2_MAJOR_VERSION_TWO)
			{
				code->mStates->addBytes(LSCRIPTDataSize[LST_INTEGER]*3*mGlobalScope->mStateCount + LSCRIPTDataSize[LST_INTEGER]);
			}
			else
			{
				code->mStates->addBytes(LSCRIPTDataSize[LST_INTEGER]*2*mGlobalScope->mStateCount + LSCRIPTDataSize[LST_INTEGER]);
			}
			integer2bytestream(code->mStates->mCodeChunk, nooffset, mGlobalScope->mStateCount);
			mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, code->mStates, NULL, stacksize, entry, entrycount, NULL);

			// now, put it all together and spit it out
			// we need 
			FILE *bcfp = LLFile::fopen(mBytecodeDest, "wb");
			
			code->build(fp, bcfp);
			fclose(bcfp);
		}
		break;
	case LSCP_EMIT_CIL_ASSEMBLY:

		// Output dependencies.
		fprintf(fp, ".assembly extern mscorlib {.ver 1:0:5000:0}\n");
		fprintf(fp, ".assembly extern LScriptLibrary {.ver 0:0:0:0}\n");

		// Output assembly name.
		fprintf(fp, ".assembly 'lsl' {.ver 0:0:0:0}\n");

		// Output class header.
		fprintf(fp, ".class public auto ansi beforefieldinit LSL extends [mscorlib]System.Object\n");
		fprintf(fp, "{\n");

		// Output globals as members.
		if(NULL != mGlobals)
		{
			print_cil_globals(fp, mGlobals);
		}

		// Output "runtime". Only needed to allow stand alone execution. Not needed when compiling to DLL and using embedded runtime.
		fprintf(fp, ".method public static  hidebysig default void Main ()  cil managed\n");
		fprintf(fp, "{\n");
		fprintf(fp, ".entrypoint\n");
		fprintf(fp, ".maxstack 2\n");
		fprintf(fp, ".locals init (class LSL V_0)\n");
		fprintf(fp, "newobj instance void class LSL::.ctor()\n");
		fprintf(fp, "stloc.0\n");
		fprintf(fp, "ldloc.0\n");
		fprintf(fp, "callvirt instance void class LSL::defaultstate_entry()\n");
		fprintf(fp, "ret\n");
		fprintf(fp, "}\n");

		// Output ctor header.
		fprintf(fp, ".method public hidebysig  specialname  rtspecialname instance default void .ctor ()  cil managed\n");
		fprintf(fp, "{\n");
		fprintf(fp, ".maxstack 500\n");

		// Initialise globals as members in ctor.
		if (mGlobals)
		{
			fdotabs(fp, tabs, tabsize);
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}

		// Output ctor footer.
		fprintf(fp, "ldarg.0\n");
		fprintf(fp, "call instance void valuetype [mscorlib]System.Object::.ctor()\n");
		fprintf(fp, "ret\n");
		fprintf(fp, "}\n");

		// Output functions as methods.
		if (mGlobalFunctions)
		{
			fdotabs(fp, tabs, tabsize);
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
			fprintf(fp, "\n");
		}

		// Output states as name mangled methods.
		fdotabs(fp, tabs, tabsize);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		fprintf(fp, "\n");

		// Output class footer.
		fprintf(fp, "}\n");

		break;
	default:
		if (mGlobals)
			mGlobals->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		if (mGlobalFunctions)
			mGlobalFunctions->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		mStates->recurse(fp, tabs, tabsize, pass, ptype, prunearg, scope, type, basetype, count, chunk, heap, stacksize, entry, entrycount, NULL);
		break;
	}
}
