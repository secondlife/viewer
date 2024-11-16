# -*- cmake -*-

include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::meshoptimizer INTERFACE IMPORTED )

use_system_binary(meshoptimizer)
use_prebuilt_binary(meshoptimizer)

if (WINDOWS)
  target_link_libraries( ll::meshoptimizer INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/meshoptimizer.lib)
elseif (LINUX)
  target_link_libraries( ll::meshoptimizer INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libmeshoptimizer.a)
elseif (DARWIN)
  target_link_libraries( ll::meshoptimizer INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libmeshoptimizer.a)
endif (WINDOWS)

target_include_directories( ll::meshoptimizer SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/meshoptimizer)
