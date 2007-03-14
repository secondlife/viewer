/** 
 * @file llviewernetwork.cpp
 * @author James Cook, Richard Nelson
 * @brief Networking constants and globals for viewer.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewernetwork.h"

LLUserServerData gUserServerDomainName[USERSERVER_COUNT] = 
{
	{ "None", "", "", ""},
	{ "Aditi", 
	  "userserver.aditi.lindenlab.com", 
	  "https://login.aditi.lindenlab.com/cgi-bin/login.cgi",
	  "http://aditi-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Agni", 
	  "userserver.agni.lindenlab.com", 
	  "https://login.agni.lindenlab.com/cgi-bin/login.cgi",
	  "https://secondlife.com/helpers/" },
	{ "DMZ",  
	  "userserver.dmz.lindenlab.com", 
	  "https://login.dmz.lindenlab.com/cgi-bin/login.cgi",
	  "http://dmz-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Siva", 
	  "userserver.siva.lindenlab.com",
	  "https://login.siva.lindenlab.com/cgi-bin/login.cgi",
	  "http://siva-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Durga",
	  "userserver.durga.lindenlab.com",
	  "https://login.durga.lindenlab.com/cgi-bin/login.cgi",
	  "http://durga-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Shakti",
	  "userserver.shakti.lindenlab.com",
	  "https://login.shakti.lindenlab.com/cgi-bin/login.cgi",
	  "http://shakti-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Soma",
	  "userserver.soma.lindenlab.com",
	  "https://login.soma.lindenlab.com/cgi-bin/login.cgi",
	  "http://soma-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Ganga",
	  "userserver.ganga.lindenlab.com",
	  "https://login.ganga.lindenlab.com/cgi-bin/login.cgi",
	  "http://ganga-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Vaak",
	  "userserver.vaak.lindenlab.com",
	  "https://login.vaak.lindenlab.com/cgi-bin/login.cgi",
	  "http://vaak-secondlife.webdev.lindenlab.com/helpers/" },
	{ "Uma",
	  "userserver.uma.lindenlab.com",
	  "https://login.uma.lindenlab.com/cgi-bin/login.cgi",
	  "http://uma-secondlife.webdev.lindenlab.com/helpers/" },
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

EUserServerDomain gUserServerChoice = USERSERVER_NONE;
char gUserServerName[MAX_STRING];		/* Flawfinder: ignore */

LLHost gUserServer;

F32 gPacketDropPercentage = 0.f;
F32 gInBandwidth = 0.f;
F32 gOutBandwidth = 0.f;

unsigned char gMACAddress[MAC_ADDRESS_BYTES];		/* Flawfinder: ignore */
