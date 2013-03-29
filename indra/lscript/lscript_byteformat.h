/** 
 * @file lscript_byteformat.h
 * @brief Shared code between compiler and assembler and LSL
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

#ifndef LL_LSCRIPT_BYTEFORMAT_H
#define LL_LSCRIPT_BYTEFORMAT_H

// Data shared between compiler/assembler and lscript execution code

#include "stdtypes.h"

const S32 LSL2_VERSION_NUMBER = 0x0200;
const S32 LSL2_VERSION1_END_NUMBER = 0x0101;
const S32 LSL2_VERSION2_START_NUMBER = 0x0200;

const S32 LSL2_MAJOR_VERSION_ONE = 1;
const S32 LSL2_MAJOR_VERSION_TWO = 2;
const S32 LSL2_CURRENT_MAJOR_VERSION = LSL2_MAJOR_VERSION_TWO;

const S32 TOP_OF_MEMORY = 16384;

typedef enum e_lscript_registers
{
	LREG_INVALID,
	LREG_IP,		// instruction pointer
	LREG_VN,		// version number
	LREG_BP,		// base pointer - what local variables are referenced from
	LREG_SP,		// stack pointer - where the top of the stack is
	LREG_HR,		// heap register - where in memory does the heap start
	LREG_HP,		// heap pointer - where is the top of the heap?
	LREG_CS,		// current state - what state are we currently in?
	LREG_NS,		// next state - what state are we currently in?
	LREG_CE,		// current events - what events are waiting to be handled?
	LREG_IE,		// in event - which event handler are we currently in?
	LREG_ER,		// event register - what events do we have active handlers for?
	LREG_FR,		// fault register - which errors are currently active?
	LREG_SLR,		// sleep register - are we sleeping?
	LREG_GVR,		// global variable register - where do global variables start
	LREG_GFR,		// global function register - where do global functions start
	LREG_SR,		// state register - where do states start
	LREG_TM,		// top of memory - where is the top of memory
	LREG_PR,		// parameter register - data passed to script from launcher
	LREG_ESR,		// energy supply register - how much energy do we have on board?
	LREG_NCE,		// 64 bit current envents - what events are waiting to be handled?
	LREG_NIE,		// 64 bit in event - which event handler are we currently in?
	LREG_NER,		// 64 bit event register - what events do we have active handlers for?
	LREG_EOF
} LSCRIPTRegisters;

const S32 gLSCRIPTRegisterAddresses[LREG_EOF] =	/* Flawfinder: ignore */
{
	0,			// LREG_INVALID
	4,			// LREG_IP
	8,			// LREG_VN
	12,			// LREG_BP
	16,			// LREG_SP
	20,			// LREG_HR
	24,			// LREG_HP
	28,			// LREG_CS
	32,			// LREG_NS
	36,			// LREG_CE
	40,			// LREG_IE
	44,			// LREG_ER
	48,			// LREG_FR
	52,			// LREG_SLR
	56,			// LREG_GVR
	60,			// LREG_GFR
	72,			// LREG_SR
	0,			// LREG_TM
	64,			// LREG_PR
	68,			// LREG_ESR
	76,			// LREG_NCE
	84,			// LREG_NIE
	92,			// LREG_NER
};

const char * const gLSCRIPTRegisterNames[LREG_EOF] =
{
	"INVALID",		// LREG_INVALID
	"IP",			// LREG_IP
	"VN",			// LREG_VN
	"BP",			// LREG_BP
	"SP",			// LREG_SP
	"HR",			// LREG_HR
	"HP",			// LREG_HP
	"CS",			// LREG_CS
	"NS",			// LREG_NS
	"CE",			// LREG_CE
	"IE",			// LREG_IE
	"ER",			// LREG_ER
	"FR",			// LREG_FR
	"SLR",			// LREG_SLR
	"GVR",			// LREG_GVR
	"GFR",			// LREG_GFR
	"SR",			// LREG_SR
	"TM",			// LREG_TM
	"PR",			// LREG_PR
	"ESR",			// LREG_ESR
	"NCE",			// LREG_NCE
	"NIE",			// LREG_NIE
	"NER",			// LREG_NER
};

typedef enum e_lscript_op_codes
{
	LOPC_INVALID,
	LOPC_NOOP,
	LOPC_POP,
	LOPC_POPS,
	LOPC_POPL,
	LOPC_POPV,
	LOPC_POPQ,
	LOPC_POPARG,
	LOPC_POPIP,
	LOPC_POPBP,
	LOPC_POPSP,
	LOPC_POPSLR,
	LOPC_DUP,
	LOPC_DUPS,
	LOPC_DUPL,
	LOPC_DUPV,
	LOPC_DUPQ,
	LOPC_STORE,
	LOPC_STORES,
	LOPC_STOREL,
	LOPC_STOREV,
	LOPC_STOREQ,
	LOPC_STOREG,
	LOPC_STOREGS,
	LOPC_STOREGL,
	LOPC_STOREGV,
	LOPC_STOREGQ,
	LOPC_LOADP,
	LOPC_LOADSP,
	LOPC_LOADLP,
	LOPC_LOADVP,
	LOPC_LOADQP,
	LOPC_LOADGP,
	LOPC_LOADGLP,
	LOPC_LOADGSP,
	LOPC_LOADGVP,
	LOPC_LOADGQP,
	LOPC_PUSH,
	LOPC_PUSHS,
	LOPC_PUSHL,
	LOPC_PUSHV,
	LOPC_PUSHQ,
	LOPC_PUSHG,
	LOPC_PUSHGS,
	LOPC_PUSHGL,
	LOPC_PUSHGV,
	LOPC_PUSHGQ,
	LOPC_PUSHIP,
	LOPC_PUSHBP,
	LOPC_PUSHSP,
	LOPC_PUSHARGB,
	LOPC_PUSHARGI,
	LOPC_PUSHARGF,
	LOPC_PUSHARGS,
	LOPC_PUSHARGV,
	LOPC_PUSHARGQ,
	LOPC_PUSHE,
	LOPC_PUSHEV,
	LOPC_PUSHEQ,
	LOPC_PUSHARGE,
	LOPC_ADD,
	LOPC_SUB,
	LOPC_MUL,
	LOPC_DIV,
	LOPC_MOD,
	LOPC_EQ,
	LOPC_NEQ,
	LOPC_LEQ,
	LOPC_GEQ,
	LOPC_LESS,
	LOPC_GREATER,
	LOPC_BITAND,
	LOPC_BITOR,
	LOPC_BITXOR,
	LOPC_BOOLAND,
	LOPC_BOOLOR,
	LOPC_NEG,
	LOPC_BITNOT,
	LOPC_BOOLNOT,
	LOPC_JUMP,
	LOPC_JUMPIF,
	LOPC_JUMPNIF,
	LOPC_STATE,
	LOPC_CALL,
	LOPC_RETURN,
	LOPC_CAST,
	LOPC_STACKTOS,
	LOPC_STACKTOL,
	LOPC_PRINT,
	LOPC_CALLLIB,
	LOPC_CALLLIB_TWO_BYTE,
	LOPC_SHL,
	LOPC_SHR,
	LOPC_EOF
} LSCRIPTOpCodesEnum;

const U8 LSCRIPTOpCodes[LOPC_EOF] =
{
	0x00,	// LOPC_INVALID
	0x00,	// LOPC_NOOP
	0x01,	// LOPC_POP
	0x02,	// LOPC_POPS
	0x03,	// LOPC_POPL
	0x04,	// LOPC_POPV
	0x05,	// LOPC_POPQ
	0x06,	// LOPC_POPARG
	0x07,	// LOPC_POPIP
	0x08,	// LOPC_POPBP
	0x09,	// LOPC_POPSP
	0x0a,	// LOPC_POPSLR
	0x20,	// LOPC_DUP
	0x21,	// LOPC_DUPS
	0x22,	// LOPC_DUPL
	0x23,	// LOPC_DUPV
	0x24,	// LOPC_DUPQ
	0x30,	// LOPC_STORE
	0x31,	// LOPC_STORES
	0x32,	// LOPC_STOREL
	0x33,	// LOPC_STOREV
	0x34,	// LOPC_STOREQ
	0x35,	// LOPC_STOREG
	0x36,	// LOPC_STOREGS
	0x37,	// LOPC_STOREGL
	0x38,	// LOPC_STOREGV
	0x39,	// LOPC_STOREGQ
	0x3a,	// LOPC_LOADP
	0x3b,	// LOPC_LOADSP
	0x3c,	// LOPC_LOADLP
	0x3d,	// LOPC_LOADVP
	0x3e,	// LOPC_LOADQP
	0x3f,	// LOPC_LOADGP
	0x40,	// LOPC_LOADGSP
	0x41,	// LOPC_LOADGLP
	0x42,	// LOPC_LOADGVP
	0x43,	// LOPC_LOADGQP
	0x50,	// LOPC_PUSH
	0x51,	// LOPC_PUSHS
	0x52,	// LOPC_PUSHL
	0x53,	// LOPC_PUSHV
	0x54,	// LOPC_PUSHQ
	0x55,	// LOPC_PUSHG
	0x56,	// LOPC_PUSHGS
	0x57,	// LOPC_PUSHGL
	0x58,	// LOPC_PUSHGV
	0x59,	// LOPC_PUSHGQ
	0x5a,	// LOPC_PUSHIP
	0x5b,	// LOPC_PUSHBP
	0x5c,	// LOPC_PUSHSP
	0x5d,	// LOPC_PUSHARGB
	0x5e,	// LOPC_PUSHARGI
	0x5f,	// LOPC_PUSHARGF
	0x60,	// LOPC_PUSHARGS
	0x61,	// LOPC_PUSHARGV
	0x62,	// LOPC_PUSHARGQ
	0x63,	// LOPC_PUSHE
	0x64,	// LOPC_PUSHEV
	0x65,	// LOPC_PUSHEQ
	0x66,	// LOPC_PUSHARGE
	0x70,	// LOPC_ADD
	0x71,	// LOPC_SUB
	0x72,	// LOPC_MUL
	0x73,	// LOPC_DIV
	0x74,	// LOPC_MOD
	0x75,	// LOPC_EQ
	0x76,	// LOPC_NEQ
	0x77,	// LOPC_LEQ
	0x78,	// LOPC_GEQ
	0x79,	// LOPC_LESS
	0x7a,	// LOPC_GREATER
	0x7b,	// LOPC_BITAND
	0x7c,	// LOPC_BITOR
	0x7d,	// LOPC_BITXOR
	0x7e,	// LOPC_BOOLAND
	0x7f,	// LOPC_BOOLOR
	0x80,	// LOPC_NEG
	0x81,	// LOPC_BITNOT
	0x82,	// LOPC_BOOLNOT
	0x90,	// LOPC_JUMP
	0x91,	// LOPC_JUMPIF
	0x92,	// LOPC_JUMPNIF
	0x93,	// LOPC_STATE
	0x94,	// LOPC_CALL
	0x95,	// LOPC_RETURN
	0xa0,	// LOPC_CAST
	0xb0,	// LOPC_STACKTOS
	0xb1,	// LOPC_STACKTOL
	0xc0,	// LOPC_PRINT
	0xd0,	// LOPC_CALLLIB
	0xd1,	// LOPC_CALLLIB_TWO_BYTE
	0xe0,	// LOPC_SHL
	0xe1	// LOPC_SHR
};

typedef enum e_lscript_state_event_type
{
	LSTT_NULL,
	LSTT_STATE_ENTRY,
	LSTT_STATE_EXIT,
	LSTT_TOUCH_START,
	LSTT_TOUCH,
	LSTT_TOUCH_END,
	LSTT_COLLISION_START,
	LSTT_COLLISION,
	LSTT_COLLISION_END,
	LSTT_LAND_COLLISION_START,
	LSTT_LAND_COLLISION,
	LSTT_LAND_COLLISION_END,
	LSTT_TIMER,
	LSTT_CHAT,
	LSTT_REZ,
	LSTT_SENSOR,
	LSTT_NO_SENSOR,
	LSTT_CONTROL,
	LSTT_MONEY,
	LSTT_EMAIL,
	LSTT_AT_TARGET,
	LSTT_NOT_AT_TARGET,
	LSTT_AT_ROT_TARGET,
	LSTT_NOT_AT_ROT_TARGET,
	LSTT_RTPERMISSIONS,
	LSTT_INVENTORY,
	LSTT_ATTACH,
	LSTT_DATASERVER,
	LSTT_LINK_MESSAGE,
	LSTT_MOVING_START,
	LSTT_MOVING_END,
	LSTT_OBJECT_REZ,
	LSTT_REMOTE_DATA,
	LSTT_HTTP_RESPONSE,
	LSTT_HTTP_REQUEST,
	LSTT_EOF,
	
	LSTT_STATE_BEGIN = LSTT_STATE_ENTRY,
	LSTT_STATE_END = LSTT_EOF
} LSCRIPTStateEventType;

const U64 LSCRIPTStateBitField[LSTT_EOF] =
{
	0x0000000000000000,		//	LSTT_NULL
	0x0000000000000001,		//	LSTT_STATE_ENTRY
	0x0000000000000002,		//	LSTT_STATE_EXIT
	0x0000000000000004,		//	LSTT_TOUCH_START
	0x0000000000000008,		//	LSTT_TOUCH
	0x0000000000000010,		//	LSTT_TOUCH_END
	0x0000000000000020,		//	LSTT_COLLISION_START
	0x0000000000000040,		//	LSTT_COLLISION
	0x0000000000000080,		//	LSTT_COLLISION_END
	0x0000000000000100,		//	LSTT_LAND_COLLISION_START
	0x0000000000000200,		//	LSTT_LAND_COLLISION
	0x0000000000000400,		//	LSTT_LAND_COLLISION_END
	0x0000000000000800,		//	LSTT_TIMER
	0x0000000000001000,		//	LSTT_CHAT
	0x0000000000002000,		//	LSTT_REZ
	0x0000000000004000,		//	LSTT_SENSOR
	0x0000000000008000,		//	LSTT_NO_SENSOR
	0x0000000000010000,		//	LSTT_CONTROL
	0x0000000000020000,		//	LSTT_MONEY
	0x0000000000040000,		//	LSTT_EMAIL
	0x0000000000080000,		//	LSTT_AT_TARGET
	0x0000000000100000,		//	LSTT_NOT_AT_TARGET
	0x0000000000200000,		//	LSTT_AT_ROT_TARGET
	0x0000000000400000,		//	LSTT_NOT_AT_ROT_TARGET
	0x0000000000800000,		//	LSTT_RTPERMISSIONS
	0x0000000001000000,		//	LSTT_INVENTORY
	0x0000000002000000,		//	LSTT_ATTACH
	0x0000000004000000,		//	LSTT_DATASERVER
	0x0000000008000000,		//  LSTT_LINK_MESSAGE
	0x0000000010000000,		//  LSTT_MOVING_START
	0x0000000020000000,		//  LSTT_MOVING_END
	0x0000000040000000,		//  LSTT_OBJECT_REZ
	0x0000000080000000,		//  LSTT_REMOTE_DATA
	0x0000000100000000LL,	// LSTT_HTTP_RESPOSE
	0x0000000200000000LL 	// LSTT_HTTP_REQUEST
};

inline S32 get_event_handler_jump_position(U64 bit_field, LSCRIPTStateEventType type)
{
	S32 count = 0, position = LSTT_STATE_ENTRY;
	while (position < type)
	{
		if (bit_field & 0x1)
		{
			count++;
		}
		bit_field >>= 1;
		position++;
	}
	return count;
}

inline S32 get_number_of_event_handlers(U64 bit_field)
{
	S32 count = 0, position = 0;
	while (position < LSTT_EOF)
	{
		if (bit_field & 0x1)
		{
			count++;
		}
		bit_field >>= 1;
		position++;
	}
	return count;
}

typedef enum e_lscript_types
{
	LST_NULL,
	LST_INTEGER,
	LST_FLOATINGPOINT,
	LST_STRING,
	LST_KEY,
	LST_VECTOR,
	LST_QUATERNION,
	LST_LIST,
	LST_UNDEFINED,
	LST_EOF
} LSCRIPTType;

const U8 LSCRIPTTypeByte[LST_EOF] =
{
	LST_NULL,
	LST_INTEGER,
	LST_FLOATINGPOINT,
	LST_STRING,
	LST_KEY,
	LST_VECTOR,
	LST_QUATERNION,
	LST_LIST,
	LST_NULL,
};

const U8 LSCRIPTTypeHi4Bits[LST_EOF] =
{
	LST_NULL,
	LST_INTEGER << 4,
	LST_FLOATINGPOINT << 4,
	LST_STRING << 4,
	LST_KEY << 4,
	LST_VECTOR << 4,
	LST_QUATERNION << 4,
	LST_LIST << 4,
	LST_UNDEFINED << 4,
};

const char * const LSCRIPTTypeNames[LST_EOF] = 	/*Flawfinder: ignore*/
{
	"VOID",
	"integer",
	"float",
	"string",
	"key",
	"vector",
	"quaternion",
	"list",
	"invalid"
};

const S32 LSCRIPTDataSize[LST_EOF] =
{
	0,	// VOID
	4,	// integer
	4,	// float
	4,	// string
	4,	// key
	12,	// vector
	16,	// quaternion
	4,	// list
	0	// invalid
};


typedef enum e_lscript_runtime_faults
{
	LSRF_INVALID,
	LSRF_MATH,
	LSRF_STACK_HEAP_COLLISION,
	LSRF_BOUND_CHECK_ERROR,
	LSRF_HEAP_ERROR,
	LSRF_VERSION_MISMATCH,
	LSRF_MISSING_INVENTORY,
	LSRF_SANDBOX,
	LSRF_CHAT_OVERRUN,
	LSRF_TOO_MANY_LISTENS,
	LSRF_NESTING_LISTS,
	LSRF_CLI,
	LSRF_EOF
} LSCRIPTRunTimeFaults;

extern const char* LSCRIPTRunTimeFaultStrings[LSRF_EOF]; 	/*Flawfinder: ignore*/

typedef enum e_lscript_runtime_permissions
{
	SCRIPT_PERMISSION_DEBIT,
	SCRIPT_PERMISSION_TAKE_CONTROLS,
	SCRIPT_PERMISSION_REMAP_CONTROLS,
	SCRIPT_PERMISSION_TRIGGER_ANIMATION,
	SCRIPT_PERMISSION_ATTACH,
	SCRIPT_PERMISSION_RELEASE_OWNERSHIP,
	SCRIPT_PERMISSION_CHANGE_LINKS,
	SCRIPT_PERMISSION_CHANGE_JOINTS,
	SCRIPT_PERMISSION_CHANGE_PERMISSIONS,
	SCRIPT_PERMISSION_TRACK_CAMERA,
	SCRIPT_PERMISSION_CONTROL_CAMERA,
	SCRIPT_PERMISSION_TELEPORT,
	SCRIPT_PERMISSION_EOF
} LSCRIPTRunTimePermissions;

const U32 LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_EOF] =
{
	(0x1 << 1),	//	SCRIPT_PERMISSION_DEBIT,
	(0x1 << 2),	//	SCRIPT_PERMISSION_TAKE_CONTROLS,
	(0x1 << 3),	//	SCRIPT_PERMISSION_REMAP_CONTROLS,
	(0x1 << 4),	//	SCRIPT_PERMISSION_TRIGGER_ANIMATION,
	(0x1 << 5),	//	SCRIPT_PERMISSION_ATTACH,
	(0x1 << 6),	//	SCRIPT_PERMISSION_RELEASE_OWNERSHIP,
	(0x1 << 7),	//	SCRIPT_PERMISSION_CHANGE_LINKS,
	(0x1 << 8),	//	SCRIPT_PERMISSION_CHANGE_JOINTS,
	(0x1 << 9),	//	SCRIPT_PERMISSION_CHANGE_PERMISSIONS
	(0x1 << 10),//	SCRIPT_PERMISSION_TRACK_CAMERA
	(0x1 << 11),//	SCRIPT_PERMISSION_CONTROL_CAMERA
	(0x1 << 12),//	SCRIPT_PERMISSION_TELEPORT
};

// http_request string constants
extern const char* URL_REQUEST_GRANTED;
extern const char* URL_REQUEST_DENIED;
extern const U64 LSL_HTTP_REQUEST_TIMEOUT_USEC;

#endif

