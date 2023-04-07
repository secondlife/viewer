/** 
 * @file llsechandler_basic.cpp
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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


#include "llviewerprecompiledheaders.h"
#include "llsecapi.h"
#include "llsechandler_basic.h"
#include "llsdserialize.h"
#include "llviewernetwork.h"
#include "llxorcipher.h"
#include "llfile.h"
#include "lldir.h"
#include "llviewercontrol.h"
#include "llexception.h"
#include "stringize.h"
#include <vector>
#include <ios>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <iostream>
#include <iomanip>
#include <time.h>
#include "llmachineid.h"


static const std::string DEFAULT_CREDENTIAL_STORAGE = "credential";

// 128 bits of salt data...
#define STORE_SALT_SIZE 16 
#define BUFFER_READ_SIZE 256
std::string cert_string_from_asn1_string(ASN1_STRING* value);
std::string cert_string_from_octet_string(ASN1_OCTET_STRING* value);

LLSD _basic_constraints_ext(X509* cert);
LLSD _key_usage_ext(X509* cert);
LLSD _ext_key_usage_ext(X509* cert);
std::string _subject_key_identifier(X509 *cert);
LLSD _authority_key_identifier(X509* cert);
void _validateCert(int validation_policy,
                   LLPointer<LLCertificate> cert,
                   const LLSD& validation_params,
                   int depth);

LLBasicCertificate::LLBasicCertificate(const std::string& pem_cert,
                                       const LLSD* validation_params) 
{
	// BIO_new_mem_buf returns a read only bio, but takes a void* which isn't const
	// so we need to cast it.
	BIO * pem_bio = BIO_new_mem_buf((void*)pem_cert.c_str(), pem_cert.length());
	if(pem_bio == NULL)
	{
        LL_WARNS("SECAPI") << "Could not allocate an openssl memory BIO." << LL_ENDL;
        LLTHROW(LLAllocationCertException(LLSD::emptyMap()));
	}
	mCert = NULL;
	PEM_read_bio_X509(pem_bio, &mCert, 0, NULL);
	BIO_free(pem_bio);
	if (!mCert)
	{
        LL_WARNS("SECAPI") << "Could not decode certificate to x509." << LL_ENDL;
        LLTHROW(LLInvalidCertificate(LLSD::emptyMap()));
	}
}


LLBasicCertificate::LLBasicCertificate(X509* pCert,
                                       const LLSD* validation_params) 
{
	if (!pCert)
	{
		LLTHROW(LLInvalidCertificate(LLSD::emptyMap()));
	}	
	mCert = X509_dup(pCert);
    // it is tempting to run _validateCert here, but doing so causes problems
    // the trick is figuring out which aspects to validate. TBD
}

LLBasicCertificate::~LLBasicCertificate() 
{
	if(mCert)
	{
		X509_free(mCert);
        mCert = NULL;
	}
}

//
// retrieve the pem using the openssl functionality
std::string LLBasicCertificate::getPem() const
{ 
	char * pem_bio_chars = NULL;
	// a BIO is the equivalent of a 'std::stream', and
	// can be a file, mem stream, whatever.  Grab a memory based
	// BIO for the result
	BIO *pem_bio = BIO_new(BIO_s_mem());
	if (!pem_bio)
	{
		LL_WARNS("SECAPI") << "Could not allocate an openssl memory BIO." << LL_ENDL;		
		return std::string();
	}
	PEM_write_bio_X509(pem_bio, mCert);
	int length = BIO_get_mem_data(pem_bio, &pem_bio_chars);
	std::string result = std::string(pem_bio_chars, length);
	BIO_free(pem_bio);
	return result;
}

// get the DER encoding for the cert
// DER is a binary encoding format for certs...
std::vector<U8> LLBasicCertificate::getBinary() const
{ 
	U8 * der_bio_data = NULL;
	// get a memory bio 
	BIO *der_bio = BIO_new(BIO_s_mem());
	if (!der_bio)
	{
		LL_WARNS("SECAPI") << "Could not allocate an openssl memory BIO." << LL_ENDL;			
		return std::vector<U8>();
	}
	i2d_X509_bio(der_bio, mCert);
	int length = BIO_get_mem_data(der_bio, &der_bio_data);
	std::vector<U8> result(length);
	// vectors are guranteed to be a contiguous chunk of memory.
	memcpy(&result[0], der_bio_data,  length);
	BIO_free(der_bio);
	return result;
}


void LLBasicCertificate::getLLSD(LLSD &llsd)
{
	if (mLLSDInfo.isUndefined())
	{
		_initLLSD();
	}
	llsd = mLLSDInfo;
}

// Initialize the LLSD info for the certificate
LLSD& LLBasicCertificate::_initLLSD()
{ 

	// call the various helpers to build the LLSD
	mLLSDInfo[CERT_SUBJECT_NAME] = cert_name_from_X509_NAME(X509_get_subject_name(mCert));
	mLLSDInfo[CERT_ISSUER_NAME] = cert_name_from_X509_NAME(X509_get_issuer_name(mCert));
	mLLSDInfo[CERT_SUBJECT_NAME_STRING] = cert_string_name_from_X509_NAME(X509_get_subject_name(mCert));
	mLLSDInfo[CERT_ISSUER_NAME_STRING] = cert_string_name_from_X509_NAME(X509_get_issuer_name(mCert));
	ASN1_INTEGER *sn = X509_get_serialNumber(mCert);
	if (sn != NULL)
	{
		mLLSDInfo[CERT_SERIAL_NUMBER] = cert_string_from_asn1_integer(sn);
	}
	
	mLLSDInfo[CERT_VALID_TO] = cert_date_from_asn1_time(X509_get_notAfter(mCert));
	mLLSDInfo[CERT_VALID_FROM] = cert_date_from_asn1_time(X509_get_notBefore(mCert));
	// add the known extensions
	mLLSDInfo[CERT_BASIC_CONSTRAINTS] = _basic_constraints_ext(mCert);
	mLLSDInfo[CERT_KEY_USAGE] = _key_usage_ext(mCert);
	mLLSDInfo[CERT_EXTENDED_KEY_USAGE] = _ext_key_usage_ext(mCert);
	mLLSDInfo[CERT_SUBJECT_KEY_IDENTFIER] = _subject_key_identifier(mCert);
	mLLSDInfo[CERT_AUTHORITY_KEY_IDENTIFIER] = _authority_key_identifier(mCert);
	return mLLSDInfo; 
}

// Retrieve the basic constraints info
LLSD _basic_constraints_ext(X509* cert)
{
	LLSD result;
	BASIC_CONSTRAINTS *bs = (BASIC_CONSTRAINTS *)X509_get_ext_d2i(cert, NID_basic_constraints, NULL, NULL);
	if(bs)
	{
		result = LLSD::emptyMap();
		// Determines whether the cert can be used as a CA
		result[CERT_BASIC_CONSTRAINTS_CA] = (bool)bs->ca;
	
		if(bs->pathlen) 
		{
			// the pathlen determines how deep a certificate chain can be from
			// this CA
			if((bs->pathlen->type == V_ASN1_NEG_INTEGER)
			   || !bs->ca) 
			{
				result[CERT_BASIC_CONSTRAINTS_PATHLEN] = 0;
			} 
			else 
			{
				result[CERT_BASIC_CONSTRAINTS_PATHLEN] = (int)ASN1_INTEGER_get(bs->pathlen);
			}
		}

		BASIC_CONSTRAINTS_free( bs );
	}
	return result;
}

// retrieve the key usage, which specifies how the cert can be used.
// 
LLSD _key_usage_ext(X509* cert)
{
	LLSD result;
	ASN1_STRING *usage_str = (ASN1_STRING *)X509_get_ext_d2i(cert, NID_key_usage, NULL, NULL);
	if(usage_str)
	{
		result = LLSD::emptyArray();
		long usage = 0;
		if(usage_str->length > 0) 
		{
			usage = usage_str->data[0];
			if(usage_str->length > 1)
			{
				usage |= usage_str->data[1] << 8;
			}
		}
		ASN1_STRING_free(usage_str);
		if(usage)
		{
			if(usage & KU_DIGITAL_SIGNATURE) result.append(LLSD((std::string)CERT_KU_DIGITAL_SIGNATURE));
			if(usage & KU_NON_REPUDIATION) result.append(LLSD((std::string)CERT_KU_NON_REPUDIATION));
			if(usage & KU_KEY_ENCIPHERMENT) result.append(LLSD((std::string)CERT_KU_KEY_ENCIPHERMENT));
			if(usage & KU_DATA_ENCIPHERMENT) result.append(LLSD((std::string)CERT_KU_DATA_ENCIPHERMENT));
			if(usage & KU_KEY_AGREEMENT) result.append(LLSD((std::string)CERT_KU_KEY_AGREEMENT));
			if(usage & KU_KEY_CERT_SIGN) result.append(LLSD((std::string)CERT_KU_CERT_SIGN));			
			if(usage & KU_CRL_SIGN) result.append(LLSD((std::string)CERT_KU_CRL_SIGN));	
			if(usage & KU_ENCIPHER_ONLY) result.append(LLSD((std::string)CERT_KU_ENCIPHER_ONLY));				
			if(usage & KU_DECIPHER_ONLY) result.append(LLSD((std::string)CERT_KU_DECIPHER_ONLY));		
		}
	}
	return result;
}

// retrieve the extended key usage for the cert
LLSD _ext_key_usage_ext(X509* cert)
{
	LLSD result;
	EXTENDED_KEY_USAGE *eku = (EXTENDED_KEY_USAGE *)X509_get_ext_d2i(cert, NID_ext_key_usage, NULL, NULL);
	if(eku)
	{
		result = LLSD::emptyArray();
		while(sk_ASN1_OBJECT_num(eku))
		{
			ASN1_OBJECT *usage = sk_ASN1_OBJECT_pop(eku);
			if(usage)
			{
				int nid = OBJ_obj2nid(usage);
				if (nid)
				{
					std::string sn = OBJ_nid2sn(nid);
					result.append(sn);
				}
				ASN1_OBJECT_free(usage);
			}
		}

		EXTENDED_KEY_USAGE_free( eku );
	}
	return result;
}

// retrieve the subject key identifier of the cert
std::string _subject_key_identifier(X509 *cert)
{
    std::string result;
	ASN1_OCTET_STRING *skeyid = (ASN1_OCTET_STRING *)X509_get_ext_d2i(cert, NID_subject_key_identifier, NULL, NULL);
	if(skeyid)
	{
		result = cert_string_from_octet_string(skeyid);
		ASN1_OCTET_STRING_free( skeyid );
	}
	return result;
}

// retrieve the authority key identifier of the cert
LLSD _authority_key_identifier(X509* cert)
{
	LLSD result;
	AUTHORITY_KEYID *akeyid = (AUTHORITY_KEYID *)X509_get_ext_d2i(cert, NID_authority_key_identifier, NULL, NULL);
	if(akeyid)
	{
		result = LLSD::emptyMap();
		if(akeyid->keyid)
		{
			result[CERT_AUTHORITY_KEY_IDENTIFIER_ID] = cert_string_from_octet_string(akeyid->keyid);
		}
		if(akeyid->serial)
		{
			result[CERT_AUTHORITY_KEY_IDENTIFIER_SERIAL] = cert_string_from_asn1_integer(akeyid->serial);
		}	

		AUTHORITY_KEYID_free( akeyid );
	}
	// we ignore the issuer name in the authority key identifier, we check the issue name via
	// the the issuer name entry in the cert.
	return result;
}

// retrieve an openssl x509 object,
// which must be freed by X509_free
X509* LLBasicCertificate::getOpenSSLX509() const
{ 
	return X509_dup(mCert); 
}  

// generate a single string containing the subject or issuer
// name of the cert.
std::string cert_string_name_from_X509_NAME(X509_NAME* name)
{
	char * name_bio_chars = NULL;
	// get a memory bio
	BIO *name_bio = BIO_new(BIO_s_mem());
	// stream the name into the bio.  The name will be in the 'short name' format
	X509_NAME_print_ex(name_bio, name, 0, XN_FLAG_RFC2253);
	int length = BIO_get_mem_data(name_bio, &name_bio_chars);
	std::string result = std::string(name_bio_chars, length);
	BIO_free(name_bio);
	return result;
}

// generate an LLSD from a certificate name (issuer or subject name).  
// the name will be strings indexed by the 'long form'
LLSD cert_name_from_X509_NAME(X509_NAME* name)
{
	LLSD result = LLSD::emptyMap();
	int name_entries = X509_NAME_entry_count(name);
	for (int entry_index=0; entry_index < name_entries; entry_index++) 
	{
		char buffer[32];
		X509_NAME_ENTRY *entry = X509_NAME_get_entry(name, entry_index);
		
		std::string name_value = std::string((const char*)ASN1_STRING_get0_data(X509_NAME_ENTRY_get_data(entry)), 
											 ASN1_STRING_length(X509_NAME_ENTRY_get_data(entry)));

		ASN1_OBJECT* name_obj = X509_NAME_ENTRY_get_object(entry);		
		OBJ_obj2txt(buffer, sizeof(buffer), name_obj, 0);
		std::string obj_buffer_str = std::string(buffer);
		result[obj_buffer_str] = name_value;
	}
	
	return result;
}

// Generate a string from an ASN1 integer.  ASN1 Integers are
// bignums, so they can be 'infinitely' long, therefore we
// cannot simply use a conversion to U64 or something.
// We retrieve as a readable string for UI

std::string cert_string_from_asn1_integer(ASN1_INTEGER* value)
{
	std::string result;
	BIGNUM *bn = ASN1_INTEGER_to_BN(value, NULL);
	if(bn)
	{
		char * ascii_bn = BN_bn2hex(bn);

		if(ascii_bn)
		{
			result = ascii_bn;
			OPENSSL_free(ascii_bn);
		}
		BN_free(bn);
	}
	return result;
}

// Generate a string from an OCTET string.
// we retrieve as a 

std::string cert_string_from_octet_string(ASN1_OCTET_STRING* value)
{
	
	std::stringstream result;
	result << std::hex << std::setprecision(2);
	for (int i=0; i < value->length; i++)
	{
		if (i != 0) 
		{
			result << ":";
		}
		result  << std::setfill('0') << std::setw(2) << (int)value->data[i];
	}
	return result.str();
}

// Generate a string from an ASN1 integer.  ASN1 Integers are
// bignums, so they can be 'infinitely' long, therefore we
// cannot simply use a conversion to U64 or something.
// We retrieve as a readable string for UI

std::string cert_string_from_asn1_string(ASN1_STRING* value)
{
	char * string_bio_chars = NULL;
	std::string result;
	// get a memory bio
	BIO *string_bio = BIO_new(BIO_s_mem());
	if(!string_bio)
	{
		// stream the name into the bio.  The name will be in the 'short name' format
		ASN1_STRING_print_ex(string_bio, value, ASN1_STRFLGS_RFC2253);
		int length = BIO_get_mem_data(string_bio, &string_bio_chars);
		result = std::string(string_bio_chars, length);
		BIO_free(string_bio);
	}
	else
	{
		LL_WARNS("SECAPI") << "Could not allocate an openssl memory BIO." << LL_ENDL;
	}
	
	return result;
}

// retrieve a date structure from an ASN1 time, for 
// validity checking.
LLDate cert_date_from_asn1_time(ASN1_TIME* asn1_time)
{
	
	struct tm timestruct = {0};
	int i = asn1_time->length;
	
	if (i < 10)
	{
		return LLDate();
	}	
	// convert the date from the ASN1 time (which is a string in ZULU time), to
	// a timeval.
	timestruct.tm_year = (asn1_time->data[0]-'0') * 10 + (asn1_time->data[1]-'0');
	
	/* Deal with Year 2000 */
	if (timestruct.tm_year < 70)
		timestruct.tm_year += 100;
	
	timestruct.tm_mon = (asn1_time->data[2]-'0') * 10 + (asn1_time->data[3]-'0') - 1;
	timestruct.tm_mday = (asn1_time->data[4]-'0') * 10 + (asn1_time->data[5]-'0');
	timestruct.tm_hour = (asn1_time->data[6]-'0') * 10 + (asn1_time->data[7]-'0');
	timestruct.tm_min = (asn1_time->data[8]-'0') * 10 + (asn1_time->data[9]-'0');
	timestruct.tm_sec = (asn1_time->data[10]-'0') * 10 + (asn1_time->data[11]-'0');

#if LL_WINDOWS
	return LLDate((F64)_mkgmtime(&timestruct));
#else // LL_WINDOWS
	return LLDate((F64)timegm(&timestruct));
#endif // LL_WINDOWS
}

// class LLBasicCertificateVector
// This class represents a list of certificates, implemented by a vector of certificate pointers.
// it contains implementations of the virtual functions for iterators, search, add, remove, etc.
//

//  Find a certificate in the list.
// It will find a cert that has minimally the params listed, with the values being the same
LLBasicCertificateVector::iterator LLBasicCertificateVector::find(const LLSD& params)
{
	// loop through the entire vector comparing the values in the certs
	// against those passed in via the params.
	// params should be a map.  Only the items specified in the map will be
	// checked, but they must match exactly, even if they're maps or arrays.
    bool found = false;
	iterator cert = begin();
	while ( !found && cert != end() )
	{
		found = true;
		LLSD cert_info;
		(*cert)->getLLSD(cert_info);
		for (LLSD::map_const_iterator param = params.beginMap();
			 found && param != params.endMap();
			 param++)
		{
			if (   !cert_info.has((std::string)param->first)
                || !valueCompareLLSD(cert_info[(std::string)param->first], param->second))
			{
				found = false;
			}
		}
        if (!found)
        {
            cert++;
        }
	}
	return cert;
}

// Insert a certificate into the store.  If the certificate already 
// exists in the store, nothing is done.
void  LLBasicCertificateVector::insert(iterator _iter, 
				       LLPointer<LLCertificate> cert)
{
	LLSD cert_info;
	cert->getLLSD(cert_info);
	if (cert_info.isMap() && cert_info.has(CERT_SUBJECT_KEY_IDENTFIER))
	{
		LLSD existing_cert_info = LLSD::emptyMap();
		existing_cert_info[CERT_SUBJECT_KEY_IDENTFIER] = cert_info[CERT_SUBJECT_KEY_IDENTFIER];
		if(find(existing_cert_info) == end())
		{
			BasicIteratorImpl *basic_iter = dynamic_cast<BasicIteratorImpl*>(_iter.mImpl.get());
			if (basic_iter)
			{
				mCerts.insert(basic_iter->mIter, cert);
			}
            else
            {
                LL_WARNS("SECAPI") << "Invalid certificate postion vector"
                                   << LL_ENDL;
            }
		}
        else
        {
            LL_DEBUGS("SECAPI") << "Certificate already in vector: "
                                << "'" << cert_info << "'"
                                << LL_ENDL;
        }

	}
    else
    {
        LL_WARNS("SECAPI") << "Certificate does not have Subject Key Identifier; not inserted: "
                           << "'" << cert_info << "'"
                           << LL_ENDL;
    }
}

// remove a certificate from the store
LLPointer<LLCertificate> LLBasicCertificateVector::erase(iterator _iter)
{
	
	if (_iter != end())
	{
		BasicIteratorImpl *basic_iter = dynamic_cast<BasicIteratorImpl*>(_iter.mImpl.get());
		LLPointer<LLCertificate> result = (*_iter);
		mCerts.erase(basic_iter->mIter);
		return result;
	}
	return NULL;
}


//
// LLBasicCertificateStore
// This class represents a store of CA certificates.  The basic implementation
// uses a crt file such as the ca-bundle.crt in the existing SL implementation.
LLBasicCertificateStore::LLBasicCertificateStore(const std::string& filename)
{
	mFilename = filename;
	load_from_file(filename);
}

void LLBasicCertificateStore::load_from_file(const std::string& filename)
{
    int loaded = 0;
    int rejected = 0;

	// scan the PEM file extracting each certificate
	if (LLFile::isfile(filename))
	{
        BIO* file_bio = BIO_new(BIO_s_file());
        if(file_bio)
        {
            if (BIO_read_filename(file_bio, filename.c_str()) > 0)
            {	
                X509 *cert_x509 = NULL;
                while((PEM_read_bio_X509(file_bio, &cert_x509, 0, NULL)) && 
                      (cert_x509 != NULL))
                {
                    try
                    {
                        LLPointer<LLBasicCertificate> new_cert(new LLBasicCertificate(cert_x509));
                        LLSD validation_params;
                        _validateCert(VALIDATION_POLICY_TIME,
                                      new_cert,
                                      validation_params,
                                      0);
                        add(new_cert);
                        LL_DEBUGS("SECAPI") << "Loaded valid cert for "
                                            << "Name '" << cert_string_name_from_X509_NAME(X509_get_subject_name(cert_x509)) << "'";
                        std::string skeyid(_subject_key_identifier(cert_x509));
                        LL_CONT << " Id '" << skeyid << "'"
                                << LL_ENDL;
                        loaded++;
                    }
                    catch (LLCertException& cert_exception)
                    {
                        LLSD cert_info(cert_exception.getCertData());
                        LL_DEBUGS("SECAPI_BADCERT","SECAPI") << "invalid certificate (" << cert_exception.what() << "): " << cert_info << LL_ENDL;
                        rejected++;
                    }
                    catch (...)
                    {
                        LOG_UNHANDLED_EXCEPTION("creating certificate from the certificate store file");
                        rejected++;
                    }
                    X509_free(cert_x509);
                    cert_x509 = NULL;
                }
                BIO_free(file_bio);
            }
            else
            {
                LL_WARNS("SECAPI") << "BIO read failed for " << filename << LL_ENDL;
            }

            LL_INFOS("SECAPI") << "loaded " << loaded << " good certificates (rejected " << rejected << ") from " << filename << LL_ENDL;
        }
        else
        {
            LL_WARNS("SECAPI") << "Could not allocate a file BIO" << LL_ENDL;
        }
    }
    else
    {
        // since the user certificate store may not be there, this is not a warning
        LL_INFOS("SECAPI") << "Certificate store not found at " << filename << LL_ENDL;
    }
}


LLBasicCertificateStore::~LLBasicCertificateStore()
{
}


// persist the store
void LLBasicCertificateStore::save()
{
	llofstream file_store(mFilename.c_str(), std::ios_base::binary);
	if(!file_store.fail())
	{
		for(iterator cert = begin();
			cert != end();
			cert++)
		{
			std::string pem = (*cert)->getPem();
			if(!pem.empty())
			{
				file_store << (*cert)->getPem() << std::endl;
			}
		}
		file_store.close();
	}
	else
	{
		LL_WARNS("SECAPI") << "Could not open certificate store " << mFilename << "for save" << LL_ENDL;
	}
}

// return the store id
std::string LLBasicCertificateStore::storeId() const
{
	// this is the basic handler which uses the ca-bundle.crt store,
	// so we ignore this.
	return std::string("");
}


//
// LLBasicCertificateChain
// This class represents a chain of certs, each cert being signed by the next cert
// in the chain.  Certs must be properly signed by the parent
LLBasicCertificateChain::LLBasicCertificateChain(X509_STORE_CTX* store)
{

	// we're passed in a context, which contains a cert, and a blob of untrusted
	// certificates which compose the chain.
	if((store == NULL) || X509_STORE_CTX_get0_cert(store) == NULL)
	{
		LL_WARNS("SECAPI") << "An invalid store context was passed in when trying to create a certificate chain" << LL_ENDL;
		return;
	}
	// grab the child cert
	LLPointer<LLCertificate> current = new LLBasicCertificate(X509_STORE_CTX_get0_cert(store));

	add(current);
	if(X509_STORE_CTX_get0_untrusted(store) != NULL)
	{
		// if there are other certs in the chain, we build up a vector
		// of untrusted certs so we can search for the parents of each
		// consecutive cert.
		LLBasicCertificateVector untrusted_certs;
		for(int i = 0; i < sk_X509_num(X509_STORE_CTX_get0_untrusted(store)); i++)
		{
			LLPointer<LLCertificate> cert = new LLBasicCertificate(sk_X509_value(X509_STORE_CTX_get0_untrusted(store), i));
			untrusted_certs.add(cert);

		}		
		while(untrusted_certs.size() > 0)
		{
			LLSD find_data = LLSD::emptyMap();
			LLSD cert_data;
			current->getLLSD(cert_data);
			// we simply build the chain via subject/issuer name as the
			// client should not have passed in multiple CA's with the same 
			// subject name.  If they did, it'll come out in the wash during
			// validation.
			find_data[CERT_SUBJECT_NAME_STRING] = cert_data[CERT_ISSUER_NAME_STRING]; 
			LLBasicCertificateVector::iterator issuer = untrusted_certs.find(find_data);
			if (issuer != untrusted_certs.end())
			{
				current = untrusted_certs.erase(issuer);
				add(current);
			}
			else
			{
				break;
			}
		}
	}
}


// subdomain wildcard specifiers can be divided into 3 parts
// the part before the first *, the part after the first * but before
// the second *, and the part after the second *.
// It then iterates over the second for each place in the string
// that it matches.  ie if the subdomain was testfoofoobar, and
// the wildcard was test*foo*bar, it would match test, then
// recursively match foofoobar and foobar

bool _cert_subdomain_wildcard_match(const std::string& subdomain,
									const std::string& wildcard)
{
	// split wildcard into the portion before the *, and the portion after

	int wildcard_pos = wildcard.find_first_of('*');	
	// check the case where there is no wildcard.
	if(wildcard_pos == wildcard.npos)
	{
		return (subdomain == wildcard);
	}
	
	// we need to match the first part of the subdomain string up to the wildcard
	// position
	if(subdomain.substr(0, wildcard_pos) != wildcard.substr(0, wildcard_pos))
	{
		// the first portions of the strings didn't match
		return FALSE;
	}
	
	// as the portion of the wildcard string before the * matched, we need to check the
	// portion afterwards.  Grab that portion.
	std::string new_wildcard_string = wildcard.substr( wildcard_pos+1, wildcard.npos);
	if(new_wildcard_string.empty())
	{
		// we had nothing after the *, so it's an automatic match
		return TRUE;
	}
	
	// grab the portion of the remaining wildcard string before the next '*'.  We need to find this
	// within the remaining subdomain string. and then recursively check.
	std::string new_wildcard_match_string = new_wildcard_string.substr(0, new_wildcard_string.find_first_of('*'));
	
	// grab the portion of the subdomain after the part that matched the initial wildcard portion
	std::string new_subdomain = subdomain.substr(wildcard_pos, subdomain.npos);
	
	// iterate through the current subdomain, finding instances of the match string.
	int sub_pos = new_subdomain.find_first_of(new_wildcard_match_string);
	while(sub_pos != std::string::npos)
	{
		new_subdomain = new_subdomain.substr(sub_pos, std::string::npos);
		if(_cert_subdomain_wildcard_match(new_subdomain, new_wildcard_string))
		{
			return TRUE;
		}
		sub_pos = new_subdomain.find_first_of(new_wildcard_match_string, 1);


	}
	// didn't find any instances of the match string that worked in the subdomain, so fail.
	return FALSE;
}


// RFC2459 does not address wildcards as part of it's name matching
// specification, and there is no RFC specifying wildcard matching,
// RFC2818 does a few statements about wildcard matching, but is very 
// general.  Generally, wildcard matching is per implementation, although
// it's pretty similar.
// in our case, we use the '*' wildcard character only, within each
// subdomain.  The hostname and the CN specification should have the
// same number of subdomains.
// We then iterate that algorithm over each subdomain.
bool _cert_hostname_wildcard_match(const std::string& hostname, const std::string& common_name)
{
	std::string new_hostname = hostname;
	std::string new_cn = common_name;
	
	// find the last '.' in the hostname and the match name.
	int subdomain_pos = new_hostname.find_last_of('.');
	int subcn_pos = new_cn.find_last_of('.');
	
	// if the last char is a '.', strip it
	if(subdomain_pos == (new_hostname.length()-1))
	{
		new_hostname = new_hostname.substr(0, subdomain_pos);
		subdomain_pos = new_hostname.find_last_of('.');
	}
	if(subcn_pos == (new_cn.length()-1))
	{
		new_cn = new_cn.substr(0, subcn_pos);
		subcn_pos = new_cn.find_last_of('.');
	}	

	// Check to see if there are any further '.' in the string.  
	while((subcn_pos != std::string::npos) && (subdomain_pos != std::string::npos))
	{
		// snip out last subdomain in both the match string and the hostname
		// The last bit for 'my.current.host.com' would be 'com'  
		std::string cn_part = new_cn.substr(subcn_pos+1, std::string::npos);
		std::string hostname_part = new_hostname.substr(subdomain_pos+1, std::string::npos);
		
		if(!_cert_subdomain_wildcard_match(new_hostname.substr(subdomain_pos+1, std::string::npos),
										   cn_part))
		{
			return FALSE;
		}
		new_hostname = new_hostname.substr(0, subdomain_pos);
		new_cn = new_cn.substr(0, subcn_pos);
		subdomain_pos = new_hostname.find_last_of('.');
		subcn_pos = new_cn.find_last_of('.');
	}	
	// check to see if the most significant portion of the common name is '*'.  If so, we can
	// simply return success as child domains are also matched.
	if(new_cn == "*")
	{
		// if it's just a '*' we support all child domains as well, so '*.
		return TRUE;
	}
	
	return _cert_subdomain_wildcard_match(new_hostname, new_cn);

}

// validate that the LLSD array in llsd_set contains the llsd_value 
bool _LLSDArrayIncludesValue(const LLSD& llsd_set, LLSD llsd_value)
{
	for(LLSD::array_const_iterator set_value = llsd_set.beginArray();
		set_value != llsd_set.endArray();
		set_value++)
	{
		if(valueCompareLLSD((*set_value), llsd_value))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void _validateCert(int validation_policy,
				  LLPointer<LLCertificate> cert,
				  const LLSD& validation_params,
				  int depth)
{
	LLSD current_cert_info;
	cert->getLLSD(current_cert_info);		
	// check basic properties exist in the cert
	if(!current_cert_info.has(CERT_SUBJECT_NAME) || !current_cert_info.has(CERT_SUBJECT_NAME_STRING))
	{
		LLTHROW(LLCertException(current_cert_info, "Cert doesn't have a Subject Name"));
	}
	
	if(!current_cert_info.has(CERT_ISSUER_NAME_STRING))
	{
		LLTHROW(LLCertException(current_cert_info, "Cert doesn't have an Issuer Name"));
	}
	
	// check basic properties exist in the cert
	if(!current_cert_info.has(CERT_VALID_FROM) || !current_cert_info.has(CERT_VALID_TO))
	{
		LLTHROW(LLCertException(current_cert_info, "Cert doesn't have an expiration period"));
	}
	if (!current_cert_info.has(CERT_SUBJECT_KEY_IDENTFIER))
	{
		LLTHROW(LLCertException(current_cert_info, "Cert doesn't have a Subject Key Id"));
	}

	if (validation_policy & VALIDATION_POLICY_TIME)
	{
		LLDate validation_date(time(NULL));
		if(validation_params.has(CERT_VALIDATION_DATE))
		{
			validation_date = validation_params[CERT_VALIDATION_DATE];
		}
		
		if((validation_date < current_cert_info[CERT_VALID_FROM].asDate()) ||
		   (validation_date > current_cert_info[CERT_VALID_TO].asDate()))
		{
			LLTHROW(LLCertValidationExpirationException(current_cert_info, validation_date));
		}
	}
	if (validation_policy & VALIDATION_POLICY_SSL_KU)
	{
		// This stanza of code was changed 2021-06-09 as per details in SL-15370.
		// Brief summary: a renewed certificate from Akamai only contains the
		// 'Digital Signature' field and not the 'Key Encipherment' one. This code 
		// used to look for both and throw an exception at startup (ignored) and 
		// (for example) when buying L$ in the Viewer (fails with a UI message
		// and an entry in the Viewer log). This modified code removes the second 
		// check for the 'Key Encipherment' field. If Akamai can provide a 
		// replacement certificate that has both fields, then this modified code 
		// will not be required.
		if (current_cert_info.has(CERT_KEY_USAGE) && current_cert_info[CERT_KEY_USAGE].isArray() &&
			!(_LLSDArrayIncludesValue(current_cert_info[CERT_KEY_USAGE], 
									   LLSD((std::string)CERT_KU_DIGITAL_SIGNATURE)))
			)
		{
			LLTHROW(LLCertKeyUsageValidationException(current_cert_info));
		}
		// only validate EKU if the cert has it
        if(current_cert_info.has(CERT_EXTENDED_KEY_USAGE)
           && current_cert_info[CERT_EXTENDED_KEY_USAGE].isArray()
           && (!_LLSDArrayIncludesValue(current_cert_info[CERT_EXTENDED_KEY_USAGE],
                                         LLSD((std::string)CERT_EKU_TLS_SERVER_AUTH)))
           && (!_LLSDArrayIncludesValue(current_cert_info[CERT_EXTENDED_KEY_USAGE], 
                                         LLSD((std::string)CERT_EKU_SERVER_AUTH)))
           )
		{
			LLTHROW(LLCertKeyUsageValidationException(current_cert_info));
		}
	}
	if (validation_policy & VALIDATION_POLICY_CA_KU)
	{
		if (current_cert_info.has(CERT_KEY_USAGE) && current_cert_info[CERT_KEY_USAGE].isArray() &&
			(!_LLSDArrayIncludesValue(current_cert_info[CERT_KEY_USAGE], 
									   (std::string)CERT_KU_CERT_SIGN)))
			{
				LLTHROW(LLCertKeyUsageValidationException(current_cert_info));
			}
	}
	
	// validate basic constraints
	if ((validation_policy & VALIDATION_POLICY_CA_BASIC_CONSTRAINTS) &&
		current_cert_info.has(CERT_BASIC_CONSTRAINTS) && 
		current_cert_info[CERT_BASIC_CONSTRAINTS].isMap())
	{
		if(!current_cert_info[CERT_BASIC_CONSTRAINTS].has(CERT_BASIC_CONSTRAINTS_CA) ||
		   !current_cert_info[CERT_BASIC_CONSTRAINTS][CERT_BASIC_CONSTRAINTS_CA])
		{
				LLTHROW(LLCertBasicConstraintsValidationException(current_cert_info));
		}
		if (current_cert_info[CERT_BASIC_CONSTRAINTS].has(CERT_BASIC_CONSTRAINTS_PATHLEN) &&
			((current_cert_info[CERT_BASIC_CONSTRAINTS][CERT_BASIC_CONSTRAINTS_PATHLEN].asInteger() != 0) &&
			 (depth > current_cert_info[CERT_BASIC_CONSTRAINTS][CERT_BASIC_CONSTRAINTS_PATHLEN].asInteger())))
		{
			LLTHROW(LLCertBasicConstraintsValidationException(current_cert_info));
		}
	}
}

bool _verify_signature(LLPointer<LLCertificate> parent, 
					   LLPointer<LLCertificate> child)
{
	bool verify_result = FALSE; 
	LLSD cert1, cert2;
	parent->getLLSD(cert1);
	child->getLLSD(cert2);
	X509 *signing_cert = parent->getOpenSSLX509();
	X509 *child_cert = child->getOpenSSLX509();
	if((signing_cert != NULL) && (child_cert != NULL))
	{
		EVP_PKEY *pkey = X509_get_pubkey(signing_cert);
		
		
		if(pkey)
		{
			int verify_code = X509_verify(child_cert, pkey);
			verify_result = ( verify_code > 0);
			EVP_PKEY_free(pkey);
		}
		else
		{
			LL_WARNS("SECAPI") << "Could not validate the cert chain signature, as the public key of the signing cert could not be retrieved" << LL_ENDL;
		}

	}
	else
	{
		LL_WARNS("SECAPI") << "Signature verification failed as there are no certs in the chain" << LL_ENDL;
	}
	if(child_cert)
	{
		X509_free(child_cert);
	}
	if(signing_cert)
	{
		X509_free(signing_cert);
	}
	return verify_result;
}


// validate the certificate chain against a store.
// There are many aspects of cert validatioin policy involved in
// trust validation.  The policies in this validation algorithm include
// * Hostname matching for SSL certs
// * Expiration time matching
// * Signature validation
// * Chain trust (is the cert chain trusted against the store)
// * Basic constraints
// * key usage and extended key usage
// TODO: We should add 'authority key identifier' for chaining.
// This algorithm doesn't simply validate the chain by itself
// and verify the last cert is in the certificate store, or points
// to a cert in the store.  It validates whether any cert in the chain
// is trusted in the store, even if it's not the last one.
void LLBasicCertificateStore::validate(int validation_policy,
									   LLPointer<LLCertificateChain> cert_chain,
									   const LLSD& validation_params)
{
	// If --no-verify-ssl-cert was passed on the command line, stop right now.
	if (gSavedSettings.getBOOL("NoVerifySSLCert"))
    {
        LL_WARNS_ONCE("SECAPI") << "All Certificate validation disabled; viewer operation is insecure" << LL_ENDL;
        return;
    }

	if(cert_chain->size() < 1)
	{
		LLTHROW(LLCertException(LLSD::emptyMap(), "No certs in chain"));
	}
	iterator current_cert = cert_chain->begin();
	LLSD validation_date;
	if (validation_params.has(CERT_VALIDATION_DATE))
	{
		validation_date = validation_params[CERT_VALIDATION_DATE];
	}

    // get LLSD info from the cert to throw in any exception
	LLSD 	current_cert_info;
    (*current_cert)->getLLSD(current_cert_info);

	if (validation_policy & VALIDATION_POLICY_HOSTNAME)
	{
		if(!validation_params.has(CERT_HOSTNAME))
		{
			LLTHROW(LLCertException(current_cert_info, "No hostname passed in for validation"));
		}
		if(!current_cert_info.has(CERT_SUBJECT_NAME) || !current_cert_info[CERT_SUBJECT_NAME].has(CERT_NAME_CN))
		{
			LLTHROW(LLInvalidCertificate(current_cert_info));
		}
		
		LL_DEBUGS("SECAPI") << "Validating the hostname " << validation_params[CERT_HOSTNAME].asString() << 
		     "against the cert CN " << current_cert_info[CERT_SUBJECT_NAME][CERT_NAME_CN].asString() << LL_ENDL;
		if(!_cert_hostname_wildcard_match(validation_params[CERT_HOSTNAME].asString(),
										  current_cert_info[CERT_SUBJECT_NAME][CERT_NAME_CN].asString()))
		{
			throw LLCertValidationHostnameException(validation_params[CERT_HOSTNAME].asString(),
													current_cert_info);
		}
	}
    
	// check the cache of already validated certs
	X509* cert_x509 = (*current_cert)->getOpenSSLX509();
	if(!cert_x509)
	{
		LLTHROW(LLInvalidCertificate(current_cert_info));
	}

    std::string subject_name(cert_string_name_from_X509_NAME(X509_get_subject_name(cert_x509)));
    std::string skeyid(_subject_key_identifier(cert_x509));

    LL_DEBUGS("SECAPI") << "attempting to validate cert "
                        << " for '" << (validation_params.has(CERT_HOSTNAME) ? validation_params[CERT_HOSTNAME].asString() : "(unknown hostname)") << "'"
                        << " as subject name '" << subject_name << "'"
                        << " subject key id '" << skeyid << "'"
                        << LL_ENDL;

	X509_free( cert_x509 );
	cert_x509 = NULL;
    if (skeyid.empty())
    {
        LLTHROW(LLCertException(current_cert_info, "No Subject Key Id"));
    }
    
	t_cert_cache::iterator cache_entry = mTrustedCertCache.find(skeyid);
	if(cache_entry != mTrustedCertCache.end())
	{
		// this cert is in the cache, so validate the time.
		if (validation_policy & VALIDATION_POLICY_TIME)
		{
			LLDate validation_date;
			if(validation_params.has(CERT_VALIDATION_DATE))
			{
				validation_date = validation_params[CERT_VALIDATION_DATE];
			}
            else
            {
                validation_date = LLDate(time(NULL)); // current time
            }
			
			if((validation_date < cache_entry->second.first) ||
			   (validation_date > cache_entry->second.second))
			{
				LLTHROW(LLCertValidationExpirationException(current_cert_info, validation_date));
			}
		}
		// successfully found in cache
		LL_DEBUGS("SECAPI") << "Valid cert for '" << validation_params[CERT_HOSTNAME].asString() << "'"
                            << " skeyid '" << skeyid << "'"
                            << " found in cache"
                            << LL_ENDL;
		return;
	}
	if(current_cert_info.isUndefined())
	{
		(*current_cert)->getLLSD(current_cert_info);
	}
	LLDate from_time = current_cert_info[CERT_VALID_FROM].asDate();
	LLDate to_time = current_cert_info[CERT_VALID_TO].asDate();
	int depth = 0;
	LLPointer<LLCertificate> previous_cert;
	// loop through the cert chain, validating the current cert against the next one.
	while(current_cert != cert_chain->end())
	{
		int local_validation_policy = validation_policy;
		if(current_cert == cert_chain->begin())
		{
			// for the child cert, we don't validate CA stuff
			local_validation_policy &= ~(VALIDATION_POLICY_CA_KU | 
										 VALIDATION_POLICY_CA_BASIC_CONSTRAINTS);
		}
		else
		{
			// for non-child certs, we don't validate SSL Key usage
			local_validation_policy &= ~VALIDATION_POLICY_SSL_KU;				
			if(!_verify_signature((*current_cert),
								  previous_cert))
			{
                LLSD previous_cert_info;
                previous_cert->getLLSD(previous_cert_info);
                LLTHROW(LLCertValidationInvalidSignatureException(previous_cert_info));
			}
		}
		_validateCert(local_validation_policy,
					  (*current_cert),
					  validation_params,
					  depth);
		
		// look for a CA in the CA store that may belong to this chain.
		LLSD cert_search_params = LLSD::emptyMap();		
		// is the cert itself in the store?
		cert_search_params[CERT_SUBJECT_KEY_IDENTFIER] = current_cert_info[CERT_SUBJECT_KEY_IDENTFIER];
		LLCertificateStore::iterator found_store_cert = find(cert_search_params);
		if(found_store_cert != end())
		{
			mTrustedCertCache[skeyid] = std::pair<LLDate, LLDate>(from_time, to_time);
            LL_DEBUGS("SECAPI") << "Valid cert "
                                << " for '" << (validation_params.has(CERT_HOSTNAME) ? validation_params[CERT_HOSTNAME].asString() : "(unknown hostname)") << "'";
            X509* cert_x509 = (*found_store_cert)->getOpenSSLX509();
            std::string found_cert_subject_name(cert_string_name_from_X509_NAME(X509_get_subject_name(cert_x509)));
            X509_free(cert_x509);
            LL_CONT << " as '" << found_cert_subject_name << "'"
                    << " skeyid '" << current_cert_info[CERT_SUBJECT_KEY_IDENTFIER].asString() << "'"
                    << " found in cert store"
                    << LL_ENDL;	
			return;
		}
		
		// is the parent in the cert store?
			
		cert_search_params = LLSD::emptyMap();
		cert_search_params[CERT_SUBJECT_NAME_STRING] = current_cert_info[CERT_ISSUER_NAME_STRING];
		if (current_cert_info.has(CERT_AUTHORITY_KEY_IDENTIFIER))
		{
			LLSD cert_aki = current_cert_info[CERT_AUTHORITY_KEY_IDENTIFIER];
			if(cert_aki.has(CERT_AUTHORITY_KEY_IDENTIFIER_ID))
			{
				cert_search_params[CERT_SUBJECT_KEY_IDENTFIER] = cert_aki[CERT_AUTHORITY_KEY_IDENTIFIER_ID];
			}
			if(cert_aki.has(CERT_AUTHORITY_KEY_IDENTIFIER_SERIAL))
			{
				cert_search_params[CERT_SERIAL_NUMBER] = cert_aki[CERT_AUTHORITY_KEY_IDENTIFIER_SERIAL];
			}
		}
		found_store_cert = find(cert_search_params);
		
		if(found_store_cert != end())
		{
			// validate the store cert against the depth
			_validateCert(validation_policy & VALIDATION_POLICY_CA_BASIC_CONSTRAINTS,
						  (*found_store_cert),
						  LLSD(),
						  depth);
			
			// verify the signature of the CA
			if(!_verify_signature((*found_store_cert),
								  (*current_cert)))
			{
				LLTHROW(LLCertValidationInvalidSignatureException(current_cert_info));
			}			
			// successfully validated.
			mTrustedCertCache[skeyid] = std::pair<LLDate, LLDate>(from_time, to_time);		
            LL_DEBUGS("SECAPI") << "Verified and cached cert for '" << validation_params[CERT_HOSTNAME].asString() << "'"
                                << " as '" << subject_name << "'"
                                << " id '" << skeyid << "'"
                                << " using CA '" << cert_search_params[CERT_SUBJECT_NAME_STRING] << "'"
                                << " with id '" <<  cert_search_params[CERT_SUBJECT_KEY_IDENTFIER].asString() << "' found in cert store"
                                << LL_ENDL;	
			return;
		}
		previous_cert = (*current_cert);
		current_cert++;
		depth++;
		if(current_cert != cert_chain->end())
		{
			(*current_cert)->getLLSD(current_cert_info);
		}
	}
	if (validation_policy & VALIDATION_POLICY_TRUSTED)
	{
		// we reached the end without finding a trusted cert.
        LLSD last_cert_info;
        ((*cert_chain)[cert_chain->size()-1])->getLLSD(last_cert_info);
		LLTHROW(LLCertValidationTrustException(last_cert_info));
	}
    else
    {
        LL_DEBUGS("SECAPI") << "! Caching untrusted cert for '" << subject_name << "'"
                            << " skeyid '" << skeyid << "' in cert store because ! VALIDATION_POLICY_TRUSTED"
                            << LL_ENDL;	
        mTrustedCertCache[skeyid] = std::pair<LLDate, LLDate>(from_time, to_time);	
    }
}


// LLSecAPIBasicHandler Class
// Interface handler class for the various security storage handlers.

// We read the file on construction, and write it on destruction.  This
// means multiple processes cannot modify the datastore.
LLSecAPIBasicHandler::LLSecAPIBasicHandler(const std::string& protected_data_file,
										   const std::string& legacy_password_path)
{
	mProtectedDataFilename = protected_data_file;
	mProtectedDataMap = LLSD::emptyMap();
	mLegacyPasswordPath = legacy_password_path;

}

LLSecAPIBasicHandler::LLSecAPIBasicHandler()
{
}


void LLSecAPIBasicHandler::init()
{
	mProtectedDataMap = LLSD::emptyMap();
	if (mProtectedDataFilename.length() == 0)
	{
		mProtectedDataFilename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
															"bin_conf.dat");
		mLegacyPasswordPath = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS, "password.dat");
	
		mProtectedDataFilename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
															"bin_conf.dat");	
		std::string store_file = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
														"CA.pem");
		
		
		LL_INFOS("SECAPI") << "Loading user certificate store from " << store_file << LL_ENDL;
		mStore = new LLBasicCertificateStore(store_file);
		
		// grab the application ca-bundle.crt file that contains the well-known certs shipped
		// with the product
		std::string ca_file_path = gDirUtilp->getCAFile();
		LL_INFOS("SECAPI") << "Loading application certificate store from " << ca_file_path << LL_ENDL;
		LLPointer<LLBasicCertificateStore> app_ca_store = new LLBasicCertificateStore(ca_file_path);
		
		// push the applicate CA files into the store, therefore adding any new CA certs that 
		// updated
		for(LLCertificateVector::iterator i = app_ca_store->begin();
			i != app_ca_store->end();
			i++)
		{
			mStore->add(*i);
		}
		
	}
	_readProtectedData(); // initialize mProtectedDataMap
						  // may throw LLProtectedDataException if saved datamap is not decryptable
}
LLSecAPIBasicHandler::~LLSecAPIBasicHandler()
{
	_writeProtectedData();
}

void LLSecAPIBasicHandler::_readProtectedData(unsigned char *unique_id, U32 id_len)
{
	// attempt to load the file into our map
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	llifstream protected_data_stream(mProtectedDataFilename.c_str(), 
									llifstream::binary);

	if (!protected_data_stream.fail()) {
		U8 salt[STORE_SALT_SIZE];
		U8 buffer[BUFFER_READ_SIZE];
		U8 decrypted_buffer[BUFFER_READ_SIZE];
		int decrypted_length;	
		LLXORCipher cipher(unique_id, id_len);

		// read in the salt and key
		protected_data_stream.read((char *)salt, STORE_SALT_SIZE);
		if (protected_data_stream.gcount() < STORE_SALT_SIZE)
		{
			LLTHROW(LLProtectedDataException("Config file too short."));
		}

		cipher.decrypt(salt, STORE_SALT_SIZE);		

		// totally lame.  As we're not using the OS level protected data, we need to
		// at least obfuscate the data.  We do this by using a salt stored at the head of the file
		// to encrypt the data, therefore obfuscating it from someone using simple existing tools.
		// We do include the MAC address as part of the obfuscation, which would require an
		// attacker to get the MAC address as well as the protected store, which improves things
		// somewhat.  It would be better to use the password, but as this store
		// will be used to store the SL password when the user decides to have SL remember it, 
		// so we can't use that.  OS-dependent store implementations will use the OS password/storage 
		// mechanisms and are considered to be more secure.
		// We've a strong intent to move to OS dependent protected data stores.
		

		// read in the rest of the file.
		EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        // todo: ctx error handling

		EVP_DecryptInit(ctx, EVP_rc4(), salt, NULL);
		// allocate memory:
		std::string decrypted_data;	
		
		while(protected_data_stream.good()) {
			// read data as a block:
			protected_data_stream.read((char *)buffer, BUFFER_READ_SIZE);
			
			EVP_DecryptUpdate(ctx, decrypted_buffer, &decrypted_length, 
							  buffer, protected_data_stream.gcount());
			decrypted_data.append((const char *)decrypted_buffer, protected_data_stream.gcount());
		}
		
		// RC4 is a stream cipher, so we don't bother to EVP_DecryptFinal, as there is
		// no block padding.
        EVP_CIPHER_CTX_free(ctx);
		std::istringstream parse_stream(decrypted_data);
		if (parser->parse(parse_stream, mProtectedDataMap, 
						  LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
		{
			LLTHROW(LLProtectedDataException("Config file cannot be decrypted."));
		}
	}
}

void LLSecAPIBasicHandler::_readProtectedData()
{
    unsigned char unique_id[MAC_ADDRESS_BYTES];
    try
    {
        // try default id
        LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
        _readProtectedData(unique_id, sizeof(unique_id));
    }
    catch(LLProtectedDataException&)
    {
        // try with legacy id, it will return false if it is identical to getUniqueID
        // or if it is not assigned/not in use
        if (LLMachineID::getLegacyID(unique_id, sizeof(unique_id)))
        {
            _readProtectedData(unique_id, sizeof(unique_id));
        }
        else
        {
            throw;
        }
    }
}

void LLSecAPIBasicHandler::_writeProtectedData()
{	
	std::ostringstream formatted_data_ostream;
	U8 salt[STORE_SALT_SIZE];
	U8 buffer[BUFFER_READ_SIZE];
	U8 encrypted_buffer[BUFFER_READ_SIZE];

	
	if(mProtectedDataMap.isUndefined())
	{
		LLFile::remove(mProtectedDataFilename);
		return;
	}
	// create a string with the formatted data.
	LLSDSerialize::toXML(mProtectedDataMap, formatted_data_ostream);
	std::istringstream formatted_data_istream(formatted_data_ostream.str());
	// generate the seed
	RAND_bytes(salt, STORE_SALT_SIZE);

	
	// write to a temp file so we don't clobber the initial file if there is
	// an error.
	std::string tmp_filename = mProtectedDataFilename + ".tmp";
	
	llofstream protected_data_stream(tmp_filename.c_str(), 
                                     std::ios_base::binary);
    EVP_CIPHER_CTX *ctx = NULL;
	try
	{
		
		ctx = EVP_CIPHER_CTX_new();
        // todo: ctx error handling

		EVP_EncryptInit(ctx, EVP_rc4(), salt, NULL);
		unsigned char unique_id[MAC_ADDRESS_BYTES];
        LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
		LLXORCipher cipher(unique_id, sizeof(unique_id));
		cipher.encrypt(salt, STORE_SALT_SIZE);
		protected_data_stream.write((const char *)salt, STORE_SALT_SIZE);

		while (formatted_data_istream.good())
		{
			formatted_data_istream.read((char *)buffer, BUFFER_READ_SIZE);
			if(formatted_data_istream.gcount() == 0)
			{
				break;
			}
			int encrypted_length;
			EVP_EncryptUpdate(ctx, encrypted_buffer, &encrypted_length, 
						  buffer, formatted_data_istream.gcount());
			protected_data_stream.write((const char *)encrypted_buffer, encrypted_length);
		}
		
		// no EVP_EncrypteFinal, as this is a stream cipher
        EVP_CIPHER_CTX_free(ctx);

		protected_data_stream.close();
	}
	catch (...)
	{
		LOG_UNHANDLED_EXCEPTION("LLProtectedDataException(Error writing Protected Data Store)");
		// it's good practice to clean up any secure information on error
		// (even though this file isn't really secure.  Perhaps in the future
		// it may be, however.
		LLFile::remove(tmp_filename);

        if (ctx)
        {
            EVP_CIPHER_CTX_free(ctx);
        }

		// EXP-1825 crash in LLSecAPIBasicHandler::_writeProtectedData()
		// Decided throwing an exception here was overkill until we figure out why this happens
		//LLTHROW(LLProtectedDataException("Error writing Protected Data Store"));
	}

	try
	{
		// move the temporary file to the specified file location.
		if(((	(LLFile::isfile(mProtectedDataFilename) != 0)
			 && (LLFile::remove(mProtectedDataFilename) != 0)))
		   || (LLFile::rename(tmp_filename, mProtectedDataFilename)))
		{
			LL_WARNS() << "LLProtectedDataException(Could not overwrite protected data store)" << LL_ENDL;
			LLFile::remove(tmp_filename);

			// EXP-1825 crash in LLSecAPIBasicHandler::_writeProtectedData()
			// Decided throwing an exception here was overkill until we figure out why this happens
			//LLTHROW(LLProtectedDataException("Could not overwrite protected data store"));
		}
	}
	catch (...)
	{
		LOG_UNHANDLED_EXCEPTION(STRINGIZE("renaming '" << tmp_filename << "' to '"
										  << mProtectedDataFilename << "'"));
		// it's good practice to clean up any secure information on error
		// (even though this file isn't really secure.  Perhaps in the future
		// it may be, however).
		LLFile::remove(tmp_filename);

		//crash in LLSecAPIBasicHandler::_writeProtectedData()
		// Decided throwing an exception here was overkill until we figure out why this happens
		//LLTHROW(LLProtectedDataException("Error writing Protected Data Store"));
	}
}

// instantiate a certificate from a pem string
LLPointer<LLCertificate> LLSecAPIBasicHandler::getCertificate(const std::string& pem_cert)
{
	LLPointer<LLCertificate> result = new LLBasicCertificate(pem_cert);
	return result;
}
		

		
// instiate a certificate from an openssl X509 structure
LLPointer<LLCertificate> LLSecAPIBasicHandler::getCertificate(X509* openssl_cert)
{
	LLPointer<LLCertificate> result = new LLBasicCertificate(openssl_cert);
	return result;		
}
		
// instantiate a chain from an X509_STORE_CTX
LLPointer<LLCertificateChain> LLSecAPIBasicHandler::getCertificateChain(X509_STORE_CTX* chain)
{
	LLPointer<LLCertificateChain> result = new LLBasicCertificateChain(chain);
	return result;
}
		
// instantiate a cert store given it's id.  if a persisted version
// exists, it'll be loaded.  If not, one will be created (but not
// persisted)
LLPointer<LLCertificateStore> LLSecAPIBasicHandler::getCertificateStore(const std::string& store_id)
{
	return mStore;
}
		
// retrieve protected data
LLSD LLSecAPIBasicHandler::getProtectedData(const std::string& data_type,
											const std::string& data_id)
{

	if (mProtectedDataMap.has(data_type) && 
		mProtectedDataMap[data_type].isMap() && 
		mProtectedDataMap[data_type].has(data_id))
	{
		return mProtectedDataMap[data_type][data_id];
	}
																				
	return LLSD();
}

void LLSecAPIBasicHandler::deleteProtectedData(const std::string& data_type,
											   const std::string& data_id)
{
	if (mProtectedDataMap.has(data_type) &&
		mProtectedDataMap[data_type].isMap() &&
		mProtectedDataMap[data_type].has(data_id))
		{
			mProtectedDataMap[data_type].erase(data_id);
		}
}


//
// persist data in a protected store
//
void LLSecAPIBasicHandler::setProtectedData(const std::string& data_type,
											const std::string& data_id,
											const LLSD& data)
{
	if (!mProtectedDataMap.has(data_type) || !mProtectedDataMap[data_type].isMap()) {
		mProtectedDataMap[data_type] = LLSD::emptyMap();
	}
	
	mProtectedDataMap[data_type][data_id] = data; 
}

// persist data in a protected store's map
void LLSecAPIBasicHandler::addToProtectedMap(const std::string& data_type,
											 const std::string& data_id,
											 const std::string& map_elem,
											 const LLSD& data)
{
    if (!mProtectedDataMap.has(data_type) || !mProtectedDataMap[data_type].isMap()) {
        mProtectedDataMap[data_type] = LLSD::emptyMap();
    }

    if (!mProtectedDataMap[data_type].has(data_id) || !mProtectedDataMap[data_type][data_id].isMap()) {
        mProtectedDataMap[data_type][data_id] = LLSD::emptyMap();
    }

    mProtectedDataMap[data_type][data_id][map_elem] = data;
}

// remove data from protected store's map
void LLSecAPIBasicHandler::removeFromProtectedMap(const std::string& data_type,
												  const std::string& data_id,
												  const std::string& map_elem)
{
    if (mProtectedDataMap.has(data_type) &&
        mProtectedDataMap[data_type].isMap() &&
        mProtectedDataMap[data_type].has(data_id) &&
        mProtectedDataMap[data_type][data_id].isMap() &&
        mProtectedDataMap[data_type][data_id].has(map_elem))
    {
        mProtectedDataMap[data_type][data_id].erase(map_elem);
    }
}

void LLSecAPIBasicHandler::syncProtectedMap()
{
    // TODO - consider unifing these functions
    _writeProtectedData();
}
//
// Create a credential object from an identifier and authenticator.  credentials are
// per grid.
LLPointer<LLCredential> LLSecAPIBasicHandler::createCredential(const std::string& grid,
															   const LLSD& identifier, 
															   const LLSD& authenticator)
{
	LLPointer<LLSecAPIBasicCredential> result = new LLSecAPIBasicCredential(grid);
	result->setCredentialData(identifier, authenticator);
	return result;
}

// Load a credential from default credential store, given the grid
LLPointer<LLCredential> LLSecAPIBasicHandler::loadCredential(const std::string& grid)
{
	LLSD credential = getProtectedData(DEFAULT_CREDENTIAL_STORAGE, grid);
	LLPointer<LLSecAPIBasicCredential> result = new LLSecAPIBasicCredential(grid);
	if(credential.isMap() && 
	   credential.has("identifier"))
	{

		LLSD identifier = credential["identifier"];
		LLSD authenticator;
		if (credential.has("authenticator"))
		{
			authenticator = credential["authenticator"];
		}
		result->setCredentialData(identifier, authenticator);
	}
	else
	{
		// credential was not in protected storage, so pull the credential
		// from the legacy store.
		std::string first_name = gSavedSettings.getString("FirstName");
		std::string last_name = gSavedSettings.getString("LastName");
		
		if ((first_name != "") &&
			(last_name != ""))
		{
			LLSD identifier = LLSD::emptyMap();
			LLSD authenticator;
			identifier["type"] = "agent";
			identifier["first_name"] = first_name;
			identifier["last_name"] = last_name;
			
			std::string legacy_password = _legacyLoadPassword();
			if (legacy_password.length() > 0)
			{
				authenticator = LLSD::emptyMap();
				authenticator["type"] = "hash";
				authenticator["algorithm"] = "md5";
				authenticator["secret"] = legacy_password;
			}
			result->setCredentialData(identifier, authenticator);
		}		
	}
	return result;
}

// Save the credential to the credential store.  Save the authenticator also if requested.
// That feature is used to implement the 'remember password' functionality.
void LLSecAPIBasicHandler::saveCredential(LLPointer<LLCredential> cred, bool save_authenticator)
{
	LLSD credential = LLSD::emptyMap();
	credential["identifier"] = cred->getIdentifier(); 
	if (save_authenticator) 
	{
		credential["authenticator"] = cred->getAuthenticator();
	}
	LL_DEBUGS("SECAPI") << "Saving Credential " << cred->getGrid() << ":" << cred->userID() << " " << save_authenticator << LL_ENDL;
	setProtectedData(DEFAULT_CREDENTIAL_STORAGE, cred->getGrid(), credential);
	//*TODO: If we're saving Agni credentials, should we write the
	// credentials to the legacy password.dat/etc?
	_writeProtectedData();
}

// Remove a credential from the credential store.
void LLSecAPIBasicHandler::deleteCredential(LLPointer<LLCredential> cred)
{
	LLSD undefVal;
	deleteProtectedData(DEFAULT_CREDENTIAL_STORAGE, cred->getGrid());
	cred->setCredentialData(undefVal, undefVal);
	_writeProtectedData();
}

// has map of credentials declared as specific storage
bool LLSecAPIBasicHandler::hasCredentialMap(const std::string& storage, const std::string& grid)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLSD credential = getProtectedData(storage, grid);

    return credential.isMap();
}

// returns true if map is empty or does not exist
bool LLSecAPIBasicHandler::emptyCredentialMap(const std::string& storage, const std::string& grid)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLSD credential = getProtectedData(storage, grid);

    return !credential.isMap() || credential.size() == 0;
}

// Load map of credentials from specified credential store, given the grid
void LLSecAPIBasicHandler::loadCredentialMap(const std::string& storage, const std::string& grid, credential_map_t& credential_map)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLSD credential = getProtectedData(storage, grid);
    if (credential.isMap())
    {
        LLSD::map_const_iterator crd_it = credential.beginMap();
        for (; crd_it != credential.endMap(); crd_it++)
        {
            LLSD::String name = crd_it->first;
            const LLSD &link_map = crd_it->second;
            LLPointer<LLSecAPIBasicCredential> result = new LLSecAPIBasicCredential(grid);
            if (link_map.has("identifier"))
            {
                LLSD identifier = link_map["identifier"];
                LLSD authenticator;
                if (link_map.has("authenticator"))
                {
                    authenticator = link_map["authenticator"];
                }
                result->setCredentialData(identifier, authenticator);
            }
            credential_map[name] = result;
        }
    }
}

LLPointer<LLCredential> LLSecAPIBasicHandler::loadFromCredentialMap(const std::string& storage, const std::string& grid, const std::string& userkey)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLPointer<LLSecAPIBasicCredential> result = new LLSecAPIBasicCredential(grid);

    LLSD credential = getProtectedData(storage, grid);
    if (credential.isMap() && credential.has(userkey) && credential[userkey].has("identifier"))
    {
        LLSD identifier = credential[userkey]["identifier"];
        LLSD authenticator;
        if (credential[userkey].has("authenticator"))
        {
            authenticator = credential[userkey]["authenticator"];
        }
        result->setCredentialData(identifier, authenticator);
    }

    return result;
}

// add item to map of credentials from specific storage
void LLSecAPIBasicHandler::addToCredentialMap(const std::string& storage, LLPointer<LLCredential> cred, bool save_authenticator)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    std::string user_id = cred->userID();
    LLSD credential = LLSD::emptyMap();
    credential["identifier"] = cred->getIdentifier();
    if (save_authenticator)
    {
        credential["authenticator"] = cred->getAuthenticator();
    }
    LL_DEBUGS("SECAPI") << "Saving Credential " << cred->getGrid() << ":" << cred->userID() << " " << save_authenticator << LL_ENDL;
    addToProtectedMap(storage, cred->getGrid(), user_id, credential);

    _writeProtectedData();
}

// remove item from map of credentials from specific storage
void LLSecAPIBasicHandler::removeFromCredentialMap(const std::string& storage, LLPointer<LLCredential> cred)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLSD undefVal;
    removeFromProtectedMap(storage, cred->getGrid(), cred->userID());
    cred->setCredentialData(undefVal, undefVal);
    _writeProtectedData();
}

// remove item from map of credentials from specific storage
void LLSecAPIBasicHandler::removeFromCredentialMap(const std::string& storage, const std::string& grid, const std::string& userkey)
{
    if (storage == DEFAULT_CREDENTIAL_STORAGE)
    {
        LL_ERRS() << "Storing maps in default, single-items storage is not allowed" << LL_ENDL;
    }

    LLSD undefVal;
    LLPointer<LLCredential> cred = loadFromCredentialMap(storage, grid, userkey);
    removeFromProtectedMap(storage, grid, userkey);
    cred->setCredentialData(undefVal, undefVal);
    _writeProtectedData();
}

// remove item from map of credentials from specific storage
void LLSecAPIBasicHandler::removeCredentialMap(const std::string& storage, const std::string& grid)
{
    deleteProtectedData(storage, grid);
    _writeProtectedData();
}

// load the legacy hash for agni, and decrypt it given the 
// mac address
std::string LLSecAPIBasicHandler::_legacyLoadPassword()
{
	const S32 HASHED_LENGTH = 32;	
	std::vector<U8> buffer(HASHED_LENGTH);
	llifstream password_file(mLegacyPasswordPath.c_str(), llifstream::binary);
	
	if(password_file.fail())
	{
		return std::string("");
	}
	
	password_file.read((char*)&buffer[0], buffer.size());
	if(password_file.gcount() != buffer.size())
	{
		return std::string("");
	}
	
	// Decipher with MAC address
	unsigned char unique_id[MAC_ADDRESS_BYTES];
    LLMachineID::getUniqueID(unique_id, sizeof(unique_id));
	LLXORCipher cipher(unique_id, sizeof(unique_id));
	cipher.decrypt(&buffer[0], buffer.size());
	
	return std::string((const char*)&buffer[0], buffer.size());
}


// return an identifier for the user
std::string LLSecAPIBasicCredential::userID() const
{
	if (!mIdentifier.isMap())
	{
		return mGrid + "(null)";
	}
	else if ((std::string)mIdentifier["type"] == "agent")
	{
		std::string id = (std::string)mIdentifier["first_name"] + "_" + (std::string)mIdentifier["last_name"];
		LLStringUtil::toLower(id);
		return id;
	}
	else if ((std::string)mIdentifier["type"] == "account")
	{
		std::string id = (std::string)mIdentifier["account_name"];
		LLStringUtil::toLower(id);
		return id;
	}

	return "unknown";
}

// return a printable user identifier
std::string LLSecAPIBasicCredential::asString() const
{
	if (!mIdentifier.isMap())
	{
		return mGrid + ":(null)";
	}
	else if ((std::string)mIdentifier["type"] == "agent")
	{
		return mGrid + ":" + (std::string)mIdentifier["first_name"] + " " + (std::string)mIdentifier["last_name"];
	}
	else if ((std::string)mIdentifier["type"] == "account")
	{
		return mGrid + ":" + (std::string)mIdentifier["account_name"];
	}

	return mGrid + ":(unknown type)";
}


bool valueCompareLLSD(const LLSD& lhs, const LLSD& rhs)
{
	if (lhs.type() != rhs.type())
	{
		return FALSE;
	}
    if (lhs.isMap())
	{
		// iterate through the map, verifying the right hand side has all of the
		// values that the left hand side has.
		for (LLSD::map_const_iterator litt = lhs.beginMap();
			 litt != lhs.endMap();
			 litt++)
		{
			if (!rhs.has(litt->first))
			{
				return FALSE;
			}
		}
		
		// Now validate that the left hand side has everything the
		// right hand side has, and that the values are equal.
		for (LLSD::map_const_iterator ritt = rhs.beginMap();
			 ritt != rhs.endMap();
			 ritt++)
		{
			if (!lhs.has(ritt->first))
			{
				return FALSE;
			}
			if (!valueCompareLLSD(lhs[ritt->first], ritt->second))
			{
				return FALSE;
			}
		}
		return TRUE;
	}
    else if (lhs.isArray())
	{
		LLSD::array_const_iterator ritt = rhs.beginArray();
		// iterate through the array, comparing
		for (LLSD::array_const_iterator litt = lhs.beginArray();
			 litt != lhs.endArray();
			 litt++)
		{
			if (!valueCompareLLSD(*ritt, *litt))
			{
				return FALSE;
			}
			ritt++;
		}
		
		return (ritt == rhs.endArray());
	}
    else
	{
		// simple type, compare as string
		return (lhs.asString() == rhs.asString());
	}
	
}
