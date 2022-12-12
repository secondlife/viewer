/**
 * @file   llinstancetrackersubclass.h
 * @author Nat Goodspeed
 * @date   2022-12-09
 * @brief  Intermediate class to get subclass-specific types from
 *         LLInstanceTracker instance-retrieval methods.
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Copyright (c) 2022, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLINSTANCETRACKERSUBCLASS_H)
#define LL_LLINSTANCETRACKERSUBCLASS_H

#include <memory>                   // std::shared_ptr, std::weak_ptr

/**
 * Derive your subclass S of a subclass T of LLInstanceTracker<T> from
 * LLInstanceTrackerSubclass<S, T> to perform appropriate downcasting and
 * filtering for LLInstanceTracker access methods.
 *
 * LLInstanceTracker<T> uses CRTP, so that getWeak(), getInstance(), snapshot
 * and instance_snapshot return pointers and references to T. The trouble is
 * that subclasses T0 and T1 derived from T also get pointers and references
 * to their base class T, requiring explicit downcasting. Moreover,
 * T0::getInstance() shouldn't find an instance of any T subclass other than
 * T0. Nor should T0::snapshot.
 *
 * @code
 * class Tracked: public LLInstanceTracker<Tracked, std::string>
 * {
 * private:
 *     using super = LLInstanceTracker<Tracked, std::string>;
 * public:
 *     Tracked(const std::string& name): super(name) {}
 *     // All references to Tracked::ptr_t, Tracked::getInstance() etc.
 *     // appropriately use Tracked.
 *     // ...
 * };
 *
 * // But now we derive SubTracked from Tracked. We need SubTracked::ptr_t,
 * // SubTracked::getInstance() etc. to use SubTracked, not Tracked.
 * // This LLInstanceTrackerSubclass specialization is itself derived from
 * // Tracked.
 * class SubTracked: public LLInstanceTrackerSubclass<SubTracked, Tracked>
 * {
 * private:
 *     using super = LLInstanceTrackerSubclass<SubTracked, Tracked>;
 * public:
 *     // LLInstanceTrackerSubclass's constructor forwards to Tracked's.
 *     SubTracked(const std::string& name): super(name) {}
 *     // SubTracked::getInstance() returns std::shared_ptr<SubTracked>, etc.
 *     // ...
 * @endcode
 */
template <typename SUBCLASS, typename T>
class LLInstanceTrackerSubclass: public T
{
public:
    using ptr_t  = std::shared_ptr<SUBCLASS>;
    using weak_t = std::weak_ptr<SUBCLASS>;

    // forward any constructor call to the corresponding T ctor
    template <typename... ARGS>
    LLInstanceTrackerSubclass(ARGS&&... args):
        T(std::forward<ARGS>(args)...)
    {}

    weak_t getWeak()
    {
        // call base-class getWeak(), try to lock, downcast to SUBCLASS
        return std::dynamic_pointer_cast<SUBCLASS>(T::getWeak().lock());
    }

    template <typename KEY>
    static ptr_t getInstance(const KEY& k)
    {
        return std::dynamic_pointer_cast<SUBCLASS>(T::getInstance(k));
    }

    using snapshot          = typename T::template snapshot_of<SUBCLASS>;
    using instance_snapshot = typename T::template instance_snapshot_of<SUBCLASS>;
    using key_snapshot      = typename T::template key_snapshot_of<SUBCLASS>;

    static size_t instanceCount()
    {
        // T::instanceCount() lies because our snapshot, et al., won't
        // necessarily return all the T instances -- only those that are also
        // SUBCLASS instances. Count those.
        size_t count = 0;
        for (const auto& pair : snapshot())
            ++count;
        return count;
    }
};

#endif /* ! defined(LL_LLINSTANCETRACKERSUBCLASS_H) */
