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
void LLControlGroup::setString(std::string_view name, const std::string& val){}
std::string LLControlGroup::getString(std::string_view name)
{

    if (name == "FirstName")
        return gFirstName;
    else if (name == "LastName")
        return gLastName;
    return "";
}

// Stub for --no-verify-ssl-cert
bool LLControlGroup::getBOOL(std::string_view name) { return false; }

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
        "        Serial Number: ef:54:d8:f7:da:18:e8:19\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: Jul 23 11:46:26 2024 GMT\n"
        "            Not After : Jul 21 11:46:26 2034 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:c6:cc:07:f4:0b:17:06:4d:a6:30:b4:c7:02:6b:\n"
        "                    9d:a4:47:a6:09:0e:60:1a:32:d4:6b:42:88:ee:c5:\n"
        "                    b9:e9:fb:b5:0b:60:dc:a2:45:92:a5:bb:88:12:fc:\n"
        "                    42:1a:80:32:79:16:62:7a:97:af:84:28:53:3c:c1:\n"
        "                    f2:68:c0:4e:45:e4:0a:63:f9:34:1d:a2:8b:cc:70:\n"
        "                    df:c6:65:c0:ba:31:32:d2:9d:0c:c8:ce:dc:11:12:\n"
        "                    a4:11:fa:d3:c8:56:e2:31:8a:e3:fb:91:40:da:25:\n"
        "                    55:d1:f2:75:9b:4d:fa:b8:1f:b5:6d:9b:e1:fe:5d:\n"
        "                    e8:c4:02:79:14:ef:7d:5a:b3:3a:1e:b6:d0:60:2c:\n"
        "                    90:dc:22:e2:c5:ae:85:1f:b4:9d:7a:20:f8:af:63:\n"
        "                    56:25:1a:64:f3:9c:3f:9a:cf:68:08:0a:37:db:d0:\n"
        "                    a3:65:26:db:80:82:ff:e0:1b:51:c8:ee:f6:ad:c2:\n"
        "                    b4:f2:ab:d2:e8:85:86:77:28:d0:63:4a:71:78:41:\n"
        "                    e3:8c:7f:71:51:31:af:24:3f:fa:8d:d0:d8:0b:e2:\n"
        "                    7e:79:33:8a:bb:d2:00:9e:2e:c8:cd:d5:50:92:b8:\n"
        "                    5c:5a:0b:99:ef:05:39:67:da:be:70:36:51:37:37:\n"
        "                    20:6f:84:ab:29:11:00:7b:38:32:ba:0b:bc:34:a6:\n"
        "                    b5:c6:a7:f0:c0:25:2d:38:0b:72:40:ab:cf:e6:ff:\n"
        "                    97:75:ff:e2:a9:3c:2a:57:ce:e4:52:20:8c:de:fe:\n"
        "                    68:ce:54:85:37:ba:b3:7f:2e:53:58:ea:9b:ac:79:\n"
        "                    6b:16:65:b8:11:88:5a:46:eb:9e:9e:80:3c:89:91:\n"
        "                    35:e0:c5:33:45:c8:86:4d:25:51:39:b1:72:97:2b:\n"
        "                    b3:c8:c9:e8:11:cd:32:41:c8:c1:56:22:7e:33:81:\n"
        "                    85:61:ab:da:9e:6e:5f:24:1c:0f:9b:fa:da:9d:86:\n"
        "                    1a:66:f6:32:2a:10:80:ea:72:7a:4a:ef:c0:f2:7c:\n"
        "                    43:02:e6:70:19:6a:e1:02:0a:00:80:51:1c:a3:03:\n"
        "                    8b:6d:89:9f:91:37:90:d6:d8:9c:73:77:06:9e:bc:\n"
        "                    95:89:66:ee:43:40:a3:ee:43:a3:f6:2d:43:dd:7b:\n"
        "                    f0:2f:0b:12:37:49:b7:81:5a:e2:54:6d:71:88:ff:\n"
        "                    fe:7e:41:25:35:4c:b4:b9:62:65:dd:9f:1f:7a:06:\n"
        "                    6e:2b:20:58:78:da:08:66:a8:f1:89:de:8f:7f:5c:\n"
        "                    5e:c2:72:33:7f:b6:8e:41:4c:26:f6:4c:d4:0e:11:\n"
        "                    44:da:c7:14:f7:8b:79:4e:53:29:87:15:b1:12:e9:\n"
        "                    19:2b:54:33:d6:2e:7f:bd:42:20:be:fc:d7:9c:b4:\n"
        "                    7a:0a:db\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Subject Key Identifier:\n"
        "                4D:7D:AE:0D:A5:5E:22:5A:6A:8F:19:61:54:B3:58:CB:7B:C0:BD:DA\n"
        "            X509v3 Authority Key Identifier:\n"
        "                keyid:4D:7D:AE:0D:A5:5E:22:5A:6A:8F:19:61:54:B3:58:CB:7B:C0:BD:DA\n"
        "\n"
        "            X509v3 Basic Constraints:\n"
        "                CA:TRUE\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         5b:40:71:96:c8:d1:57:3f:fc:f2:3c:75:fb:c9:a6:a7:63:8a:\n"
        "         22:23:96:0f:40:77:77:e2:7f:76:fc:5f:7b:1c:bd:ea:ca:f0:\n"
        "         be:1a:fd:59:e6:0e:00:d1:78:44:01:28:f4:01:68:67:78:cf:\n"
        "         78:43:36:ac:b2:5c:13:0e:2a:94:59:88:9e:64:46:42:0a:9b:\n"
        "         be:7d:2d:10:11:fe:8b:64:01:fb:00:c5:2e:47:63:c0:93:3a:\n"
        "         4a:f8:6c:fc:a9:16:58:ab:bc:7b:6b:20:31:9d:d7:d8:84:01:\n"
        "         cc:ce:52:7f:a1:18:2f:5c:c9:59:58:9a:98:b9:ef:54:d7:a0:\n"
        "         56:79:28:ba:ad:f5:e5:fd:7e:d8:d6:be:dd:25:76:6f:fa:8a:\n"
        "         07:f6:8e:0f:83:43:19:ee:96:c4:c9:54:df:19:5a:4c:ae:25:\n"
        "         57:a2:5d:d5:e8:0a:66:d8:19:e9:c4:44:ba:6a:3b:b3:86:ae:\n"
        "         44:c0:7c:6e:e5:a0:6c:45:bb:7f:34:94:e9:d3:d4:f4:04:0b:\n"
        "         eb:fc:9a:fa:67:d4:e5:83:5e:08:09:9c:70:a9:d3:0d:8a:08:\n"
        "         ed:3c:04:33:4f:ac:02:d9:5c:99:62:12:fc:0e:8d:55:8a:ce:\n"
        "         ca:28:5a:1a:9e:c9:59:8e:f0:f5:19:c7:30:1e:59:1f:3c:77:\n"
        "         6d:fc:a2:31:ec:bf:83:fd:14:26:91:68:88:05:4c:87:82:e0:\n"
        "         33:f4:ee:d8:56:97:23:3a:00:9b:e7:a2:10:c2:83:28:c6:c0:\n"
        "         c1:92:49:95:c1:d3:e1:43:e8:8f:0c:d0:ae:e3:50:17:1a:8d:\n"
        "         0f:4a:60:71:76:8e:9e:fb:15:76:cd:cd:69:2c:59:24:69:d2:\n"
        "         0f:f2:d5:0e:96:95:2b:2e:d7:81:ed:b3:7b:6f:ce:60:32:b5:\n"
        "         f0:f6:74:ea:27:3a:ee:2c:96:7b:e0:06:6c:33:25:c4:60:da:\n"
        "         76:de:c4:a1:22:b6:b1:63:57:10:3c:62:60:98:47:39:9e:38:\n"
        "         ce:c7:ef:75:75:19:d3:26:2a:cf:46:e3:b0:72:38:49:ee:c3:\n"
        "         4e:52:97:e5:e5:b8:bc:b1:45:56:98:54:0a:63:c8:87:ff:a0:\n"
        "         cb:28:12:5c:8f:a2:6e:a7:f9:50:98:2d:a5:26:08:df:16:29:\n"
        "         19:63:7f:6c:b4:41:20:f7:5d:ef:6a:90:fd:1a:08:1c:c2:4c:\n"
        "         3e:77:ea:e0:df:c0:dd:aa:a2:36:e7:e8:be:98:39:0a:68:59:\n"
        "         8e:a0:71:2f:7c:92:ab:e0:c4:c1:c2:eb:89:b6:34:ce:44:ab:\n"
        "         f9:f6:a4:c8:7b:ad:a8:bc:c9:04:7c:d5:4c:a4:d2:8b:54:23:\n"
        "         89:68:86:4e:07:36:d9:bc\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIGSTCCBDGgAwIBAgIJAO9U2PfaGOgZMA0GCSqGSIb3DQEBCwUAMIG6MQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\n"
        "aXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUg\n"
        "RW5naW5lZXJpbmcxITAfBgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEk\n"
        "MCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMB4XDTI0MDcyMzEx\n"
        "NDYyNloXDTM0MDcyMTExNDYyNlowgboxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApD\n"
        "YWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMwEQYDVQQKDApMaW5k\n"
        "ZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEhMB8GA1UE\n"
        "AwwYSW50ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZIhvcNAQkBFhVub3Jl\n"
        "cGx5QGxpbmRlbmxhYi5jb20wggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoIC\n"
        "AQDGzAf0CxcGTaYwtMcCa52kR6YJDmAaMtRrQojuxbnp+7ULYNyiRZKlu4gS/EIa\n"
        "gDJ5FmJ6l6+EKFM8wfJowE5F5Apj+TQdoovMcN/GZcC6MTLSnQzIztwREqQR+tPI\n"
        "VuIxiuP7kUDaJVXR8nWbTfq4H7Vtm+H+XejEAnkU731aszoettBgLJDcIuLFroUf\n"
        "tJ16IPivY1YlGmTznD+az2gICjfb0KNlJtuAgv/gG1HI7vatwrTyq9LohYZ3KNBj\n"
        "SnF4QeOMf3FRMa8kP/qN0NgL4n55M4q70gCeLsjN1VCSuFxaC5nvBTln2r5wNlE3\n"
        "NyBvhKspEQB7ODK6C7w0prXGp/DAJS04C3JAq8/m/5d1/+KpPCpXzuRSIIze/mjO\n"
        "VIU3urN/LlNY6puseWsWZbgRiFpG656egDyJkTXgxTNFyIZNJVE5sXKXK7PIyegR\n"
        "zTJByMFWIn4zgYVhq9qebl8kHA+b+tqdhhpm9jIqEIDqcnpK78DyfEMC5nAZauEC\n"
        "CgCAURyjA4ttiZ+RN5DW2JxzdwaevJWJZu5DQKPuQ6P2LUPde/AvCxI3SbeBWuJU\n"
        "bXGI//5+QSU1TLS5YmXdnx96Bm4rIFh42ghmqPGJ3o9/XF7CcjN/to5BTCb2TNQO\n"
        "EUTaxxT3i3lOUymHFbES6RkrVDPWLn+9QiC+/NectHoK2wIDAQABo1AwTjAdBgNV\n"
        "HQ4EFgQUTX2uDaVeIlpqjxlhVLNYy3vAvdowHwYDVR0jBBgwFoAUTX2uDaVeIlpq\n"
        "jxlhVLNYy3vAvdowDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAgEAW0Bx\n"
        "lsjRVz/88jx1+8mmp2OKIiOWD0B3d+J/dvxfexy96srwvhr9WeYOANF4RAEo9AFo\n"
        "Z3jPeEM2rLJcEw4qlFmInmRGQgqbvn0tEBH+i2QB+wDFLkdjwJM6Svhs/KkWWKu8\n"
        "e2sgMZ3X2IQBzM5Sf6EYL1zJWViamLnvVNegVnkouq315f1+2Na+3SV2b/qKB/aO\n"
        "D4NDGe6WxMlU3xlaTK4lV6Jd1egKZtgZ6cREumo7s4auRMB8buWgbEW7fzSU6dPU\n"
        "9AQL6/ya+mfU5YNeCAmccKnTDYoI7TwEM0+sAtlcmWIS/A6NVYrOyihaGp7JWY7w\n"
        "9RnHMB5ZHzx3bfyiMey/g/0UJpFoiAVMh4LgM/Tu2FaXIzoAm+eiEMKDKMbAwZJJ\n"
        "lcHT4UPojwzQruNQFxqND0pgcXaOnvsVds3NaSxZJGnSD/LVDpaVKy7Xge2ze2/O\n"
        "YDK18PZ06ic67iyWe+AGbDMlxGDadt7EoSK2sWNXEDxiYJhHOZ44zsfvdXUZ0yYq\n"
        "z0bjsHI4Se7DTlKX5eW4vLFFVphUCmPIh/+gyygSXI+ibqf5UJgtpSYI3xYpGWN/\n"
        "bLRBIPdd72qQ/RoIHMJMPnfq4N/A3aqiNufovpg5CmhZjqBxL3ySq+DEwcLribY0\n"
        "zkSr+fakyHutqLzJBHzVTKTSi1QjiWiGTgc22bw=\n"
        "-----END CERTIFICATE-----\n"
    );


    const std::string mPemIntermediateCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number: 85:bb:4b:66:26:db:9a:c6\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: Jul 23 11:46:33 2024 GMT\n"
        "            Not After : Jul 21 11:46:33 2034 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Intermediate CA/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:be:f7:d2:cb:e4:5c:46:7b:e2:11:22:89:72:da:\n"
        "                    77:72:ec:05:87:19:f7:77:07:fd:67:d7:af:13:d5:\n"
        "                    76:12:92:dd:69:4d:22:47:b0:3d:94:8a:6a:95:85:\n"
        "                    34:b8:78:c3:9d:63:32:b1:4b:0a:b6:0e:05:7b:ab:\n"
        "                    06:23:fc:0d:21:b5:fc:c6:6a:5a:36:be:6e:fc:c7:\n"
        "                    47:97:a3:18:2e:33:cd:0e:8a:75:2b:b7:29:e9:68:\n"
        "                    4a:90:53:45:db:73:ff:b3:e5:c1:d4:6b:dd:3a:b1:\n"
        "                    ef:53:9f:23:e9:c6:87:ce:67:b9:fb:a4:d5:76:21:\n"
        "                    03:cb:c5:72:6b:c5:a6:07:55:fb:47:90:e8:92:38:\n"
        "                    73:14:11:8e:ff:21:b9:35:64:5a:61:c7:fc:1f:e4:\n"
        "                    4d:47:e5:03:cc:0b:c3:69:66:71:84:0c:18:2f:61:\n"
        "                    7f:34:dd:f2:91:e3:b7:9d:a8:b8:db:3f:6e:6f:96:\n"
        "                    fa:34:06:82:04:c8:18:cc:de:8b:7f:26:b5:48:53:\n"
        "                    fb:fb:15:7b:0e:38:60:fe:da:21:98:8d:73:07:b2:\n"
        "                    6b:fd:ad:21:59:e7:84:66:e1:04:16:1c:be:13:34:\n"
        "                    28:43:2c:09:3d:e4:77:2a:a4:ad:6d:f9:26:04:f7:\n"
        "                    43:73:9b:d9:ea:1a:43:6a:b4:db:88:f8:f9:bd:34:\n"
        "                    f8:a6:e8:7a:ab:b4:b2:e1:29:47:a6:ba:b8:65:9c:\n"
        "                    c6:b3:af:13:43:38:ef:2a:05:77:9f:8f:f0:0c:56:\n"
        "                    21:c2:92:d2:2c:c3:32:50:d1:62:ae:51:fc:99:e6:\n"
        "                    b8:38:f8:83:1d:8d:40:11:e0:1d:51:5d:3f:fa:55:\n"
        "                    61:b6:18:09:1e:71:af:95:64:9c:ea:c6:11:64:f0:\n"
        "                    a8:02:7d:bb:c8:54:2e:57:48:32:7c:51:66:0d:d6:\n"
        "                    3e:0e:ed:5e:30:a8:a6:47:03:64:5c:89:21:45:90:\n"
        "                    e1:4c:91:bc:bd:81:6e:73:a9:14:27:e6:0d:6d:38:\n"
        "                    dc:50:9d:b2:56:66:60:6c:66:b9:5d:bb:8c:96:2d:\n"
        "                    89:5e:0d:2b:ed:b8:03:31:ce:0a:ff:82:03:f5:b2:\n"
        "                    3b:e5:27:de:61:d8:8f:bf:a2:6a:64:b0:4a:87:23:\n"
        "                    40:28:a3:f1:ec:96:50:cd:83:50:2d:78:71:92:f2:\n"
        "                    88:75:b0:9d:cd:0b:e4:62:a6:a5:63:11:fc:b4:ba:\n"
        "                    9f:c6:67:40:2c:ad:a4:ef:94:f0:f9:a0:ba:e1:52:\n"
        "                    2e:27:d9:6b:1d:82:23:ed:3c:0b:0b:d2:bc:14:be:\n"
        "                    6d:b1:69:ad:3e:25:3a:66:d2:d1:af:9f:88:45:25:\n"
        "                    6b:6e:be:1f:a0:e7:b2:9f:6d:24:94:0d:f4:c2:75:\n"
        "                    f9:1f:5d\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Basic Constraints:\n"
        "                CA:TRUE, pathlen:0\n"
        "            X509v3 Key Usage:\n"
        "                Digital Signature, Certificate Sign, CRL Sign\n"
        "            X509v3 Subject Key Identifier:\n"
        "                56:98:DC:45:25:11:E2:8C:2B:EA:D6:C6:E2:C8:BE:2C:C8:69:FF:FF\n"
        "            X509v3 Authority Key Identifier:\n"
        "                keyid:4D:7D:AE:0D:A5:5E:22:5A:6A:8F:19:61:54:B3:58:CB:7B:C0:BD:DA\n"
        "                DirName:/C=US/ST=California/L=San Francisco/O=Linden Lab/OU=Second Life Engineering/CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "                serial:EF:54:D8:F7:DA:18:E8:19\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         ae:d0:30:ac:31:49:20:86:0b:34:01:58:08:94:68:cc:38:9c:\n"
        "         f7:13:5c:46:19:33:ed:54:5e:e4:43:f3:59:33:5c:50:d9:89:\n"
        "         8b:ee:75:67:a8:c7:0e:d1:30:c2:4e:a3:2e:a8:64:2d:6a:a8:\n"
        "         f4:bd:b1:32:dc:bc:46:48:5d:1a:18:d8:e8:0b:8c:fe:7b:51:\n"
        "         d9:dd:b9:e3:4b:d1:f9:e0:22:46:dd:37:5b:b2:cb:72:8e:9c:\n"
        "         4b:da:67:df:fd:ce:86:49:21:31:4e:99:b6:d4:38:0b:14:5d:\n"
        "         ad:97:ba:8f:e2:08:15:85:73:eb:4a:7d:01:49:af:63:ae:2d:\n"
        "         e3:9d:0a:d7:11:c2:03:d3:15:21:97:be:3d:d2:ea:ab:cc:93:\n"
        "         16:98:64:80:72:eb:c2:78:0a:09:69:c4:2b:5d:df:30:7b:be:\n"
        "         9b:02:34:73:62:9f:95:b1:cf:08:e8:9e:57:a8:37:31:cf:2c:\n"
        "         8c:18:b1:d5:7a:25:90:d6:b6:76:28:1b:e2:b1:cf:1b:f1:ef:\n"
        "         dd:2f:d3:07:af:81:e3:5f:fc:5a:e7:3c:a9:37:0d:9c:78:5b:\n"
        "         58:dc:89:54:70:a4:5b:ff:9f:64:30:a3:85:12:32:69:a5:02:\n"
        "         73:d9:1d:ff:69:1f:d4:97:8f:d0:a8:90:8c:dd:2e:45:a1:b1:\n"
        "         e3:8a:82:fc:fc:08:41:01:51:92:87:9a:09:7b:35:c3:cc:48:\n"
        "         81:39:30:a9:f4:41:3b:06:a3:06:21:cc:4b:bc:1b:76:58:94:\n"
        "         d1:e4:22:70:7f:20:7e:7a:b4:fa:7f:e8:79:c1:8c:89:9e:e9:\n"
        "         e3:72:2a:43:72:47:9e:bb:26:ed:64:2c:c8:54:f7:b4:95:c2:\n"
        "         c4:e9:8b:df:d5:10:a7:ed:a5:7a:94:97:c4:76:45:e3:6c:c0:\n"
        "         0e:a6:2a:76:d5:1d:2f:ad:99:32:c6:7b:f6:41:e0:65:37:0f:\n"
        "         c0:1f:c5:99:4a:75:fd:6c:e0:f1:f0:58:49:2d:81:10:ca:d8:\n"
        "         eb:2b:c3:9b:a9:d9:a9:f5:6c:6d:26:fd:b8:32:92:58:f4:65:\n"
        "         0b:d1:8e:03:1e:d5:6a:95:d4:46:9e:65:dd:e5:85:36:e6:31:\n"
        "         77:3a:1a:20:2b:07:b7:f1:9a:4e:8d:54:22:5a:54:1c:72:5c:\n"
        "         1f:b4:1a:5b:21:ed:06:5a:9a:e5:3c:01:c9:9b:af:50:61:f2:\n"
        "         29:6b:ec:6d:19:bb:2e:02:94:ca:36:71:ef:45:39:f1:a5:25:\n"
        "         10:0e:90:bc:a7:b3:5b:ab:af:f1:19:88:6a:09:2f:1f:d0:24:\n"
        "         a8:62:ed:d9:1a:65:89:65:16:a5:55:de:33:e8:7a:81:66:72:\n"
        "         91:17:5e:1d:22:72:f7:b8\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIHNjCCBR6gAwIBAgIJAIW7S2Ym25rGMA0GCSqGSIb3DQEBCwUAMIG6MQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\n"
        "aXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUg\n"
        "RW5naW5lZXJpbmcxITAfBgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEk\n"
        "MCIGCSqGSIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tMB4XDTI0MDcyMzEx\n"
        "NDYzM1oXDTM0MDcyMTExNDYzM1owgcIxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApD\n"
        "YWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMwEQYDVQQKDApMaW5k\n"
        "ZW4gTGFiMSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEpMCcGA1UE\n"
        "AwwgSW50ZWdyYXRpb24gVGVzdCBJbnRlcm1lZGlhdGUgQ0ExJDAiBgkqhkiG9w0B\n"
        "CQEWFW5vcmVwbHlAbGluZGVubGFiLmNvbTCCAiIwDQYJKoZIhvcNAQEBBQADggIP\n"
        "ADCCAgoCggIBAL730svkXEZ74hEiiXLad3LsBYcZ93cH/WfXrxPVdhKS3WlNIkew\n"
        "PZSKapWFNLh4w51jMrFLCrYOBXurBiP8DSG1/MZqWja+bvzHR5ejGC4zzQ6KdSu3\n"
        "KeloSpBTRdtz/7PlwdRr3Tqx71OfI+nGh85nufuk1XYhA8vFcmvFpgdV+0eQ6JI4\n"
        "cxQRjv8huTVkWmHH/B/kTUflA8wLw2lmcYQMGC9hfzTd8pHjt52ouNs/bm+W+jQG\n"
        "ggTIGMzei38mtUhT+/sVew44YP7aIZiNcweya/2tIVnnhGbhBBYcvhM0KEMsCT3k\n"
        "dyqkrW35JgT3Q3Ob2eoaQ2q024j4+b00+Kboequ0suEpR6a6uGWcxrOvE0M47yoF\n"
        "d5+P8AxWIcKS0izDMlDRYq5R/JnmuDj4gx2NQBHgHVFdP/pVYbYYCR5xr5VknOrG\n"
        "EWTwqAJ9u8hULldIMnxRZg3WPg7tXjCopkcDZFyJIUWQ4UyRvL2BbnOpFCfmDW04\n"
        "3FCdslZmYGxmuV27jJYtiV4NK+24AzHOCv+CA/WyO+Un3mHYj7+iamSwSocjQCij\n"
        "8eyWUM2DUC14cZLyiHWwnc0L5GKmpWMR/LS6n8ZnQCytpO+U8PmguuFSLifZax2C\n"
        "I+08CwvSvBS+bbFprT4lOmbS0a+fiEUla26+H6Dnsp9tJJQN9MJ1+R9dAgMBAAGj\n"
        "ggEzMIIBLzAPBgNVHRMECDAGAQH/AgEAMAsGA1UdDwQEAwIBhjAdBgNVHQ4EFgQU\n"
        "VpjcRSUR4owr6tbG4si+LMhp//8wge8GA1UdIwSB5zCB5IAUTX2uDaVeIlpqjxlh\n"
        "VLNYy3vAvdqhgcCkgb0wgboxCzAJBgNVBAYTAlVTMRMwEQYDVQQIDApDYWxpZm9y\n"
        "bmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMRMwEQYDVQQKDApMaW5kZW4gTGFi\n"
        "MSAwHgYDVQQLDBdTZWNvbmQgTGlmZSBFbmdpbmVlcmluZzEhMB8GA1UEAwwYSW50\n"
        "ZWdyYXRpb24gVGVzdCBSb290IENBMSQwIgYJKoZIhvcNAQkBFhVub3JlcGx5QGxp\n"
        "bmRlbmxhYi5jb22CCQDvVNj32hjoGTANBgkqhkiG9w0BAQsFAAOCAgEArtAwrDFJ\n"
        "IIYLNAFYCJRozDic9xNcRhkz7VRe5EPzWTNcUNmJi+51Z6jHDtEwwk6jLqhkLWqo\n"
        "9L2xMty8RkhdGhjY6AuM/ntR2d2540vR+eAiRt03W7LLco6cS9pn3/3OhkkhMU6Z\n"
        "ttQ4CxRdrZe6j+IIFYVz60p9AUmvY64t450K1xHCA9MVIZe+PdLqq8yTFphkgHLr\n"
        "wngKCWnEK13fMHu+mwI0c2KflbHPCOieV6g3Mc8sjBix1XolkNa2digb4rHPG/Hv\n"
        "3S/TB6+B41/8Wuc8qTcNnHhbWNyJVHCkW/+fZDCjhRIyaaUCc9kd/2kf1JeP0KiQ\n"
        "jN0uRaGx44qC/PwIQQFRkoeaCXs1w8xIgTkwqfRBOwajBiHMS7wbdliU0eQicH8g\n"
        "fnq0+n/oecGMiZ7p43IqQ3JHnrsm7WQsyFT3tJXCxOmL39UQp+2lepSXxHZF42zA\n"
        "DqYqdtUdL62ZMsZ79kHgZTcPwB/FmUp1/Wzg8fBYSS2BEMrY6yvDm6nZqfVsbSb9\n"
        "uDKSWPRlC9GOAx7VapXURp5l3eWFNuYxdzoaICsHt/GaTo1UIlpUHHJcH7QaWyHt\n"
        "Blqa5TwByZuvUGHyKWvsbRm7LgKUyjZx70U58aUlEA6QvKezW6uv8RmIagkvH9Ak\n"
        "qGLt2RpliWUWpVXeM+h6gWZykRdeHSJy97g=\n"
        "-----END CERTIFICATE-----\n"
    );

    const std::string mPemChildCert(
        "Certificate:\n"
        "    Data:\n"
        "        Version: 3 (0x2)\n"
        "        Serial Number: 9e:8d:34:13:e7:9b:f9:31\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "        Issuer: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Intermediate CA/emailAddress=noreply@lindenlab.com\n"
        "        Validity\n"
        "            Not Before: Jul 23 11:46:39 2024 GMT\n"
        "            Not After : Jul 21 11:46:39 2034 GMT\n"
        "        Subject: C=US, ST=California, L=San Francisco, O=Linden Lab, OU=Second Life Engineering, CN=Integration Test Server Cert/emailAddress=noreply@lindenlab.com\n"
        "        Subject Public Key Info:\n"
        "            Public Key Algorithm: rsaEncryption\n"
        "                Public-Key: (4096 bit)\n"
        "                Modulus:\n"
        "                    00:d8:ac:0c:27:8f:ea:c0:4d:21:e4:75:55:31:57:\n"
        "                    83:46:47:14:1e:f5:67:ae:98:60:c4:97:6d:e8:53:\n"
        "                    f2:4d:3b:ec:6f:08:bc:1e:c0:e2:a6:75:b5:90:1d:\n"
        "                    30:a2:59:68:32:10:2b:29:67:fc:99:f1:24:6a:36:\n"
        "                    73:60:31:6b:c7:a0:b8:b0:38:60:b1:59:23:2c:ab:\n"
        "                    25:a2:c8:b0:bc:2c:c6:d7:4c:87:37:1b:5e:51:a4:\n"
        "                    63:3e:c4:6d:ed:da:5e:d3:ad:8a:6d:52:e4:87:38:\n"
        "                    33:76:cf:f2:86:58:b3:10:a4:91:8d:3d:4f:27:9a:\n"
        "                    8b:b4:d7:67:90:31:1c:f5:7f:78:af:6f:f2:dd:39:\n"
        "                    d0:16:16:7b:46:ad:88:1b:3b:74:6b:10:29:8b:64:\n"
        "                    ba:ed:9f:a7:69:99:55:8f:73:0d:18:a3:7f:40:20:\n"
        "                    3a:41:4a:94:39:62:8b:fe:c6:9d:79:d0:cd:1c:e2:\n"
        "                    d4:74:bb:43:75:eb:86:8b:30:c1:8d:cc:14:ab:75:\n"
        "                    2e:f5:3e:0c:05:cb:e4:c3:92:d8:81:8c:df:a5:4e:\n"
        "                    2e:0b:ae:17:15:9b:e6:dd:9e:16:46:42:27:92:8a:\n"
        "                    0e:3a:74:1e:d1:3f:ee:7e:a5:d7:ec:1c:63:d4:96:\n"
        "                    5b:36:f9:15:ee:da:66:ac:5e:de:91:d9:08:24:fb:\n"
        "                    5d:fc:9b:77:dd:ff:20:a6:67:6f:48:41:5e:5a:ac:\n"
        "                    13:a4:2c:2a:f2:a3:15:86:e2:84:33:34:e3:91:27:\n"
        "                    8b:37:ba:b0:c7:5e:1a:0d:b9:f2:4e:0c:55:e6:bb:\n"
        "                    d9:63:f5:05:7b:aa:19:e5:57:ce:a5:b1:46:4b:b3:\n"
        "                    04:f6:a0:97:26:ed:48:ed:97:93:a6:75:b1:a3:42:\n"
        "                    fc:cc:57:89:da:44:e9:16:a6:30:2c:01:8e:f2:ed:\n"
        "                    be:45:05:08:8a:af:1e:07:51:89:cf:51:4c:aa:f3:\n"
        "                    b3:f0:6f:db:21:80:11:32:0a:23:e2:ff:cc:59:15:\n"
        "                    eb:ff:d2:b8:d6:a1:c1:b4:96:12:82:bf:3f:68:ad:\n"
        "                    c8:61:50:f8:88:4f:d0:be:8e:29:64:1a:16:a5:d9:\n"
        "                    29:76:16:cd:70:37:c4:f2:1f:4e:c6:57:36:dd:c1:\n"
        "                    27:19:72:ef:98:7e:34:25:3f:76:b1:ea:15:b2:38:\n"
        "                    6e:d3:43:03:7a:2b:78:91:9a:19:26:2a:31:b7:5e:\n"
        "                    b7:22:c4:fd:bf:93:10:a4:23:3f:d7:79:53:28:5d:\n"
        "                    2e:ba:0c:b0:5e:0a:b4:c4:a1:71:75:88:1b:b2:0e:\n"
        "                    2c:67:08:7b:f0:f6:37:d3:aa:39:50:03:a3:7c:17:\n"
        "                    1d:52:52:2a:6b:d0:a2:54:2e:ba:11:bc:26:a9:16:\n"
        "                    a6:1b:79\n"
        "                Exponent: 65537 (0x10001)\n"
        "        X509v3 extensions:\n"
        "            X509v3 Basic Constraints:\n"
        "                CA:FALSE\n"
        "            X509v3 Key Usage:\n"
        "                Digital Signature, Key Encipherment\n"
        "            X509v3 Extended Key Usage:\n"
        "                TLS Web Server Authentication\n"
        "            X509v3 Subject Key Identifier:\n"
        "                7B:1A:F9:2B:C4:B2:F6:AE:D6:F2:8E:B1:73:FB:DD:11:CA:DB:F8:87\n"
        "            X509v3 Authority Key Identifier:\n"
        "                keyid:56:98:DC:45:25:11:E2:8C:2B:EA:D6:C6:E2:C8:BE:2C:C8:69:FF:FF\n"
        "                DirName:/C=US/ST=California/L=San Francisco/O=Linden Lab/OU=Second Life Engineering/CN=Integration Test Root CA/emailAddress=noreply@lindenlab.com\n"
        "                serial:85:BB:4B:66:26:DB:9A:C6\n"
        "    Signature Algorithm: sha256WithRSAEncryption\n"
        "         ad:7c:50:12:24:62:62:83:e9:dd:81:1a:12:1c:6d:ae:1e:a6:\n"
        "         01:cc:93:8b:ac:83:7c:3d:57:d7:7f:d2:13:40:82:c7:27:07:\n"
        "         31:d8:c4:01:04:64:9c:dc:ae:7b:52:bd:f5:62:7a:d0:7c:13:\n"
        "         1a:19:86:6a:ce:9a:ba:69:07:77:75:b6:67:56:d0:c3:8d:6f:\n"
        "         59:5f:ac:31:83:32:2c:4f:8c:85:8c:f3:56:5b:e0:83:16:19:\n"
        "         c9:55:4d:56:2c:e0:06:f8:71:85:4b:7e:c6:20:b3:f6:5b:85:\n"
        "         6a:b7:0f:0e:0c:75:38:6a:aa:53:cc:b0:bf:c1:fd:a1:01:8a:\n"
        "         7e:5a:0b:4d:51:fc:1b:14:b0:8d:62:17:b7:5d:6a:64:30:80:\n"
        "         aa:50:9a:23:9e:19:46:11:9d:49:d1:35:81:87:80:8c:9c:71:\n"
        "         61:26:07:23:5d:a7:ea:4e:0c:53:77:bd:eb:18:6d:63:8b:2c:\n"
        "         e1:83:bb:bb:f8:3e:7c:e8:0d:19:1e:be:35:aa:99:0f:c7:25:\n"
        "         0c:a8:f9:74:02:c8:4c:8e:bb:13:18:fd:aa:21:34:bc:2d:9f:\n"
        "         10:96:e2:99:e3:9a:d7:91:0e:1e:77:20:70:e9:b4:63:25:f8:\n"
        "         ea:14:1f:24:b0:6a:8b:2a:f4:61:b1:0d:7d:18:bc:1d:6d:04:\n"
        "         11:b2:9f:a2:a7:55:be:2b:2c:2f:c1:d8:95:13:73:af:1c:96:\n"
        "         49:30:9c:9c:94:81:6c:9b:a7:87:5c:cf:46:95:95:4a:6f:bf:\n"
        "         df:c9:3d:74:3e:24:6e:44:1e:14:8b:68:23:e4:00:b5:a5:b7:\n"
        "         5b:a9:ea:16:5f:fa:b1:d3:1a:b1:9b:36:ef:a4:7a:6f:a3:b0:\n"
        "         97:35:ac:70:c0:cc:8e:a2:d3:40:0e:c1:70:0b:d5:ce:cd:51:\n"
        "         82:8a:40:72:04:8d:62:af:ba:a8:e7:a8:e9:b9:99:b7:5c:5d:\n"
        "         27:96:b2:3d:f9:0d:26:8c:3f:db:ac:86:97:be:f1:2c:0b:ca:\n"
        "         90:07:93:96:f4:75:c3:e8:4c:f6:a8:a2:3f:da:11:21:e7:b1:\n"
        "         8c:62:36:ae:91:a9:2a:73:ba:67:f5:24:16:c3:ee:b7:b1:b4:\n"
        "         e3:8a:28:23:84:cf:38:c6:f0:8e:21:f6:b8:76:9a:6d:d1:e3:\n"
        "         74:81:7a:22:20:a0:82:2a:31:8a:ba:44:0b:61:5a:aa:ba:c6:\n"
        "         07:99:36:0a:24:06:2f:8e:c1:1c:4b:f0:65:72:fb:e9:b5:31:\n"
        "         59:13:2c:c6:f8:5b:91:e2:d8:96:f3:1a:06:0b:2a:62:12:4d:\n"
        "         5e:65:c9:e9:e4:00:99:a6:d3:60:1f:c3:d6:cc:a6:9b:a5:14:\n"
        "         1b:4d:db:e7:3d:52:7e:2c\n"
        "-----BEGIN CERTIFICATE-----\n"
        "MIIHSTCCBTGgAwIBAgIJAJ6NNBPnm/kxMA0GCSqGSIb3DQEBCwUAMIHCMQswCQYD\n"
        "VQQGEwJVUzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5j\n"
        "aXNjbzETMBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUg\n"
        "RW5naW5lZXJpbmcxKTAnBgNVBAMMIEludGVncmF0aW9uIFRlc3QgSW50ZXJtZWRp\n"
        "YXRlIENBMSQwIgYJKoZIhvcNAQkBFhVub3JlcGx5QGxpbmRlbmxhYi5jb20wHhcN\n"
        "MjQwNzIzMTE0NjM5WhcNMzQwNzIxMTE0NjM5WjCBvjELMAkGA1UEBhMCVVMxEzAR\n"
        "BgNVBAgMCkNhbGlmb3JuaWExFjAUBgNVBAcMDVNhbiBGcmFuY2lzY28xEzARBgNV\n"
        "BAoMCkxpbmRlbiBMYWIxIDAeBgNVBAsMF1NlY29uZCBMaWZlIEVuZ2luZWVyaW5n\n"
        "MSUwIwYDVQQDDBxJbnRlZ3JhdGlvbiBUZXN0IFNlcnZlciBDZXJ0MSQwIgYJKoZI\n"
        "hvcNAQkBFhVub3JlcGx5QGxpbmRlbmxhYi5jb20wggIiMA0GCSqGSIb3DQEBAQUA\n"
        "A4ICDwAwggIKAoICAQDYrAwnj+rATSHkdVUxV4NGRxQe9WeumGDEl23oU/JNO+xv\n"
        "CLwewOKmdbWQHTCiWWgyECspZ/yZ8SRqNnNgMWvHoLiwOGCxWSMsqyWiyLC8LMbX\n"
        "TIc3G15RpGM+xG3t2l7TrYptUuSHODN2z/KGWLMQpJGNPU8nmou012eQMRz1f3iv\n"
        "b/LdOdAWFntGrYgbO3RrECmLZLrtn6dpmVWPcw0Yo39AIDpBSpQ5Yov+xp150M0c\n"
        "4tR0u0N164aLMMGNzBSrdS71PgwFy+TDktiBjN+lTi4LrhcVm+bdnhZGQieSig46\n"
        "dB7RP+5+pdfsHGPUlls2+RXu2masXt6R2Qgk+138m3fd/yCmZ29IQV5arBOkLCry\n"
        "oxWG4oQzNOORJ4s3urDHXhoNufJODFXmu9lj9QV7qhnlV86lsUZLswT2oJcm7Ujt\n"
        "l5OmdbGjQvzMV4naROkWpjAsAY7y7b5FBQiKrx4HUYnPUUyq87Pwb9shgBEyCiPi\n"
        "/8xZFev/0rjWocG0lhKCvz9orchhUPiIT9C+jilkGhal2Sl2Fs1wN8TyH07GVzbd\n"
        "wScZcu+YfjQlP3ax6hWyOG7TQwN6K3iRmhkmKjG3XrcixP2/kxCkIz/XeVMoXS66\n"
        "DLBeCrTEoXF1iBuyDixnCHvw9jfTqjlQA6N8Fx1SUipr0KJULroRvCapFqYbeQID\n"
        "AQABo4IBQjCCAT4wCQYDVR0TBAIwADALBgNVHQ8EBAMCBaAwEwYDVR0lBAwwCgYI\n"
        "KwYBBQUHAwEwHQYDVR0OBBYEFHsa+SvEsvau1vKOsXP73RHK2/iHMIHvBgNVHSME\n"
        "gecwgeSAFFaY3EUlEeKMK+rWxuLIvizIaf//oYHApIG9MIG6MQswCQYDVQQGEwJV\n"
        "UzETMBEGA1UECAwKQ2FsaWZvcm5pYTEWMBQGA1UEBwwNU2FuIEZyYW5jaXNjbzET\n"
        "MBEGA1UECgwKTGluZGVuIExhYjEgMB4GA1UECwwXU2Vjb25kIExpZmUgRW5naW5l\n"
        "ZXJpbmcxITAfBgNVBAMMGEludGVncmF0aW9uIFRlc3QgUm9vdCBDQTEkMCIGCSqG\n"
        "SIb3DQEJARYVbm9yZXBseUBsaW5kZW5sYWIuY29tggkAhbtLZibbmsYwDQYJKoZI\n"
        "hvcNAQELBQADggIBAK18UBIkYmKD6d2BGhIcba4epgHMk4usg3w9V9d/0hNAgscn\n"
        "BzHYxAEEZJzcrntSvfVietB8ExoZhmrOmrppB3d1tmdW0MONb1lfrDGDMixPjIWM\n"
        "81Zb4IMWGclVTVYs4Ab4cYVLfsYgs/ZbhWq3Dw4MdThqqlPMsL/B/aEBin5aC01R\n"
        "/BsUsI1iF7ddamQwgKpQmiOeGUYRnUnRNYGHgIyccWEmByNdp+pODFN3vesYbWOL\n"
        "LOGDu7v4PnzoDRkevjWqmQ/HJQyo+XQCyEyOuxMY/aohNLwtnxCW4pnjmteRDh53\n"
        "IHDptGMl+OoUHySwaosq9GGxDX0YvB1tBBGyn6KnVb4rLC/B2JUTc68clkkwnJyU\n"
        "gWybp4dcz0aVlUpvv9/JPXQ+JG5EHhSLaCPkALWlt1up6hZf+rHTGrGbNu+kem+j\n"
        "sJc1rHDAzI6i00AOwXAL1c7NUYKKQHIEjWKvuqjnqOm5mbdcXSeWsj35DSaMP9us\n"
        "hpe+8SwLypAHk5b0dcPoTPaooj/aESHnsYxiNq6RqSpzumf1JBbD7rextOOKKCOE\n"
        "zzjG8I4h9rh2mm3R43SBeiIgoIIqMYq6RAthWqq6xgeZNgokBi+OwRxL8GVy++m1\n"
        "MVkTLMb4W5Hi2JbzGgYLKmISTV5lyenkAJmm02Afw9bMppulFBtN2+c9Un4s\n"
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
            validation_bio = BIO_new_mem_buf((void*)mPemTestCert.c_str(), static_cast<S32>(mPemTestCert.length()));
            PEM_read_bio_X509(validation_bio, &mX509TestCert, 0, NULL);
            BIO_free(validation_bio);

            validation_bio = BIO_new_mem_buf((void*)mPemRootCert.c_str(), static_cast<S32>(mPemRootCert.length()));
            PEM_read_bio_X509(validation_bio, &mX509RootCert, 0, NULL);
            BIO_free(validation_bio);

            validation_bio = BIO_new_mem_buf((void*)mPemIntermediateCert.c_str(), static_cast<S32>(mPemIntermediateCert.length()));
            PEM_read_bio_X509(validation_bio, &mX509IntermediateCert, 0, NULL);
            BIO_free(validation_bio);

            validation_bio = BIO_new_mem_buf((void*)mPemChildCert.c_str(), static_cast<S32>(mPemChildCert.length()));
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
                      "emailAddress=noreply@lindenlab.com,CN=Integration Test Intermediate CA,OU=Second Life Engineering,O=Linden Lab,L=San Francisco,ST=California,C=US");
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
        ensure_equals("serial number", (std::string)llsd_cert["serial_number"], "9E8D3413E79BF931");
        ensure_equals("valid from", (std::string)llsd_cert["valid_from"], "2024-07-23T11:46:39Z");
        ensure_equals("valid to", (std::string)llsd_cert["valid_to"], "2034-07-21T11:46:39Z");
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
        handler->saveCredential(my_cred, false);
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
        find_info["subjectKeyIdentifier"] = "7b:1a:f9:2b:c4:b2:f6:ae:d6:f2:8e:b1:73:fb:dd:11:ca:db:f8:87";
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
        ensure_equals("two elements in store [1]", test_chain->size(), 1);
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
        ensure_equals("two elements in store [2]", test_chain->size(), 2);
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
        ensure_equals("two elements in store [3]", test_chain->size(), 1);
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
        ensure_equals("two elements in store [4]", test_chain->size(), 2);
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

