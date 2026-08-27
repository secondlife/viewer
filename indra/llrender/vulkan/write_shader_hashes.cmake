# Build-local identities are deliberately generated after validation and
# reflection. Shader compiler versions are not pinned, so these are evidence
# for one build rather than source-controlled canonical hashes.

foreach(required_variable IN ITEMS VERTEX_MODULE FRAGMENT_MODULE OUTPUT_FILE)
  if (NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
    message(FATAL_ERROR "write_shader_hashes.cmake requires ${required_variable}")
  endif ()
endforeach ()

if (NOT EXISTS "${VERTEX_MODULE}" OR NOT EXISTS "${FRAGMENT_MODULE}")
  message(FATAL_ERROR "shader modules must exist before hashing")
endif ()

file(SHA256 "${VERTEX_MODULE}" vertex_hash)
file(SHA256 "${FRAGMENT_MODULE}" fragment_hash)
file(WRITE "${OUTPUT_FILE}.tmp"
     "vertex ${vertex_hash}\nfragment ${fragment_hash}\n")
file(RENAME "${OUTPUT_FILE}.tmp" "${OUTPUT_FILE}")
