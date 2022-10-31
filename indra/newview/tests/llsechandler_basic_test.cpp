/** 
 * @file llsechandler_basic_test.cpp
 * @author Roxie
 * @date 2009-02-10
 * @brief Test the 'basic' sec handler functions
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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
#include "../llviewerprecompiledheaders.h"
#include "../test/lltut.h"
#include "../llsecapi.h"
#include "../llsechandler_basic.h"
#include "../../llxml/llcontrol.h"
#include "../llviewernetwork.h"
#include "lluuid.h"
#include "lldate.h"
#include "llxorcipher.h"
#include "apr_base64.h"
#include <vector>
#include <ios>
#include <llsdserialize.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include "llxorcipher.h"
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "../llmachineid.h"

#define ensure_throws(str, exc_type, cert, func, ...) \
try \
{ \
func(__VA_ARGS__); \
fail("throws, " str); \
} \
catch(exc_type& except) \
{ \
LLSD cert_data; \
cert->getLLSD(cert_data); \
ensure("Exception cert is incorrect for " str, valueCompareLLSD(except.getCertData(), cert_data)); \
}

extern bool _cert_hostname_wildcard_match(const std::string& hostname, const std::string& wildcard_string);

//----------------------------------------------------------------------------               
// Mock objects for the dependencies of the code we're testing                               

std::string gFirstName;
std::string gLastName;
LLControlGroup::LLControlGroup(const std::string& name)
: LLInstanceTracker<LLControlGroup, std::string>(name) {}
LLControlGroup::~LLControlGroup() {}
LLControlVariable* LLControlGroup::declareString(const std::string& name,
                                   const std::string& initial_val,
                                   const std::string& comment,
                                   LLControlVariable::ePersist persist) {return NULL;}
void LLControlGroup::setString(const std::string& name, const std::string& val){}
std::string LLControlGroup::getString(const std::string& name)
{

    if (name == "FirstName")
        return gFirstName;
    else if (name == "LastName")
        return gLastName;
    return "";
}

// Stub for --no-verify-ssl-cert
BOOL LLControlGroup::getBOOL(const std::string& name) { return FALSE; }

LLSD LLCredential::getLoginParams()
{
    LLSD result = LLSD::emptyMap();
    
    // legacy credential
    result["passwd"] = "$1$testpasssd";
    result["first"] = "myfirst";
    result["last"] ="mylast";
    return result;
}

void LLCredential::identifierType(std::string &idType)
{
}

void LLCredential::authenticatorType(std::string &idType)
{
}


LLControlGroup gSavedSettings("test");
unsigned char gMACAddress[MAC_ADDRESS_BYTES] = {77,21,46,31,89,2};


S32 LLMachineID::getUniqueID(unsigned char *unique_id, size_t len)
{
    memcpy(unique_id, gMACAddress, len);
    return 1;
}
S32 LLMachineID::getLegacyID(unsigned char *unique_id, size_t len)
{
    return 0;
}
S32 LLMachineID::init() { return 1; }
    

LLCertException::LLCertException(const LLSD& cert_data, const std::string& msg)
    : LLException(msg),
    mCertData(cert_data)
{
    LL_WARNS("SECAPI") << "Certificate Error: " << msg << LL_ENDL;
}


// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
    const std::string mPemTestCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number:\n"
        "            04:00:00:00:00:01:15:4b:5a:c3:94\n"
        "    Signature Algorithm: sha1WithRSAEncryption\n"
        "        Issuer: C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA\n"
        "        Validity\n"
        "            Not Before: Sep  1 12:00:00 1998 GMT\n"
        "            Not After : Jan 28 12:00:00 2028 GMT\n"
        "        Subject: C=BE, O=GlobalSign nv-sa, OU=Root CA, CN=GlobalSign Root CA\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (2048 bit)\n"
        "                Modulus:\n"
        "                    00:da:0e:e6:99:8d:ce:a3:e3:4f:8a:7e:fb:f1:8b:\n"
        "                    83:25:6b:ea:48:1f:f1:2a:b0:b9:95:11:04:bd:f0:\n"
        "                    63:d1:e2:67:66:cf:1c:dd:cf:1b:48:2b:ee:8d:89:\n"
        "                    8e:9a:af:29:80:65:ab:e9:c7:2d:12:cb:ab:1c:4c:\n"
        "                    70:07:a1:3d:0a:30:cd:15:8d:4f:f8:dd:d4:8c:50:\n"
        "                    15:1c:ef:50:ee:c4:2e:f7:fc:e9:52:f2:91:7d:e0:\n"
        "                    6d:d5:35:30:8e:5e:43:73:f2:41:e9:d5:6a:e3:b2:\n"
        "                    89:3a:56:39:38:6f:06:3c:88:69:5b:2a:4d:c5:a7:\n"
        "                    54:b8:6c:89:cc:9b:f9:3c:ca:e5:fd:89:f5:12:3c:\n"
        "                    92:78:96:d6:dc:74:6e:93:44:61:d1:8d:c7:46:b2:\n"
        "                    75:0e:86:e8:19:8a:d5:6d:6c:d5:78:16:95:a2:e9:\n"
        "                    c8:0a:38:eb:f2:24:13:4f:73:54:93:13:85:3a:1b:\n"
        "                    bc:1e:34:b5:8b:05:8c:b9:77:8b:b1:db:1f:20:91:\n"
        "                    ab:09:53:6e:90:ce:7b:37:74:b9:70:47:91:22:51:\n"
        "                    63:16:79:ae:b1:ae:41:26:08:c8:19:2b:d1:46:aa:\n"
        "                    48:d6:64:2a:d7:83:34:ff:2c:2a:c1:6c:19:43:4a:\n"
        "                    07:85:e7:d3:7c:f6:21:68:ef:ea:f2:52:9f:7f:93:\n"
        "                    90:cf\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Key Usage: critical\n"
        "                Certificate Sign, CRL Sign\n"
        "            X509v3 Basic Constraints: critical\n"
        "                CA:TRUE\n"
        "            X509v3 Subject Key Identifier: \n"
        "                60:7B:66:1A:45:0D:97:CA:89:50:2F:7D:04:CD:34:A8:FF:FC:FD:4B\n"
        "    Signature Algorithm: sha1WithRSAEncryption\n"
        "         d6:73:e7:7c:4f:76:d0:8d:bf:ec:ba:a2:be:34:c5:28:32:b5:\n"
        "         7c:fc:6c:9c:2c:2b:bd:09:9e:53:bf:6b:5e:aa:11:48:b6:e5:\n"
        "         08:a3:b3:ca:3d:61:4d:d3:46:09:b3:3e:c3:a0:e3:63:55:1b:\n"
        "         f2:ba:ef:ad:39:e1:43:b9:38:a3:e6:2f:8a:26:3b:ef:a0:50:\n"
        "         56:f9:c6:0a:fd:38:cd:c4:0b:70:51:94:97:98:04:df:c3:5f:\n"
        "         94:d5:15:c9:14:41:9c:c4:5d:75:64:15:0d:ff:55:30:ec:86:\n"
        "         8f:ff:0d:ef:2c:b9:63:46:f6:aa:fc:df:bc:69:fd:2e:12:48:\n"
        "         64:9a:e0:95:f0:a6:ef:29:8f:01:b1:15:b5:0c:1d:a5:fe:69:\n"
        "         2c:69:24:78:1e:b3:a7:1c:71:62:ee:ca:c8:97:ac:17:5d:8a:\n"
        "         c2:f8:47:86:6e:2a:c4:56:31:95:d0:67:89:85:2b:f9:6c:a6:\n"
        "         5d:46:9d:0c:aa:82:e4:99:51:dd:70:b7:db:56:3d:61:e4:6a:\n"
        "         e1:5c:d6:f6:fe:3d:de:41:cc:07:ae:63:52:bf:53:53:f4:2b:\n"
        "         e9:c7:fd:b6:f7:82:5f:85:d2:41:18:db:81:b3:04:1c:c5:1f:\n"
        "         a4:80:6f:15:20:c9:de:0c:88:0a:1d:d6:66:55:e2:fc:48:c9:\n"
        "         29:26:69:e0\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
        "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
        "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
        "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
        "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
        "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
        "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
        "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
        "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
        "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
        "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
        "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
        "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
        "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
        "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
        "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
        "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
        "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
        "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
        "-----END CERTIFICATE-----\n"
                                   );

    /*
     * The following certificates were generated using the instructions at
     * https://jamielinux.com/docs/openssl-certificate-authority/sign-server-and-client-certificates.html
     * with the exception that the server certificate has a longer expiration time, and the full text
     * expansion was included in the certificates.
     */
    const std::string mPemRootCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number:\n"
        "            82:2f:8f:eb:8d:06:24:b0\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: May 22 22:19:45 2018 GMT\n"
        "            Not After : May 17 22:19:45 2038 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:bd:e0:79:dd:3b:a6:ac:87:d0:39:f0:58:c7:a4:\n"
        "                    42:42:f6:5f:93:b0:36:04:b5:e2:d5:f7:2a:c0:6c:\n"
        "                    a0:13:d2:1e:02:81:57:02:50:4c:57:b7:ef:27:9e:\n"
        "                    f6:f1:f1:30:30:72:1e:57:34:e5:3f:82:3c:21:c4:\n"
        "                    66:d2:73:63:6c:91:e6:dd:49:9e:9c:b1:34:6a:81:\n"
        "                    45:a1:6e:c4:50:28:f2:d8:e3:fe:80:2f:83:aa:28:\n"
        "                    91:b4:8c:57:c9:f1:16:d9:0c:87:3c:25:80:a0:81:\n"
        "                    8d:71:f2:96:e2:16:f1:97:c4:b0:d8:53:bb:13:6c:\n"
        "                    73:54:2f:29:94:85:cf:86:6e:75:71:ad:39:e3:fc:\n"
        "                    39:12:53:93:1c:ce:39:e0:33:da:49:b7:3d:af:b0:\n"
        "                    37:ce:77:09:03:27:32:70:c0:9c:7f:9c:89:ce:90:\n"
        "                    45:b0:7d:94:8b:ff:13:27:ba:88:7f:ae:c4:aa:73:\n"
        "                    d5:47:b8:87:69:89:80:0c:c1:22:18:78:c2:0d:47:\n"
        "                    d9:10:ff:80:79:0d:46:71:ec:d9:ba:c9:f3:77:fd:\n"
        "                    92:6d:1f:0f:d9:54:18:6d:f6:72:24:5c:5c:3d:43:\n"
        "                    49:35:3e:1c:28:de:7e:44:dc:29:c3:9f:62:04:46:\n"
        "                    aa:c4:e6:69:6a:15:f8:e3:74:1c:14:e9:f4:97:7c:\n"
        "                    30:6c:d4:28:fc:2a:0e:1d:6d:39:2e:1d:f9:17:43:\n"
        "                    35:5d:23:e7:ba:e3:a8:e9:97:6b:3c:3e:23:ef:d8:\n"
        "                    bc:fb:7a:57:37:39:93:59:03:fc:78:ca:b1:31:ef:\n"
        "                    26:19:ed:56:e1:63:c3:ad:99:80:5b:47:b5:03:35:\n"
        "                    5f:fe:6a:a6:21:63:ec:50:fb:4e:c9:f9:ae:a5:66:\n"
        "                    d0:55:33:8d:e6:c5:50:5a:c6:8f:5c:34:45:a7:72:\n"
        "                    da:50:f6:66:4c:19:f5:d1:e4:fb:11:8b:a1:b5:4e:\n"
        "                    09:43:81:3d:39:28:86:3b:fe:07:28:97:02:b5:3a:\n"
        "                    07:5f:4a:20:80:1a:7d:a4:8c:f7:6c:f6:c5:9b:f6:\n"
        "                    61:e5:c7:b0:c3:d5:58:38:7b:bb:47:1e:34:d6:16:\n"
        "                    55:c5:d2:6c:b0:93:77:b1:90:69:06:b1:53:cb:1b:\n"
        "                    84:71:cf:b8:87:1b:1e:44:35:b4:2b:bb:04:59:58:\n"
        "                    0b:e8:93:d8:ae:21:9b:b1:1c:89:30:ae:11:80:77:\n"
        "                    cc:16:f3:d6:35:ed:a1:b3:70:b3:4f:cd:a1:56:99:\n"
        "                    ee:0e:c0:00:a4:09:70:c3:5b:0b:be:a1:07:18:dd:\n"
        "                    c6:f4:6d:8b:58:bc:f9:bb:4b:01:2c:f6:cc:2c:9b:\n"
        "                    87:0e:b1:4f:9c:10:be:fc:45:e2:a4:ec:7e:fc:ff:\n"
        "                    45:b8:53\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Subject Key Identifier: \n"
        "                8A:22:C6:9C:2E:11:F3:40:0C:CE:82:0C:22:59:FF:F8:7F:D0:B9:13\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:8A:22:C6:9C:2E:11:F3:40:0C:CE:82:0C:22:59:FF:F8:7F:D0:B9:13\n"
        "\n"
        "            X509v3 Basic Constraints: critical\n"
        "                CA:TRUE\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Certificate Sign, CRL Sign\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         b3:cb:33:eb:0e:02:64:f4:55:9a:3d:03:9a:cf:6a:4c:18:43:\n"
        "         f7:42:cb:65:dc:61:52:e5:9f:2f:42:97:3c:93:16:22:d4:af:\n"
        "         ae:b2:0f:c3:9b:ef:e0:cc:ee:b6:b1:69:a3:d8:da:26:c3:ad:\n"
        "         3b:c5:64:dc:9f:d4:c2:53:4b:91:6d:c4:92:09:0b:ac:f0:99:\n"
        "         be:6f:b9:3c:03:4a:6d:9f:01:5d:ec:5a:9a:f3:a7:e5:3b:2c:\n"
        "         99:57:7d:7e:25:15:68:20:12:30:96:16:86:f5:db:74:90:60:\n"
        "         fe:8b:df:99:f6:f7:62:49:9f:bc:8d:45:23:0a:c8:73:b8:79:\n"
        "         80:3c:b9:e5:72:85:4b:b3:81:66:74:a2:72:92:4c:44:fd:7b:\n"
        "         46:2e:21:a2:a9:81:a2:f3:26:4d:e3:89:7d:78:b0:c6:6f:b5:\n"
        "         87:cb:ee:25:ed:27:1f:75:13:fa:6d:e9:37:73:ad:07:bb:af:\n"
        "         d3:6c:87:ea:02:01:70:bd:53:aa:ce:39:2c:d4:66:39:33:aa:\n"
        "         d1:9c:ee:67:e3:a9:45:d2:7b:2e:54:09:af:70:5f:3f:5a:67:\n"
        "         2e:6c:72:ef:e0:9d:92:28:4a:df:ba:0b:b7:23:ca:5b:04:11:\n"
        "         45:d1:51:e9:ea:c9:ec:54:fa:34:46:ae:fc:dc:6c:f8:1e:2c:\n"
        "         9e:f4:71:51:8d:b5:a1:26:9a:13:30:be:1e:41:25:59:58:05:\n"
        "         2c:64:c8:f9:5e:38:ae:dc:93:b0:8a:d6:38:74:02:cb:ce:ce:\n"
        "         95:31:76:f6:7c:bf:a4:a1:8e:27:fd:ca:74:82:d1:e1:4d:b6:\n"
        "         48:51:fa:c5:17:59:22:a3:84:be:82:c8:83:ec:61:a0:f4:ee:\n"
        "         2c:e3:a3:ea:e5:51:c9:d3:4f:db:85:bd:ba:7a:52:14:b6:03:\n"
        "         ed:43:17:d8:d7:1c:22:5e:c9:56:d9:d6:81:96:11:e3:5e:01:\n"
        "         40:91:30:09:da:a3:5f:d3:27:60:e5:9d:6c:da:d0:f0:39:01:\n"
        "         23:4a:a6:15:7a:4a:82:eb:ec:72:4a:1d:36:dc:6f:83:c4:85:\n"
        "         84:b5:8d:cd:09:e5:12:63:f3:21:56:c8:64:6b:db:b8:cf:d4:\n"
        "         df:ca:a8:24:8e:df:8d:63:a5:96:84:bf:ff:8b:7e:46:7a:f0:\n"
        "         c7:73:7c:70:8a:f5:17:d0:ac:c8:89:1e:d7:89:42:0f:4d:66:\n"
        "         c4:d8:bb:36:a8:ae:ca:e1:cf:e2:88:f6:cf:b0:44:4a:5f:81:\n"
        "         50:4b:d6:28:81:cd:6c:f0:ec:e6:09:08:f2:59:91:a2:69:ac:\n"
        "         c7:81:fa:ab:61:3e:db:6f:f6:7f:db:1a:9e:b9:5d:cc:cc:33:\n"
        "         fa:95:c6:f7:8d:4b:30:f3\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGXDCCBESgAwIBAgIJAIIvj+uNBiSwMA0GCSqGSIb3DQEBCwUAMIG6MQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\n"
        "aXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUg\n"
        "RW5naW5lZXJpbmcxITAfBgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEk\n"
        "MCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMB4XDTE4MDUyMjIy\n"
        "MTk0NVoXDTM4MDUxNzIyMTk0NVowgboxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApD\n"
        "YWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMwEQYDVQQKDApMaW5k\n"
        "ZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEhMB8GA1UE\n"
        "AwwYSW50ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZIhvcNAQkBFhVub3Jl\n"
        "cGx5QGxpbmRlbmxhYi5jb20wggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC\n"
        "AQC94HndO6ash9A58FjHpEJC9l+TsDYEteLV9yrAbKAT0h4CgVcCUExXt+8nnvbx\n"
        "8TAwch5XNOU/gjwhxGbSc2NskebdSZ6csTRqgUWhbsRQKPLY4/6AL4OqKJG0jFfJ\n"
        "8RbZDIc8JYCggY1x8pbiFvGXxLDYU7sTbHNULymUhc+GbnVxrTnj/DkSU5Mczjng\n"
        "M9pJtz2vsDfOdwkDJzJwwJx/nInOkEWwfZSL/xMnuoh/rsSqc9VHuIdpiYAMwSIY\n"
        "eMINR9kQ/4B5DUZx7Nm6yfN3/ZJtHw/ZVBht9nIkXFw9Q0k1Phwo3n5E3CnDn2IE\n"
        "RqrE5mlqFfjjdBwU6fSXfDBs1Cj8Kg4dbTkuHfkXQzVdI+e646jpl2s8PiPv2Lz7\n"
        "elc3OZNZA/x4yrEx7yYZ7VbhY8OtmYBbR7UDNV/+aqYhY+xQ+07J+a6lZtBVM43m\n"
        "xVBaxo9cNEWnctpQ9mZMGfXR5PsRi6G1TglDgT05KIY7/gcolwK1OgdfSiCAGn2k\n"
        "jPds9sWb9mHlx7DD1Vg4e7tHHjTWFlXF0mywk3exkGkGsVPLG4Rxz7iHGx5ENbQr\n"
        "uwRZWAvok9iuIZuxHIkwrhGAd8wW89Y17aGzcLNPzaFWme4OwACkCXDDWwu+oQcY\n"
        "3cb0bYtYvPm7SwEs9swsm4cOsU+cEL78ReKk7H78/0W4UwIDAQABo2MwYTAdBgNV\n"
        "HQ4EFgQUiiLGnC4R80AMzoIMIln/+H/QuRMwHwYDVR0jBBgwFoAUiiLGnC4R80AM\n"
        "zoIMIln/+H/QuRMwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwDQYJ\n"
        "KoZIhvcNAQELBQADggIBALPLM+sOAmT0VZo9A5rPakwYQ/dCy2XcYVLlny9ClzyT\n"
        "FiLUr66yD8Ob7+DM7raxaaPY2ibDrTvFZNyf1MJTS5FtxJIJC6zwmb5vuTwDSm2f\n"
        "AV3sWprzp+U7LJlXfX4lFWggEjCWFob123SQYP6L35n292JJn7yNRSMKyHO4eYA8\n"
        "ueVyhUuzgWZ0onKSTET9e0YuIaKpgaLzJk3jiX14sMZvtYfL7iXtJx91E/pt6Tdz\n"
        "rQe7r9Nsh+oCAXC9U6rOOSzUZjkzqtGc7mfjqUXSey5UCa9wXz9aZy5scu/gnZIo\n"
        "St+6C7cjylsEEUXRUenqyexU+jRGrvzcbPgeLJ70cVGNtaEmmhMwvh5BJVlYBSxk\n"
        "yPleOK7ck7CK1jh0AsvOzpUxdvZ8v6Shjif9ynSC0eFNtkhR+sUXWSKjhL6CyIPs\n"
        "YaD07izjo+rlUcnTT9uFvbp6UhS2A+1DF9jXHCJeyVbZ1oGWEeNeAUCRMAnao1/T\n"
        "J2DlnWza0PA5ASNKphV6SoLr7HJKHTbcb4PEhYS1jc0J5RJj8yFWyGRr27jP1N/K\n"
        "qCSO341jpZaEv/+LfkZ68MdzfHCK9RfQrMiJHteJQg9NZsTYuzaorsrhz+KI9s+w\n"
        "REpfgVBL1iiBzWzw7OYJCPJZkaJprMeB+qthPttv9n/bGp65XczMM/qVxveNSzDz\n"
        "-----END CERTIFICATE-----\n"
                                   );            

    const std::string mPemIntermediateCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number: 4096 (0x1000)\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: May 22 22:39:08 2018 GMT\n"
        "            Not After : May 19 22:39:08 2028 GMT\n"
        "        Subject: C=US, ST=California, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Intermediate CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:ce:a3:70:e2:c4:fb:4b:97:90:a1:30:bb:c1:1b:\n"
        "                    13:b9:aa:7e:46:17:a3:26:8d:69:3f:5e:73:95:e8:\n"
        "                    6a:b1:0a:b4:8f:50:65:e3:c6:5c:39:24:34:df:0b:\n"
        "                    b7:cc:ce:62:0c:36:5a:12:2c:fe:35:4c:e9:1c:ac:\n"
        "                    80:5e:24:99:d7:aa:bd:be:48:c0:62:64:77:36:88:\n"
        "                    66:ce:f4:a8:dd:d2:76:24:62:90:55:41:fc:1d:13:\n"
        "                    4e:a7:4e:57:bc:a8:a4:59:4b:2c:5a:1c:d8:cc:16:\n"
        "                    de:e8:88:30:c9:95:df:2f:a6:14:28:0f:eb:34:46:\n"
        "                    12:58:ba:da:0e:e6:de:9c:15:f6:f4:e3:9f:74:aa:\n"
        "                    70:89:79:8b:e9:5a:7b:18:54:15:94:3a:23:0a:65:\n"
        "                    78:05:d9:33:90:2a:ce:15:18:0d:52:fc:5c:31:65:\n"
        "                    20:d0:12:37:8c:11:80:ba:d4:b0:82:73:00:4b:49:\n"
        "                    be:cb:d6:bc:e7:cd:61:f3:00:98:99:74:5a:37:81:\n"
        "                    49:96:7e:14:01:1b:86:d2:d0:06:94:40:63:63:46:\n"
        "                    11:fc:33:5c:bd:3a:5e:d4:e5:44:47:64:50:bd:a6:\n"
        "                    97:55:70:64:9b:26:cc:de:20:82:90:6a:83:41:9c:\n"
        "                    6f:71:47:14:be:cb:68:7c:85:be:ef:2e:76:12:19:\n"
        "                    d3:c9:87:32:b4:ac:60:20:16:28:2d:af:bc:e8:01:\n"
        "                    c6:7f:fb:d8:11:d5:f4:b7:14:bd:27:08:5b:72:be:\n"
        "                    09:e0:91:c8:9c:7b:b4:b3:12:ef:32:36:be:b1:b9:\n"
        "                    a2:b7:e3:69:47:30:76:ba:9c:9b:19:99:4d:53:dd:\n"
        "                    5c:e8:2c:f1:b2:64:69:cf:15:bd:f8:bb:58:95:73:\n"
        "                    58:38:95:b4:7a:cf:84:29:a6:c2:db:f0:bd:ef:97:\n"
        "                    26:d4:99:ac:d7:c7:be:b0:0d:11:f4:26:86:2d:77:\n"
        "                    42:52:25:d7:56:c7:e3:97:b1:36:5c:97:71:d0:9b:\n"
        "                    f5:b5:50:8d:f9:ff:fb:10:77:3c:b5:53:6d:a1:43:\n"
        "                    35:a9:03:32:05:ab:d7:f5:d1:19:bd:5f:92:a3:00:\n"
        "                    2a:79:37:a4:76:4f:e9:32:0d:e4:86:bb:ea:c3:1a:\n"
        "                    c5:33:e8:16:d4:a5:d8:e0:e8:bb:c2:f0:22:15:e2:\n"
        "                    d9:8c:ae:ac:7d:2b:bf:eb:a3:4c:3b:29:1d:94:ac:\n"
        "                    a3:bb:6d:ba:6d:03:91:03:cf:46:12:c4:66:21:c5:\n"
        "                    c6:67:d8:11:19:79:01:0e:6e:84:1c:76:6f:11:3d:\n"
        "                    eb:94:89:c5:6a:26:1f:cd:e0:11:8b:51:ee:99:35:\n"
        "                    69:e5:7f:0b:77:2a:94:e4:4b:64:b9:83:04:30:05:\n"
        "                    e4:a2:e3\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Subject Key Identifier: \n"
        "                83:21:DE:EC:C0:79:03:6D:1E:83:F3:E5:97:29:D5:5A:C0:96:40:FA\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:8A:22:C6:9C:2E:11:F3:40:0C:CE:82:0C:22:59:FF:F8:7F:D0:B9:13\n"
        "\n"
        "            X509v3 Basic Constraints: critical\n"
        "                CA:TRUE, pathlen:0\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Certificate Sign, CRL Sign\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         a3:6c:85:9a:2e:4e:7e:5d:83:63:0f:f5:4f:a9:7d:ec:0e:6f:\n"
        "         ae:d7:ba:df:64:e0:46:0e:3d:da:18:15:2c:f3:73:ca:81:b1:\n"
        "         10:d9:53:14:21:7d:72:5c:94:88:a5:9d:ad:ab:45:42:c6:64:\n"
        "         a9:d9:2e:4e:29:47:2c:b1:95:07:b7:62:48:68:1f:68:13:1c:\n"
        "         d2:a0:fb:5e:38:24:4a:82:0a:87:c9:93:20:43:7e:e9:f9:79:\n"
        "         ef:03:a2:bd:9e:24:6b:0a:01:5e:4a:36:c5:7d:7a:fe:d6:aa:\n"
        "         2f:c2:8c:38:8a:99:3c:b0:6a:e5:60:be:56:d6:eb:60:03:55:\n"
        "         24:42:a0:1a:fa:91:24:a3:53:15:75:5d:c8:eb:7c:1e:68:5a:\n"
        "         7e:13:34:e3:85:37:1c:76:3f:77:67:1b:ed:1b:52:17:fc:4a:\n"
        "         a3:e2:74:84:80:2c:69:fc:dd:7d:26:97:c4:2a:69:7d:9c:dc:\n"
        "         61:97:70:29:a7:3f:2b:5b:2b:22:51:fd:fe:6a:5d:f9:e7:14:\n"
        "         48:b7:2d:c8:33:58:fc:f2:5f:27:f7:26:16:be:be:b5:aa:a2:\n"
        "         64:53:3c:69:e8:b5:61:eb:ab:91:a5:b4:09:9b:f6:98:b8:5c:\n"
        "         5b:24:2f:93:f5:2b:9c:8c:58:fb:26:3f:67:53:d7:42:64:e8:\n"
        "         79:77:73:41:4e:e3:02:39:0b:b6:68:97:8b:84:e8:1d:83:a8:\n"
        "         15:f1:06:46:47:80:42:5e:14:e2:61:8a:76:84:d5:d4:71:7f:\n"
        "         4e:ff:d9:74:87:ff:32:c5:87:20:0a:d4:59:40:3e:d8:17:ef:\n"
        "         da:65:e9:0a:51:fe:1e:c3:46:91:d2:ee:e4:23:57:97:87:d4:\n"
        "         a6:a5:eb:ef:81:6a:d8:8c:d6:1f:8e:b1:18:4c:6b:89:32:55:\n"
        "         53:68:26:9e:bb:03:be:2c:e9:8b:ff:97:9c:1c:ac:28:c3:9f:\n"
        "         0b:b7:93:23:24:31:63:e4:19:13:f2:bb:08:71:b7:c5:c5:c4:\n"
        "         10:ff:dc:fc:33:54:a4:5e:ec:a3:fe:0a:80:ca:9c:bc:95:6f:\n"
        "         5f:39:91:3b:61:69:16:94:0f:57:4b:fc:4b:b1:be:72:98:5d:\n"
        "         10:f9:08:a7:d6:e0:e8:3d:5d:54:7d:fa:4b:6a:dd:98:41:ed:\n"
        "         84:a1:39:67:5c:6c:7f:0c:b0:e1:98:c1:14:ed:fe:1e:e8:05:\n"
        "         8d:7f:6a:24:cb:1b:05:42:0d:7f:13:ba:ca:b5:91:db:a5:f0:\n"
        "         40:2b:70:7a:2a:a5:5d:ed:56:0c:f0:c2:72:ee:63:dd:cb:5d:\n"
        "         76:f6:08:e6:e6:30:ef:3a:b2:16:34:41:a4:e1:30:14:bc:c7:\n"
        "         f9:23:3a:1a:70:df:b8:cc\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGSDCCBDCgAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwgboxCzAJBgNVBAYTAlVT\n"
        "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMw\n"
        "EQYDVQQKDApMaW5kZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVl\n"
        "cmluZzEhMB8GA1UEAwwYSW50ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZI\n"
        "hvcNAQkBFhVub3JlcGx5QGxpbmRlbmxhYi5jb20wHhcNMTgwNTIyMjIzOTA4WhcN\n"
        "MjgwNTE5MjIzOTA4WjCBqjELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3Ju\n"
        "aWExEzARBgNVBAoMCkxpbmRlbiBMYWIxIDAeBgNVBAsMF1NlY29uZCBMaWZlIEVu\n"
        "Z2luZWVyaW5nMSkwJwYDVQQDDCBJbnRlZ3JhdGlvbiBUZXN0IEludGVybWVkaWF0\n"
        "ZSBDQTEkMCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMIICIjAN\n"
        "BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAzqNw4sT7S5eQoTC7wRsTuap+Rhej\n"
        "Jo1pP15zlehqsQq0j1Bl48ZcOSQ03wu3zM5iDDZaEiz+NUzpHKyAXiSZ16q9vkjA\n"
        "YmR3NohmzvSo3dJ2JGKQVUH8HRNOp05XvKikWUssWhzYzBbe6IgwyZXfL6YUKA/r\n"
        "NEYSWLraDubenBX29OOfdKpwiXmL6Vp7GFQVlDojCmV4BdkzkCrOFRgNUvxcMWUg\n"
        "0BI3jBGAutSwgnMAS0m+y9a8581h8wCYmXRaN4FJln4UARuG0tAGlEBjY0YR/DNc\n"
        "vTpe1OVER2RQvaaXVXBkmybM3iCCkGqDQZxvcUcUvstofIW+7y52EhnTyYcytKxg\n"
        "IBYoLa+86AHGf/vYEdX0txS9Jwhbcr4J4JHInHu0sxLvMja+sbmit+NpRzB2upyb\n"
        "GZlNU91c6CzxsmRpzxW9+LtYlXNYOJW0es+EKabC2/C975cm1Jms18e+sA0R9CaG\n"
        "LXdCUiXXVsfjl7E2XJdx0Jv1tVCN+f/7EHc8tVNtoUM1qQMyBavX9dEZvV+SowAq\n"
        "eTekdk/pMg3khrvqwxrFM+gW1KXY4Oi7wvAiFeLZjK6sfSu/66NMOykdlKyju226\n"
        "bQORA89GEsRmIcXGZ9gRGXkBDm6EHHZvET3rlInFaiYfzeARi1HumTVp5X8LdyqU\n"
        "5EtkuYMEMAXkouMCAwEAAaNmMGQwHQYDVR0OBBYEFIMh3uzAeQNtHoPz5Zcp1VrA\n"
        "lkD6MB8GA1UdIwQYMBaAFIoixpwuEfNADM6CDCJZ//h/0LkTMBIGA1UdEwEB/wQI\n"
        "MAYBAf8CAQAwDgYDVR0PAQH/BAQDAgGGMA0GCSqGSIb3DQEBCwUAA4ICAQCjbIWa\n"
        "Lk5+XYNjD/VPqX3sDm+u17rfZOBGDj3aGBUs83PKgbEQ2VMUIX1yXJSIpZ2tq0VC\n"
        "xmSp2S5OKUcssZUHt2JIaB9oExzSoPteOCRKggqHyZMgQ37p+XnvA6K9niRrCgFe\n"
        "SjbFfXr+1qovwow4ipk8sGrlYL5W1utgA1UkQqAa+pEko1MVdV3I63weaFp+EzTj\n"
        "hTccdj93ZxvtG1IX/Eqj4nSEgCxp/N19JpfEKml9nNxhl3Appz8rWysiUf3+al35\n"
        "5xRIty3IM1j88l8n9yYWvr61qqJkUzxp6LVh66uRpbQJm/aYuFxbJC+T9SucjFj7\n"
        "Jj9nU9dCZOh5d3NBTuMCOQu2aJeLhOgdg6gV8QZGR4BCXhTiYYp2hNXUcX9O/9l0\n"
        "h/8yxYcgCtRZQD7YF+/aZekKUf4ew0aR0u7kI1eXh9SmpevvgWrYjNYfjrEYTGuJ\n"
        "MlVTaCaeuwO+LOmL/5ecHKwow58Lt5MjJDFj5BkT8rsIcbfFxcQQ/9z8M1SkXuyj\n"
        "/gqAypy8lW9fOZE7YWkWlA9XS/xLsb5ymF0Q+Qin1uDoPV1UffpLat2YQe2EoTln\n"
        "XGx/DLDhmMEU7f4e6AWNf2okyxsFQg1/E7rKtZHbpfBAK3B6KqVd7VYM8MJy7mPd\n"
        "y1129gjm5jDvOrIWNEGk4TAUvMf5IzoacN+4zA==\n"
        "-----END CERTIFICATE-----\n"
                                           );

    const std::string mPemChildCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number: 4096 (0x1000)\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Intermediate CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: May 22 22:58:15 2018 GMT\n"
        "            Not After : Jul 19 22:58:15 2024 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Server Cert/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (2048 bit)\n"
        "                Modulus:\n"
        "                    00:bf:a1:1c:76:82:4a:10:1d:25:0e:02:e2:7a:64:\n"
        "                    54:c7:94:c5:c0:98:d5:35:f3:cb:cb:30:ba:31:9c:\n"
        "                    bd:4c:2f:4a:4e:24:03:4b:87:5c:c1:5c:fe:d9:89:\n"
        "                    3b:cb:01:bc:eb:a5:b7:78:dc:b3:58:e5:78:a7:15:\n"
        "                    34:50:30:aa:16:3a:b2:94:17:6d:1e:7f:b2:70:1e:\n"
        "                    96:41:bb:1d:e3:22:80:fa:dc:00:6a:fb:34:3e:67:\n"
        "                    e7:c2:21:2f:1b:d3:af:04:49:91:eb:bb:60:e0:26:\n"
        "                    52:75:28:8a:08:5b:91:56:4e:51:50:40:51:70:af:\n"
        "                    cb:80:66:c8:59:e9:e2:48:a8:62:d0:26:67:80:0a:\n"
        "                    12:16:d1:f6:15:9e:1f:f5:92:37:f3:c9:2f:03:9e:\n"
        "                    22:f6:60:5a:76:45:8c:01:2c:99:54:72:19:db:b7:\n"
        "                    72:e6:5a:69:f3:e9:31:65:5d:0f:c7:5c:9c:17:29:\n"
        "                    71:14:7f:db:47:c9:1e:65:a2:41:b0:2f:14:17:ec:\n"
        "                    4b:25:f2:43:8f:b4:a3:8d:37:1a:07:34:b3:29:bb:\n"
        "                    8a:44:8e:84:08:a2:1b:76:7a:cb:c2:39:2f:6e:e3:\n"
        "                    fc:d6:91:b5:1f:ce:58:91:57:70:35:6e:25:a9:48:\n"
        "                    0e:07:cf:4e:dd:16:42:65:cf:8a:42:b3:27:e6:fe:\n"
        "                    6a:e3\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Basic Constraints: \n"
        "                CA:FALSE\n"
        "            Netscape Cert Type: \n"
        "                SSL Server\n"
        "            Netscape Comment: \n"
        "                OpenSSL Generated Server Certificate\n"
        "            X509v3 Subject Key Identifier: \n"
        "                BB:59:9F:DE:6B:51:A7:6C:B3:6D:5B:8B:42:F7:B1:65:77:17:A4:E4\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:83:21:DE:EC:C0:79:03:6D:1E:83:F3:E5:97:29:D5:5A:C0:96:40:FA\n"
        "                DirName:/C=US/ST=California/L=San Francisco/O=Linden Lab/OU=Second Life Engineering/CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "                serial:10:00\n"
        "\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Key Encipherment\n"
        "            X509v3 Extended Key Usage: \n"
        "                TLS Web Server Authentication\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         18:a6:58:55:9b:d4:af:7d:8a:27:d3:28:3a:4c:4b:42:4e:f0:\n"
        "         30:d6:d9:95:11:48:12:0a:96:40:d9:2b:21:39:c5:d4:8d:e5:\n"
        "         10:bc:68:78:69:0b:9f:15:4a:0b:f1:ab:99:45:0c:20:5f:27:\n"
        "         df:e7:14:2d:4a:30:f2:c2:8d:37:73:36:1a:27:55:5a:08:5f:\n"
        "         71:a1:5e:05:83:b2:59:fe:02:5e:d7:4a:30:15:23:58:04:cf:\n"
        "         48:cc:b0:71:88:9c:6b:57:f0:04:0a:d3:a0:64:6b:ee:f3:5f:\n"
        "         ea:ac:e1:2b:b9:7f:79:b8:db:ce:72:48:72:db:c8:5c:38:72:\n"
        "         31:55:d0:ff:6b:bd:73:23:a7:30:18:5d:ed:47:18:0a:67:8e:\n"
        "         53:32:0e:99:9b:96:72:45:7f:c6:00:2c:5d:1a:97:53:75:3a:\n"
        "         0b:49:3d:3a:00:37:14:67:0c:28:97:34:87:aa:c5:32:e4:ae:\n"
        "         34:83:12:4a:10:f7:0e:74:d4:5f:73:bd:ef:0c:b7:d8:0a:7d:\n"
        "         8e:8d:5a:48:bd:f4:8e:7b:f9:4a:15:3b:61:c9:5e:40:59:6e:\n"
        "         c7:a8:a4:02:28:72:c5:54:8c:77:f4:55:a7:86:c0:38:a0:68:\n"
        "         19:da:0f:72:5a:a9:7e:69:9f:9c:3a:d6:66:aa:e1:f4:fd:f9:\n"
        "         b8:4b:6c:71:9e:f0:38:02:c7:6a:9e:dc:e6:fb:ef:23:59:4f:\n"
        "         5c:84:0a:df:ea:86:1f:fd:0e:5c:fa:c4:e5:50:1c:10:cf:89:\n"
        "         4e:08:0e:4c:4b:61:1a:49:12:f7:e9:4b:17:71:43:7b:6d:b6:\n"
        "         b5:9f:d4:3b:c7:88:53:48:63:b6:00:80:8f:49:0a:c5:7e:58:\n"
        "         ac:78:d8:b9:06:b0:bc:86:e2:2e:48:5b:c3:24:fa:aa:72:d8:\n"
        "         ec:f6:c7:91:9f:0f:c8:b5:fd:2b:b2:a7:bc:2f:40:20:2b:47:\n"
        "         e0:d1:1d:94:52:6f:6b:be:12:b6:8c:dc:11:db:71:e6:19:ef:\n"
        "         a8:71:8b:ad:d3:32:c0:1c:a4:3f:b3:0f:af:e5:50:e1:ff:41:\n"
        "         a4:b7:6f:57:71:af:fd:16:4c:e8:24:b3:99:1b:cf:12:8f:43:\n"
        "         05:80:ba:18:19:0a:a5:ec:49:81:41:4c:7e:28:b2:21:f2:59:\n"
        "         6e:4a:ed:de:f9:fa:99:85:60:1f:e6:c2:42:5c:08:00:3c:84:\n"
        "         06:a9:24:d4:cf:7b:6e:1b:59:1d:f4:70:16:03:a1:e0:0b:00:\n"
        "         95:5c:39:03:fc:9d:1c:8e:f7:59:0c:61:47:f6:7f:07:22:48:\n"
        "         83:40:ac:e1:98:5f:c7:be:05:d5:29:2b:bf:0d:03:0e:e9:5e:\n"
        "         2b:dd:09:18:fe:5e:30:61\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGbjCCBFagAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwgaoxCzAJBgNVBAYTAlVT\n"
        "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRMwEQYDVQQKDApMaW5kZW4gTGFiMSAwHgYD\n"
        "VQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEpMCcGA1UEAwwgSW50ZWdyYXRp\n"
        "b24gVGVzdCBJbnRlcm1lZGlhdGUgQ0ExJDAiBgkqhkiG9w0BCQEWFW5vcmVwbHlA\n"
        "bGluZGVubGFiLmNvbTAeFw0xODA1MjIyMjU4MTVaFw0yNDA3MTkyMjU4MTVaMIG+\n"
        "MQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2Fu\n"
        "IEZyYW5jaXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25k\n"
        "IExpZmUgRW5naW5lZXJpbmcxJTAjBgNVBAMMHEludGVncmF0aW9uIFRlc3QgU2Vy\n"
        "dmVyIENlcnQxJDAiBgkqhkiG9w0BCQEWFW5vcmVwbHlAbGluZGVubGFiLmNvbTCC\n"
        "ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL+hHHaCShAdJQ4C4npkVMeU\n"
        "xcCY1TXzy8swujGcvUwvSk4kA0uHXMFc/tmJO8sBvOult3jcs1jleKcVNFAwqhY6\n"
        "spQXbR5/snAelkG7HeMigPrcAGr7ND5n58IhLxvTrwRJkeu7YOAmUnUoighbkVZO\n"
        "UVBAUXCvy4BmyFnp4kioYtAmZ4AKEhbR9hWeH/WSN/PJLwOeIvZgWnZFjAEsmVRy\n"
        "Gdu3cuZaafPpMWVdD8dcnBcpcRR/20fJHmWiQbAvFBfsSyXyQ4+0o403Ggc0sym7\n"
        "ikSOhAiiG3Z6y8I5L27j/NaRtR/OWJFXcDVuJalIDgfPTt0WQmXPikKzJ+b+auMC\n"
        "AwEAAaOCAYYwggGCMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgZAMDMGCWCG\n"
        "SAGG+EIBDQQmFiRPcGVuU1NMIEdlbmVyYXRlZCBTZXJ2ZXIgQ2VydGlmaWNhdGUw\n"
        "HQYDVR0OBBYEFLtZn95rUadss21bi0L3sWV3F6TkMIHoBgNVHSMEgeAwgd2AFIMh\n"
        "3uzAeQNtHoPz5Zcp1VrAlkD6oYHApIG9MIG6MQswCQYDVQQGEwJVUzETMBEGA1UE\n"
        "CAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzETMBEGA1UECgwK\n"
        "TGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUgRW5naW5lZXJpbmcxITAf\n"
        "BgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEkMCIGCSqGSIb3DQEJARYV\n"
        "bm9yZXBseUBsaW5kZW5sYWIuY29tggIQADAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0l\n"
        "BAwwCgYIKwYBBQUHAwEwDQYJKoZIhvcNAQELBQADggIBABimWFWb1K99iifTKDpM\n"
        "S0JO8DDW2ZURSBIKlkDZKyE5xdSN5RC8aHhpC58VSgvxq5lFDCBfJ9/nFC1KMPLC\n"
        "jTdzNhonVVoIX3GhXgWDsln+Al7XSjAVI1gEz0jMsHGInGtX8AQK06Bka+7zX+qs\n"
        "4Su5f3m4285ySHLbyFw4cjFV0P9rvXMjpzAYXe1HGApnjlMyDpmblnJFf8YALF0a\n"
        "l1N1OgtJPToANxRnDCiXNIeqxTLkrjSDEkoQ9w501F9zve8Mt9gKfY6NWki99I57\n"
        "+UoVO2HJXkBZbseopAIocsVUjHf0VaeGwDigaBnaD3JaqX5pn5w61maq4fT9+bhL\n"
        "bHGe8DgCx2qe3Ob77yNZT1yECt/qhh/9Dlz6xOVQHBDPiU4IDkxLYRpJEvfpSxdx\n"
        "Q3tttrWf1DvHiFNIY7YAgI9JCsV+WKx42LkGsLyG4i5IW8Mk+qpy2Oz2x5GfD8i1\n"
        "/Suyp7wvQCArR+DRHZRSb2u+EraM3BHbceYZ76hxi63TMsAcpD+zD6/lUOH/QaS3\n"
        "b1dxr/0WTOgks5kbzxKPQwWAuhgZCqXsSYFBTH4osiHyWW5K7d75+pmFYB/mwkJc\n"
        "CAA8hAapJNTPe24bWR30cBYDoeALAJVcOQP8nRyO91kMYUf2fwciSINArOGYX8e+\n"
        "BdUpK78NAw7pXivdCRj+XjBh\n"
        "-----END CERTIFICATE-----\n"
                                    );
        
    // Test wrapper declaration : wrapping nothing for the moment
    struct sechandler_basic_test
    {
        X509 *mX509TestCert, *mX509RootCert, *mX509IntermediateCert, *mX509ChildCert;
        LLSD mValidationDate;
        
        sechandler_basic_test()
        {
            LLMachineID::init();
            OpenSSL_add_all_algorithms();
            OpenSSL_add_all_ciphers();
            OpenSSL_add_all_digests();  
            ERR_load_crypto_strings();
            gFirstName = "";
            gLastName = "";
            mValidationDate[CERT_VALIDATION_DATE] = LLDate("2017-04-11T00:00:00.00Z");
            LLFile::remove("test_password.dat");
            LLFile::remove("sechandler_settings.tmp");          

            mX509TestCert = NULL;
            mX509RootCert = NULL;
            mX509IntermediateCert = NULL;
            mX509ChildCert = NULL;

            // Read each of the 4 Pem certs and store in mX509*Cert pointers
            BIO * validation_bio;
            validation_bio = BIO_new_mem_buf((void*)mPemTestCert.c_str(), mPemTestCert.length());           
            PEM_read_bio_X509(validation_bio, &mX509TestCert, 0, NULL);
            BIO_free(validation_bio);

            validation_bio = BIO_new_mem_buf((void*)mPemRootCert.c_str(), mPemRootCert.length());
            PEM_read_bio_X509(validation_bio, &mX509RootCert, 0, NULL);
            BIO_free(validation_bio);

            validation_bio = BIO_new_mem_buf((void*)mPemIntermediateCert.c_str(), mPemIntermediateCert.length());
            PEM_read_bio_X509(validation_bio, &mX509IntermediateCert, 0, NULL);
            BIO_free(validation_bio);   

            validation_bio = BIO_new_mem_buf((void*)mPemChildCert.c_str(), mPemChildCert.length());
            PEM_read_bio_X509(validation_bio, &mX509ChildCert, 0, NULL);
            BIO_free(validation_bio);               
        }
        ~sechandler_basic_test()
        {
            LLFile::remove("test_password.dat");
            LLFile::remove("sechandler_settings.tmp");
            LLFile::remove("mycertstore.pem");
            X509_free(mX509TestCert);
            X509_free(mX509RootCert);
            X509_free(mX509IntermediateCert);
            X509_free(mX509ChildCert);
        }
    };
    
    // Tut templating thingamagic: test group, object and test instance
    typedef test_group<sechandler_basic_test> sechandler_basic_test_factory;
    typedef sechandler_basic_test_factory::object sechandler_basic_test_object;
    tut::sechandler_basic_test_factory tut_test("LLSecHandler");
    
    // ---------------------------------------------------------------------------------------
    // Test functions 
    // ---------------------------------------------------------------------------------------
    // test cert data retrieval
    template<> template<>
    void sechandler_basic_test_object::test<1>()
    {
        try
        {
            LLPointer<LLBasicCertificate> test_cert(new LLBasicCertificate(mPemTestCert, &mValidationDate));
            LL_INFOS() << "ok" << LL_ENDL;
        }
        catch (LLCertException& cert_exception)
        {
            LL_INFOS() << "cert ex: " << cert_exception.getCertData() << LL_ENDL;
            fail("cert exception");
        }
        catch (...)
        {
            LOG_UNHANDLED_EXCEPTION("test 1");
            fail("other exception");
        }
    }
    
    template<> template<>
    void sechandler_basic_test_object::test<2>()
    {
        LLPointer<LLCertificate> test_cert(new LLBasicCertificate(mPemChildCert, &mValidationDate));
    
        LLSD llsd_cert;
        test_cert->getLLSD(llsd_cert);
        //std::ostringstream llsd_value;
        //llsd_value << LLSDOStreamer<LLSDNotationFormatter>(llsd_cert) << std::endl;
        LL_DEBUGS() << "test 1 cert " << llsd_cert << LL_ENDL;
        
        ensure_equals("Issuer Name/commonName", (std::string)llsd_cert["issuer_name"]["commonName"], "Integration Test Intermediate CA");
        ensure_equals("Issuer Name/countryName", (std::string)llsd_cert["issuer_name"]["countryName"], "US");
        ensure_equals("Issuer Name/state", (std::string)llsd_cert["issuer_name"]["stateOrProvinceName"], "California");
        ensure_equals("Issuer Name/org name", (std::string)llsd_cert["issuer_name"]["organizationName"], "Linden Lab");
        ensure_equals("Issuer Name/org unit", (std::string)llsd_cert["issuer_name"]["organizationalUnitName"], "Second Life Engineering");
        ensure_equals("Issuer name string", (std::string)llsd_cert["issuer_name_string"],
                      "emailAddress=noreply@lindenlab.com,CN=Integration Test Intermediate CA,OU=Second Life Engineering,O=Linden Lab,ST=California,C=US");
        ensure_equals("subject Name/commonName", (std::string)llsd_cert["subject_name"]["commonName"],
                      "Integration Test Server Cert");
        ensure_equals("subject Name/countryName", (std::string)llsd_cert["subject_name"]["countryName"], "US");
        ensure_equals("subject Name/state", (std::string)llsd_cert["subject_name"]["stateOrProvinceName"], "California");
        ensure_equals("subject Name/localityName", (std::string)llsd_cert["subject_name"]["localityName"], "San Francisco");
        ensure_equals("subject Name/org name", (std::string)llsd_cert["subject_name"]["organizationName"], "Linden Lab");
        ensure_equals("subjectName/org unit", 
               (std::string)llsd_cert["subject_name"]["organizationalUnitName"], "Second Life Engineering");

        ensure_equals("subject name string", 
               (std::string)llsd_cert["subject_name_string"],
                      "emailAddress=noreply@lindenlab.com,CN=Integration Test Server Cert,OU=Second Life Engineering,O=Linden Lab,L=San Francisco,ST=California,C=US");
        ensure_equals("serial number", (std::string)llsd_cert["serial_number"], "1000");
        ensure_equals("valid from", (std::string)llsd_cert["valid_from"], "2018-05-22T22:58:15Z");
        ensure_equals("valid to", (std::string)llsd_cert["valid_to"], "2024-07-19T22:58:15Z");
        LLSD expectedKeyUsage = LLSD::emptyArray();
        expectedKeyUsage.append(LLSD((std::string)"digitalSignature"));
        expectedKeyUsage.append(LLSD((std::string)"keyEncipherment"));
        ensure("key usage", valueCompareLLSD(llsd_cert["keyUsage"], expectedKeyUsage));
        ensure_equals("basic constraints", llsd_cert["basicConstraints"]["CA"].asInteger(), 0);
        
        ensure("x509 is equal", !X509_cmp(mX509ChildCert, test_cert->getOpenSSLX509()));
    }

    
    // test protected data
    template<> template<>
    void sechandler_basic_test_object::test<3>()
    {
        std::string protected_data = "sUSh3wj77NG9oAMyt3XIhaej3KLZhLZWFZvI6rIGmwUUOmmelrRg0NI9rkOj8ZDpTPxpwToaBT5u"
        "GQhakdaGLJznr9bHr4/6HIC1bouKj4n2rs4TL6j2WSjto114QdlNfLsE8cbbE+ghww58g8SeyLQO"
        "nyzXoz+/PBz0HD5SMFDuObccoPW24gmqYySz8YoEWhSwO0pUtEEqOjVRsAJgF5wLAtJZDeuilGsq"
        "4ZT9Y4wZ9Rh8nnF3fDUL6IGamHe1ClXM1jgBu10F6UMhZbnH4C3aJ2E9+LiOntU+l3iCb2MpkEpr"
        "82r2ZAMwIrpnirL/xoYoyz7MJQYwUuMvBPToZJrxNSsjI+S2Z+I3iEJAELMAAA==";
        
        std::vector<U8> binary_data(apr_base64_decode_len(protected_data.c_str()));
        apr_base64_decode_binary(&binary_data[0], protected_data.c_str());

        LLXORCipher cipher(gMACAddress, MAC_ADDRESS_BYTES);
        cipher.decrypt(&binary_data[0], 16);
        unsigned char unique_id[MAC_ADDRESS_BYTES];
        LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
        LLXORCipher cipher2(unique_id, sizeof(unique_id));
        cipher2.encrypt(&binary_data[0], 16);
        std::ofstream temp_file("sechandler_settings.tmp", std::ofstream::binary);
        temp_file.write((const char *)&binary_data[0], binary_data.size());
        temp_file.close();

        LLPointer<LLSecAPIBasicHandler> handler = new LLSecAPIBasicHandler("sechandler_settings.tmp",
                                                                           "test_password.dat");
        handler->init();                                                                        
        // data retrieval for existing data
        LLSD data = handler->getProtectedData("test_data_type", "test_data_id");


        ensure_equals("retrieve existing data1", (std::string)data["data1"], "test_data_1");
        ensure_equals("retrieve existing data2", (std::string)data["data2"], "test_data_2");
        ensure_equals("retrieve existing data3", (std::string)data["data3"]["elem1"], "test element1");
        
        // data storage
        LLSD store_data = LLSD::emptyMap();
        store_data["store_data1"] = "test_store_data1";
        store_data["store_data2"] = 27;
        store_data["store_data3"] = LLSD::emptyMap();
        store_data["store_data3"]["subelem1"] = "test_subelem1";
        
        handler->setProtectedData("test_data_type", "test_data_id1", store_data);
        data = handler->getProtectedData("test_data_type", "test_data_id");
        
        data = handler->getProtectedData("test_data_type", "test_data_id");
        // verify no overwrite of existing data
        ensure_equals("verify no overwrite 1", (std::string)data["data1"], "test_data_1");
        ensure_equals("verify no overwrite 2", (std::string)data["data2"], "test_data_2");
        ensure_equals("verify no overwrite 3", (std::string)data["data3"]["elem1"], "test element1");
        
        // verify written data is good
        data = handler->getProtectedData("test_data_type", "test_data_id1");
        ensure_equals("verify stored data1", (std::string)data["store_data1"], "test_store_data1");
        ensure_equals("verify stored data2", (int)data["store_data2"], 27);
        ensure_equals("verify stored data3", (std::string)data["store_data3"]["subelem1"], "test_subelem1");
        
        // verify overwrite works
        handler->setProtectedData("test_data_type", "test_data_id", store_data);    
        data = handler->getProtectedData("test_data_type", "test_data_id");
        ensure_equals("verify overwrite stored data1", (std::string)data["store_data1"], "test_store_data1");
        ensure_equals("verify overwrite stored data2", (int)data["store_data2"], 27);
        ensure_equals("verify overwrite stored data3", (std::string)data["store_data3"]["subelem1"], "test_subelem1");
        
        // verify other datatype doesn't conflict
        store_data["store_data3"] = "test_store_data3";
        store_data["store_data4"] = 28;
        store_data["store_data5"] = LLSD::emptyMap();
        store_data["store_data5"]["subelem2"] = "test_subelem2";
        
        handler->setProtectedData("test_data_type1", "test_data_id", store_data);   
        data = handler->getProtectedData("test_data_type1", "test_data_id");
        ensure_equals("verify datatype stored data3", (std::string)data["store_data3"], "test_store_data3");
        ensure_equals("verify datatype stored data4", (int)data["store_data4"], 28);
        ensure_equals("verify datatype stored data5", (std::string)data["store_data5"]["subelem2"], "test_subelem2");   
        
        // test data not found

        data = handler->getProtectedData("test_data_type1", "test_data_not_found");
        ensure("not found", data.isUndefined());

        // cause a 'write' by using 'LLPointer' to delete then instantiate a handler
        handler = NULL;
        handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
        handler->init();

        data = handler->getProtectedData("test_data_type1", "test_data_id");
        ensure_equals("verify datatype stored data3a", (std::string)data["store_data3"], "test_store_data3");
        ensure_equals("verify datatype stored data4a", (int)data["store_data4"], 28);
        ensure_equals("verify datatype stored data5a", (std::string)data["store_data5"]["subelem2"], "test_subelem2");  
        
        // rewrite the initial file to verify reloads
        handler = NULL;
        std::ofstream temp_file2("sechandler_settings.tmp", std::ofstream::binary);
        temp_file2.write((const char *)&binary_data[0], binary_data.size());
        temp_file2.close();
        
        // cause a 'write'
        handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
        handler->init();        
        data = handler->getProtectedData("test_data_type1", "test_data_id");
        ensure("not found", data.isUndefined());
        
        handler->deleteProtectedData("test_data_type", "test_data_id");
        ensure("Deleted data not found", handler->getProtectedData("test_data_type", "test_data_id").isUndefined());
        
        LLFile::remove("sechandler_settings.tmp");
        handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
        handler->init();        
        data = handler->getProtectedData("test_data_type1", "test_data_id");
        ensure("not found", data.isUndefined());
        handler = NULL;
        
        ensure(LLFile::isfile("sechandler_settings.tmp"));
    }
    
    // test credenitals
    template<> template<>
    void sechandler_basic_test_object::test<4>()
    {
        LLPointer<LLSecAPIBasicHandler> handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
        handler->init();

        LLSD my_id = LLSD::emptyMap();
        LLSD my_authenticator = LLSD::emptyMap();
        my_id["type"] = "test_type";
        my_id["username"] = "testuser@lindenlab.com";
        my_authenticator["type"] = "test_auth";
        my_authenticator["creds"] = "12345";

        // test creation of credentials     
        LLPointer<LLCredential> my_cred = handler->createCredential("my_grid", my_id, my_authenticator);

        // test retrieval of credential components
        ensure_equals("basic credential creation: identifier", my_id, my_cred->getIdentifier());
        ensure_equals("basic credential creation: authenticator", my_authenticator, my_cred->getAuthenticator());
        ensure_equals("basic credential creation: grid", "my_grid", my_cred->getGrid());
        
        // test setting/overwriting of credential components
        my_id["first_name"] = "firstname";
        my_id.erase("username");
        my_authenticator.erase("creds");
        my_authenticator["hash"] = "6563245";
        
        my_cred->setCredentialData(my_id, my_authenticator);
        ensure_equals("set credential data: identifier", my_id, my_cred->getIdentifier());
        ensure_equals("set credential data: authenticator", my_authenticator, my_cred->getAuthenticator());
        ensure_equals("set credential data: grid", "my_grid", my_cred->getGrid());      
            
        // test loading of a credential, that hasn't been saved, without
        // any legacy saved credential data
        LLPointer<LLCredential> my_new_cred = handler->loadCredential("my_grid2");
        ensure("unknown credential load test", my_new_cred->getIdentifier().isMap());
        ensure("unknown credential load test", !my_new_cred->getIdentifier().has("type"));      
        ensure("unknown credential load test", my_new_cred->getAuthenticator().isMap());
        ensure("unknown credential load test", !my_new_cred->getAuthenticator().has("type"));   
        // test saving of a credential
        handler->saveCredential(my_cred, true);

        // test loading of a known credential
        my_new_cred = handler->loadCredential("my_grid");
        ensure_equals("load a known credential: identifier", my_id, my_new_cred->getIdentifier());
        ensure_equals("load a known credential: authenticator",my_authenticator, my_new_cred->getAuthenticator());
        ensure_equals("load a known credential: grid", "my_grid", my_cred->getGrid());
    
        // test deletion of a credential
        handler->deleteCredential(my_new_cred);

        ensure("delete credential: identifier", my_new_cred->getIdentifier().isUndefined());
        ensure("delete credentialt: authenticator", my_new_cred->getIdentifier().isUndefined());
        ensure_equals("delete credential: grid", "my_grid", my_cred->getGrid());        
        // load unknown cred
        
        my_new_cred = handler->loadCredential("my_grid");
        ensure("deleted credential load test", my_new_cred->getIdentifier().isMap());
        ensure("deleted credential load test", !my_new_cred->getIdentifier().has("type"));      
        ensure("deleted credential load test", my_new_cred->getAuthenticator().isMap());
        ensure("deleted credential load test", !my_new_cred->getAuthenticator().has("type"));
        
        // test loading of an unknown credential with legacy saved username, but without
        // saved password
        gFirstName = "myfirstname";
        gLastName = "mylastname";
        my_new_cred = handler->loadCredential("my_legacy_grid");
        ensure_equals("legacy credential with no password: type", 
                      (const std::string)my_new_cred->getIdentifier()["type"], "agent");
        ensure_equals("legacy credential with no password: first_name", 
                      (const std::string)my_new_cred->getIdentifier()["first_name"], "myfirstname");
        ensure_equals("legacy credential with no password: last_name",
                      (const std::string)my_new_cred->getIdentifier()["last_name"], "mylastname");
        
        ensure("legacy credential with no password: no authenticator", my_new_cred->getAuthenticator().isUndefined());
        
        // test loading of an unknown credential with legacy saved password and username

        std::string hashed_password = "fSQcLG03eyIWJmkzfyYaKm81dSweLmsxeSAYKGE7fSQ=";       
        int length = apr_base64_decode_len(hashed_password.c_str());
        std::vector<char> decoded_password(length);
        apr_base64_decode(&decoded_password[0], hashed_password.c_str());
        LLXORCipher cipher(gMACAddress, MAC_ADDRESS_BYTES);
        cipher.decrypt((U8*)&decoded_password[0], length);
        unsigned char unique_id[MAC_ADDRESS_BYTES];
        LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
        LLXORCipher cipher2(unique_id, sizeof(unique_id));
        cipher2.encrypt((U8*)&decoded_password[0], length);
        llofstream password_file("test_password.dat", std::ofstream::binary);
        password_file.write(&decoded_password[0], length); 
        password_file.close();
        
        my_new_cred = handler->loadCredential("my_legacy_grid2");       
        ensure_equals("legacy credential with password: type", 
                      (const std::string)my_new_cred->getIdentifier()["type"], "agent");
        ensure_equals("legacy credential with password: first_name", 
                      (const std::string)my_new_cred->getIdentifier()["first_name"], "myfirstname");
        ensure_equals("legacy credential with password: last_name",
                      (const std::string)my_new_cred->getIdentifier()["last_name"], "mylastname");
        
        LLSD legacy_authenticator = my_new_cred->getAuthenticator();
        ensure_equals("legacy credential with password: type", 
                      (std::string)legacy_authenticator["type"], 
                      "hash");
        ensure_equals("legacy credential with password: algorithm", 
                      (std::string)legacy_authenticator["algorithm"], 
                      "md5");   
        ensure_equals("legacy credential with password: algorithm", 
                      (std::string)legacy_authenticator["secret"], 
                      "01234567890123456789012345678901");
        
        // test creation of credentials     
        my_cred = handler->createCredential("mysavedgrid", my_id, my_authenticator);
        // test save without saving authenticator.      
        handler->saveCredential(my_cred, FALSE);
        my_new_cred = handler->loadCredential("mysavedgrid");   
        ensure_equals("saved credential without auth", 
                      (const std::string)my_new_cred->getIdentifier()["type"], "test_type");
        ensure("no authenticator values were saved", my_new_cred->getAuthenticator().isUndefined());
    }

    // test cert vector
    template<> template<>
    void sechandler_basic_test_object::test<5>()
    {
        // validate create from empty vector
        LLPointer<LLBasicCertificateVector> test_vector = new LLBasicCertificateVector();
        ensure_equals("when loading with nothing, we should result in no certs in vector", test_vector->size(), 0);
        
        test_vector->add(new LLBasicCertificate(mPemTestCert, &mValidationDate));
        ensure_equals("one element in vector", test_vector->size(), 1);
        test_vector->add(new LLBasicCertificate(mPemChildCert, &mValidationDate));
        ensure_equals("two elements in vector after add", test_vector->size(), 2);
        
        // add duplicate; should be a no-op (and log at DEBUG level)
        test_vector->add(new LLBasicCertificate(mPemChildCert, &mValidationDate));
        ensure_equals("two elements in vector after re-add", test_vector->size(), 2);

        // validate order
        X509* test_cert = (*test_vector)[0]->getOpenSSLX509();      
        ensure("first cert added remains first cert", !X509_cmp(test_cert, mX509TestCert));
        X509_free(test_cert);
        
        test_cert = (*test_vector)[1]->getOpenSSLX509();    
        ensure("second cert is second cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);       
        
        //
        // validate iterator
        //
        LLBasicCertificateVector::iterator current_cert = test_vector->begin();
        LLBasicCertificateVector::iterator copy_current_cert = current_cert;
        // operator++(int)
        ensure("validate iterator++ element in vector is expected cert", *current_cert++ == (*test_vector)[0]);
        ensure("validate 2nd iterator++ element in vector is expected cert", *current_cert++ == (*test_vector)[1]);
        ensure("validate end iterator++", current_cert == test_vector->end());
        
        // copy 
        ensure("validate copy iterator element in vector is expected cert", *copy_current_cert == (*test_vector)[0]);       
        
        // operator--(int)
        current_cert--;
        ensure("validate iterator-- element in vector is expected cert", *current_cert-- == (*test_vector)[1]);     
        ensure("validate iterator-- element in vector is expected cert", *current_cert == (*test_vector)[0]);
        
        ensure("begin iterator is equal", current_cert == test_vector->begin());
        
        // operator++
        ensure("validate ++iterator element in vector is expected cert", *++current_cert == (*test_vector)[1]);             
        ensure("end of cert vector after ++iterator", ++current_cert == test_vector->end());
        // operator--
        ensure("validate --iterator element in vector is expected cert", *--current_cert == (*test_vector)[1]);     
        ensure("validate 2nd --iterator element in vector is expected cert", *--current_cert == (*test_vector)[0]);     
        
        test_vector->erase(test_vector->begin());
        ensure_equals("one element in store after remove", test_vector->size(), 1);
        test_cert = (*test_vector)[0]->getOpenSSLX509();
        ensure("Child cert remains", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        
        // validate insert
        test_vector->insert(test_vector->begin(), new LLBasicCertificate(mPemIntermediateCert, &mValidationDate));
        test_cert = (*test_vector)[0]->getOpenSSLX509();
        ensure_equals("two elements in store after insert", test_vector->size(), 2);
        ensure("validate intermediate cert was inserted at first position", !X509_cmp(test_cert, mX509IntermediateCert));
        X509_free(test_cert);
        test_cert = (*test_vector)[1]->getOpenSSLX509();
        ensure("validate child cert still there", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);

        //validate find
        LLSD find_info = LLSD::emptyMap();
        find_info["subjectKeyIdentifier"] = "bb:59:9f:de:6b:51:a7:6c:b3:6d:5b:8b:42:f7:b1:65:77:17:a4:e4";
        LLBasicCertificateVector::iterator found_cert = test_vector->find(find_info);
        ensure("found some cert", found_cert != test_vector->end());
        X509* found_x509 = (*found_cert).get()->getOpenSSLX509();
        ensure("child cert was found", !X509_cmp(found_x509, mX509ChildCert));
        X509_free(found_x509);

        find_info["subjectKeyIdentifier"] = "00:00:00:00"; // bogus 
        current_cert =test_vector->find(find_info);
        ensure("didn't find cert", current_cert == test_vector->end());     
    }   
    
    // test cert store
    template<> template<>
    void sechandler_basic_test_object::test<6>()
    {
        // validate load with nothing
        LLFile::remove("mycertstore.pem");
        LLPointer<LLBasicCertificateStore> test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_equals("when loading with nothing, we should result in no certs in store", test_store->size(), 0);
        
        // validate load with empty file
        test_store->save();
        test_store = NULL;
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_equals("when loading with nothing, we should result in no certs in store", test_store->size(), 0);
        test_store=NULL;
        
        // instantiate a cert store from a file
        llofstream certstorefile("mycertstore.pem", std::ios::out);
        certstorefile << mPemChildCert << std::endl << mPemTestCert << std::endl;
        certstorefile.close();
        // validate loaded certs
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_equals("two elements in store", test_store->size(), 2);
        
        // operator[]
        X509* test_cert = (*test_store)[0]->getOpenSSLX509();

        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        test_cert = (*test_store)[1]->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509TestCert));   
        X509_free(test_cert);


        // validate save
        LLFile::remove("mycertstore.pem");
        test_store->save();
        test_store = NULL;
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_equals("two elements in store after save", test_store->size(), 2);               
        LLCertificateStore::iterator current_cert = test_store->begin();        
        test_cert = (*current_cert)->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        current_cert++;
        X509_free(test_cert);
        test_cert = (*current_cert)->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509TestCert));   
        X509_free(test_cert);
        current_cert++;
        ensure("end of cert store", current_cert == test_store->end());
        
    }
    
    // cert name wildcard matching
    template<> template<>
    void sechandler_basic_test_object::test<7>()
    {
        ensure("simple name match", 
               _cert_hostname_wildcard_match("foo", "foo"));

        ensure("simple name match, with end period", 
               _cert_hostname_wildcard_match("foo.", "foo."));
        
        ensure("simple name match, with begin period", 
               _cert_hostname_wildcard_match(".foo", ".foo"));      

        ensure("simple name match, with mismatched period cn", 
               _cert_hostname_wildcard_match("foo.", "foo"));   
        
        ensure("simple name match, with mismatched period hostname", 
               _cert_hostname_wildcard_match("foo", "foo."));   
        
        ensure("simple name match, with subdomain", 
               _cert_hostname_wildcard_match("foo.bar", "foo.bar"));    
        
        ensure("stutter name match", 
               _cert_hostname_wildcard_match("foobbbbfoo", "foo*bbbfoo"));          
        
        ensure("simple name match, with beginning wildcard", 
               _cert_hostname_wildcard_match("foobar", "*bar"));    
        
        ensure("simple name match, with ending wildcard", 
               _cert_hostname_wildcard_match("foobar", "foo*"));
        
        ensure("simple name match, with beginning null wildcard", 
               _cert_hostname_wildcard_match("foobar", "*foobar"));         

        ensure("simple name match, with ending null wildcard", 
               _cert_hostname_wildcard_match("foobar", "foobar*"));
        
        ensure("simple name match, with embedded wildcard", 
               _cert_hostname_wildcard_match("foobar", "f*r"));     
        
        ensure("simple name match, with embedded null wildcard", 
               _cert_hostname_wildcard_match("foobar", "foo*bar"));

        ensure("simple name match, with dual embedded wildcard", 
               _cert_hostname_wildcard_match("foobar", "f*o*ar"));      

        ensure("simple name mismatch", 
               !_cert_hostname_wildcard_match("bar", "foo"));
        
        ensure("simple name mismatch, with end period", 
               !_cert_hostname_wildcard_match("foobar.", "foo."));
        
        ensure("simple name mismatch, with begin period", 
               !_cert_hostname_wildcard_match(".foobar", ".foo"));      
        
        ensure("simple name mismatch, with subdomain", 
               !_cert_hostname_wildcard_match("foobar.bar", "foo.bar"));    
        
        ensure("simple name mismatch, with beginning wildcard", 
               !_cert_hostname_wildcard_match("foobara", "*bar"));  
        
        ensure("simple name mismatch, with ending wildcard", 
               !_cert_hostname_wildcard_match("oobar", "foo*"));
        
        ensure("simple name mismatch, with embedded wildcard", 
               !_cert_hostname_wildcard_match("oobar", "f*r"));     
        
        ensure("simple name mismatch, with dual embedded wildcard", 
               !_cert_hostname_wildcard_match("foobar", "f*d*ar"));
        
        ensure("simple wildcard", 
               _cert_hostname_wildcard_match("foobar", "*"));
        
        ensure("long domain", 
               _cert_hostname_wildcard_match("foo.bar.com", "foo.bar.com"));
        
        ensure("long domain with multiple wildcards", 
               _cert_hostname_wildcard_match("foo.bar.com", "*.b*r.com"));  

        ensure("end periods", 
               _cert_hostname_wildcard_match("foo.bar.com.", "*.b*r.com."));    
        
        ensure("match end period", 
               _cert_hostname_wildcard_match("foo.bar.com.", "*.b*r.com"));
        
        ensure("match end period2", 
               _cert_hostname_wildcard_match("foo.bar.com", "*.b*r.com."));
        
        ensure("wildcard mismatch", 
               !_cert_hostname_wildcard_match("bar.com", "*.bar.com")); 
        
        ensure("wildcard match", 
               _cert_hostname_wildcard_match("foo.bar.com", "*.bar.com"));  

        ensure("wildcard match", 
               _cert_hostname_wildcard_match("foo.foo.bar.com", "*.bar.com"));  
        
        ensure("wildcard match", 
               _cert_hostname_wildcard_match("foo.foo.bar.com", "*.*.com"));
        
        ensure("wildcard mismatch", 
               !_cert_hostname_wildcard_match("foo.foo.bar.com", "*.foo.com"));         
    }
    
    // test cert chain
    template<> template<>
    void sechandler_basic_test_object::test<8>()
    {
        // validate create from empty chain
        LLPointer<LLBasicCertificateChain> test_chain = new LLBasicCertificateChain(NULL);
        ensure_equals("when loading with nothing, we should result in no certs in chain", test_chain->size(), 0);

        // Single cert in the chain.
        X509_STORE_CTX *test_store = X509_STORE_CTX_new();
        X509_STORE_CTX_set_cert(test_store, mX509ChildCert);
        X509_STORE_CTX_set0_untrusted(test_store, NULL);
        test_chain = new LLBasicCertificateChain(test_store);
        X509_STORE_CTX_free(test_store);
        ensure_equals("two elements in store", test_chain->size(), 1);      
        X509* test_cert = (*test_chain)[0]->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);       
        
        // cert + CA
        
        test_store = X509_STORE_CTX_new();
        X509_STORE_CTX_set_cert(test_store, mX509ChildCert);
        X509_STORE_CTX_set0_untrusted(test_store, sk_X509_new_null());
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509IntermediateCert);
        test_chain = new LLBasicCertificateChain(test_store);
        X509_STORE_CTX_free(test_store);
        ensure_equals("two elements in store", test_chain->size(), 2);  
        test_cert = (*test_chain)[0]->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        test_cert = (*test_chain)[1]->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509IntermediateCert));   
        X509_free(test_cert);

        // cert + nonrelated
        
        test_store = X509_STORE_CTX_new();
        X509_STORE_CTX_set_cert(test_store, mX509ChildCert);
        X509_STORE_CTX_set0_untrusted(test_store, sk_X509_new_null());
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509TestCert);
        test_chain = new LLBasicCertificateChain(test_store);
        X509_STORE_CTX_free(test_store);
        ensure_equals("two elements in store", test_chain->size(), 1);  
        test_cert = (*test_chain)[0]->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        
        // cert + CA + nonrelated
        test_store = X509_STORE_CTX_new();
        X509_STORE_CTX_set_cert(test_store, mX509ChildCert);
        X509_STORE_CTX_set0_untrusted(test_store, sk_X509_new_null());
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509IntermediateCert);
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509TestCert);
        test_chain = new LLBasicCertificateChain(test_store);
        X509_STORE_CTX_free(test_store);
        ensure_equals("two elements in store", test_chain->size(), 2);  
        test_cert = (*test_chain)[0]->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        test_cert = (*test_chain)[1]->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509IntermediateCert));   
        X509_free(test_cert);

        // cert + intermediate + CA 
        test_store = X509_STORE_CTX_new();
        X509_STORE_CTX_set_cert(test_store, mX509ChildCert);
        X509_STORE_CTX_set0_untrusted(test_store, sk_X509_new_null());
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509IntermediateCert);
        sk_X509_push(X509_STORE_CTX_get0_untrusted(test_store), mX509RootCert);
        test_chain = new LLBasicCertificateChain(test_store);
        X509_STORE_CTX_free(test_store);
        ensure_equals("three elements in store", test_chain->size(), 3);    
        test_cert = (*test_chain)[0]->getOpenSSLX509();
        ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
        X509_free(test_cert);
        test_cert = (*test_chain)[1]->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509IntermediateCert));   
        X509_free(test_cert);

        test_cert = (*test_chain)[2]->getOpenSSLX509();
        ensure("validate second element in store is expected cert", !X509_cmp(test_cert, mX509RootCert));   
        X509_free(test_cert);       
    }

    // test cert validation
    template<> template<>
    void sechandler_basic_test_object::test<9>()
    {
        // start with a trusted store with our known root cert
        LLFile::remove("mycertstore.pem");
        LLPointer<LLBasicCertificateStore> test_store = new LLBasicCertificateStore("mycertstore.pem");
        test_store->add(new LLBasicCertificate(mX509RootCert, &mValidationDate));
        LLSD validation_params;
        
        // validate basic trust for a chain containing only the intermediate cert.  (1 deep)
        LLPointer<LLBasicCertificateChain> test_chain = new LLBasicCertificateChain(NULL);

        test_chain->add(new LLBasicCertificate(mX509IntermediateCert, &mValidationDate));

        test_store->validate(0, test_chain, validation_params);

        // add the root certificate to the chain and revalidate
        test_chain->add(new LLBasicCertificate(mX509RootCert, &mValidationDate));   
        test_store->validate(0, test_chain, validation_params);

        // add the child cert at the head of the chain, and revalidate (3 deep chain)
        test_chain->insert(test_chain->begin(), new LLBasicCertificate(mX509ChildCert, &mValidationDate));
        test_store->validate(0, test_chain, validation_params);

        // basic failure cases
        test_chain = new LLBasicCertificateChain(NULL);
        //validate with only the child cert in chain, but child cert was previously
        // trusted
        test_chain->add(new LLBasicCertificate(mX509ChildCert, &mValidationDate));
        
        // validate without the trust flag.
        test_store->validate(VALIDATION_POLICY_TRUSTED, test_chain, validation_params); 
        
        // Validate with child cert but no parent, and no parent in CA store
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_throws("no CA, with only a child cert", 
                      LLCertValidationTrustException, 
                      (*test_chain)[0],
                      test_store->validate, 
                      VALIDATION_POLICY_TRUSTED, 
                      test_chain, 
                      validation_params);


        // validate without the trust flag.
        test_store->validate(0, test_chain, validation_params);     

        // clear out the store
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        // append the intermediate cert
        test_chain->add(new LLBasicCertificate(mX509IntermediateCert, &mValidationDate));       
        ensure_throws("no CA, with child and intermediate certs", 
                      LLCertValidationTrustException, 
                      (*test_chain)[1],
                      test_store->validate, 
                      VALIDATION_POLICY_TRUSTED | VALIDATION_POLICY_TRUSTED, 
                      test_chain, 
                      validation_params);
        // validate without the trust flag
        test_store->validate(0, test_chain, validation_params);

        // Test time validity
        LLSD child_info;
        ((*test_chain)[0])->getLLSD(child_info);
        validation_params = LLSD::emptyMap();
        validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_FROM].asDate().secondsSinceEpoch() + 1.0);  
        test_store->validate(VALIDATION_POLICY_TIME | VALIDATION_POLICY_TRUSTED,
                             test_chain, validation_params);

        validation_params = LLSD::emptyMap();       
        validation_params[CERT_VALIDATION_DATE] = child_info[CERT_VALID_FROM].asDate();
        
        validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_FROM].asDate().secondsSinceEpoch() - 1.0);
        
        // test not yet valid
        ensure_throws("Child cert not yet valid" , 
                      LLCertValidationExpirationException, 
                      (*test_chain)[0],
                      test_store->validate, 
                      VALIDATION_POLICY_TIME | VALIDATION_POLICY_TRUSTED, 
                      test_chain, 
                      validation_params);   
        validation_params = LLSD::emptyMap();       
        validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_TO].asDate().secondsSinceEpoch() + 1.0);
        
        // test cert expired
        ensure_throws("Child cert expired", 
                      LLCertValidationExpirationException, 
                      (*test_chain)[0],
                      test_store->validate, 
                      VALIDATION_POLICY_TIME | VALIDATION_POLICY_TRUSTED, 
                      test_chain, 
                      validation_params);

        // test SSL KU
        // validate basic trust for a chain containing child and intermediate.
        test_chain = new LLBasicCertificateChain(NULL);
        test_chain->add(new LLBasicCertificate(mX509ChildCert, &mValidationDate));
        test_chain->add(new LLBasicCertificate(mX509IntermediateCert, &mValidationDate));
        test_store->validate(VALIDATION_POLICY_SSL_KU | VALIDATION_POLICY_TRUSTED,
                             test_chain, validation_params);    

        test_chain = new LLBasicCertificateChain(NULL);
        test_chain->add(new LLBasicCertificate(mX509TestCert, &mValidationDate));

        test_store = new LLBasicCertificateStore("mycertstore.pem");        
        ensure_throws("Cert doesn't have ku", 
                      LLCertKeyUsageValidationException, 
                      (*test_chain)[0],
                      test_store->validate, 
                      VALIDATION_POLICY_SSL_KU | VALIDATION_POLICY_TRUSTED, 
                      test_chain, 
                      validation_params);
        
        test_store->validate(0, test_chain, validation_params); 
    }

};

