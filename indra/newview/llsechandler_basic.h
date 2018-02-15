/** 
 * @file llsechandler_basic.h
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

    // The optional validation_params allow us to make the unit test time-invariant
	LLBasicCertificate(const std::string& pem_cert, const LLSD* validation_params = NULL);
	LLBasicCertificate(X509* openSSLX509, const LLSD* validation_params = NULL);
	
	virtual ~LLBasicCertificate();
	
	virtual std::string getPem() const;
	virtual std::vector<U8> getBinary() const;
	virtual void getLLSD(LLSD &llsd);

	virtual X509* getOpenSSLX509() const;
	
	// set llsd elements for testing
	void setLLSD(const std::string name, const LLSD& value) { mLLSDInfo[name] = value; }

protected:

	// certificates are stored as X509 objects, as validation and
	// other functionality is via openssl
	X509* mCert;
	
	LLSD& _initLLSD();
	LLSD mLLSDInfo;
};


// class LLBasicCertificateVector
// Class representing a list of certificates
// This implementation uses a stl vector of certificates.
class LLBasicCertificateVector : virtual public LLCertificateVector
{
	
public:
	LLBasicCertificateVector() {}
	
	virtual ~LLBasicCertificateVector() {}
	
	// Implementation of the basic iterator implementation.
	// The implementation uses a vector iterator derived from 
	// the vector in the LLBasicCertificateVector class
	class BasicIteratorImpl : public iterator_impl
	{
	public:
		BasicIteratorImpl(std::vector<LLPointer<LLCertificate> >::iterator _iter) { mIter = _iter;}
		virtual ~BasicIteratorImpl() {};
		// seek forward or back.  Used by the operator++/operator-- implementations
		virtual void seek(bool incr)
		{
			if(incr)
			{
				mIter++;
			}
			else
			{
				mIter--;
			}
		}
		// create a copy of the iterator implementation class, used by the iterator copy constructor
		virtual LLPointer<iterator_impl> clone() const
		{
			return new BasicIteratorImpl(mIter);
		}
		
		virtual bool equals(const LLPointer<iterator_impl>& _iter) const
		{
			const BasicIteratorImpl *rhs_iter = dynamic_cast<const BasicIteratorImpl *>(_iter.get());
			llassert(rhs_iter);
			if (!rhs_iter) return 0;
			return (mIter == rhs_iter->mIter);
		}
		virtual LLPointer<LLCertificate> get()
		{
			return *mIter;
		}
	protected:
		friend class LLBasicCertificateVector;
		std::vector<LLPointer<LLCertificate> >::iterator mIter;
	};
	
	// numeric index of the vector
	virtual LLPointer<LLCertificate> operator[](int _index) { return mCerts[_index];}
	
	// Iteration
	virtual iterator begin() { return iterator(new BasicIteratorImpl(mCerts.begin())); }
	
	virtual iterator end() {  return iterator(new BasicIteratorImpl(mCerts.end())); }
	
	// find a cert given params
	virtual iterator find(const LLSD& params);
	
	// return the number of certs in the store
	virtual int size() const { return mCerts.size(); }	
	
	// insert the cert to the store.  if a copy of the cert already exists in the store, it is removed first
	virtual void  add(LLPointer<LLCertificate> cert) { insert(end(), cert); }
	
	// insert the cert to the store.  if a copy of the cert already exists in the store, it is removed first
	virtual void  insert(iterator _iter, LLPointer<LLCertificate> cert);	
	
	// remove a certificate from the store
	virtual LLPointer<LLCertificate> erase(iterator _iter);
	
protected:
	std::vector<LLPointer<LLCertificate> >mCerts;	
};

// class LLCertificateStore
// represents a store of certificates, typically a store of root CA
// certificates.  The store can be persisted, and can be used to validate
// a cert chain
//
class LLBasicCertificateStore : virtual public LLBasicCertificateVector, public LLCertificateStore
{
public:
	LLBasicCertificateStore(const std::string& filename);
	void load_from_file(const std::string& filename);
	
	virtual ~LLBasicCertificateStore();
	
	// persist the store
	virtual void save();
	
	// return the store id
	virtual std::string storeId() const;
	
	// validate a certificate chain against a certificate store, using the
	// given validation policy.
	virtual void validate(int validation_policy,
						  LLPointer<LLCertificateChain> ca_chain,
						  const LLSD& validation_params);
	
protected:
	std::vector<LLPointer<LLCertificate> >            mCerts;
	
	// cache of cert sha1 hashes to from/to date pairs, to improve
	// performance of cert trust.  Note, these are not the CA certs,
	// but the certs that have been validated against this store.
	typedef std::map<std::string, std::pair<LLDate, LLDate> > t_cert_cache;
	t_cert_cache mTrustedCertCache;
	
	std::string mFilename;
};

// class LLCertificateChain
// Class representing a chain of certificates in order, with the 
// first element being the child cert.
class LLBasicCertificateChain : virtual public LLBasicCertificateVector, public LLCertificateChain
{
	
public:
	LLBasicCertificateChain(const X509_STORE_CTX * store);
	
	virtual ~LLBasicCertificateChain() {}
	
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
	
	void init();
	
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
	LLPointer<LLBasicCertificateStore> mStore;
	
	std::string mLegacyPasswordPath;
};

bool valueCompareLLSD(const LLSD& lhs, const LLSD& rhs);

#endif // LLSECHANDLER_BASIC



