/** 
 * @file llxmlrpctransaction.h
 * @brief LLXMLRPCTransaction and related class header file
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LLXMLRPCTRANSACTION_H
#define LLXMLRPCTRANSACTION_H

#include <string>

typedef struct _xmlrpc_request* XMLRPC_REQUEST;
typedef struct _xmlrpc_value* XMLRPC_VALUE;
	// foward decl of types from xmlrpc.h (this usage is type safe)

class LLXMLRPCValue
	// a c++ wrapper around XMLRPC_VALUE
{
public:
	LLXMLRPCValue()						: mV(NULL) { }
	LLXMLRPCValue(XMLRPC_VALUE value)	: mV(value) { }
	
	bool isValid() const;
	
	std::string	asString()	const;
	int			asInt()		const;
	bool		asBool()	const;
	double		asDouble()	const;

	LLXMLRPCValue operator[](const char*) const;
	
	LLXMLRPCValue rewind();
	LLXMLRPCValue next();

	static LLXMLRPCValue createArray();
	static LLXMLRPCValue createStruct();

	void append(LLXMLRPCValue&);
	void appendString(const std::string&);
	void appendInt(int);
	void appendBool(bool);
	void appendDouble(double);
	void appendValue(LLXMLRPCValue&);

	void append(const char*, LLXMLRPCValue&);
	void appendString(const char*, const std::string&);
	void appendInt(const char*, int);
	void appendBool(const char*, bool);
	void appendDouble(const char*, double);
	void appendValue(const char*, LLXMLRPCValue&);
	
	void free();
		// only call this on the top level created value
	
	XMLRPC_VALUE getValue() const;
	
private:
	XMLRPC_VALUE mV;
};


class LLXMLRPCTransaction
	// an asynchronous request and respones via XML-RPC
{
public:
	LLXMLRPCTransaction(const std::string& uri,
		XMLRPC_REQUEST request, bool useGzip = true);
		// does not take ownership of the request object
		// request can be freed as soon as the transaction is constructed

	LLXMLRPCTransaction(const std::string& uri,
		const std::string& method, LLXMLRPCValue params, bool useGzip = true);
		// *does* take control of the request value, you must not free it
		
	~LLXMLRPCTransaction();
	
	typedef enum {
		StatusNotStarted,
		StatusStarted,
		StatusDownloading,
		StatusComplete,
		StatusCURLError,
		StatusXMLRPCError,
		StatusOtherError
	} Status;

	bool process();
		// run the request a little, returns true when done
		
	Status status(int* curlCode);
		// return status, and extended CURL code, if code isn't null
	std::string statusMessage();
		// return a message string, suitable for showing the user
	std::string statusURI();
		// return a URI for the user with more information
		// can be empty

	XMLRPC_REQUEST response();
	LLXMLRPCValue responseValue();
		// only valid if StatusComplete, otherwise NULL
		// retains ownership of the result object, don't free it
	
	F64 transferRate();
		// only valid if StsatusComplete, otherwise 0.0
		
private:
	class Impl;
	Impl& impl;
};



#endif // LLXMLRPCTRANSACTION_H
