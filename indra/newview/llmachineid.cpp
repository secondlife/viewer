/** 
 * @file llmachineid.cpp
 * @brief retrieves unique machine ids
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "lluuid.h"
#include "llmachineid.h"
#if LL_WINDOWS
#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#elif LL_DARWIN
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#endif
unsigned char static_unique_id[] =  {0,0,0,0,0,0};
unsigned char static_legacy_id[] =  {0,0,0,0,0,0};
bool static has_static_unique_id = false;
bool static has_static_legacy_id = false;

#if LL_WINDOWS

class LLWMIMethods
{
public:
    LLWMIMethods()
    :   pLoc(NULL),
        pSvc(NULL)
    {
        initCOMObjects();
    }

    ~LLWMIMethods()
    {
        if (isInitialized())
        {
            cleanCOMObjects();
        }
    }

    bool isInitialized() { return SUCCEEDED(mHR); }
    bool getWindowsProductNumber(unsigned char *unique_id, size_t len);
    bool getDiskDriveSerialNumber(unsigned char *unique_id, size_t len);
    bool getProcessorSerialNumber(unsigned char *unique_id, size_t len);
    bool getMotherboardSerialNumber(unsigned char *unique_id, size_t len);
    bool getComputerSystemProductUUID(unsigned char *unique_id, size_t len);
    bool getGenericSerialNumber(const BSTR &select, const LPCWSTR &variable, unsigned char *unique_id, size_t len, bool validate_as_uuid = false);

private:
    void initCOMObjects();
    void cleanCOMObjects();

    HRESULT mHR;
    IWbemLocator *pLoc;
    IWbemServices *pSvc;
};


void LLWMIMethods::initCOMObjects()
{
# pragma comment(lib, "wbemuuid.lib")
    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    mHR = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(mHR))
    {
        LL_DEBUGS("AppInit") << "Failed to initialize COM library. Error code = 0x" << std::hex << mHR << LL_ENDL;
        return;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    mHR = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );

    if (FAILED(mHR))
    {
        LL_WARNS("AppInit") << "Failed to initialize security. Error code = 0x" << std::hex << mHR << LL_ENDL;
        CoUninitialize();
        return;               // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    mHR = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID *)&pLoc);

    if (FAILED(mHR))
    {
        LL_WARNS("AppInit") << "Failed to create IWbemLocator object." << " Err code = 0x" << std::hex << mHR << LL_ENDL;
        CoUninitialize();
        return;               // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    mHR = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (e.g. Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(mHR))
    {
        LL_WARNS("AppInit") << "Could not connect. Error code = 0x" << std::hex << mHR << LL_ENDL;
        pLoc->Release();
        CoUninitialize();
        return;               // Program has failed.
    }

    LL_DEBUGS("AppInit") << "Connected to ROOT\\CIMV2 WMI namespace" << LL_ENDL;

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    mHR = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(mHR))
    {
        LL_WARNS("AppInit") << "Could not set proxy blanket. Error code = 0x" << std::hex << mHR << LL_ENDL;
        cleanCOMObjects();
        return;               // Program has failed.
    }
}


void LLWMIMethods::cleanCOMObjects()
{
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
}

bool LLWMIMethods::getWindowsProductNumber(unsigned char *unique_id, size_t len)
{
    // wmic path Win32_ComputerSystemProduct get UUID
    return getGenericSerialNumber(bstr_t("SELECT * FROM Win32_OperatingSystem"), L"SerialNumber", unique_id, len);
}

bool LLWMIMethods::getDiskDriveSerialNumber(unsigned char *unique_id, size_t len)
{
    // wmic path Win32_DiskDrive get DeviceID,SerialNumber
    return getGenericSerialNumber(bstr_t("SELECT * FROM Win32_DiskDrive"), L"SerialNumber", unique_id, len);
}

bool LLWMIMethods::getProcessorSerialNumber(unsigned char *unique_id, size_t len)
{
    // wmic path Win32_Processor get DeviceID,ProcessorId
    return getGenericSerialNumber(bstr_t("SELECT * FROM Win32_Processor"), L"ProcessorId", unique_id, len);
}

bool LLWMIMethods::getMotherboardSerialNumber(unsigned char *unique_id, size_t len)
{
    // wmic path Win32_Processor get DeviceID,ProcessorId
    return getGenericSerialNumber(bstr_t("SELECT * FROM Win32_BaseBoard"), L"SerialNumber", unique_id, len);
}

bool LLWMIMethods::getComputerSystemProductUUID(unsigned char *unique_id, size_t len)
{
    // UUID from Win32_ComputerSystemProduct is motherboard's uuid and is identical to csproduct's uuid
    // wmic csproduct get name,identifyingnumber,uuid
    // wmic path Win32_ComputerSystemProduct get UUID
    return getGenericSerialNumber(bstr_t("SELECT * FROM Win32_ComputerSystemProduct"), L"UUID", unique_id, len, true);
}

bool LLWMIMethods::getGenericSerialNumber(const BSTR &select, const LPCWSTR &variable, unsigned char *unique_id, size_t len, bool validate_as_uuid)
{
    if (!isInitialized())
    {
        return false;
    }

    HRESULT hres;

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        select,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        LL_WARNS("AppInit") << "Query for operating system name failed." << " Error code = 0x" << std::hex << hres << LL_ENDL;
        return false;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    bool found = false;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        // Get the value of the Name property
        hr = pclsObj->Get(variable, 0, &vtProp, 0, 0);
        if (FAILED(hr))
        {
            LL_WARNS() << "Failed to get SerialNumber. Error code = 0x" << std::hex << hres << LL_ENDL;
            pclsObj->Release();
            pclsObj = NULL;
            continue;
        }

        // use characters in the returned Serial Number to create a byte array of size len
        BSTR serialNumber(vtProp.bstrVal);
        unsigned int serial_size = SysStringLen(serialNumber);
        if (serial_size < 1) // < len?
        {
            VariantClear(&vtProp);
            pclsObj->Release();
            pclsObj = NULL;
            continue;
        }

        if (validate_as_uuid)
        {
            std::wstring ws(serialNumber, serial_size);
            std::string str(ws.begin(), ws.end());

            if (!LLUUID::validate(str))
            {
                VariantClear(&vtProp);
                pclsObj->Release();
                pclsObj = NULL;
                continue;
            }

            static const LLUUID f_uuid("FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF");
            LLUUID id(str);

            if (id.isNull() || id == f_uuid)
            {
                // Not unique id
                VariantClear(&vtProp);
                pclsObj->Release();
                pclsObj = NULL;
                continue;
            }
        }
        LL_INFOS("AppInit") << " Serial Number : " << vtProp.bstrVal << LL_ENDL;

        unsigned int j = 0;

        while (j < serial_size && vtProp.bstrVal[j] != 0)
        {
            for (unsigned int i = 0; i < len; i++)
            {
                if (j >= serial_size || vtProp.bstrVal[j] == 0)
                    break;

                unique_id[i] = (unsigned int)(unique_id[i] + serialNumber[j]);
                j++;
            }
        }
        VariantClear(&vtProp);

        pclsObj->Release();
        pclsObj = NULL;
        found = true;
        break;
    }

    // Cleanup
    // ========

    if (pEnumerator)
        pEnumerator->Release();

    return found;
}
#elif LL_DARWIN
bool getSerialNumber(unsigned char *unique_id, size_t len)
{
    CFStringRef serial_cf_str = NULL;
    io_service_t platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                                 IOServiceMatching("IOPlatformExpertDevice"));
    if (platformExpert)
    {
        serial_cf_str = (CFStringRef) IORegistryEntryCreateCFProperty(platformExpert,
                                                                     CFSTR(kIOPlatformSerialNumberKey),
                                                                     kCFAllocatorDefault, 0);
        IOObjectRelease(platformExpert);
    }
    
    if (serial_cf_str)
    {
        char buffer[64] = {0};
        std::string serial_str("");
        if (CFStringGetCString(serial_cf_str, buffer, 64, kCFStringEncodingUTF8))
        {
            serial_str = buffer;
        }

        S32 serial_size = serial_str.size();
        
        if(serial_str.size() > 0)
        {
            S32 j = 0;
            while (j < serial_size)
            {
                for (S32 i = 0; i < len; i++)
                {
                    if (j >= serial_size)
                        break;

                    unique_id[i] = (unsigned int)(unique_id[i] + serial_str[j]);
                    j++;
                }
            }
            return true;
        }
    }
    return false;
}
#endif

// get an unique machine id.
// NOT THREAD SAFE - do before setting up threads.
// MAC Address doesn't work for Windows 7 since the first returned hardware MAC address changes with each reboot,  Go figure??

S32 LLMachineID::init()
{
    size_t len = sizeof(static_unique_id);
    memset(static_unique_id, 0, len);
    S32 ret_code = 0;
#if LL_WINDOWS

    LLWMIMethods comInit;

    if (comInit.getWindowsProductNumber(static_legacy_id, len))
    {
        // Bios id can change on windows update, so it is not the best id to use
        // but since old viewer already use them, we might need this id to decode
        // passwords
        has_static_legacy_id = true;
    }

    // Try motherboard/bios id, if it is present it is supposed to be sufficiently unique
    if (comInit.getComputerSystemProductUUID(static_unique_id, len))
    {
        has_static_unique_id = true;
        LL_DEBUGS("AppInit") << "Using product uuid as unique id" << LL_ENDL;
    }

    // Fallback to legacy
    if (!has_static_unique_id)
    {
        if (has_static_legacy_id)
        {
            memcpy(static_unique_id, &static_legacy_id, len);
            // Since ids are identical, mark legacy as not present
            // to not cause retry's in sechandler
            has_static_legacy_id = false;
            has_static_unique_id = true;
            LL_DEBUGS("AppInit") << "Using legacy serial" << LL_ENDL;
        }
        else
        {
            return 1; // Program has failed.
        }
    }

    ret_code=0;
#elif LL_DARWIN
    if (getSerialNumber(static_unique_id, len))
    {
        has_static_unique_id = true;
        LL_DEBUGS("AppInit") << "Using Serial number as unique id" << LL_ENDL;
    }

    {
        unsigned char * staticPtr = (unsigned char *)(&static_legacy_id[0]);
        ret_code = LLUUID::getNodeID(staticPtr);
        has_static_legacy_id = true;
    }

    // Fallback to legacy
    if (!has_static_unique_id)
    {
        if (has_static_legacy_id)
        {
            memcpy(static_unique_id, &static_legacy_id, len);
            // Since ids are identical, mark legacy as not present
            // to not cause retry's in sechandler
            has_static_legacy_id = false;
            has_static_unique_id = true;
            LL_DEBUGS("AppInit") << "Using legacy serial" << LL_ENDL;
        }
    }
#else
    unsigned char * staticPtr = (unsigned char *)(&static_unique_id[0]);
    ret_code = LLUUID::getNodeID(staticPtr);
    has_static_unique_id = true;
    has_static_legacy_id = false;
#endif

        LL_INFOS("AppInit") << "UniqueID: 0x";
        // Code between here and LL_ENDL is not executed unless the LL_DEBUGS
        // actually produces output
        for (size_t i = 0; i < len; ++i)
        {
            // Copy each char to unsigned int to hexify. Sending an unsigned
            // char to a std::ostream tries to represent it as a char, not
            // what we want here.
            unsigned byte = static_unique_id[i];
            LL_CONT << std::hex << std::setw(2) << std::setfill('0') << byte;
        }
        // Reset default output formatting to avoid nasty surprises!
        LL_CONT << std::dec << std::setw(0) << std::setfill(' ') << LL_ENDL;

        return ret_code;
}


S32 LLMachineID::getUniqueID(unsigned char *unique_id, size_t len)
{
    if (has_static_unique_id)
    {
        memcpy ( unique_id, &static_unique_id, len);
        return 1;
    }
    return 0;
}

S32 LLMachineID::getLegacyID(unsigned char *unique_id, size_t len)
{
    if (has_static_legacy_id)
    {
        memcpy(unique_id, &static_legacy_id, len);
        return 1;
    }
    return 0;
}
