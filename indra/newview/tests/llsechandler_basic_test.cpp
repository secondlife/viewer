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
#include "../test/lldoctest.h"
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
TEST_SUITE("LLSecHandler") {

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
        
};

TEST_CASE_FIXTURE(sechandler_basic_test, "test_1")
{

        try
        {
            LLPointer<LLBasicCertificate> test_cert(new LLBasicCertificate(mPemTestCert, &mValidationDate));
            LL_INFOS() << "ok" << LL_ENDL;
        
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_2")
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
        CHECK_MESSAGE(valueCompareLLSD(llsd_cert["keyUsage"], expectedKeyUsage, "key usage"));
        ensure_equals("basic constraints", llsd_cert["basicConstraints"]["CA"].asInteger(), 0);

        CHECK_MESSAGE(!X509_cmp(mX509ChildCert, test_cert->getOpenSSLX509(, "x509 is equal")));
    
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_3")
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
        CHECK_MESSAGE(data.isUndefined(, "not found"));

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
        CHECK_MESSAGE(data.isUndefined(, "not found"));

        handler->deleteProtectedData("test_data_type", "test_data_id");
        CHECK_MESSAGE(handler->getProtectedData("test_data_type", "test_data_id", "Deleted data not found").isUndefined());

        LLFile::remove("sechandler_settings.tmp");
        handler = new LLSecAPIBasicHandler("sechandler_settings.tmp", "test_password.dat");
        handler->init();
        data = handler->getProtectedData("test_data_type1", "test_data_id");
        CHECK_MESSAGE(data.isUndefined(, "not found"));
        handler = NULL;

        ensure(LLFile::isfile("sechandler_settings.tmp"));
    
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_4")
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
        CHECK_MESSAGE(my_id == my_cred->getIdentifier(, "basic credential creation: identifier"));
        CHECK_MESSAGE(my_authenticator == my_cred->getAuthenticator(, "basic credential creation: authenticator"));
        CHECK_MESSAGE("my_grid" == my_cred->getGrid(, "basic credential creation: grid"));

        // test setting/overwriting of credential components
        my_id["first_name"] = "firstname";
        my_id.erase("username");
        my_authenticator.erase("creds");
        my_authenticator["hash"] = "6563245";

        my_cred->setCredentialData(my_id, my_authenticator);
        CHECK_MESSAGE(my_id == my_cred->getIdentifier(, "set credential data: identifier"));
        CHECK_MESSAGE(my_authenticator == my_cred->getAuthenticator(, "set credential data: authenticator"));
        CHECK_MESSAGE("my_grid" == my_cred->getGrid(, "set credential data: grid"));

        // test loading of a credential, that hasn't been saved, without
        // any legacy saved credential data
        LLPointer<LLCredential> my_new_cred = handler->loadCredential("my_grid2");
        CHECK_MESSAGE(my_new_cred->getIdentifier(, "unknown credential load test").isMap());
        CHECK_MESSAGE(!my_new_cred->getIdentifier(, "unknown credential load test").has("type"));
        CHECK_MESSAGE(my_new_cred->getAuthenticator(, "unknown credential load test").isMap());
        CHECK_MESSAGE(!my_new_cred->getAuthenticator(, "unknown credential load test").has("type"));
        // test saving of a credential
        handler->saveCredential(my_cred, true);

        // test loading of a known credential
        my_new_cred = handler->loadCredential("my_grid");
        CHECK_MESSAGE(my_id == my_new_cred->getIdentifier(, "load a known credential: identifier"));
        CHECK_MESSAGE(my_authenticator == my_new_cred->getAuthenticator(, "load a known credential: authenticator"));
        CHECK_MESSAGE("my_grid" == my_cred->getGrid(, "load a known credential: grid"));

        // test deletion of a credential
        handler->deleteCredential(my_new_cred);

        CHECK_MESSAGE(my_new_cred->getIdentifier(, "delete credential: identifier").isUndefined());
        CHECK_MESSAGE(my_new_cred->getIdentifier(, "delete credentialt: authenticator").isUndefined());
        CHECK_MESSAGE("my_grid" == my_cred->getGrid(, "delete credential: grid"));
        // load unknown cred

        my_new_cred = handler->loadCredential("my_grid");
        CHECK_MESSAGE(my_new_cred->getIdentifier(, "deleted credential load test").isMap());
        CHECK_MESSAGE(!my_new_cred->getIdentifier(, "deleted credential load test").has("type"));
        CHECK_MESSAGE(my_new_cred->getAuthenticator(, "deleted credential load test").isMap());
        CHECK_MESSAGE(!my_new_cred->getAuthenticator(, "deleted credential load test").has("type"));

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

        CHECK_MESSAGE(my_new_cred->getAuthenticator(, "legacy credential with no password: no authenticator").isUndefined());

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
        CHECK_MESSAGE(my_new_cred->getAuthenticator(, "no authenticator values were saved").isUndefined());
    
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_5")
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
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509TestCert, "first cert added remains first cert"));
        X509_free(test_cert);

        test_cert = (*test_vector)[1]->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509ChildCert, "second cert is second cert"));
        X509_free(test_cert);

        //
        // validate iterator
        //
        LLBasicCertificateVector::iterator current_cert = test_vector->begin();
        LLBasicCertificateVector::iterator copy_current_cert = current_cert;
        // operator++(int)
        CHECK_MESSAGE(*current_cert++ == (*test_vector, "validate iterator++ element in vector is expected cert")[0]);
        CHECK_MESSAGE(*current_cert++ == (*test_vector, "validate 2nd iterator++ element in vector is expected cert")[1]);
        CHECK_MESSAGE(current_cert == test_vector->end(, "validate end iterator++"));

        // copy
        CHECK_MESSAGE(*copy_current_cert == (*test_vector, "validate copy iterator element in vector is expected cert")[0]);

        // operator--(int)
        current_cert--;
        CHECK_MESSAGE(*current_cert-- == (*test_vector, "validate iterator-- element in vector is expected cert")[1]);
        CHECK_MESSAGE(*current_cert == (*test_vector, "validate iterator-- element in vector is expected cert")[0]);

        CHECK_MESSAGE(current_cert == test_vector->begin(, "begin iterator is equal"));

        // operator++
        CHECK_MESSAGE(*++current_cert == (*test_vector, "validate ++iterator element in vector is expected cert")[1]);
        CHECK_MESSAGE(++current_cert == test_vector->end(, "end of cert vector after ++iterator"));
        // operator--
        CHECK_MESSAGE(*--current_cert == (*test_vector, "validate --iterator element in vector is expected cert")[1]);
        CHECK_MESSAGE(*--current_cert == (*test_vector, "validate 2nd --iterator element in vector is expected cert")[0]);

        test_vector->erase(test_vector->begin());
        ensure_equals("one element in store after remove", test_vector->size(), 1);
        test_cert = (*test_vector)[0]->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509ChildCert, "Child cert remains"));
        X509_free(test_cert);

        // validate insert
        test_vector->insert(test_vector->begin(), new LLBasicCertificate(mPemIntermediateCert, &mValidationDate));
        test_cert = (*test_vector)[0]->getOpenSSLX509();
        ensure_equals("two elements in store after insert", test_vector->size(), 2);
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509IntermediateCert, "validate intermediate cert was inserted at first position"));
        X509_free(test_cert);
        test_cert = (*test_vector)[1]->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509ChildCert, "validate child cert still there"));
        X509_free(test_cert);

        //validate find
        LLSD find_info = LLSD::emptyMap();
        find_info["subjectKeyIdentifier"] = "7b:1a:f9:2b:c4:b2:f6:ae:d6:f2:8e:b1:73:fb:dd:11:ca:db:f8:87";
        LLBasicCertificateVector::iterator found_cert = test_vector->find(find_info);
        CHECK_MESSAGE(found_cert != test_vector->end(, "found some cert"));
        X509* found_x509 = (*found_cert).get()->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(found_x509, mX509ChildCert, "child cert was found"));
        X509_free(found_x509);

        find_info["subjectKeyIdentifier"] = "00:00:00:00"; // bogus
        current_cert =test_vector->find(find_info);
        CHECK_MESSAGE(current_cert == test_vector->end(, "didn't find cert"));
    
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_6")
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

        CHECK_MESSAGE(!X509_cmp(test_cert, mX509ChildCert, "validate first element in store is expected cert"));
        X509_free(test_cert);
        test_cert = (*test_store)[1]->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509TestCert, "validate second element in store is expected cert"));
        X509_free(test_cert);


        // validate save
        LLFile::remove("mycertstore.pem");
        test_store->save();
        test_store = NULL;
        test_store = new LLBasicCertificateStore("mycertstore.pem");
        ensure_equals("two elements in store after save", test_store->size(), 2);
        LLCertificateStore::iterator current_cert = test_store->begin();
        test_cert = (*current_cert)->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509ChildCert, "validate first element in store is expected cert"));
        current_cert++;
        X509_free(test_cert);
        test_cert = (*current_cert)->getOpenSSLX509();
        CHECK_MESSAGE(!X509_cmp(test_cert, mX509TestCert, "validate second element in store is expected cert"));
        X509_free(test_cert);
        current_cert++;
        CHECK_MESSAGE(current_cert == test_store->end(, "end of cert store"));

    
}

TEST_CASE_FIXTURE(sechandler_basic_test, "test_7")
{

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo", "foo", "simple name match"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.", "foo.", "simple name match, with end period"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match(".foo", ".foo", "simple name match, with begin period"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.", "foo", "simple name match, with mismatched period cn"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo", "foo.", "simple name match, with mismatched period hostname"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar", "foo.bar", "simple name match, with subdomain"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobbbbfoo", "foo*bbbfoo", "stutter name match"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "*bar", "simple name match, with beginning wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "foo*", "simple name match, with ending wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "*foobar", "simple name match, with beginning null wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "foobar*", "simple name match, with ending null wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "f*r", "simple name match, with embedded wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "foo*bar", "simple name match, with embedded null wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "f*o*ar", "simple name match, with dual embedded wildcard"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("bar", "foo", "simple name mismatch"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("foobar.", "foo.", "simple name mismatch, with end period"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match(".foobar", ".foo", "simple name mismatch, with begin period"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("foobar.bar", "foo.bar", "simple name mismatch, with subdomain"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("foobara", "*bar", "simple name mismatch, with beginning wildcard"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("oobar", "foo*", "simple name mismatch, with ending wildcard"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("oobar", "f*r", "simple name mismatch, with embedded wildcard"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("foobar", "f*d*ar", "simple name mismatch, with dual embedded wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foobar", "*", "simple wildcard"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com", "foo.bar.com", "long domain"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com", "*.b*r.com", "long domain with multiple wildcards"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com.", "*.b*r.com.", "end periods"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com.", "*.b*r.com", "match end period"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com", "*.b*r.com.", "match end period2"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("bar.com", "*.bar.com", "wildcard mismatch"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.bar.com", "*.bar.com", "wildcard match"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.foo.bar.com", "*.bar.com", "wildcard match"));

        CHECK_MESSAGE(_cert_hostname_wildcard_match("foo.foo.bar.com", "*.*.com", "wildcard match"));

        CHECK_MESSAGE(!_cert_hostname_wildcard_match("foo.foo.bar.com", "*.foo.com", "wildcard mismatch"));
    
}

} // TEST_SUITE
