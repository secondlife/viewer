/** 
 * @file lldxhardware.cpp
 * @brief LLDXHardware implementation
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifdef LL_WINDOWS

// Culled from some Microsoft sample code

#include "linden_common.h"

#include <assert.h>
#include <dxdiag.h>

#include <boost/tokenizer.hpp>

#include "lldxhardware.h"
#include "llerror.h"

#include "llstring.h"
#include "llstl.h"

void (*gWriteDebug)(const char* msg) = NULL;
LLDXHardware gDXHardware;

//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

std::string get_string(IDxDiagContainer *containerp, WCHAR *wszPropName)
{
    HRESULT hr;
	VARIANT var;
	WCHAR wszPropValue[256];

	VariantInit( &var );
	hr = containerp->GetProp(wszPropName, &var );
	if( SUCCEEDED(hr) )
	{
		// Switch off the type.  There's 4 different types:
		switch( var.vt )
		{
			case VT_UI4:
				swprintf( wszPropValue, L"%d", var.ulVal );	/* Flawfinder: ignore */
				break;
			case VT_I4:
				swprintf( wszPropValue, L"%d", var.lVal );	/* Flawfinder: ignore */
				break;
			case VT_BOOL:
				wcscpy( wszPropValue, (var.boolVal) ? L"true" : L"false" );	/* Flawfinder: ignore */
				break;
			case VT_BSTR:
				wcsncpy( wszPropValue, var.bstrVal, 255 );	/* Flawfinder: ignore */
				wszPropValue[255] = 0;
				break;
		}
	}
	// Clear the variant (this is needed to free BSTR memory)
	VariantClear( &var );

	return utf16str_to_utf8str(wszPropValue);
}


LLVersion::LLVersion()
{
	mValid = FALSE;
	S32 i;
	for (i = 0; i < 4; i++)
	{
		mFields[i] = 0;
	}
}

BOOL LLVersion::set(const std::string &version_string)
{
	S32 i;
	for (i = 0; i < 4; i++)
	{
		mFields[i] = 0;
	}
	// Split the version string.
	std::string str(version_string);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".", "", boost::keep_empty_tokens);
	tokenizer tokens(str, sep);

	tokenizer::iterator iter = tokens.begin();
	S32 count = 0;
	for (;(iter != tokens.end()) && (count < 4);++iter)
	{
		mFields[count] = atoi(iter->c_str());
		count++;
	}
	if (count < 4)
	{
		//llwarns << "Potentially bogus version string!" << version_string << llendl;
		for (i = 0; i < 4; i++)
		{
			mFields[i] = 0;
		}
		mValid = FALSE;
	}
	else
	{
		mValid = TRUE;
	}
	return mValid;
}

S32 LLVersion::getField(const S32 field_num)
{
	if (!mValid)
	{
		return -1;
	}
	else
	{
		return mFields[field_num];
	}
}

LLString LLDXDriverFile::dump()
{
	if (gWriteDebug)
	{
		gWriteDebug("Filename:");
		gWriteDebug(mName.c_str());
		gWriteDebug("\n");
		gWriteDebug("Ver:");
		gWriteDebug(mVersionString.c_str());
		gWriteDebug("\n");
		gWriteDebug("Date:");
		gWriteDebug(mDateString.c_str());
		gWriteDebug("\n");
	}
	llinfos << mFilepath << llendl;
	llinfos << mName << llendl;
	llinfos << mVersionString << llendl;
	llinfos << mDateString << llendl;

	return "";
}

LLDXDevice::~LLDXDevice()
{
	for_each(mDriverFiles.begin(), mDriverFiles.end(), DeletePairedPointer());
}

std::string LLDXDevice::dump()
{
	if (gWriteDebug)
	{
		gWriteDebug("StartDevice\n");
		gWriteDebug("DeviceName:");
		gWriteDebug(mName.c_str());
		gWriteDebug("\n");
		gWriteDebug("PCIString:");
		gWriteDebug(mPCIString.c_str());
		gWriteDebug("\n");
	}
	llinfos << llendl;
	llinfos << "DeviceName:" << mName << llendl;
	llinfos << "PCIString:" << mPCIString << llendl;
	llinfos << "Drivers" << llendl;
	llinfos << "-------" << llendl;
	for (driver_file_map_t::iterator iter = mDriverFiles.begin(),
			 end = mDriverFiles.end();
		 iter != end; iter++)
	{
		LLDXDriverFile *filep = iter->second;
		filep->dump();
	}
	if (gWriteDebug)
	{
		gWriteDebug("EndDevice\n");
	}

	return "";
}

LLDXDriverFile *LLDXDevice::findDriver(const std::string &driver)
{
	for (driver_file_map_t::iterator iter = mDriverFiles.begin(),
			 end = mDriverFiles.end();
		 iter != end; iter++)
	{
		LLDXDriverFile *filep = iter->second;
		if (!utf8str_compare_insensitive(filep->mName,driver))
		{
			return filep;
		}
	}

	return NULL;
}

LLDXHardware::LLDXHardware()
{
	mVRAM = 0;
	gWriteDebug = NULL;
}

void LLDXHardware::cleanup()
{
	for_each(mDevices.begin(), mDevices.end(), DeletePairedPointer());
}

LLString LLDXHardware::dumpDevices()
{
	if (gWriteDebug)
	{
		gWriteDebug("\n");
		gWriteDebug("StartAllDevices\n");
	}
	for (device_map_t::iterator iter = mDevices.begin(),
			 end = mDevices.end();
		 iter != end; iter++)
	{
		LLDXDevice *devicep = iter->second;
		devicep->dump();
	}
	if (gWriteDebug)
	{
		gWriteDebug("EndAllDevices\n\n");
	}
	return "";
}

LLDXDevice *LLDXHardware::findDevice(const std::string &vendor, const std::string &devices)
{
	// Iterate through different devices tokenized in devices string
	std::string str(devices);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
	tokenizer tokens(str, sep);

	tokenizer::iterator iter = tokens.begin();
	for (;iter != tokens.end();++iter)
	{
		std::string dev_str = *iter;
		for (device_map_t::iterator iter = mDevices.begin(),
				 end = mDevices.end();
			 iter != end; iter++)
		{
			LLDXDevice *devicep = iter->second;
			if ((devicep->mVendorID == vendor)
				&& (devicep->mDeviceID == dev_str))
			{
				return devicep;
			}
		}
	}

	return NULL;
}

BOOL LLDXHardware::getInfo(BOOL vram_only)
{
	LLTimer hw_timer;
	BOOL ok = FALSE;
    HRESULT       hr;

    CoInitialize(NULL);

    IDxDiagProvider *dx_diag_providerp = NULL;
    IDxDiagContainer *dx_diag_rootp = NULL;
	IDxDiagContainer *devices_containerp = NULL;
	IDxDiagContainer *system_device_containerp= NULL;
	IDxDiagContainer *device_containerp = NULL;
	IDxDiagContainer *file_containerp = NULL;
	IDxDiagContainer *driver_containerp = NULL;

    // CoCreate a IDxDiagProvider*
	llinfos << "CoCreateInstance IID_IDxDiagProvider" << llendl;
    hr = CoCreateInstance(CLSID_DxDiagProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider,
                          (LPVOID*) &dx_diag_providerp);

	if (FAILED(hr))
	{
		llwarns << "No DXDiag provider found!  DirectX 9 not installed!" << llendl;
		gWriteDebug("No DXDiag provider found!  DirectX 9 not installed!\n");
		goto LCleanup;
	}
    if (SUCCEEDED(hr)) // if FAILED(hr) then dx9 is not installed
    {
        // Fill out a DXDIAG_INIT_PARAMS struct and pass it to IDxDiagContainer::Initialize
        // Passing in TRUE for bAllowWHQLChecks, allows dxdiag to check if drivers are 
        // digital signed as logo'd by WHQL which may connect via internet to update 
        // WHQL certificates.    
        DXDIAG_INIT_PARAMS dx_diag_init_params;
        ZeroMemory(&dx_diag_init_params, sizeof(DXDIAG_INIT_PARAMS));

        dx_diag_init_params.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
        dx_diag_init_params.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
        dx_diag_init_params.bAllowWHQLChecks        = TRUE;
        dx_diag_init_params.pReserved               = NULL;

		llinfos << "dx_diag_providerp->Initialize" << llendl;
        hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
        if(FAILED(hr))
		{
            goto LCleanup;
		}

		llinfos << "dx_diag_providerp->GetRootContainer" << llendl;
        hr = dx_diag_providerp->GetRootContainer( &dx_diag_rootp );
        if(FAILED(hr) || !dx_diag_rootp)
		{
            goto LCleanup;
		}

		HRESULT hr;

		// Get display driver information
		llinfos << "dx_diag_rootp->GetChildContainer" << llendl;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices", &devices_containerp);
		if(FAILED(hr) || !devices_containerp)
		{
            goto LCleanup;
		}

		// Get device 0
		llinfos << "devices_containerp->GetChildContainer" << llendl;
		hr = devices_containerp->GetChildContainer(L"0", &device_containerp);
		if(FAILED(hr) || !device_containerp)
		{
            goto LCleanup;
		}
		
		// Get the English VRAM string
		std::string ram_str = get_string(device_containerp, L"szDisplayMemoryEnglish");

		// We don't need the device any more
		SAFE_RELEASE(device_containerp);

		// Dump the string as an int into the structure
		char *stopstring;
		mVRAM = strtol(ram_str.c_str(), &stopstring, 10); 
		llinfos << "VRAM Detected: " << mVRAM << " DX9 string: " << ram_str << llendl;
		
		if (vram_only)
		{
			ok = TRUE;
			goto LCleanup;
		}

		// Now let's get device and driver information
		// Get the IDxDiagContainer object called "DxDiag_SystemDevices".
		// This call may take some time while dxdiag gathers the info.
		DWORD num_devices = 0;
	    WCHAR wszContainer[256];
		llinfos << "dx_diag_rootp->GetChildContainer DxDiag_SystemDevices" << llendl;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_SystemDevices", &system_device_containerp);
		if (FAILED(hr))
		{
			goto LCleanup;
		}

		hr = system_device_containerp->GetNumberOfChildContainers(&num_devices);
		if (FAILED(hr))
		{
			goto LCleanup;
		}

		llinfos << "DX9 iterating over devices" << llendl;
		S32 device_num = 0;
		for (device_num = 0; device_num < (S32)num_devices; device_num++)
		{
			hr = system_device_containerp->EnumChildContainerNames(device_num, wszContainer, 256);
			if (FAILED(hr))
			{
				goto LCleanup;
			}

			hr = system_device_containerp->GetChildContainer(wszContainer, &device_containerp);
			if (FAILED(hr) || device_containerp == NULL)
			{
				goto LCleanup;
			}

			std::string device_name = get_string(device_containerp, L"szDescription");
			std::string device_id = get_string(device_containerp, L"szDeviceID");

			LLDXDevice *dxdevicep = new LLDXDevice;
			dxdevicep->mName = device_name;
			dxdevicep->mPCIString = device_id;
			mDevices[dxdevicep->mPCIString] = dxdevicep;

			// Split the PCI string based on vendor, device, subsys, rev.
			std::string str(device_id);
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			boost::char_separator<char> sep("&\\", "", boost::keep_empty_tokens);
			tokenizer tokens(str, sep);

			tokenizer::iterator iter = tokens.begin();
			S32 count = 0;
			BOOL valid = TRUE;
			for (;(iter != tokens.end()) && (count < 3);++iter)
			{
				switch (count)
				{
				case 0:
					if (strcmp(iter->c_str(), "PCI"))
					{
						valid = FALSE;
					}
					break;
				case 1:
					dxdevicep->mVendorID = iter->c_str();
					break;
				case 2:
					dxdevicep->mDeviceID = iter->c_str();
					break;
				default:
					// Ignore it
					break;
				}
				count++;
			}


			// Now, iterate through the related drivers
			hr = device_containerp->GetChildContainer(L"Drivers", &driver_containerp);
			if (FAILED(hr) || !driver_containerp)
			{
				goto LCleanup;
			}

			DWORD num_files = 0;
			hr = driver_containerp->GetNumberOfChildContainers(&num_files);
			if (FAILED(hr))
			{
				goto LCleanup;
			}

			S32 file_num = 0;
			for (file_num = 0; file_num < (S32)num_files; file_num++ )
			{
				hr = driver_containerp->EnumChildContainerNames(file_num, wszContainer, 256);
				if (FAILED(hr))
				{
					goto LCleanup;
				}

				hr = driver_containerp->GetChildContainer(wszContainer, &file_containerp);
				if (FAILED(hr) || file_containerp == NULL)
				{
					goto LCleanup;
				}

				std::string driver_path = get_string(file_containerp, L"szPath");
				std::string driver_name = get_string(file_containerp, L"szName");
				std::string driver_version = get_string(file_containerp, L"szVersion");
				std::string driver_date = get_string(file_containerp, L"szDatestampEnglish");

				LLDXDriverFile *dxdriverfilep = new LLDXDriverFile;
				dxdriverfilep->mName = driver_name;
				dxdriverfilep->mFilepath= driver_path;
				dxdriverfilep->mVersionString = driver_version;
				dxdriverfilep->mVersion.set(driver_version);
				dxdriverfilep->mDateString = driver_date;

				dxdevicep->mDriverFiles[driver_name] = dxdriverfilep;

				SAFE_RELEASE(file_containerp);
			}
			SAFE_RELEASE(device_containerp);
		}
    }

	dumpDevices();
	ok = TRUE;

LCleanup:
	if (!ok)
	{
		llwarns << "DX9 probe failed" << llendl;
		gWriteDebug("DX9 probe failed\n");
	}

	SAFE_RELEASE(file_containerp);
	SAFE_RELEASE(driver_containerp);
	SAFE_RELEASE(device_containerp);
	SAFE_RELEASE(devices_containerp);
    SAFE_RELEASE(dx_diag_rootp);
    SAFE_RELEASE(dx_diag_providerp);
    
    CoUninitialize();
    
    return ok;
}

void LLDXHardware::setWriteDebugFunc(void (*func)(const char*))
{
	gWriteDebug = func;
}

#endif
