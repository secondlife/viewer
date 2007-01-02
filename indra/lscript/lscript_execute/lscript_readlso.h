/** 
 * @file lscript_readlso.h
 * @brief classes to read lso file
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LSCRIPT_READLSO_H
#define LL_LSCRIPT_READLSO_H

#include "lscript_byteconvert.h"
#include "linked_lists.h"

// list of op code print functions
void print_noop(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pop(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pops(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_poparg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popip(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popbp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_popslr(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_dup(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dups(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_dupq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_store(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stores(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storel(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storev(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storeq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storeg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_storegq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadlp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadvp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadqp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadglp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgvp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_loadgqp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_push(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushgq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_puship(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushbp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushsp(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargb(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargi(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargf(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargs(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargv(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushargq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushe(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pushev(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pusheq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_pusharge(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_add(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_sub(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_mul(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_div(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_mod(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_eq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_neq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_leq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_geq(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_less(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_greater(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_bitand(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitxor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_booland(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_boolor(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_shl(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_shr(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_neg(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_bitnot(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_boolnot(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_jump(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_jumpif(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_jumpnif(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_state(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_call(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_return(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_cast(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stacktos(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_stacktol(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_print(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

void print_calllib(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
void print_calllib_two_byte(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);

class LLScriptLSOParse
{
public:
	LLScriptLSOParse(FILE *fp);
	LLScriptLSOParse(U8 *buffer);
	~LLScriptLSOParse();

	void initOpCodePrinting();

	void printData(FILE *fp);
	void printNameDesc(FILE *fp);
	void printRegisters(FILE *fp);
	void printGlobals(FILE *fp);
	void printGlobalFunctions(FILE *fp);
	void printStates(FILE *fp);
	void printHeap(FILE *fp);
	void printOpCodes(FILE *fp, S32 &offset, S32 tabs);
	void printOpCodeRange(FILE *fp, S32 start, S32 end, S32 tabs);

	U8	*mRawData;
	void (*mPrintOpCodes[0x100])(FILE *fp, U8 *buffer, S32 &offset, S32 tabs);
};


void lso_print_tabs(FILE *fp, S32 tabs);

#endif
