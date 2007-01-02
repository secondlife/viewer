/** 
 * @file lleventpoll.h
 * @brief LLEvDescription of the LLEventPoll class.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLEVENTPOLL_H
#define LL_LLEVENTPOLL_H

class LLHTTPNode;


class LLEventPoll
	///< implements the viewer side of server-to-viewer pushed events.
{
public:
	LLEventPoll(const std::string& pollURL, const LLHTTPNode& treeRoot);
		/**< Start polling the URL.
		
			 The object will automatically responde to events
			 by calling handlers in the tree.
		*/
		

	virtual ~LLEventPoll();
		///< will stop polling, cancelling any poll in progress.


private:
	class Impl;
	Impl& impl;
};


#endif // LL_LLEVENTPOLL_H
