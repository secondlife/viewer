/** 
 * @file lscript_byteconvert.h
 * @brief Shared code for compiler and assembler for LSL
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

// data shared between compiler/assembler
// used to convert data between byte stream and outside data types

#ifndef LL_LSCRIPT_BYTECONVERT_H
#define LL_LSCRIPT_BYTECONVERT_H

#include "stdtypes.h"
#include "v3math.h"
#include "llquaternion.h"
#include "lscript_byteformat.h"
#include "lluuid.h"

void reset_hp_to_safe_spot(const U8 *buffer);

// remember that LScript byte stream is BigEndian
void set_fault(const U8 *stream, LSCRIPTRunTimeFaults fault);

inline S32 bytestream2integer(const U8 *stream, S32 &offset)
{
	stream += offset;
	offset += 4;
	return (*stream<<24) | (*(stream + 1)<<16) | (*(stream + 2)<<8) | *(stream + 3);
}

inline U32 bytestream2unsigned_integer(const U8 *stream, S32 &offset)
{
	stream += offset;
	offset += 4;
	return (*stream<<24) | (*(stream + 1)<<16) | (*(stream + 2)<<8) | *(stream + 3);
}

inline U64 bytestream2u64(const U8 *stream, S32 &offset)
{
	stream += offset;
	offset += 8;
	return ((U64)(*stream)<<56)| ((U64)(*(stream + 1))<<48) | ((U64)(*(stream + 2))<<40) | ((U64)(*(stream + 3))<<32) | 
		   ((U64)(*(stream + 4))<<24) | ((U64)(*(stream + 5))<<16) | ((U64)(*(stream + 6))<<8) | (U64)(*(stream + 7));
}

inline void integer2bytestream(U8 *stream, S32 &offset, S32 integer)
{
	stream += offset;
	offset += 4;
	*(stream)	= (integer >> 24);
	*(stream + 1)	= (integer >> 16) & 0xff;
	*(stream + 2)	= (integer >> 8) & 0xff;
	*(stream + 3)	= (integer) & 0xff;
}

inline void unsigned_integer2bytestream(U8 *stream, S32 &offset, U32 integer)
{
	stream += offset;
	offset += 4;
	*(stream)	= (integer >> 24);
	*(stream + 1)	= (integer >> 16) & 0xff;
	*(stream + 2)	= (integer >> 8) & 0xff;
	*(stream + 3)	= (integer) & 0xff;
}
inline void u642bytestream(U8 *stream, S32 &offset, U64 integer)
{
	stream += offset;
	offset += 8;
	*(stream)		= (U8)(integer >> 56);
	*(stream + 1)	= (U8)((integer >> 48) & 0xff);
	*(stream + 2)	= (U8)((integer >> 40) & 0xff);
	*(stream + 3)	= (U8)((integer >> 32) & 0xff);
	*(stream + 4)	= (U8)((integer >> 24) & 0xff);
	*(stream + 5)	= (U8)((integer >> 16) & 0xff);
	*(stream + 6)	= (U8)((integer >> 8) & 0xff);
	*(stream + 7)	= (U8)((integer) & 0xff);
}

inline S16 bytestream2s16(const U8 *stream, S32 &offset)
{
	stream += offset;
	offset += 2;
	return (*stream<<8) | *(stream + 1);
}

inline void s162bytestream(U8 *stream, S32 &offset, S16 integer)
{
	stream += offset;
	offset += 2;
	*(stream)		= (integer >> 8);
	*(stream + 1)	= (integer) & 0xff;
}

inline U16 bytestream2u16(const U8 *stream, S32 &offset)
{
	stream += offset;
	offset += 2;
	return (*stream<<8) | *(stream + 1);
}

inline void u162bytestream(U8 *stream, S32 &offset, U16 integer)
{
	stream += offset;
	offset += 2;
	*(stream)		= (integer >> 8);
	*(stream + 1)	= (integer) & 0xff;
}

inline F32 bytestream2float(const U8 *stream, S32 &offset)
{
	S32 value = bytestream2integer(stream, offset);
	F32 fpvalue = *(F32 *)&value;
	if (!llfinite(fpvalue))
	{
		fpvalue = 0;
		set_fault(stream, LSRF_MATH);
	}
	return fpvalue;
}

inline void float2bytestream(U8 *stream, S32 &offset, F32 floatingpoint)
{
	S32 value = *(S32 *)&floatingpoint;
	integer2bytestream(stream, offset, value);
}

inline void bytestream_int2float(U8 *stream, S32 &offset)
{
	S32 value = bytestream2integer(stream, offset);
	offset -= 4;
	F32 fpvalue = (F32)value;
	if (!llfinite(fpvalue))
	{
		fpvalue = 0;
		set_fault(stream, LSRF_MATH);
	}
	float2bytestream(stream, offset, fpvalue);
}

// Returns true on success, return false and clip copy on buffer overflow
inline bool bytestream2char(char *buffer, const U8 *stream, S32 &offset, S32 buffsize)
{
	S32 source_len = strlen( (const char *)stream+offset );
	S32 copy_len = buffsize - 1;
	if( copy_len > source_len )
	{
		copy_len = source_len;
	}

	// strncpy without \0 padding overhead
	memcpy( buffer, stream+offset, copy_len );
	buffer[copy_len] = 0;

	offset += source_len + 1; // advance past source string, include terminating '\0'

	return source_len < buffsize;
}

inline void char2bytestream(U8 *stream, S32 &offset, const char *buffer)
{
	while ((*(stream + offset++) = *buffer++))
		;
}

inline U8 bytestream2byte(const U8 *stream, S32 &offset)
{
	return *(stream + offset++);
}

inline void byte2bytestream(U8 *stream, S32 &offset, U8 byte)
{
	*(stream + offset++) = byte;
}

inline void bytestream2bytestream(U8 *dest, S32 &dest_offset, const U8 *src, S32 &src_offset, S32 count)
{
	while (count)
	{
		(*(dest + dest_offset++)) = (*(src + src_offset++));
		count--;
	}
}

inline void uuid2bytestream(U8 *stream, S32 &offset, const LLUUID &uuid)
{
	S32 i;
	for (i = 0; i < UUID_BYTES; i++)
	{
		*(stream + offset++) = uuid.mData[i];
	}
}

inline void bytestream2uuid(U8 *stream, S32 &offset, LLUUID &uuid)
{
	S32 i;
	for (i = 0; i < UUID_BYTES; i++)
	{
		uuid.mData[i] = *(stream + offset++);
	}
}

// vectors and quaternions and encoded in backwards order to match the way in which they are stored on the stack
inline void bytestream2vector(LLVector3 &vector, const U8 *stream, S32 &offset)
{
	S32 value = bytestream2integer(stream, offset);
	vector.mV[VZ] = *(F32 *)&value;
	if (!llfinite(vector.mV[VZ]))
	{
		vector.mV[VZ] = 0;
		set_fault(stream, LSRF_MATH);
	}
	value = bytestream2integer(stream, offset);
	vector.mV[VY] = *(F32 *)&value;
	if (!llfinite(vector.mV[VY]))
	{
		vector.mV[VY] = 0;
		set_fault(stream, LSRF_MATH);
	}
	value = bytestream2integer(stream, offset);
	vector.mV[VX] = *(F32 *)&value;
	if (!llfinite(vector.mV[VX]))
	{
		vector.mV[VX] = 0;
		set_fault(stream, LSRF_MATH);
	}
}

inline void vector2bytestream(U8 *stream, S32 &offset, LLVector3 &vector)
{
	S32 value = *(S32 *)&vector.mV[VZ];
	integer2bytestream(stream, offset, value);
	value = *(S32 *)&vector.mV[VY];
	integer2bytestream(stream, offset, value);
	value = *(S32 *)&vector.mV[VX];
	integer2bytestream(stream, offset, value);
}

inline void bytestream2quaternion(LLQuaternion &quat, const U8 *stream, S32 &offset)
{
	S32 value = bytestream2integer(stream, offset);
	quat.mQ[VS] = *(F32 *)&value;
	if (!llfinite(quat.mQ[VS]))
	{
		quat.mQ[VS] = 0;
		set_fault(stream, LSRF_MATH);
	}
	value = bytestream2integer(stream, offset);
	quat.mQ[VZ] = *(F32 *)&value;
	if (!llfinite(quat.mQ[VZ]))
	{
		quat.mQ[VZ] = 0;
		set_fault(stream, LSRF_MATH);
	}
	value = bytestream2integer(stream, offset);
	quat.mQ[VY] = *(F32 *)&value;
	if (!llfinite(quat.mQ[VY]))
	{
		quat.mQ[VY] = 0;
		set_fault(stream, LSRF_MATH);
	}
	value = bytestream2integer(stream, offset);
	quat.mQ[VX] = *(F32 *)&value;
	if (!llfinite(quat.mQ[VX]))
	{
		quat.mQ[VX] = 0;
		set_fault(stream, LSRF_MATH);
	}
}

inline void quaternion2bytestream(U8 *stream, S32 &offset, LLQuaternion &quat)
{
	S32 value = *(S32 *)&quat.mQ[VS];
	integer2bytestream(stream, offset, value);
	value = *(S32 *)&quat.mQ[VZ];
	integer2bytestream(stream, offset, value);
	value = *(S32 *)&quat.mQ[VY];
	integer2bytestream(stream, offset, value);
	value = *(S32 *)&quat.mQ[VX];
	integer2bytestream(stream, offset, value);
}

inline S32 get_register(const U8 *stream, LSCRIPTRegisters reg)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	return bytestream2integer(stream, offset);
}

inline F32 get_register_fp(U8 *stream, LSCRIPTRegisters reg)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	F32 value = bytestream2float(stream, offset);
	if (!llfinite(value))
	{
		value = 0;
		set_fault(stream, LSRF_MATH);
	}
	return value;
}
inline U64 get_register_u64(U8 *stream, LSCRIPTRegisters reg)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	return bytestream2u64(stream, offset);
}

inline U64 get_event_register(U8 *stream, LSCRIPTRegisters reg, S32 major_version)
{
	if (major_version == 1)
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg];
		return (U64)bytestream2integer(stream, offset);
	}
	else if (major_version == 2)
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg + (LREG_NCE - LREG_CE)];
		return bytestream2u64(stream, offset);
	}
	else
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg];
		return (U64)bytestream2integer(stream, offset);
	}
}

inline void set_register(U8 *stream, LSCRIPTRegisters reg, S32 value)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	integer2bytestream(stream, offset, value);
}

inline void set_register_fp(U8 *stream, LSCRIPTRegisters reg, F32 value)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	float2bytestream(stream, offset, value);
}

inline void set_register_u64(U8 *stream, LSCRIPTRegisters reg, U64 value)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	u642bytestream(stream, offset, value);
}

inline void set_event_register(U8 *stream, LSCRIPTRegisters reg, U64 value, S32 major_version)
{
	if (major_version == 1)
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg];
		integer2bytestream(stream, offset, (S32)value);
	}
	else if (major_version == 2)
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg + (LREG_NCE - LREG_CE)];
		u642bytestream(stream, offset, value);
	}
	else
	{
		S32 offset = gLSCRIPTRegisterAddresses[reg];
		integer2bytestream(stream, offset, (S32)value);
	}
}


inline F32 add_register_fp(U8 *stream, LSCRIPTRegisters reg, F32 value)
{
	S32 offset = gLSCRIPTRegisterAddresses[reg];
	F32 newvalue = bytestream2float(stream, offset);
	newvalue += value;
	if (!llfinite(newvalue))
	{
		newvalue = 0;
		set_fault(stream, LSRF_MATH);
	}
	offset = gLSCRIPTRegisterAddresses[reg];
	float2bytestream(stream, offset, newvalue);
	return newvalue;
}

void lsa_print_heap(U8 *buffer);


inline void set_fault(const U8 *stream, LSCRIPTRunTimeFaults fault)
{
   S32 fr = get_register(stream, LREG_FR);
   // record the first error
   if (!fr)
   {
	   if (  (fault == LSRF_HEAP_ERROR)
		   ||(fault == LSRF_STACK_HEAP_COLLISION)
		   ||(fault == LSRF_BOUND_CHECK_ERROR))
	   {
			reset_hp_to_safe_spot(stream);
//		    lsa_print_heap((U8 *)stream);
	   }
       fr = LSCRIPTRunTimeFaultBits[fault];
       set_register((U8 *)stream, LREG_FR, fr);
   }
}

inline BOOL set_ip(U8 *stream, S32 ip)
{
	// Verify that the Instruction Pointer is in a valid
	// code area (between the Global Function Register
	// and Heap Register).
	S32 gfr = get_register(stream, LREG_GFR);
	if (ip == 0)
	{
		set_register(stream, LREG_IP, ip);
		return TRUE;
	}
	if (ip < gfr)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	S32 hr = get_register(stream, LREG_HR);
	if (ip >= hr)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	set_register(stream, LREG_IP, ip);
	return TRUE;
}

inline BOOL set_bp(U8 *stream, S32 bp)
{
	// Verify that the Base Pointer is in a valid
	// data area (between the Heap Pointer and
	// the Top of Memory, and below the
	// Stack Pointer).
	S32 hp = get_register(stream, LREG_HP);
	if (bp <= hp)
	{
		set_fault(stream, LSRF_STACK_HEAP_COLLISION);
		return FALSE;
	}
	S32 tm = get_register(stream, LREG_TM);
	if (bp >= tm)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	S32 sp = get_register(stream, LREG_SP);
	if (bp < sp)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	set_register(stream, LREG_BP, bp);
	return TRUE;
}

inline BOOL set_sp(U8 *stream, S32 sp)
{
	// Verify that the Stack Pointer is in a valid
	// data area (between the Heap Pointer and
	// the Top of Memory).
	S32 hp = get_register(stream, LREG_HP);
	if (sp <= hp)
	{
		set_fault(stream, LSRF_STACK_HEAP_COLLISION);
		return FALSE;
	}
	S32 tm = get_register(stream, LREG_TM);
	if (sp >= tm)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	set_register(stream, LREG_SP, sp);
	return TRUE;
}

inline void lscript_push(U8 *stream, U8 value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= 1;

	if (set_sp(stream, sp))
	{
		*(stream + sp) = value;
	}
}

inline void lscript_push(U8 *stream, S32 value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= LSCRIPTDataSize[LST_INTEGER];

	if (set_sp(stream, sp))
	{
		integer2bytestream(stream, sp, value);
	}
}

inline void lscript_push(U8 *stream, F32 value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= LSCRIPTDataSize[LST_FLOATINGPOINT];

	if (set_sp(stream, sp))
	{
		float2bytestream(stream, sp, value);
	}
}

inline void lscript_push(U8 *stream, LLVector3 &value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= LSCRIPTDataSize[LST_VECTOR];

	if (set_sp(stream, sp))
	{
		vector2bytestream(stream, sp, value);
	}
}

inline void lscript_push(U8 *stream, LLQuaternion &value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= LSCRIPTDataSize[LST_QUATERNION];

	if (set_sp(stream, sp))
	{
		quaternion2bytestream(stream, sp, value);
	}
}

inline void lscript_pusharg(U8 *stream, S32 arg)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= arg;

	set_sp(stream, sp);
}

inline void lscript_poparg(U8 *stream, S32 arg)
{
	S32 sp = get_register(stream, LREG_SP);
	sp += arg;

	set_sp(stream, sp);
}

inline U8 lscript_pop_char(U8 *stream)
{
	S32 sp = get_register(stream, LREG_SP);
	U8 value = *(stream + sp++);
	set_sp(stream, sp);
	return value;
}

inline S32 lscript_pop_int(U8 *stream)
{
	S32 sp = get_register(stream, LREG_SP);
	S32 value = bytestream2integer(stream, sp);
	set_sp(stream, sp);
	return value;
}

inline F32 lscript_pop_float(U8 *stream)
{
	S32 sp = get_register(stream, LREG_SP);
	F32 value = bytestream2float(stream, sp);
	if (!llfinite(value))
	{
		value = 0;
		set_fault(stream, LSRF_MATH);
	}
	set_sp(stream, sp);
	return value;
}

inline void lscript_pop_vector(U8 *stream, LLVector3 &value)
{
	S32 sp = get_register(stream, LREG_SP);
	bytestream2vector(value, stream, sp);
	set_sp(stream, sp);
}

inline void lscript_pop_quaternion(U8 *stream, LLQuaternion &value)
{
	S32 sp = get_register(stream, LREG_SP);
	bytestream2quaternion(value, stream, sp);
	set_sp(stream, sp);
}

inline void lscript_pusharge(U8 *stream, S32 value)
{
	S32 sp = get_register(stream, LREG_SP);
	sp -= value;
	if (set_sp(stream, sp))
	{
		S32 i;
		for (i = 0; i < value; i++)
		{
			*(stream + sp++) = 0;
		}
	}
}

inline BOOL lscript_check_local(U8 *stream, S32 &address, S32 size)
{
	S32 sp = get_register(stream, LREG_SP);
	S32 bp = get_register(stream, LREG_BP);

	address += size;
	address = bp - address;

	if (address < sp - size)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	S32 tm = get_register(stream, LREG_TM);
	if (address + size > tm)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	return TRUE;
}

inline BOOL lscript_check_global(U8 *stream, S32 &address, S32 size)
{
	S32 gvr = get_register(stream, LREG_GVR);

	// Possibility of overwriting registers?  -- DK 09/07/04
	if (address < 0)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}

	address += gvr;
	S32 gfr = get_register(stream, LREG_GFR);

	if (address + size > gfr)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	return TRUE;
}

inline void lscript_local_store(U8 *stream, S32 address, S32 value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_INTEGER]))
		integer2bytestream(stream, address, value);
}

inline void lscript_local_store(U8 *stream, S32 address, F32 value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_FLOATINGPOINT]))
		float2bytestream(stream, address, value);
}

inline void lscript_local_store(U8 *stream, S32 address, LLVector3 value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_VECTOR]))
		vector2bytestream(stream, address, value);
}

inline void lscript_local_store(U8 *stream, S32 address, LLQuaternion value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_QUATERNION]))
		quaternion2bytestream(stream, address, value);
}

inline void lscript_global_store(U8 *stream, S32 address, S32 value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_INTEGER]))
		integer2bytestream(stream, address, value);
}

inline void lscript_global_store(U8 *stream, S32 address, F32 value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_FLOATINGPOINT]))
		float2bytestream(stream, address, value);
}

inline void lscript_global_store(U8 *stream, S32 address, LLVector3 value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_VECTOR]))
		vector2bytestream(stream, address, value);
}

inline void lscript_global_store(U8 *stream, S32 address, LLQuaternion value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_QUATERNION]))
		quaternion2bytestream(stream, address, value);
}

inline S32 lscript_local_get(U8 *stream, S32 address)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_INTEGER]))
		return bytestream2integer(stream, address);
	return 0;
}

inline void lscript_local_get(U8 *stream, S32 address, F32 &value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_FLOATINGPOINT]))
		value = bytestream2float(stream, address);
	if (!llfinite(value))
	{
		value = 0;
		set_fault(stream, LSRF_MATH);
	}
}

inline void lscript_local_get(U8 *stream, S32 address, LLVector3 &value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_VECTOR]))
		bytestream2vector(value, stream, address);
}

inline void lscript_local_get(U8 *stream, S32 address, LLQuaternion &value)
{
	if (lscript_check_local(stream, address, LSCRIPTDataSize[LST_QUATERNION]))
		bytestream2quaternion(value, stream, address);
}

inline S32 lscript_global_get(U8 *stream, S32 address)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_INTEGER]))
		return bytestream2integer(stream, address);
	return 0;
}

inline void lscript_global_get(U8 *stream, S32 address, F32 &value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_FLOATINGPOINT]))
		value = bytestream2float(stream, address);
	if (!llfinite(value))
	{
		value = 0;
		set_fault(stream, LSRF_MATH);
	}
}

inline void lscript_global_get(U8 *stream, S32 address, LLVector3 &value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_VECTOR]))
		bytestream2vector(value, stream, address);
}

inline void lscript_global_get(U8 *stream, S32 address, LLQuaternion &value)
{
	if (lscript_check_global(stream, address, LSCRIPTDataSize[LST_QUATERNION]))
		bytestream2quaternion(value, stream, address);
}



inline S32 get_state_event_opcoode_start(U8 *stream, S32 state, LSCRIPTStateEventType event)
{
	// get the start of the state table
	S32 sr = get_register(stream, LREG_SR);

	// get the position of the jump to the desired state
	S32 value = get_register(stream, LREG_VN);

	S32 state_offset_offset = 0;
	S32 major_version = 0;
	if (value == LSL2_VERSION1_END_NUMBER)
	{
		major_version = LSL2_MAJOR_VERSION_ONE;
		state_offset_offset = sr + LSCRIPTDataSize[LST_INTEGER] + LSCRIPTDataSize[LST_INTEGER]*2*state;
	}
	else if (value == LSL2_VERSION_NUMBER)
	{
		major_version = LSL2_MAJOR_VERSION_TWO;
		state_offset_offset = sr + LSCRIPTDataSize[LST_INTEGER] + LSCRIPTDataSize[LST_INTEGER]*3*state;
	}
	if ( state_offset_offset < 0 || state_offset_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	// get the actual position in memory of the desired state
	S32 state_offset = sr + bytestream2integer(stream, state_offset_offset);
	if ( state_offset < 0 || state_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	// save that value
	S32 state_offset_base = state_offset;

	// jump past the state name
	S32 event_jump_offset = state_offset_base + bytestream2integer(stream, state_offset);

	// get the location of the event offset
	S32 event_offset = event_jump_offset + LSCRIPTDataSize[LST_INTEGER]*2*get_event_handler_jump_position(get_event_register(stream, LREG_ER, major_version), event);
	if ( event_offset < 0 || event_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	// now, jump to the event
	S32 event_start = bytestream2integer(stream, event_offset);
	if ( event_start < 0 || event_start > TOP_OF_MEMORY )
	{
		return -1;
	}
	event_start += event_jump_offset;

	S32 event_start_original = event_start;

	// now skip past the parameters
	S32 opcode_offset = bytestream2integer(stream, event_start);
	if ( opcode_offset < 0 || opcode_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	return opcode_offset + event_start_original;
}


inline U64 get_handled_events(U8 *stream, S32 state)
{
	U64 retvalue = 0;
	// get the start of the state table
	S32 sr = get_register(stream, LREG_SR);

	// get the position of the jump to the desired state
	S32 value = get_register(stream, LREG_VN);
	S32 state_handled_offset = 0;
	if (value == LSL2_VERSION1_END_NUMBER)
	{
		state_handled_offset = sr + LSCRIPTDataSize[LST_INTEGER]*2*state + 2*LSCRIPTDataSize[LST_INTEGER];	
		retvalue = bytestream2integer(stream, state_handled_offset);
	}
	else if (value == LSL2_VERSION_NUMBER)
	{
		state_handled_offset = sr + LSCRIPTDataSize[LST_INTEGER]*3*state + 2*LSCRIPTDataSize[LST_INTEGER];	
		retvalue = bytestream2u64(stream, state_handled_offset);
	}

	// get the handled events
	return retvalue;
}

// Returns -1 on error
inline S32 get_event_stack_size(U8 *stream, S32 state, LSCRIPTStateEventType event)
{
	// get the start of the state table
	S32 sr = get_register(stream, LREG_SR);

	// get state offset
	S32 value = get_register(stream, LREG_VN);
	S32 state_offset_offset = 0;
	S32 major_version = 0;
	if (value == LSL2_VERSION1_END_NUMBER)
	{
		major_version = LSL2_MAJOR_VERSION_ONE;
		state_offset_offset = sr + LSCRIPTDataSize[LST_INTEGER] + LSCRIPTDataSize[LST_INTEGER]*2*state;
	}
	else if (value == LSL2_VERSION_NUMBER)
	{
		major_version = LSL2_MAJOR_VERSION_TWO;
		state_offset_offset = sr + LSCRIPTDataSize[LST_INTEGER] + LSCRIPTDataSize[LST_INTEGER]*3*state;
	}

	if ( state_offset_offset < 0 || state_offset_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	S32 state_offset = bytestream2integer(stream, state_offset_offset);
	state_offset += sr;

	state_offset_offset = state_offset;
	if ( state_offset_offset < 0 || state_offset_offset > TOP_OF_MEMORY )
	{
		return -1;
	}

	// skip to jump table
	S32 jump_table = bytestream2integer(stream, state_offset_offset);

	jump_table += state_offset;
	if ( jump_table < 0 || jump_table > TOP_OF_MEMORY )
	{
		return -1;
	}

	// get the position of the jump to the desired state
	S32 stack_size_offset = jump_table + LSCRIPTDataSize[LST_INTEGER]*2*get_event_handler_jump_position(get_event_register(stream, LREG_ER, major_version), event) + LSCRIPTDataSize[LST_INTEGER];

	// get the handled events
	S32 stack_size = bytestream2integer(stream, stack_size_offset);
	if ( stack_size < 0 || stack_size > TOP_OF_MEMORY )
	{
		return -1;
	}

	return stack_size;
}

inline LSCRIPTStateEventType return_first_event(S32 event)
{
	S32 count = 1;
	while (count < LSTT_EOF)
	{
		if (event & 0x1)
		{
			return (LSCRIPTStateEventType) count;
		}
		else
		{
			event >>= 1;
			count++;
		}
	}
	return LSTT_NULL;
}


// the safe instruction versions of these commands will only work if offset is between
// GFR and HR, meaning that it is an instruction (more or less) in global functions or event handlers

inline BOOL safe_instruction_check_address(U8 *stream, S32 offset, S32 size)
{
	S32 gfr = get_register(stream, LREG_GFR);
	if (offset < gfr)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	else
	{
		S32 hr = get_register(stream, LREG_HR);
		if (offset + size > hr)
		{
			set_fault(stream, LSRF_BOUND_CHECK_ERROR);
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
}

inline BOOL safe_heap_check_address(U8 *stream, S32 offset, S32 size)
{
	S32 hr = get_register(stream, LREG_HR);
	if (offset < hr)
	{
		set_fault(stream, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	else
	{
		S32 hp = get_register(stream, LREG_HP);
		if (offset + size > hp)
		{
			set_fault(stream, LSRF_BOUND_CHECK_ERROR);
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
}

inline U8 safe_instruction_bytestream2byte(U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, 1))
	{
		return *(stream + offset++);
	}
	else
	{
		return 0;
	}
}

inline void safe_instruction_byte2bytestream(U8 *stream, S32 &offset, U8 byte)
{
	if (safe_instruction_check_address(stream, offset, 1))
	{
		*(stream + offset++) = byte;
	}
}

inline S32 safe_instruction_bytestream2integer(U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_INTEGER]))
	{
		return (bytestream2integer(stream, offset));
	}
	else
	{
		return 0;
	}
}

inline void safe_instruction_integer2bytestream(U8 *stream, S32 &offset, S32 value)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_INTEGER]))
	{
		integer2bytestream(stream, offset, value);
	}
}

inline U16 safe_instruction_bytestream2u16(U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, 2))
	{
		return (bytestream2u16(stream, offset));
	}
	else
	{
		return 0;
	}
}

inline void safe_instruction_u162bytestream(U8 *stream, S32 &offset, U16 value)
{
	if (safe_instruction_check_address(stream, offset, 2))
	{
		u162bytestream(stream, offset, value);
	}
}

inline F32 safe_instruction_bytestream2float(U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_INTEGER]))
	{
		F32 value = bytestream2float(stream, offset);
		if (!llfinite(value))
		{
			value = 0;
			set_fault(stream, LSRF_MATH);
		}
		return value;
	}
	else
	{
		return 0;
	}
}

inline void safe_instruction_float2bytestream(U8 *stream, S32 &offset, F32 value)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_FLOATINGPOINT]))
	{
		float2bytestream(stream, offset, value);
	}
}

inline void safe_instruction_bytestream2char(char *buffer, U8 *stream, S32 &offset, S32 buffsize)
{
	// This varies from the old method. Previously, we would copy up until we got an error,
	// then halt the script via safe_isntruction_check_address. Now we don't bother
	// copying a thing if there's an error.

	if( safe_instruction_check_address(stream, offset, strlen( (const char *)stream + offset ) + 1 ) )
	{
		// Takes the same parms as this function. Won't overread, per above check.
		bytestream2char( buffer, stream, offset, buffsize );
	}
	else
	{
		// Truncate - no point in copying
		*buffer = 0;
	}
}

inline void safe_instruction_bytestream_count_char(U8 *stream, S32 &offset)
{
	while (  (safe_instruction_check_address(stream, offset, 1))
		   &&(*(stream + offset++)))
		;
}

inline void safe_heap_bytestream_count_char(U8 *stream, S32 &offset)
{
	while (  (safe_heap_check_address(stream, offset, 1))
		   &&(*(stream + offset++)))
		;
}

inline void safe_instruction_char2bytestream(U8 *stream, S32 &offset, const char* buffer)
{
	while (  (safe_instruction_check_address(stream, offset, 1))
		   &&(*(stream + offset++) = *buffer++))
		;
}

inline void safe_instruction_bytestream2vector(LLVector3 &value, U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_VECTOR]))
	{
		bytestream2vector(value, stream, offset);
	}
}

inline void safe_instruction_vector2bytestream(U8 *stream, S32 &offset, LLVector3 &value)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_VECTOR]))
	{
		vector2bytestream(stream, offset, value);
	}
}

inline void safe_instruction_bytestream2quaternion(LLQuaternion &value, U8 *stream, S32 &offset)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_QUATERNION]))
	{
		bytestream2quaternion(value, stream, offset);
	}
}

inline void safe_instruction_quaternion2bytestream(U8 *stream, S32 &offset, LLQuaternion &value)
{
	if (safe_instruction_check_address(stream, offset, LSCRIPTDataSize[LST_QUATERNION]))
	{
		quaternion2bytestream(stream, offset, value);
	}
}

static inline LSCRIPTType char2type(char type)
{
		switch(type)
		{
		case 'i':
			return LST_INTEGER;
		case 'f':
			return LST_FLOATINGPOINT;
		case 's':
			return LST_STRING;
		case 'k':
			return LST_KEY;
		case 'v':
			return LST_VECTOR;
		case 'q':
			return LST_QUATERNION;
		case 'l':
			return LST_LIST;
		default:
			return LST_NULL;
		}
}

#endif
