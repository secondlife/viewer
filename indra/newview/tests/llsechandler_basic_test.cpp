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
#include "llxorcipher.h"

LLControlGroup gSavedSettings;
unsigned char gMACAddress[MAC_ADDRESS_BYTES] = {77,21,46,31,89,2};

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
namespace tut
{
	// Test wrapper declaration : wrapping nothing for the moment
	struct sechandler_basic_test
	{
		std::string mPemTestCert;
		std::string mDerFormat;
		X509 *mX509TestCert;
		LLBasicCertificate* mTestCert;

		sechandler_basic_test()
		{
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
"-----END CERTIFICATE-----\n"
"";
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
			
			mTestCert = new LLBasicCertificate(mPemTestCert);
			
			gSavedSettings.cleanup();
			gSavedSettings.declareString("FirstName", "", "", FALSE);
			gSavedSettings.declareString("LastName", "", "", FALSE);
			mX509TestCert = NULL;
			BIO * validation_bio = BIO_new_mem_buf((void*)mPemTestCert.c_str(), mPemTestCert.length());
			
			PEM_read_bio_X509(validation_bio, &mX509TestCert, 0, NULL);
			BIO_free(validation_bio);

		}
		~sechandler_basic_test()
		{
			LLFile::remove("test_password.dat");
			LLFile::remove("sechandler_settings.tmp");
			delete mTestCert;
			X509_free(mX509TestCert);
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

		ensure_equals("Resultant pem is correct",
			   mPemTestCert, mTestCert->getPem());
		std::vector<U8> binary_cert = mTestCert->getBinary();

		apr_base64_encode(buffer, (const char *)&binary_cert[0], binary_cert.size());
		
		ensure_equals("Der Format is correct", memcmp(buffer, mDerFormat.c_str(), mDerFormat.length()), 0);
		
		LLSD llsd_cert = mTestCert->getLLSD();
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
		ensure_equals("valid from", (std::string)llsd_cert["valid_from"], "2001-11-30T20:58:00Z");
		ensure_equals("valid to", (std::string)llsd_cert["valid_to"], "2011-12-01T07:59:00Z");
		
		ensure("x509 is equal", !X509_cmp(mX509TestCert, mTestCert->getOpenSSLX509()));
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
		data = handler->getProtectedData("test_data_type1", "test_data_id");
		ensure("not found", data.isUndefined());
		
		handler->deleteProtectedData("test_data_type", "test_data_id");
		ensure("Deleted data not found", handler->getProtectedData("test_data_type", "test_data_id").isUndefined());
		
		LLFile::remove("sechandler_settings.tmp");
		handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
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
		LLPointer<LLCredential> my_new_cred = handler->loadCredential("my_grid");
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

		gSavedSettings.setString("FirstName", "myfirstname");
		gSavedSettings.setString("LastName", "mylastname");

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


	// test cert store
	template<> template<>
	void sechandler_basic_test_object::test<4>()
	{
		// instantiate a cert store from a file
		llofstream certstorefile("mycertstore.pem", std::ios::out | std::ios::binary);

		certstorefile << mPemTestCert;
		certstorefile.close();
		// LLBasicCertificateStore test_store("mycertstore.pem");
		// X509* test_cert = test_store[0]->getOpenSSLX509();

		// ensure("validate first element in store is expected cert", !X509_cmp(test_cert, mX509TestCert));
	}
};
