/** 
 * @file llsingleton.cpp
 * @author Brad Kittenbrink
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "linden_common.h"
#include "llsingleton.h"

#include "llerror.h"
#include "llerrorcontrol.h"
#include "lldependencies.h"
#include "llexception.h"
#include "llcoros.h"
#include <boost/foreach.hpp>
#include <algorithm>
#include <iostream>                 // std::cerr in dire emergency
#include <sstream>
#include <stdexcept>

namespace {
void log(LLError::ELevel level,
         const char* p1, const char* p2, const char* p3, const char* p4);
} // anonymous namespace

// Our master list of all LLSingletons is itself an LLSingleton. We used to
// store it in a function-local static, but that could get destroyed before
// the last of the LLSingletons -- and ~LLSingletonBase() definitely wants to
// remove itself from the master list. Since the whole point of this master
// list is to help track inter-LLSingleton dependencies, and since we have
// this implicit dependency from every LLSingleton to the master list, make it
// an LLSingleton.
class LLSingletonBase::MasterList:
    public LLSingleton<LLSingletonBase::MasterList>
{
private:
    LLSINGLETON_EMPTY_CTOR(MasterList);

    // Independently of the LLSingleton locks governing construction,
    // destruction and other state changes of the MasterList instance itself,
    // we must also defend each of the data structures owned by the
    // MasterList.
    // This must be a recursive_mutex because, while the lock is held for
    // manipulating some data in the master list, we must also check whether
    // it's safe to log -- which involves querying a different LLSingleton --
    // which requires accessing the master list.
    typedef std::recursive_mutex mutex_t;
    typedef std::unique_lock<mutex_t> lock_t;

    mutex_t mMutex;

public:
    // Instantiate this to both obtain a reference to MasterList::instance()
    // and lock its mutex for the lifespan of this Lock instance.
    class Lock
    {
    public:
        Lock():
            mMasterList(MasterList::instance()),
            mLock(mMasterList.mMutex)
        {}
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;
        MasterList& get() const { return mMasterList; }
        operator MasterList&() const { return get(); }

    protected:
        MasterList& mMasterList;
        MasterList::lock_t mLock;
    };

private:
    // This is the master list of all instantiated LLSingletons (save the
    // MasterList itself) in arbitrary order. You MUST call dep_sort() before
    // traversing this list.
    list_t mMaster;

public:
    // Instantiate this to obtain a reference to MasterList::mMaster and to
    // hold the MasterList lock for the lifespan of this LockedMaster
    // instance.
    struct LockedMaster: public Lock
    {
        list_t& get() const { return mMasterList.mMaster; }
        operator list_t&() const { return get(); }
    };

private:
    // We need to maintain a stack of LLSingletons currently being
    // initialized, either in the constructor or in initSingleton(). However,
    // managing that as a stack depends on having a DISTINCT 'initializing'
    // stack for every C++ stack in the process! And we have a distinct C++
    // stack for every running coroutine. Therefore this stack must be based
    // on a coroutine-local pointer.
    // This local_ptr isn't static because it's a member of an LLSingleton.
    LLCoros::local_ptr<list_t> mInitializing;

public:
    // Instantiate this to obtain a reference to the coroutine-specific
    // initializing list and to hold the MasterList lock for the lifespan of
    // this LockedInitializing instance.
    struct LockedInitializing: public Lock
    {
    public:
        LockedInitializing():
            // only do the lookup once, cache the result
            // note that the lock is already locked during this lookup
            mList(&mMasterList.get_initializing_())
        {}
        list_t& get() const
        {
            if (! mList)
            {
                LLTHROW(LLException("Trying to use LockedInitializing "
                                    "after cleanup_initializing()"));
            }
            return *mList;
        }
        operator list_t&() const { return get(); }
        void log(const char* verb, const char* name);
        void cleanup_initializing()
        {
            mMasterList.cleanup_initializing_();
            mList = nullptr;
        }

    private:
        // Store pointer since cleanup_initializing() must clear it.
        list_t* mList;
    };

private:
    list_t& get_initializing_()
    {
        LLSingletonBase::list_t* current = mInitializing.get();
        if (! current)
        {
            // If the running coroutine doesn't already have an initializing
            // stack, allocate a new one and save it for future reference.
            current = new LLSingletonBase::list_t();
            mInitializing.reset(current);
        }
        return *current;
    }

    // By the time mInitializing is destroyed, its value for every coroutine
    // except the running one must have been reset() to nullptr. So every time
    // we pop the list to empty, reset() the running coroutine's local_ptr.
    void cleanup_initializing_()
    {
        mInitializing.reset(nullptr);
    }
};

void LLSingletonBase::add_master()
{
    // As each new LLSingleton is constructed, add to the master list.
    // This temporary LockedMaster should suffice to hold the MasterList lock
    // during the push_back() call.
    MasterList::LockedMaster().get().push_back(this);
}

void LLSingletonBase::remove_master()
{
    // When an LLSingleton is destroyed, remove from master list.
    // add_master() used to capture the iterator to the newly-added list item
    // so we could directly erase() it from the master list. Unfortunately
    // that runs afoul of destruction-dependency order problems. So search the
    // master list, and remove this item IF FOUND. We have few enough
    // LLSingletons, and they are so rarely destroyed (once per run), that the
    // cost of a linear search should not be an issue.
    // This temporary LockedMaster should suffice to hold the MasterList lock
    // during the remove() call.
    MasterList::LockedMaster().get().remove(this);
}

//static
LLSingletonBase::list_t::size_type LLSingletonBase::get_initializing_size()
{
    return MasterList::LockedInitializing().get().size();
}

LLSingletonBase::~LLSingletonBase() {}

void LLSingletonBase::push_initializing(const char* name)
{
    MasterList::LockedInitializing locked_list;
    // log BEFORE pushing so logging singletons don't cry circularity
    locked_list.log("Pushing", name);
    locked_list.get().push_back(this);
}

void LLSingletonBase::pop_initializing()
{
    // Lock the MasterList for the duration of this call
    MasterList::LockedInitializing locked_list;
    list_t& list(locked_list.get());

    if (list.empty())
    {
        logerrs("Underflow in stack of currently-initializing LLSingletons at ",
                classname(this).c_str(), "::getInstance()");
    }

    // Now we know list.back() exists: capture it
    LLSingletonBase* back(list.back());
    // and pop it
    list.pop_back();

    // The viewer launches an open-ended number of coroutines. While we don't
    // expect most of them to initialize LLSingleton instances, our present
    // get_initializing() logic could lead to an open-ended number of map
    // entries. So every time we pop the stack back to empty, delete the entry
    // entirely.
    if (list.empty())
    {
        locked_list.cleanup_initializing();
    }

    // Now validate the newly-popped LLSingleton.
    if (back != this)
    {
        logerrs("Push/pop mismatch in stack of currently-initializing LLSingletons: ",
                classname(this).c_str(), "::getInstance() trying to pop ",
                classname(back).c_str());
    }

    // log AFTER popping so logging singletons don't cry circularity
    locked_list.log("Popping", typeid(*back).name());
}

void LLSingletonBase::reset_initializing(list_t::size_type size)
{
    // called for cleanup in case the LLSingleton subclass constructor throws
    // an exception

    // The tricky thing about this, the reason we have a separate method
    // instead of just calling pop_initializing(), is (hopefully remote)
    // possibility that the exception happened *before* the
    // push_initializing() call in LLSingletonBase's constructor. So only
    // remove the stack top if in fact we've pushed something more than the
    // previous size.
    MasterList::LockedInitializing locked_list;
    list_t& list(locked_list.get());

    while (list.size() > size)
    {
        list.pop_back();
    }

    // as in pop_initializing()
    if (list.empty())
    {
        locked_list.cleanup_initializing();
    }
}

void LLSingletonBase::MasterList::LockedInitializing::log(const char* verb, const char* name)
{
        LL_DEBUGS("LLSingleton") << verb << ' ' << demangle(name) << ';';
        if (mList)
        {
            for (list_t::const_reverse_iterator ri(mList->rbegin()), rend(mList->rend());
                 ri != rend; ++ri)
            {
                LLSingletonBase* sb(*ri);
                LL_CONT << ' ' << classname(sb);
            }
        }
        LL_ENDL;
}

void LLSingletonBase::capture_dependency()
{
    MasterList::LockedInitializing locked_list;
    list_t& initializing(locked_list.get());
    // Did this getInstance() call come from another LLSingleton, or from
    // vanilla application code? Note that although this is a nontrivial
    // method, the vast majority of its calls arrive here with initializing
    // empty().
    if (! initializing.empty())
    {
        // getInstance() is being called by some other LLSingleton. But -- is
        // this a circularity? That is, does 'this' already appear in the
        // initializing stack?
        // For what it's worth, normally 'initializing' should contain very
        // few elements.
        list_t::const_iterator found =
            std::find(initializing.begin(), initializing.end(), this);
        if (found != initializing.end())
        {
            list_t::const_iterator it_next = found;
            it_next++;

            // Report the circularity. Requiring the coder to dig through the
            // logic to diagnose exactly how we got here is less than helpful.
            std::ostringstream out;
            for ( ; found != initializing.end(); ++found)
            {
                // 'found' is an iterator; *found is an LLSingletonBase*; **found
                // is the actual LLSingletonBase instance.
                LLSingletonBase* foundp(*found);
                out << classname(foundp) << " -> ";
            }
            // Decide which log helper to call.
            if (it_next == initializing.end())
            {
                // Points to self after construction, but during initialization.
                // Singletons can initialize other classes that depend onto them,
                // so this is expected.
                //
                // Example: LLNotifications singleton initializes default channels.
                // Channels register themselves with singleton once done.
                logdebugs("LLSingleton circularity: ", out.str().c_str(),
                    classname(this).c_str(), "");
            }
            else
            {
                // Actual circularity with other singleton (or single singleton is used extensively).
                // Dependency can be unclear.
                logwarns("LLSingleton circularity: ", out.str().c_str(),
                    classname(this).c_str(), "");
            }
        }
        else
        {
            // Here 'this' is NOT already in the 'initializing' stack. Great!
            // Record the dependency.
            // initializing.back() is the LLSingletonBase* currently being
            // initialized. Store 'this' in its mDepends set.
            LLSingletonBase* current(initializing.back());
            if (current->mDepends.insert(this).second)
            {
                // only log the FIRST time we hit this dependency!
                logdebugs(classname(current).c_str(),
                          " depends on ", classname(this).c_str());
            }
        }
    }
}

//static
LLSingletonBase::vec_t LLSingletonBase::dep_sort()
{
    // While it would theoretically be possible to maintain a static
    // SingletonDeps through the life of the program, dynamically adding and
    // removing LLSingletons as they are created and destroyed, in practice
    // it's less messy to construct it on demand. The overhead of doing so
    // should happen basically once: for deleteAll().
    typedef LLDependencies<LLSingletonBase*> SingletonDeps;
    SingletonDeps sdeps;
    // Lock while traversing the master list 
    MasterList::LockedMaster master;
    for (LLSingletonBase* sp : master.get())
    {
        // Build the SingletonDeps structure by adding, for each
        // LLSingletonBase* sp in the master list, sp itself. It has no
        // associated value type in our SingletonDeps, hence the 0. We don't
        // record the LLSingletons it must follow; rather, we record the ones
        // it must precede. Copy its mDepends to a KeyList to express that.
        sdeps.add(sp, 0,
                  SingletonDeps::KeyList(),
                  SingletonDeps::KeyList(sp->mDepends.begin(), sp->mDepends.end()));
    }
    vec_t ret;
    ret.reserve(master.get().size());
    // We should be able to effect this with a transform_iterator that
    // extracts just the first (key) element from each sorted_iterator, then
    // uses vec_t's range constructor... but frankly this is more
    // straightforward, as long as we remember the above reserve() call!
    for (const SingletonDeps::sorted_iterator::value_type pair : sdeps.sort())
    {
        ret.push_back(pair.first);
    }
    // The master list is not itself pushed onto the master list. Add it as
    // the very last entry -- it is the LLSingleton on which ALL others
    // depend! -- so our caller will process it.
    ret.push_back(&master.Lock::get());
    return ret;
}

void LLSingletonBase::cleanup_()
{
    logdebugs("calling ", classname(this).c_str(), "::cleanupSingleton()");
    try
    {
        cleanupSingleton();
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION(classname(this) + "::cleanupSingleton()");
    }
}

//static
void LLSingletonBase::deleteAll()
{
    // It's essential to traverse these in dependency order.
    BOOST_FOREACH(LLSingletonBase* sp, dep_sort())
    {
        // Capture the class name first: in case of exception, don't count on
        // being able to extract it later.
        const std::string name = classname(sp);
        try
        {
            // Call static method through instance function pointer.
            if (! sp->mDeleteSingleton)
            {
                // This Should Not Happen... but carry on.
                logwarns(name.c_str(), "::mDeleteSingleton not initialized!");
            }
            else
            {
                // properly initialized: call it.
                logdebugs("calling ", name.c_str(), "::deleteSingleton()");
                // From this point on, DO NOT DEREFERENCE sp!
                sp->mDeleteSingleton();
            }
        }
        catch (const std::exception& e)
        {
            logwarns("Exception in ", name.c_str(), "::deleteSingleton(): ", e.what());
        }
        catch (...)
        {
            logwarns("Unknown exception in ", name.c_str(), "::deleteSingleton()");
        }
    }
}

/*---------------------------- Logging helpers -----------------------------*/
namespace {

void log(LLError::ELevel level,
         const char* p1, const char* p2, const char* p3, const char* p4)
{
    LL_VLOGS(level, "LLSingleton") << p1 << p2 << p3 << p4 << LL_ENDL;
}

} // anonymous namespace        

//static
void LLSingletonBase::logwarns(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_WARN, p1, p2, p3, p4);
}

//static
void LLSingletonBase::loginfos(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_INFO, p1, p2, p3, p4);
}

//static
void LLSingletonBase::logdebugs(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_DEBUG, p1, p2, p3, p4);
}

//static
void LLSingletonBase::logerrs(const char* p1, const char* p2, const char* p3, const char* p4)
{
    log(LLError::LEVEL_ERROR, p1, p2, p3, p4);
    // The other important side effect of LL_ERRS() is
    // https://www.youtube.com/watch?v=OMG7paGJqhQ (emphasis on OMG)
    std::ostringstream out;
    out << p1 << p2 << p3 << p4;
    auto crash = LLError::getFatalFunction();
    if (crash)
    {
        crash(out.str());
    }
    else
    {
        LLError::crashAndLoop(out.str());
    }
}

std::string LLSingletonBase::demangle(const char* mangled)
{
    return LLError::Log::demangle(mangled);
}
