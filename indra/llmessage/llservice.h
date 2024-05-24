/**
 * @file llservice.h
 * @author Phoenix
 * @date 2004-11-21
 * @brief Declaration file for LLService and related classes.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLSERVICE_H
#define LL_LLSERVICE_H

#include <string>
#include <map>
//#include <boost/intrusive_ptr.hpp>
//#include <boost/shared_ptr.hpp>

//#include "llframetimer.h"
#include "lliopipe.h"
#include "llchainio.h"

#if 0
class LLServiceCreator;
/**
 * intrusive pointer support
 */
namespace boost
{
    void intrusive_ptr_add_ref(LLServiceCreator* p);
    void intrusive_ptr_release(LLServiceCreator* p);
};
#endif

/**
 * @class LLServiceCreator
 * @brief This class is an abstract base class for classes which create
 * new <code>LLService</code> instances.
 *
 * Derive classes from this class which appropriately implement the
 * <code>operator()</code> and destructor.
 * @see LLService
 */
#if 0
class LLServiceCreator
{
public:
    typedef boost::intrusive_ptr<LLService> service_t;
    virtual ~LLServiceCreator() {}
    virtual service_t activate() = 0;
    virtual void discard() = 0;

protected:
    LLServiceCreator() : mReferenceCount(0)
    {
    }

private:
    friend void boost::intrusive_ptr_add_ref(LLServiceCreator* p);
    friend void boost::intrusive_ptr_release(LLServiceCreator* p);
    U32 mReferenceCount;
};
#endif

#if 0
namespace boost
{
    inline void intrusive_ptr_add_ref(LLServiceCreator* p)
    {
        ++p->mReferenceCount;
    }
    inline void intrusive_ptr_release(LLServiceCreator* p)
    {
        if(p && 0 == --p->mReferenceCount)
        {
            delete p;
        }
    }
};
#endif

/**
 * @class LLService
 * @brief This class is the base class for the service classes.
 * @see LLIOPipe
 *
 * The services map a string to a chain factory with a known interface
 * at the front of the chain. So, to activate a service, call
 * <code>activate()</code> with the name of the service needed which
 * will call the associated factory, and return a pointer to the
 * known interface.
 * <b>NOTE:</b> If you are implementing a service factory, it is
 * vitally important that the service pipe is at the front of the
 * chain.
 */
class LLService : public LLIOPipe
{
public:
    //typedef boost::intrusive_ptr<LLServiceCreator> creator_t;
    //typedef boost::intrusive_ptr<LLService> service_t;
    typedef std::shared_ptr<LLChainIOFactory> creator_t;

    /**
     * @brief This method is used to register a protocol name with a
     * a functor that creates the service.
     *
     * THOROUGH_DESCRIPTION
     * @param aParameter A brief description of aParameter.
     * @return Returns true if a service was successfully registered.
     */
    static bool registerCreator(const std::string& name, creator_t fn);

    /**
     * @brief This method connects to a service by name.
     *
     * @param name The name of the service to connect to.
     * @param chain The constructed chain including the service instance.
     * @param context Context for the activation.
     * @return An instance of the service for use or NULL on failure.
     */
    static LLIOPipe* activate(
        const std::string& name,
        LLPumpIO::chain_t& chain,
        LLSD context);

    /**
     * @brief
     *
     * @param name The name of the service to discard.
     * @return true if service creator was found and discarded.
     */
    static bool discard(const std::string& name);

protected:
    // The creation factory static data.
    typedef std::map<std::string, creator_t> creators_t;
    static creators_t sCreatorFunctors;

protected:
    // construction & destruction. since this class is an abstract
    // base class, it is up to Service implementations to actually
    // deal with construction and not a public method. How that
    // construction takes place will be handled by the service
    // creators.
    LLService();
    virtual ~LLService();

protected:
    // This frame timer records how long this service has
    // existed. Useful for derived services to give themselves a
    // lifetime and expiration.
    // *NOTE: Phoenix - This functionaity has been moved to the
    // pump. 2005-12-13
    //LLFrameTimer mTimer;

    // Since services are designed in an 'ask now, respond later'
    // idiom which probably crosses thread boundaries, almost all
    // services will need a handle to a response pipe. It will usually
    // be the job of the service author to derive a useful
    // implementation of response, and up to the service subscriber to
    // further derive that class to do something useful when the
    // response comes in.
    LLIOPipe::ptr_t mResponse;
};


#endif // LL_LLSERVICE_H
