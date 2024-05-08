# Linden Lab GLTF Implementation
 
Currently in prototype stage.  Much functionality is missing (blend shapes, 
multiple texture coordinates, etc).

GLTF Specification can be found here: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
If this implementation disagrees with the GLTF Specification, the specification is correct.

Class structure and naming should match the GLTF Specification as closely as possible while
conforming to the LL coding standards.  All code in headers should be contained in the 
LL::GLTF namespace.

The implementation serves both the client and the server.

# Design Principles

The implementation MUST be capable of round-trip serialization with no data loss beyond F64 to F32 conversions.
The implementation MUST use the same indexing scheme as the GLTF specification.  Do not store pointers where the
GLTF specification stores indices, store indices.
Limit dependencies on llcommon as much as possible.  Prefer std::, boost::, and (soon) glm:: over LL facsimiles.
Usage of LLSD is forbidden in the LL::GLTF namespace. 
Use "using namespace" liberally in .cpp files, but never in .h files.
"using Foo = Bar" is permissible in .h files within the LL::GLTF namespace.

# Loading, Copying, and Serialization
Each class should provide two functions (Primitive shown for example):

```
// Serialize to the provided json object.
// "obj" should be "this" in json form on return
// Do not serialize default values
void serialize(boost::json::object& obj) const;

// Initialize from a provided json value
const Primitive& operator=(const Value& src);
```

Most of the serialization implementation exists in buffer_util.h

NEVER include buffer_util.h from a header.  


Loading from and saving to disk (import/export) is currently done using tinygltf, but this is not a long term
solution.  Eventually the implementation should rely solely on boost::json for reading and writing .gltf
files and should handle .bin files natively.  

When serializing Images and Buffers to the server, clients MUST store a single UUID "uri" field and nothing else.
The server MUST reject any data that violates this requirement.

Clients MUST remove any Images from Buffers prior to upload to the server.
Servers MAY reject Assets that contain Buffers with unreferenced data.



 

