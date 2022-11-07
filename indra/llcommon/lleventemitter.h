/** 
 * @file lleventemitter.h
 * @brief General event emitter class
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

// header guard
#ifndef LL_EVENTEMITTER_H
#define LL_EVENTEMITTER_H

// standard headers
#include <algorithm>
#include <typeinfo>
#include <iostream>
#include <string>
#include <list>

#include "stdtypes.h"

///////////////////////////////////////////////////////////////////////////////
//  templatized emitter class
template < class T >
class eventEmitter
{
    public:
        typedef typename T::EventType EventType;
        typedef std::list< T* > ObserverContainer;
        typedef void ( T::*observerMethod )( const EventType& );

    protected:
        ObserverContainer observers;

    public:
        eventEmitter () { };

        ~eventEmitter () { };

        ///////////////////////////////////////////////////////////////////////////////
        //
        BOOL addObserver ( T* observerIn )
        {
            if ( ! observerIn )
                return FALSE;

            // check if observer already exists
            if ( std::find ( observers.begin (), observers.end (), observerIn ) != observers.end () )
                return FALSE;

            // save it
            observers.push_back ( observerIn );

            return true;
        };

        ///////////////////////////////////////////////////////////////////////////////
        //
        BOOL remObserver ( T* observerIn )
        {
            if ( ! observerIn )
                return FALSE;

            observers.remove ( observerIn );

            return TRUE;
        };

        ///////////////////////////////////////////////////////////////////////////////
        //
        void update ( observerMethod method, const EventType& msgIn )
        {
            typename std::list< T* >::iterator iter = observers.begin ();

            while ( iter != observers.end () )
            {
                ( ( *iter )->*method ) ( msgIn );

                ++iter;
            };
        };
};

#endif // lleventemitter_h
