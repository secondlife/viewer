/** 
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
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

#include "llviewerprecompiledheaders.h"

#include "llviewernetwork.h"

LLGridData gGridInfo[GRID_INFO_COUNT] = 
{
	{ "None", "", "", ""},
	{ "Aditi", 
	  "util.aditi.lindenlab.com", 
	  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
	  "http://aditi-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Agni", 
	  "util.agni.lindenlab.com", 
	  "https://login.agni.lindenlab.com/cgi-bin/login.cgi",
	  "https://secondlife.com/helpers/" },
	{ "Aruna",
	  "util.aruna.lindenlab.com",
	  "https://login.aruna.lindenlab.com/cgi-bin/login.cgi",
	  "http://aruna-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Durga",
	  "util.durga.lindenlab.com",
	  "https://login.durga.lindenlab.com/cgi-bin/login.cgi",
	  "http://durga-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Ganga",
	  "util.ganga.lindenlab.com",
	  "https://login.ganga.lindenlab.com/cgi-bin/login.cgi",
	  "http://ganga-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Mitra",
	  "util.mitra.lindenlab.com",
	  "https://login.mitra.lindenlab.com/cgi-bin/login.cgi",
	  "http://mitra-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Mohini",
	  "util.mohini.lindenlab.com",
	  "https://login.mohini.lindenlab.com/cgi-bin/login.cgi",
	  "http://mohini-secondlife.webdev.lindenlab.com/helpers/" },
  	{ "Nandi",
	  "util.nandi.lindenlab.com",
	  "https://login.nandi.lindenlab.com/cgi-bin/login.cgi",
	  "http://nandi-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Radha",
	  "util.radha.lindenlab.com",
	  "https://login.radha.lindenlab.com/cgi-bin/login.cgi",
	  "http://radha-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Ravi",
	  "util.ravi.lindenlab.com",
	  "https://login.ravi.lindenlab.com/cgi-bin/login.cgi",
	  "http://ravi-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Siva", 
	  "util.siva.lindenlab.com",
	  "https://login.siva.lindenlab.com/cgi-bin/login.cgi",
	  "http://siva-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Shakti",
	  "util.shakti.lindenlab.com",
	  "https://login.shakti.lindenlab.com/cgi-bin/login.cgi",
	  "http://shakti-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Soma",
	  "util.soma.lindenlab.com",
	  "https://login.soma.lindenlab.com/cgi-bin/login.cgi",
	  "http://soma-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Uma",
	  "util.uma.lindenlab.com",
	  "https://login.uma.lindenlab.com/cgi-bin/login.cgi",
	  "http://uma-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Vaak",
	  "util.vaak.lindenlab.com",
	  "https://login.vaak.lindenlab.com/cgi-bin/login.cgi",
	  "http://vaak-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Yami",
	  "util.yami.lindenlab.com",
	  "https://login.yami.lindenlab.com/cgi-bin/login.cgi",
	  "http://yami-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Local", 
	  "localhost", 
	  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",
	  "" },
	{ "Other", 
	  "", 
	  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",
	  "" }
};

// Use this to figure out which domain name and login URI to use.

EGridInfo gGridChoice = GRID_INFO_NONE;
char gGridName[MAX_STRING];		/* Flawfinder: ignore */

F32 gPacketDropPercentage = 0.f;
F32 gInBandwidth = 0.f;
F32 gOutBandwidth = 0.f;

unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */
