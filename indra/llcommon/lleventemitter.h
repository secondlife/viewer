/** 
 * @file lleventemitter.h
 * @brief General event emitter class
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
//	templatized emitter class
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
