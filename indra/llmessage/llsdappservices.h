/** 
 * @file llsdappservices.h
 * @author Phoenix
 * @date 2006-09-12
 * @brief Header file to declare the /app common web services.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSDAPPSERVICES_H
#define LL_LLSDAPPSERVICES_H

/** 
 * @class LLSDAppServices
 * @brief This class forces a link to llsdappservices if the static
 * method is called which declares the /app web services.
 */
class LLSDAppServices
{
public:
	/**
	 * @brief Call this method to declare the /app common web services.
	 *
	 * This will register:
	 *  /app/config
	 *  /app/config/runtime-override
	 *  /app/config/runtime-override/<option-name>
	 *  /app/config/command-line
	 *  /app/config/specific
	 *  /app/config/general
	 *  /app/config/default
	 *  /app/config/live
	 *  /app/config/live/<option-name>
	 */
	static void useServices();
};


#endif // LL_LLSDAPPSERVICES_H
