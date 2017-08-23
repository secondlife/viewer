/** 
 * @file lldxhardware.cpp
 * @brief LLDXHardware implementation
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

#ifdef LL_WINDOWS

// Culled from some Microsoft sample code

#include "linden_common.h"

#define INITGUID
#include <dxdiag.h>
#undef INITGUID

#include <wbemidl.h>
#include <comdef.h>

#include <boost/tokenizer.hpp>

#include "lldxhardware.h"

#include "llerror.h"

#include "llstring.h"
#include "llstl.h"
#include "lltimer.h"

void (*gWriteDebug)(const char* msg) = NULL;
LLDXHardware gDXHardware;

//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

typedef BOOL ( WINAPI* PfnCoSetProxyBlanket )( IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc,
                                               OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel,
                                               RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );

HRESULT GetVideoMemoryViaWMI( WCHAR* strInputDeviceID, DWORD* pdwAdapterRam )
{
    HRESULT hr;
    bool bGotMemory = false;
    HRESULT hrCoInitialize = S_OK;
    IWbemLocator* pIWbemLocator = nullptr;
    IWbemServices* pIWbemServices = nullptr;
    BSTR pNamespace = nullptr;

    *pdwAdapterRam = 0;
    hrCoInitialize = CoInitialize( 0 );

    hr = CoCreateInstance( CLSID_WbemLocator,
                           nullptr,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           ( LPVOID* )&pIWbemLocator );
#ifdef PRINTF_DEBUGGING
    if( FAILED( hr ) ) wprintf( L"WMI: CoCreateInstance failed: 0x%0.8x\n", hr );
#endif

    if( SUCCEEDED( hr ) && pIWbemLocator )
    {
        // Using the locator, connect to WMI in the given namespace.
        pNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );

        hr = pIWbemLocator->ConnectServer( pNamespace, nullptr, nullptr, 0L,
                                           0L, nullptr, nullptr, &pIWbemServices );
#ifdef PRINTF_DEBUGGING
        if( FAILED( hr ) ) wprintf( L"WMI: pIWbemLocator->ConnectServer failed: 0x%0.8x\n", hr );
#endif
        if( SUCCEEDED( hr ) && pIWbemServices != 0 )
        {
            HINSTANCE hinstOle32 = nullptr;

            hinstOle32 = LoadLibraryW( L"ole32.dll" );
            if( hinstOle32 )
            {
                PfnCoSetProxyBlanket pfnCoSetProxyBlanket = nullptr;

                pfnCoSetProxyBlanket = ( PfnCoSetProxyBlanket )GetProcAddress( hinstOle32, "CoSetProxyBlanket" );
                if( pfnCoSetProxyBlanket != 0 )
                {
                    // Switch security level to IMPERSONATE. 
                    pfnCoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, 0 );
                }

                FreeLibrary( hinstOle32 );
            }

            IEnumWbemClassObject* pEnumVideoControllers = nullptr;
            BSTR pClassName = nullptr;

            pClassName = SysAllocString( L"Win32_VideoController" );

            hr = pIWbemServices->CreateInstanceEnum( pClassName, 0,
                                                     nullptr, &pEnumVideoControllers );
#ifdef PRINTF_DEBUGGING
            if( FAILED( hr ) ) wprintf( L"WMI: pIWbemServices->CreateInstanceEnum failed: 0x%0.8x\n", hr );
#endif

            if( SUCCEEDED( hr ) && pEnumVideoControllers )
            {
                IWbemClassObject* pVideoControllers[10] = {0};
                DWORD uReturned = 0;
                BSTR pPropName = nullptr;

                // Get the first one in the list
                pEnumVideoControllers->Reset();
                hr = pEnumVideoControllers->Next( 5000,             // timeout in 5 seconds
                                                  10,                  // return the first 10
                                                  pVideoControllers,
                                                  &uReturned );
#ifdef PRINTF_DEBUGGING
                if( FAILED( hr ) ) wprintf( L"WMI: pEnumVideoControllers->Next failed: 0x%0.8x\n", hr );
                if( uReturned == 0 ) wprintf( L"WMI: pEnumVideoControllers uReturned == 0\n" );
#endif

                VARIANT var;
                if( SUCCEEDED( hr ) )
                {
                    bool bFound = false;
                    for( UINT iController = 0; iController < uReturned; iController++ )
                    {
                        if ( !pVideoControllers[iController] )
                            continue;

                        pPropName = SysAllocString( L"PNPDeviceID" );
                        hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, nullptr, nullptr );
#ifdef PRINTF_DEBUGGING
                        if( FAILED( hr ) )
                            wprintf( L"WMI: pVideoControllers[iController]->Get PNPDeviceID failed: 0x%0.8x\n", hr );
#endif
                        if( SUCCEEDED( hr ) )
                        {
                            if( wcsstr( var.bstrVal, strInputDeviceID ) != 0 )
                                bFound = true;
                        }
                        VariantClear( &var );
                        if( pPropName ) SysFreeString( pPropName );

                        if( bFound )
                        {
                            pPropName = SysAllocString( L"AdapterRAM" );
                            hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, nullptr, nullptr );
#ifdef PRINTF_DEBUGGING
                            if( FAILED( hr ) )
                                wprintf( L"WMI: pVideoControllers[iController]->Get AdapterRAM failed: 0x%0.8x\n",
                                         hr );
#endif
                            if( SUCCEEDED( hr ) )
                            {
                                bGotMemory = true;
                                *pdwAdapterRam = var.ulVal;
                            }
                            VariantClear( &var );
                            if( pPropName ) SysFreeString( pPropName );
                            break;
                        }
                        SAFE_RELEASE( pVideoControllers[iController] );
                    }
                }
            }

            if( pClassName )
                SysFreeString( pClassName );
            SAFE_RELEASE( pEnumVideoControllers );
        }

        if( pNamespace )
            SysFreeString( pNamespace );
        SAFE_RELEASE( pIWbemServices );
    }

    SAFE_RELEASE( pIWbemLocator );

    if( SUCCEEDED( hrCoInitialize ) )
        CoUninitialize();

    if( bGotMemory )
        return S_OK;
    else
        return E_FAIL;
}

//Getting the version of graphics controller driver via WMI
std::string LLDXHardware::getDriverVersionWMI()
{
	std::string mDriverVersion;
	HRESULT hrCoInitialize = S_OK;
	HRESULT hres;
	hrCoInitialize = CoInitialize(0);
	IWbemLocator *pLoc = NULL;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);
	
	if (FAILED(hres))
	{
		LL_DEBUGS("AppInit") << "Failed to initialize COM library. Error code = 0x" << hres << LL_ENDL;
		return std::string();                  // Program has failed.
	}

	IWbemServices *pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (e.g. Kerberos)
		0,                       // Context object 
		&pSvc                    // pointer to IWbemServices proxy
		);

	if (FAILED(hres))
	{
		LL_WARNS("AppInit") << "Could not connect. Error code = 0x" << hres << LL_ENDL;
		pLoc->Release();
		CoUninitialize();
		return std::string();                // Program has failed.
	}

	LL_DEBUGS("AppInit") << "Connected to ROOT\\CIMV2 WMI namespace" << LL_ENDL;

	// Set security levels on the proxy -------------------------
	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
		);

	if (FAILED(hres))
	{
		LL_WARNS("AppInit") << "Could not set proxy blanket. Error code = 0x" << hres << LL_ENDL;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return std::string();               // Program has failed.
	}
	IEnumWbemClassObject* pEnumerator = NULL;

	// Get the data from the query
	ULONG uReturn = 0;
	hres = pSvc->ExecQuery( 
		bstr_t("WQL"),
		bstr_t("SELECT * FROM Win32_VideoController"), //Consider using Availability to filter out disabled controllers
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		LL_WARNS("AppInit") << "Query for operating system name failed." << " Error code = 0x" << hres << LL_ENDL;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return std::string();               // Program has failed.
	}

	while (pEnumerator)
	{
		IWbemClassObject *pclsObj = NULL;
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;               // If quantity less then 1.
		}

		VARIANT vtProp;

		// Get the value of the Name property
		hr = pclsObj->Get(L"DriverVersion", 0, &vtProp, 0, 0);

		if (FAILED(hr))
		{
			LL_WARNS("AppInit") << "Query for name property failed." << " Error code = 0x" << hr << LL_ENDL;
			pSvc->Release();
			pLoc->Release();
			CoUninitialize();
			return std::string();               // Program has failed.
		}

		// use characters in the returned driver version
		BSTR driverVersion(vtProp.bstrVal);

		//convert BSTR to std::string
		std::wstring ws(driverVersion, SysStringLen(driverVersion));
		std::string str(ws.begin(), ws.end());
		LL_INFOS("AppInit") << " DriverVersion : " << str << LL_ENDL;

		if (mDriverVersion.empty())
		{
			mDriverVersion = str;
		}
		else if (mDriverVersion != str)
		{
			LL_WARNS("DriverVersion") << "Different versions of drivers. Version of second driver : " << str << LL_ENDL;
		}

		VariantClear(&vtProp);
		pclsObj->Release();
	}

	// Cleanup
	// ========
	if (pSvc)
	{
		pSvc->Release();
	}
	if (pLoc)
	{
		pLoc->Release();
	}
	if (pEnumerator)
	{
		pEnumerator->Release();
	}
	if (SUCCEEDED(hrCoInitialize))
	{
		CoUninitialize();
	}
	return mDriverVersion;
}

void get_wstring(IDxDiagContainer* containerp, WCHAR* wszPropName, WCHAR* wszPropValue, int outputSize)
{
	HRESULT hr;
	VARIANT var;

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
				wcsncpy( wszPropValue, var.bstrVal, outputSize-1 );	/* Flawfinder: ignore */
				wszPropValue[outputSize-1] = 0;
				break;
		}
	}
	// Clear the variant (this is needed to free BSTR memory)
	VariantClear( &var );
}

std::string get_string(IDxDiagContainer *containerp, WCHAR *wszPropName)
{
    WCHAR wszPropValue[256];
	get_wstring(containerp, wszPropName, wszPropValue, 256);

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
		//LL_WARNS() << "Potentially bogus version string!" << version_string << LL_ENDL;
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

std::string LLDXDriverFile::dump()
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
	LL_INFOS() << mFilepath << LL_ENDL;
	LL_INFOS() << mName << LL_ENDL;
	LL_INFOS() << mVersionString << LL_ENDL;
	LL_INFOS() << mDateString << LL_ENDL;

	return "";
}

LLDXDevice::~LLDXDevice()
{
	for_each(mDriverFiles.begin(), mDriverFiles.end(), DeletePairedPointer());
	mDriverFiles.clear();
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
	LL_INFOS() << LL_ENDL;
	LL_INFOS() << "DeviceName:" << mName << LL_ENDL;
	LL_INFOS() << "PCIString:" << mPCIString << LL_ENDL;
	LL_INFOS() << "Drivers" << LL_ENDL;
	LL_INFOS() << "-------" << LL_ENDL;
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
  // for_each(mDevices.begin(), mDevices.end(), DeletePairedPointer());
  // mDevices.clear();
}

/*
std::string LLDXHardware::dumpDevices()
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
*/

BOOL LLDXHardware::getInfo(BOOL vram_only)
{
	LLTimer hw_timer;
	BOOL ok = FALSE;
    HRESULT       hr;

    CoInitialize(NULL);

    IDxDiagProvider *dx_diag_providerp = NULL;
    IDxDiagContainer *dx_diag_rootp = NULL;
	IDxDiagContainer *devices_containerp = NULL;
	// IDxDiagContainer *system_device_containerp= NULL;
	IDxDiagContainer *device_containerp = NULL;
	IDxDiagContainer *file_containerp = NULL;
	IDxDiagContainer *driver_containerp = NULL;

    // CoCreate a IDxDiagProvider*
	LL_DEBUGS("AppInit") << "CoCreateInstance IID_IDxDiagProvider" << LL_ENDL;
    hr = CoCreateInstance(CLSID_DxDiagProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider,
                          (LPVOID*) &dx_diag_providerp);

	if (FAILED(hr))
	{
		LL_WARNS("AppInit") << "No DXDiag provider found!  DirectX 9 not installed!" << LL_ENDL;
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

		LL_DEBUGS("AppInit") << "dx_diag_providerp->Initialize" << LL_ENDL;
        hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
        if(FAILED(hr))
		{
            goto LCleanup;
		}

		LL_DEBUGS("AppInit") << "dx_diag_providerp->GetRootContainer" << LL_ENDL;
        hr = dx_diag_providerp->GetRootContainer( &dx_diag_rootp );
        if(FAILED(hr) || !dx_diag_rootp)
		{
            goto LCleanup;
		}

		HRESULT hr;

		// Get display driver information
		LL_DEBUGS("AppInit") << "dx_diag_rootp->GetChildContainer" << LL_ENDL;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices", &devices_containerp);
		if(FAILED(hr) || !devices_containerp)
		{
            goto LCleanup;
		}

		// Get device 0
		LL_DEBUGS("AppInit") << "devices_containerp->GetChildContainer" << LL_ENDL;
		hr = devices_containerp->GetChildContainer(L"0", &device_containerp);
		if(FAILED(hr) || !device_containerp)
		{
            goto LCleanup;
		}
		
		DWORD vram = 0;

		WCHAR deviceID[512];

		get_wstring(device_containerp, L"szDeviceID", deviceID, 512);
		
		if (SUCCEEDED(GetVideoMemoryViaWMI(deviceID, &vram))) 
		{
			mVRAM = vram/(1024*1024);
		}
		else
		{ // Get the English VRAM string
		  std::string ram_str = get_string(device_containerp, L"szDisplayMemoryEnglish");

		  // We don't need the device any more
		  SAFE_RELEASE(device_containerp);

		  // Dump the string as an int into the structure
		  char *stopstring;
		  mVRAM = strtol(ram_str.c_str(), &stopstring, 10); 
		  LL_INFOS("AppInit") << "VRAM Detected: " << mVRAM << " DX9 string: " << ram_str << LL_ENDL;
		}

		if (vram_only)
		{
			ok = TRUE;
			goto LCleanup;
		}


		/* for now, we ONLY do vram_only the rest of this
		   is commented out, to ensure no-one is tempted
		   to use it
		
		// Now let's get device and driver information
		// Get the IDxDiagContainer object called "DxDiag_SystemDevices".
		// This call may take some time while dxdiag gathers the info.
		DWORD num_devices = 0;
	    WCHAR wszContainer[256];
		LL_DEBUGS("AppInit") << "dx_diag_rootp->GetChildContainer DxDiag_SystemDevices" << LL_ENDL;
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

		LL_DEBUGS("AppInit") << "DX9 iterating over devices" << LL_ENDL;
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
		*/
    }

    // dumpDevices();
    ok = TRUE;
	
LCleanup:
	if (!ok)
	{
		LL_WARNS("AppInit") << "DX9 probe failed" << LL_ENDL;
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

LLSD LLDXHardware::getDisplayInfo()
{
	LLTimer hw_timer;
    HRESULT       hr;
	LLSD ret;
    CoInitialize(NULL);

    IDxDiagProvider *dx_diag_providerp = NULL;
    IDxDiagContainer *dx_diag_rootp = NULL;
	IDxDiagContainer *devices_containerp = NULL;
	IDxDiagContainer *device_containerp = NULL;
	IDxDiagContainer *file_containerp = NULL;
	IDxDiagContainer *driver_containerp = NULL;

    // CoCreate a IDxDiagProvider*
	LL_INFOS() << "CoCreateInstance IID_IDxDiagProvider" << LL_ENDL;
    hr = CoCreateInstance(CLSID_DxDiagProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider,
                          (LPVOID*) &dx_diag_providerp);

	if (FAILED(hr))
	{
		LL_WARNS() << "No DXDiag provider found!  DirectX 9 not installed!" << LL_ENDL;
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

		LL_INFOS() << "dx_diag_providerp->Initialize" << LL_ENDL;
        hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
        if(FAILED(hr))
		{
            goto LCleanup;
		}

		LL_INFOS() << "dx_diag_providerp->GetRootContainer" << LL_ENDL;
        hr = dx_diag_providerp->GetRootContainer( &dx_diag_rootp );
        if(FAILED(hr) || !dx_diag_rootp)
		{
            goto LCleanup;
		}

		HRESULT hr;

		// Get display driver information
		LL_INFOS() << "dx_diag_rootp->GetChildContainer" << LL_ENDL;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices", &devices_containerp);
		if(FAILED(hr) || !devices_containerp)
		{
            goto LCleanup;
		}

		// Get device 0
		LL_INFOS() << "devices_containerp->GetChildContainer" << LL_ENDL;
		hr = devices_containerp->GetChildContainer(L"0", &device_containerp);
		if(FAILED(hr) || !device_containerp)
		{
            goto LCleanup;
		}
		
		// Get the English VRAM string
		std::string ram_str = get_string(device_containerp, L"szDisplayMemoryEnglish");


		// Dump the string as an int into the structure
		char *stopstring;
		ret["VRAM"] = strtol(ram_str.c_str(), &stopstring, 10);
		std::string device_name = get_string(device_containerp, L"szDescription");
		ret["DeviceName"] = device_name;
		std::string device_driver=  get_string(device_containerp, L"szDriverVersion");
		ret["DriverVersion"] = device_driver;
        
        // ATI has a slightly different version string
        if(device_name.length() >= 4 && device_name.substr(0,4) == "ATI ")
        {
            // get the key
            HKEY hKey;
            const DWORD RV_SIZE = 100;
            WCHAR release_version[RV_SIZE];

            // Hard coded registry entry.  Using this since it's simpler for now.
            // And using EnumDisplayDevices to get a registry key also requires
            // a hard coded Query value.
            if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\ATI Technologies\\CBT"), &hKey))
            {
                // get the value
                DWORD dwType = REG_SZ;
                DWORD dwSize = sizeof(WCHAR) * RV_SIZE;
                if(ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("ReleaseVersion"), 
                    NULL, &dwType, (LPBYTE)release_version, &dwSize))
                {
                    // print the value
                    // windows doesn't guarantee to be null terminated
                    release_version[RV_SIZE - 1] = NULL;
                    ret["DriverVersion"] = utf16str_to_utf8str(release_version);

                }
                RegCloseKey(hKey);
            }
        }    
    }

LCleanup:
	SAFE_RELEASE(file_containerp);
	SAFE_RELEASE(driver_containerp);
	SAFE_RELEASE(device_containerp);
	SAFE_RELEASE(devices_containerp);
    SAFE_RELEASE(dx_diag_rootp);
    SAFE_RELEASE(dx_diag_providerp);
    
    CoUninitialize();
	return ret;
}

void LLDXHardware::setWriteDebugFunc(void (*func)(const char*))
{
	gWriteDebug = func;
}

#endif
