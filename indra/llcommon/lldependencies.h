/**
 * @file   lldependencies.h
 * @author Nat Goodspeed
 * @date   2008-09-17
 * @brief  LLDependencies: a generic mechanism for expressing "b must follow a,
 *         but precede c"
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

#if ! defined(LL_LLDEPENDENCIES_H)
#define LL_LLDEPENDENCIES_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <iosfwd>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

/*****************************************************************************
*   Utilities
*****************************************************************************/
/**
 * generic range transformer: given a range acceptable to Boost.Range (e.g. a
 * standard container, an iterator pair, ...) and a unary function to apply to
 * each element of the range, make a corresponding range that lazily applies
 * that function to each element on dereferencing.
 */
template<typename FUNCTION, typename RANGE>
inline
boost::iterator_range<boost::transform_iterator<FUNCTION,
                                                typename boost::range_const_iterator<RANGE>::type> >
make_transform_range(const RANGE& range, FUNCTION function)
{
    // shorthand for the iterator type embedded in our return type
    typedef boost::transform_iterator<FUNCTION, typename boost::range_const_iterator<RANGE>::type>
        transform_iterator;
    return boost::make_iterator_range(transform_iterator(boost::begin(range), function),
                                      transform_iterator(boost::end(range),   function));
}

/// non-const version of make_transform_range()
template<typename FUNCTION, typename RANGE>
inline
boost::iterator_range<boost::transform_iterator<FUNCTION,
                                                typename boost::range_iterator<RANGE>::type> >
make_transform_range(RANGE& range, FUNCTION function)
{
    // shorthand for the iterator type embedded in our return type
    typedef boost::transform_iterator<FUNCTION, typename boost::range_iterator<RANGE>::type>
        transform_iterator;
    return boost::make_iterator_range(transform_iterator(boost::begin(range), function),
                                      transform_iterator(boost::end(range),   function));
}

/**
 * From any range compatible with Boost.Range, instantiate any class capable
 * of accepting an iterator pair.
 */
template<class TYPE>
struct instance_from_range: public TYPE
{
    template<typename RANGE>
    instance_from_range(RANGE range):
        TYPE(boost::begin(range), boost::end(range))
    {}
};

/*****************************************************************************
*   LLDependencies
*****************************************************************************/
/**
 * LLDependencies components that should not be reinstantiated for each KEY,
 * NODE specialization
 */
class LLDependenciesBase
{
public:
    virtual ~LLDependenciesBase() {}

    /**
     * Exception thrown by sort() if there's a cycle
     */
    struct Cycle: public std::runtime_error
    {
        Cycle(const std::string& what): std::runtime_error(what) {}
    };

    /**
     * Provide a short description of this LLDependencies instance on the
     * specified output stream, assuming that its KEY type has an operator<<()
     * that works with std::ostream.
     *
     * Pass @a full as @c false to omit any keys without dependency constraints.
     */
    virtual std::ostream& describe(std::ostream& out, bool full=true) const;

    /// describe() to a string
    virtual std::string describe(bool full=true) const;

protected:
    typedef std::vector< std::pair<int, int> > EdgeList;
    typedef std::vector<int> VertexList;
    VertexList topo_sort(int vertices, const EdgeList& edges) const;

    /**
     * refpair is specifically intended to capture a pair of references. This
     * is better than std::pair<T1&, T2&> because some implementations of
     * std::pair's ctor accept const references to the two types. If the
     * types are themselves references, this results in an illegal reference-
     * to-reference.
     */
    template<typename T1, typename T2>
    struct refpair
    {
        refpair(T1 value1, T2 value2):
            first(value1),
            second(value2)
        {}
        T1 first;
        T2 second;
    };
};

/// describe() helper: for most types, report the type as usual
template<typename T>
inline
std::ostream& LLDependencies_describe(std::ostream& out, const T& key)
{
    out << key;
    return out;
}

/// specialize LLDependencies_describe() for std::string
template<>
inline
std::ostream& LLDependencies_describe(std::ostream& out, const std::string& key)
{
    out << '"' << key << '"';
    return out;
}

/**
 * It's reasonable to use LLDependencies in a keys-only way, more or less like
 * std::set. For that case, the default NODE type is an empty struct.
 */
struct LLDependenciesEmpty
{
    LLDependenciesEmpty() {}
    /**
     * Give it a constructor accepting void* so caller can pass placeholder
     * values such as NULL or 0 rather than having to write
     * LLDependenciesEmpty().
     */
    LLDependenciesEmpty(void*) {}    
};

/**
 * This class manages abstract dependencies between node types of your
 * choosing. As with a std::map, nodes are copied when add()ed, so the node
 * type should be relatively lightweight; to manipulate dependencies between
 * expensive objects, use a pointer type.
 *
 * For a given node, you may state the keys of nodes that must precede it
 * and/or nodes that must follow it. The sort() method will produce an order
 * that should work, or throw an exception if the constraints are impossible.
 * We cache results to minimize the cost of repeated sort() calls.
 */
template<typename KEY = std::string,
         typename NODE = LLDependenciesEmpty>
class LLDependencies: public LLDependenciesBase
{
    typedef LLDependencies<KEY, NODE> self_type;

    /**
     * Internally, we bundle the client's NODE with its before/after keys.
     */
    struct DepNode
    {
        typedef std::set<KEY> dep_set;
        DepNode(const NODE& node_, const dep_set& after_, const dep_set& before_):
            node(node_),
            after(after_),
            before(before_)
        {}
        NODE node;
        dep_set after, before;    
    };
    typedef std::map<KEY, DepNode> DepNodeMap;
    typedef typename DepNodeMap::value_type DepNodeMapEntry;

    /// We have various ways to get the dependencies for a given DepNode.
    /// Rather than having to restate each one for 'after' and 'before'
    /// separately, pass a dep_selector so we can apply each to either.
    typedef boost::function<const typename DepNode::dep_set&(const DepNode&)> dep_selector;

public:
    LLDependencies() {}

    typedef KEY key_type;
    typedef NODE node_type;

    /// param type used to express lists of other node keys -- note that such
    /// lists can be initialized with boost::assign::list_of()
    typedef std::vector<KEY> KeyList;

    /**
     * Add a new node. State its dependencies on other nodes (which may not
     * yet have been added) by listing the keys of nodes this new one must
     * follow, and separately the keys of nodes this new one must precede.
     *
     * The node you pass is @em copied into an internal data structure. If you
     * want to modify the node value after add()ing it, capture the returned
     * NODE& reference.
     *
     * @note
     * Actual dependency analysis is deferred to the sort() method, so 
     * you can add an arbitrary number of nodes without incurring analysis
     * overhead for each. The flip side of this is that add()ing nodes that
     * define a cycle leaves this object in a state in which sort() will
     * always throw the Cycle exception.
     *
     * Two distinct use cases are anticipated:
     * * The data used to load this object are completely known at compile
     * time (e.g. LLEventPump listener names). A Cycle exception represents a
     * bug which can be corrected by the coder. The program need neither catch
     * Cycle nor attempt to salvage the state of this object.
     * * The data are loaded at runtime, therefore the universe of
     * dependencies cannot be known at compile time. The client code should
     * catch Cycle.
     * ** If a Cycle exception indicates fatally-flawed input data, this
     * object can simply be discarded, possibly with the entire program run.
     * ** If it is essential to restore this object to a working state, the
     * simplest workaround is to remove() nodes in LIFO order.
     * *** It may be useful to add functionality to this class to track the
     * add() chronology, providing a pop() method to remove the most recently
     * added node.
     * *** It may further be useful to add a restore() method which would
     * pop() until sort() no longer throws Cycle. This method would be
     * expensive -- but it's not clear that client code could resolve the
     * problem more cheaply.
     */
    NODE& add(const KEY& key, const NODE& node = NODE(),
              const KeyList& after = KeyList(),
              const KeyList& before = KeyList())
    {
        // Get the passed-in lists as sets for equality comparison
        typename DepNode::dep_set
            after_set(after.begin(), after.end()),
            before_set(before.begin(), before.end());
        // Try to insert the new node; if it already exists, find the old
        // node instead.
        std::pair<typename DepNodeMap::iterator, bool> inserted =
            mNodes.insert(typename DepNodeMap::value_type(key,
                                                          DepNode(node, after_set, before_set)));
        if (! inserted.second)      // bool indicating success of insert()
        {
            // We already have a node by this name. Have its dependencies
            // changed? If the existing node's dependencies are identical, the
            // result will be unchanged, so we can leave the cache intact.
            // Regardless of inserted.second, inserted.first is the iterator
            // to the newly-inserted (or existing) map entry. Of course, that
            // entry's second is the DepNode of interest.
            if (inserted.first->second.after  != after_set ||
                inserted.first->second.before != before_set)
            {
                // Dependencies have changed: clear the cached result.
                mCache.clear();
                // save the new dependencies
                inserted.first->second.after  = after_set;
                inserted.first->second.before = before_set;
            }
        }
        else                        // this node is new
        {
            // This will change results.
            mCache.clear();
        }
        return inserted.first->second.node;
    }

    /// the value of an iterator, showing both KEY and its NODE
    typedef refpair<const KEY&, NODE&> value_type;
    /// the value of a const_iterator
    typedef refpair<const KEY&, const NODE&> const_value_type;

private:
    // Extract functors
    static value_type value_extract(DepNodeMapEntry& entry)
    {
        return value_type(entry.first, entry.second.node);
    }

    static const_value_type const_value_extract(const DepNodeMapEntry& entry)
    {
        return const_value_type(entry.first, entry.second.node);
    }

    // All the iterator access methods return iterator ranges just to cut down
    // on the friggin' boilerplate!!

    /// generic mNodes range method
    template<typename ITERATOR, typename FUNCTION>
    boost::iterator_range<ITERATOR> generic_range(FUNCTION function)
    {
        return make_transform_range(mNodes, function);
    }

    /// generic mNodes const range method
    template<typename ITERATOR, typename FUNCTION>
    boost::iterator_range<ITERATOR> generic_range(FUNCTION function) const
    {
        return make_transform_range(mNodes, function);
    }

public:
    /// iterator over value_type entries
    typedef boost::transform_iterator<boost::function<value_type(DepNodeMapEntry&)>,
                                      typename DepNodeMap::iterator> iterator;
    /// range over value_type entries
    typedef boost::iterator_range<iterator> range;

    /// iterate over value_type <i>in @c KEY order</i> rather than dependency order
    range get_range()
    {
        return generic_range<iterator>(value_extract);
    }

    /// iterator over const_value_type entries
    typedef boost::transform_iterator<boost::function<const_value_type(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_iterator;
    /// range over const_value_type entries
    typedef boost::iterator_range<const_iterator> const_range;

    /// iterate over const_value_type <i>in @c KEY order</i> rather than dependency order
    const_range get_range() const
    {
        return generic_range<const_iterator>(const_value_extract);
    }

    /// iterator over stored NODEs
    typedef boost::transform_iterator<boost::function<NODE&(DepNodeMapEntry&)>,
                                      typename DepNodeMap::iterator> node_iterator;
    /// range over stored NODEs
    typedef boost::iterator_range<node_iterator> node_range;

    /// iterate over NODE <i>in @c KEY order</i> rather than dependency order
    node_range get_node_range()
    {
        // First take a DepNodeMapEntry and extract a reference to its
        // DepNode, then from that extract a reference to its NODE.
        return generic_range<node_iterator>(
            boost::bind<NODE&>(&DepNode::node,
                               boost::bind<DepNode&>(&DepNodeMapEntry::second, _1)));
    }

    /// const iterator over stored NODEs
    typedef boost::transform_iterator<boost::function<const NODE&(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_node_iterator;
    /// const range over stored NODEs
    typedef boost::iterator_range<const_node_iterator> const_node_range;

    /// iterate over const NODE <i>in @c KEY order</i> rather than dependency order
    const_node_range get_node_range() const
    {
        // First take a DepNodeMapEntry and extract a reference to its
        // DepNode, then from that extract a reference to its NODE.
        return generic_range<const_node_iterator>(
            boost::bind<const NODE&>(&DepNode::node,
                                     boost::bind<const DepNode&>(&DepNodeMapEntry::second, _1)));
    }

    /// const iterator over stored KEYs
    typedef boost::transform_iterator<boost::function<const KEY&(const DepNodeMapEntry&)>,
                                      typename DepNodeMap::const_iterator> const_key_iterator;
    /// const range over stored KEYs
    typedef boost::iterator_range<const_key_iterator> const_key_range;
    // We don't provide a non-const iterator over KEYs because they should be
    // immutable, and in fact our underlying std::map won't give us non-const
    // references.

    /// iterate over const KEY <i>in @c KEY order</i> rather than dependency order
    const_key_range get_key_range() const
    {
        // From a DepNodeMapEntry, extract a reference to its KEY.
        return generic_range<const_key_iterator>(
            boost::bind<const KEY&>(&DepNodeMapEntry::first, _1));
    }

    /**
     * Find an existing NODE, or return NULL. We decided to avoid providing a
     * method analogous to std::map::find(), for a couple of reasons:
     *
     * * For a find-by-key, getting back an iterator to the (key, value) pair
     * is less than useful, since you already have the key in hand.
     * * For a failed request, comparing to end() is problematic. First, we
     * provide range accessors, so it's more code to get end(). Second, we
     * provide a number of different ranges -- quick, to which one's end()
     * should we compare the iterator returned by find()?
     *
     * The returned pointer is solely to allow expressing the not-found
     * condition. LLDependencies still owns the found NODE.
     */
    const NODE* get(const KEY& key) const
    {
        typename DepNodeMap::const_iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            return &found->second.node;
        }
        return NULL;
    }

    /**
     * non-const get()
     */
    NODE* get(const KEY& key)
    {
        // Use const implementation, then cast away const-ness of return
        return const_cast<NODE*>(const_cast<const self_type*>(this)->get(key));
    }

    /**
     * Remove a node with specified key. This operation is the major reason
     * we rebuild the graph on the fly instead of storing it.
     */
    bool remove(const KEY& key)
    {
        typename DepNodeMap::iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            mNodes.erase(found);
            return true;
        }
        return false;
    }

private:
    /// cached list of iterators
    typedef std::vector<iterator> iterator_list;
    typedef typename iterator_list::iterator iterator_list_iterator;

public:
    /**
     * The return type of the sort() method needs some explanation. Provide a
     * public typedef to facilitate storing the result.
     *
     * * We will prepare mCache by looking up DepNodeMap iterators.
     * * We want to return a range containing iterators that will walk mCache.
     * * If we simply stored DepNodeMap iterators and returned
     * (mCache.begin(), mCache.end()), dereferencing each iterator would
     * obtain a DepNodeMap iterator.
     * * We want the caller to loop over @c value_type: pair<KEY, NODE>.
     * * This requires two transformations:
     * ** mCache must contain @c LLDependencies::iterator so that
     * dereferencing each entry will obtain an @c LLDependencies::value_type
     * rather than a DepNodeMapEntry.
     * ** We must wrap mCache's iterators in boost::indirect_iterator so that
     * dereferencing one of our returned iterators will also dereference the
     * iterator contained in mCache.
     */
    typedef boost::iterator_range<boost::indirect_iterator<iterator_list_iterator> > sorted_range;
    /// for convenience in looping over a sorted_range
    typedef typename sorted_range::iterator sorted_iterator;

    /**
     * Once we've loaded in the dependencies of interest, arrange them into an
     * order that works -- or throw Cycle exception.
     *
     * Return an iterator range over (key, node) pairs that traverses them in
     * the desired order.
     */
    sorted_range sort() const
    {
        // Changes to mNodes cause us to clear our cache, so empty mCache
        // means it's invalid and should be recomputed. However, if mNodes is
        // also empty, then an empty mCache represents a valid order, so don't
        // bother sorting.
        if (mCache.empty() && ! mNodes.empty())
        {
            // Construct a map of node keys to distinct vertex numbers -- even for
            // nodes mentioned only in before/after constraints, that haven't yet
            // been explicitly added. Rely on std::map rejecting a second attempt
            // to insert the same key. Use the map's size() as the vertex number
            // to get a distinct value for each successful insertion.
            typedef std::map<KEY, int> VertexMap;
            VertexMap vmap;
            // Nest each of these loops because !@#$%? MSVC warns us that its
            // former broken behavior has finally been fixed -- and our builds
            // treat warnings as errors.
            {
                for (typename DepNodeMap::const_iterator nmi = mNodes.begin(), nmend = mNodes.end();
                     nmi != nmend; ++nmi)
                {
                    vmap.insert(typename VertexMap::value_type(nmi->first, vmap.size()));
                    for (typename DepNode::dep_set::const_iterator ai = nmi->second.after.begin(),
                                                                   aend = nmi->second.after.end();
                         ai != aend; ++ai)
                    {
                        vmap.insert(typename VertexMap::value_type(*ai, vmap.size()));
                    }
                    for (typename DepNode::dep_set::const_iterator bi = nmi->second.before.begin(),
                                                                   bend = nmi->second.before.end();
                         bi != bend; ++bi)
                    {
                        vmap.insert(typename VertexMap::value_type(*bi, vmap.size()));
                    }
                }
            }
            // Define the edges. For this we must traverse mNodes again, mapping
            // all the known key dependencies to integer pairs.
            EdgeList edges;
            {
                for (typename DepNodeMap::const_iterator nmi = mNodes.begin(), nmend = mNodes.end();
                     nmi != nmend; ++nmi)
                {
                    int thisnode = vmap[nmi->first];
                    // after dependencies: build edges from the named node to this one
                    for (typename DepNode::dep_set::const_iterator ai = nmi->second.after.begin(),
                                                                   aend = nmi->second.after.end();
                         ai != aend; ++ai)
                    {
                        edges.push_back(EdgeList::value_type(vmap[*ai], thisnode));
                    }
                    // before dependencies: build edges from this node to the
                    // named one
                    for (typename DepNode::dep_set::const_iterator bi = nmi->second.before.begin(),
                                                                   bend = nmi->second.before.end();
                         bi != bend; ++bi)
                    {
                        edges.push_back(EdgeList::value_type(thisnode, vmap[*bi]));
                    }
                }
            }
            // Hide the gory details of our topological sort, since they shouldn't
            // get reinstantiated for each distinct NODE type.
            VertexList sorted(topo_sort(vmap.size(), edges));
            // Build the reverse of vmap to look up the key for each vertex
            // descriptor. vmap contains exactly one entry for each distinct key,
            // and we're certain that the associated int values are distinct
            // indexes. The fact that they're not in order is irrelevant.
            KeyList vkeys(vmap.size());
            for (typename VertexMap::const_iterator vmi = vmap.begin(), vmend = vmap.end();
                 vmi != vmend; ++vmi)
            {
                vkeys[vmi->second] = vmi->first;
            }
            // Walk the sorted output list, building the result into mCache so
            // we'll have it next time someone asks.
            mCache.clear();
            for (VertexList::const_iterator svi = sorted.begin(), svend = sorted.end();
                 svi != svend; ++svi)
            {
                // We're certain that vkeys[*svi] exists. However, there might not
                // yet be a corresponding entry in mNodes.
                self_type* non_const_this(const_cast<self_type*>(this));
                typename DepNodeMap::iterator found = non_const_this->mNodes.find(vkeys[*svi]);
                if (found != non_const_this->mNodes.end())
                {
                    // Make an iterator of appropriate type.
                    mCache.push_back(iterator(found, value_extract));
                }
            }
        }
        // Whether or not we've just recomputed mCache, it should now contain
        // the results we want. Return a range of indirect_iterators over it
        // so that dereferencing a returned iterator will dereference the
        // iterator stored in mCache and directly reference the (key, node)
        // pair.
        boost::indirect_iterator<iterator_list_iterator>
            begin(mCache.begin()),
            end(mCache.end());
        return sorted_range(begin, end);
    }

    /// Override base-class describe() with actual implementation
    virtual std::ostream& describe(std::ostream& out, bool full=true) const
    {
        typename DepNodeMap::const_iterator dmi(mNodes.begin()), dmend(mNodes.end());
        if (dmi != dmend)
        {
            std::string sep;
            describe(out, sep, *dmi, full);
            while (++dmi != dmend)
            {
                describe(out, sep, *dmi, full);
            }
        }
        return out;
    }

    /// describe() helper: report a DepNodeEntry
    static std::ostream& describe(std::ostream& out, std::string& sep,
                                  const DepNodeMapEntry& entry, bool full)
    {
        // If we were asked for a full report, describe every node regardless
        // of whether it has dependencies. If we were asked to suppress
        // independent nodes, describe this one if either after or before is
        // non-empty.
        if (full || (! entry.second.after.empty()) || (! entry.second.before.empty()))
        {
            out << sep;
            sep = "\n";
            if (! entry.second.after.empty())
            {
                out << "after ";
                describe(out, entry.second.after);
                out << " -> ";
            }
            LLDependencies_describe(out, entry.first);
            if (! entry.second.before.empty())
            {
                out << " -> before ";
                describe(out, entry.second.before);
            }
        }
        return out;
    }

    /// describe() helper: report a dep_set
    static std::ostream& describe(std::ostream& out, const typename DepNode::dep_set& keys)
    {
        out << '(';
        typename DepNode::dep_set::const_iterator ki(keys.begin()), kend(keys.end());
        if (ki != kend)
        {
            LLDependencies_describe(out, *ki);
            while (++ki != kend)
            {
                out << ", ";
                LLDependencies_describe(out, *ki);
            }
        }
        out << ')';
        return out;
    }

    /// Iterator over the before/after KEYs on which a given NODE depends
    typedef typename DepNode::dep_set::const_iterator dep_iterator;
    /// range over the before/after KEYs on which a given NODE depends
    typedef boost::iterator_range<dep_iterator> dep_range;

    /// dependencies access from key
    dep_range get_dep_range_from_key(const KEY& key, const dep_selector& selector) const
    {
        typename DepNodeMap::const_iterator found = mNodes.find(key);
        if (found != mNodes.end())
        {
            return dep_range(selector(found->second));
        }
        // We want to return an empty range. On some platforms a default-
        // constructed range (e.g. dep_range()) does NOT suffice! The client
        // is likely to try to iterate from boost::begin(range) to
        // boost::end(range); yet these iterators might not be valid. Instead
        // return a range over a valid, empty container.
        static const typename DepNode::dep_set empty_deps;
        return dep_range(empty_deps.begin(), empty_deps.end());
    }

    /// dependencies access from any one of our key-order iterators
    template<typename ITERATOR>
    dep_range get_dep_range_from_xform(const ITERATOR& iterator, const dep_selector& selector) const
    {
        return dep_range(selector(iterator.base()->second));
    }

    /// dependencies access from sorted_iterator
    dep_range get_dep_range_from_sorted(const sorted_iterator& sortiter,
                                        const dep_selector& selector) const
    {
        // sorted_iterator is a boost::indirect_iterator wrapping an mCache
        // iterator, which we can obtain by sortiter.base(). Deferencing that
        // gets us an mCache entry, an 'iterator' -- one of our traversal
        // iterators -- on which we can use get_dep_range_from_xform().
        return get_dep_range_from_xform(*sortiter.base(), selector);
    }

    /**
     * Get a range over the after KEYs stored for the passed KEY or iterator,
     * in <i>arbitrary order.</i> If you pass a nonexistent KEY, returns empty
     * range -- same as a KEY with no after KEYs. Detect existence of a KEY
     * using get() instead.
     */
    template<typename KEY_OR_ITER>
    dep_range get_after_range(const KEY_OR_ITER& key) const;

    /**
     * Get a range over the before KEYs stored for the passed KEY or iterator,
     * in <i>arbitrary order.</i> If you pass a nonexistent KEY, returns empty
     * range -- same as a KEY with no before KEYs. Detect existence of a KEY
     * using get() instead.
     */
    template<typename KEY_OR_ITER>
    dep_range get_before_range(const KEY_OR_ITER& key) const;

private:
    DepNodeMap mNodes;
    mutable iterator_list mCache;
};

/**
 * Functor to get a dep_range from a KEY or iterator -- generic case. If the
 * passed value isn't one of our iterator specializations, assume it's
 * convertible to the KEY type.
 */
template<typename KEY_ITER>
struct LLDependencies_dep_range_from
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const KEY_ITER& key,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_key(key, selector);
    }
};

/// Specialize LLDependencies_dep_range_from for our key-order iterators
template<typename FUNCTION, typename ITERATOR>
struct LLDependencies_dep_range_from< boost::transform_iterator<FUNCTION, ITERATOR> >
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const boost::transform_iterator<FUNCTION, ITERATOR>& iter,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_xform(iter, selector);
    }
};

/// Specialize LLDependencies_dep_range_from for sorted_iterator
template<typename BASEITER>
struct LLDependencies_dep_range_from< boost::indirect_iterator<BASEITER> >
{
    template<typename KEY, typename NODE, typename SELECTOR>
    typename LLDependencies<KEY, NODE>::dep_range
    operator()(const LLDependencies<KEY, NODE>& deps,
               const boost::indirect_iterator<BASEITER>& iter,
               const SELECTOR& selector)
    {
        return deps.get_dep_range_from_sorted(iter, selector);
    }
};

/// generic get_after_range() implementation
template<typename KEY, typename NODE>
template<typename KEY_OR_ITER>
typename LLDependencies<KEY, NODE>::dep_range
LLDependencies<KEY, NODE>::get_after_range(const KEY_OR_ITER& key_iter) const
{
    return LLDependencies_dep_range_from<KEY_OR_ITER>()(
        *this,
        key_iter,
        boost::bind<const typename DepNode::dep_set&>(&DepNode::after, _1));
}

/// generic get_before_range() implementation
template<typename KEY, typename NODE>
template<typename KEY_OR_ITER>
typename LLDependencies<KEY, NODE>::dep_range
LLDependencies<KEY, NODE>::get_before_range(const KEY_OR_ITER& key_iter) const
{
    return LLDependencies_dep_range_from<KEY_OR_ITER>()(
        *this,
        key_iter,
        boost::bind<const typename DepNode::dep_set&>(&DepNode::before, _1));
}

#endif /* ! defined(LL_LLDEPENDENCIES_H) */
