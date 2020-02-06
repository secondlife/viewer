/** 
 * @file llsecapi.h
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
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

#ifndef LLSECAPI_H
#define LLSECAPI_H
#include <vector>
#include "llwin32headerslean.h"
#include <openssl/x509.h>
#include <ostream>
#include "llpointer.h"
#include "llexception.h"

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






struct LLProtectedDataException: public LLException
{
	LLProtectedDataException(const std::string& msg):
		LLException(msg)
	{
		LL_WARNS("SECAPI") << "Protected Data Error: " << msg << LL_ENDL;
	}
};

// class LLCertificate
// parent class providing an interface for certifiate.
// LLCertificates are considered unmodifiable
// Certificates are pulled out of stores, or created via
// factory calls
class LLCertificate : public LLThreadSafeRefCount
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
	virtual void getLLSD(LLSD& llsd)=0; 
	
	// return an openSSL X509 struct for the certificate
	virtual X509* getOpenSSLX509() const=0;

};

// class LLCertificateVector
// base class for a list of certificates.


class LLCertificateVector : public LLThreadSafeRefCount
{
	
public:
	
	LLCertificateVector() {};
	virtual ~LLCertificateVector() {};
	
	// base iterator implementation class, providing
	// the functionality needed for the iterator class.
	class iterator_impl : public LLThreadSafeRefCount
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

// class LLCertificateChain
// Class representing a chain of certificates in order, with the 
// first element being the child cert.
class LLCertificateChain : virtual public LLCertificateVector
{	
	
public:
	LLCertificateChain() {}
	
	virtual ~LLCertificateChain() {}
	
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
	
	// validate a certificate chain given the params.
	// Will throw exceptions on error
	
	virtual void validate(int validation_policy,
						  LLPointer<LLCertificateChain> cert_chain,
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


#define CRED_IDENTIFIER_TYPE_ACCOUNT "account"
#define CRED_IDENTIFIER_TYPE_AGENT "agent"
#define CRED_AUTHENTICATOR_TYPE_CLEAR "clear"
#define CRED_AUTHENTICATOR_TYPE_HASH   "hash"
//
// LLCredential - interface for credentials providing the following functionality:
// * Persistence of credential information based on grid (for saving username/password)
// * Serialization to an OGP identifier/authenticator pair
// 
class LLCredential  : public LLThreadSafeRefCount
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
	virtual void identifierType(std::string& idType);
	virtual LLSD getAuthenticator() { return mAuthenticator; }
	virtual void authenticatorType(std::string& authType);
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

class LLCertException: public LLException
{
public:
	LLCertException(const LLSD& cert_data, const std::string& msg): LLException(msg),
        mCertData(cert_data)
	{
		LL_WARNS("SECAPI") << "Certificate Error: " << msg << LL_ENDL;
	}
	virtual ~LLCertException() throw() {}
	LLSD getCertData() const { return mCertData; }
protected:
	LLSD mCertData;
};

class LLInvalidCertificate : public LLCertException
{
public:
	LLInvalidCertificate(const LLSD& cert_data) : LLCertException(cert_data, "CertInvalid")
	{
	}
	virtual ~LLInvalidCertificate() throw() {}
protected:
};

class LLCertValidationTrustException : public LLCertException
{
public:
	LLCertValidationTrustException(const LLSD& cert_data) : LLCertException(cert_data, "CertUntrusted")
	{
	}
	virtual ~LLCertValidationTrustException() throw() {}
protected:
};

class LLCertValidationHostnameException : public LLCertException
{
public:
	LLCertValidationHostnameException(std::string hostname,
									  const LLSD& cert_data) : LLCertException(cert_data, "CertInvalidHostname")
	{
		mHostname = hostname;
	}
	virtual ~LLCertValidationHostnameException() throw() {}
	std::string getHostname() { return mHostname; }
protected:
	std::string mHostname;
};

class LLCertValidationExpirationException : public LLCertException
{
public:
	LLCertValidationExpirationException(const LLSD& cert_data,
										LLDate current_time) : LLCertException(cert_data, "CertExpired")
	{
		mTime = current_time;
	}
	virtual ~LLCertValidationExpirationException() throw() {}
	LLDate GetTime() { return mTime; }
protected:
	LLDate mTime;
};

class LLCertKeyUsageValidationException : public LLCertException
{
public:
	LLCertKeyUsageValidationException(const LLSD& cert_data) : LLCertException(cert_data, "CertKeyUsage")
	{
	}
	virtual ~LLCertKeyUsageValidationException() throw() {}
protected:
};

class LLCertBasicConstraintsValidationException : public LLCertException
{
public:
	LLCertBasicConstraintsValidationException(const LLSD& cert_data) : LLCertException(cert_data, "CertBasicConstraints")
	{
	}
	virtual ~LLCertBasicConstraintsValidationException() throw() {}
protected:
};

class LLCertValidationInvalidSignatureException : public LLCertException
{
public:
	LLCertValidationInvalidSignatureException(const LLSD& cert_data) : LLCertException(cert_data, "CertInvalidSignature")
	{
	}
	virtual ~LLCertValidationInvalidSignatureException() throw() {}
protected:
};

// LLSecAPIHandler Class
// Interface handler class for the various security storage handlers.
class LLSecAPIHandler : public LLThreadSafeRefCount
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

	// persist data in a protected store's map
	virtual void addToProtectedMap(const std::string& data_type,
								   const std::string& data_id,
								   const std::string& map_elem,
								   const LLSD& data)=0;

	// remove data from protected store's map
	virtual void removeFromProtectedMap(const std::string& data_type,
										const std::string& data_id,
										const std::string& map_elem)=0;

public:
	virtual LLPointer<LLCredential> createCredential(const std::string& grid,
													 const LLSD& identifier, 
													 const LLSD& authenticator)=0;
	
	virtual LLPointer<LLCredential> loadCredential(const std::string& grid)=0;
	
	virtual void saveCredential(LLPointer<LLCredential> cred, bool save_authenticator)=0;
	
	virtual void deleteCredential(LLPointer<LLCredential> cred)=0;

	// has map of credentials declared as specific storage
	virtual bool hasCredentialMap(const std::string& storage,
								  const std::string& grid)=0;

	// returns true if map is empty or does not exist
	virtual bool emptyCredentialMap(const std::string& storage,
									const std::string& grid)=0;

	// load map of credentials from specific storage
	typedef std::map<std::string, LLPointer<LLCredential> > credential_map_t;
	virtual void loadCredentialMap(const std::string& storage,
								   const std::string& grid,
								   credential_map_t& credential_map)=0;

	// load single username from map of credentials from specific storage
	virtual LLPointer<LLCredential> loadFromCredentialMap(const std::string& storage,
														  const std::string& grid,
														  const std::string& userid)=0;

	// add item to map of credentials from specific storage
	virtual void addToCredentialMap(const std::string& storage,
									LLPointer<LLCredential> cred,
									bool save_authenticator)=0;

	// remove item from map of credentials from specific storage
	virtual void removeFromCredentialMap(const std::string& storage,
										 LLPointer<LLCredential> cred)=0;

	// remove item from map of credentials from specific storage
	virtual void removeFromCredentialMap(const std::string& storage,
										 const std::string& grid,
										 const std::string& userid)=0;

	virtual void removeCredentialMap(const std::string& storage,
									 const std::string& grid)=0;
	
};

void initializeSecHandler();
				
// retrieve a security api depending on the api type
LLPointer<LLSecAPIHandler> getSecHandler(const std::string& handler_type);

void registerSecHandler(const std::string& handler_type, 
						LLPointer<LLSecAPIHandler>& handler);

extern LLPointer<LLSecAPIHandler> gSecAPIHandler;


#endif // LL_SECAPI_H
