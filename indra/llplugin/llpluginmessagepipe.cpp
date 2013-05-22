/** 
 * @file llpluginmessagepipe.cpp
 * @brief Classes that implement connections from the plugin system to pipes/pumps.
 *
 * @cond
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

LLPluginMessagePipe::LLPluginMessagePipe(LLPluginMessagePipeOwner *owner, LLSocket::ptr_t socket):
	mInputMutex(gAPRPoolp),
	mOutputMutex(gAPRPoolp),
	mOutputStartIndex(0),
	mOwner(owner),
	mSocket(socket)
{
	mOwner->setMessagePipe(this);
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
	LLMutexLock lock(&mOutputMutex);

	// If we're starting to use up too much memory, clear
	if (mOutputStartIndex > 1024 * 1024)
	{
		mOutput = mOutput.substr(mOutputStartIndex);
		mOutputStartIndex = 0;
	}
		
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
	bool result = pumpOutput();
	
	if(result)
	{
		result = pumpInput(timeout);
	}
	
	return result;
}

bool LLPluginMessagePipe::pumpOutput()
{
	bool result = true;
	
	if(mSocket)
	{
		apr_status_t status;
		apr_size_t in_size, out_size;
		
		LLMutexLock lock(&mOutputMutex);

		const char * output_data = &(mOutput.data()[mOutputStartIndex]);
		if(*output_data != '\0')
		{
			// write any outgoing messages
			in_size = (apr_size_t) (mOutput.size() - mOutputStartIndex);
			out_size = in_size;
			
			setSocketTimeout(0);
			
//			LL_INFOS("Plugin") << "before apr_socket_send, size = " << size << LL_ENDL;

			status = apr_socket_send(mSocket->getSocket(),
									 output_data,
									 &out_size);

//			LL_INFOS("Plugin") << "after apr_socket_send, size = " << size << LL_ENDL;
			
			if((status == APR_SUCCESS) || APR_STATUS_IS_EAGAIN(status))
			{
				// Success or Socket buffer is full... 
				
				// If we've pumped the entire string, clear it
				if (out_size == in_size)
				{
					mOutputStartIndex = 0;
					mOutput.clear();
				}
				else
				{
					llassert(in_size > out_size);
					
					// Remove the written part from the buffer and try again later.
					mOutputStartIndex += out_size;
				}
			}
			else if(APR_STATUS_IS_EOF(status))
			{
				// This is what we normally expect when a plugin exits.
				llinfos << "Got EOF from plugin socket. " << llendl;

				if(mOwner)
				{
					mOwner->socketError(status);
				}
				result = false;
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
	}
	
	return result;
}

bool LLPluginMessagePipe::pumpInput(F64 timeout)
{
	bool result = true;

	if(mSocket)
	{
		apr_status_t status;
		apr_size_t size;

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
			
			if(timeout == 0.0f)
			{
				// If we have no timeout, start out with a full read.
				request_size = sizeof(input_buf);
			}
			else
			{
				// Start out by reading one byte, so that any data received will wake us up.
				request_size = 1;
			}
			
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
				{
					LLMutexLock lock(&mInputMutex);
					mInput.append(input_buf, size);
				}

				if(status == APR_SUCCESS)
				{
					LL_DEBUGS("PluginSocket") << "success, read " << size << LL_ENDL;

					if(size != request_size)
					{
						// This was a short read, so we're done.
						break;
					}
				}
				else if(APR_STATUS_IS_TIMEUP(status))
				{
					LL_DEBUGS("PluginSocket") << "TIMEUP, read " << size << LL_ENDL;

					// Timeout was hit.  Since the initial read is 1 byte, this should never be a partial read.
					break;
				}
				else if(APR_STATUS_IS_EAGAIN(status))
				{
					LL_DEBUGS("PluginSocket") << "EAGAIN, read " << size << LL_ENDL;

					// Non-blocking read returned immediately.
					break;
				}
				else if(APR_STATUS_IS_EOF(status))
				{
					// This is what we normally expect when a plugin exits.
					LL_INFOS("PluginSocket") << "Got EOF from plugin socket. " << LL_ENDL;

					if(mOwner)
					{
						mOwner->socketError(status);
					}
					result = false;
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

				if(timeout != 0.0f)
				{
					// Second and subsequent reads should not use the timeout
					setSocketTimeout(0);
					// and should try to fill the input buffer
					request_size = sizeof(input_buf);
				}
			}
			
			processInput();
		}
	}
	
	return result;	
}

void LLPluginMessagePipe::processInput(void)
{
	// Look for input delimiter(s) in the input buffer.
	int delim;
	mInputMutex.lock();
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
			mInputMutex.unlock();
			mOwner->receiveMessageRaw(message);
			mInputMutex.lock();
		}
		else
		{
			LL_WARNS("Plugin") << "!mOwner" << LL_ENDL;
		}
	}
	mInputMutex.unlock();
}

