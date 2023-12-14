# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::lualibs INTERFACE IMPORTED )

use_system_binary( lualibs )

use_prebuilt_binary(lua)

target_include_directories( ll::lualibs SYSTEM INTERFACE
                            ${LIBS_PREBUILT_DIR}/include
)

if (WINDOWS)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/lua54.lib)
elseif (DARWIN)
  target_link_libraries(ll::lualibs INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/liblua.a)
elseif (LINUX)
endif (WINDOWS)
