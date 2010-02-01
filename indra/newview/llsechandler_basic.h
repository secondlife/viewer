/** 
 * @file llsechandler_basic.h
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LLSECHANDLER_BASIC
#define LLSECHANDLER_BASIC

#include "llsecapi.h"
#include <vector>
#include <openssl/x509.h>

// helpers
extern LLSD cert_name_from_X509_NAME(X509_NAME* name);
extern std::string cert_string_name_from_X509_NAME(X509_NAME* name);
extern std::string cert_string_from_asn1_integer(ASN1_INTEGER* value);
extern LLDate cert_date_from_asn1_time(ASN1_TIME* asn1_time);
extern std::string cert_get_digest(const std::string& digest_type, X509 *cert);


// class LLCertificate
// 
class LLBasicCertificate : public LLCertificate
{
public:		
	LOG_CLASS(LLBasicCertificate);

	LLBasicCertificate(const std::string& pem_cert);
	LLBasicCertificate(X509* openSSLX509);
	
	virtual ~LLBasicCertificate();
	
	virtual std::string getPem();
	virtual std::vector<U8> getBinary();
	virtual LLSD getLLSD();

	virtual X509* getOpenSSLX509();
protected:
	// certificates are stored as X509 objects, as validation and
	// other functionality is via openssl
	X509* mCert;
};

// class LLCertificateStore
// represents a store of certificates, typically a store of root CA
// certificates.  The store can be persisted, and can be used to validate
// a cert chain
//
class LLBasicCertificateStore : public LLCertificateStore
{
public:
	LLBasicCertificateStore(const std::string& filename);
	LLBasicCertificateStore(const X509_STORE* store);
	virtual ~LLBasicCertificateStore();
	
	virtual X509_STORE* getOpenSSLX509Store();  // return an openssl X509_STORE  
	// for this store
	
	// add a copy of a cert to the store
	virtual void  append(const LLCertificate& cert);
	
	// add a copy of a cert to the store
	virtual void insert(const int index, const LLCertificate& cert);
	
	// remove a certificate from the store
	virtual void remove(int index);
	
	// return a certificate at the index
	virtual LLPointer<LLCertificate> operator[](int index);
	// return the number of certs in the store
	virtual int len() const;
	
	// load the store from a persisted location
	virtual void load(const std::string& store_id);
	
	// persist the store
	virtual void save();
	
	// return the store id
	virtual std::string storeId();
	
	// validate a cert chain
	virtual bool validate(const LLCertificateChain& cert_chain) const;
};

// LLSecAPIBasicCredential class
class LLSecAPIBasicCredential : public LLCredential
{
public:
	LLSecAPIBasicCredential(const std::string& grid) : LLCredential(grid) {} 
	virtual ~LLSecAPIBasicCredential() {}
	// return a value representing the user id, (could be guid, name, whatever)
	virtual std::string userID() const;	
	
	// printible string identifying the credential.
	virtual std::string asString() const;
};

// LLSecAPIBasicHandler Class
// Interface handler class for the various security storage handlers.
class LLSecAPIBasicHandler : public LLSecAPIHandler
{
public:
	
	LLSecAPIBasicHandler(const std::string& protected_data_filename,
						 const std::string& legacy_password_path);
	LLSecAPIBasicHandler();
	
	virtual ~LLSecAPIBasicHandler();
	
	// instantiate a certificate from a pem string
	virtual LLPointer<LLCertificate> getCertificate(const std::string& pem_cert);
	
	
	// instiate a certificate from an openssl X509 structure
	virtual LLPointer<LLCertificate> getCertificate(X509* openssl_cert);
	
	// instantiate a chain from an X509_STORE_CTX
	virtual LLPointer<LLCertificateChain> getCertificateChain(const X509_STORE_CTX* chain);
	
	// instantiate a cert store given it's id.  if a persisted version
	// exists, it'll be loaded.  If not, one will be created (but not
	// persisted)
	virtual LLPointer<LLCertificateStore> getCertificateStore(const std::string& store_id);
	
	// persist data in a protected store
	virtual void setProtectedData(const std::string& data_type,
								  const std::string& data_id,
								  const LLSD& data);
	
	// retrieve protected data
	virtual LLSD getProtectedData(const std::string& data_type,
								  const std::string& data_id);
	
	// delete a protected data item from the store
	virtual void deleteProtectedData(const std::string& data_type,
									 const std::string& data_id);
	
	// credential management routines
	
	virtual LLPointer<LLCredential> createCredential(const std::string& grid,
													 const LLSD& identifier, 
													 const LLSD& authenticator);
	
	virtual LLPointer<LLCredential> loadCredential(const std::string& grid);

	virtual void saveCredential(LLPointer<LLCredential> cred, bool save_authenticator);
	
	virtual void deleteCredential(LLPointer<LLCredential> cred);
	
protected:
	void _readProtectedData();
	void _writeProtectedData();
	std::string _legacyLoadPassword();

	std::string mProtectedDataFilename;
	LLSD mProtectedDataMap;
	
	std::string mLegacyPasswordPath;
};

#endif // LLSECHANDLER_BASIC



