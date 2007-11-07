/** 
 * @file llviewernetwork.h
 * @author James Cook
 * @brief Networking constants and globals for viewer.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLVIEWERNETWORK_H
#define LL_LLVIEWERNETWORK_H

class LLHost;

enum EGridInfo
{
	GRID_INFO_NONE,
	GRID_INFO_ADITI,
	GRID_INFO_AGNI,
	GRID_INFO_DMZ,
	GRID_INFO_SIVA,
	GRID_INFO_DURGA,
	GRID_INFO_SHAKTI,
	GRID_INFO_SOMA,
	GRID_INFO_GANGA,
	GRID_INFO_VAAK,
	GRID_INFO_UMA,
	GRID_INFO_LOCAL,
	GRID_INFO_OTHER, // IP address set via -user or other command line option
	GRID_INFO_COUNT
};


struct LLGridData
{
	const char* mLabel;
	const char* mName;
	const char* mLoginURI;
	const char* mHelperURI;
};

extern F32 gPacketDropPercentage;
extern F32 gInBandwidth;
extern F32 gOutBandwidth;
extern EGridInfo gGridChoice;
extern LLGridData gGridInfo[];
extern char gGridName[MAX_STRING];		/* Flawfinder: ignore */

const S32 MAC_ADDRESS_BYTES = 6;
extern unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */

#endif
