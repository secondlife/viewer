/** 
 * @file lleventemitter.h
 * @brief General event emitter class
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
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
