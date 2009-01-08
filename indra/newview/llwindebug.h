/** 
 * @file llwindebug.h
 * @brief LLWinDebug class header file
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLWINDEBUG_H
#define LL_LLWINDEBUG_H

#include "stdtypes.h"
#include <dbghelp.h>

class LLWinDebug
{
public:

	/** 
	* @brief initialize the llwindebug exception filter callback
	* 
	* Hand a windows unhandled exception filter to LLWinDebug
	* This method should only be called to change the
	* exception filter used by llwindebug.
	*
	* Setting filter_func to NULL will clear any custom filters.
	**/
	static void initExceptionHandler(LPTOP_LEVEL_EXCEPTION_FILTER filter_func);

	/** 
	* @brief check the status of the exception filter.
	*
	* Resets unhandled exception filter to the filter specified 
	* w/ initExceptionFilter). 
	* Returns false if the exception filter was modified.
	*
	* *NOTE:Mani In the past mozlib has been accused of
	* overriding the exception filter. If the mozlib filter 
	* is required, perhaps we can chain calls from our 
	* filter to mozlib's.
	**/
	static bool checkExceptionHandler();

	static void generateCrashStacks(struct _EXCEPTION_POINTERS *pExceptionInfo = NULL);
	static void clearCrashStacks(); // Delete the crash stack file(s).

	static void writeDumpToFile(MINIDUMP_TYPE type, MINIDUMP_EXCEPTION_INFORMATION *ExInfop, const std::string& filename);
private:
};

#endif // LL_LLWINDEBUG_H
