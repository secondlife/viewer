/** 
 * @file lscript_rt_interface.h
 * @brief Interface between compiler library and applications
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LSCRIPT_RT_INTERFACE_H
#define LL_LSCRIPT_RT_INTERFACE_H

BOOL lscript_compile(char *filename, BOOL is_god_like = FALSE);
BOOL lscript_compile(const char* src_filename, const char* dst_filename,
					 const char* err_filename, BOOL is_god_like = FALSE);
void lscript_run(char *filename, BOOL b_debug);


#endif
