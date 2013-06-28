/** 
 * @file lscript_typecheck.h
 * @brief typechecks script
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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
