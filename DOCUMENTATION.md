# Documentation Style Guide

This document outlines the documentation standards for the Second Life Viewer project.

## Philosophy

Our documentation should be **conversational, practical, and engineer-focused**. We write for engineers who want to understand not just *what* the code does, but *why* it exists and *how* to use it effectively. This is especially important for our open source program, and third party viewer projects.

## Tone and Voice

### Conversational and Approachable
- Use clear, direct language: "This manages...", "Use this when...", "This handles..."
- Write like you're explaining to a colleague, not reading from a technical manual. Many viewer developers are doing this in their free time, and out of love for Second Life.
- Be friendly but professional: "Consider using this for..." or "This is designed for..."
- Avoid vague language like "magical" or "where the magic happens" - be specific about what code does
- Be mindful of platform differences: behavior on your system may differ on other configurations
- Assume that people will use it out of spec. Be respectful but clear about intended use cases.

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
 * Handles can become invalid when their target nodes are destroyed. This method
 * verifies handle validity before use. Always check this before accessing
 * node data when the handle's validity is uncertain.
 * 
 * @return true if the handle points to a valid node, false otherwise
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

As a rule, you should avoid duplicate documentation between the header and implementation. The header should document what the API does and how to use it, while the implementation file should document specific implementation details, optimizations, and non-obvious behaviors.

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
- Inline comments explaining non-obvious code sections
- Why specific buffer sizes or magic numbers were chosen
- Algorithmic optimizations and their trade-offs
- Workarounds for known issues or limitations
- Important behavioral details not visible from the API

These should be documented as inline comments close to the relevant code. Avoid speculation - only document what you can verify from the code or existing comments.

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

### Implementation Comments

For inline comments in implementation files:
- Explain non-obvious algorithms and optimizations
- Document performance trade-offs
- Keep comments concise and factual
- Add context for workarounds or unusual approaches

Example:
```cpp
// Combine IP:port into single 64-bit key for map lookup
U64 ipport = (((U64)ip) << 32) | (U64)port;

// Remove by moving last element to this object's position (swap-and-pop for O(1) removal)
mActiveObjects[idx] = mActiveObjects[last_index];

// Stack-allocated buffer to avoid heap fragmentation from frequent allocations
U8 compressed_dpbuffer[2048];
```

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

### API Documentation vs Implementation Documentation

**For public APIs:**
- Document the public contract - what it does, how to use it
- Include usage examples for complex APIs
- Explain preconditions, postconditions, and invariants
- Focus on the "what" and "why" from a user perspective

**For implementations:**
- Document implementation-specific details
- Explain algorithmic choices and optimizations
- Add inline comments for non-obvious code sections
- Focus on the "how" and internal "why"

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

## Documentation Based on Real Usage Patterns

When documenting existing systems, **ground your documentation in actual implementation details** rather than theoretical design. This creates more valuable, accurate documentation that reflects how the code actually works.

### Research Real Usage First

Before writing documentation, **search the codebase** to understand:
- How the classes are actually instantiated and initialized
- What the most common method calls are and in what contexts  
- How lifecycle methods are actually triggered
- What performance optimizations exist in practice
- How error handling works in real scenarios
- How the component integrates with the broader system

### Document Actual Patterns, Not Idealized Ones

**Bad approach:**
```cpp
/**
 * @brief Motion registry for animation system.
 * 
 * @code
 * // Theoretical usage
 * LLMotionRegistry registry;
 * registry.registerMotion(uuid, constructor);
 * @endcode
 */
```

**Good approach:**
```cpp
/**
 * @brief Factory registry for creating motion instances by UUID.
 * 
 * All motion types are registered once during the first avatar initialization
 * to populate the global registry shared by all characters.
 * 
 * @code
 * // Actual registration pattern in LLVOAvatar::initInstance()
 * if (LLCharacter::sInstances.size() == 1) {
 *     registerMotion(ANIM_AGENT_WALK, LLKeyframeWalkMotion::create);
 *     registerMotion(ANIM_AGENT_TARGET, LLTargetingMotion::create);
 *     // ... ~40 more motion types registered once globally
 * }
 * @endcode
 */
```

### Include Real Performance Details

Document actual performance characteristics found in the codebase:
- Specific limits (e.g., "32-motion instance limit with aggressive purging")
- Actual optimization thresholds (e.g., "MIN_PIXEL_AREA_EYE: 25000.f for close-up only")
- Real memory management patterns (e.g., "purgeExcessMotions() called every frame")
- Performance trade-offs that exist in practice

### Reference Stable API Elements

**Do reference:**
- ✅ Method names (`LLMotionController::updateMotions()`)
- ✅ Class names (`LLVOAvatar::initInstance()`)
- ✅ Constant names (`MAX_MOTION_INSTANCES`)
- ✅ Enum values (`STATUS_HOLD`, `ADDITIVE_BLEND`)

**Don't reference:**
- ❌ Specific file paths (`/Users/dev/llvoavatar.cpp`)
- ❌ Line number ranges (`lines 1208-1262`)
- ❌ File organization details that can change

Method and class names are much more stable than file locations and provide valuable API context for developers.

### Show Real Error Handling

Document actual error handling patterns, not idealized ones:

```cpp
/**
 * @brief Initializes motion with robust error handling.
 * 
 * @code
 * LLMotionInitStatus status = motion->onInitialize(character);
 * switch (status) {
 *     case STATUS_FAILURE:
 *         LL_INFOS() << "Motion " << id << " init failed." << LL_ENDL;
 *         sRegistry.markBad(id);  // Blacklist to prevent future attempts
 *         delete motion;
 *         return nullptr;
 *     case STATUS_HOLD:
 *         mLoadingMotions.insert(motion);  // Retry next frame
 *         break;
 *     case STATUS_SUCCESS:
 *         mLoadedMotions.insert(motion);   // Ready for activation
 *         break;
 * }
 * @endcode
 */
```

### Update Documentation Iteratively

After documenting based on research:
1. **Cross-reference with actual usage** - Search for how the documented classes are really used
2. **Update with real patterns** - Replace theoretical examples with actual usage patterns  
3. **Add missing performance details** - Include the optimizations and limits you discover
4. **Document edge cases** - Include the error handling and edge cases that actually exist

This approach creates documentation that serves as a true guide to how the system works, rather than how it might theoretically work.

## Working with Existing Code

When documenting legacy code:
- **Preserve existing comments** - They often contain valuable historical context
- **Don't replace, augment** - Add new documentation alongside existing comments
- **Verify claims** - Don't make assumptions about why code exists; document only what you can verify
- **Respect the unknown** - If you don't understand why something is done a certain way, don't guess

## Anti-Patterns to Avoid

**Don't write encyclopedia entries:**
```cpp
// Bad: Technical but not helpful
/**
 * @brief Implements Kahn's algorithm for topological sorting of directed acyclic graphs.
 */

// Good: Explains the practical benefit
/**
 * @brief Returns nodes in topological order (dependencies before dependents).
 * 
 * Useful for dependency graphs where you need to process items in order.
 * Each node in the returned list comes after all its dependencies, making
 * this ideal for build systems, execution ordering, or any dependency-based
 * processing.
 */
```

**Avoid overly casual language:**
```cpp
// Bad: Too casual
/**
 * @brief This is where the magic happens for object updates.
 */

// Good: Clear and professional
/**
 * @brief Core frame update that coordinates all object-related processing.
 */
```

**Don't just repeat the function name:**
```cpp
// Bad: Says nothing useful
/**
 * @brief Gets the node count.
 */

// Good: Explains what the count represents
/**
 * @brief Gets the number of valid nodes in the graph.
 * 
 * @return Count of active, usable nodes. Excludes nodes that have been
 * removed but whose slots haven't been recycled yet.
 */
```

### Language Guide

**Professional alternatives to overly casual phrases:**
- Instead of "This is the thing that..." → "This component handles..."
- Instead of "Where the magic happens" → "Core processing occurs here" or "Primary functionality"
- Instead of "You should probably..." → "Consider using..." or "Typically used for..."
- Instead of "This is a hack" → "This is a workaround for..." or "This addresses a limitation..."
- Instead of "Great for..." → "Useful for..." or "Designed for..."

## Tools and Automation

- Use Doxygen-compatible comments for consistency
- Consider automated checks for missing documentation
- Review documentation during code review with the same rigor as code
- Test that examples actually compile and work

## Example

Here's an example of how LLViewerObjectList is documented:

```cpp
/**
 * @brief Manages all viewer-side objects in the virtual world.
 * 
 * This is the master registry for everything you can see in Second Life - from
 * avatars to prims to particles. It serves as the central hub that tracks,
 * updates, and manages the lifecycle of all objects in your viewing area,
 * keeping the virtual world synchronized between the server and your viewer.
 * 
 * The class handles:
 * - Object creation, updates, and destruction based on server messages
 * - Tracking orphaned objects (children whose parents haven't loaded yet)
 * - Managing active objects that need frequent updates
 * - Caching and fetching object physics and cost data
 * - Providing fast lookups by UUID or local ID
 * 
 * Performance note: This class is on the hot path for frame updates, so many
 * operations are optimized for speed over memory usage.
 */
```

Notice how this:
- Starts with a clear, one-line summary
- Uses concrete descriptions ("master registry") without being overly casual
- Lists specific responsibilities
- Includes performance considerations that matter to developers
- Maintains a professional but approachable tone throughout

## Conclusion

Good documentation is a conversation with future users (including yourself). Write like you're helping someone solve a real problem, not just describing what the code does. The extra effort to make documentation conversational and practical pays dividends in code maintainability and developer productivity.
