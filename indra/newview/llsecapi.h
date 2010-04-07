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
#include <ostream>

#ifdef LL_WINDOWS
#pragma warning(disable:4250)
#endif // LL_WINDOWS

// All error handling is via exceptions.


#define CERT_SUBJECT_NAME "subject_name"
#define CERT_ISSUER_NAME "issuer_name"
#define CERT_NAME_CN "commonName"
		
#define CERT_SUBJECT_NAME_STRING "subject_name_string"
#define CERT_ISSUER_NAME_STRING "issuer_name_string"
		
#define CERT_SERIAL_NUMBER "serial_number"
		
#define CERT_VALID_FROM "valid_from"
#define CERT_VALID_TO "valid_to"
#define CERT_SHA1_DIGEST "sha1_digest"
#define CERT_MD5_DIGEST "md5_digest"
#define CERT_HOSTNAME "hostname"
#define CERT_BASIC_CONSTRAINTS "basicConstraints"
#define CERT_BASIC_CONSTRAINTS_CA "CA"
#define CERT_BASIC_CONSTRAINTS_PATHLEN "pathLen"

#define CERT_KEY_USAGE "keyUsage"
#define CERT_KU_DIGITAL_SIGNATURE    "digitalSignature"
#define CERT_KU_NON_REPUDIATION      "nonRepudiation"
#define CERT_KU_KEY_ENCIPHERMENT     "keyEncipherment"
#define CERT_KU_DATA_ENCIPHERMENT    "dataEncipherment"
#define CERT_KU_KEY_AGREEMENT        "keyAgreement"
#define CERT_KU_CERT_SIGN        "certSigning"
#define CERT_KU_CRL_SIGN             "crlSigning"
#define CERT_KU_ENCIPHER_ONLY        "encipherOnly"
#define CERT_KU_DECIPHER_ONLY        "decipherOnly"

#define BASIC_SECHANDLER "BASIC_SECHANDLER"
#define CERT_VALIDATION_DATE "validation_date"

#define CERT_EXTENDED_KEY_USAGE "extendedKeyUsage"
#define CERT_EKU_SERVER_AUTH SN_server_auth

#define CERT_SUBJECT_KEY_IDENTFIER "subjectKeyIdentifier"
#define CERT_AUTHORITY_KEY_IDENTIFIER "authorityKeyIdentifier"
#define CERT_AUTHORITY_KEY_IDENTIFIER_ID "authorityKeyIdentifierId"
#define CERT_AUTHORITY_KEY_IDENTIFIER_NAME "authorityKeyIdentifierName"
#define CERT_AUTHORITY_KEY_IDENTIFIER_SERIAL "authorityKeyIdentifierSerial"

// validate the current time lies within 
// the validation period of the cert
#define VALIDATION_POLICY_TIME 1

// validate that the CA, or some cert in the chain
// lies within the certificate store
#define VALIDATION_POLICY_TRUSTED 2

// validate that the subject name of
// the cert contains the passed in hostname
// or validates against the hostname
#define VALIDATION_POLICY_HOSTNAME 4


// validate that the cert contains the SSL EKU
#define VALIDATION_POLICY_SSL_KU 8

// validate that the cert contains the SSL EKU
#define VALIDATION_POLICY_CA_KU 16

#define VALIDATION_POLICY_CA_BASIC_CONSTRAINTS 32

// validate that the cert is correct for SSL
#define VALIDATION_POLICY_SSL (VALIDATION_POLICY_TIME | \
                               VALIDATION_POLICY_HOSTNAME | \
                               VALIDATION_POLICY_TRUSTED | \
                               VALIDATION_POLICY_SSL_KU | \
                               VALIDATION_POLICY_CA_BASIC_CONSTRAINTS | \
                               VALIDATION_POLICY_CA_KU)






class LLProtectedDataException
{
public:
	LLProtectedDataException(const char *msg) 
	{
		LL_WARNS("SECAPI") << "Protected Data Error: " << (std::string)msg << LL_ENDL;
		mMsg = (std::string)msg;
	}
	std::string getMessage() { return mMsg; }
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
	virtual std::string getPem() const=0; 
	
	// return a DER encoded certificate
	virtual std::vector<U8> getBinary() const=0;  
	
	// return an LLSD object containing information about the certificate
	// such as its name, signature, expiry time, serial number
	virtual LLSD getLLSD() const=0; 
	
	// return an openSSL X509 struct for the certificate
	virtual X509* getOpenSSLX509() const=0;

};

// class LLCertificateVector
// base class for a list of certificates.


class LLCertificateVector : public LLRefCount
{
	
public:
	
	LLCertificateVector() {};
	virtual ~LLCertificateVector() {};
	
	// base iterator implementation class, providing
	// the functionality needed for the iterator class.
	class iterator_impl : public LLRefCount
	{
	public:
		iterator_impl() {};
		virtual ~iterator_impl() {};
		virtual void seek(bool incr)=0;
		virtual LLPointer<iterator_impl> clone() const=0;
		virtual bool equals(const LLPointer<iterator_impl>& _iter) const=0;
		virtual LLPointer<LLCertificate> get()=0;
	};
	
	// iterator class
	class iterator
	{
	public:
		iterator(LLPointer<iterator_impl> impl) : mImpl(impl) {}
		iterator() : mImpl(NULL) {}
		iterator(const iterator& _iter) {mImpl = _iter.mImpl->clone(); }
		~iterator() {}
		iterator& operator++() { if(mImpl.notNull()) mImpl->seek(true); return *this;}
		iterator& operator--() { if(mImpl.notNull()) mImpl->seek(false); return *this;}
		
		iterator operator++(int) { iterator result = *this; if(mImpl.notNull()) mImpl->seek(true); return result;}
		iterator operator--(int) { iterator result = *this; if(mImpl.notNull()) mImpl->seek(false); return result;}
		LLPointer<LLCertificate> operator*() { return mImpl->get(); }		
		
		LLPointer<iterator_impl> mImpl;
	protected:
		friend bool operator==(const LLCertificateVector::iterator& _lhs, const LLCertificateVector::iterator& _rhs);
		bool equals(const iterator& _iter) const { return mImpl->equals(_iter.mImpl); }
	};
	
	// numeric indexer
	virtual LLPointer<LLCertificate> operator[](int)=0;
	
	// Iteration
	virtual iterator begin()=0;
	
	virtual iterator end()=0;
	
	// find a cert given params
	virtual iterator find(const LLSD& params) =0;
	
	// return the number of certs in the store
	virtual int size() const = 0;	
	
	// append the cert to the store.  if a copy of the cert already exists in the store, it is removed first
	virtual void  add(LLPointer<LLCertificate> cert)=0;
	
	// insert the cert to the store.  if a copy of the cert already exists in the store, it is removed first
	virtual void  insert(iterator location, LLPointer<LLCertificate> cert)=0;	
	
	// remove a certificate from the store
	virtual LLPointer<LLCertificate> erase(iterator cert)=0;	
};


// class LLCertificateStore
// represents a store of certificates, typically a store of root CA
// certificates.  The store can be persisted, and can be used to validate
// a cert chain
//
class LLCertificateStore : virtual public LLCertificateVector
{
	
public:
	
	LLCertificateStore() {}
	virtual ~LLCertificateStore() {}
	
	// persist the store
	virtual void save()=0;
	
	// return the store id
	virtual std::string storeId() const=0;
};

// class LLCertificateChain
// Class representing a chain of certificates in order, with the 
// first element being the child cert.
class LLCertificateChain : virtual public LLCertificateVector
{	

public:
	LLCertificateChain() {}
	
	virtual ~LLCertificateChain() {}

	// validate a certificate chain given the params.
	// Will throw exceptions on error
	
	virtual void validate(int validation_policy,
						  LLPointer<LLCertificateStore> ca_store,
						  const LLSD& validation_params) =0;
};




inline
bool operator==(const LLCertificateVector::iterator& _lhs, const LLCertificateVector::iterator& _rhs)
{
	return _lhs.equals(_rhs);
}
inline
bool operator!=(const LLCertificateVector::iterator& _lhs, const LLCertificateVector::iterator& _rhs)
{
	return !(_lhs == _rhs);
}


//
// LLCredential - interface for credentials providing the following functionality:
// * persistance of credential information based on grid (for saving username/password)
// * serialization to an OGP identifier/authenticator pair
// 
class LLCredential  : public LLRefCount
{
public:
	
	LLCredential() {}
	
	LLCredential(const std::string& grid)
	{
		mGrid = grid;
		mIdentifier = LLSD::emptyMap();
		mAuthenticator = LLSD::emptyMap();
	}
	
	virtual ~LLCredential() {}
	
	virtual void setCredentialData(const LLSD& identifier, const LLSD& authenticator) 
	{ 
		mIdentifier = identifier;
		mAuthenticator = authenticator;
	}
	virtual LLSD getIdentifier() { return mIdentifier; }
	virtual LLSD getAuthenticator() { return mAuthenticator; }
	virtual LLSD getLoginParams();
	virtual std::string getGrid() { return mGrid; }
	

	virtual void clearAuthenticator() { mAuthenticator = LLSD(); } 
	virtual std::string userID() const { return std::string("unknown");}
	virtual std::string asString() const { return std::string("unknown");}
	operator std::string() const { return asString(); }
protected:
	LLSD mIdentifier;
	LLSD mAuthenticator;
	std::string mGrid;
};

std::ostream& operator <<(std::ostream& s, const LLCredential& cred);


// All error handling is via exceptions.

class LLCertException
{
public:
	LLCertException(LLPointer<LLCertificate> cert, const char* msg)
	{

		mCert = cert;

		LL_WARNS("SECAPI") << "Certificate Error: " << (std::string)msg << LL_ENDL;
		mMsg = (std::string)msg;
	}
	LLPointer<LLCertificate> getCert() { return mCert; }
	std::string getMessage() { return mMsg; }
protected:
	LLPointer<LLCertificate> mCert;
	std::string mMsg;
};

class LLInvalidCertificate : public LLCertException
{
public:
	LLInvalidCertificate(LLPointer<LLCertificate> cert) : LLCertException(cert, "CertInvalid")
	{
	}
protected:
};

class LLCertValidationTrustException : public LLCertException
{
public:
	LLCertValidationTrustException(LLPointer<LLCertificate> cert) : LLCertException(cert, "CertUntrusted")
	{
	}
protected:
};

class LLCertValidationHostnameException : public LLCertException
{
public:
	LLCertValidationHostnameException(std::string hostname,
									  LLPointer<LLCertificate> cert) : LLCertException(cert, "CertInvalidHostname")
	{
		mHostname = hostname;
	}
	
	std::string getHostname() { return mHostname; }
protected:
	std::string mHostname;
};

class LLCertValidationExpirationException : public LLCertException
{
public:
	LLCertValidationExpirationException(LLPointer<LLCertificate> cert,
										LLDate current_time) : LLCertException(cert, "CertExpired")
	{
		mTime = current_time;
	}
	LLDate GetTime() { return mTime; }
protected:
	LLDate mTime;
};

class LLCertKeyUsageValidationException : public LLCertException
{
public:
	LLCertKeyUsageValidationException(LLPointer<LLCertificate> cert) : LLCertException(cert, "CertKeyUsage")
	{
	}
protected:
};

class LLCertBasicConstraintsValidationException : public LLCertException
{
public:
	LLCertBasicConstraintsValidationException(LLPointer<LLCertificate> cert) : LLCertException(cert, "CertBasicConstraints")
	{
	}
protected:
};

class LLCertValidationInvalidSignatureException : public LLCertException
{
public:
	LLCertValidationInvalidSignatureException(LLPointer<LLCertificate> cert) : LLCertException(cert, "CertInvalidSignature")
	{
	}
protected:
};

// LLSecAPIHandler Class
// Interface handler class for the various security storage handlers.
class LLSecAPIHandler : public LLRefCount
{
public:
	
	
	LLSecAPIHandler() {}
	virtual ~LLSecAPIHandler() {}
	
	// initialize the SecAPIHandler
	virtual void init() {};
	
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
	
	// delete a protected data item from the store
	virtual void deleteProtectedData(const std::string& data_type,
									 const std::string& data_id)=0;
	
	virtual LLPointer<LLCredential> createCredential(const std::string& grid,
													 const LLSD& identifier, 
													 const LLSD& authenticator)=0;
	
	virtual LLPointer<LLCredential> loadCredential(const std::string& grid)=0;
	
	virtual void saveCredential(LLPointer<LLCredential> cred, bool save_authenticator)=0;
	
	virtual void deleteCredential(LLPointer<LLCredential> cred)=0;
	
};

void initializeSecHandler();
				
// retrieve a security api depending on the api type
LLPointer<LLSecAPIHandler> getSecHandler(const std::string& handler_type);

void registerSecHandler(const std::string& handler_type, 
						LLPointer<LLSecAPIHandler>& handler);

extern LLPointer<LLSecAPIHandler> gSecAPIHandler;


int secapiSSLCertVerifyCallback(X509_STORE_CTX *ctx, void *param);


#endif // LL_SECAPI_H
