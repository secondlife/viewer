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
S32 LLMachineID::init() { return 1; }
	

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

    const std::string mPemRootCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number:\n"
        "            bb:28:84:73:42:18:8b:67\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: Apr 10 19:28:59 2017 GMT\n"
        "            Not After : Apr  5 19:28:59 2037 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:af:ea:5d:a6:b3:e2:28:d6:98:48:69:4e:10:b8:\n"
        "                    03:3e:5c:6b:af:e3:d6:f5:e6:1e:b5:6e:77:f0:eb:\n"
        "                    9c:72:2a:ba:f0:9e:f9:a9:d3:7f:9d:64:5c:a5:f2:\n"
        "                    16:99:7c:96:67:69:aa:f1:3e:27:b6:03:c3:f6:8e:\n"
        "                    c1:f9:01:3e:35:04:bf:a4:ff:12:78:77:4b:39:e7:\n"
        "                    e4:93:09:e7:74:b3:3a:07:47:a2:9c:d2:1d:8c:e8:\n"
        "                    77:d9:c2:1c:4e:eb:51:dd:28:82:d4:e0:22:6d:32:\n"
        "                    4a:2e:25:53:b1:46:ff:49:18:99:8d:d6:ad:db:16:\n"
        "                    a5:0d:4a:d1:7c:19:d6:c7:08:7e:d2:90:1f:f9:e1:\n"
        "                    9c:54:bd:bd:c4:75:4f:10:01:78:09:35:5a:f2:2f:\n"
        "                    e5:42:36:76:17:cf:42:c9:ab:ef:aa:23:1e:50:3d:\n"
        "                    f2:9d:17:d1:d0:e9:6c:94:8e:a8:5d:d1:a1:8b:13:\n"
        "                    be:45:cc:77:6b:cb:4b:ad:23:87:1d:16:4a:ac:9d:\n"
        "                    e2:b8:07:c4:17:2b:53:ca:87:7b:81:dd:ad:5c:0a:\n"
        "                    87:00:8a:87:ae:84:cb:81:e2:9f:75:49:2b:e5:b7:\n"
        "                    78:63:be:68:fd:2f:f1:ee:10:f9:51:ef:7f:f1:59:\n"
        "                    f1:43:8d:c3:6d:33:29:4a:e5:25:cb:e1:0f:2a:e7:\n"
        "                    e5:8a:92:cf:5e:56:25:79:92:5e:70:d7:5f:de:55:\n"
        "                    a5:09:77:cf:06:26:62:2d:f6:86:a8:39:02:1d:0b:\n"
        "                    2d:d6:06:d1:68:2e:03:cf:7f:a5:2a:bb:b2:f5:48:\n"
        "                    22:57:bc:1a:18:f0:f9:33:99:f7:20:b7:ac:b7:06:\n"
        "                    01:5d:0b:62:7e:83:f0:00:a0:96:51:9b:0d:1d:23:\n"
        "                    c5:62:b9:27:ba:f8:bd:16:45:cf:13:31:79:6d:5f:\n"
        "                    a9:8b:59:f5:74:97:30:ac:a8:e8:05:fa:72:e5:f0:\n"
        "                    c7:33:8d:20:3d:4c:f3:6b:8e:43:3e:0e:51:9a:2e:\n"
        "                    e2:1d:e6:29:f2:d7:bc:a2:5d:54:e8:90:d3:07:20:\n"
        "                    b0:6e:71:3f:13:ef:c3:7e:9a:cb:57:83:1b:f6:32:\n"
        "                    82:65:cd:69:73:9c:ab:95:76:97:47:2f:ab:b5:3c:\n"
        "                    eb:90:a9:5c:0c:03:24:02:0f:3a:00:08:37:ee:b4:\n"
        "                    e9:21:af:92:cd:a2:49:fe:d5:f3:8f:89:5d:2b:53:\n"
        "                    66:cf:bc:78:d0:37:76:b8:16:d5:8d:21:bf:8f:98:\n"
        "                    b5:43:29:a1:32:ec:8c:58:9b:6b:3a:52:12:89:d1:\n"
        "                    3f:63:01:5f:e5:1b:d2:be:75:d9:65:29:9e:12:a1:\n"
        "                    c4:de:3a:a9:25:94:94:32:d7:e8:ca:d3:02:9b:2f:\n"
        "                    92:9a:11\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Subject Key Identifier: \n"
        "                CC:4E:CF:A0:E2:60:4F:BE:F2:77:51:1D:6E:3E:C6:B6:5A:38:23:A8\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:CC:4E:CF:A0:E2:60:4F:BE:F2:77:51:1D:6E:3E:C6:B6:5A:38:23:A8\n"
        "\n"
        "            X509v3 Basic Constraints: critical\n"
        "                CA:TRUE\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Certificate Sign, CRL Sign\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         68:b8:c5:d6:dd:e2:2f:5d:29:0b:aa:9f:10:66:88:fd:61:5d:\n"
        "         3a:0a:e0:aa:29:7f:42:4f:db:86:57:c3:96:e3:97:ff:bd:e7:\n"
        "         1e:c5:4d:00:87:64:3c:80:68:d6:f9:61:00:47:5e:f1:92:7f:\n"
        "         6f:0c:c7:8a:87:2b:b3:10:ff:22:8c:0a:8f:9f:5d:14:88:90:\n"
        "         52:12:a0:32:29:ea:8c:21:90:ed:0c:6a:70:26:43:81:bb:6e:\n"
        "         e2:36:4f:72:10:36:87:61:5d:27:f6:19:d9:83:ad:4b:51:7f:\n"
        "         5c:33:64:fd:2e:ac:86:80:95:bc:12:c6:26:02:06:9a:46:8b:\n"
        "         76:d9:89:e4:d6:02:bc:34:7c:f5:9a:51:e1:14:42:c9:7e:68:\n"
        "         16:be:b3:50:e1:42:4b:05:32:8c:d0:2d:44:df:3e:d2:86:a7:\n"
        "         89:20:b6:ee:bd:c8:dd:ad:f9:96:a2:1b:84:ad:51:87:23:66:\n"
        "         c0:fa:09:df:c0:d1:72:5e:a8:28:60:3f:6d:75:1d:6b:bc:a6:\n"
        "         d1:10:d7:be:d9:ac:26:b4:df:58:10:6e:09:33:6b:42:c8:79:\n"
        "         f5:38:53:4d:56:11:15:b8:39:2c:97:e4:7e:a9:63:b7:9a:b4:\n"
        "         b1:ab:7d:4c:3e:80:97:47:f8:dd:2e:74:e2:43:ad:6c:b4:88:\n"
        "         26:2c:1f:f2:88:ab:49:35:bc:65:27:db:59:c2:e6:1a:e5:ad:\n"
        "         f1:c3:44:fb:92:8a:1c:0e:b5:11:7a:00:26:90:e7:73:ee:c0:\n"
        "         8b:d6:b8:fd:ec:e7:80:a7:d2:6f:68:8c:bc:4d:4c:90:20:97:\n"
        "         85:33:7e:03:1b:88:8a:4d:5e:3c:00:f7:78:ec:2d:80:ec:09:\n"
        "         37:27:50:62:54:da:48:64:c9:30:1c:8a:3e:de:08:82:60:8b:\n"
        "         19:da:e2:a7:19:fb:0e:1f:95:b7:cd:1c:c2:cb:07:06:97:c0:\n"
        "         03:65:d5:a0:6f:03:66:22:11:e8:23:c9:98:83:d4:0e:a4:4b:\n"
        "         e5:62:02:62:67:b6:bd:3c:80:92:60:20:2e:0f:0a:59:75:7e:\n"
        "         b1:8e:0c:53:08:bd:12:09:2f:a0:53:dc:8d:46:77:68:bc:99:\n"
        "         7d:1d:41:66:f6:93:86:d4:64:f7:f6:5e:97:8c:4a:1d:93:38:\n"
        "         9c:3b:7c:4e:e9:69:e8:83:c8:0f:f3:3a:42:b5:44:d1:5f:d2:\n"
        "         9a:33:e3:be:1b:8f:74:23:c4:4e:ca:cf:91:38:d6:ee:67:32:\n"
        "         25:62:4f:a1:64:1a:b9:52:98:39:c2:a0:e0:7f:b9:51:74:78:\n"
        "         cc:af:55:08:d6:86:11:62:80:7f:b6:39:a2:60:ee:b7:99:a6:\n"
        "         59:04:76:51:85:e3:ba:59\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGXDCCBESgAwIBAgIJALsohHNCGItnMA0GCSqGSIb3DQEBCwUAMIG6MQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\n"
        "aXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUg\n"
        "RW5naW5lZXJpbmcxITAfBgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEk\n"
        "MCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMB4XDTE3MDQxMDE5\n"
        "Mjg1OVoXDTM3MDQwNTE5Mjg1OVowgboxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApD\n"
        "YWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMwEQYDVQQKDApMaW5k\n"
        "ZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEhMB8GA1UE\n"
        "AwwYSW50ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZIhvcNAQkBFhVub3Jl\n"
        "cGx5QGxpbmRlbmxhYi5jb20wggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC\n"
        "AQCv6l2ms+Io1phIaU4QuAM+XGuv49b15h61bnfw65xyKrrwnvmp03+dZFyl8haZ\n"
        "fJZnaarxPie2A8P2jsH5AT41BL+k/xJ4d0s55+STCed0szoHR6Kc0h2M6HfZwhxO\n"
        "61HdKILU4CJtMkouJVOxRv9JGJmN1q3bFqUNStF8GdbHCH7SkB/54ZxUvb3EdU8Q\n"
        "AXgJNVryL+VCNnYXz0LJq++qIx5QPfKdF9HQ6WyUjqhd0aGLE75FzHdry0utI4cd\n"
        "FkqsneK4B8QXK1PKh3uB3a1cCocAioeuhMuB4p91SSvlt3hjvmj9L/HuEPlR73/x\n"
        "WfFDjcNtMylK5SXL4Q8q5+WKks9eViV5kl5w11/eVaUJd88GJmIt9oaoOQIdCy3W\n"
        "BtFoLgPPf6Uqu7L1SCJXvBoY8Pkzmfcgt6y3BgFdC2J+g/AAoJZRmw0dI8ViuSe6\n"
        "+L0WRc8TMXltX6mLWfV0lzCsqOgF+nLl8MczjSA9TPNrjkM+DlGaLuId5iny17yi\n"
        "XVTokNMHILBucT8T78N+mstXgxv2MoJlzWlznKuVdpdHL6u1POuQqVwMAyQCDzoA\n"
        "CDfutOkhr5LNokn+1fOPiV0rU2bPvHjQN3a4FtWNIb+PmLVDKaEy7IxYm2s6UhKJ\n"
        "0T9jAV/lG9K+ddllKZ4SocTeOqkllJQy1+jK0wKbL5KaEQIDAQABo2MwYTAdBgNV\n"
        "HQ4EFgQUzE7PoOJgT77yd1Edbj7Gtlo4I6gwHwYDVR0jBBgwFoAUzE7PoOJgT77y\n"
        "d1Edbj7Gtlo4I6gwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwDQYJ\n"
        "KoZIhvcNAQELBQADggIBAGi4xdbd4i9dKQuqnxBmiP1hXToK4Kopf0JP24ZXw5bj\n"
        "l/+95x7FTQCHZDyAaNb5YQBHXvGSf28Mx4qHK7MQ/yKMCo+fXRSIkFISoDIp6owh\n"
        "kO0ManAmQ4G7buI2T3IQNodhXSf2GdmDrUtRf1wzZP0urIaAlbwSxiYCBppGi3bZ\n"
        "ieTWArw0fPWaUeEUQsl+aBa+s1DhQksFMozQLUTfPtKGp4kgtu69yN2t+ZaiG4St\n"
        "UYcjZsD6Cd/A0XJeqChgP211HWu8ptEQ177ZrCa031gQbgkza0LIefU4U01WERW4\n"
        "OSyX5H6pY7eatLGrfUw+gJdH+N0udOJDrWy0iCYsH/KIq0k1vGUn21nC5hrlrfHD\n"
        "RPuSihwOtRF6ACaQ53PuwIvWuP3s54Cn0m9ojLxNTJAgl4UzfgMbiIpNXjwA93js\n"
        "LYDsCTcnUGJU2khkyTAcij7eCIJgixna4qcZ+w4flbfNHMLLBwaXwANl1aBvA2Yi\n"
        "EegjyZiD1A6kS+ViAmJntr08gJJgIC4PCll1frGODFMIvRIJL6BT3I1Gd2i8mX0d\n"
        "QWb2k4bUZPf2XpeMSh2TOJw7fE7paeiDyA/zOkK1RNFf0poz474bj3QjxE7Kz5E4\n"
        "1u5nMiViT6FkGrlSmDnCoOB/uVF0eMyvVQjWhhFigH+2OaJg7reZplkEdlGF47pZ\n"
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
        "            Not Before: Apr 10 20:24:52 2017 GMT\n"
        "            Not After : Apr  8 20:24:52 2027 GMT\n"
        "        Subject: C=US, ST=California, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Intermediate CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:b4:9b:29:c6:22:c4:de:78:71:ed:2d:0d:90:32:\n"
        "                    fc:da:e7:8c:51:51:d2:fe:ec:e4:ca:5c:c8:5e:e0:\n"
        "                    c2:97:50:b7:c2:bd:22:91:35:4b:fd:b4:ac:20:21:\n"
        "                    b0:59:15:49:40:ff:91:9e:94:22:91:59:68:f4:ed:\n"
        "                    84:81:8d:be:15:67:02:8e:bf:6d:86:39:7e:42:3a:\n"
        "                    ea:72:9e:ca:5b:ef:1e:96:6c:bc:30:65:c1:73:f6:\n"
        "                    87:92:1f:24:f7:fb:39:77:b1:49:6b:27:5c:21:ba:\n"
        "                    f6:f9:1d:d5:6d:cc:58:8e:6d:d1:6b:fe:ec:89:34:\n"
        "                    34:80:d9:03:27:d5:6f:bc:7f:c7:b3:8c:63:4d:34:\n"
        "                    37:61:d0:f9:54:2e:2a:a8:85:03:04:22:b7:19:5b:\n"
        "                    a3:57:e4:43:a1:88:3c:42:04:c8:c3:fb:ef:0c:78:\n"
        "                    da:76:8c:e1:27:90:b1:b4:e2:c5:f3:b0:7c:0c:95:\n"
        "                    3e:cd:ed:ee:f8:28:28:c0:ba:64:e9:b5:0a:42:f3:\n"
        "                    8f:b1:dd:cc:41:58:a7:e7:a1:b0:2c:8e:58:55:3e:\n"
        "                    8c:d7:db:f2:51:38:96:4f:ae:1d:8e:ae:e3:87:1a:\n"
        "                    6c:8f:6b:3b:5a:1a:a9:49:bc:69:79:9f:28:6f:e2:\n"
        "                    ac:08:40:52:d9:87:c9:f2:27:d7:fb:62:85:5f:7f:\n"
        "                    09:a9:64:07:7b:7a:0e:ba:a5:58:18:23:aa:b2:df:\n"
        "                    66:77:f6:6a:ee:f7:79:18:30:12:b2:cf:60:79:af:\n"
        "                    86:d5:b8:db:ee:a0:13:2f:80:e1:69:0d:67:14:e5:\n"
        "                    9a:99:4c:10:2d:b1:26:6c:b8:3c:10:2f:8e:db:cb:\n"
        "                    4a:9e:9e:50:a2:98:76:49:7b:26:c1:8f:bf:50:00:\n"
        "                    f3:af:06:98:0a:af:78:03:84:5d:56:41:e0:90:7c:\n"
        "                    9a:a7:4d:5a:62:4d:8f:6a:cd:0e:27:c3:0c:4a:ba:\n"
        "                    68:8c:ff:e5:b9:21:a1:60:a3:d6:7b:2c:5c:09:3d:\n"
        "                    46:ec:4d:c9:b3:09:72:2a:ce:9b:65:f9:56:5e:6e:\n"
        "                    2e:24:64:4a:29:7f:17:1d:92:1d:bd:6e:d7:ce:73:\n"
        "                    cf:57:23:00:1d:db:bc:77:d4:fe:b1:ea:40:34:5c:\n"
        "                    01:94:ee:c5:6a:5e:ce:63:d2:61:c9:55:ca:13:93:\n"
        "                    e8:be:0f:00:0a:f5:6c:fc:31:e3:08:05:a4:9a:b2:\n"
        "                    8e:85:b5:0d:fd:fd:6f:d9:10:e4:68:8a:1b:81:27:\n"
        "                    da:14:c6:08:5a:bd:f1:ec:c6:41:ac:05:d7:cc:63:\n"
        "                    4e:e8:e0:18:7e:f3:ed:4b:60:81:dd:07:fe:5d:ad:\n"
        "                    9a:7c:80:99:6b:06:0f:ae:f6:7d:27:27:a0:3d:05:\n"
        "                    c6:cb:dd\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Subject Key Identifier: \n"
        "                CC:57:77:7A:16:10:AE:94:99:A1:9F:AB:2F:79:42:74:D7:BE:8E:63\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:CC:4E:CF:A0:E2:60:4F:BE:F2:77:51:1D:6E:3E:C6:B6:5A:38:23:A8\n"
        "\n"
        "            X509v3 Basic Constraints: critical\n"
        "                CA:TRUE, pathlen:0\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Certificate Sign, CRL Sign\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         41:78:c6:7d:0f:1f:0e:82:c8:7e:3a:56:7d:f7:a6:5e:c3:dc:\n"
        "         88:9e:e5:77:7d:c5:3c:70:2f:8a:cf:93:59:92:8f:17:04:5b:\n"
        "         d7:d5:58:d9:cc:d6:df:77:0b:5f:db:ea:54:b6:3b:ec:d6:c4:\n"
        "         26:4f:63:54:06:ae:bc:5f:c3:b5:00:52:6f:2a:f6:c0:84:0b:\n"
        "         3e:fd:fe:82:87:82:40:5f:f7:08:5b:17:42:5e:46:60:66:77:\n"
        "         8f:04:2d:c0:7a:50:c2:58:42:10:fc:99:f8:30:3a:c6:ba:fa:\n"
        "         13:a5:ee:19:f8:4c:c8:72:37:64:16:16:ef:7e:a1:cb:df:af:\n"
        "         26:c5:ff:88:46:30:04:80:4c:cd:1a:56:f6:7a:4a:7b:c2:5e:\n"
        "         58:3b:ec:84:30:92:9c:7c:83:39:59:7f:57:f2:e7:1a:2c:ed:\n"
        "         d9:e4:8a:1f:7e:ce:92:25:d9:78:c5:1b:f4:c6:31:10:79:3d:\n"
        "         8b:1d:e9:50:6d:87:2d:01:55:e0:59:c1:45:cd:ad:de:68:00:\n"
        "         91:9b:2a:9d:f5:aa:56:8d:48:9a:bf:aa:46:57:90:ba:4b:5d:\n"
        "         70:cf:1b:b2:9d:5d:21:8d:5d:b5:9e:35:96:e5:34:2b:37:52:\n"
        "         ec:f7:03:9e:ca:e4:80:dd:1c:e3:89:e4:cd:67:5e:45:5e:88:\n"
        "         3b:2c:28:19:f2:ae:d2:51:7d:9b:12:5a:74:64:ea:41:b4:98:\n"
        "         6c:85:87:58:45:01:29:c3:0f:e7:1a:76:72:0f:d1:2a:c8:62:\n"
        "         b6:2d:67:42:3c:0b:bf:1d:2a:ab:85:19:aa:7c:42:b3:0f:c1:\n"
        "         9f:1b:b7:b5:ff:19:cb:2e:d8:98:b7:99:35:a3:34:ba:31:0a:\n"
        "         ba:59:fd:fe:72:53:3d:19:a7:36:4f:e1:a5:51:dd:ff:9f:6d:\n"
        "         a1:22:64:01:dc:f4:8a:19:d3:5a:95:b6:a0:59:f8:28:f8:a1:\n"
        "         bc:50:41:f5:f7:1a:42:e2:a1:aa:cc:44:36:64:ba:eb:b0:06:\n"
        "         05:58:2c:92:57:cd:8f:6a:ac:04:ba:4f:4d:71:4b:d4:c4:0d:\n"
        "         13:a2:75:de:48:c7:af:ef:1a:0d:d1:ac:94:53:68:c4:b8:2b:\n"
        "         88:4f:9d:78:b0:9b:a7:c4:a6:57:ad:3d:f5:1e:b4:fe:1d:d7:\n"
        "         42:6c:c4:c5:f6:8c:29:5c:92:3a:7d:79:f2:0d:01:ff:3c:29:\n"
        "         01:b9:91:59:7a:ea:e3:59:bd:67:28:3b:46:60:2c:e4:fd:61:\n"
        "         49:8d:3d:7f:ce:c2:d7:1d:2f:da:74:2f:38:e6:b2:f0:1f:5f:\n"
        "         43:dc:43:6c:e2:e3:c8:25:e6:6e:72:6b:90:50:f8:5c:9a:98:\n"
        "         20:0e:04:e2:b3:59:c9:3a\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGSDCCBDCgAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwgboxCzAJBgNVBAYTAlVT\n"
        "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMw\n"
        "EQYDVQQKDApMaW5kZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVl\n"
        "cmluZzEhMB8GA1UEAwwYSW50ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZI\n"
        "hvcNAQkBFhVub3JlcGx5QGxpbmRlbmxhYi5jb20wHhcNMTcwNDEwMjAyNDUyWhcN\n"
        "MjcwNDA4MjAyNDUyWjCBqjELMAkGA1UEBhMCVVMxEzARBgNVBAgMCkNhbGlmb3Ju\n"
        "aWExEzARBgNVBAoMCkxpbmRlbiBMYWIxIDAeBgNVBAsMF1NlY29uZCBMaWZlIEVu\n"
        "Z2luZWVyaW5nMSkwJwYDVQQDDCBJbnRlZ3JhdGlvbiBUZXN0IEludGVybWVkaWF0\n"
        "ZSBDQTEkMCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMIICIjAN\n"
        "BgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAtJspxiLE3nhx7S0NkDL82ueMUVHS\n"
        "/uzkylzIXuDCl1C3wr0ikTVL/bSsICGwWRVJQP+RnpQikVlo9O2EgY2+FWcCjr9t\n"
        "hjl+Qjrqcp7KW+8elmy8MGXBc/aHkh8k9/s5d7FJaydcIbr2+R3VbcxYjm3Ra/7s\n"
        "iTQ0gNkDJ9VvvH/Hs4xjTTQ3YdD5VC4qqIUDBCK3GVujV+RDoYg8QgTIw/vvDHja\n"
        "dozhJ5CxtOLF87B8DJU+ze3u+CgowLpk6bUKQvOPsd3MQVin56GwLI5YVT6M19vy\n"
        "UTiWT64djq7jhxpsj2s7WhqpSbxpeZ8ob+KsCEBS2YfJ8ifX+2KFX38JqWQHe3oO\n"
        "uqVYGCOqst9md/Zq7vd5GDASss9gea+G1bjb7qATL4DhaQ1nFOWamUwQLbEmbLg8\n"
        "EC+O28tKnp5Qoph2SXsmwY+/UADzrwaYCq94A4RdVkHgkHyap01aYk2Pas0OJ8MM\n"
        "SrpojP/luSGhYKPWeyxcCT1G7E3JswlyKs6bZflWXm4uJGRKKX8XHZIdvW7XznPP\n"
        "VyMAHdu8d9T+sepANFwBlO7Fal7OY9JhyVXKE5Povg8ACvVs/DHjCAWkmrKOhbUN\n"
        "/f1v2RDkaIobgSfaFMYIWr3x7MZBrAXXzGNO6OAYfvPtS2CB3Qf+Xa2afICZawYP\n"
        "rvZ9JyegPQXGy90CAwEAAaNmMGQwHQYDVR0OBBYEFMxXd3oWEK6UmaGfqy95QnTX\n"
        "vo5jMB8GA1UdIwQYMBaAFMxOz6DiYE++8ndRHW4+xrZaOCOoMBIGA1UdEwEB/wQI\n"
        "MAYBAf8CAQAwDgYDVR0PAQH/BAQDAgGGMA0GCSqGSIb3DQEBCwUAA4ICAQBBeMZ9\n"
        "Dx8Ogsh+OlZ996Zew9yInuV3fcU8cC+Kz5NZko8XBFvX1VjZzNbfdwtf2+pUtjvs\n"
        "1sQmT2NUBq68X8O1AFJvKvbAhAs+/f6Ch4JAX/cIWxdCXkZgZnePBC3AelDCWEIQ\n"
        "/Jn4MDrGuvoTpe4Z+EzIcjdkFhbvfqHL368mxf+IRjAEgEzNGlb2ekp7wl5YO+yE\n"
        "MJKcfIM5WX9X8ucaLO3Z5Ioffs6SJdl4xRv0xjEQeT2LHelQbYctAVXgWcFFza3e\n"
        "aACRmyqd9apWjUiav6pGV5C6S11wzxuynV0hjV21njWW5TQrN1Ls9wOeyuSA3Rzj\n"
        "ieTNZ15FXog7LCgZ8q7SUX2bElp0ZOpBtJhshYdYRQEpww/nGnZyD9EqyGK2LWdC\n"
        "PAu/HSqrhRmqfEKzD8GfG7e1/xnLLtiYt5k1ozS6MQq6Wf3+clM9Gac2T+GlUd3/\n"
        "n22hImQB3PSKGdNalbagWfgo+KG8UEH19xpC4qGqzEQ2ZLrrsAYFWCySV82PaqwE\n"
        "uk9NcUvUxA0TonXeSMev7xoN0ayUU2jEuCuIT514sJunxKZXrT31HrT+HddCbMTF\n"
        "9owpXJI6fXnyDQH/PCkBuZFZeurjWb1nKDtGYCzk/WFJjT1/zsLXHS/adC845rLw\n"
        "H19D3ENs4uPIJeZucmuQUPhcmpggDgTis1nJOg==\n"
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
        "            Not Before: Apr 10 21:35:07 2017 GMT\n"
        "            Not After : Apr 20 21:35:07 2018 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Server Cert/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (2048 bit)\n"
        "                Modulus:\n"
        "                    00:ba:51:fb:01:57:44:2f:99:03:36:82:c0:6a:d2:\n"
        "                    17:1d:f9:e1:49:71:b1:d1:61:c4:90:61:40:99:aa:\n"
        "                    8e:78:99:40:c8:b7:f5:bd:78:a5:7a:c8:fb:73:33:\n"
        "                    74:c0:78:ee:2d:55:08:78:6c:e4:e0:87:4a:34:df:\n"
        "                    6a:25:f7:8c:86:87:0e:f6:df:00:a7:42:4f:89:e3:\n"
        "                    b1:c0:db:2a:9d:96:2b:6f:47:66:04:9b:e8:f0:18:\n"
        "                    ce:7b:4b:bf:8b:6e:24:7e:df:89:07:b4:f5:69:1d:\n"
        "                    4e:9d:9d:c1:6b:19:51:60:56:3e:4a:b8:c2:c0:9d:\n"
        "                    67:fb:fe:d7:73:fa:61:38:85:9b:b0:5f:80:db:a1:\n"
        "                    57:5e:9f:90:af:7d:33:31:7d:bd:73:0b:a2:d5:1e:\n"
        "                    ff:10:a5:6d:fb:c7:55:e6:a0:81:21:f5:d7:23:e5:\n"
        "                    9c:c1:f2:29:8a:aa:83:9f:75:9f:84:fc:65:4c:29:\n"
        "                    b3:98:1f:a6:05:0b:1a:a8:0d:68:2e:20:47:2d:06:\n"
        "                    46:de:92:3d:eb:02:a3:b2:9f:65:66:44:7c:b0:da:\n"
        "                    55:77:f5:5a:9f:c0:58:b6:ff:7d:31:41:72:cc:bd:\n"
        "                    7a:1d:58:36:a8:f2:ca:6a:ca:6b:03:29:ac:94:ad:\n"
        "                    93:f4:7a:14:52:b3:ce:61:e1:7e:6c:8f:08:ad:a9:\n"
        "                    5d:37\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Basic Constraints: \n"
        "                CA:FALSE\n"
        "            Netscape Cert Type: \n"
        "                SSL Server\n"
        "            Netscape Comment: \n"
        "                OpenSSL Generated Server Certificate\n"
        "            X509v3 Subject Key Identifier: \n"
        "                6B:69:AA:91:99:C8:8C:01:72:58:D3:1F:F8:29:73:9C:98:F7:3F:5F\n"
        "            X509v3 Authority Key Identifier: \n"
        "                keyid:CC:57:77:7A:16:10:AE:94:99:A1:9F:AB:2F:79:42:74:D7:BE:8E:63\n"
        "                DirName:/C=US/ST=California/L=San Francisco/O=Linden Lab/OU=Second Life Engineering/CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "                serial:10:00\n"
        "\n"
        "            X509v3 Key Usage: critical\n"
        "                Digital Signature, Key Encipherment\n"
        "            X509v3 Extended Key Usage: \n"
        "                TLS Web Server Authentication\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         ac:35:1a:96:65:28:7c:ed:c5:e3:b9:ef:52:9e:66:b8:63:2e:\n"
        "         de:73:97:3c:91:d5:02:a3:62:9e:c6:5f:f7:18:ed:7f:f8:a1:\n"
        "         66:d2:bc:12:fd:90:b8:fb:ef:ce:fe:e4:21:5e:b9:d1:c9:65:\n"
        "         13:4b:d0:e5:d0:9a:9b:f3:d6:79:bd:9b:af:25:93:01:32:5c:\n"
        "         14:48:03:c1:f7:c6:19:80:d4:1b:f7:e3:82:59:0c:50:0d:85:\n"
        "         97:64:e5:4e:2f:5e:cb:b6:dc:a0:44:64:32:ba:57:ee:45:26:\n"
        "         58:c2:36:71:a8:90:3a:37:48:33:75:79:8e:4f:b1:2d:65:6e:\n"
        "         04:9f:35:28:40:97:f3:80:c1:c8:bb:b9:cd:a2:aa:42:a9:9a:\n"
        "         c6:ab:ac:48:a4:eb:0a:17:19:a0:44:9d:8a:7f:b1:21:a1:14:\n"
        "         ac:0f:71:e0:e8:28:07:44:8a:e7:70:c9:af:19:08:8f:be:2c:\n"
        "         79:af:62:af:9f:8e:d8:4a:c5:09:d5:27:1a:29:c3:2a:f1:b9:\n"
        "         a2:df:0b:e4:22:22:4e:26:11:ad:3d:39:4c:e6:53:49:d5:65:\n"
        "         8c:e8:68:98:91:50:40:ff:fd:ac:ef:71:12:28:a8:b3:5f:f7:\n"
        "         b3:26:2e:eb:f4:d0:d4:68:31:ee:4a:78:b3:85:60:37:1b:21:\n"
        "         2d:e9:f2:67:5a:64:17:e5:30:fc:2d:ed:59:a0:06:8d:90:ea:\n"
        "         ba:26:2f:d8:ac:68:98:db:42:87:39:65:64:b6:08:9f:70:dc:\n"
        "         74:8d:ac:26:ce:8e:a7:dc:1d:41:de:82:7c:00:46:d0:23:74:\n"
        "         b5:5a:4c:91:e4:92:11:a4:13:fd:50:05:86:89:c4:fd:11:ce:\n"
        "         17:44:8f:35:ea:c8:4e:8c:a5:e1:ed:62:32:ff:2f:f7:92:f3:\n"
        "         f7:5c:d2:e7:27:d8:ff:f7:92:7d:dc:a6:ca:d9:e0:92:9d:db:\n"
        "         34:9e:6e:c8:f4:f1:d0:d8:30:c2:85:87:c5:f6:ed:0b:d4:b1:\n"
        "         a6:7c:c1:cd:55:41:c0:e4:cf:06:62:31:fd:4e:b1:eb:45:71:\n"
        "         5b:7c:42:02:4c:ee:74:27:8a:81:11:f1:32:89:40:c9:85:03:\n"
        "         bb:e8:73:55:53:f0:73:eb:47:68:4c:34:9a:1d:7d:cb:54:50:\n"
        "         59:c7:82:3e:42:5c:81:51:7a:01:71:86:a1:b0:da:e6:09:3a:\n"
        "         29:ee:e9:9e:58:19:d7:81:69:bd:3c:5a:02:49:6f:3c:03:0e:\n"
        "         4a:79:06:50:40:8a:60:11:35:6b:56:fc:34:46:52:68:ca:d3:\n"
        "         3a:c1:85:bc:e4:25:57:70:b4:ab:36:d8:8b:0a:6b:8d:7b:b7:\n"
        "         88:7d:10:33:6e:be:83:e6\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGbjCCBFagAwIBAgICEAAwDQYJKoZIhvcNAQELBQAwgaoxCzAJBgNVBAYTAlVT\n"
        "MRMwEQYDVQQIDApDYWxpZm9ybmlhMRMwEQYDVQQKDApMaW5kZW4gTGFiMSAwHgYD\n"
        "VQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEpMCcGA1UEAwwgSW50ZWdyYXRp\n"
        "b24gVGVzdCBJbnRlcm1lZGlhdGUgQ0ExJDAiBgkqhkiG9w0BCQEWFW5vcmVwbHlA\n"
        "bGluZGVubGFiLmNvbTAeFw0xNzA0MTAyMTM1MDdaFw0xODA0MjAyMTM1MDdaMIG+\n"
        "MQswCQYDVQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2Fu\n"
        "IEZyYW5jaXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25k\n"
        "IExpZmUgRW5naW5lZXJpbmcxJTAjBgNVBAMMHEludGVncmF0aW9uIFRlc3QgU2Vy\n"
        "dmVyIENlcnQxJDAiBgkqhkiG9w0BCQEWFW5vcmVwbHlAbGluZGVubGFiLmNvbTCC\n"
        "ASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALpR+wFXRC+ZAzaCwGrSFx35\n"
        "4UlxsdFhxJBhQJmqjniZQMi39b14pXrI+3MzdMB47i1VCHhs5OCHSjTfaiX3jIaH\n"
        "DvbfAKdCT4njscDbKp2WK29HZgSb6PAYzntLv4tuJH7fiQe09WkdTp2dwWsZUWBW\n"
        "Pkq4wsCdZ/v+13P6YTiFm7BfgNuhV16fkK99MzF9vXMLotUe/xClbfvHVeaggSH1\n"
        "1yPlnMHyKYqqg591n4T8ZUwps5gfpgULGqgNaC4gRy0GRt6SPesCo7KfZWZEfLDa\n"
        "VXf1Wp/AWLb/fTFBcsy9eh1YNqjyymrKawMprJStk/R6FFKzzmHhfmyPCK2pXTcC\n"
        "AwEAAaOCAYYwggGCMAkGA1UdEwQCMAAwEQYJYIZIAYb4QgEBBAQDAgZAMDMGCWCG\n"
        "SAGG+EIBDQQmFiRPcGVuU1NMIEdlbmVyYXRlZCBTZXJ2ZXIgQ2VydGlmaWNhdGUw\n"
        "HQYDVR0OBBYEFGtpqpGZyIwBcljTH/gpc5yY9z9fMIHoBgNVHSMEgeAwgd2AFMxX\n"
        "d3oWEK6UmaGfqy95QnTXvo5joYHApIG9MIG6MQswCQYDVQQGEwJVUzETMBEGA1UE\n"
        "CAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzETMBEGA1UECgwK\n"
        "TGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUgRW5naW5lZXJpbmcxITAf\n"
        "BgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEkMCIGCSqGSIb3DQEJARYV\n"
        "bm9yZXBseUBsaW5kZW5sYWIuY29tggIQADAOBgNVHQ8BAf8EBAMCBaAwEwYDVR0l\n"
        "BAwwCgYIKwYBBQUHAwEwDQYJKoZIhvcNAQELBQADggIBAKw1GpZlKHztxeO571Ke\n"
        "ZrhjLt5zlzyR1QKjYp7GX/cY7X/4oWbSvBL9kLj7787+5CFeudHJZRNL0OXQmpvz\n"
        "1nm9m68lkwEyXBRIA8H3xhmA1Bv344JZDFANhZdk5U4vXsu23KBEZDK6V+5FJljC\n"
        "NnGokDo3SDN1eY5PsS1lbgSfNShAl/OAwci7uc2iqkKpmsarrEik6woXGaBEnYp/\n"
        "sSGhFKwPceDoKAdEiudwya8ZCI++LHmvYq+fjthKxQnVJxopwyrxuaLfC+QiIk4m\n"
        "Ea09OUzmU0nVZYzoaJiRUED//azvcRIoqLNf97MmLuv00NRoMe5KeLOFYDcbIS3p\n"
        "8mdaZBflMPwt7VmgBo2Q6romL9isaJjbQoc5ZWS2CJ9w3HSNrCbOjqfcHUHegnwA\n"
        "RtAjdLVaTJHkkhGkE/1QBYaJxP0RzhdEjzXqyE6MpeHtYjL/L/eS8/dc0ucn2P/3\n"
        "kn3cpsrZ4JKd2zSebsj08dDYMMKFh8X27QvUsaZ8wc1VQcDkzwZiMf1OsetFcVt8\n"
        "QgJM7nQnioER8TKJQMmFA7voc1VT8HPrR2hMNJodfctUUFnHgj5CXIFRegFxhqGw\n"
        "2uYJOinu6Z5YGdeBab08WgJJbzwDDkp5BlBAimARNWtW/DRGUmjK0zrBhbzkJVdw\n"
        "tKs22IsKa417t4h9EDNuvoPm\n"
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
		ensure_equals("valid from", (std::string)llsd_cert["valid_from"], "2017-04-10T21:35:07Z");
		ensure_equals("valid to", (std::string)llsd_cert["valid_to"], "2018-04-20T21:35:07Z");
		LLSD expectedKeyUsage = LLSD::emptyArray();
		expectedKeyUsage.append(LLSD((std::string)"digitalSignature"));
		expectedKeyUsage.append(LLSD((std::string)"keyEncipherment"));
		ensure("key usage", valueCompareLLSD(llsd_cert["keyUsage"], expectedKeyUsage));
		ensure_equals("basic constraints", (bool)llsd_cert["basicConstraints"]["CA"].asInteger(), 0);
		
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
		find_info["subjectKeyIdentifier"] = "6b:69:aa:91:99:c8:8c:01:72:58:d3:1f:f8:29:73:9c:98:f7:3f:5f";
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
		test_store->cert = mX509ChildCert;		
		test_store->untrusted = NULL;
		test_chain = new LLBasicCertificateChain(test_store);
		X509_STORE_CTX_free(test_store);
		ensure_equals("two elements in store", test_chain->size(), 1);		
		X509* test_cert = (*test_chain)[0]->getOpenSSLX509();
		ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
		X509_free(test_cert);		
		
		// cert + CA
		
		test_store = X509_STORE_CTX_new();
		test_store->cert = mX509ChildCert;
		test_store->untrusted = sk_X509_new_null();
		sk_X509_push(test_store->untrusted, mX509IntermediateCert);
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
		test_store->cert = mX509ChildCert;
		test_store->untrusted = sk_X509_new_null();
		sk_X509_push(test_store->untrusted, mX509TestCert);
		test_chain = new LLBasicCertificateChain(test_store);
		X509_STORE_CTX_free(test_store);
		ensure_equals("two elements in store", test_chain->size(), 1);	
		test_cert = (*test_chain)[0]->getOpenSSLX509();
		ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509ChildCert));
		X509_free(test_cert);
		
		// cert + CA + nonrelated
		test_store = X509_STORE_CTX_new();
		test_store->cert = mX509ChildCert;
		test_store->untrusted = sk_X509_new_null();
		sk_X509_push(test_store->untrusted, mX509IntermediateCert);
		sk_X509_push(test_store->untrusted, mX509TestCert);
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
		test_store->cert = mX509ChildCert;
		test_store->untrusted = sk_X509_new_null();
		sk_X509_push(test_store->untrusted, mX509IntermediateCert);
		sk_X509_push(test_store->untrusted, mX509RootCert);
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

