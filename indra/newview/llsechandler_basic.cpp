/** 
 * @file llsechandler_basic.cpp
 * @brief Security API for services such as certificate handling
 * secure local storage, etc.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2000, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h"
#include "llsecapi.h"
#include "llsechandler_basic.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "lldir.h"
#include <vector>
#include <ios>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/rand.h>
#include <iostream>
#include <iomanip>
#include <time.h>

// 128 bits of salt data...
#define STORE_SALT_SIZE 16 
#define BUFFER_READ_SIZE 256


LLBasicCertificate::LLBasicCertificate(const std::string& pem_cert) 
{
	
	// BIO_new_mem_buf returns a read only bio, but takes a void* which isn't const
	// so we need to cast it.
	BIO * pem_bio = BIO_new_mem_buf((void*)pem_cert.c_str(), pem_cert.length());
	
	mCert = NULL;
	PEM_read_bio_X509(pem_bio, &mCert, 0, NULL);
	BIO_free(pem_bio);
	if (!mCert)
	{
		throw LLCertException("Error parsing certificate");
	}
}


LLBasicCertificate::LLBasicCertificate(X509* pCert) 
{
	if (!pCert || !pCert->cert_info)
	{
		throw LLCertException("Invalid certificate");
	}	
	mCert = X509_dup(pCert);
}

LLBasicCertificate::~LLBasicCertificate() 
{

	X509_free(mCert);
}

//
// retrieve the pem using the openssl functionality
std::string LLBasicCertificate::getPem()
{ 
	char * pem_bio_chars = NULL;
	// a BIO is the equivalent of a 'std::stream', and
	// can be a file, mem stream, whatever.  Grab a memory based
	// BIO for the result
	BIO *pem_bio = BIO_new(BIO_s_mem());
	if (!pem_bio)
	{
		throw LLCertException("couldn't allocate memory buffer");		
	}
	PEM_write_bio_X509(pem_bio, mCert);
	int length = BIO_get_mem_data(pem_bio, &pem_bio_chars);
	std::string result = std::string(pem_bio_chars, length);
	BIO_free(pem_bio);
	return result;
}

// get the DER encoding for the cert
// DER is a binary encoding format for certs...
std::vector<U8> LLBasicCertificate::getBinary()
{ 
	U8 * der_bio_data = NULL;
	// get a memory bio 
	BIO *der_bio = BIO_new(BIO_s_mem());
	if (!der_bio)
	{
		throw LLCertException("couldn't allocate memory buffer");		
	}
	i2d_X509_bio(der_bio, mCert);
	int length = BIO_get_mem_data(der_bio, &der_bio_data);
	std::vector<U8> result(length);
	// vectors are guranteed to be a contiguous chunk of memory.
	memcpy(&result[0], der_bio_data,  length);
	BIO_free(der_bio);
	return result;
}


LLSD LLBasicCertificate::getLLSD()
{ 
	LLSD result;

	// call the various helpers to build the LLSD
	result[CERT_SUBJECT_NAME] = cert_name_from_X509_NAME(X509_get_subject_name(mCert));
	result[CERT_ISSUER_NAME] = cert_name_from_X509_NAME(X509_get_issuer_name(mCert));
	result[CERT_SUBJECT_NAME_STRING] = cert_string_name_from_X509_NAME(X509_get_subject_name(mCert));
	result[CERT_ISSUER_NAME_STRING] = cert_string_name_from_X509_NAME(X509_get_issuer_name(mCert));
	ASN1_INTEGER *sn = X509_get_serialNumber(mCert);
	if (sn != NULL)
	{
		result[CERT_SERIAL_NUMBER] = cert_string_from_asn1_integer(sn);
	}
	
	result[CERT_VALID_TO] = cert_date_from_asn1_time(X509_get_notAfter(mCert));
	result[CERT_VALID_FROM] = cert_date_from_asn1_time(X509_get_notBefore(mCert));
	result[CERT_SHA1_DIGEST] = cert_get_digest("sha1", mCert);
	result[CERT_MD5_DIGEST] = cert_get_digest("md5", mCert);



	return result; 
}

X509* LLBasicCertificate::getOpenSSLX509()
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
		
		std::string name_value = std::string((const char*)M_ASN1_STRING_data(X509_NAME_ENTRY_get_data(entry)), 
											 M_ASN1_STRING_length(X509_NAME_ENTRY_get_data(entry)));

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
	BIGNUM *bn = ASN1_INTEGER_to_BN(value, NULL);
	char * ascii_bn = BN_bn2hex(bn);

	BN_free(bn);

	std::string result(ascii_bn);
	OPENSSL_free(ascii_bn);
	return result;
}

// retrieve a date structure from an ASN1 time, for 
// validity checking.
LLDate cert_date_from_asn1_time(ASN1_TIME* asn1_time)
{
	
	struct tm timestruct = {0};
	int i = asn1_time->length;
	
	if (i < 10)
		throw LLCertException("invalid certificate time value");
	
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
	
	return LLDate((F64)mktime(&timestruct));
}

// Generate a string containing a digest.  The digest time is 'ssh1' or
// 'md5', and the resulting string is of the form "aa:12:5c:' and so on
std::string cert_get_digest(const std::string& digest_type, X509 *cert)
{
	unsigned char digest_data[1024];
	unsigned int len = 1024;
	std::stringstream result;
	const EVP_MD* digest = NULL;
	OpenSSL_add_all_digests();
	// we could use EVP_get_digestbyname, but that requires initializer code which
	// would require us to complicate things by plumbing it into the system.
	if (digest_type == "md5")
	{
		digest = EVP_md5();
	}
	else if (digest_type == "sha1")
	{
		digest = EVP_sha1();
	}
	else
	{
		throw LLCertException("Invalid digest");
	}

	X509_digest(cert, digest, digest_data, &len);
	result << std::hex << std::setprecision(2);
	for (unsigned int i=0; i < len; i++)
	{
		if (i != 0) 
		{
			result << ":";
		}
		result  << std::setfill('0') << std::setw(2) << (int)digest_data[i];
	}
	return result.str();
}


// LLSecAPIBasicHandler Class
// Interface handler class for the various security storage handlers.

// We read the file on construction, and write it on destruction.  This
// means multiple processes cannot modify the datastore.
LLSecAPIBasicHandler::LLSecAPIBasicHandler(const std::string& protected_data_file)
{
	mProtectedDataFilename = protected_data_file;
	mProtectedDataMap = LLSD::emptyMap();
	_readProtectedData();
}

LLSecAPIBasicHandler::LLSecAPIBasicHandler()
{
	std::string mProtectedDataFilename = gDirUtilp->getExpandedFilename(LL_PATH_USER_SETTINGS,
														  "bin_conf.dat");

	mProtectedDataMap = LLSD::emptyMap();
	_readProtectedData();
}

LLSecAPIBasicHandler::~LLSecAPIBasicHandler()
{
	_writeProtectedData();
}

void LLSecAPIBasicHandler::_readProtectedData()
{	
	// attempt to load the file into our map
	LLPointer<LLSDParser> parser = new LLSDXMLParser();
	
	llifstream protected_data_stream(mProtectedDataFilename.c_str(), 
									llifstream::binary);
	if (!protected_data_stream.fail()) {
		int offset;
		U8 salt[STORE_SALT_SIZE];
		U8 buffer[BUFFER_READ_SIZE];
		U8 decrypted_buffer[BUFFER_READ_SIZE];
		int decrypted_length;		
		

		// read in the salt and key
		protected_data_stream.read((char *)salt, STORE_SALT_SIZE);
		offset = 0;
		if (protected_data_stream.gcount() < STORE_SALT_SIZE)
		{
			throw LLProtectedDataException("Corrupt Protected Data Store");
		}
		
		// totally lame.  As we're not using the OS level protected data, we need to
		// at least obfuscate the data.  We do this by using a salt stored at the head of the file
		// to encrypt the data, therefore obfuscating it from someone using simple existing tools.
		// It would be better to use the password, but as this store
		// will be used to store the SL password when the user decides to have SL remember it, 
		// so we can't use that.  OS-dependent store implementations will use the OS password/storage 
		// mechanisms and are considered to be more secure.
		// We've a strong intent to move to OS dependent protected data stores.
		

		// read in the rest of the file.
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
		EVP_DecryptInit(&ctx, EVP_rc4(), salt, NULL);
		// allocate memory:
		std::string decrypted_data;	
		
		while(protected_data_stream.good()) {
			// read data as a block:
			protected_data_stream.read((char *)buffer, BUFFER_READ_SIZE);
			
			EVP_DecryptUpdate(&ctx, decrypted_buffer, &decrypted_length, 
							  buffer, protected_data_stream.gcount());
			decrypted_data.append((const char *)decrypted_buffer, protected_data_stream.gcount());
		}
		
		// RC4 is a stream cipher, so we don't bother to EVP_DecryptFinal, as there is
		// no block padding.
		EVP_CIPHER_CTX_cleanup(&ctx);
		std::istringstream parse_stream(decrypted_data);
		if (parser->parse(parse_stream, mProtectedDataMap, 
						  LLSDSerialize::SIZE_UNLIMITED) == LLSDParser::PARSE_FAILURE)
		{
			throw LLProtectedDataException("Corrupt Protected Data Store");
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
										llofstream::binary);
	try
	{
		
		EVP_CIPHER_CTX ctx;
		EVP_CIPHER_CTX_init(&ctx);
		EVP_EncryptInit(&ctx, EVP_rc4(), salt, NULL);
		protected_data_stream.write((const char *)salt, STORE_SALT_SIZE);	
		while (formatted_data_istream.good())
		{
			formatted_data_istream.read((char *)buffer, BUFFER_READ_SIZE);
			if(formatted_data_istream.gcount() == 0)
			{
				break;
			}
			int encrypted_length;
			EVP_EncryptUpdate(&ctx, encrypted_buffer, &encrypted_length, 
						  buffer, formatted_data_istream.gcount());
			protected_data_stream.write((const char *)encrypted_buffer, encrypted_length);
		}
		
		// no EVP_EncrypteFinal, as this is a stream cipher
		EVP_CIPHER_CTX_cleanup(&ctx);

		protected_data_stream.close();
	}
	catch (...)
	{
		// it's good practice to clean up any secure information on error
		// (even though this file isn't really secure.  Perhaps in the future
		// it may be, however.
		LLFile::remove(tmp_filename);
		throw LLProtectedDataException("Error writing Protected Data Store");
	}

	// move the temporary file to the specified file location.
	if((LLFile::remove(mProtectedDataFilename) != 0) || 
	   (LLFile::rename(tmp_filename, mProtectedDataFilename)))
	{
		LLFile::remove(tmp_filename);
		throw LLProtectedDataException("Could not overwrite protected data store");
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
LLPointer<LLCertificateChain> LLSecAPIBasicHandler::getCertificateChain(const X509_STORE_CTX* chain)
{
	LLPointer<LLCertificateChain> result = NULL;
	return result;
}
		
// instantiate a cert store given it's id.  if a persisted version
// exists, it'll be loaded.  If not, one will be created (but not
// persisted)
LLPointer<LLCertificateStore> LLSecAPIBasicHandler::getCertificateStore(const std::string& store_id)
{
	LLPointer<LLCertificateStore> result;
	return result;
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

// persist data in a protected store
void LLSecAPIBasicHandler::setProtectedData(const std::string& data_type,
									  const std::string& data_id,
									  const LLSD& data)
{
	if (!mProtectedDataMap.has(data_type) || !mProtectedDataMap[data_type].isMap()) {
		mProtectedDataMap[data_type] = LLSD::emptyMap();
	}
	
	mProtectedDataMap[data_type][data_id] = data; 
}