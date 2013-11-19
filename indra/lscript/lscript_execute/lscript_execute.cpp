/**
 * @file lscript_execute.cpp
 * @brief classes to execute bytecode
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

#include "linden_common.h"

#include <algorithm>
#include <sstream>

#include "lscript_execute.h"
#include "lltimer.h"
#include "lscript_readlso.h"
#include "lscript_library.h"
#include "lscript_heapruntime.h"
#include "lscript_alloc.h"
#include "llstat.h"


// Static
const	S32	DEFAULT_SCRIPT_TIMER_CHECK_SKIP = 4;
S32		LLScriptExecute::sTimerCheckSkip = DEFAULT_SCRIPT_TIMER_CHECK_SKIP;

void (*binary_operations[LST_EOF][LST_EOF])(U8 *buffer, LSCRIPTOpCodesEnum opcode);
void (*unary_operations[LST_EOF])(U8 *buffer, LSCRIPTOpCodesEnum opcode);

const char* LSCRIPTRunTimeFaultStrings[LSRF_EOF] =		/*Flawfinder: ignore*/
{
	"Invalid",				//	LSRF_INVALID,
	"Math Error",			//	LSRF_MATH,
	"Stack-Heap Collision",	//	LSRF_STACK_HEAP_COLLISION,
	"Bounds Check Error",	//	LSRF_BOUND_CHECK_ERROR,
	"Heap Error",			//	LSRF_HEAP_ERROR,
	"Version Mismatch",		//	LSRF_VERSION_MISMATCH,
	"Missing Inventory",	//	LSRF_MISSING_INVENTORY,
	"Hit Sandbox Limit",	//	LSRF_SANDBOX,
	"Chat Overrun",			//	LSRF_CHAT_OVERRUN,
	"Too Many Listens",			  //	LSRF_TOO_MANY_LISTENS,
	"Lists may not contain lists", //	LSRF_NESTING_LISTS,
	"CLI Exception" // LSRF_CLI
};

void LLScriptExecuteLSL2::startRunning() {}
void LLScriptExecuteLSL2::stopRunning() {}

const char* URL_REQUEST_GRANTED = "URL_REQUEST_GRANTED";
const char* URL_REQUEST_DENIED = "URL_REQUEST_DENIED";

// HTTP Requests to LSL scripts will time out after 25 seconds.
const U64 LSL_HTTP_REQUEST_TIMEOUT_USEC = 25 * USEC_PER_SEC; 

LLScriptExecuteLSL2::LLScriptExecuteLSL2(LLFILE *fp)
{
	U8  sizearray[4];
	size_t filesize;
	S32 pos = 0;
	if (fread(&sizearray, 1, 4, fp) != 4)
	{
		llwarns << "Short read" << llendl;
		filesize = 0;
	} else {
		filesize = bytestream2integer(sizearray, pos);
	}
	mBuffer = new U8[filesize];
	fseek(fp, 0, SEEK_SET);
	if (fread(mBuffer, 1, filesize, fp) != filesize)
	{
		llwarns << "Short read" << llendl;
	}
	fclose(fp);

	init();
}

LLScriptExecuteLSL2::LLScriptExecuteLSL2(const U8* bytecode, U32 bytecode_size)
{
	mBuffer = new U8[TOP_OF_MEMORY];
	memset(mBuffer + bytecode_size, 0, TOP_OF_MEMORY - bytecode_size);
	S32 src_offset = 0;
	S32 dest_offset = 0;
	bytestream2bytestream(mBuffer, dest_offset, bytecode, src_offset, bytecode_size);
	mBytecodeSize = bytecode_size;
	mBytecode = new U8[mBytecodeSize];
	memcpy(mBytecode, bytecode, mBytecodeSize);
	init();
}

LLScriptExecute::~LLScriptExecute() {}
LLScriptExecuteLSL2::~LLScriptExecuteLSL2()
{
	delete[] mBuffer;
	delete[] mBytecode;
}

void LLScriptExecuteLSL2::init()
{
	S32 i, j;

	mInstructionCount = 0;

	for (i = 0; i < 256; i++)
	{
		mExecuteFuncs[i] = run_noop;
	}
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_NOOP]] = run_noop;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POP]] = run_pop;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPS]] = run_pops;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPL]] = run_popl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPV]] = run_popv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPQ]] = run_popq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPARG]] = run_poparg;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPIP]] = run_popip;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPBP]] = run_popbp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPSP]] = run_popsp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_POPSLR]] = run_popslr;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DUP]] = run_dup;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DUPS]] = run_dups;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DUPL]] = run_dupl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DUPV]] = run_dupv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DUPQ]] = run_dupq;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STORE]] = run_store;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STORES]] = run_stores;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREL]] = run_storel;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREV]] = run_storev;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREQ]] = run_storeq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREG]] = run_storeg;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREGL]] = run_storegl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREGS]] = run_storegs;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREGV]] = run_storegv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STOREGQ]] = run_storegq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADP]] = run_loadp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADSP]] = run_loadsp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADLP]] = run_loadlp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADVP]] = run_loadvp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADQP]] = run_loadqp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADGP]] = run_loadgp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADGSP]] = run_loadgsp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADGLP]] = run_loadglp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADGVP]] = run_loadgvp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LOADGQP]] = run_loadgqp;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSH]] = run_push;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHS]] = run_pushs;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHL]] = run_pushl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHV]] = run_pushv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHQ]] = run_pushq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHG]] = run_pushg;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHGS]] = run_pushgs;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHGL]] = run_pushgl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHGV]] = run_pushgv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHGQ]] = run_pushgq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHIP]] = run_puship;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHSP]] = run_pushsp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHBP]] = run_pushbp;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGB]] = run_pushargb;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGI]] = run_pushargi;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGF]] = run_pushargf;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGS]] = run_pushargs;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGV]] = run_pushargv;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGQ]] = run_pushargq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHE]] = run_pushe;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHEV]] = run_pushev;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHEQ]] = run_pusheq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PUSHARGE]] = run_pusharge;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_ADD]] = run_add;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_SUB]] = run_sub;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_MUL]] = run_mul;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_DIV]] = run_div;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_MOD]] = run_mod;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_EQ]] = run_eq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_NEQ]] = run_neq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LEQ]] = run_leq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_GEQ]] = run_geq;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_LESS]] = run_less;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_GREATER]] = run_greater;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BITAND]] = run_bitand;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BITOR]] = run_bitor;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BITXOR]] = run_bitxor;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BOOLAND]] = run_booland;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BOOLOR]] = run_boolor;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_SHL]] = run_shl;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_SHR]] = run_shr;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_NEG]] = run_neg;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BITNOT]] = run_bitnot;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_BOOLNOT]] = run_boolnot;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_JUMP]] = run_jump;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_JUMPIF]] = run_jumpif;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_JUMPNIF]] = run_jumpnif;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STATE]] = run_state;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_CALL]] = run_call;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_RETURN]] = run_return;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_CAST]] = run_cast;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STACKTOS]] = run_stacktos;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_STACKTOL]] = run_stacktol;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_PRINT]] = run_print;

	mExecuteFuncs[LSCRIPTOpCodes[LOPC_CALLLIB]] = run_calllib;
	mExecuteFuncs[LSCRIPTOpCodes[LOPC_CALLLIB_TWO_BYTE]] = run_calllib_two_byte;

	for (i = 0; i < LST_EOF; i++)
	{
		for (j = 0; j < LST_EOF; j++)
		{
			binary_operations[i][j] = unknown_operation;
		}
	}

	binary_operations[LST_INTEGER][LST_INTEGER] = integer_integer_operation;
	binary_operations[LST_INTEGER][LST_FLOATINGPOINT] = integer_float_operation;
	binary_operations[LST_INTEGER][LST_VECTOR] = integer_vector_operation;

	binary_operations[LST_FLOATINGPOINT][LST_INTEGER] = float_integer_operation;
	binary_operations[LST_FLOATINGPOINT][LST_FLOATINGPOINT] = float_float_operation;
	binary_operations[LST_FLOATINGPOINT][LST_VECTOR] = float_vector_operation;

	binary_operations[LST_STRING][LST_STRING] = string_string_operation;
	binary_operations[LST_STRING][LST_KEY] = string_key_operation;

	binary_operations[LST_KEY][LST_STRING] = key_string_operation;
	binary_operations[LST_KEY][LST_KEY] = key_key_operation;

	binary_operations[LST_VECTOR][LST_INTEGER] = vector_integer_operation;
	binary_operations[LST_VECTOR][LST_FLOATINGPOINT] = vector_float_operation;
	binary_operations[LST_VECTOR][LST_VECTOR] = vector_vector_operation;
	binary_operations[LST_VECTOR][LST_QUATERNION] = vector_quaternion_operation;

	binary_operations[LST_QUATERNION][LST_QUATERNION] = quaternion_quaternion_operation;

	binary_operations[LST_INTEGER][LST_LIST] = integer_list_operation;
	binary_operations[LST_FLOATINGPOINT][LST_LIST] = float_list_operation;
	binary_operations[LST_STRING][LST_LIST] = string_list_operation;
	binary_operations[LST_KEY][LST_LIST] = key_list_operation;
	binary_operations[LST_VECTOR][LST_LIST] = vector_list_operation;
	binary_operations[LST_QUATERNION][LST_LIST] = quaternion_list_operation;
	binary_operations[LST_LIST][LST_INTEGER] = list_integer_operation;
	binary_operations[LST_LIST][LST_FLOATINGPOINT] = list_float_operation;
	binary_operations[LST_LIST][LST_STRING] = list_string_operation;
	binary_operations[LST_LIST][LST_KEY] = list_key_operation;
	binary_operations[LST_LIST][LST_VECTOR] = list_vector_operation;
	binary_operations[LST_LIST][LST_QUATERNION] = list_quaternion_operation;
	binary_operations[LST_LIST][LST_LIST] = list_list_operation;

	for (i = 0; i < LST_EOF; i++)
	{
		unary_operations[i] = unknown_operation;
	}

	unary_operations[LST_INTEGER] = integer_operation;
	unary_operations[LST_FLOATINGPOINT] = float_operation;
	unary_operations[LST_VECTOR] = vector_operation;
	unary_operations[LST_QUATERNION] = quaternion_operation;

}


// Utility routine for when there's a boundary error parsing bytecode
void LLScriptExecuteLSL2::recordBoundaryError( const LLUUID &id )
{
	set_fault(mBuffer, LSRF_BOUND_CHECK_ERROR);
	llwarns << "Script boundary error for ID " << id << llendl;
}


//	set IP to the event handler with some error checking
void LLScriptExecuteLSL2::setStateEventOpcoodeStartSafely( S32 state, LSCRIPTStateEventType event, const LLUUID &id )
{
	S32			opcode_start = get_state_event_opcoode_start( mBuffer, state, event );
	if ( opcode_start == -1 )
	{
		recordBoundaryError( id );
	}
	else
	{
		set_ip( mBuffer, opcode_start );
	}
}




S32 lscript_push_variable(LLScriptLibData *data, U8 *buffer);

void LLScriptExecuteLSL2::resumeEventHandler(BOOL b_print, const LLUUID &id, F32 time_slice)
{
	//	call opcode run function pointer with buffer and IP
	mInstructionCount++;
	S32 value = get_register(mBuffer, LREG_IP);
	S32 tvalue = value;
	S32	opcode = safe_instruction_bytestream2byte(mBuffer, tvalue);
	mExecuteFuncs[opcode](mBuffer, value, b_print, id);
	set_ip(mBuffer, value);
	add_register_fp(mBuffer, LREG_ESR, -0.1f);
	//	lsa_print_heap(mBuffer);

	if (b_print)
	{
		lsa_print_heap(mBuffer);
		printf("ip: 0x%X\n", get_register(mBuffer, LREG_IP));
		printf("sp: 0x%X\n", get_register(mBuffer, LREG_SP));
		printf("bp: 0x%X\n", get_register(mBuffer, LREG_BP));
		printf("hr: 0x%X\n", get_register(mBuffer, LREG_HR));
		printf("hp: 0x%X\n", get_register(mBuffer, LREG_HP));
	}

	// NOTE: Babbage: all mExecuteFuncs return false.
}

void LLScriptExecuteLSL2::callEventHandler(LSCRIPTStateEventType event, const LLUUID &id, F32 time_slice)
{
	S32 major_version = getMajorVersion();
	// push a zero to be popped
	lscript_push(mBuffer, 0);
	// push sp as current bp
	S32 sp = get_register(mBuffer, LREG_SP);
	lscript_push(mBuffer, sp);

	// Update current handler and current events registers.
	set_event_register(mBuffer, LREG_IE, LSCRIPTStateBitField[event], major_version);
	U64 current_events = get_event_register(mBuffer, LREG_CE, major_version);
	current_events &= ~LSCRIPTStateBitField[event];
	set_event_register(mBuffer, LREG_CE, current_events, major_version);

	// now, push any additional stack space
	U32 current_state = get_register(mBuffer, LREG_CS);
	S32 additional_size = get_event_stack_size(mBuffer, current_state, event);
	lscript_pusharge(mBuffer, additional_size);

	// now set the bp correctly
	sp = get_register(mBuffer, LREG_SP);
	sp += additional_size;
	set_bp(mBuffer, sp);

	// set IP to the function
	S32			opcode_start = get_state_event_opcoode_start(mBuffer, current_state, event);
	set_ip(mBuffer, opcode_start);
}

//void callStateExitHandler()
//{
//	// push a zero to be popped
//	lscript_push(mBuffer, 0);
//	// push sp as current bp
//	S32 sp = get_register(mBuffer, LREG_SP);
//	lscript_push(mBuffer, sp);
//
//	// now, push any additional stack space
//	S32 additional_size = get_event_stack_size(mBuffer, current_state, LSTT_STATE_EXIT);
//	lscript_pusharge(mBuffer, additional_size);
//
//	sp = get_register(mBuffer, LREG_SP);
//	sp += additional_size;
//	set_bp(mBuffer, sp);
//
//	// set IP to the event handler
//	S32			opcode_start = get_state_event_opcoode_start(mBuffer, current_state, LSTT_STATE_EXIT);
//	set_ip(mBuffer, opcode_start);
//}
//
//void callStateEntryHandler()
//{
//	// push a zero to be popped
//	lscript_push(mBuffer, 0);
//	// push sp as current bp
//	S32 sp = get_register(mBuffer, LREG_SP);
//	lscript_push(mBuffer, sp);
//
//	event = return_first_event((S32)LSCRIPTStateBitField[LSTT_STATE_ENTRY]);
//	set_event_register(mBuffer, LREG_IE, LSCRIPTStateBitField[event], major_version);
//	current_events &= ~LSCRIPTStateBitField[event];
//	set_event_register(mBuffer, LREG_CE, current_events, major_version);
//
//	// now, push any additional stack space
//	S32 additional_size = get_event_stack_size(mBuffer, current_state, event) - size;
//	lscript_pusharge(mBuffer, additional_size);
//
//	// now set the bp correctly
//	sp = get_register(mBuffer, LREG_SP);
//	sp += additional_size + size;
//	set_bp(mBuffer, sp);
//	// set IP to the function
//	S32			opcode_start = get_state_event_opcoode_start(mBuffer, current_state, event);
//	set_ip(mBuffer, opcode_start);
//}

void LLScriptExecuteLSL2::callQueuedEventHandler(LSCRIPTStateEventType event, const LLUUID &id, F32 time_slice)
{
	S32 major_version = getMajorVersion();
	LLScriptDataCollection* eventdata;

	for (eventdata = mEventData.mEventDataList.getFirstData(); eventdata; eventdata = mEventData.mEventDataList.getNextData())
	{
		if (eventdata->mType == event)
		{
			// push a zero to be popped
			lscript_push(mBuffer, 0);
			// push sp as current bp
			S32 sp = get_register(mBuffer, LREG_SP);
			lscript_push(mBuffer, sp);

			// Update current handler and current events registers.
			set_event_register(mBuffer, LREG_IE, LSCRIPTStateBitField[event], major_version);
			U64 current_events = get_event_register(mBuffer, LREG_CE, major_version);
			current_events &= ~LSCRIPTStateBitField[event];
			set_event_register(mBuffer, LREG_CE, current_events, major_version);

			// push any arguments that need to be pushed onto the stack
			// last piece of data will be type LST_NULL
			LLScriptLibData	*data = eventdata->mData;
			U32 size = 0;
			while (data->mType)
			{
				size += lscript_push_variable(data, mBuffer);
				data++;
			}
			// now, push any additional stack space
			U32 current_state = get_register(mBuffer, LREG_CS);
			S32 additional_size = get_event_stack_size(mBuffer, current_state, event) - size;
			lscript_pusharge(mBuffer, additional_size);

			// now set the bp correctly
			sp = get_register(mBuffer, LREG_SP);
			sp += additional_size + size;
			set_bp(mBuffer, sp);

			// set IP to the function
			S32			opcode_start = get_state_event_opcoode_start(mBuffer, current_state, event);
			set_ip(mBuffer, opcode_start);

			mEventData.mEventDataList.deleteCurrentData();
			break;
		}
	}
}

void LLScriptExecuteLSL2::callNextQueuedEventHandler(U64 event_register, const LLUUID &id, F32 time_slice)
{
	S32 major_version = getMajorVersion();
	LLScriptDataCollection* eventdata = mEventData.getNextEvent();
	if (eventdata)
	{
		LSCRIPTStateEventType event = eventdata->mType;

		// make sure that we can actually handle this one
		if (LSCRIPTStateBitField[event] & event_register)
		{
			// push a zero to be popped
			lscript_push(mBuffer, 0);
			// push sp as current bp
			S32 sp = get_register(mBuffer, LREG_SP);
			lscript_push(mBuffer, sp);

			// Update current handler and current events registers.
			set_event_register(mBuffer, LREG_IE, LSCRIPTStateBitField[event], major_version);
			U64 current_events = get_event_register(mBuffer, LREG_CE, major_version);
			current_events &= ~LSCRIPTStateBitField[event];
			set_event_register(mBuffer, LREG_CE, current_events, major_version);

			// push any arguments that need to be pushed onto the stack
			// last piece of data will be type LST_NULL
			LLScriptLibData	*data = eventdata->mData;
			U32 size = 0;
			while (data->mType)
			{
				size += lscript_push_variable(data, mBuffer);
				data++;
			}

			// now, push any additional stack space
			U32 current_state = get_register(mBuffer, LREG_CS);
			S32 additional_size = get_event_stack_size(mBuffer, current_state, event) - size;
			lscript_pusharge(mBuffer, additional_size);

			// now set the bp correctly
			sp = get_register(mBuffer, LREG_SP);
			sp += additional_size + size;
			set_bp(mBuffer, sp);

			// set IP to the function
			S32			opcode_start = get_state_event_opcoode_start(mBuffer, current_state, event);
			set_ip(mBuffer, opcode_start);
		}
		else
		{
			llwarns << "Somehow got an event that we're not registered for!" << llendl;
		}
		delete eventdata;
	}
}

U64 LLScriptExecuteLSL2::nextState()
{
	// copy NS to CS
	S32 next_state = get_register(mBuffer, LREG_NS);
	set_register(mBuffer, LREG_CS, next_state);

	// copy new state's handled events into ER (SR + CS*4 + 4)
	return get_handled_events(mBuffer, next_state);
}

//virtual 
void LLScriptExecuteLSL2::addEvent(LLScriptDataCollection* event)
{
	mEventData.addEventData(event);
}

//virtual 
void LLScriptExecuteLSL2::removeEventType(LSCRIPTStateEventType event_type)
{
	mEventData.removeEventType(event_type);
}

//virtual 
F32 LLScriptExecuteLSL2::getSleep() const
{
	return get_register_fp(mBuffer, LREG_SLR);
}

//virtual 
void LLScriptExecuteLSL2::setSleep(F32 value)
{
	set_register_fp(mBuffer, LREG_SLR, value);
}

//virtual 
U64 LLScriptExecuteLSL2::getCurrentHandler()
{
	return get_event_register(mBuffer, LREG_IE, getMajorVersion());
}

//virtual 
F32 LLScriptExecuteLSL2::getEnergy() const
{
	return get_register_fp(mBuffer, LREG_ESR);
}

//virtual 
void LLScriptExecuteLSL2::setEnergy(F32 value)
{
	set_register_fp(mBuffer, LREG_ESR, value);
}

//virtual 
U32 LLScriptExecuteLSL2::getFreeMemory()
{
	return get_register(mBuffer, LREG_SP) - get_register(mBuffer, LREG_HP);
}

//virtual 
S32 LLScriptExecuteLSL2::getParameter()
{
	return get_register(mBuffer, LREG_PR);
}

//virtual 
void LLScriptExecuteLSL2::setParameter(S32 value)
{
	set_register(mBuffer, LREG_PR, value);
}


S32 LLScriptExecuteLSL2::writeState(U8 **dest, U32 header_size, U32 footer_size)
{
	// data format:
	// 4 bytes of size of Registers, Name and Description, and Global Variables
	// Registers, Name and Description, and Global Variables data
	// 4 bytes of size of Heap
	// Heap data
	// 4 bytes of stack size
	// Stack data

	S32 registers_size = get_register(mBuffer, LREG_GFR);

	if (get_register(mBuffer, LREG_HP) > TOP_OF_MEMORY)
		reset_hp_to_safe_spot(mBuffer);

	S32 heap_size = get_register(mBuffer, LREG_HP) - get_register(mBuffer, LREG_HR);
	S32 stack_size = get_register(mBuffer, LREG_TM) - get_register(mBuffer, LREG_SP);
	S32 total_size = registers_size + LSCRIPTDataSize[LST_INTEGER] + 
						heap_size + LSCRIPTDataSize[LST_INTEGER] + 
						stack_size + LSCRIPTDataSize[LST_INTEGER];

	// actually allocate data
	delete[] *dest;
	*dest = new U8[header_size + total_size + footer_size];
	memset(*dest, 0, header_size + total_size + footer_size);
	S32 dest_offset = header_size;
	S32 src_offset = 0;

	// registers
	integer2bytestream(*dest, dest_offset, registers_size);

	// llinfos << "Writing CE: " << getCurrentEvents() << llendl;
	bytestream2bytestream(*dest, dest_offset, mBuffer, src_offset, registers_size);

	// heap
	integer2bytestream(*dest, dest_offset, heap_size);

	src_offset = get_register(mBuffer, LREG_HR);
	bytestream2bytestream(*dest, dest_offset, mBuffer, src_offset, heap_size);

	// stack
	integer2bytestream(*dest, dest_offset, stack_size);

	src_offset = get_register(mBuffer, LREG_SP);
	bytestream2bytestream(*dest, dest_offset, mBuffer, src_offset, stack_size);

	return total_size;
}

S32 LLScriptExecuteLSL2::writeBytecode(U8 **dest)
{
	// data format:
	// registers through top of heap
	// Heap data
	S32 total_size = get_register(mBuffer, LREG_HP);

	// actually allocate data
	delete [] *dest;
	*dest = new U8[total_size];
	S32 dest_offset = 0;
	S32 src_offset = 0;

	bytestream2bytestream(*dest, dest_offset, mBuffer, src_offset, total_size);

	return total_size;
}

S32 LLScriptExecuteLSL2::readState(U8 *src)
{
	// first, blitz heap and stack
	S32 hr = get_register(mBuffer, LREG_HR);
	S32 tm = get_register(mBuffer, LREG_TM);
	memset(mBuffer + hr, 0, tm - hr);

	S32 src_offset = 0;
	S32 dest_offset = 0;
	S32 size;

	// read register size
	size = bytestream2integer(src, src_offset);

	// copy data into register area
	bytestream2bytestream(mBuffer, dest_offset, src, src_offset, size);
//	llinfos << "Read CE: " << getCurrentEvents() << llendl;
	if (get_register(mBuffer, LREG_TM) != TOP_OF_MEMORY)
	{
		llwarns << "Invalid state. Top of memory register does not match"
				<< " constant." << llendl;
		reset_hp_to_safe_spot(mBuffer);
		return -1;
	}
	
	// read heap size
	size = bytestream2integer(src, src_offset);

	// set dest offset
	dest_offset = get_register(mBuffer, LREG_HR);

	if (dest_offset + size > TOP_OF_MEMORY)
	{
		reset_hp_to_safe_spot(mBuffer);
		return -1;
	}

	// copy data into heap area
	bytestream2bytestream(mBuffer, dest_offset, src, src_offset, size);

	// read stack size
	size = bytestream2integer(src, src_offset);

	// set dest offset
	dest_offset = get_register(mBuffer, LREG_SP);

	if (dest_offset + size > TOP_OF_MEMORY)
	{
		reset_hp_to_safe_spot(mBuffer);
		return -1;
	}

	// copy data into heap area
	bytestream2bytestream(mBuffer, dest_offset, src, src_offset, size);

	// Return offset to first byte after read data.
	return src_offset;
}

void LLScriptExecuteLSL2::reset()
{
	LLScriptExecute::reset();

	const U8 *src = getBytecode();
	S32 size = getBytecodeSize();

	if (!src)
		return;

	// first, blitz heap and stack
	S32 hr = get_register(mBuffer, LREG_HR);
	S32 tm = get_register(mBuffer, LREG_TM);
	memset(mBuffer + hr, 0, tm - hr);

	S32 dest_offset = 0;
	S32 src_offset = 0;

	bytestream2bytestream(mBuffer, dest_offset, src, src_offset, size);
}

S32 LLScriptExecuteLSL2::getMajorVersion() const
{
	S32 version = getVersion();
	S32 major_version = 0;
	if (version == LSL2_VERSION1_END_NUMBER){
		major_version = 1;
	}
	else if (version == LSL2_VERSION_NUMBER)
	{
		major_version = 2;
	}
	return major_version;
}

U32 LLScriptExecuteLSL2::getUsedMemory()
{
	return getBytecodeSize();
}

LLScriptExecute::LLScriptExecute() :
	mReset(FALSE)
{
}

void LLScriptExecute::reset()
{
	mReset = FALSE;
}

bool LLScriptExecute::isYieldDue() const
{
	if(mReset)
	{
		return true;
	}
			
	if(getSleep() > 0.f)
	{
		return true;
	}

	if(isFinished())
	{
		return true;
	}

	// State changes can occur within a single time slice,
	// but LLScriptData's clean up is required. Yield here
	// to allow LLScriptData to perform cleanup and then call
	// runQuanta again.
	if(isStateChangePending())
	{
		return true;
	}

	return false;
}

// Run smallest number of instructions possible: 
// a single instruction for LSL2, a segment between save tests for Mono
void LLScriptExecute::runInstructions(BOOL b_print, const LLUUID &id, 
									 const char **errorstr, 
									 U32& events_processed,
									 F32 quanta)
{
	//  is there a fault?
	//	if yes, print out message and exit
	S32 value = getVersion();
	if ( (value != LSL2_VERSION1_END_NUMBER) && (value != LSL2_VERSION_NUMBER) )
	{
		setFault(LSRF_VERSION_MISMATCH);
	}
	value = getFaults();
	if (value > LSRF_INVALID && value < LSRF_EOF)
	{
		if (b_print)
		{
			printf("Error!\n");
		}
		*errorstr = LSCRIPTRunTimeFaultStrings[value];
		return;
	}
	else
	{
		*errorstr = NULL;
	}

	if (! isFinished())
	{
		resumeEventHandler(b_print, id, quanta);
		return;
	}
	else
	{
		// make sure that IE is zero
		setCurrentHandler(0);

		//	if no, we're in a state and waiting for an event
		U64 current_events = getCurrentEvents();
		U64 event_register = getEventHandlers();

		//	check NS to see if need to switch states (NS != CS)
		if (isStateChangePending())
		{
			// ok, blow away any pending events
			deleteAllEvents();

			// if yes, check state exit flag is set
			if (current_events & LSCRIPTStateBitField[LSTT_STATE_EXIT])
			{
				// if yes, clear state exit flag
				setCurrentHandler(LSCRIPTStateBitField[LSTT_STATE_EXIT]);
				current_events &= ~LSCRIPTStateBitField[LSTT_STATE_EXIT];
				setCurrentEvents(current_events);

				// check state exit event handler
				// if there is a handler, call it
				if (event_register & LSCRIPTStateBitField[LSTT_STATE_EXIT])
				{
					++events_processed;
					callEventHandler(LSTT_STATE_EXIT, id, quanta);
					return;
				}
			}

			// if no handler or no state exit flag switch to new state
			// set state entry flag and clear other CE flags
			current_events = LSCRIPTStateBitField[LSTT_STATE_ENTRY];
			setCurrentEvents(current_events);

			U64 handled_events = nextState();
			setEventHandlers(handled_events);
		}

		// try to get next event from stack
		BOOL b_done = FALSE;
		LSCRIPTStateEventType event = LSTT_NULL;

		current_events = getCurrentEvents();
		event_register = getEventHandlers();

		// first, check to see if state_entry or onrez are raised and handled
		if ((current_events & LSCRIPTStateBitField[LSTT_STATE_ENTRY])
			&&(current_events & event_register))
		{
			++events_processed;
			callEventHandler(LSTT_STATE_ENTRY, id, quanta);
			b_done = TRUE;
		}
		else if ((current_events & LSCRIPTStateBitField[LSTT_REZ])
				 &&(current_events & event_register))
		{
			++events_processed;
			callQueuedEventHandler(LSTT_REZ, id, quanta);
			b_done = TRUE;
		}

		if (!b_done)
		{
			// Call handler for next queued event.
			if(getEventCount() > 0)
			{
				++events_processed;
				callNextQueuedEventHandler(event_register, id, quanta);
			}
			else
			{
				// if no data waiting, do it the old way:
				U64 handled_current = current_events & event_register;
				if (handled_current)
				{
					event = return_first_event((S32)handled_current);
					++events_processed;
					callEventHandler(event, id, quanta);
				}
			}
			b_done = TRUE;
		}
	}
}

// Run for a single timeslice, or until a yield or state transition is due
F32 LLScriptExecute::runQuanta(BOOL b_print, const LLUUID &id, const char **errorstr, F32 quanta, U32& events_processed, LLTimer& timer)
{
	S32 timer_checks = 0;
	F32 inloop = 0;

	// Loop while not finished, yield not due and time remaining
	// NOTE: Default implementation does not do adaptive timer skipping
	// to preserve current LSL behaviour and not break scripts that rely
	// on current execution speed.
	while(true)
	{
		runInstructions(b_print, id, errorstr,
						events_processed, quanta);
		
		if(isYieldDue())
		{
			break;
		}
		else if(timer_checks++ >= LLScriptExecute::sTimerCheckSkip)
		{
			inloop = timer.getElapsedTimeF32();
			if(inloop > quanta)
			{
				break;
			}
			timer_checks = 0;
		}
	}
	if (inloop == 0.0f)
	{
		inloop = timer.getElapsedTimeF32();
	}
	return inloop;
}

F32 LLScriptExecute::runNested(BOOL b_print, const LLUUID &id, const char **errorstr, F32 quanta, U32& events_processed, LLTimer& timer)
{
	return LLScriptExecute::runQuanta(b_print, id, errorstr, quanta, events_processed, timer);
}

BOOL run_noop(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tNOOP\n", offset);
	offset++;
	return FALSE;
}

BOOL run_pop(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOP\n", offset);
	offset++;
	lscript_poparg(buffer, LSCRIPTDataSize[LST_INTEGER]);
	return FALSE;
}

BOOL run_pops(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPS\n", offset);
	offset++;
	S32 address = lscript_pop_int(buffer);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_popl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPL\n", offset);
	offset++;
	S32 address = lscript_pop_int(buffer);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_popv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPV\n", offset);
	offset++;
	lscript_poparg(buffer, LSCRIPTDataSize[LST_VECTOR]);
	return FALSE;
}

BOOL run_popq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPQ\n", offset);
	offset++;
	lscript_poparg(buffer, LSCRIPTDataSize[LST_QUATERNION]);
	return FALSE;
}

BOOL run_poparg(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPARG ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);
	lscript_poparg(buffer, arg);
	return FALSE;
}

BOOL run_popip(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPIP\n", offset);
	offset++;
	offset = lscript_pop_int(buffer);
	return FALSE;
}

BOOL run_popbp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPBP\n", offset);
	offset++;
	S32 bp = lscript_pop_int(buffer);
	set_bp(buffer, bp);
	return FALSE;
}

BOOL run_popsp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPSP\n", offset);
	offset++;
	S32 sp = lscript_pop_int(buffer);
	set_sp(buffer, sp);
	return FALSE;
}

BOOL run_popslr(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPOPSLR\n", offset);
	offset++;
	S32 slr = lscript_pop_int(buffer);
	set_register(buffer, LREG_SLR, slr);
	return FALSE;
}

BOOL run_dup(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDUP\n", offset);
	offset++;
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_dups(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDUPS\n", offset);
	offset++;
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_dupl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDUPL\n", offset);
	offset++;
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_dupv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDUPV\n", offset);
	offset++;
	S32 sp = get_register(buffer, LREG_SP);
	LLVector3 value;
	bytestream2vector(value, buffer, sp);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_dupq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDUPV\n", offset);
	offset++;
	S32 sp = get_register(buffer, LREG_SP);
	LLQuaternion value;
	bytestream2quaternion(value, buffer, sp);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_store(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTORE ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_stores(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTORES ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);

	S32 address = lscript_local_get(buffer, arg);

	lscript_local_store(buffer, arg, value);
	lsa_increase_ref_count(buffer, value);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_storel(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREL ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);

	S32 address = lscript_local_get(buffer, arg);

	lscript_local_store(buffer, arg, value);
	lsa_increase_ref_count(buffer, value);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_storev(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREV ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	S32 sp = get_register(buffer, LREG_SP);
	bytestream2vector(value, buffer, sp);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_storeq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREQ ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	S32 sp = get_register(buffer, LREG_SP);
	bytestream2quaternion(value, buffer, sp);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_storeg(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREG ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_storegs(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGS ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);

	S32 address = lscript_global_get(buffer, arg);

	lscript_global_store(buffer, arg, value);

	lsa_increase_ref_count(buffer, value);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_storegl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGL ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 sp = get_register(buffer, LREG_SP);
	S32 value = bytestream2integer(buffer, sp);

	S32 address = lscript_global_get(buffer, arg);

	lscript_global_store(buffer, arg, value);

	lsa_increase_ref_count(buffer, value);
	if (address)
		lsa_decrease_ref_count(buffer, address);
	return FALSE;
}

BOOL run_storegv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGV ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	S32 sp = get_register(buffer, LREG_SP);
	bytestream2vector(value, buffer, sp);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_storegq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGQ ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	S32 sp = get_register(buffer, LREG_SP);
	bytestream2quaternion(value, buffer, sp);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_pop_int(buffer);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadsp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTORESP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_pop_int(buffer);

	S32 address = lscript_local_get(buffer, arg);
	if (address)
		lsa_decrease_ref_count(buffer, address);

	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadlp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTORELP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_pop_int(buffer);

	S32 address = lscript_local_get(buffer, arg);
	if (address)
		lsa_decrease_ref_count(buffer, address);

	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadvp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREVP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	lscript_pop_vector(buffer, value);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadqp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREQP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	lscript_pop_quaternion(buffer, value);
	lscript_local_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadgp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_pop_int(buffer);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadgsp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGSP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);
	S32 value = lscript_pop_int(buffer);

	S32 address = lscript_global_get(buffer, arg);
	if (address)
		lsa_decrease_ref_count(buffer, address);

	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadglp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGLP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_pop_int(buffer);

	S32 address = lscript_global_get(buffer, arg);
	if (address)
		lsa_decrease_ref_count(buffer, address);

	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadgvp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGVP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	lscript_pop_vector(buffer, value);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_loadgqp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTOREGQP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	lscript_pop_quaternion(buffer, value);
	lscript_global_store(buffer, arg, value);
	return FALSE;
}

BOOL run_push(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSH ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_local_get(buffer, arg);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_pushs(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHS ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_local_get(buffer, arg);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_pushl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHL ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_local_get(buffer, arg);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_pushv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHV ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	lscript_local_get(buffer, arg, value);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_pushq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHQ ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	lscript_local_get(buffer, arg, value);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_pushg(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHG ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_global_get(buffer, arg);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_pushgs(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHGS ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_global_get(buffer, arg);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_pushgl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHGL ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	S32 value = lscript_global_get(buffer, arg);
	lscript_push(buffer, value);
	lsa_increase_ref_count(buffer, value);
	return FALSE;
}

BOOL run_pushgv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHGV ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLVector3 value;
	lscript_global_get(buffer, arg, value);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_pushgq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHGQ ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("0x%X\n", arg);
	LLQuaternion value;
	lscript_global_get(buffer, arg, value);
	lscript_push(buffer, value);
	return FALSE;
}

BOOL run_puship(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHIP\n", offset);
	offset++;
	lscript_push(buffer, offset);
	return FALSE;
}

BOOL run_pushbp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHBP\n", offset);
	offset++;
	lscript_push(buffer, get_register(buffer, LREG_BP));
	return FALSE;
}

BOOL run_pushsp(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHSP\n", offset);
	offset++;
	lscript_push(buffer, get_register(buffer, LREG_SP));
	return FALSE;
}

BOOL run_pushargb(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHGARGB ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	if (b_print)
		printf("%d\n", (U32)arg);
	lscript_push(buffer, arg);
	return FALSE;
}

BOOL run_pushargi(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGI ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);
	lscript_push(buffer, arg);
	return FALSE;
}

BOOL run_pushargf(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGF ", offset);
	offset++;
	F32 arg = safe_instruction_bytestream2float(buffer, offset);
	if (b_print)
		printf("%f\n", arg);
	lscript_push(buffer, arg);
	return FALSE;
}

BOOL run_pushargs(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGS ", offset);
	S32 toffset = offset;
	safe_instruction_bytestream_count_char(buffer, toffset);
	S32 size = toffset - offset;
	char *arg = new char[size];
	offset++;
	safe_instruction_bytestream2char(arg, buffer, offset, size);
	if (b_print)
		printf("%s\n", arg);
	S32 address = lsa_heap_add_data(buffer, new LLScriptLibData(arg), get_max_heap_size(buffer), TRUE);
	lscript_push(buffer, address);
	delete [] arg;
	return FALSE;
}

BOOL run_pushargv(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGV ", offset);
	offset++;
	LLVector3 arg;
	safe_instruction_bytestream2vector(arg, buffer, offset);
	if (b_print)
		printf("< %f, %f, %f >\n", arg.mV[VX], arg.mV[VY], arg.mV[VZ]);
	lscript_push(buffer, arg);
	return FALSE;
}
BOOL run_pushargq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGQ ", offset);
	offset++;
	LLQuaternion arg;
	safe_instruction_bytestream2quaternion(arg, buffer, offset);
	if (b_print)
		printf("< %f, %f, %f, %f >\n", arg.mQ[VX], arg.mQ[VY], arg.mQ[VZ], arg.mQ[VS]);
	lscript_push(buffer, arg);
	return FALSE;
}
BOOL run_pushe(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHE\n", offset);
	offset++;
	lscript_pusharge(buffer, LSCRIPTDataSize[LST_INTEGER]);
	return FALSE;
}
BOOL run_pushev(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHEV\n", offset);
	offset++;
	lscript_pusharge(buffer, LSCRIPTDataSize[LST_VECTOR]);
	return FALSE;
}
BOOL run_pusheq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHEQ\n", offset);
	offset++;
	lscript_pusharge(buffer, LSCRIPTDataSize[LST_QUATERNION]);
	return FALSE;
}
BOOL run_pusharge(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPUSHARGE ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);
	lscript_pusharge(buffer, arg);
	return FALSE;
}

void print_type(U8 type)
{
	if (type == LSCRIPTTypeByte[LST_INTEGER])
	{
		printf("integer");
	}
	else if (type == LSCRIPTTypeByte[LST_FLOATINGPOINT])
	{
		printf("float");
	}
	else if (type == LSCRIPTTypeByte[LST_STRING])
	{
		printf("string");
	}
	else if (type == LSCRIPTTypeByte[LST_KEY])
	{
		printf("key");
	}
	else if (type == LSCRIPTTypeByte[LST_VECTOR])
	{
		printf("vector");
	}
	else if (type == LSCRIPTTypeByte[LST_QUATERNION])
	{
		printf("quaternion");
	}
	else if (type == LSCRIPTTypeByte[LST_LIST])
	{
		printf("list");
	}
}

void unknown_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	printf("Unknown arithmetic operation!\n");
}

void integer_integer_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 result = 0;

	switch(opcode)
	{
	case LOPC_ADD:
		result = lside + rside;
		break;
	case LOPC_SUB:
		result = lside - rside;
		break;
	case LOPC_MUL:
		result = lside * rside;
		break;
	case LOPC_DIV:
		if (rside){
			if( ( rside == -1 ) || ( rside == (S32) 0xffffffff ) )//	division by -1 can have funny results: multiplication is OK: SL-31252
			{
				result = -1 * lside;
			}
			else
			{
				result = lside / rside;
			}
		}
		else
			set_fault(buffer, LSRF_MATH);
		break;
	case LOPC_MOD:
		if (rside)
		{
			if (rside == -1 || rside == 1 )	 // mod(1) = mod(-1) = 0: SL-31252
			{
				result = 0;
			}
			else
			{
				result = lside % rside;
			}
		}
		else
			set_fault(buffer, LSRF_MATH);
		break;
	case LOPC_EQ:
		result = (lside == rside);
		break;
	case LOPC_NEQ:
		result = (lside != rside);
		break;
	case LOPC_LEQ:
		result = (lside <= rside);
		break;
	case LOPC_GEQ:
		result = (lside >= rside);
		break;
	case LOPC_LESS:
		result = (lside < rside);
		break;
	case LOPC_GREATER:
		result = (lside > rside);
		break;
	case LOPC_BITAND:
		result = (lside & rside);
		break;
	case LOPC_BITOR:
		result = (lside | rside);
		break;
	case LOPC_BITXOR:
		result = (lside ^ rside);
		break;
	case LOPC_BOOLAND:
		result = (lside && rside);
		break;
	case LOPC_BOOLOR:
		result = (lside || rside);
		break;
	case LOPC_SHL:
		result = (lside << rside);
		break;
	case LOPC_SHR:
		result = (lside >> rside);
		break;
	default:
		break;
	}
	lscript_push(buffer, result);
}

void integer_float_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	F32 rside = lscript_pop_float(buffer);
	S32 resulti = 0;
	F32 resultf = 0;

	switch(opcode)
	{
	case LOPC_ADD:
		resultf = lside + rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_SUB:
		resultf = lside - rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_MUL:
		resultf = lside * rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_DIV:
		if (rside)
			resultf = lside / rside;
		else
			set_fault(buffer, LSRF_MATH);
		lscript_push(buffer, resultf);
		break;
	case LOPC_EQ:
		resulti = (lside == rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = (lside != rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LEQ:
		resulti = (lside <= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GEQ:
		resulti = (lside >= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LESS:
		resulti = (lside < rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GREATER:
		resulti = (lside > rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void integer_vector_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	LLVector3 rside;
	lscript_pop_vector(buffer, rside);

	switch(opcode)
	{
	case LOPC_MUL:
		rside *= (F32)lside;
		lscript_push(buffer, rside);
		break;
	default:
		break;
	}
}

void float_integer_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	F32 lside = lscript_pop_float(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti = 0;
	F32 resultf = 0;

	switch(opcode)
	{
	case LOPC_ADD:
		resultf = lside + rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_SUB:
		resultf = lside - rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_MUL:
		resultf = lside * rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_DIV:
		if (rside)
			resultf = lside / rside;
		else
			set_fault(buffer, LSRF_MATH);
		lscript_push(buffer, resultf);
		break;
	case LOPC_EQ:
		resulti = (lside == rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = (lside != rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LEQ:
		resulti = (lside <= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GEQ:
		resulti = (lside >= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LESS:
		resulti = (lside < rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GREATER:
		resulti = (lside > rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void float_float_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	F32 lside = lscript_pop_float(buffer);
	F32 rside = lscript_pop_float(buffer);
	F32 resultf = 0;
	S32 resulti = 0;

	switch(opcode)
	{
	case LOPC_ADD:
		resultf = lside + rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_SUB:
		resultf = lside - rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_MUL:
		resultf = lside * rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_DIV:
		if (rside)
			resultf = lside / rside;
		else
			set_fault(buffer, LSRF_MATH);
		lscript_push(buffer, resultf);
		break;
	case LOPC_EQ:
		resulti = (lside == rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = (lside != rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LEQ:
		resulti = (lside <= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GEQ:
		resulti = (lside >= rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_LESS:
		resulti = (lside < rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_GREATER:
		resulti = (lside > rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void float_vector_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	F32 lside = lscript_pop_float(buffer);
	LLVector3 rside;
	lscript_pop_vector(buffer, rside);

	switch(opcode)
	{
	case LOPC_MUL:
		rside *= lside;
		lscript_push(buffer, rside);
		break;
	default:
		break;
	}
}

void string_string_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti;
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		address = lsa_cat_strings(buffer, lside, rside, get_max_heap_size(buffer));
		lscript_push(buffer, address);
		break;
	case LOPC_EQ:
		resulti = !lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void string_key_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti;

	switch(opcode)
	{
	case LOPC_NEQ:
		resulti = lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_EQ:
		resulti = !lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void key_string_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti;

	switch(opcode)
	{
	case LOPC_NEQ:
		resulti = lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_EQ:
		resulti = !lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void key_key_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti;

	switch(opcode)
	{
	case LOPC_EQ:
		resulti = !lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = lsa_cmp_strings(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void vector_integer_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	S32 rside = lscript_pop_int(buffer);

	switch(opcode)
	{
	case LOPC_MUL:
		lside *= (F32)rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_DIV:
		if (rside)
			lside *= (1.f/rside);
		else
			set_fault(buffer, LSRF_MATH);
		lscript_push(buffer, lside);
		break;
	default:
		break;
	}
}

void vector_float_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	F32 rside = lscript_pop_float(buffer);

	switch(opcode)
	{
	case LOPC_MUL:
		lside *= rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_DIV:
		if (rside)
			lside *= (1.f/rside);
		else
			set_fault(buffer, LSRF_MATH);
		lscript_push(buffer, lside);
		break;
	default:
		break;
	}
}

void vector_vector_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	LLVector3 rside;
	lscript_pop_vector(buffer, rside);
	S32 resulti = 0;
	F32 resultf = 0.f;

	switch(opcode)
	{
	case LOPC_ADD:
		lside += rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_SUB:
		lside -= rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_MUL:
		resultf = lside * rside;
		lscript_push(buffer, resultf);
		break;
	case LOPC_MOD:
		lside = lside % rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_EQ:
		resulti = (lside == rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = (lside != rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void vector_quaternion_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	LLQuaternion rside;
	lscript_pop_quaternion(buffer, rside);

	switch(opcode)
	{
	case LOPC_MUL:
		lside = lside * rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_DIV:
		lside = lside * rside.conjQuat();
		lscript_push(buffer, lside);
		break;
	default:
		break;
	}
}

void quaternion_quaternion_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLQuaternion lside;
	lscript_pop_quaternion(buffer, lside);
	LLQuaternion rside;
	lscript_pop_quaternion(buffer, rside);
	S32 resulti = 0;

	switch(opcode)
	{
	case LOPC_ADD:
		lside = lside + rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_SUB:
		lside = lside - rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_MUL:
		lside *= rside;
		lscript_push(buffer, lside);
		break;
	case LOPC_DIV:
		lside = lside * rside.conjQuat();
		lscript_push(buffer, lside);
		break;
	case LOPC_EQ:
		resulti = (lside == rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = (lside != rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

void integer_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(lside);
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void float_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	F32 lside = lscript_pop_float(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(lside);
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void string_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *string = lsa_get_data(buffer, lside, TRUE);
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = string;
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void key_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *key = lsa_get_data(buffer, lside, TRUE);
			// need to convert key to key, since it comes out like a string
			if (key->mType == LST_STRING)
			{
				key->mKey = key->mString;
				key->mString = NULL;
				key->mType = LST_KEY;
			}
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = key;
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void vector_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(lside);
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void quaternion_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLQuaternion lside;
	lscript_pop_quaternion(buffer, lside);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(lside);
			address = lsa_preadd_lists(buffer, list, rside, get_max_heap_size(buffer));
			lscript_push(buffer, address);
			list->mListp = NULL;
			delete list;
		}
		break;
	default:
		break;
	}
}

void list_integer_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(rside);
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_float_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	F32 rside = lscript_pop_float(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(rside);
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_string_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *string = lsa_get_data(buffer, rside, TRUE);
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = string;
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_key_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *key = lsa_get_data(buffer, rside, TRUE);
			// need to convert key to key, since it comes out like a string
			if (key->mType == LST_STRING)
			{
				key->mKey = key->mString;
				key->mString = NULL;
				key->mType = LST_KEY;
			}
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = key;
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_vector_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	LLVector3 rside;
	lscript_pop_vector(buffer, rside);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(rside);
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_quaternion_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	LLQuaternion rside;
	lscript_pop_quaternion(buffer, rside);
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		{
			LLScriptLibData *list = new LLScriptLibData;
			list->mType = LST_LIST;
			list->mListp = new LLScriptLibData(rside);
			address = lsa_postadd_lists(buffer, lside, list, get_max_heap_size(buffer));
			list->mListp = NULL;
			delete list;
			lscript_push(buffer, address);
		}
		break;
	default:
		break;
	}
}

void list_list_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 rside = lscript_pop_int(buffer);
	S32 resulti;
	S32 address;

	switch(opcode)
	{
	case LOPC_ADD:
		address = lsa_cat_lists(buffer, lside, rside, get_max_heap_size(buffer));
		lscript_push(buffer, address);
		break;
	case LOPC_EQ:
		resulti = !lsa_cmp_lists(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	case LOPC_NEQ:
		resulti = lsa_cmp_lists(buffer, lside, rside);
		lscript_push(buffer, resulti);
		break;
	default:
		break;
	}
}

static U8 safe_op_index(U8 index)
{
	if(index >= LST_EOF)
	{
		// Operations on LST_NULL will always be unknown_operation.
		index = LST_NULL;
	}
	return index;
}

BOOL run_add(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tADD ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_ADD);
	return FALSE;
}

BOOL run_sub(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSUB ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_SUB);
	return FALSE;
}
BOOL run_mul(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tMUL ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_MUL);
	return FALSE;
}
BOOL run_div(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tDIV ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_DIV);
	return FALSE;
}
BOOL run_mod(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tMOD ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_MOD);
	return FALSE;
}

BOOL run_eq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tEQ ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_EQ);
	return FALSE;
}
BOOL run_neq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tNEQ ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_NEQ);
	return FALSE;
}
BOOL run_leq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tLEQ ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_LEQ);
	return FALSE;
}
BOOL run_geq(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tGEQ ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_GEQ);
	return FALSE;
}
BOOL run_less(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tLESS ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_LESS);
	return FALSE;
}
BOOL run_greater(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tGREATER ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 arg1 = safe_op_index(arg >> 4);
	U8 arg2 = safe_op_index(arg & 0xf);
	if (b_print)
	{
		print_type(arg1);
		printf(", ");
		print_type(arg2);
		printf("\n");
	}
	binary_operations[arg1][arg2](buffer, LOPC_GREATER);
	return FALSE;
}

BOOL run_bitand(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBITAND\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_BITAND);
	return FALSE;
}
BOOL run_bitor(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBITOR\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_BITOR);
	return FALSE;
}
BOOL run_bitxor(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBITXOR\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_BITXOR);
	return FALSE;
}
BOOL run_booland(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBOOLAND\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_BOOLAND);
	return FALSE;
}
BOOL run_boolor(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBOOLOR\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_BOOLOR);
	return FALSE;
}

BOOL run_shl(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSHL\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_SHL);
	return FALSE;
}
BOOL run_shr(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSHR\n", offset);
	offset++;
	binary_operations[LST_INTEGER][LST_INTEGER](buffer, LOPC_SHR);
	return FALSE;
}

void integer_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	S32 lside = lscript_pop_int(buffer);
	S32 result = 0;

	switch(opcode)
	{
	case LOPC_NEG:
		result = -lside;
		break;
	case LOPC_BITNOT:
		result = ~lside;
		break;
	case LOPC_BOOLNOT:
		result = !lside;
		break;
	default:
		break;
	}
	lscript_push(buffer, result);
}

void float_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	F32 lside = lscript_pop_float(buffer);
	F32 result = 0;

	switch(opcode)
	{
	case LOPC_NEG:
		result = -lside;
		lscript_push(buffer, result);
		break;
	default:
		break;
	}
}

void vector_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLVector3 lside;
	lscript_pop_vector(buffer, lside);
	LLVector3 result;

	switch(opcode)
	{
	case LOPC_NEG:
		result = -lside;
		lscript_push(buffer, result);
		break;
	default:
		break;
	}
}

void quaternion_operation(U8 *buffer, LSCRIPTOpCodesEnum opcode)
{
	LLQuaternion lside;
	lscript_pop_quaternion(buffer, lside);
	LLQuaternion result;

	switch(opcode)
	{
	case LOPC_NEG:
		result = -lside;
		lscript_push(buffer, result);
		break;
	default:
		break;
	}
}

BOOL run_neg(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tNEG ", offset);
	offset++;
	U8 arg = safe_op_index(safe_instruction_bytestream2byte(buffer, offset));
	if (b_print)
	{
		print_type(arg);
		printf("\n");
	}
	unary_operations[arg](buffer, LOPC_NEG);
	return FALSE;
}

BOOL run_bitnot(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBITNOT\n", offset);
	offset++;
	unary_operations[LST_INTEGER](buffer, LOPC_BITNOT);
	return FALSE;
}

BOOL run_boolnot(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tBOOLNOT\n", offset);
	offset++;
	unary_operations[LST_INTEGER](buffer, LOPC_BOOLNOT);
	return FALSE;
}

BOOL run_jump(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tJUMP ", offset);
	offset++;
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);
	offset += arg;
	return FALSE;
}

BOOL run_jumpif(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tJUMPIF ", offset);
	offset++;
	U8 type = safe_instruction_bytestream2byte(buffer, offset);
	if (b_print)
	{
		print_type(type);
		printf(", ");
	}
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);

	if (type == LST_INTEGER)
	{
		S32 test = lscript_pop_int(buffer);
		if (test)
		{
			offset += arg;
		}
	}
	else if (type == LST_FLOATINGPOINT)
	{
		F32 test = lscript_pop_float(buffer);
		if (test)
		{
			offset += arg;
		}
	}
	else if (type == LST_VECTOR)
	{
		LLVector3 test;
		lscript_pop_vector(buffer, test);
		if (!test.isExactlyZero())
		{
			offset += arg;
		}
	}
	else if (type == LST_QUATERNION)
	{
		LLQuaternion test;
		lscript_pop_quaternion(buffer, test);
		if (!test.isIdentity())
		{
			offset += arg;
		}
	}
	else if (type == LST_STRING)
	{
		S32 base_address = lscript_pop_int(buffer);
		// this bit of nastiness is to get around that code paths to
		// local variables can result in lack of initialization and
		// function clean up of ref counts isn't based on scope (a
		// mistake, I know)
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			S32 string = address;
			string += SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				char *sdata = new char[size];
				bytestream2char(sdata, buffer, string, size);
				if (strlen(sdata))		/*Flawfinder: ignore*/
				{
					offset += arg;
				}
				delete [] sdata;
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
	}
	else if (type == LST_KEY)
	{
		S32 base_address = lscript_pop_int(buffer);
		// this bit of nastiness is to get around that code paths to
		// local variables can result in lack of initialization and
		// function clean up of ref counts isn't based on scope (a
		// mistake, I know)
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			S32 string = address;
			string += SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				char *sdata = new char[size];
				bytestream2char(sdata, buffer, string, size);
				if (strlen(sdata))		/*Flawfinder: ignore*/
				{
					LLUUID id;
					if (id.set(sdata) && id.notNull())
						offset += arg;
				}
				delete [] sdata;
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
	}
	else if (type == LST_LIST)
	{
		S32 base_address = lscript_pop_int(buffer);
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			if (safe_heap_check_address(buffer, address + SIZEOF_SCRIPT_ALLOC_ENTRY, 1))
			{
				LLScriptLibData *list = lsa_get_list_ptr(buffer, base_address, TRUE);
				if (list && list->getListLength())
				{
					offset += arg;
				}
				delete list;
			}
		}
	}
	return FALSE;
}

BOOL run_jumpnif(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tJUMPNIF ", offset);
	offset++;
	U8 type = safe_instruction_bytestream2byte(buffer, offset);
	if (b_print)
	{
		print_type(type);
		printf(", ");
	}
	S32 arg = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", arg);

	if (type == LST_INTEGER)
	{
		S32 test = lscript_pop_int(buffer);
		if (!test)
		{
			offset += arg;
		}
	}
	else if (type == LST_FLOATINGPOINT)
	{
		F32 test = lscript_pop_float(buffer);
		if (!test)
		{
			offset += arg;
		}
	}
	else if (type == LST_VECTOR)
	{
		LLVector3 test;
		lscript_pop_vector(buffer, test);
		if (test.isExactlyZero())
		{
			offset += arg;
		}
	}
	else if (type == LST_QUATERNION)
	{
		LLQuaternion test;
		lscript_pop_quaternion(buffer, test);
		if (test.isIdentity())
		{
			offset += arg;
		}
	}
	else if (type == LST_STRING)
	{
		S32 base_address = lscript_pop_int(buffer);
		// this bit of nastiness is to get around that code paths to
		// local variables can result in lack of initialization and
		// function clean up of ref counts isn't based on scope (a
		// mistake, I know)
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			S32 string = address;
			string += SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				char *sdata = new char[size];
				bytestream2char(sdata, buffer, string, size);
				if (!strlen(sdata))		/*Flawfinder: ignore*/
				{
					offset += arg;
				}
				delete [] sdata;
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
	}
	else if (type == LST_KEY)
	{
		S32 base_address = lscript_pop_int(buffer);
		// this bit of nastiness is to get around that code paths to
		// local variables can result in lack of initialization and
		// function clean up of ref counts isn't based on scope (a
		// mistake, I know)
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			S32 string = address;
			string += SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				char *sdata = new char[size];
				bytestream2char(sdata, buffer, string, size);
				if (strlen(sdata))		/*Flawfinder: ignore*/
				{
					LLUUID id;
					if (!id.set(sdata) || id.isNull())
						offset += arg;
				}
				else
				{
					offset += arg;
				}
				delete [] sdata;
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
	}
	else if (type == LST_LIST)
	{
		S32 base_address = lscript_pop_int(buffer);
		// this bit of nastiness is to get around that code paths to
		// local variables can result in lack of initialization and
		// function clean up of ref counts isn't based on scope (a
		// mistake, I know)
		S32 address = base_address + get_register(buffer, LREG_HR) - 1;
		if (address)
		{
			if (safe_heap_check_address(buffer, address + SIZEOF_SCRIPT_ALLOC_ENTRY, 1))
			{
				LLScriptLibData *list = lsa_get_list_ptr(buffer, base_address, TRUE);
				if (!list || !list->getListLength())
				{
					offset += arg;
				}
				delete list;
			}
		}
	}
	return FALSE;
}

BOOL run_state(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tSTATE ", offset);
	offset++;
	S32 state = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", state);

	S32 bp = lscript_pop_int(buffer);
	set_bp(buffer, bp);

	offset = lscript_pop_int(buffer);

	S32 major_version = 0;
	S32 value = get_register(buffer, LREG_VN);
	if (value == LSL2_VERSION1_END_NUMBER)
	{
		major_version = 1;
	}
	else if (value == LSL2_VERSION_NUMBER)
	{
		major_version = 2;
	}
					
	S32 current_state = get_register(buffer, LREG_CS);
	if (state != current_state)
	{
		U64 ce = get_event_register(buffer, LREG_CE, major_version);
		ce |= LSCRIPTStateBitField[LSTT_STATE_EXIT];
		set_event_register(buffer, LREG_CE, ce, major_version);
	}
	set_register(buffer, LREG_NS, state);
	return FALSE;
}

BOOL run_call(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tCALL ", offset);
	offset++;
	S32 func = safe_instruction_bytestream2integer(buffer, offset);
	if (b_print)
		printf("%d\n", func);

	lscript_local_store(buffer, -8, offset);

	S32 minimum = get_register(buffer, LREG_GFR);
	S32 maximum = get_register(buffer, LREG_SR);
	S32 lookup = minimum + func*4 + 4;
	S32 function;

	if (  (lookup >= minimum)
		&&(lookup < maximum))
	{
		function = bytestream2integer(buffer, lookup) + minimum;
		if (  (lookup >= minimum)
			&&(lookup < maximum))
		{
			offset = function;
			offset += bytestream2integer(buffer, function);
		}
		else
		{
			set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		}
	}
	else
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
	}
	return FALSE;
}

BOOL run_return(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tRETURN\n", offset);
	offset++;
	
	// SEC-53: babbage: broken instructions may allow inbalanced pushes and
	// pops which can cause caller BP and return IP to be corrupted, so restore
	// SP from BP before popping caller BP and IP.
	S32 bp = get_register(buffer, LREG_BP);
	set_sp(buffer, bp);
	
	bp = lscript_pop_int(buffer);
	set_bp(buffer, bp);
	offset = lscript_pop_int(buffer);
	return FALSE;
}



BOOL run_cast(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	char caststr[1024];		/*Flawfinder: ignore*/
	if (b_print)
		printf("[0x%X]\tCAST ", offset);
	offset++;
	U8 arg = safe_instruction_bytestream2byte(buffer, offset);
	U8 from = arg >> 4;
	U8 to = arg & 0xf;
	if (b_print)
	{
		print_type(from);
		printf(", ");
		print_type(to);
		printf("\n");
	}

	switch(from)
	{
	case LST_INTEGER:
		{
			switch(to)
			{
			case LST_INTEGER:
				break;
			case LST_FLOATINGPOINT:
				{
					S32 source = lscript_pop_int(buffer);
					F32 dest = (F32)source;
					lscript_push(buffer, dest);
				}
				break;
			case LST_STRING:
				{
					S32 address, source = lscript_pop_int(buffer);
					snprintf(caststr, sizeof(caststr), "%d", source);		/* Flawfinder: ignore */
					address = lsa_heap_add_data(buffer, new LLScriptLibData(caststr), get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			case LST_LIST:
				{
					S32 address, source = lscript_pop_int(buffer);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = new LLScriptLibData(source);
					address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_FLOATINGPOINT:
		{
			switch(to)
			{
			case LST_INTEGER:
				{
					F32 source = lscript_pop_float(buffer);
					S32 dest = (S32)source;
					lscript_push(buffer, dest);
				}
				break;
			case LST_FLOATINGPOINT:
				break;
			case LST_STRING:
				{
					S32 address;
					F32 source = lscript_pop_float(buffer);
					snprintf(caststr, sizeof(caststr), "%f", source);		/* Flawfinder: ignore */
					address = lsa_heap_add_data(buffer, new LLScriptLibData(caststr), get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			case LST_LIST:
				{
					S32 address;
					F32 source = lscript_pop_float(buffer);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = new LLScriptLibData(source);
					address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_STRING:
		{
			switch(to)
			{
			case LST_INTEGER:
				{
					S32 base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
					S32 address = base_address + get_register(buffer, LREG_HR) - 1;
					if (address)
					{
						S32 string = address;
						string += SIZEOF_SCRIPT_ALLOC_ENTRY;
						if (safe_heap_check_address(buffer, string, 1))
						{
							S32 toffset = string;
							safe_heap_bytestream_count_char(buffer, toffset);
							S32 size = toffset - string;
							char *arg = new char[size];
							bytestream2char(arg, buffer, string, size);
							// S32 length = strlen(arg);
							S32 dest;
							S32 base;

							// Check to see if this is a hexidecimal number.
							if (  (arg[0] == '0') &&
								  (arg[1] == 'x' || arg[1] == 'X') )
							{
								// Let strtoul do a hex conversion.
								base = 16;
							}
							else
							{
								// Force base-10, so octal is never used.
								base = 10;
							}

							dest = strtoul(arg, NULL, base);

							lscript_push(buffer, dest);
							delete [] arg;
						}
						lsa_decrease_ref_count(buffer, base_address);
					}
				}
				break;
			case LST_FLOATINGPOINT:
				{
					S32 base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
					S32 address = base_address + get_register(buffer, LREG_HR) - 1;
					if (address)
					{
						S32 string = address;
						string += SIZEOF_SCRIPT_ALLOC_ENTRY;
						if (safe_heap_check_address(buffer, string, 1))
						{
							S32 toffset = string;
							safe_heap_bytestream_count_char(buffer, toffset);
							S32 size = toffset - string;
							char *arg = new char[size];
							bytestream2char(arg, buffer, string, size);
							F32 dest = (F32)atof(arg);


							lscript_push(buffer, dest);
							delete [] arg;
						}
						lsa_decrease_ref_count(buffer, base_address);
					}
				}
				break;
			case LST_STRING:
				break;
			case LST_LIST:
				{
					S32 saddress = lscript_pop_int(buffer);
					LLScriptLibData *string = lsa_get_data(buffer, saddress, TRUE);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = string;
					S32 address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			case LST_VECTOR:
				{
					S32 base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
					S32 address = base_address + get_register(buffer, LREG_HR) - 1;
					if (address)
					{
						S32 string = address;
						string += SIZEOF_SCRIPT_ALLOC_ENTRY;
						if (safe_heap_check_address(buffer, string, 1))
						{
							S32 toffset = string;
							safe_heap_bytestream_count_char(buffer, toffset);
							S32 size = toffset - string;
							char *arg = new char[size];
							bytestream2char(arg, buffer, string, size);
							LLVector3 vec;
							S32 num = sscanf(arg, "<%f, %f, %f>", &vec.mV[VX], &vec.mV[VY], &vec.mV[VZ]);
							if (num != 3)
							{
								vec = LLVector3::zero;
							}
							lscript_push(buffer, vec);
							delete [] arg;
						}
						lsa_decrease_ref_count(buffer, base_address);
					}
				}
				break;
			case LST_QUATERNION:
				{
					S32 base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
					S32 address = base_address + get_register(buffer, LREG_HR) - 1;
					if (address)
					{
						S32 string = address;
						string += SIZEOF_SCRIPT_ALLOC_ENTRY;
						if (safe_heap_check_address(buffer, string, 1))
						{
							S32 toffset = string;
							safe_heap_bytestream_count_char(buffer, toffset);
							S32 size = toffset - string;
							char *arg = new char[size];
							bytestream2char(arg, buffer, string, size);
							LLQuaternion quat;
							S32 num = sscanf(arg, "<%f, %f, %f, %f>", &quat.mQ[VX], &quat.mQ[VY], &quat.mQ[VZ], &quat.mQ[VW]);
							if (num != 4)
							{
								quat = LLQuaternion::DEFAULT;

							}
							lscript_push(buffer, quat);
							delete [] arg;
						}
						lsa_decrease_ref_count(buffer, base_address);
					}
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_KEY:
		{
			switch(to)
			{
			case LST_KEY:
				break;
			case LST_STRING:
				break;
			case LST_LIST:
				{
					S32 saddress = lscript_pop_int(buffer);
					LLScriptLibData *string = lsa_get_data(buffer, saddress, TRUE);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = string;
					S32 address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_VECTOR:
		{
			switch(to)
			{
			case LST_VECTOR:
				break;
			case LST_STRING:
				{
					S32 address;
					LLVector3 source;
					lscript_pop_vector(buffer, source);
					snprintf(caststr, sizeof(caststr), "<%5.5f, %5.5f, %5.5f>", source.mV[VX], source.mV[VY], source.mV[VZ]);		/* Flawfinder: ignore */
					address = lsa_heap_add_data(buffer, new LLScriptLibData(caststr), get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			case LST_LIST:
				{
					S32 address;
					LLVector3 source;
					lscript_pop_vector(buffer, source);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = new LLScriptLibData(source);
					address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_QUATERNION:
		{
			switch(to)
			{
			case LST_QUATERNION:
				break;
			case LST_STRING:
				{
					S32 address;
					LLQuaternion source;
					lscript_pop_quaternion(buffer, source);
					snprintf(caststr, sizeof(caststr), "<%5.5f, %5.5f, %5.5f, %5.5f>", source.mQ[VX], source.mQ[VY], source.mQ[VZ], source.mQ[VS]);		/* Flawfinder: ignore */
					address = lsa_heap_add_data(buffer, new LLScriptLibData(caststr), get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			case LST_LIST:
				{
					S32 address;
					LLQuaternion source;
					lscript_pop_quaternion(buffer, source);
					LLScriptLibData *list = new LLScriptLibData;
					list->mType = LST_LIST;
					list->mListp = new LLScriptLibData(source);
					address = lsa_heap_add_data(buffer, list, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, address);
				}
				break;
			default:
				break;
			}
		}
		break;
	case LST_LIST:
		{
			switch(to)
			{
			case LST_LIST:
				break;
			case LST_STRING:
				{
					S32 address = lscript_pop_int(buffer);
					LLScriptLibData *list = lsa_get_data(buffer, address, TRUE);
					LLScriptLibData *list_root = list;

					std::ostringstream dest;
					while (list)
					{
						list->print(dest, FALSE);
						list = list->mListp;
					}
					delete list_root;
					char *tmp = strdup(dest.str().c_str());
					LLScriptLibData *string = new LLScriptLibData(tmp);
					free(tmp);
					tmp = NULL;
					S32 destaddress = lsa_heap_add_data(buffer, string, get_max_heap_size(buffer), TRUE);
					lscript_push(buffer, destaddress);
				}
				break;
			default:
				break;
			}
		}
		break;
		default:
			break;
	}
	return FALSE;
}

BOOL run_stacktos(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	offset++;
	S32 length = lscript_pop_int(buffer);
	S32 i;
	char *arg = new char[length];
	S32 fault;
	for (i = 0; i < length; i++)
	{
		fault = get_register(buffer, LREG_FR);
		if (fault)
			break;

		arg[length - i - 1] = lscript_pop_char(buffer);
	}
	S32 address = lsa_heap_add_data(buffer, new LLScriptLibData(arg), get_max_heap_size(buffer), TRUE);
	lscript_push(buffer, address);
	delete [] arg;
	return FALSE;
}

void lscript_stacktol_pop_variable(LLScriptLibData *data, U8 *buffer, char type)
{
	S32 address, string;
	S32 base_address;

	switch(type)
	{
	case LST_INTEGER:
		data->mType = LST_INTEGER;
		data->mInteger = lscript_pop_int(buffer);
		break;
	case LST_FLOATINGPOINT:
		data->mType = LST_FLOATINGPOINT;
		data->mFP = lscript_pop_float(buffer);
		break;
	case LST_KEY:
		data->mType = LST_KEY;

		base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
		address = base_address + get_register(buffer, LREG_HR) - 1;

		if (address)
		{
			string = address + SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				data->mKey = new char[size];
				bytestream2char(data->mKey, buffer, string, size);
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
		else
		{
			data->mKey = new char[1];
			data->mKey[0] = 0;
		}
		break;
	case LST_STRING:
		data->mType = LST_STRING;

		base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
		address = base_address + get_register(buffer, LREG_HR) - 1;

		if (address)
		{
			string = address + SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				data->mString = new char[size];
				bytestream2char(data->mString, buffer, string, size);
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
		else
		{
			data->mString = new char[1];
			data->mString[0] = 0;
		}
		break;
	case LST_VECTOR:
		data->mType = LST_VECTOR;
		lscript_pop_vector(buffer, data->mVec);
		break;
	case LST_QUATERNION:
		data->mType = LST_QUATERNION;
		lscript_pop_quaternion(buffer, data->mQuat);
		break;
	case LST_LIST:
		data->mType = LST_LIST;
		address = lscript_pop_int(buffer);
		data->mListp = lsa_get_data(buffer, address, TRUE);
		break;
	}
}

BOOL run_stacktol(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	offset++;
	S32 length = safe_instruction_bytestream2integer(buffer, offset);
	S32 i;
	S32 fault;

	S8 type;

	LLScriptLibData *data = new LLScriptLibData, *tail;
	data->mType = LST_LIST;

	for (i = 0; i < length; i++)
	{
		fault = get_register(buffer, LREG_FR);
		if (fault)
			break;

		type = lscript_pop_char(buffer);

		tail = new LLScriptLibData;

		lscript_stacktol_pop_variable(tail, buffer, type);

		tail->mListp = data->mListp;
		data->mListp = tail;
	}
	S32 address = lsa_heap_add_data(buffer,data, get_max_heap_size(buffer), TRUE);
	lscript_push(buffer, address);
	return FALSE;
}

BOOL run_print(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	if (b_print)
		printf("[0x%X]\tPRINT ", offset);
	offset++;
	U8 type = safe_instruction_bytestream2byte(buffer, offset);
	if (b_print)
	{
		print_type(type);
		printf("\n");
	}
	switch(type)
	{
	case LST_INTEGER:
		{
			S32 source = lscript_pop_int(buffer);
			printf("%d\n", source);
		}
		break;
	case LST_FLOATINGPOINT:
		{
			F32 source = lscript_pop_float(buffer);
			printf("%f\n", source);
		}
		break;
	case LST_STRING:
		{
			S32 base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
			S32	address = base_address + get_register(buffer, LREG_HR) - 1;

			if (address)
			{
				S32 string = address;
				string += SIZEOF_SCRIPT_ALLOC_ENTRY;
				if (safe_heap_check_address(buffer, string, 1))
				{
					S32 toffset = string;
					safe_heap_bytestream_count_char(buffer, toffset);
					S32 size = toffset - string;
					char *arg = new char[size];
					bytestream2char(arg, buffer, string, size);
					printf("%s\n", arg);
					delete [] arg;
				}
				lsa_decrease_ref_count(buffer, base_address);
			}
		}
		break;
	case LST_VECTOR:
		{
			LLVector3 source;
			lscript_pop_vector(buffer, source);
			printf("< %f, %f, %f >\n", source.mV[VX], source.mV[VY], source.mV[VZ]);
		}
		break;
	case LST_QUATERNION:
		{
			LLQuaternion source;
			lscript_pop_quaternion(buffer, source);
			printf("< %f, %f, %f, %f >\n", source.mQ[VX], source.mQ[VY], source.mQ[VZ], source.mQ[VS]);
		}
		break;
	case LST_LIST:
		{
			S32 base_address = lscript_pop_int(buffer);
			LLScriptLibData *data = lsa_get_data(buffer, base_address, TRUE);
			LLScriptLibData *print = data;

			printf("list\n");

			while (print)
			{
				switch(print->mType)
				{
				case LST_INTEGER:
					{
						printf("%d\n", print->mInteger);
					}
					break;
				case LST_FLOATINGPOINT:
					{
						printf("%f\n", print->mFP);
					}
					break;
				case LST_STRING:
					{
						printf("%s\n", print->mString);
					}
					break;
				case LST_KEY:
					{
						printf("%s\n", print->mKey);
					}
					break;
				case LST_VECTOR:
					{
						printf("< %f, %f, %f >\n", print->mVec.mV[VX], print->mVec.mV[VY], print->mVec.mV[VZ]);
					}
					break;
				case LST_QUATERNION:
					{
						printf("< %f, %f, %f, %f >\n", print->mQuat.mQ[VX], print->mQuat.mQ[VY], print->mQuat.mQ[VZ], print->mQuat.mQ[VS]);
					}
					break;
				default:
					break;
				}
				print = print->mListp;
			}
			delete data;
		}
		break;
	default:
		break;
	}
	return FALSE;
}


void lscript_run(const std::string& filename, BOOL b_debug)
{
	LLTimer	timer;

	const char *error;
	LLScriptExecuteLSL2 *execute = NULL;

	if (filename.empty())
	{
		llerrs << "filename is NULL" << llendl;
		// Just reporting error is likely not enough. Need
		// to check how to abort or error out gracefully
		// from this function. XXXTBD
	}
	LLFILE* file = LLFile::fopen(filename, "r");  /* Flawfinder: ignore */
	if(file)
	{
		execute = new LLScriptExecuteLSL2(file);
		fclose(file);
	}
	if (execute)
	{
		timer.reset();
		F32 time_slice = 3600.0f; // 1 hr.
		U32 events_processed = 0;

		do {
			LLTimer timer2;
			execute->runQuanta(b_debug, LLUUID::null, &error,
							   time_slice, events_processed, timer2);
		} while (!execute->isFinished());

		F32 time = timer.getElapsedTimeF32();
		F32 ips = execute->mInstructionCount / time;
		llinfos << execute->mInstructionCount << " instructions in " << time << " seconds" << llendl;
		llinfos << ips/1000 << "K instructions per second" << llendl;
		printf("ip: 0x%X\n", get_register(execute->mBuffer, LREG_IP));
		printf("sp: 0x%X\n", get_register(execute->mBuffer, LREG_SP));
		printf("bp: 0x%X\n", get_register(execute->mBuffer, LREG_BP));
		printf("hr: 0x%X\n", get_register(execute->mBuffer, LREG_HR));
		printf("hp: 0x%X\n", get_register(execute->mBuffer, LREG_HP));
		delete execute;
		fclose(file);
	}
}

void lscript_pop_variable(LLScriptLibData *data, U8 *buffer, char type)
{
	S32 address, string;
	S32 base_address;

	switch(type)
	{
	case 'i':
		data->mType = LST_INTEGER;
		data->mInteger = lscript_pop_int(buffer);
		break;
	case 'f':
		data->mType = LST_FLOATINGPOINT;
		data->mFP = lscript_pop_float(buffer);
		break;
	case 'k':
		data->mType = LST_KEY;
		data->mKey = NULL;
		
		base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
		address = base_address + get_register(buffer, LREG_HR) - 1;

		if (address)
		{
			string = address + SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				data->mKey = new char[size];
				bytestream2char(data->mKey, buffer, string, size);
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
		if (data->mKey == NULL)
		{
			data->mKey = new char[1];
			data->mKey[0] = 0;
		}
		break;
	case 's':
		data->mType = LST_STRING;
		data->mString = NULL;

		base_address = lscript_pop_int(buffer);
	// this bit of nastiness is to get around that code paths to local variables can result in lack of initialization
	// and function clean up of ref counts isn't based on scope (a mistake, I know)
		address = base_address + get_register(buffer, LREG_HR) - 1;

		if (address)
		{
			string = address + SIZEOF_SCRIPT_ALLOC_ENTRY;
			if (safe_heap_check_address(buffer, string, 1))
			{
				S32 toffset = string;
				safe_heap_bytestream_count_char(buffer, toffset);
				S32 size = toffset - string;
				data->mString = new char[size];
				bytestream2char(data->mString, buffer, string, size);
			}
			lsa_decrease_ref_count(buffer, base_address);
		}
		if (data->mString == NULL)
		{
			data->mString = new char[1];
			data->mString[0] = 0;
		}
		break;
	case 'l':
		{
			S32 base_address = lscript_pop_int(buffer);
			data->mType = LST_LIST;
			data->mListp = lsa_get_list_ptr(buffer, base_address, TRUE);
		}
		break;
	case 'v':
		data->mType = LST_VECTOR;
		lscript_pop_vector(buffer, data->mVec);
		break;
	case 'q':
		data->mType = LST_QUATERNION;
		lscript_pop_quaternion(buffer, data->mQuat);
		break;
	}
}

void lscript_push_return_variable(LLScriptLibData *data, U8 *buffer)
{
	S32 address;
	switch(data->mType)
	{
	case LST_INTEGER:
		lscript_local_store(buffer, -12, data->mInteger);
		break;
	case LST_FLOATINGPOINT:
		lscript_local_store(buffer, -12, data->mFP);
		break;
	case LST_KEY:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_local_store(buffer, -12, address);
		break;
	case LST_STRING:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_local_store(buffer, -12, address);
		break;
	case LST_LIST:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_local_store(buffer, -12, address);
		break;
	case LST_VECTOR:
		lscript_local_store(buffer, -20, data->mVec);
		break;
	case LST_QUATERNION:
		lscript_local_store(buffer, -24, data->mQuat);
		break;
	default:
		break;
	}
}

S32 lscript_push_variable(LLScriptLibData *data, U8 *buffer)
{
	S32 address;
	switch(data->mType)
	{
	case LST_INTEGER:
		lscript_push(buffer, data->mInteger);
		break;
	case LST_FLOATINGPOINT:
		lscript_push(buffer, data->mFP);
		return 4;
		break;
	case LST_KEY:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_push(buffer, address);
		return 4;
		break;
	case LST_STRING:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_push(buffer, address);
		return 4;
		break;
	case LST_LIST:
		address = lsa_heap_add_data(buffer, data, get_max_heap_size(buffer), FALSE);
		lscript_push(buffer, address);
		return 4;
		break;
	case LST_VECTOR:
		lscript_push(buffer, data->mVec);
		return 12;
		break;
	case LST_QUATERNION:
		lscript_push(buffer, data->mQuat);
		return 16;
		break;
	default:
		break;
	}
	return 4;
}


// Shared code for run_calllib() and run_calllib_two_byte()
BOOL run_calllib_common(U8 *buffer, S32 &offset, const LLUUID &id, U16 arg)
{
	if (arg >= gScriptLibrary.mFunctions.size())
	{
		set_fault(buffer, LSRF_BOUND_CHECK_ERROR);
		return FALSE;
	}
	LLScriptLibraryFunction const & function = gScriptLibrary.mFunctions[arg];

	// pull out the arguments and the return values
	LLScriptLibData	*arguments = NULL;
	LLScriptLibData	*returnvalue = NULL;

	S32 i, number;

	if (function.mReturnType)
	{
		returnvalue = new LLScriptLibData;
	}

	if (function.mArgs)
	{
		number = (S32)strlen(function.mArgs);		//Flawfinder: ignore
		arguments = new LLScriptLibData[number];
	}
	else
	{
		number = 0;
	}

	for (i = number - 1; i >= 0; i--)
	{
		lscript_pop_variable(&arguments[i], buffer, function.mArgs[i]);
	}

	// Actually execute the function call
	function.mExecFunc(returnvalue, arguments, id);

	add_register_fp(buffer, LREG_ESR, -(function.mEnergyUse));
	add_register_fp(buffer, LREG_SLR, function.mSleepTime);

	if (returnvalue)
	{
		returnvalue->mType = char2type(*function.mReturnType);
		lscript_push_return_variable(returnvalue, buffer);
	}

	delete [] arguments;
	delete returnvalue;

	// reset the BP after calling the library files
	S32 bp = lscript_pop_int(buffer);
	set_bp(buffer, bp);

	// pop off the spot for the instruction pointer
	lscript_poparg(buffer, 4);
	return FALSE;
}


BOOL run_calllib(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	offset++;
	U16 arg = (U16) safe_instruction_bytestream2byte(buffer, offset);
	if (b_print &&
		arg < gScriptLibrary.mFunctions.size())
	{
		printf("[0x%X]\tCALLLIB ", offset);
		LLScriptLibraryFunction const & function = gScriptLibrary.mFunctions[arg];
		printf("%d (%s)\n", (U32)arg, function.mName);
		//printf("%s\n", function.mDesc);
	}
	return run_calllib_common(buffer, offset, id, arg);
}

BOOL run_calllib_two_byte(U8 *buffer, S32 &offset, BOOL b_print, const LLUUID &id)
{
	offset++;
	U16 arg = safe_instruction_bytestream2u16(buffer, offset);
	if (b_print &&
		arg < gScriptLibrary.mFunctions.size())
	{
		printf("[0x%X]\tCALLLIB ", (offset-1));
		LLScriptLibraryFunction const & function = gScriptLibrary.mFunctions[arg];
		printf("%d (%s)\n", (U32)arg, function.mName);
		//printf("%s\n", function.mDesc);
	}
	return run_calllib_common(buffer, offset, id, arg);
}
