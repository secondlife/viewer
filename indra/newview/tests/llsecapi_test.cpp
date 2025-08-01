/**
 * @file llsecapi_test.cpp
 * @author Roxie
 * @date 2009-02-10
 * @brief Test the sec api functionality
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
#include "../llviewerprecompiledheaders.h"
#include "../llviewernetwork.h"
#include "../test/lldoctest.h"
#include "../llsecapi.h"
#include "../llsechandler_basic.h"
#include "../../llxml/llcontrol.h"


//----------------------------------------------------------------------------
// Mock objects for the dependencies of the code we're testing

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
    return "";
}


LLControlGroup gSavedSettings("test");

LLSecAPIBasicHandler::LLSecAPIBasicHandler() {}
void LLSecAPIBasicHandler::init() {}
LLSecAPIBasicHandler::~LLSecAPIBasicHandler() {}
LLPointer<LLCertificate> LLSecAPIBasicHandler::getCertificate(const std::string& pem_cert) { return NULL; }
LLPointer<LLCertificate> LLSecAPIBasicHandler::getCertificate(X509* openssl_cert) { return NULL; }
LLPointer<LLCertificateChain> LLSecAPIBasicHandler::getCertificateChain(X509_STORE_CTX* chain) { return NULL; }
LLPointer<LLCertificateStore> LLSecAPIBasicHandler::getCertificateStore(const std::string& store_id) { return NULL; }
void LLSecAPIBasicHandler::setProtectedData(const std::string& data_type, const std::string& data_id, const LLSD& data) {}
void LLSecAPIBasicHandler::addToProtectedMap(const std::string& data_type, const std::string& data_id, const std::string& map_elem, const LLSD& data) {}
void LLSecAPIBasicHandler::removeFromProtectedMap(const std::string& data_type, const std::string& data_id, const std::string& map_elem) {}
void LLSecAPIBasicHandler::syncProtectedMap() {}
LLSD LLSecAPIBasicHandler::getProtectedData(const std::string& data_type, const std::string& data_id) { return LLSD(); }
void LLSecAPIBasicHandler::deleteProtectedData(const std::string& data_type, const std::string& data_id) {}
LLPointer<LLCredential> LLSecAPIBasicHandler::createCredential(const std::string& grid, const LLSD& identifier, const LLSD& authenticator) { return NULL; }
LLPointer<LLCredential> LLSecAPIBasicHandler::loadCredential(const std::string& grid) { return NULL; }
void LLSecAPIBasicHandler::saveCredential(LLPointer<LLCredential> cred, bool save_authenticator) {}
void LLSecAPIBasicHandler::deleteCredential(LLPointer<LLCredential> cred) {}
bool LLSecAPIBasicHandler::hasCredentialMap(const std::string& storage, const std::string& grid) { return false; }
bool LLSecAPIBasicHandler::emptyCredentialMap(const std::string& storage, const std::string& grid) { return false; }
void LLSecAPIBasicHandler::loadCredentialMap(const std::string& storage, const std::string& grid, credential_map_t& credential_map) {}
LLPointer<LLCredential> LLSecAPIBasicHandler::loadFromCredentialMap(const std::string& storage, const std::string& grid, const std::string& userkey) { return NULL; }
void LLSecAPIBasicHandler::addToCredentialMap(const std::string& storage, LLPointer<LLCredential> cred, bool save_authenticator) {}
void LLSecAPIBasicHandler::removeFromCredentialMap(const std::string& storage, LLPointer<LLCredential> cred) {}
void LLSecAPIBasicHandler::removeFromCredentialMap(const std::string& storage, const std::string& grid, const std::string& userkey) {}
void LLSecAPIBasicHandler::removeCredentialMap(const std::string& storage, const std::string& grid) {}

// -------------------------------------------------------------------------------------------
// TUT
// -------------------------------------------------------------------------------------------
TEST_SUITE("LLSecAPI") {

struct secapiTest
{


        secapiTest()
        {
        
};

TEST_CASE_FIXTURE(secapiTest, "test_1")
{

        // retrieve an unknown handler

        CHECK_MESSAGE(getSecHandler("unknown", "'Unknown' handler should be NULL") == nullptr);
        LLPointer<LLSecAPIHandler> test1_handler =  new LLSecAPIBasicHandler();
        registerSecHandler("sectest1", test1_handler);
        CHECK_MESSAGE(getSecHandler("unknown", "'Unknown' handler should be NULL") == nullptr);
        LLPointer<LLSecAPIHandler> retrieved_test1_handler = getSecHandler("sectest1");
        CHECK_MESSAGE(retrieved_test1_handler == test1_handler, "Retrieved sectest1 handler should be the same");

        // insert a second handler
        LLPointer<LLSecAPIHandler> test2_handler =  new LLSecAPIBasicHandler();
        registerSecHandler("sectest2", test2_handler);
        CHECK_MESSAGE(getSecHandler("unknown", "'Unknown' handler should be NULL") == nullptr);
        retrieved_test1_handler = getSecHandler("sectest1");
        CHECK_MESSAGE(retrieved_test1_handler == test1_handler, "Retrieved sectest1 handler should be the same");

        LLPointer<LLSecAPIHandler> retrieved_test2_handler = getSecHandler("sectest2");
        CHECK_MESSAGE(retrieved_test2_handler == test2_handler, "Retrieved sectest1 handler should be the same");

    
}

} // TEST_SUITE

