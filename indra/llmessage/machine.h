/** 
 * @file machine.h
 * @brief LLMachine class header file
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_MACHINE_H
#define LL_MACHINE_H

#include "net.h"
#include "llhost.h"

typedef enum e_machine_type
{
	MT_NULL,
	MT_SIMULATOR,
	MT_VIEWER,
	MT_SPACE_SERVER,
	MT_OBJECT_REPOSITORY,
	MT_PROXY,
	MT_EOF
} EMachineType;

const U32 ADDRESS_STRING_SIZE = 12;

class LLMachine
{
public:
	LLMachine() 
		: mMachineType(MT_NULL), mControlPort(0) {}	

	LLMachine(EMachineType machine_type, U32 ip, S32 port) 
		: mMachineType(machine_type), mControlPort(0), mHost(ip,port) {}

	LLMachine(EMachineType machine_type, const LLHost &host) 
		: mMachineType(machine_type) {mHost = host; mControlPort = 0;}

	~LLMachine()	{}

	// get functions
	EMachineType	getMachineType()	const { return mMachineType; }
	U32				getMachineIP()		const { return mHost.getAddress(); }
	S32				getMachinePort()	const { return mHost.getPort(); }
	const LLHost	&getMachineHost()	const { return mHost; }
	// The control port is the listen port of the parent process that
	// launched this machine. 0 means none or not known.
	const S32		&getControlPort() 	const { return mControlPort; }
	BOOL			isValid()			const { return (mHost.getPort() != 0); }	// TRUE if corresponds to functioning machine

	// set functions
	void			setMachineType(EMachineType machine_type)	{ mMachineType = machine_type; }
	void			setMachineIP(U32 ip)						{ mHost.setAddress(ip); }
	void			setMachineHost(const LLHost &host)	 	    { mHost = host; }

	void 			setMachinePort(S32 port);
	void			setControlPort( S32 port );


	// member variables

// Someday these should be made private.
// When they are, some of the code that breaks should 
// become member functions of LLMachine -- Leviathan
//private:

	// I fixed the others, somebody should fix these! - djs
	EMachineType	mMachineType;

protected:

	S32				mControlPort;
	LLHost          mHost;
};

#endif
