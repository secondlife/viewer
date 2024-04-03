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
else ()
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libLuau.CodeGen.a)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libLuau.Compiler.a)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libLuau.Config.a)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libLuau.VM.a)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libLuau.Ast.a)
endif ()
