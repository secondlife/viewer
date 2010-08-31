/** 
 * @file lscript_typecheck.h
 * @brief typechecks script
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

#ifndef LL_LSCRIPT_TYPECHECK_H
#define LL_LSCRIPT_TYPECHECK_H

#include "lscript_error.h"

LSCRIPTType implicit_casts(LSCRIPTType left_side, LSCRIPTType right_side);
BOOL legal_casts(LSCRIPTType cast, LSCRIPTType base);
LSCRIPTType promote(LSCRIPTType left_side, LSCRIPTType right_side);
BOOL legal_assignment(LSCRIPTType left_side, LSCRIPTType right_side);

typedef enum e_lscript_expression_types
{
	LET_NULL,
	LET_ASSIGNMENT,
	LET_ADD_ASSIGN,
	LET_SUB_ASSIGN,
	LET_MUL_ASSIGN,
	LET_DIV_ASSIGN,
	LET_MOD_ASSIGN,
	LET_EQUALITY,
	LET_NOT_EQUALS,
	LET_LESS_EQUALS,
	LET_GREATER_EQUALS,
	LET_LESS_THAN,
	LET_GREATER_THAN,
	LET_PLUS,
	LET_MINUS,
	LET_TIMES,
	LET_DIVIDE,
	LET_MOD,
	LET_BIT_AND,
	LET_BIT_OR,
	LET_BIT_XOR,
	LET_BOOLEAN_AND,
	LET_BOOLEAN_OR,
	LET_PARENTHESIS,
	LET_UNARY_MINUS,
	LET_BOOLEAN_NOT,
	LET_BIT_NOT,
	LET_PRE_INCREMENT,
	LET_PRE_DECREMENT,
	LET_CAST,
	LET_VECTOR_INITIALIZER,
	LET_QUATERNION_INITIALIZER,
	LET_LIST_INITIALIZER,
	LET_LVALUE,
	LET_POST_INCREMENT,
	LET_POST_DECREMENT,
	LET_FUNCTION_CALL,
	LET_CONSTANT,
	LET_FOR_EXPRESSION_LIST,
	LET_FUNC_EXPRESSION_LIST,
	LET_LIST_EXPRESSION_LIST,
	LET_PRINT,
	LET_SHIFT_LEFT,
	LET_SHIFT_RIGHT,
	LET_EOF
} LSCRIPTExpressionType;

BOOL legal_binary_expression(LSCRIPTType &result, LSCRIPTType left_side, LSCRIPTType right_side, LSCRIPTExpressionType expression);
BOOL legal_unary_expression(LSCRIPTType &result, LSCRIPTType left_side, LSCRIPTExpressionType expression);

void init_supported_expressions(void);

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

#endif
