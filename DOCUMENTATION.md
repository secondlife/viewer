# Documentation Style Guide

This document outlines the documentation standards for the Second Life Viewer project.

## Philosophy

Our documentation should be **conversational, practical, and engineer-focused**. We write for engineers who want to understand not just *what* the code does, but *why* it exists and *how* to use it effectively. This is especially important for our open source program, and third party viewer projects.

## Tone and Voice

### Conversational and Approachable
- Use phrases like "Think of this as...", "This is the thing that makes...", "Great for..."
- Write like you're explaining to a friend, not reading from a technical manual.  Many viewer developers are doing this in their free time, and out of love for Second Life.
- It's okay to be somewhat casual: "You should probably use this for..." or "This is like using a sledge hammer for...".  Avoid vague "magical" statements.
- Don't get cocky: just because it meets your criteria on your system does not mean it does on everyone else's.
- Assume that people will use it out of spec. Be respectful but firm on the use cases for a given piece of code.

### Practical and Grounded
- Connect abstract concepts to real-world use cases
- Explain the "why" behind design decisions
- Include performance implications and trade-offs
- Mention common pitfalls or gotchas

### Examples of Good Tone:
```cpp
/**
 * @brief Checks if a handle still points to a valid node.
 * 
 * Just because you have a handle doesn't mean the node still exists. This method
 * tells you if the handle is still good to use. Always check this before accessing
 * node data if you're not sure about the handle's validity.
 */
```

## Structure and Format

### Doxygen Style Headers
Every public API should have a complete doxygen comment with:

```cpp
/**
 * @brief One-line summary that completes "This function..."
 * 
 * Detailed explanation that covers:
 * - What this does in practical terms
 * - Why you'd want to use it
 * - Any important behavior or constraints
 * - Performance considerations if relevant
 * 
 * @param paramName Description of what this parameter controls
 * @return Description of what you get back and when it might be null/empty
 * @throws ExceptionType When and why this might throw
 * 
 * @code
 * // Practical usage example
 * auto result = someFunction(exampleParam);
 * if (result.has_value()) {
 *     // What to do with the result
 * }
 * @endcode
 */
```

As a rule, you should avoid duplicate documentation between the header and implementation.  If there's a unique or special behavior for a given method, it should be documented in the header.

### Required Elements

1. **@brief** - Concise but complete summary
2. **Detailed explanation** - 2-4 sentences explaining practical usage
3. **@param/@return** - For all parameters and return values
4. **@throws** - For any exceptions that might be thrown
5. **@code example** - Real, compilable usage example

## Content Guidelines

### What to Document

**Everything public:**
- Classes and their purpose
- All public methods and their parameters
- Public member variables and type aliases (with inline `/// comments`)
- Iterators and their behavior
- Helper functions and utilities

**Everything private (equally important):**
- All private methods with full documentation
- Private member variables (with inline `/// comments`)
- Internal data structures and their invariants
- Helper classes and implementation details
- Complex algorithms and their rationale

Private APIs often contain the most critical and complex logic. Future maintainers need to understand not just what private methods do, but why they exist, what assumptions they make, and why they're not exposed publicly.

**Static, const, etc. variables and methods**
- Describe why things are static
- Describe why a variable is a constant

These things are all great to use when you have a great use case for them. Let others know why to avoid potential disagreements in PRs and so on later.

Even methods should get this treatment.  Why is a given method static?  Why isn't it an instance method?  Etc.

**Implementation quirks, specifics**
- Describe why, for example, you are allocating a large chunk of memory in a seemingly random location
- Provide dicussion around why a given block does a unique thing that might not seem obvious at first

These should be in the actual implementations themselves.  Avoid having tons of implementation specific descriptions in the header documentation if you can.

### What to Emphasize

**Engineer Intent:**
- What problem does this solve?
- When would you choose this over alternatives?
- What are the common use patterns?

**Practical Details:**
- Performance characteristics
- Memory usage implications
- Thread safety guarantees
- Error conditions and handling

**Implementation Insights:**
- Why specific design choices were made
- How complex algorithms work at a high level
- Important invariants or assumptions
- Behavioral quirks and details

### Code Examples

Save this for more complex methods that aren't immediately obvious in how they should be utilized:

```cpp
/**
 * @code
 * // Create a task graph
 * AcyclicGraph<Task> taskGraph;
 * auto compile = taskGraph.addNode(CompileTask{}, "compile");
 * auto link = taskGraph.addNode(LinkTask{}, "link");
 * 
 * // Set up dependencies
 * taskGraph.addDependency(link, compile);  // Link depends on compile
 * 
 * // Execute in order
 * auto order = taskGraph.getTopologicalOrder();
 * for (auto task : order) {
 *     executeTask(task);
 * }
 * @endcode
 */
```

**Good examples:**
- Show real, practical usage
- Demonstrate the most common use case
- Include error handling where relevant
- Use meaningful variable names

## Specific Patterns

### Private Methods and Internal APIs

Private methods deserve the same level of documentation as public ones, with additional emphasis on:

```cpp
/**
 * @brief What this internal method accomplishes within the system.
 * 
 * Detailed explanation covering:
 * - Why this functionality is needed internally
 * - What assumptions it makes about system state
 * - Why this is private rather than public (safety, complexity, etc.)
 * - How it fits into the larger internal workflow
 * - Any invariants it maintains or requires
 * 
 * This method is private because [specific reason - too low-level, unsafe
 * without proper setup, implementation detail that might change, etc.]
 * 
 * @param param What this parameter controls in the internal context
 * @return What gets returned and how it's used internally
 * 
 * @code
 * // Example showing how this is used internally
 * // (May reference private state or other private methods)
 * @endcode
 */
```

**Key elements for private API documentation:**
- **Why it's private** - Explain the specific reasons (complexity, safety, implementation detail)
- **Internal context** - How it fits into private workflows
- **Assumptions** - What state the object must be in, what's been validated already
- **Invariants** - What this method maintains or requires
- **Future maintainers** - What someone modifying this needs to know

### Classes and Major Components

```cpp
/**
 * @brief One-sentence description of what this class represents.
 * 
 * Longer explanation that covers:
 * - What real-world problem this solves
 * - Key design principles or guarantees
 * - How it fits into the larger system
 * 
 * The explanation should make someone say "oh, I need this for..."
 * rather than "what does this do?"
 * 
 * @tparam TemplateParam Description of template parameters
 * 
 * @code
 * // Complete usage example showing the main workflow
 * @endcode
 */
```

### Methods with Complex Behavior

```cpp
/**
 * @brief What this method accomplishes for the user.
 * 
 * Explanation of how it works, including:
 * - Important behavior that might be surprising
 * - Performance characteristics
 * - When you'd use this vs alternatives
 * 
 * @param param Description focusing on what the user provides
 * @return What the user gets back and what it means
 * 
 * @code
 * // Example showing typical usage
 * @endcode
 */
```

### Member Variables

```cpp
std::vector<Node> nodes;                    /// Brief description of what this contains
std::vector<uint32_t> free_indices;         /// Why this exists and how it's used  
std::vector<uint32_t> slot_generations;     /// Key insight about this design choice
```

### Iterators and Complex APIs

For iterators, document:
- STL compatibility requirements
- What iteration order means
- How invalid elements are handled
- Performance characteristics
- Typical usage patterns

## Quality Checklist

### For Every Public Method:
- [ ] Clear @brief that explains the user benefit
- [ ] Detailed explanation covering the "why"
- [ ] All parameters documented with user perspective
- [ ] Return value explained including edge cases
- [ ] Practical code example
- [ ] Performance notes if relevant

### For Every Private Method:
- [ ] Clear @brief explaining internal purpose
- [ ] Why this is private rather than public
- [ ] What assumptions it makes about object state
- [ ] How it fits into internal workflows
- [ ] What invariants it maintains or requires
- [ ] Internal usage example if complex

### For Every Class:
- [ ] Purpose explained in terms of user problems
- [ ] Key design principles mentioned
- [ ] Template parameters explained
- [ ] Main usage pattern demonstrated
- [ ] Relationship to other components noted
- [ ] Internal architecture overview for complex classes

### For the Overall File:
- [ ] All public APIs documented
- [ ] All private methods documented
- [ ] Member variables have inline documentation
- [ ] Internal data structures explained
- [ ] File-level overview explaining the component's role

## Anti-Patterns to Avoid

**Don't write encyclopedia entries:**
```cpp
// Bad: Technical but not helpful
/**
 * @brief Implements Kahn's algorithm for topological sorting of directed acyclic graphs.
 */

// Good: Explains the user benefit
/**
 * @brief Returns nodes in topological order (dependencies before dependents).
 * 
 * This is useful for dependency graphs. You get back a list where every
 * node comes after its dependencies. Perfect for execution ordering, build systems,
 * or any time you need to process things in dependency order.
 */
```

**Don't just repeat the function name:**
```cpp
// Bad: Says nothing useful
/**
 * @brief Gets the node count.
 */

// Good: Explains what the count means
/**
 * @brief Gets the number of valid nodes in the graph.
 * 
 * @return The count of nodes that are actually valid and usable.
 * Doesn't count nodes that have been removed but whose slots haven't
 * been reused yet.
 */
```

## Tools and Automation

- Use Doxygen-compatible comments for consistency
- Consider automated checks for missing documentation
- Review documentation during code review with the same rigor as code
- Test that examples actually compile and work

## Conclusion

Good documentation is a conversation with future users (including yourself). Write like you're helping someone solve a real problem, not just describing what the code does. The extra effort to make documentation conversational and practical pays dividends in code maintainability and developer productivity.
