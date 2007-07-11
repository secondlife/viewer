/** 
 * @file llmsgvariabletype.h
 * @brief Declaration of the EMsgVariableType enumeration.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLMSGVARIABLETYPE_H
#define LL_LLMSGVARIABLETYPE_H

typedef enum e_message_variable_type
{
	MVT_NULL,
	MVT_FIXED,
	MVT_VARIABLE,
	MVT_U8,
	MVT_U16,
	MVT_U32,
	MVT_U64,
	MVT_S8,
	MVT_S16,
	MVT_S32,
	MVT_S64,
	MVT_F32,
	MVT_F64,
	MVT_LLVector3,
	MVT_LLVector3d,
	MVT_LLVector4,
	MVT_LLQuaternion,
	MVT_LLUUID,	
	MVT_BOOL,
	MVT_IP_ADDR,
	MVT_IP_PORT,
	MVT_U16Vec3,
	MVT_U16Quat,
	MVT_S16Array,
	MVT_EOL
} EMsgVariableType;

#endif // LL_LLMSGVARIABLETYPE_H
