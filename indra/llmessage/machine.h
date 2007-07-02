/** 
 * @file machine.h
 * @brief LLMachine class header file
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MACHINE_H
#define LL_MACHINE_H

#include "llerror.h"
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

	void 	setMachinePort(S32 port)
			{ 
				if (port < 0) 
				{
					llinfos << "Can't assign a negative number to LLMachine::mPort" << llendl;
					mHost.setPort(0);
				}
				else 
				{
					mHost.setPort(port); 
				}
			}

	void	setControlPort( S32 port ) 
			{
				if (port < 0) 
				{
					llinfos << "Can't assign a negative number to LLMachine::mControlPort" << llendl;
					mControlPort = 0;
				}
				else 
				{
					mControlPort = port; 
				}
			}


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
