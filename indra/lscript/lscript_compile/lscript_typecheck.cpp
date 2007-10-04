/** 
 * @file lscript_typecheck.cpp
 * @brief typechecks script
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "lscript_tree.h"

/*
  LScript automatic type casting

  LST_INTEGER			-> LST_INTEGER

  LST_FLOATINGPOINT		-> LST_FLOATINGPOINT
  LST_INTEGER			-> LST_FLOATINGPOINT

  LST_FLOATINGPOINT		-> LST_STRING
  LST_INTEGER			-> LST_STRING
  LST_STRING			-> LST_STRING
  LST_VECTOR			-> LST_STRING
  LST_QUATERNION		-> LST_STRING
  LST_LIST				-> LST_STRING

  LST_VECTOR			-> LST_VECTOR
  
  LST_QUATERNION		-> LST_QUATERNION
  
  LST_FLOATINGPOINT		-> LST_LIST
  LST_INTEGER			-> LST_LIST
  LST_STRING			-> LST_LIST
  LST_VECTOR			-> LST_LIST
  LST_QUATERNION		-> LST_LIST
  LST_LIST				-> LST_LIST
*/

LSCRIPTType implicit_casts(LSCRIPTType left_side, LSCRIPTType right_side)
{
	switch(left_side)
	{
		// shouldn't be doing an operation on void types
	case LST_NULL:
		return LST_NULL;
	// shouldn't be doing an operation on undefined types
	case LST_UNDEFINED:
		return LST_UNDEFINED;
	// only integers can become integers
	case LST_INTEGER:
		switch(right_side)
		{
		case LST_INTEGER:
			return LST_INTEGER;
		default:
			return LST_UNDEFINED;
		}
	// only integers and floats can become floats
	case LST_FLOATINGPOINT:
		switch(right_side)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
			return LST_FLOATINGPOINT;
		default:
			return LST_UNDEFINED;
		}
	// only strings and keys can become strings
	case LST_STRING:
		switch(right_side)
		{
		case LST_STRING:
		case LST_KEY:
			return LST_STRING;
		default:
			return LST_UNDEFINED;
		}
	// only strings and keys can become keys
	case LST_KEY:
		switch(right_side)
		{
		case LST_STRING:
		case LST_KEY:
			return LST_KEY;
		default:
			return LST_UNDEFINED;
		}
	// only vectors can become vectors
	case LST_VECTOR:
		switch(right_side)
		{
		case LST_VECTOR:
			return LST_VECTOR;
		default:
			return LST_UNDEFINED;
		}
	// only quaternions can become quaternions
	case LST_QUATERNION:
		switch(right_side)
		{
		case LST_QUATERNION:
			return LST_QUATERNION;
		default:
			return LST_UNDEFINED;
		}
	// only lists can become lists
	case LST_LIST:
		switch(right_side)
		{
		case LST_LIST:
			return LST_LIST;
		default:
			return LST_UNDEFINED;
		}
	default:
		return LST_UNDEFINED;
	}
}

LSCRIPTType promote(LSCRIPTType left_side, LSCRIPTType right_side)
{
	LSCRIPTType type;
	type = implicit_casts(left_side, right_side);
	if (type != LST_UNDEFINED)
	{
		return type;
	}
	type = implicit_casts(right_side, left_side);
	if (type != LST_UNDEFINED)
	{
		return type;
	}
	return LST_UNDEFINED;
}

BOOL legal_assignment(LSCRIPTType left_side, LSCRIPTType right_side)
{
	// this is to prevent cascading errors
	if (  (left_side == LST_UNDEFINED)
		||(right_side == LST_UNDEFINED))
	{
		return TRUE;
	}

	if (implicit_casts(left_side, right_side) != LST_UNDEFINED)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL legal_casts(LSCRIPTType cast, LSCRIPTType base)
{
	switch(base)
	{
	// shouldn't be doing an operation on void types
	case LST_NULL:
		return FALSE;
	// shouldn't be doing an operation on undefined types
	case LST_UNDEFINED:
		return FALSE;
	case LST_INTEGER:
		switch(cast)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
		case LST_STRING:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	case LST_FLOATINGPOINT:
		switch(cast)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
		case LST_STRING:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	case LST_STRING:
		switch(cast)
		{
		case LST_INTEGER:
		case LST_FLOATINGPOINT:
		case LST_STRING:
		case LST_KEY:
		case LST_VECTOR:
		case LST_QUATERNION:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	case LST_KEY:
		switch(cast)
		{
		case LST_STRING:
		case LST_KEY:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	case LST_VECTOR:
		switch(cast)
		{
		case LST_VECTOR:
		case LST_STRING:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	case LST_QUATERNION:
		switch(cast)
		{
		case LST_QUATERNION:
		case LST_STRING:
		case LST_LIST:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	// lists can only be cast to lists and strings
	case LST_LIST:
		switch(cast)
		{
		case LST_LIST:
		case LST_STRING:
			return TRUE;
			break;
		default:
			return FALSE;
			break;
		}
		break;
	default:
		return FALSE;
		break;
	}
}

LSCRIPTType gSupportedExpressionArray[LET_EOF][LST_EOF][LST_EOF];

void init_supported_expressions(void)
{
	S32 i, j, k;
	// zero out, then set the ones that matter
	for (i = 0; i < LET_EOF; i++)
	{
		for (j = 0; j < LST_EOF; j++)
		{
			for (k = 0; k < LST_EOF; k++)
			{
				gSupportedExpressionArray[i][j][k] = LST_NULL;
			}
		}
	}

	// LET_ASSIGNMENT
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_STRING][LST_STRING] = LST_STRING;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_KEY][LST_KEY] = LST_KEY;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_VECTOR][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_INTEGER] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_FLOATINGPOINT] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_STRING] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_KEY] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_VECTOR] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_QUATERNION] = LST_LIST;
	gSupportedExpressionArray[LET_ASSIGNMENT][LST_LIST][LST_LIST] = LST_LIST;

	// LET_ADD_ASSIGN
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_STRING][LST_STRING] = LST_STRING;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_VECTOR][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_INTEGER] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_FLOATINGPOINT] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_STRING] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_KEY] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_VECTOR] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_QUATERNION] = LST_LIST;
	gSupportedExpressionArray[LET_ADD_ASSIGN][LST_LIST][LST_LIST] = LST_LIST;

	// LET_SUB_ASSIGN
	gSupportedExpressionArray[LET_SUB_ASSIGN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_SUB_ASSIGN][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_SUB_ASSIGN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_SUB_ASSIGN][LST_VECTOR][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_SUB_ASSIGN][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_MUL_ASSIGN
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_VECTOR][LST_INTEGER] = LST_VECTOR;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_INTEGER][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_VECTOR][LST_FLOATINGPOINT] = LST_VECTOR;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_FLOATINGPOINT][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_VECTOR][LST_VECTOR] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_VECTOR][LST_QUATERNION] = LST_VECTOR;
	gSupportedExpressionArray[LET_MUL_ASSIGN][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_DIV_ASSIGN
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_VECTOR][LST_INTEGER] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_VECTOR][LST_FLOATINGPOINT] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_VECTOR][LST_QUATERNION] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIV_ASSIGN][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_MOD_ASSIGN
	gSupportedExpressionArray[LET_MOD_ASSIGN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_MOD_ASSIGN][LST_VECTOR][LST_VECTOR] = LST_VECTOR;

	// LET_EQUALITY
	gSupportedExpressionArray[LET_EQUALITY][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_STRING][LST_STRING] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_STRING][LST_KEY] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_KEY][LST_STRING] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_KEY][LST_KEY] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_VECTOR][LST_VECTOR] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_QUATERNION][LST_QUATERNION] = LST_INTEGER;
	gSupportedExpressionArray[LET_EQUALITY][LST_LIST][LST_LIST] = LST_INTEGER;

	// LET_NOT_EQUALS
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_STRING][LST_STRING] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_STRING][LST_KEY] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_KEY][LST_STRING] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_KEY][LST_KEY] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_VECTOR][LST_VECTOR] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_QUATERNION][LST_QUATERNION] = LST_INTEGER;
	gSupportedExpressionArray[LET_NOT_EQUALS][LST_LIST][LST_LIST] = LST_INTEGER;

	// LET_LESS_EQUALS
	gSupportedExpressionArray[LET_LESS_EQUALS][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_EQUALS][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_EQUALS][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_EQUALS][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;

	// LET_GREATER_EQUALS
	gSupportedExpressionArray[LET_GREATER_EQUALS][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_EQUALS][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_EQUALS][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_EQUALS][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;

	// LET_LESS_THAN
	gSupportedExpressionArray[LET_LESS_THAN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_THAN][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_THAN][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_LESS_THAN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;

	// LET_GREATER_THAN
	gSupportedExpressionArray[LET_GREATER_THAN][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_THAN][LST_INTEGER][LST_FLOATINGPOINT] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_THAN][LST_FLOATINGPOINT][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_GREATER_THAN][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_INTEGER;

	// LET_PLUS
	gSupportedExpressionArray[LET_PLUS][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_PLUS][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_PLUS][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_PLUS][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_PLUS][LST_STRING][LST_STRING] = LST_STRING;
	gSupportedExpressionArray[LET_PLUS][LST_VECTOR][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_PLUS][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_INTEGER] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_FLOATINGPOINT] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_STRING] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_KEY] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_VECTOR] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_QUATERNION] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_INTEGER][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_FLOATINGPOINT][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_STRING][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_KEY][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_VECTOR][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_QUATERNION][LST_LIST] = LST_LIST;
	gSupportedExpressionArray[LET_PLUS][LST_LIST][LST_LIST] = LST_LIST;

	// LET_MINUS
	gSupportedExpressionArray[LET_MINUS][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_MINUS][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MINUS][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MINUS][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_MINUS][LST_VECTOR][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_MINUS][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_TIMES
	gSupportedExpressionArray[LET_TIMES][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_TIMES][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_TIMES][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_TIMES][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_TIMES][LST_VECTOR][LST_INTEGER] = LST_VECTOR;
	gSupportedExpressionArray[LET_TIMES][LST_INTEGER][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_TIMES][LST_VECTOR][LST_FLOATINGPOINT] = LST_VECTOR;
	gSupportedExpressionArray[LET_TIMES][LST_FLOATINGPOINT][LST_VECTOR] = LST_VECTOR;
	gSupportedExpressionArray[LET_TIMES][LST_VECTOR][LST_VECTOR] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_TIMES][LST_VECTOR][LST_QUATERNION] = LST_VECTOR;
	gSupportedExpressionArray[LET_TIMES][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_DIVIDE
	gSupportedExpressionArray[LET_DIVIDE][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_DIVIDE][LST_INTEGER][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_DIVIDE][LST_FLOATINGPOINT][LST_INTEGER] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_DIVIDE][LST_FLOATINGPOINT][LST_FLOATINGPOINT] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_DIVIDE][LST_VECTOR][LST_INTEGER] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIVIDE][LST_VECTOR][LST_FLOATINGPOINT] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIVIDE][LST_VECTOR][LST_QUATERNION] = LST_VECTOR;
	gSupportedExpressionArray[LET_DIVIDE][LST_QUATERNION][LST_QUATERNION] = LST_QUATERNION;

	// LET_MOD
	gSupportedExpressionArray[LET_MOD][LST_INTEGER][LST_INTEGER] = LST_INTEGER;
	gSupportedExpressionArray[LET_MOD][LST_VECTOR][LST_VECTOR] = LST_VECTOR;

	// LET_BIT_AND
	gSupportedExpressionArray[LET_BIT_AND][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_BIT_OR
	gSupportedExpressionArray[LET_BIT_OR][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_BIT_XOR
	gSupportedExpressionArray[LET_BIT_XOR][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_BOOLEAN_AND
	gSupportedExpressionArray[LET_BOOLEAN_AND][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_BOOLEAN_OR
	gSupportedExpressionArray[LET_BOOLEAN_OR][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_SHIFT_LEFT
	gSupportedExpressionArray[LET_SHIFT_LEFT][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_SHIFT_RIGHT
	gSupportedExpressionArray[LET_SHIFT_RIGHT][LST_INTEGER][LST_INTEGER] = LST_INTEGER;

	// LET_PARENTHESIS
	gSupportedExpressionArray[LET_PARENTHESIS][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_PARENTHESIS][LST_FLOATINGPOINT][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_PARENTHESIS][LST_STRING][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_PARENTHESIS][LST_LIST][LST_NULL] = LST_INTEGER;

	// LET_UNARY_MINUS
	gSupportedExpressionArray[LET_UNARY_MINUS][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_UNARY_MINUS][LST_FLOATINGPOINT][LST_NULL] = LST_FLOATINGPOINT;
	gSupportedExpressionArray[LET_UNARY_MINUS][LST_VECTOR][LST_NULL] = LST_VECTOR;
	gSupportedExpressionArray[LET_UNARY_MINUS][LST_QUATERNION][LST_NULL] = LST_QUATERNION;

	// LET_BOOLEAN_NOT
	gSupportedExpressionArray[LET_BOOLEAN_NOT][LST_INTEGER][LST_NULL] = LST_INTEGER;

	// LET_BIT_NOT
	gSupportedExpressionArray[LET_BIT_NOT][LST_INTEGER][LST_NULL] = LST_INTEGER;

	// LET_PRE_INCREMENT
	gSupportedExpressionArray[LET_PRE_INCREMENT][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_PRE_INCREMENT][LST_FLOATINGPOINT][LST_NULL] = LST_FLOATINGPOINT;

	// LET_PRE_DECREMENT
	gSupportedExpressionArray[LET_PRE_DECREMENT][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_PRE_DECREMENT][LST_FLOATINGPOINT][LST_NULL] = LST_FLOATINGPOINT;

	// LET_POST_INCREMENT
	gSupportedExpressionArray[LET_POST_INCREMENT][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_POST_INCREMENT][LST_FLOATINGPOINT][LST_NULL] = LST_FLOATINGPOINT;

	// LET_POST_DECREMENT
	gSupportedExpressionArray[LET_POST_DECREMENT][LST_INTEGER][LST_NULL] = LST_INTEGER;
	gSupportedExpressionArray[LET_POST_DECREMENT][LST_FLOATINGPOINT][LST_NULL] = LST_FLOATINGPOINT;
}

BOOL legal_binary_expression(LSCRIPTType &result, LSCRIPTType left_side, LSCRIPTType right_side, LSCRIPTExpressionType expression)
{
	if (  (left_side == LST_UNDEFINED)
		||(right_side == LST_UNDEFINED))
	{
		result = LST_UNDEFINED;
		return TRUE;
	}

	if (  (left_side == LST_NULL)
		||(right_side == LST_NULL))
	{
		result = LST_UNDEFINED;
		return FALSE;
	}

	result = gSupportedExpressionArray[expression][left_side][right_side];
	if (result)
		return TRUE;
	else
	{
		result = LST_UNDEFINED;
		return FALSE;
	}
}

BOOL legal_unary_expression(LSCRIPTType &result, LSCRIPTType left_side, LSCRIPTExpressionType expression)
{
	if (left_side == LST_UNDEFINED)
	{
		result = LST_UNDEFINED;
		return TRUE;
	}

	if (left_side == LST_NULL)
	{
		result = LST_UNDEFINED;
		return FALSE;
	}

	result = gSupportedExpressionArray[expression][left_side][LST_NULL];
	if (result)
		return TRUE;
	else
	{
		result = LST_UNDEFINED;
		return FALSE;
	}
}
