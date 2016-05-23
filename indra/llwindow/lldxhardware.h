/** 
 * @file lldxhardware.h
 * @brief LLDXHardware definition
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLDXHARDWARE_H
#define LL_LLDXHARDWARE_H

#include <map>

#include "stdtypes.h"
#include "llstring.h"
#include "llsd.h"

class LLVersion
{
public:
	LLVersion();
	BOOL set(const std::string &version_string);
	S32 getField(const S32 field_num);
protected:
	std::string mVersionString;
	S32 mFields[4];
	BOOL mValid;
};

class LLDXDriverFile
{
public:
	std::string dump();

public:
	std::string mFilepath;
	std::string mName;
	std::string mVersionString;
	LLVersion mVersion;
	std::string mDateString;
};

class LLDXDevice
{
public:
	~LLDXDevice();
	std::string dump();

	LLDXDriverFile *findDriver(const std::string &driver);
public:
	std::string mName;
	std::string mPCIString;
	std::string mVendorID;
	std::string mDeviceID;

	typedef std::map<std::string, LLDXDriverFile *> driver_file_map_t;
	driver_file_map_t mDriverFiles;
};


class LLDXHardware
{
public:
	LLDXHardware();

	void setWriteDebugFunc(void (*func)(const char*));
	void cleanup();

	// Returns TRUE on success.
	// vram_only TRUE does a "light" probe.
	BOOL getInfo(BOOL vram_only);

	S32 getVRAM() const { return mVRAM; }

	LLSD getDisplayInfo();

	// Find a particular device that matches the following specs.
	// Empty strings indicate that you don't care.
	// You can separate multiple devices with '|' chars to indicate you want
	// ANY of them to match and return.
	// LLDXDevice *findDevice(const std::string &vendor, const std::string &devices);

	// std::string dumpDevices();
public:
	typedef std::map<std::string, LLDXDevice *> device_map_t;
	// device_map_t mDevices;
protected:
	S32 mVRAM;
};

extern void (*gWriteDebug)(const char* msg);
extern LLDXHardware gDXHardware;

#endif // LL_LLDXHARDWARE_H
