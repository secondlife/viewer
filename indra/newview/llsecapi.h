/** 
 * @file llsecapi.h
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

#ifndef LLSECAPI_H
#define LLSECAPI_H
#include <vector>
#include <openssl/x509.h>

// All error handling is via exceptions.


#define CERT_SUBJECT_NAME "subject_name"
#define CERT_ISSUER_NAME "issuer_name"
		
#define  CERT_SUBJECT_NAME_STRING "subject_name_string"
#define CERT_ISSUER_NAME_STRING "issuer_name_string"
		
#define CERT_SERIAL_NUMBER "serial_number"
		
#define CERT_VALID_FROM "valid_from"
#define CERT_VALID_TO "valid_to"
#define CERT_SHA1_DIGEST "sha1_digest"
#define CERT_MD5_DIGEST "md5_digest"

#define BASIC_SECHANDLER "BASIC_SECHANDLER"


// All error handling is via exceptions.

class LLCertException
{
public:
	LLCertException(const char* msg)
	{
		llerrs << "Certificate Error: " << msg << llendl;
		mMsg = std::string(msg);
	}
protected:
	std::string mMsg;
};

class LLProtectedDataException
{
public:
	LLProtectedDataException(const char *msg) 
	{
		llerrs << "Certificate Error: " << msg << llendl;
		mMsg = std::string(msg);
	}
protected:
	std::string mMsg;
};

// class LLCertificate
// parent class providing an interface for certifiate.
// LLCertificates are considered unmodifiable
// Certificates are pulled out of stores, or created via
// factory calls
class LLCertificate : public LLRefCount
{
	LOG_CLASS(LLCertificate);
public:
	LLCertificate() {}
	
	virtual ~LLCertificate() {}
	
	// return a PEM encoded certificate.  The encoding
	// includes the -----BEGIN CERTIFICATE----- and end certificate elements
	virtual std::string getPem()=0; 
	
	// return a DER encoded certificate
	virtual std::vector<U8> getBinary()=0;  
	
	// return an LLSD object containing information about the certificate
	// such as its name, signature, expiry time, serial number
	virtual LLSD getLLSD()=0; 
	
	// return an openSSL X509 struct for the certificate
	virtual X509* getOpenSSLX509()=0;

};


// class LLCertificateChain
// Class representing a chain of certificates in order, with the 
// 0th element being the CA
class LLCertificateChain : public LLRefCount
{
	LOG_CLASS(LLCertificateChain);
	static const int VT_SSL = 0;
	static const int VT_AGENT_DOMAIN = 1;
	static const int VT_GRID_DOMAIN = 1;
	
public:
	LLCertificateChain() {}
	
	virtual ~LLCertificateChain() {}
	
	virtual X509_STORE getOpenSSLX509Store()=0;  // return an openssl X509_STORE  
	// for this store
	
	virtual void appendCert(const LLCertificate& cert)=0;  // append a cert to the end 
	//of the chain
	
	virtual LLPointer<LLCertificate>& operator [](int index)=0; // retrieve a certificate 
	// from the chain by index
	// -1 == end of chain
	
	virtual int len() const =0;  // return number of certificates in the chain
	
	// validate a certificate chain given the params.
	// validation type indicates whether it's simply an SSL cert, or 
	// something more specific
	virtual bool validate(int validation_type, 
						  const LLSD& validation_params) const =0;
};


// class LLCertificateStore
// represents a store of certificates, typically a store of root CA
// certificates.  The store can be persisted, and can be used to validate
// a cert chain
//
class LLCertificateStore : public LLRefCount
{
public:
	LLCertificateStore() {}
	virtual ~LLCertificateStore() {}
	
	virtual X509_STORE getOpenSSLX509Store()=0;  // return an openssl X509_STORE  
	// for this store
	
	// add a copy of a cert to the store
	virtual void  append(const LLCertificate& cert)=0;
	
	// add a copy of a cert to the store
	virtual void insert(const int index, const LLCertificate& cert)=0;
	
	// remove a certificate from the store
	virtual void remove(int index)=0;
	
	// return a certificate at the index
	virtual LLPointer<LLCertificate>& operator[](int index)=0;
	// return the number of certs in the store
	virtual int len() const =0;
	
	// load the store from a persisted location
	virtual void load(const std::string& store_id)=0;
	
	// persist the store
	virtual void save()=0;
	
	// return the store id
	virtual std::string storeId()=0;
	
	// validate a cert chain
	virtual bool validate(const LLCertificateChain& cert_chain) const=0;
};


// LLSecAPIHandler Class
// Interface handler class for the various security storage handlers.
class LLSecAPIHandler : public LLRefCount
{
public:
	
	LLSecAPIHandler() {}
	virtual ~LLSecAPIHandler() {}
	
	// instantiate a certificate from a pem string
	virtual LLPointer<LLCertificate> getCertificate(const std::string& pem_cert)=0;
	
	
	// instiate a certificate from an openssl X509 structure
	virtual LLPointer<LLCertificate> getCertificate(X509* openssl_cert)=0;
	
	// instantiate a chain from an X509_STORE_CTX
	virtual LLPointer<LLCertificateChain> getCertificateChain(const X509_STORE_CTX* chain)=0;
	
	// instantiate a cert store given it's id.  if a persisted version
	// exists, it'll be loaded.  If not, one will be created (but not
	// persisted)
	virtual LLPointer<LLCertificateStore> getCertificateStore(const std::string& store_id)=0;
	
	// persist data in a protected store
	virtual void setProtectedData(const std::string& data_type,
								  const std::string& data_id,
								  const LLSD& data)=0;
	
	// retrieve protected data
	virtual LLSD getProtectedData(const std::string& data_type,
								  const std::string& data_id)=0;
};

void secHandlerInitialize();
				
// retrieve a security api depending on the api type
LLPointer<LLSecAPIHandler> getSecHandler(const std::string& handler_type);

void registerSecHandler(const std::string& handler_type, 
						LLPointer<LLSecAPIHandler>& handler);

#endif // LL_SECAPI_H
