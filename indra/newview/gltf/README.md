# Linden Lab GLTF Implementation

Currently in prototype stage.  Much functionality is missing (blend shapes,
multiple texture coordinates, etc).

GLTF Specification can be found here: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html.
If this implementation disagrees with the GLTF Specification, the specification is correct.

Class structure and naming should match the GLTF Specification as closely as possible while
conforming to the LL coding standards.  All code in headers should be contained in the
LL::GLTF namespace.

The implementation serves both the client and the server.

## Design Principles

- The implementation MUST be capable of round-trip serialization with no data loss beyond F64 to F32 conversions.
- The implementation MUST use the same indexing scheme as the GLTF specification.  Do not store pointers where the
- GLTF specification stores indices, store indices.
- Limit dependencies on llcommon as much as possible.  Prefer std::, boost::, and (soon) glm:: over LL facsimiles.
- Usage of LLSD is forbidden in the LL::GLTF namespace.
- Use "using namespace" liberally in .cpp files, but never in .h files.
- "using Foo = Bar" is permissible in .h files within the LL::GLTF namespace.

## Loading, Copying, and Serialization
Each class should provide two functions (Primitive shown for example):

```
// Serialize to the provided json object.
// "obj" should be "this" in json form on return
// Do not serialize default values
void serialize(boost::json::object& obj) const;

// Initialize from a provided json value
const Primitive& operator=(const Value& src);
```

"serialize" implementations should use "write":

```
void Primitive::serialize(boost::json::object& dst) const
{
    write(mMaterial, "material", dst, -1);
    write(mMode, "mode", dst, TINYGLTF_MODE_TRIANGLES);
    write(mIndices, "indices", dst, INVALID_INDEX);
    write(mAttributes, "attributes", dst);
}
```

And operator= implementations should use "copy":

```
const Primitive& Primitive::operator=(const Value& src)
{
    if (src.is_object())
    {
        copy(src, "material", mMaterial);
        copy(src, "mode", mMode);
        copy(src, "indices", mIndices);
        copy(src, "attributes", mAttributes);

        mGLMode = gltf_mode_to_gl_mode(mMode);
    }
    return *this;
}
```

Parameters to "write" and "copy" MUST be ordered "src" before "dst"
so the code reads as "write src to dst" and "copy src to dst".

When reading string constants from GLTF json (i.e. "OPAQUE", "TRIANGLES"), these
strings should be converted to enums inside operator=.  It is permissible to
store the original strings during prototyping to aid in development, but eventually
we'll purge these strings from the implementation.  However, implementations MUST
preserve any and all "name" members.

"write" and "copy" implementations MUST be stored in buffer_util.h.
As implementers encounter new data types, you'll see compiler errors
pointing at templates in buffer_util.h.  See vec3 as a known good
example of how to add support for a new type (there are bad examples, so beware):

```
// vec3
template<>
inline bool copy(const Value& src, vec3& dst)
{
    if (src.is_array())
    {
        const boost::json::array& arr = src.as_array();
        if (arr.size() == 3)
        {
            if (arr[0].is_double() &&
                arr[1].is_double() &&
                arr[2].is_double())
            {
                dst = vec3(arr[0].get_double(), arr[1].get_double(), arr[2].get_double());
            }
            return true;
        }
    }
    return false;
}

template<>
inline bool write(const vec3& src, Value& dst)
{
    dst = boost::json::array();
    boost::json::array& arr = dst.as_array();
    arr.resize(3);
    arr[0] = src.x;
    arr[1] = src.y;
    arr[2] = src.z;
    return true;
}

```

"write" MUST return true if ANY data was written
"copy" MUST return true if ANY data was copied

Speed is important, but so is safety.  In writers, try to avoid redundant copies
(prefer resize over push_back, convert dst to an empty array and fill it, don't
make an array on the stack and copy it into dst).

boost::json WILL throw exceptions if you call as_foo() on a mismatched type but
WILL NOT throw exceptions on get_foo with a mismatched type.  ALWAYS check is_foo
before calling as_foo or get_foo.  DO NOT add exception handlers.  If boost throws
an exception in serialization, the fix is to add type checks.  If we see a large
number of crash reports from boost::json exceptions, each of those reports
indicates a place where we're missing "is_foo" checks.  They are gold.  Do not
bury them with an exception handler.

DO NOT rely on existing type conversion tools in the LL codebase -- LL data models
conflict with the GLTF specification so we MUST provide conversions independent of
our existing implementations.

### JSON Serialization ###



NEVER include buffer_util.h from a header.

Loading from and saving to disk (import/export) is currently done using tinygltf, but this is not a long term
solution.  Eventually the implementation should rely solely on boost::json for reading and writing .gltf
files and should handle .bin files natively.

When serializing Images and Buffers to the server, clients MUST store a single UUID "uri" field and nothing else.
The server MUST reject any data that violates this requirement.

Clients MUST remove any Images from Buffers prior to upload to the server.
Servers MAY reject Assets that contain Buffers with unreferenced data.

... to be continued.



