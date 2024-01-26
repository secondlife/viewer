# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::lualibs INTERFACE IMPORTED )

use_system_binary( lualibs )

use_prebuilt_binary(luau)

target_include_directories( ll::lualibs SYSTEM INTERFACE
                            ${LIBS_PREBUILT_DIR}/include
)

if (WINDOWS)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/Luau.Ast.lib)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/Luau.CodeGen.lib)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/Luau.Compiler.lib)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/Luau.Config.lib)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/Luau.VM.lib)
endif (WINDOWS)
