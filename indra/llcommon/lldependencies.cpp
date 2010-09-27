/**
 * @file   lldependencies.cpp
 * @author Nat Goodspeed
 * @date   2008-09-17
 * @brief  Implementation for lldependencies.
 * 
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * Copyright (c) 2008, Linden Research, Inc.
 * $/LicenseInfo$
 */

// Precompiled header
#include "linden_common.h"
// associated header
#include "lldependencies.h"
// STL headers
#include <map>
#include <sstream>
// std headers
// external library headers
#include <boost/graph/graph_traits.hpp>  // for boost::graph_traits
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/exception.hpp>
// other Linden headers

LLDependenciesBase::VertexList LLDependenciesBase::topo_sort(int vertices, const EdgeList& edges) const
{
    // Construct a Boost Graph Library graph according to the constraints
    // we've collected. It seems as though we ought to be able to capture
    // the uniqueness of vertex keys using a setS of vertices with a
    // string property -- but I don't yet understand adjacency_list well
    // enough to get there. All the examples I've seen so far use integers
    // for vertices.
    // Define the Graph type. Use a vector for vertices so we can use the
    // default topological_sort vertex lookup by int index. Use a set for
    // edges because the same dependency may be stated twice: Node "a" may
    // specify that it must precede "b", while "b" may also state that it
    // must follow "a".
    typedef boost::adjacency_list<boost::setS, boost::vecS, boost::directedS,
                                  boost::no_property> Graph;
    // Instantiate the graph. Without vertex properties, we need say no
    // more about vertices than the total number.
    Graph g(edges.begin(), edges.end(), vertices);
    // topo sort
    typedef boost::graph_traits<Graph>::vertex_descriptor VertexDesc;
    typedef std::vector<VertexDesc> SortedList;
    SortedList sorted;
    // note that it throws not_a_dag if it finds a cycle
    try
    {
        boost::topological_sort(g, std::back_inserter(sorted));
    }
    catch (const boost::not_a_dag& e)
    {
        // translate to the exception we define
        std::ostringstream out;
        out << "LLDependencies cycle: " << e.what() << '\n';
        // Omit independent nodes: display only those that might contribute to
        // the cycle.
        describe(out, false);
        throw Cycle(out.str());
    }
    // A peculiarity of boost::topological_sort() is that it emits results in
    // REVERSE topological order: to get the result you want, you must
    // traverse the SortedList using reverse iterators.
    return VertexList(sorted.rbegin(), sorted.rend());
}

std::ostream& LLDependenciesBase::describe(std::ostream& out, bool full) const
{
    // Should never encounter this base-class implementation; may mean that
    // the KEY type doesn't have a suitable ostream operator<<().
    out << "<no description available>";
    return out;
}

std::string LLDependenciesBase::describe(bool full) const
{
    // Just use the ostream-based describe() on a std::ostringstream. The
    // implementation is here mostly so that we can avoid #include <sstream>
    // in the header file.
    std::ostringstream out;
    describe(out, full);
    return out.str();
}
