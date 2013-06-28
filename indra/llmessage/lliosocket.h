/** 
 * @file lliosocket.h
 * @author Phoenix
 * @date 2005-07-31
 * @brief Declaration of files used for handling sockets and associated pipes
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

#ifndef LL_LLIOSOCKET_H
#define LL_LLIOSOCKET_H

/** 
 * The socket interface provided here is a simple wraper around apr
 * sockets, with a pipe source and sink to read and write off of the
 * socket. Every socket only performs non-blocking operations except
 * the server socket which only performs blocking operations when an
 * OS poll indicates it will not block.
 */

#include "lliopipe.h"
#include "apr_pools.h"
#include "apr_network_io.h"
#include "llchainio.h"

class LLHost;

/** 
 * @class LLSocket
 * @brief Implementation of a wrapper around a socket.
 *
 * An instance of this class represents a single socket over it's
 * entire life - from uninitialized, to connected, to a listening
 * socket depending on it's purpose. This class simplifies our access
 * into the socket interface by only providing stream/tcp and
 * datagram/udp sockets - the only types we are interested in, since
 * those are the only properly supported by all of our platforms.
 */
class LLSocket
{
public:
	/** 
	 * @brief Reference counted shared pointers to sockets.
	 */
	typedef boost::shared_ptr<LLSocket> ptr_t;

	/** 
	 * @brief Type of socket to create.
	 */
	enum EType
	{
		STREAM_TCP,
		DATAGRAM_UDP,
	};

	/** 
	 * @brief Anonymous enumeration to help identify ports
	 */
	enum
	{
		PORT_INVALID = (U16)-1,
		PORT_EPHEMERAL = 0,
	};

	/** 
	 * @brief Create a socket.
	 *
	 * This is the call you would use if you intend to create a listen
	 * socket. If you intend the socket to be known to external
	 * clients without prior port notification, do not use
	 * PORT_EPHEMERAL.
	 * @param pool The apr pool to use. A child pool will be created
	 * and associated with the socket.
	 * @param type The type of socket to create
	 * @param port The port for the socket
	 * @return A valid socket shared pointer if the call worked.
	 */
	static ptr_t create(
		apr_pool_t* pool,
		EType type,
		U16 port = PORT_EPHEMERAL);

	/** 
	 * @brief Create a LLSocket when you already have an apr socket.
	 *
	 * This method assumes an ephemeral port. This is typically used
	 * by calls which spawn a socket such as a call to
	 * <code>accept()</code> as in the server socket. This call should
	 * not fail if you have a valid apr socket.
	 * Because of the nature of how accept() works, you are expected
	 * to create a new pool for the socket, use that pool for the
	 * accept, and pass it in here where it will be bound with the
	 * socket and destroyed at the same time.
	 * @param socket The apr socket to use 
	 * @param pool The pool used to create the socket. *NOTE: The pool
	 * passed in will be DESTROYED.
	 * @return A valid socket shared pointer if the call worked.
	 */
	static ptr_t create(apr_socket_t* socket, apr_pool_t* pool);

	/** 
	 * @brief Perform a blocking connect to a host. Do not use in production.
	 *
	 * @param host The host to connect this socket to.
	 * @return Returns true if the connect was successful.
	 */
	bool blockingConnect(const LLHost& host);

	/** 
	 * @brief Get the type of socket
	 */
	//EType getType() const { return mType; }

	/** 
	 * @brief Get the port.
	 *
	 * This will return PORT_EPHEMERAL if bind was never called.
	 * @return Returns the port associated with this socket.
	 */
	U16 getPort() const { return mPort; }

	/** 
	 * @brief Get the apr socket implementation.
	 *
	 * @return Returns the raw apr socket.
	 */
	apr_socket_t* getSocket() const { return mSocket; }

	/** 
	 * @brief Set default socket options, with SO_NONBLOCK = 0 and a timeout in us.
	 * @param timeout Number of microseconds to wait on this socket. Any
	 * negative number means block-forever. TIMEOUT OF 0 IS NON-PORTABLE.
	 */
	void setBlocking(S32 timeout);

	/**
	 * @brief Set default socket options, with SO_NONBLOCK = 1 and timeout = 0.
	 */
	void setNonBlocking();

protected:
	/**
	 * @brief Protected constructor since should only make sockets
	 * with one of the two <code>create()</code> calls.
	 */
	LLSocket(apr_socket_t* socket, apr_pool_t* pool);

public:
	/** 
	 * @brief Do not call this directly.
	 */
	~LLSocket();

protected:
	// The apr socket.
	apr_socket_t* mSocket;

	// our memory pool
	apr_pool_t* mPool;

	// The port if we know it.
	U16 mPort;

	//EType mType;
};

/** 
 * @class LLIOSocketReader
 * @brief An LLIOPipe implementation which reads from a socket.
 * @see LLIOPipe
 *
 * An instance of a socket reader wraps around an LLSocket and
 * performs non-blocking reads and passes it to the next pipe in the
 * chain.
 */
class LLIOSocketReader : public LLIOPipe
{
public:
	LLIOSocketReader(LLSocket::ptr_t socket);
	~LLIOSocketReader();

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Process the data coming in the socket.
	 *
	 * Since the socket and next pipe must exist for process to make
	 * any sense, this method will return STATUS_PRECONDITION_NOT_MET
	 * unless if they are not known.
	 * If a STATUS_STOP returned by the next link in the chain, this
	 * reader will turn of the socket polling.
	 * @param buffer Pointer to a buffer which needs processing. Probably NULL.
	 * @param bytes Number of bytes to in buffer to process. Probably 0.
	 * @param eos True if this function is the last. Almost always false.
	 * @param read Number of bytes actually processed.
	 * @param pump The pump which is calling process. May be NULL.
	 * @param context A data structure to pass structured data
	 * @return STATUS_OK unless the preconditions are not met.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	LLSocket::ptr_t mSource;
	std::vector<U8> mBuffer;
	bool mInitialized;
};

/** 
 * @class LLIOSocketWriter
 * @brief An LLIOPipe implementation which writes to a socket
 * @see LLIOPipe
 *
 * An instance of a socket writer wraps around an LLSocket and
 * performs non-blocking writes of the data passed in.
 */
class LLIOSocketWriter : public LLIOPipe
{
public:
	LLIOSocketWriter(LLSocket::ptr_t socket);
	~LLIOSocketWriter();

protected:
	/* @name LLIOPipe virtual implementations
	 */
	//@{
	/** 
	 * @brief Write the data in buffer to the socket.
	 *
	 * Since the socket pipe must exist for process to make any sense,
	 * this method will return STATUS_PRECONDITION_NOT_MET if it is
	 * not known.
	 * @param buffer Pointer to a buffer which needs processing.
	 * @param bytes Number of bytes to in buffer to process.
	 * @param eos True if this function is the last.
	 * @param read Number of bytes actually processed.
	 * @param pump The pump which is calling process. May be NULL.
	 * @param context A data structure to pass structured data
	 * @return A return code for the write.
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	LLSocket::ptr_t mDestination;
	U8* mLastWritten;
	bool mInitialized;
};

/** 
 * @class LLIOServerSocket
 * @brief An IOPipe implementation which listens and spawns connected
 * sockets.
 * @see LLIOPipe, LLChainIOFactory
 *
 * Each server socket instance coordinates with a pump to ensure it
 * only processes waiting connections. It uses the provided socket,
 * and assumes it is correctly initialized. When the connection is
 * established, the server will call the chain factory to build a
 * chain, and attach a socket reader and the front and a socket writer
 * at the end. It is up to the chain factory to create something which
 * correctly handles the established connection using the reader as a
 * source, and the writer as the final sink.
 * The newly added chain timeout is in DEFAULT_CHAIN_EXPIRY_SECS unless
 * adjusted with a call to setResponseTimeout().
 */
class LLIOServerSocket : public LLIOPipe
{
public:
	typedef LLSocket::ptr_t socket_t;
	typedef boost::shared_ptr<LLChainIOFactory> factory_t;
	LLIOServerSocket(apr_pool_t* pool, socket_t listener, factory_t reactor);
	virtual ~LLIOServerSocket();

	/** 
	 * @brief Set the timeout for the generated chains.
	 *
	 * This value is passed directly to the LLPumpIO::addChain()
	 * method. The default on construction is set to
	 * DEFAULT_CHAIN_EXPIRY_SECS which is a reasonable value for most
	 * applications based on this library. Avoid passing in
	 * NEVER_CHAIN_EXPIRY_SECS unless you have another method of
	 * harvesting chains.
	 * @param timeout_secs The seconds before timeout for the response chain. 
	 */
	void setResponseTimeout(F32 timeout_secs);

	/* @name LLIOPipe virtual implementations
	 */
	//@{
protected:
	/** 
	 * @brief Process the data in buffer
	 */
	virtual EStatus process_impl(
		const LLChannelDescriptors& channels,
		buffer_ptr_t& buffer,
		bool& eos,
		LLSD& context,
		LLPumpIO* pump);
	//@}

protected:
	apr_pool_t* mPool;
	socket_t mListenSocket;
	factory_t mReactor;
	bool mInitialized;
	F32 mResponseTimeout;
};

#if 0
/** 
 * @class LLIODataSocket
 * @brief BRIEF_DESC
 *
 * THOROUGH_DESCRIPTION
 */
class LLIODataSocket : public LLIOSocket
{
public:
	/**
	 * @brief Construct a datagram socket.
	 *
	 * If you pass in LLIOSocket::PORT_EPHEMERAL as the suggested
	 * port, The socket will not be in a 'listen' mode, but can still
	 * read data sent back to it's port. When suggested_port is not
	 * ephemeral or invalid and bind fails, the port discovery
	 * algorithm will search through a limited set of ports to
	 * try to find an open port. If that process fails, getPort() will
	 * return LLIOSocket::PORT_INVALID
	 * @param suggested_port The port you would like to bind. Use
	 * LLIOSocket::PORT_EPHEMERAL for an unspecified port.
	 * @param start_discovery_port The start range for
	 * @param pool The pool to use for allocation.
	 */
	LLIODataSocket(
		U16 suggested_port,
		U16 start_discovery_port,
		apr_pool_t* pool);
	virtual ~LLIODataSocket();

protected:

private:
	apr_socket_t* mSocket;
};
#endif

#endif // LL_LLIOSOCKET_H
