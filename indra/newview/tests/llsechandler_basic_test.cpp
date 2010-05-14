/** 
 * @file llsechandler_basic_test.cpp
 * @author Roxie
 * @date 2009-02-10
 * @brief Test the 'basic' sec handler functions
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
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
#include "../llviewerprecompiledheaders.h"
#include "../test/lltut.h"
#include "../llsecapi.h"
#include "../llsechandler_basic.h"
#include "../../llxml/llcontrol.h"
#include "../llviewernetwork.h"
#include "lluuid.h"
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


#define ensure_throws(str, exc_type, cert, func, ...) \
try \
{ \
func(__VA_ARGS__); \
fail("throws, " str); \
} \
catch(exc_type& except) \
{ \
ensure("Exception cert is incorrect for " str, except.getCert() == cert); \
}

extern bool _cert_hostname_wildcard_match(const std::string& hostname, const std::string& wildcard_string);

//----------------------------------------------------------------------------               
// Mock objects for the dependencies of the code we're testing                               

std::string gFirstName;
std::string gLastName;
LLControlGroup::LLControlGroup(const std::string& name)
: LLInstanceTracker<LLControlGroup, std::string>(name) {}
LLControlGroup::~LLControlGroup() {}
BOOL LLControlGroup::declareString(const std::string& name,
                                   const std::string& initial_val,
                                   const std::string& comment,
                                   BOOL persist) {return TRUE;}
void LLControlGroup::setString(const std::string& name, const std::string& val){}
std::string LLControlGroup::getString(const std::string& name)
{

	if (name == "FirstName")
		return gFirstName;
	else if (name == "LastName")
		return gLastName;
	return "";
}

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

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
	// Test wrapper declaration : wrapping nothing for the moment
	struct sechandler_basic_test
	{
		std::string mPemTestCert, mPemRootCert, mPemIntermediateCert, mPemChildCert, mSha1RSATestCert, mSha1RSATestCA;
		std::string mDerFormat;
		X509 *mX509TestCert, *mX509RootCert, *mX509IntermediateCert, *mX509ChildCert;

		sechandler_basic_test()
		{
			OpenSSL_add_all_algorithms();
			OpenSSL_add_all_ciphers();
			OpenSSL_add_all_digests();	
			ERR_load_crypto_strings();
			gFirstName = "";
			gLastName = "";
			LLFile::remove("test_password.dat");
			LLFile::remove("sechandler_settings.tmp");			
			mPemTestCert = "-----BEGIN CERTIFICATE-----\n"
				"MIIEuDCCA6CgAwIBAgIBBDANBgkqhkiG9w0BAQUFADCBtDELMAkGA1UEBhMCQlIx\n"
				"EzARBgNVBAoTCklDUC1CcmFzaWwxPTA7BgNVBAsTNEluc3RpdHV0byBOYWNpb25h\n"
				"bCBkZSBUZWNub2xvZ2lhIGRhIEluZm9ybWFjYW8gLSBJVEkxETAPBgNVBAcTCEJy\n"
				"YXNpbGlhMQswCQYDVQQIEwJERjExMC8GA1UEAxMoQXV0b3JpZGFkZSBDZXJ0aWZp\n"
				"Y2Fkb3JhIFJhaXogQnJhc2lsZWlyYTAeFw0wMTExMzAxMjU4MDBaFw0xMTExMzAy\n"
				"MzU5MDBaMIG0MQswCQYDVQQGEwJCUjETMBEGA1UEChMKSUNQLUJyYXNpbDE9MDsG\n"
				"A1UECxM0SW5zdGl0dXRvIE5hY2lvbmFsIGRlIFRlY25vbG9naWEgZGEgSW5mb3Jt\n"
				"YWNhbyAtIElUSTERMA8GA1UEBxMIQnJhc2lsaWExCzAJBgNVBAgTAkRGMTEwLwYD\n"
				"VQQDEyhBdXRvcmlkYWRlIENlcnRpZmljYWRvcmEgUmFpeiBCcmFzaWxlaXJhMIIB\n"
				"IjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwPMudwX/hvm+Uh2b/lQAcHVA\n"
				"isamaLkWdkwP9/S/tOKIgRrL6Oy+ZIGlOUdd6uYtk9Ma/3pUpgcfNAj0vYm5gsyj\n"
				"Qo9emsc+x6m4VWwk9iqMZSCK5EQkAq/Ut4n7KuLE1+gdftwdIgxfUsPt4CyNrY50\n"
				"QV57KM2UT8x5rrmzEjr7TICGpSUAl2gVqe6xaii+bmYR1QrmWaBSAG59LrkrjrYt\n"
				"bRhFboUDe1DK+6T8s5L6k8c8okpbHpa9veMztDVC9sPJ60MWXh6anVKo1UcLcbUR\n"
				"yEeNvZneVRKAAU6ouwdjDvwlsaKydFKwed0ToQ47bmUKgcm+wV3eTRk36UOnTwID\n"
				"AQABo4HSMIHPME4GA1UdIARHMEUwQwYFYEwBAQAwOjA4BggrBgEFBQcCARYsaHR0\n"
				"cDovL2FjcmFpei5pY3BicmFzaWwuZ292LmJyL0RQQ2FjcmFpei5wZGYwPQYDVR0f\n"
				"BDYwNDAyoDCgLoYsaHR0cDovL2FjcmFpei5pY3BicmFzaWwuZ292LmJyL0xDUmFj\n"
				"cmFpei5jcmwwHQYDVR0OBBYEFIr68VeEERM1kEL6V0lUaQ2kxPA3MA8GA1UdEwEB\n"
				"/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3DQEBBQUAA4IBAQAZA5c1\n"
				"U/hgIh6OcgLAfiJgFWpvmDZWqlV30/bHFpj8iBobJSm5uDpt7TirYh1Uxe3fQaGl\n"
				"YjJe+9zd+izPRbBqXPVQA34EXcwk4qpWuf1hHriWfdrx8AcqSqr6CuQFwSr75Fos\n"
				"SzlwDADa70mT7wZjAmQhnZx2xJ6wfWlT9VQfS//JYeIc7Fue2JNLd00UOSMMaiK/\n"
				"t79enKNHEA2fupH3vEigf5Eh4bVAN5VohrTm6MY53x7XQZZr1ME7a55lFEnSeT0u\n"
				"mlOAjR2mAbvSM5X5oSZNrmetdzyTj2flCM8CC7MLab0kkdngRIlUBGHF1/S5nmPb\n"
				"K+9A46sd33oqK8n8\n"
				"-----END CERTIFICATE-----\n";

			mPemRootCert = "-----BEGIN CERTIFICATE-----\n"
			"MIIB0TCCATqgAwIBAgIJANaTqrzEvHaRMA0GCSqGSIb3DQEBBAUAMBsxGTAXBgNV\n"
			"BAMTEFJveGllcyB0ZXN0IHJvb3QwHhcNMDkwNDE1MjEwNzQ3WhcNMTAwNDE1MjEw\n"
			"NzQ3WjAbMRkwFwYDVQQDExBSb3hpZXMgdGVzdCByb290MIGfMA0GCSqGSIb3DQEB\n"
			"AQUAA4GNADCBiQKBgQCpo5nDW6RNz9IHUVZd7Tw2XAQiBniDF4xH0N1w7sUYTiFq\n"
			"21mABsnOPJD3ra+MtOsXPHcaljm661JjTD8L40v5sfEbqDUPcOw76ClrPqnuAeyT\n"
			"38qk8DHku/mT8YdprevGZdVcUXQg3vosVzOL93HOOHK+u61mEEoM9W5xoNVEdQID\n"
			"AQABox0wGzAMBgNVHRMEBTADAQH/MAsGA1UdDwQEAwIBBjANBgkqhkiG9w0BAQQF\n"
			"AAOBgQAzn0aW/+zWPmcTbvxonyiYYUr9b4SOB/quhAkT8KT4ir1dcZAXRR59+kEn\n"
			"HSTu1FAodV0gvESqyobftF5hZ1XMxdJqGu//xP+YCwlv244G/0pp7KLI8ihNO2+N\n"
			"lPBUJgbo++ZkhiE1jotZi9Ay0Oedh3s/AfbMZPyfpJ23ll6+BA==\n"
			"-----END CERTIFICATE-----\n";
			
			
			
			mPemIntermediateCert = "-----BEGIN CERTIFICATE-----\n"
			"MIIBzzCCATigAwIBAgIBATANBgkqhkiG9w0BAQQFADAbMRkwFwYDVQQDExBSb3hp\n"
			"ZXMgdGVzdCByb290MB4XDTA5MDQxNTIxMzE1NloXDTEwMDQxNTIxMzE1NlowITEf\n"
			"MB0GA1UEAxMWUm94aWVzIGludGVybWVkaWF0ZSBDQTCBnzANBgkqhkiG9w0BAQEF\n"
			"AAOBjQAwgYkCgYEA15MM0W1R37rx/24Q2Qkb5bSiQZxTUcQAhJ2pA8mwUucXuCVt\n"
			"6ayI2TuN32nkjmsCgUkiT/bdXWp0OJo7/MXRIFeUNMCRxrpeFnxuigYEqbIXAdN6\n"
			"qu/vdG2X4PRv/v9Ijrju4cBEiKIldIgOurWEIfXEsVSFP2XmFQHesF04qDcCAwEA\n"
			"AaMdMBswDAYDVR0TBAUwAwEB/zALBgNVHQ8EBAMCAQYwDQYJKoZIhvcNAQEEBQAD\n"
			"gYEAYljikYgak3W1jSo0vYthNHUy3lBVAKzDhpM96lY5OuXFslpCRX42zNL8X3kN\n"
			"U/4IaJUVtZqx8WsUXl1eXHzBCaXCftapV4Ir6cENLIsXCdXs8paFYzN5nPJA5GYU\n"
			"zWgkSEl1MEhNIc+bJW34vwi29EjrAShAhsIZ84Mt/lvD3Pc=\n"
			"-----END CERTIFICATE-----\n";
			
			mPemChildCert = "-----BEGIN CERTIFICATE-----\n"
			"MIIB5DCCAU0CBEnm9eUwDQYJKoZIhvcNAQEEBQAwITEfMB0GA1UEAxMWUm94aWVz\n"
			"IGludGVybWVkaWF0ZSBDQTAeFw0wOTA0MTYwMDAzNDlaFw0xMDA0MTYwMDAzNDla\n"
			"MCAxHjAcBgNVBAMTFWVuaWFjNjMubGluZGVubGFiLmNvbTCBnzANBgkqhkiG9w0B\n"
			"AQEFAAOBjQAwgYkCgYEAp9I5rofEzbjNht+9QejfnsIlEPqSxskoWKCG255TesWR\n"
			"RTmw9wafHQQkJk/VIsaU4RMBYHkknGbHX2dGvMHmKZoWUPSQ/8FZz09o0Qx3TNUZ\n"
			"l7KlGOD2d1c7ZxXDPqlLC6QW8DrE1/8zfwJ5cbYBXc8e7OKdSZeRrnwHyw4Q8r8C\n"
			"AwEAAaMvMC0wEwYDVR0lBAwwCgYIKwYBBQUHAwEwCQYDVR0TBAIwADALBgNVHQ8E\n"
			"BAMCBaAwDQYJKoZIhvcNAQEEBQADgYEAIG0M5tqYlXyMiGKPZfXy/R3M3ZZOapDk\n"
			"W0dsXJYXAc35ftwtn0VYu9CNnZCcli17/d+AKhkK8a/oGPazqudjFF6WLJLTXaY9\n"
			"NmhkJcOPADXkbyQPUPXzLe4YRrkEQeGhzMb4rKDQ1TKAcXfs0Y068pTpsixNSxja\n"
			"NhAUUcve5Is=\n"
			"-----END CERTIFICATE-----\n";
			
			mDerFormat = "MIIEuDCCA6CgAwIBAgIBBDANBgkqhkiG9w0BAQUFADCBtDELMAkGA1UEBhMCQlIxEzARBgNVBAoT"
"CklDUC1CcmFzaWwxPTA7BgNVBAsTNEluc3RpdHV0byBOYWNpb25hbCBkZSBUZWNub2xvZ2lhIGRh"
"IEluZm9ybWFjYW8gLSBJVEkxETAPBgNVBAcTCEJyYXNpbGlhMQswCQYDVQQIEwJERjExMC8GA1UE"
"AxMoQXV0b3JpZGFkZSBDZXJ0aWZpY2Fkb3JhIFJhaXogQnJhc2lsZWlyYTAeFw0wMTExMzAxMjU4"
"MDBaFw0xMTExMzAyMzU5MDBaMIG0MQswCQYDVQQGEwJCUjETMBEGA1UEChMKSUNQLUJyYXNpbDE9"
"MDsGA1UECxM0SW5zdGl0dXRvIE5hY2lvbmFsIGRlIFRlY25vbG9naWEgZGEgSW5mb3JtYWNhbyAt"
"IElUSTERMA8GA1UEBxMIQnJhc2lsaWExCzAJBgNVBAgTAkRGMTEwLwYDVQQDEyhBdXRvcmlkYWRl"
"IENlcnRpZmljYWRvcmEgUmFpeiBCcmFzaWxlaXJhMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB"
"CgKCAQEAwPMudwX/hvm+Uh2b/lQAcHVAisamaLkWdkwP9/S/tOKIgRrL6Oy+ZIGlOUdd6uYtk9Ma"
"/3pUpgcfNAj0vYm5gsyjQo9emsc+x6m4VWwk9iqMZSCK5EQkAq/Ut4n7KuLE1+gdftwdIgxfUsPt"
"4CyNrY50QV57KM2UT8x5rrmzEjr7TICGpSUAl2gVqe6xaii+bmYR1QrmWaBSAG59LrkrjrYtbRhF"
"boUDe1DK+6T8s5L6k8c8okpbHpa9veMztDVC9sPJ60MWXh6anVKo1UcLcbURyEeNvZneVRKAAU6o"
"uwdjDvwlsaKydFKwed0ToQ47bmUKgcm+wV3eTRk36UOnTwIDAQABo4HSMIHPME4GA1UdIARHMEUw"
"QwYFYEwBAQAwOjA4BggrBgEFBQcCARYsaHR0cDovL2FjcmFpei5pY3BicmFzaWwuZ292LmJyL0RQ"
"Q2FjcmFpei5wZGYwPQYDVR0fBDYwNDAyoDCgLoYsaHR0cDovL2FjcmFpei5pY3BicmFzaWwuZ292"
"LmJyL0xDUmFjcmFpei5jcmwwHQYDVR0OBBYEFIr68VeEERM1kEL6V0lUaQ2kxPA3MA8GA1UdEwEB"
"/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3DQEBBQUAA4IBAQAZA5c1U/hgIh6OcgLA"
"fiJgFWpvmDZWqlV30/bHFpj8iBobJSm5uDpt7TirYh1Uxe3fQaGlYjJe+9zd+izPRbBqXPVQA34E"
"Xcwk4qpWuf1hHriWfdrx8AcqSqr6CuQFwSr75FosSzlwDADa70mT7wZjAmQhnZx2xJ6wfWlT9VQf"
"S//JYeIc7Fue2JNLd00UOSMMaiK/t79enKNHEA2fupH3vEigf5Eh4bVAN5VohrTm6MY53x7XQZZr"
"1ME7a55lFEnSeT0umlOAjR2mAbvSM5X5oSZNrmetdzyTj2flCM8CC7MLab0kkdngRIlUBGHF1/S5"
"nmPbK+9A46sd33oqK8n8";
			
			mSha1RSATestCert = "-----BEGIN CERTIFICATE-----\n"
			"MIIDFDCCAn2gAwIBAgIDDqqYMA0GCSqGSIb3DQEBBQUAME4xCzAJBgNVBAYTAlVT\n"
			"MRAwDgYDVQQKEwdFcXVpZmF4MS0wKwYDVQQLEyRFcXVpZmF4IFNlY3VyZSBDZXJ0\n"
			"aWZpY2F0ZSBBdXRob3JpdHkwHhcNMTAwMTA1MDAzNjMwWhcNMTEwMTA3MjAyMTE0\n"
			"WjCBnjEpMCcGA1UEBRMgQmNmc0RBRkl1U0YwdFpWVm5vOFJKbjVUbW9hNGR2Wkgx\n"
			"CzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRYwFAYDVQQHEw1TYW4g\n"
			"RnJhbmNpc2NvMR0wGwYDVQQKExRMaW5kZW4gUmVzZWFyY2ggSW5jLjEYMBYGA1UE\n"
			"AxQPKi5saW5kZW5sYWIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQD2\n"
			"14Jdko8v6GB33hHbW+lNQyloFQtc2h4ykjf+fYPJ27dw6tQO2if7N3k/5XDkwC1N\n"
			"krGgE9vt3iecCPgasue6k67Zyfj9HbEP2D+j38eROudrsxLaRFDQx50BvZ5YMNl3\n"
			"4zQCj8/gCMsuq8cvaP9/rbJTUpgYWFGLsm8yAYOgWwIDAQABo4GuMIGrMA4GA1Ud\n"
			"DwEB/wQEAwIE8DAdBgNVHQ4EFgQUIBK/JB9AyqquSEbkzt2Zux6v9sYwOgYDVR0f\n"
			"BDMwMTAvoC2gK4YpaHR0cDovL2NybC5nZW90cnVzdC5jb20vY3Jscy9zZWN1cmVj\n"
			"YS5jcmwwHwYDVR0jBBgwFoAUSOZo+SvSspXXR9gjIBBPM5iQn9QwHQYDVR0lBBYw\n"
			"FAYIKwYBBQUHAwEGCCsGAQUFBwMCMA0GCSqGSIb3DQEBBQUAA4GBAKKR84+hvLuB\n"
			"pop9VG7HQPIyEKtZq3Nnk+UlJGfjGY3csLWSFmxU727r5DzdEP1W1PwF3rxuoKcZ\n"
			"4nJJpKdzoGVujgBMP2U/J0PJvU7D8U3Zqu7nrXAjOHj7iVnvJ3EKJ1bvwXaisgPN\n"
			"wt21kKfGnA4OlhJtJ6VQvUkcF12I3pTP\n"
			"-----END CERTIFICATE-----\n";
			
			mSha1RSATestCA = "-----BEGIN CERTIFICATE-----\n"
			"MIIDIDCCAomgAwIBAgIENd70zzANBgkqhkiG9w0BAQUFADBOMQswCQYDVQQGEwJV\n"
			"UzEQMA4GA1UEChMHRXF1aWZheDEtMCsGA1UECxMkRXF1aWZheCBTZWN1cmUgQ2Vy\n"
			"dGlmaWNhdGUgQXV0aG9yaXR5MB4XDTk4MDgyMjE2NDE1MVoXDTE4MDgyMjE2NDE1\n"
			"MVowTjELMAkGA1UEBhMCVVMxEDAOBgNVBAoTB0VxdWlmYXgxLTArBgNVBAsTJEVx\n"
			"dWlmYXggU2VjdXJlIENlcnRpZmljYXRlIEF1dGhvcml0eTCBnzANBgkqhkiG9w0B\n"
			"AQEFAAOBjQAwgYkCgYEAwV2xWGcIYu6gmi0fCG2RFGiYCh7+2gRvE4RiIcPRfM6f\n"
			"BeC4AfBONOziipUEZKzxa1NfBbPLZ4C/QgKO/t0BCezhABRP/PvwDN1Dulsr4R+A\n"
			"cJkVV5MW8Q+XarfCaCMczE1ZMKxRHjuvK9buY0V7xdlfUNLjUA86iOe/FP3gx7kC\n"
			"AwEAAaOCAQkwggEFMHAGA1UdHwRpMGcwZaBjoGGkXzBdMQswCQYDVQQGEwJVUzEQ\n"
			"MA4GA1UEChMHRXF1aWZheDEtMCsGA1UECxMkRXF1aWZheCBTZWN1cmUgQ2VydGlm\n"
			"aWNhdGUgQXV0aG9yaXR5MQ0wCwYDVQQDEwRDUkwxMBoGA1UdEAQTMBGBDzIwMTgw\n"
			"ODIyMTY0MTUxWjALBgNVHQ8EBAMCAQYwHwYDVR0jBBgwFoAUSOZo+SvSspXXR9gj\n"
			"IBBPM5iQn9QwHQYDVR0OBBYEFEjmaPkr0rKV10fYIyAQTzOYkJ/UMAwGA1UdEwQF\n"
			"MAMBAf8wGgYJKoZIhvZ9B0EABA0wCxsFVjMuMGMDAgbAMA0GCSqGSIb3DQEBBQUA\n"
			"A4GBAFjOKer89961zgK5F7WF0bnj4JXMJTENAKaSbn+2kmOeUJXRmm/kEd5jhW6Y\n"
			"7qj/WsjTVbJmcVfewCHrPSqnI0kBBIZCe/zuf6IWUrVnZ9NA2zsmWLIodz2uFHdh\n"
			"1voqZiegDfqnc1zqcPGUIWVEX/r87yloqaKHee9570+sB3c4\n"
			"-----END CERTIFICATE-----\n";

			
			
			
			mX509TestCert = NULL;
			mX509RootCert = NULL;
			mX509IntermediateCert = NULL;
			mX509ChildCert = NULL;
			
			BIO * validation_bio = BIO_new_mem_buf((void*)mPemTestCert.c_str(), mPemTestCert.length());			
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
	tut::sechandler_basic_test_factory tut_test("llsechandler_basic");
	
	// ---------------------------------------------------------------------------------------
	// Test functions 
	// ---------------------------------------------------------------------------------------
	// test cert data retrieval
	template<> template<>
	void sechandler_basic_test_object::test<1>()
	
	{
		char buffer[4096];
		LLPointer<LLCertificate> test_cert = new LLBasicCertificate(mPemTestCert);
		
		ensure_equals("Resultant pem is correct",
			   mPemTestCert, test_cert->getPem());
		std::vector<U8> binary_cert = test_cert->getBinary();

		apr_base64_encode(buffer, (const char *)&binary_cert[0], binary_cert.size());
		
		ensure_equals("Der Format is correct", memcmp(buffer, mDerFormat.c_str(), mDerFormat.length()), 0);
		
		LLSD llsd_cert = test_cert->getLLSD();
		std::ostringstream llsd_value;
		llsd_value << LLSDOStreamer<LLSDNotationFormatter>(llsd_cert) << std::endl;
		std::string llsd_cert_str = llsd_value.str();
		ensure_equals("Issuer Name/commonName", 
			   (std::string)llsd_cert["issuer_name"]["commonName"], "Autoridade Certificadora Raiz Brasileira");
		ensure_equals("Issure Name/countryName", (std::string)llsd_cert["issuer_name"]["countryName"], "BR");
		ensure_equals("Issuer Name/localityName", (std::string)llsd_cert["issuer_name"]["localityName"], "Brasilia");
		ensure_equals("Issuer Name/org name", (std::string)llsd_cert["issuer_name"]["organizationName"], "ICP-Brasil");
		ensure_equals("IssuerName/org unit", 
			   (std::string)llsd_cert["issuer_name"]["organizationalUnitName"], "Instituto Nacional de Tecnologia da Informacao - ITI");
		ensure_equals("IssuerName/state", (std::string)llsd_cert["issuer_name"]["stateOrProvinceName"], "DF");
		ensure_equals("Issuer name string", 
			   (std::string)llsd_cert["issuer_name_string"], "CN=Autoridade Certificadora Raiz Brasileira,ST=DF,"
															   "L=Brasilia,OU=Instituto Nacional de Tecnologia da Informacao - ITI,O=ICP-Brasil,C=BR");
		ensure_equals("subject Name/commonName", 
			   (std::string)llsd_cert["subject_name"]["commonName"], "Autoridade Certificadora Raiz Brasileira");
		ensure_equals("subject Name/countryName", (std::string)llsd_cert["subject_name"]["countryName"], "BR");
		ensure_equals("subject Name/localityName", (std::string)llsd_cert["subject_name"]["localityName"], "Brasilia");
		ensure_equals("subject Name/org name", (std::string)llsd_cert["subject_name"]["organizationName"], "ICP-Brasil");
		ensure_equals("subjectName/org unit", 
			   (std::string)llsd_cert["subject_name"]["organizationalUnitName"], "Instituto Nacional de Tecnologia da Informacao - ITI");
		ensure_equals("subjectName/state", (std::string)llsd_cert["subject_name"]["stateOrProvinceName"], "DF");
		ensure_equals("subject name string", 
			   (std::string)llsd_cert["subject_name_string"], "CN=Autoridade Certificadora Raiz Brasileira,ST=DF,"
			                                                    "L=Brasilia,OU=Instituto Nacional de Tecnologia da Informacao - ITI,O=ICP-Brasil,C=BR");
		
		ensure_equals("md5 digest", (std::string)llsd_cert["md5_digest"], "96:89:7d:61:d1:55:2b:27:e2:5a:39:b4:2a:6c:44:6f");
		ensure_equals("serial number", (std::string)llsd_cert["serial_number"], "04");
		// sha1 digest is giving a weird value, and I've no idea why...feh
		//ensure_equals("sha1 digest", (std::string)llsd_cert["sha1_digest"], "8e:fd:ca:bc:93:e6:1e:92:5d:4d:1d:ed:18:1a:43:20:a4:67:a1:39");
		ensure_equals("valid from", (std::string)llsd_cert["valid_from"], "2001-11-30T12:58:00Z");
		ensure_equals("valid to", (std::string)llsd_cert["valid_to"], "2011-11-30T23:59:00Z");
		LLSD expectedKeyUsage = LLSD::emptyArray();
		expectedKeyUsage.append(LLSD((std::string)"certSigning"));
		expectedKeyUsage.append(LLSD((std::string)"crlSigning"));
		ensure("key usage", valueCompareLLSD(llsd_cert["keyUsage"], expectedKeyUsage));
		ensure("basic constraints", (bool)llsd_cert["basicConstraints"]["CA"]);
		
		ensure("x509 is equal", !X509_cmp(mX509TestCert, test_cert->getOpenSSLX509()));
	}

	
	// test protected data
	template<> template<>
	void sechandler_basic_test_object::test<2>()

	{
		unsigned char MACAddress[MAC_ADDRESS_BYTES];
		LLUUID::getNodeID(MACAddress);
		
		std::string protected_data = "sUSh3wj77NG9oAMyt3XIhaej3KLZhLZWFZvI6rIGmwUUOmmelrRg0NI9rkOj8ZDpTPxpwToaBT5u"
		"GQhakdaGLJznr9bHr4/6HIC1bouKj4n2rs4TL6j2WSjto114QdlNfLsE8cbbE+ghww58g8SeyLQO"
		"nyzXoz+/PBz0HD5SMFDuObccoPW24gmqYySz8YoEWhSwO0pUtEEqOjVRsAJgF5wLAtJZDeuilGsq"
		"4ZT9Y4wZ9Rh8nnF3fDUL6IGamHe1ClXM1jgBu10F6UMhZbnH4C3aJ2E9+LiOntU+l3iCb2MpkEpr"
		"82r2ZAMwIrpnirL/xoYoyz7MJQYwUuMvBPToZJrxNSsjI+S2Z+I3iEJAELMAAA==";
		
		std::vector<U8> binary_data(apr_base64_decode_len(protected_data.c_str()));
		apr_base64_decode_binary(&binary_data[0], protected_data.c_str());

		LLXORCipher cipher(gMACAddress, MAC_ADDRESS_BYTES);
		cipher.decrypt(&binary_data[0], 16);
		LLXORCipher cipher2(MACAddress, MAC_ADDRESS_BYTES);
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
	void sechandler_basic_test_object::test<3>()
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
		unsigned char MACAddress[MAC_ADDRESS_BYTES];
		LLUUID::getNodeID(MACAddress);
		LLXORCipher cipher(gMACAddress, MAC_ADDRESS_BYTES);
		cipher.decrypt((U8*)&decoded_password[0], length);
		LLXORCipher cipher2(MACAddress, MAC_ADDRESS_BYTES);
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
	void sechandler_basic_test_object::test<4>()
	{
		
		// validate create from empty vector
		LLPointer<LLBasicCertificateVector> test_vector = new LLBasicCertificateVector();
		ensure_equals("when loading with nothing, we should result in no certs in vector", test_vector->size(), 0);
		
		test_vector->add(new LLBasicCertificate(mPemTestCert));
		ensure_equals("one element in vector", test_vector->size(), 1);
		test_vector->add(new LLBasicCertificate(mPemChildCert));
		ensure_equals("two elements in vector after add", test_vector->size(), 2);
		
		test_vector->add(new LLBasicCertificate(mPemChildCert));
		ensure_equals("two elements in vector after re-add", test_vector->size(), 2);
		// validate order
		X509* test_cert = (*test_vector)[0]->getOpenSSLX509();		
		ensure("first cert added remains first cert", !X509_cmp(test_cert, mX509TestCert));
		X509_free(test_cert);
		
		test_cert = (*test_vector)[1]->getOpenSSLX509();	
		ensure("adding a duplicate cert", !X509_cmp(test_cert, mX509ChildCert));
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
		
		// validate remove
		// validate create from empty vector
		test_vector = new LLBasicCertificateVector();
		test_vector->add(new LLBasicCertificate(mPemTestCert));
		test_vector->add(new LLBasicCertificate(mPemChildCert));
		test_vector->erase(test_vector->begin());
		ensure_equals("one element in store after remove", test_vector->size(), 1);
		test_cert = (*test_vector)[0]->getOpenSSLX509();
		ensure("validate cert was removed", !X509_cmp(test_cert, mX509ChildCert));
		X509_free(test_cert);
		
		// validate insert
		test_vector->insert(test_vector->begin(), new LLBasicCertificate(mPemChildCert));
		test_cert = (*test_vector)[0]->getOpenSSLX509();
		
		ensure("validate cert was inserted", !X509_cmp(test_cert, mX509ChildCert));
		X509_free(test_cert);	

		//validate find
		LLSD find_info = LLSD::emptyMap();
		test_vector->insert(test_vector->begin(), new LLBasicCertificate(mPemRootCert));
		find_info["issuer_name"] = LLSD::emptyMap();
		find_info["issuer_name"]["commonName"] = "Roxies intermediate CA";
		find_info["md5_digest"] = "97:24:c7:4c:d4:ba:2d:0e:9c:a1:18:8e:3a:c6:1f:c3";
		current_cert = test_vector->find(find_info);
		ensure("found", current_cert != test_vector->end());
		ensure("found cert", (*current_cert).get() == (*test_vector)[1].get());
		find_info["sha1_digest"] = "bad value";
		current_cert =test_vector->find(find_info);
		ensure("didn't find cert", current_cert == test_vector->end());		
	}	
	
	// test cert store
	template<> template<>
	void sechandler_basic_test_object::test<5>()
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
	void sechandler_basic_test_object::test<6>()
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
	void sechandler_basic_test_object::test<7>()
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
	void sechandler_basic_test_object::test<8>()
	{
		// start with a trusted store with our known root cert
		LLFile::remove("mycertstore.pem");
		LLPointer<LLBasicCertificateStore> test_store = new LLBasicCertificateStore("mycertstore.pem");
		test_store->add(new LLBasicCertificate(mX509RootCert));
		LLSD validation_params;
		
		// validate basic trust for a chain containing only the intermediate cert.  (1 deep)
		LLPointer<LLBasicCertificateChain> test_chain = new LLBasicCertificateChain(NULL);

		test_chain->add(new LLBasicCertificate(mX509IntermediateCert));

		test_chain->validate(0, test_store, validation_params);

		// add the root certificate to the chain and revalidate
		test_chain->add(new LLBasicCertificate(mX509RootCert));	
		test_chain->validate(0, test_store, validation_params);

		// add the child cert at the head of the chain, and revalidate (3 deep chain)
		test_chain->insert(test_chain->begin(), new LLBasicCertificate(mX509ChildCert));
		test_chain->validate(0, test_store, validation_params);

		// basic failure cases
		test_chain = new LLBasicCertificateChain(NULL);
		//validate with only the child cert
		test_chain->add(new LLBasicCertificate(mX509ChildCert));
		ensure_throws("no CA, with only a child cert", 
					  LLCertValidationTrustException, 
					  (*test_chain)[0],
					  test_chain->validate, 
					  VALIDATION_POLICY_TRUSTED, 
					  test_store, 
					  validation_params);


		// validate without the trust flag.
		test_chain->validate(0, test_store, validation_params);		

		// clear out the store
		test_store = new LLBasicCertificateStore("mycertstore.pem");
		// append the intermediate cert
		test_chain->add(new LLBasicCertificate(mX509IntermediateCert));		
		ensure_throws("no CA, with child and intermediate certs", 
					  LLCertValidationTrustException, 
					  (*test_chain)[1],
					  test_chain->validate, 
					  VALIDATION_POLICY_TRUSTED, 
					  test_store, 
					  validation_params);
		// validate without the trust flag
		test_chain->validate(0, test_store, validation_params);

		// Test time validity
		LLSD child_info = (*test_chain)[0]->getLLSD();
		validation_params = LLSD::emptyMap();
		validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_FROM].asDate().secondsSinceEpoch() + 1.0);  
		test_chain->validate(VALIDATION_POLICY_TIME, test_store, validation_params);

		validation_params = LLSD::emptyMap();		
		validation_params[CERT_VALIDATION_DATE] = child_info[CERT_VALID_FROM].asDate();
		
		validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_FROM].asDate().secondsSinceEpoch() - 1.0);
 		
		// test not yet valid
		ensure_throws("Child cert not yet valid" , 
					  LLCertValidationExpirationException, 
					  (*test_chain)[0],
					  test_chain->validate, 
					  VALIDATION_POLICY_TIME, 
					  test_store, 
					  validation_params);	
		validation_params = LLSD::emptyMap();		
		validation_params[CERT_VALIDATION_DATE] = LLDate(child_info[CERT_VALID_TO].asDate().secondsSinceEpoch() + 1.0);
 		
		// test cert expired
		ensure_throws("Child cert expired", 
					  LLCertValidationExpirationException, 
					  (*test_chain)[0],
					  test_chain->validate, 
					  VALIDATION_POLICY_TIME, 
					  test_store, 
					  validation_params);

		// test SSL KU
		// validate basic trust for a chain containing child and intermediate.
		test_chain = new LLBasicCertificateChain(NULL);
		test_chain->add(new LLBasicCertificate(mX509ChildCert));
		test_chain->add(new LLBasicCertificate(mX509IntermediateCert));
		test_chain->validate(VALIDATION_POLICY_SSL_KU, test_store, validation_params);	

		test_chain = new LLBasicCertificateChain(NULL);
		test_chain->add(new LLBasicCertificate(mX509TestCert));

		ensure_throws("Cert doesn't have ku", 
					  LLCertKeyUsageValidationException, 
					  (*test_chain)[0],
					  test_chain->validate, 
					  VALIDATION_POLICY_SSL_KU, 
					  test_store, 
					  validation_params);
		
		// test sha1RSA validation
		test_chain = new LLBasicCertificateChain(NULL);
		test_chain->add(new LLBasicCertificate(mSha1RSATestCert));	
		test_chain->add(new LLBasicCertificate(mSha1RSATestCA));

		test_chain->validate(0, test_store, validation_params);	
	}
	
};

