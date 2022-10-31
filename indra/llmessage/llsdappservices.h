/** 
 * @file llsdappservices.h
 * @author Phoenix
 * @date 2006-09-12
 * @brief Header file to declare the /app common web services.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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
