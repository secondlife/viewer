/** 
 * @file lldxhardware.h
 * @brief LLDXHardware definition
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLDXHARDWARE_H
#define LL_LLDXHARDWARE_H

#include <map>

#include "stdtypes.h"
#include "llstring.h"

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
	LLString dump();

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

	// Find a particular device that matches the following specs.
	// Empty strings indicate that you don't care.
	// You can separate multiple devices with '|' chars to indicate you want
	// ANY of them to match and return.
	LLDXDevice *findDevice(const std::string &vendor, const std::string &devices);

	LLString dumpDevices();
public:
	typedef std::map<std::string, LLDXDevice *> device_map_t;
	device_map_t mDevices;
protected:
	S32 mVRAM;
};

extern void (*gWriteDebug)(const char* msg);
extern LLDXHardware gDXHardware;

#endif // LL_LLDXHARDWARE_H
