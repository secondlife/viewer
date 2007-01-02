/** 
 * @file llviewernetwork.h
 * @author James Cook
 * @brief Networking constants and globals for viewer.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERNETWORK_H
#define LL_LLVIEWERNETWORK_H

class LLHost;

enum EUserServerDomain
{
	USERSERVER_NONE,
	USERSERVER_ADITI,
	USERSERVER_AGNI,
	USERSERVER_DMZ,
	USERSERVER_SIVA,
	USERSERVER_DURGA,
	USERSERVER_SHAKTI,
	USERSERVER_SOMA,
	USERSERVER_GANGA,
	USERSERVER_LOCAL,
	USERSERVER_OTHER, // IP address set via -user or other command line option
	USERSERVER_COUNT
};


struct LLUserServerData
{
	const char* mLabel;
	const char* mName;
	const char* mLoginURI;
	const char* mHelperURI;
};

extern LLHost gUserServer;

extern F32 gPacketDropPercentage;
extern F32 gInBandwidth;
extern F32 gOutBandwidth;
extern EUserServerDomain gUserServerChoice;
extern LLUserServerData gUserServerDomainName[];
extern char gUserServerName[MAX_STRING];

const S32 MAC_ADDRESS_BYTES = 6;
extern unsigned char gMACAddress[MAC_ADDRESS_BYTES];

#endif
