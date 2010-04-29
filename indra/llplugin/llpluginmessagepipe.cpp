/** 
 * @file llpluginmessagepipe.cpp
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
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
 * @endcond
 */

#include "linden_common.h"

#include "llpluginmessagepipe.h"
#include "llbufferstream.h"

#include "llapr.h"

static const char MESSAGE_DELIMITER = '\0';

LLPluginMessagePipeOwner::LLPluginMessagePipeOwner() :
	mMessagePipe(NULL),
	mSocketError(APR_SUCCESS)
{
}

// virtual 
LLPluginMessagePipeOwner::~LLPluginMessagePipeOwner()
{
	killMessagePipe();
}

// virtual 
apr_status_t LLPluginMessagePipeOwner::socketError(apr_status_t error)
{ 
	mSocketError = error;
	return error; 
};

//virtual 
void LLPluginMessagePipeOwner::setMessagePipe(LLPluginMessagePipe *read_pipe)
{
	// Save a reference to this pipe
	mMessagePipe = read_pipe;
}

bool LLPluginMessagePipeOwner::canSendMessage(void)
{
	return (mMessagePipe != NULL);
}

bool LLPluginMessagePipeOwner::writeMessageRaw(const std::string &message)
{
	bool result = true;
	if(mMessagePipe != NULL)
	{
		result = mMessagePipe->addMessage(message);
	}
	else
	{
		LL_WARNS("Plugin") << "dropping message: " << message << LL_ENDL;
		result = false;
	}
	
	return result;
}

void LLPluginMessagePipeOwner::killMessagePipe(void)
{
	if(mMessagePipe != NULL)
	{
		delete mMessagePipe;
		mMessagePipe = NULL;
	}
}

LLPluginMessagePipe::LLPluginMessagePipe(LLPluginMessagePipeOwner *owner, LLSocket::ptr_t socket)
{
	mOwner = owner;
	mOwner->setMessagePipe(this);
	mSocket = socket;
}

LLPluginMessagePipe::~LLPluginMessagePipe()
{
	if(mOwner != NULL)
	{
		mOwner->setMessagePipe(NULL);
	}
}

bool LLPluginMessagePipe::addMessage(const std::string &message)
{
	// queue the message for later output
	mOutput += message;
	mOutput += MESSAGE_DELIMITER;	// message separator
	
	return true;
}

void LLPluginMessagePipe::clearOwner(void)
{
	// The owner is done with this pipe.  The next call to process_impl should send any remaining data and exit.
	mOwner = NULL;
}

void LLPluginMessagePipe::setSocketTimeout(apr_interval_time_t timeout_usec)
{
	// We never want to sleep forever, so force negative timeouts to become non-blocking.

	// according to this page: http://dev.ariel-networks.com/apr/apr-tutorial/html/apr-tutorial-13.html
	// blocking/non-blocking with apr sockets is somewhat non-portable.
	
	if(timeout_usec <= 0)
	{
		// Make the socket non-blocking
		apr_socket_opt_set(mSocket->getSocket(), APR_SO_NONBLOCK, 1);
		apr_socket_timeout_set(mSocket->getSocket(), 0);
	}
	else
	{
		// Make the socket blocking-with-timeout
		apr_socket_opt_set(mSocket->getSocket(), APR_SO_NONBLOCK, 1);
		apr_socket_timeout_set(mSocket->getSocket(), timeout_usec);
	}
}

bool LLPluginMessagePipe::pump(F64 timeout)
{
	bool result = true;
	
	if(mSocket)
	{
		apr_status_t status;
		apr_size_t size;
		
		if(!mOutput.empty())
		{
			// write any outgoing messages
			size = (apr_size_t)mOutput.size();
			
			setSocketTimeout(0);
			
//			LL_INFOS("Plugin") << "before apr_socket_send, size = " << size << LL_ENDL;

			status = apr_socket_send(
					mSocket->getSocket(),
					(const char*)mOutput.data(),
					&size);

//			LL_INFOS("Plugin") << "after apr_socket_send, size = " << size << LL_ENDL;
			
			if(status == APR_SUCCESS)
			{
				// success
				mOutput = mOutput.substr(size);
			}
			else if(APR_STATUS_IS_EAGAIN(status))
			{
				// Socket buffer is full... 
				// remove the written part from the buffer and try again later.
				mOutput = mOutput.substr(size);
			}
			else 
			{
				// some other error
				// Treat this as fatal.
				ll_apr_warn_status(status);
				
				if(mOwner)
				{
					mOwner->socketError(status);
				}
				result = false;
			}
		}

		// FIXME: For some reason, the apr timeout stuff isn't working properly on windows.
		// Until such time as we figure out why, don't try to use the socket timeout -- just sleep here instead.
#if LL_WINDOWS
		if(result)
		{
			if(timeout != 0.0f)
			{
				ms_sleep((int)(timeout * 1000.0f));
				timeout = 0.0f;
			}
		}
#endif
		
		// Check for incoming messages
		if(result)
		{
			char input_buf[1024];
			apr_size_t request_size;
			
			// Start out by reading one byte, so that any data received will wake us up.
			request_size = 1;
			
			// and use the timeout so we'll sleep if no data is available.
			setSocketTimeout((apr_interval_time_t)(timeout * 1000000));

			while(1)		
			{
				size = request_size;

//				LL_INFOS("Plugin") << "before apr_socket_recv, size = " << size << LL_ENDL;

				status = apr_socket_recv(
						mSocket->getSocket(), 
						input_buf, 
						&size);

//				LL_INFOS("Plugin") << "after apr_socket_recv, size = " << size << LL_ENDL;
				
				if(size > 0)
					mInput.append(input_buf, size);

				if(status == APR_SUCCESS)
				{
//					llinfos << "success, read " << size << llendl;

					if(size != request_size)
					{
						// This was a short read, so we're done.
						break;
					}
				}
				else if(APR_STATUS_IS_TIMEUP(status))
				{
//					llinfos << "TIMEUP, read " << size << llendl;

					// Timeout was hit.  Since the initial read is 1 byte, this should never be a partial read.
					break;
				}
				else if(APR_STATUS_IS_EAGAIN(status))
				{
//					llinfos << "EAGAIN, read " << size << llendl;

					// We've been doing partial reads, and we're done now.
					break;
				}
				else
				{
					// some other error
					// Treat this as fatal.
					ll_apr_warn_status(status);

					if(mOwner)
					{
						mOwner->socketError(status);
					}
					result = false;
					break;
				}

				// Second and subsequent reads should not use the timeout
				setSocketTimeout(0);
				// and should try to fill the input buffer
				request_size = sizeof(input_buf);
			}
			
			processInput();
		}
	}

	if(!result)
	{
		// If we got an error, we're done.
		LL_INFOS("Plugin") << "Error from socket, cleaning up." << LL_ENDL;
		delete this;
	}
	
	return result;	
}

void LLPluginMessagePipe::processInput(void)
{
	// Look for input delimiter(s) in the input buffer.
	int delim;
	while((delim = mInput.find(MESSAGE_DELIMITER)) != std::string::npos)
	{	
		// Let the owner process this message
		if (mOwner)
		{
			// Pull the message out of the input buffer before calling receiveMessageRaw.
			// It's now possible for this function to get called recursively (in the case where the plugin makes a blocking request)
			// and this guarantees that the messages will get dequeued correctly.
			std::string message(mInput, 0, delim);
			mInput.erase(0, delim + 1);
			mOwner->receiveMessageRaw(message);
		}
		else
		{
			LL_WARNS("Plugin") << "!mOwner" << LL_ENDL;
		}
	}
}

