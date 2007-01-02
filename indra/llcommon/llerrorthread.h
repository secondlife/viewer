/** 
 * @file llerrorthread.h
 * @brief Specialized thread to handle runtime errors.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLERRORTHREAD_H
#define LL_LLERRORTHREAD_H

#include "llthread.h"

class LLErrorThread : public LLThread
{
public:
	LLErrorThread();
	~LLErrorThread();

	/*virtual*/ void run(void);
	void setUserData(void *user_data);
	void *getUserData() const;

protected:
	void* mUserDatap; // User data associated with this thread
};

#endif // LL_LLERRORTHREAD_H
